#ifndef WAV_PLAYER_H
#define WAV_PLAYER_H

#include <stdbool.h>

// 播放器不透明结构体
typedef struct WavPlayer WavPlayer;

// 播放器状态
typedef enum {
    WAV_PLAYER_STOPPED = 0,  // 停止/未播放
    WAV_PLAYER_PLAYING,      // 正在播放
    WAV_PLAYER_PAUSED        // 已暂停
} WavPlayerState;

/**
 * @brief 创建播放器实例
 * @param wav_path   WAV文件路径，仅支持16位线性PCM格式
 * @param pcm_device ALSA设备名，如 "plughw:1,0"（ES8388模拟输出）
 * @return 成功返回播放器句柄，失败返回NULL
 */
WavPlayer* wav_player_create(const char* wav_path, const char* pcm_device);

/**
 * @brief 开始播放（从头开始）
 * @note 若正在播放/暂停，调用后会重新从头播放
 */
bool wav_player_play(WavPlayer* player);

/**
 * @brief 暂停播放
 */
bool wav_player_pause(WavPlayer* player);

/**
 * @brief 从暂停处恢复播放
 */
bool wav_player_resume(WavPlayer* player);

/**
 * @brief 停止播放
 */
void wav_player_stop(WavPlayer* player);

/**
 * @brief 获取当前播放状态
 */
WavPlayerState wav_player_get_state(WavPlayer* player);

/**
 * @brief 销毁播放器，释放所有资源（文件、ALSA设备、线程、内存）
 */
void wav_player_destroy(WavPlayer* player);

#endif
