# T-Display-CM0
board base CM0_Lite

T-Display-CM0 是面向 **LILYGO T-Display-CM0（基于 CM0_Lite 底板）** 的 Linux 硬件支持仓库，包含板载外设的内核驱动、Device Tree Overlay 以及用户态示例程序。

> [!IMPORTANT]
> 仓库中预编译的 `.ko` 文件与编译它们时使用的 Linux 内核严格绑定。若目标设备的内核版本或配置不同，请使用与目标系统匹配的内核源码重新编译驱动。

## 功能支持

| 功能 | 芯片 / 接口 | 驱动或配置 | 状态 |
| --- | --- | --- | --- |
| AMOLED 显示 | RM69A10、MIPI DSI、2 Lane、568 x 1232 | `panel_rm69a10` | 已提供驱动与 Overlay |
| 电容触摸 | Goodix GT9895、I2C | `goodix_berlin_core`、`goodix_berlin_i2c` | 已提供驱动，配置包含在显示 Overlay 中 |
| 音频编解码 | ES8388、I2C + I2S | `snd-soc-es8328-i2c` | 已提供驱动与 Overlay |
| 充电管理 | BQ25890 / BQ25896、I2C | `bq25890_charger` | 已提供驱动与 Overlay |
| 电量计 | BQ27220、I2C | `bq27xxx_battery`、`bq27xxx_battery_i2c` | 已提供驱动与 Overlay |
| IMU | QMI8658C、I2C、IIO | `qmi8658c` | 已提供驱动 |
| 键盘矩阵 | TCA8418、7 x 10 Matrix | `tca8418_keypad` | 已提供驱动与 Overlay |
| GPIO 扩展 | XL9555 / PCA9555、I2C | Linux `gpio-pca953x` | 已提供设备树配置 |
| 软件 PWM | GPIO | `pwm-gpio` | 已提供驱动 |
| 板载电源控制 | XL9555 GPIO Hog | USB、音频、相机、LoRa 电源 | 已提供 Overlay |
| 温湿度示例 | AHT20、I2C | `/dev/i2c-1`，地址 `0x38` | 已提供用户态示例 |
| LoRa 示例 | LR2021、SPI + GPIO | `/dev/spidev0.0` | 已提供 GetVersion 示例 |

## 仓库结构

```text
T-Display-CM0/
├── app/
│   ├── sample_aht20/              # AHT20 温湿度读取示例
│   └── sample_lora/               # LR2021 SPI GetVersion 示例
├── cmo_drivers/
│   ├── bq25896/                   # BQ25890/BQ25896 充电驱动
│   ├── bq27220/                   # BQ27220 电量计驱动
│   ├── es8388/                    # ES8388 ASoC 驱动
│   ├── goodix_berlin/             # Goodix GT9895 触摸驱动
│   ├── imu_qmi8658c/              # QMI8658C IIO 驱动
│   ├── panel_rm69a10_amoled/      # RM69A10 AMOLED 面板驱动
│   ├── pwm-gpio/                  # GPIO 软件 PWM 驱动
│   └── tca8418/                   # TCA8418 键盘驱动
├── dts/                           # Device Tree Overlay 源文件
├── LICENSE
└── README.md
```

## 开发环境

以下流程以 Debian / Ubuntu 开发主机和 32 位 ARM 目标系统为例。

```bash
sudo apt update
sudo apt install build-essential gcc-arm-linux-gnueabihf device-tree-compiler
```

还需要准备与目标设备正在运行的内核相匹配的 Linux 内核源码，并完成内核配置和模块编译准备：

```bash
export KERNEL_SRC=/path/to/linux-kernel

make -C "$KERNEL_SRC" ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- olddefconfig
make -C "$KERNEL_SRC" ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules_prepare
```

内核源码中的 `.config`、架构和工具链必须与目标系统保持一致。若使用发行版提供的内核头文件，也可以将 `KERNEL_SRC` 指向对应的 build 目录。

## 编译内核模块

每个驱动目录都带有独立的 `Makefile`。仓库内的 `KDIR` 默认值是开发环境示例路径，推荐在命令行覆盖它：

```bash
export KERNEL_SRC=/path/to/linux-kernel
export CROSS_ARGS="ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-"

driver_dir=cmo_drivers/panel_rm69a10_amoled
make -C "$driver_dir" PWD="$PWD/$driver_dir" KDIR="$KERNEL_SRC" CROSS="$CROSS_ARGS"

# 将 driver_dir 换成其他驱动目录即可分别编译
```

也可以一次编译所有驱动：

```bash
for driver_dir in cmo_drivers/*; do
    [ -f "$driver_dir/Makefile" ] || continue
    make -C "$driver_dir" PWD="$PWD/$driver_dir" KDIR="$KERNEL_SRC" CROSS="$CROSS_ARGS"
done
```

### 安装模块

将目标设备需要的 `.ko` 文件复制到设备后执行：

```bash
sudo mkdir -p "/lib/modules/$(uname -r)/extra/t-display-cm0"
sudo cp /path/to/modules/*.ko "/lib/modules/$(uname -r)/extra/t-display-cm0/"
sudo depmod -a
```

模块通常会在对应的设备树节点生效后自动加载，也可以手动测试。例如：

```bash
sudo modprobe panel_rm69a10
sudo modprobe goodix_berlin_core
sudo modprobe goodix_berlin_i2c
sudo modprobe qmi8658c
```

## 编译 Device Tree Overlay

Overlay 源文件使用了内核的 `dt-bindings` 头文件，因此需要先通过 C 预处理器展开，再交给 `dtc` 编译：

```bash
export KERNEL_SRC=/path/to/linux-kernel
mkdir -p build/dtbo

for overlay in dts/*-overlay.dts; do
    name=$(basename "$overlay" -overlay.dts)
    cpp -P -nostdinc \
        -I "$KERNEL_SRC/include" \
        -I dts \
        -undef -D__DTS__ -x assembler-with-cpp "$overlay" | \
        dtc -@ -I dts -O dtb -o "build/dtbo/$name.dtbo" -
done
```

生成的文件包括：

```text
audio_es8388_codec.dtbo
batt-charger.dtbo
panel_rm69a10_amoled.dtbo
pwr_ctl.dtbo
tca8418-keypad.dtbo
```

将所需的 `.dtbo` 复制到目标系统的 Overlay 目录：

```bash
sudo cp build/dtbo/*.dtbo /boot/firmware/overlays/
```

在较旧的系统中，该目录可能是 `/boot/overlays/`。然后根据实际需要在 `/boot/firmware/config.txt`（旧系统为 `/boot/config.txt`）中启用：

```ini
dtoverlay=pwr_ctl
dtoverlay=panel_rm69a10_amoled
dtoverlay=audio_es8388_codec
dtoverlay=batt-charger
dtoverlay=tca8418-keypad
```

请只启用硬件实际使用的 Overlay。修改启动配置后需要重启设备。

## 用户态示例

### AHT20 温湿度传感器

示例默认使用 `/dev/i2c-1`，I2C 地址为 `0x38`。

在开发主机交叉编译：

```bash
make -C app/sample_aht20
```

也可以直接在目标设备上编译：

```bash
make -C app/sample_aht20 CC=gcc
sudo ./app/sample_aht20/aht20
```

正常输出示例：

```text
Temp:25.30 C  Humidity:56.20 %RH
```

### LR2021 LoRa

示例使用以下接口：

| 信号 | 资源 |
| --- | --- |
| SPI | `/dev/spidev0.0` |
| RST | GPIO 7 |
| BUSY | GPIO 24 |
| CS | GPIO 8（SPI0 CE0） |
| SCLK / MOSI / MISO | GPIO 11 / 10 / 9 |

编译并运行：

```bash
make -C app/sample_lora

# 或者在目标设备上本地编译
make -C app/sample_lora CC=gcc
sudo ./app/sample_lora/lora
```

该示例通过 Linux GPIO v2 API 和 `spidev` 与 LR2021 通信，并读取芯片固件版本。

## 运行状态检查

```bash
# 查看已加载模块
lsmod

# 查看驱动探测日志
dmesg | tail -n 100

# 查看 I2C 总线设备（需要安装 i2c-tools）
sudo i2cdetect -y 1

# 查看输入设备（触摸和键盘）
cat /proc/bus/input/devices

# 查看音频设备
aplay -l
arecord -l

# 查看电源与电池
ls /sys/class/power_supply/

# 查看 QMI8658C IIO 设备
ls /sys/bus/iio/devices/
```

## 常见问题

### `Exec format error` 或 `Invalid module format`

模块的架构、内核版本或内核配置与目标设备不一致。请使用目标设备对应的内核源码和配置重新编译，并通过以下命令比较版本：

```bash
uname -r
modinfo your_driver.ko | grep vermagic
```

### 找不到 `/dev/i2c-1` 或 `/dev/spidev0.0`

请确认系统已经启用对应的 I2C / SPI 控制器，并检查启动配置和设备树是否正确加载。

### 模块已加载，但设备没有出现

加载模块只会注册驱动；还需要 Device Tree 中存在 `compatible`、I2C 地址、GPIO 和中断等匹配的设备节点。优先检查 `dmesg` 和当前生效的设备树。

## 贡献

欢迎通过 Issue 或 Pull Request 提交问题、硬件适配、驱动修复和示例程序。提交问题时建议附上：

- 板卡版本和外设型号
- Linux 内核版本（`uname -a`）
- 完整的 `dmesg` 相关日志
- 使用的 Overlay 和复现步骤

## 许可证

仓库根目录使用 [MIT License](LICENSE)。部分 Linux 内核驱动源文件带有独立的 SPDX 标识、版权声明或 GPL 模块许可证；这些文件以其各自声明为准。
