# ESP-SR 模型烧录指南

## ✅ 已完成配置

分区表已修改，添加了 `model` 分区（3MB）用于存放语音识别模型。

## 📋 分区表说明

当前分区配置（`partitions_singleapp_large.csv`）：

```
# ESP-IDF Partition Table
# Name, Type, SubType, Offset,   Size, Flags
nvs,      data, nvs,     0x9000,   24K,
phy_init, data, phy,     0xf000,    4K,
factory,  app,  factory, 0x10000,   2M,
model,    data, spiffs,  0x210000,  3M,    # SR模型（新增）
```

**重要：model分区的偏移地址是 `0x210000`**

## 📦 步骤1：下载SR模型

### 方法1：从ESP-SR GitHub下载（推荐）

1. 访问 ESP-SR releases 页面：
   ```
   https://github.com/espressif/esp-sr/releases
   ```

2. 下载适合ESP32-S3的模型包，例如：
   - **WakeNet9** (唤醒词模型): `wn9_hilexin` 或 `wn9_hiesp`
   - **MultiNet** (命令词模型): `mn7_cn`（中文）或 `mn7_en`（英文）

3. 解压模型文件

### 方法2：使用ESP-Skainet示例中的模型

```bash
# 克隆ESP-Skainet仓库（包含预置模型）
git clone --recursive https://github.com/espressif/esp-skainet.git
cd esp-skainet/model
```

## 🔧 步骤2：准备模型分区镜像

ESP-SR需要将模型打包成SPIFFS文件系统镜像。

### 安装spiffs工具

```bash
pip install mkspiffs-tool
```

### 创建SPIFFS镜像

假设你下载的模型在 `models/` 目录：

```bash
# 使用mkspiffs工具创建镜像（3MB = 0x300000）
mkspiffs -c models -b 4096 -p 256 -s 0x300000 model.bin
```

**参数说明：**
- `-c models`: 源文件目录
- `-b 4096`: 块大小（4KB）
- `-p 256`: 页大小（256字节）
- `-s 0x300000`: 分区大小（3MB）
- `model.bin`: 输出镜像文件

## 🚀 步骤3：烧录模型到ESP32

### 方法1：使用esptool.py

```bash
# 查看分区表中model分区的偏移地址
idf.py partition-table

# 烧录模型镜像到指定地址（通常是0x210000）
esptool.py -p COM3 write_flash 0x210000 model.bin
```

### 方法2：使用ESP-IDF烧录（推荐）

1. 打开项目配置：
   ```bash
   idf.py menuconfig
   ```

2. 导航到：
   ```
   Partition Table → Partition Table → Custom partition CSV file
   ```

3. 确认使用 `partitions_singleapp_large.csv`

4. 编译并烧录：
   ```bash
   # 编译项目
   idf.py build
   
   # 烧录固件和分区表
   idf.py flash
   
   # 单独烧录模型分区
   esptool.py -p COM3 write_flash 0x210000 model.bin
   ```

### 方法3：一键烧录（全部内容）

```bash
# 创建完整的Flash镜像
esptool.py --chip esp32s3 merge_bin -o flash_image.bin \
  --flash_mode dio --flash_freq 80m --flash_size 16MB \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/Demo_esp_sr.bin \
  0x210000 model.bin

# 烧录完整镜像
esptool.py -p COM3 write_flash 0x0 flash_image.bin
```

## 📝 验证模型是否加载

烧录完成后，重启设备并查看串口日志：

```
I (1234) MAIN: SR模型初始化成功，共 2 个模型
I (1235) MAIN: 找到唤醒词模型: wn9_hilexin
I (1236) MAIN: 找到命令词模型: mn7_cn
```

## 🎤 支持的模型

### 唤醒词模型（WakeNet）

| 模型名称 | 唤醒词 | 语言 |
|---------|--------|------|
| wn9_hilexin | "Hi 乐鑫" | 中文 |
| wn9_hiesp | "Hi ESP" | 英文 |
| wn9_nihaoxiaozhi | "你好小智" | 中文 |
| wn9_alexa | "Alexa" | 英文 |

### 命令词模型（MultiNet）

| 模型名称 | 语言 | 说明 |
|---------|------|------|
| mn7_cn | 中文 | 支持自定义命令词 |
| mn7_en | 英文 | 支持自定义命令词 |

## ⚠️ 注意事项

1. **分区大小**：确保模型文件总大小不超过3MB，如果模型较大，可以调整分区表中的大小
2. **对齐要求**：SPIFFS镜像必须对齐到4KB边界
3. **Flash空间**：确保16MB Flash有足够空间容纳应用+模型
4. **采样率匹配**：麦克风采样率需要调整为16kHz以匹配SR模型要求

## 🔄 如果需要更换模型

1. 准备新的模型文件
2. 重新生成SPIFFS镜像
3. 只需烧录model分区：
   ```bash
   esptool.py -p COM3 write_flash 0x210000 new_model.bin
   ```

## 🛠️ 调整麦克风采样率

由于当前麦克风配置是8kHz，需要修改为16kHz：

```c
// 在 INMP441.h 中修改
#define INMP441_SAMPLE_RATE     16000   // 改为16kHz

// 在 MAX98367A.h 中同步修改
#define MAX98367A_SAMPLE_RATE   16000   // 改为16kHz
```

## 📚 参考资源

- [ESP-SR 官方文档](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/)
- [ESP-Skainet 示例](https://github.com/espressif/esp-skainet)
- [分区表说明](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/api-guides/partition-tables.html)
