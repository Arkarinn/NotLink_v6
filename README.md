# NotLink_v6

### 介绍
- STM32H750VB为主控的CMSIS-DAP调试器，使用SPI+TIMER模拟SWD和JTAG协议。
- 稳定速度30M，读写RAM速度1200+KB。
- SWD已充分测试。
- JTAG可用于ARM烧写，烧写RISC-V单片机（例如HPM）时工作不正常。

### 软件架构
- gcc编译器
- cmake编译管理
- STM32 LL库、HAL库
- pyOCD烧录、调试、测速
- probe-rs测速
- OpenOCD烧写

### 安装编译环境
1. 编译器解压复制到`./toolchain/arm-none-eabi-gcc`，确保有`./toolchain/arm-none-eabi-gcc/bin/arm-none-eabi-gcc.exe`
2. Cmake复制到`./toolchain/cmake`，确保有`./toolchain/cmake/bin/cmake.exe`
3. Ninja复制到`./toolchain/ninja`，确保有`./toolchain/ninja/ninja.exe`
3. OpenOCD复制到`./toolchain/openocd`，确保有`./toolchain/openocd/bin/openocd.exe`
4. VsCode安装cmake_tool
5. 安装pyOCD烧写环境

### 编译烧录
1. cmake_tool选择Release或Debug
2. 配置+编译
3. 运行Task: openocd_load_flash_app_rtos_without_boot

### 引用项目
1. CherryUSB https://github.com/cherry-embedded/CherryUSB
2. STM32 HAL https://github.com/STMicroelectronics/STM32CubeH7
3. CMSIS-DAP https://github.com/ARM-software/CMSIS-DAP
4. ThreadX https://github.com/eclipse-threadx/threadx

### 参与贡献

### Bug
1. 长时间运行后串口无法收发。
2. JTAG序列工作不正常。
