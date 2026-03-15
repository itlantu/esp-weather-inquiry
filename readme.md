# ESP-Weather-Inquiry

> 基于ESP32的中国全国各城市天气查询

## 项目简介

本项目是一个基于ESP32的物联网天气查询设备固件，可以查询中国各城市的实时天气信息。

## 硬件要求

- ESP32 开发板
- 连接到互联网的WiFi网络

## 软件依赖

- ESP-IDF 开发框架
- CMake 构建工具

## 快速开始

### 1. 克隆项目

```bash
git clone https://github.com/itlantu/esp-weather-inquiry.git
cd esp-weather-inquiry
```

### 2. 配置WiFi

在 `include/wi/config.h` 中修改默认的WiFi配置：

```c
#define WI_CONFIG_DEFAULT_STA_SSID "你的WiFi名称"
#define WI_CONFIG_DEFAULT_STA_PASSWORD "你的WiFi密码"
```

### 3. 编译烧录

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## 配置说明

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| STA_SSID | ssid | WiFi名称 |
| STA_PASSWORD | password | WiFi密码 |
| AP_SSID | esp-weather-inquiry-ap | AP模式热点名称 |
| AP_PASSWORD | 12345678 | AP模式密码 |
| WEB_PORT | 80 | Web服务器端口 |
| WEB_TITLE | ESP-Weather-Inquiry | 网页标题 |

## 目录结构

```
esp-weather-inquiry/
├── CMakeLists.txt          # CMake构建配置
├── include/                # 头文件目录
│   └── wi/
│       ├── config.h        # 配置相关
│       ├── nvs.h           # 非易失性存储
│       └── comm/           # 通信相关
├── src/                    # 源代码目录
│   ├── main.c              # 主程序
│   ├── config.c            # 配置实现
│   └── comm/               # 通信实现
└── readme.md               # 说明文档
```

## 许可证

MIT License
