#include "wav_player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

// 播放缓冲区大小（字节）
#define PLAY_BUFFER_SIZE 4096

// 播放器内部结构体
struct WavPlayer {
    FILE*          fp;             // WAV文件指针
    // 解析后的音频参数
    uint32_t       sample_rate;    // 采样率
    uint16_t       num_channels;   // 声道数
    uint16_t       bits_per_sample;// 采样位深
    uint16_t       block_align;    // 每帧字节数 = 声道数*位深/8
    uint32_t       data_offset;    // PCM数据在文件中的起始偏移
    uint32_t       data_size;      // PCM数据总字节数
    uint32_t       remain_size;    // 剩余未播放的PCM字节数

    snd_pcm_t*     pcm_handle;     // ALSA播放设备句柄
    bool           hw_pause_sup;   // 硬件暂停是否支持

    pthread_t      thread_id;      // 播放线程ID
    pthread_mutex_t mutex;         // 状态互斥锁
    pthread_cond_t  cond;          // 状态同步条件变量
    WavPlayerState state;          // 当前播放状态
    bool           thread_running; // 线程运行标志

    uint8_t*       buffer;         // 播放数据缓冲区
    size_t         buffer_size;    // 缓冲区大小
};

// 小端字节序转换（不依赖系统头文件，全平台通用）
static uint16_t le16_to_h(const uint8_t* buf) {
    return (uint16_t)(buf[0] | (buf[1] << 8));
}
static uint32_t le32_to_h(const uint8_t* buf) {
    return (uint32_t)(buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
}

/**
 * 逐子块解析WAV文件头部，填充播放器参数
 * 返回0成功，-1失败
 */
static int wav_parse_header(WavPlayer* player)
{
    uint8_t buf[12];
    uint8_t subchunk_id[4];
    uint32_t subchunk_size;

    // 读取RIFF主头
    if (fread(buf, 1, 12, player->fp) != 12) {
        printf("WAV ERROR: file too short\n");
        return -1;
    }
    if (memcmp(buf, "RIFF", 4) != 0) {
        printf("WAV ERROR: not RIFF format\n");
        return -1;
    }
    if (memcmp(buf + 8, "WAVE", 4) != 0) {
        printf("WAV ERROR: not WAVE format\n");
        return -1;
    }

    bool fmt_found = false;

    // 循环遍历所有子块
    while (1) {
        // 读取子块头
        if (fread(subchunk_id, 1, 4, player->fp) != 4) {
            printf("WAV ERROR: read subchunk id failed\n");
            return -1;
        }
        if (fread(buf, 1, 4, player->fp) != 4) {
            printf("WAV ERROR: read subchunk size failed\n");
            return -1;
        }
        subchunk_size = le32_to_h(buf);
        long subchunk_data_pos = ftell(player->fp);

        // 处理fmt格式块
        if (memcmp(subchunk_id, "fmt ", 4) == 0) {
            if (subchunk_size < 16) {
                printf("WAV ERROR: fmt subchunk too small\n");
                return -1;
            }
            uint8_t fmt_buf[16];
            if (fread(fmt_buf, 1, 16, player->fp) != 16) {
                printf("WAV ERROR: read fmt data failed\n");
                return -1;
            }

            uint16_t audio_format = le16_to_h(fmt_buf);
            if (audio_format != 1) {
                printf("WAV ERROR: not PCM format (format=%d)\n", audio_format);
                return -1;
            }

            player->num_channels = le16_to_h(fmt_buf + 2);
            player->sample_rate = le32_to_h(fmt_buf + 4);
            player->bits_per_sample = le16_to_h(fmt_buf + 14);
            player->block_align = player->num_channels * player->bits_per_sample / 8;

            if (player->bits_per_sample != 16) {
                printf("WAV ERROR: only support 16bit (current=%d)\n", player->bits_per_sample);
                return -1;
            }

            fmt_found = true;
            // 跳过fmt块剩余扩展字节
            if (subchunk_size > 16) {
                fseek(player->fp, subchunk_size - 16, SEEK_CUR);
            }
        }
        // 处理data数据块，解析完成
        else if (memcmp(subchunk_id, "data", 4) == 0) {
            if (!fmt_found) {
                printf("WAV ERROR: data chunk before fmt chunk\n");
                return -1;
            }
            player->data_offset = subchunk_data_pos;
            player->data_size = subchunk_size;
            player->remain_size = subchunk_size;
            return 0; // 解析成功
        }
        // 其他子块（LIST/INFO等）直接跳过
        else {
            fseek(player->fp, subchunk_size, SEEK_CUR);
        }

        // RIFF规范：奇数长度子块末尾有1字节填充
        if (subchunk_size % 2 != 0) {
            fseek(player->fp, 1, SEEK_CUR);
        }
    }

    printf("WAV ERROR: no data subchunk found\n");
    return -1;
}

// 播放线程主函数
static void* play_thread_func(void* arg)
{
    WavPlayer* player = (WavPlayer*)arg;

    while (player->thread_running) {
        // 等待播放状态
        pthread_mutex_lock(&player->mutex);
        while (player->state != WAV_PLAYER_PLAYING && player->thread_running) {
            pthread_cond_wait(&player->cond, &player->mutex);
        }
        if (!player->thread_running) {
            pthread_mutex_unlock(&player->mutex);
            break;
        }
        pthread_mutex_unlock(&player->mutex);

        // 计算本次读取大小，不超过剩余数据
        size_t read_len = player->buffer_size;
        if (read_len > player->remain_size) {
            read_len = player->remain_size;
        }
        if (read_len == 0) {
            // 播放自然结束
            pthread_mutex_lock(&player->mutex);
            player->state = WAV_PLAYER_STOPPED;
            player->thread_running = false;
            pthread_mutex_unlock(&player->mutex);
            break;
        }

        // 读取PCM数据
        size_t bytes_read = fread(player->buffer, 1, read_len, player->fp);
        if (bytes_read == 0) {
            // 文件读取出错或读完
            pthread_mutex_lock(&player->mutex);
            player->state = WAV_PLAYER_STOPPED;
            player->thread_running = false;
            pthread_mutex_unlock(&player->mutex);
            break;
        }

        // 循环写入ALSA，处理部分写入和错误恢复
        snd_pcm_uframes_t total_frames = bytes_read / player->block_align;
        snd_pcm_uframes_t remain_frames = total_frames;
        const uint8_t* data_ptr = player->buffer;
        int write_err = 0;

        while (remain_frames > 0) {
            snd_pcm_sframes_t written = snd_pcm_writei(player->pcm_handle, data_ptr, remain_frames);
            if (written == -EAGAIN) {
                continue;
            }
            if (written < 0) {
                // 欠载、同步错误等自动恢复
                written = snd_pcm_recover(player->pcm_handle, (int)written, 0);
                if (written < 0) {
                    write_err = -1;
                    break;
                }
                continue;
            }
            data_ptr += written * player->block_align;
            remain_frames -= written;
        }

        if (write_err < 0) {
            pthread_mutex_lock(&player->mutex);
            player->state = WAV_PLAYER_STOPPED;
            player->thread_running = false;
            pthread_mutex_unlock(&player->mutex);
            break;
        }

        // 更新剩余数据量
        player->remain_size -= bytes_read;
    }

    return NULL;
}

WavPlayer* wav_player_create(const char* wav_path, const char* pcm_device)
{
    if (!wav_path || !pcm_device) return NULL;

    WavPlayer* player = (WavPlayer*)calloc(1, sizeof(WavPlayer));
    if (!player) return NULL;

    // 1. 打开并解析WAV文件
    player->fp = fopen(wav_path, "rb");
    if (!player->fp) goto fail;

    if (wav_parse_header(player) != 0) {
        goto fail;
    }

    // 2. 打开ALSA播放设备
    int ret = snd_pcm_open(&player->pcm_handle, pcm_device, SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) {
        printf("ALSA ERROR: open device failed: %s\n", snd_strerror(ret));
        goto fail;
    }

    // 3. 配置硬件参数
    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(player->pcm_handle, params);
    snd_pcm_hw_params_set_access(player->pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(player->pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(player->pcm_handle, params, player->num_channels);

    unsigned int rate = player->sample_rate;
    ret = snd_pcm_hw_params_set_rate_near(player->pcm_handle, params, &rate, NULL);
    if (ret < 0) {
        printf("ALSA ERROR: set sample rate failed\n");
        goto fail;
    }
    // 校验实际采样率偏差不超过1%
    if (abs((int)rate - (int)player->sample_rate) > (int)player->sample_rate / 100) {
        printf("ALSA WARNING: sample rate mismatch: file=%d, device=%d\n",
               player->sample_rate, rate);
    }

    snd_pcm_uframes_t period_size = 1024;
    snd_pcm_hw_params_set_period_size_near(player->pcm_handle, params, &period_size, NULL);
    // 设置缓冲区大小为4个周期，平衡延迟和抗欠载能力
    snd_pcm_uframes_t buffer_size = period_size * 4;
    snd_pcm_hw_params_set_buffer_size_near(player->pcm_handle, params, &buffer_size);

    ret = snd_pcm_hw_params(player->pcm_handle, params);
    if (ret < 0) {
        printf("ALSA ERROR: set hw params failed: %s\n", snd_strerror(ret));
        goto fail;
    }

    // 4. 正确检测硬件暂停支持性
    player->hw_pause_sup = snd_pcm_hw_params_can_pause(params);

    // 5. 初始化线程同步对象
    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);

    // 6. 分配播放缓冲区
    player->buffer_size = PLAY_BUFFER_SIZE;
    player->buffer = (uint8_t*)malloc(player->buffer_size);
    if (!player->buffer) goto fail;

    player->state = WAV_PLAYER_STOPPED;
    player->thread_running = false;
    return player;

fail:
    if (player->buffer) free(player->buffer);
    if (player->pcm_handle) snd_pcm_close(player->pcm_handle);
    if (player->fp) fclose(player->fp);
    free(player);
    return NULL;
}

bool wav_player_play(WavPlayer* player)
{
    if (!player) return false;

    // 若正在播放/暂停，先停止再重新开始
    if (player->state != WAV_PLAYER_STOPPED) {
        wav_player_stop(player);
    }

    // 重置文件指针到PCM数据起始位置，重置剩余数据
    fseek(player->fp, player->data_offset, SEEK_SET);
    player->remain_size = player->data_size;

    // 重置ALSA设备状态
    snd_pcm_prepare(player->pcm_handle);

    pthread_mutex_lock(&player->mutex);
    player->state = WAV_PLAYER_PLAYING;
    player->thread_running = true;
    pthread_mutex_unlock(&player->mutex);

    // 启动播放线程
    if (pthread_create(&player->thread_id, NULL, play_thread_func, player) != 0) {
        player->thread_running = false;
        return false;
    }

    return true;
}

bool wav_player_pause(WavPlayer* player)
{
    if (!player || player->state != WAV_PLAYER_PLAYING) return false;

    pthread_mutex_lock(&player->mutex);
    if (player->hw_pause_sup) {
        snd_pcm_pause(player->pcm_handle, 1);
    }
    player->state = WAV_PLAYER_PAUSED;
    pthread_mutex_unlock(&player->mutex);

    return true;
}

bool wav_player_resume(WavPlayer* player)
{
    if (!player || player->state != WAV_PLAYER_PAUSED) return false;

    pthread_mutex_lock(&player->mutex);
    if (player->hw_pause_sup) {
        snd_pcm_pause(player->pcm_handle, 0);
    }
    player->state = WAV_PLAYER_PLAYING;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);

    return true;
}

void wav_player_stop(WavPlayer* player)
{
    if (!player) return;

    pthread_mutex_lock(&player->mutex);
    player->state = WAV_PLAYER_STOPPED;
    player->thread_running = false;
    pthread_mutex_unlock(&player->mutex);

    // 立即丢弃ALSA缓冲区数据，强制唤醒阻塞的writei
    snd_pcm_drop(player->pcm_handle);
    // 唤醒可能阻塞在条件变量的线程
    pthread_cond_signal(&player->cond);

    // 等待线程安全退出，避免重复join
    if (player->thread_id != 0) {
        pthread_join(player->thread_id, NULL);
        player->thread_id = 0;
    }
}

WavPlayerState wav_player_get_state(WavPlayer* player)
{
    if (!player) return WAV_PLAYER_STOPPED;

    pthread_mutex_lock(&player->mutex);
    WavPlayerState state = player->state;
    pthread_mutex_unlock(&player->mutex);

    return state;
}

void wav_player_destroy(WavPlayer* player)
{
    if (!player) return;

    // 先停止播放，确保线程退出
    wav_player_stop(player);

    free(player->buffer);
    pthread_mutex_destroy(&player->mutex);
    pthread_cond_destroy(&player->cond);
    snd_pcm_close(player->pcm_handle);
    fclose(player->fp);
    free(player);
}
