<h1 align="center">T-Display-CM0</h1>

<p align="center">
  <img src="image/product_raspberry.jpg" alt="LILYGO T-Display-CM0" width="600">
</p>

<p align="center">
  <strong>简体中文</strong> | <a href="README_EN.md">English</a>
</p>

T-Display-CM0 是基于 **CM0_Lite** 底板的 Linux 智能终端开发项目，集成 AMOLED 触摸屏，并支持相机、音频、LoRa、电池、键盘、GNSS、LTE-M、温湿度传感器等扩展模块。

本仓库提供可直接部署到 Raspberry Pi OS 的资源、首次启动脚本、LVGL 桌面程序、Linux 驱动、Device Tree Overlay 和硬件资料。

> [!IMPORTANT]
> 首次使用建议从 [快速开始](#快速开始) 按顺序操作。首次启动会自动安装驱动和应用，并可能重启多次，请勿中途断电。

## 目录

- [产品简介](#产品简介)
- [仓库结构](#仓库结构)
- [快速开始](#快速开始)
- [基本操作](#基本操作)
- [功能检测](#功能检测)
- [桌面模式切换](#桌面模式切换)
- [常见问题](#常见问题)
- [开发说明](#开发说明)
- [相关资料](#相关资料)

## 产品简介

### 主要功能

| 功能 | 芯片 / 接口 | 说明 |
| --- | --- | --- |
| AMOLED 显示 | RM69A10、MIPI DSI | 分辨率 568 x 1232 |
| 电容触摸 | Goodix GT9895、I2C | 多点触摸输入 |
| 相机 | CSI-2 | 桌面 Camera 应用预览 |
| 音频 | ES8388 / ES8389、I2S | 耳机、扩展板扬声器 |
| LoRa | LR2021、SPI | 数据发送和接收 |
| 电池 | BQ25890 / BQ25896、BQ27220 | 充放电与电量信息 |
| IMU | QMI8658C、I2C | 加速度、角速度和温度 |
| 键盘 | TCA8418 | 7 x 10 键盘矩阵 |
| 温湿度 | AHT20、I2C | 温度和湿度显示 |
| GNSS | 扩展模块 | 定位、经纬度和 UTC 时间 |
| 蜂窝网络 | LTE-M 扩展模块 | SIM 信息和 UTC 时间 |

### 电源与按键

- 两个 Type-C 接口均支持 5 V 供电。
- 短按开关键约 1 秒开机。
- 长按开关键约 3 秒关机。
- 正常上电时，板载绿色指示灯点亮，蓝色指示灯闪烁。
- 屏幕显示或触摸异常时，可按板侧下方的 `RST` 键重启。

## 仓库结构

```text
T-Display-CM0/
├── deploy/
│   ├── cm0_resource_collect/     # 首次启动和自动部署资源
│   ├── desktop_alt/              # LVGL / Raspberry Pi 桌面选择
│   ├── rasp_img/                 # Raspberry Pi OS 镜像下载说明
│   └── T-Display-CM0...pdf       # 中文使用说明书
├── src/
│   ├── app/                      # 桌面程序与外设示例
│   ├── cmo_drivers/              # Linux 内核驱动
│   └── dts/                      # Device Tree Overlay 源文件
├── image/                        # README 图片
├── datasheet/                    # 产品规格书
├── schematic/                    # 原理图
├── LICENSE
└── README.md
```

## 快速开始

### 1. 准备工作

需要准备：

- T-Display-CM0 主板、屏幕及需要使用的扩展模块
- TF 卡和读卡器
- 支持数据传输的 USB 转 Type-C 线
- Windows 电脑
- [Rufus](https://rufus.ie/downloads/)
- [Raspberry Pi OS 32-bit 镜像](https://downloads.raspberrypi.com/raspios_armhf/images/raspios_armhf-2025-10-02/2025-10-01-raspios-trixie-armhf.img.xz)

本项目部署资源按照 `2025-10-01-raspios-trixie-armhf.img.xz` 准备。若更换系统版本，预编译驱动和应用可能不兼容。

### 2. 选择桌面模式

仓库默认使用 **LVGL 桌面**。如不需要切换，可直接进入下一步。

| 模式 | 用途 | 首次启动脚本 |
| --- | --- | --- |
| LVGL | T-Display-CM0 触摸应用桌面，默认选项 | `deploy/desktop_alt/desktop_lvgl/firstrun.sh` |
| Raspberry Pi Desktop | 标准 Raspberry Pi 图形桌面 | `deploy/desktop_alt/desktop_raspberry/firstrun.sh` |

需要切换时，在烧录前将所选模式的 `firstrun.sh` 复制到 `deploy/cm0_resource_collect/`，覆盖其中的同名文件。

### 3. 烧录系统镜像

1. 将 TF 卡插入电脑并启动 Rufus。
2. 在“设备”中选择 TF 卡。
3. 点击“选择”，打开 `2025-10-01-raspios-trixie-armhf.img.xz`。
4. 点击“开始”，确认写入并等待进度完成。
5. Rufus 显示“准备就绪”后，**暂时不要拔出 TF 卡**，下一步还需要访问 `bootfs` 分区。

> [!WARNING]
> Rufus 会清除所选存储设备上的数据。开始烧录前务必确认选择的是 TF 卡，而不是电脑硬盘或其他移动设备。

### 4. 部署 T-Display-CM0 资源

打开仓库中的：

```text
deploy/cm0_resource_collect/
```

将该目录内的**全部文件和文件夹**复制到 TF 卡的 `bootfs` 根目录。至少应包含：

```text
bootfs/
├── lilygo_resources/
├── append_firstrun_to_cmdline.bat
├── config.txt
├── firstrun.sh
└── firstrun.txt
```

如系统提示文件已存在，按部署包内容覆盖。

复制完成后，双击 `bootfs` 中的：

```text
append_firstrun_to_cmdline.bat
```

命令窗口可能一闪而过，这是正常现象。该脚本会把首次启动参数追加到 `cmdline.txt`。执行后确认 `bootfs` 中仍存在 `cmdline.txt`、`firstrun.sh` 和 `lilygo_resources`。

### 5. 首次启动

1. 在 Windows 中安全弹出 TF 卡。
2. 将 TF 卡插入 T-Display-CM0。
3. 确认屏幕、摄像头和扩展模块连接正确。
4. 使用 Type-C 线连接 5 V 电源。
5. 按开关键约 1 秒。
6. 绿色指示灯点亮、蓝色指示灯闪烁后，等待自动初始化完成。

首次初始化通常需要约 **2–3 分钟**。设备会创建用户、启用 SSH、安装驱动和 Overlay、部署应用与服务，并自动重启。此期间请勿拔出 TF 卡或断开电源。

默认登录信息：

| 项目 | 默认值 |
| --- | --- |
| 用户名 | `pi` |
| 密码 | `raspberry` |
| SSH | 首次部署时自动启用 |

首次登录后建议立即执行 `passwd` 修改默认密码。

初始化完成的判断方式：

- 默认 LVGL 模式进入 T-Display-CM0 应用桌面；或
- Raspberry Pi Desktop 模式进入图形桌面；或
- 串口终端显示登录提示。

## 基本操作

| 操作 | 方法 |
| --- | --- |
| 开机 | 按开关键约 1 秒 |
| 关机 | 长按开关键约 3 秒 |
| 复位 | 按板侧下方 `RST` 键 |
| 供电 | 任一 Type-C 接口接入 5 V |
| 串口调试 | 连接板载串口接口，在首次启动时查看初始化日志 |

## 功能检测

### Camera

点击桌面的 **Camera** 图标，默认打开顶部 CSI-2 摄像头并显示预览画面。

### Music

插入耳机，或连接带扬声器的扩展板。点击 **Music** 图标，再点击播放按钮，设备将播放测试音频。

### LoRa

LoRa 收发测试至少需要两台设备：

1. 第一台设备打开 **LoRa**，点击 **Send**，定时发送数据。
2. 第二台设备打开 **LoRa**，点击 **Receive**，定时接收数据。
3. 对比发送端和接收端不断更新的数据；内容对应即表示通信成功。

如需重新开始发送或接收，退出 LoRa 应用后重新进入。

### Battery

在扩展板上正确安装两节 21700 电池并打开电池开关。点击 **Battery** 图标，可查看充放电电压、电流、电量和状态。

### Weather

连接扩展板上的 AHT20 温湿度传感器，点击 **Weather** 图标，可查看当前温度和湿度。

### Keyboard

正确安装键盘扩展板后，点击 **Keyboard** 图标：

- `Shift + 字符`：输入大写或上档字符
- `Caps Lock`：锁定或取消大写
- `Fn + F4` / `Fn + F5`：控制键盘背光

### GNSS

在室外或空旷区域打开 **GNSS** 应用，点击 **Locate** 开始定位。初次定位约需 120 秒，定位完成后会显示经纬度和卫星 UTC 时间。

### Cellular

插入已开通 LTE-M 服务的物联网 SIM 卡，打开 **Cellular** 应用。界面会显示读取到的 SIM 信息并同步 UTC 时间。

## 桌面模式切换

推荐在烧录和首次启动前选择桌面模式。

切换到 Raspberry Pi Desktop：

```text
将 deploy/desktop_alt/desktop_raspberry/firstrun.sh
覆盖到 deploy/cm0_resource_collect/firstrun.sh
```

切换回 LVGL 桌面：

```text
将 deploy/desktop_alt/desktop_lvgl/firstrun.sh
覆盖到 deploy/cm0_resource_collect/firstrun.sh
```

替换脚本后，重新执行完整的烧录与首次启动流程。

## 常见问题

### 首次启动后长时间没有画面

1. 等待至少 3 分钟，首次部署期间设备可能自动重启。
2. 检查绿色灯是否点亮、蓝色灯是否闪烁。
3. 确认 `cm0_resource_collect` 中的所有内容都已复制到 `bootfs` 根目录。
4. 确认已双击执行 `append_firstrun_to_cmdline.bat`。
5. 检查屏幕排线、TF 卡和 5 V 供电。
6. 连接串口查看启动日志。

首次部署日志通常位于：

```text
/boot/firmware/firstrun-lilygo.log
/boot/firmware/lilygo_resources/install_lilygo_cm0_lite.log
```

也可以在系统中执行：

```bash
journalctl -u lilygo-firstboot-install.service
```

### 屏幕内容异常或触摸没有响应

按板侧下方 `RST` 键重启；若仍未恢复，断电后重新检查屏幕和触摸排线，再重新上电。

### 无法通过 SSH 登录

确认首次初始化已经完成，并使用默认账号 `pi` / `raspberry`。检查设备和电脑是否处于同一网络，然后通过串口或路由器确认设备 IP 地址。

### 驱动提示 `Invalid module format`

预编译 `.ko` 必须与当前 Linux 内核版本和配置匹配。若更换了系统镜像或升级了内核，请使用对应的内核源码重新编译驱动。

## 开发说明

普通用户使用 `deploy/cm0_resource_collect` 即可，不需要手动编译内核。

### LVGL 桌面应用

桌面应用源码位于 `src/app/sample_display/`，默认输出为 `sample_display.out`。交叉编译需要 32 位 ARM 工具链以及与目标系统匹配的 sysroot：

```bash
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf make
make -C src/app/sample_display RPI_SYSROOT=/path/to/rpi_sysroot
```

编译完成后，可将程序更新到部署资源中的：

```text
deploy/cm0_resource_collect/lilygo_resources/app/lilygo_desktop_app/sample_display.out
```

### 外设示例

- `src/app/sample_aht20/`：通过 `/dev/i2c-1` 读取 AHT20
- `src/app/sample_lora/`：通过 `/dev/spidev0.0` 和 GPIO 访问 LR2021

### 内核驱动与设备树

- 内核驱动源码：`src/cmo_drivers/`
- Overlay 源码：`src/dts/`
- 已部署模块：`deploy/cm0_resource_collect/lilygo_resources/drivers/`
- 已部署 Overlay：`deploy/cm0_resource_collect/lilygo_resources/dtbo/`

各驱动目录包含独立 `Makefile`，需要将其中的 `KDIR` 指向与目标系统完全匹配的 Linux 内核源码，并使用 `arm-linux-gnueabihf-` 工具链编译。仓库中的预编译模块仅适用于其对应的系统镜像。

## 相关资料

- [T-Display-CM0 中文使用说明书](<./deploy/T-Display-CM0(SKU：)使用说明书_V1.0.pdf>)
- [CM0 + CM0-KIT 中文规格书](./datasheet/CM0+cm0-KIT中文规格书.pdf)
- [T-Display-CM0 原理图](./schematic/T-DIsplay-CM0-V0.3-20260818.pdf)
- [Raspberry Pi OS 镜像下载说明](./deploy/rasp_img/README_en)

## 贡献

欢迎通过 Issue 或 Pull Request 提交问题、硬件适配、驱动修复和示例程序。反馈问题时建议附上板卡版本、系统镜像、`uname -a`、相关日志以及复现步骤。

## 许可证

仓库根目录使用 [MIT License](LICENSE)。部分 Linux 内核驱动源文件带有独立 SPDX 标识、版权声明或 GPL 模块许可证，这些文件以各自声明为准。
