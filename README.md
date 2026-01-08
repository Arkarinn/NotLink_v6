# NotLink_v6

### 介绍
STM32H750VB为主控的CMSIS-DAP调试器，使用SPI加TIM模拟SWD和JTAG协议。
稳定速度30M，读写RAM速度1200+KB。
SWD已充分测试，JTAG未测试。

### 软件架构
- gcc编译器
- cmake编译管理
- STM32 LL库、HAL库
- pyocd烧录、调试、测速
- probe-rs测速
- openocd烧写

### 安装编译环境
1. 编译器解压复制到`./toolchain/arm-none-eabi-gcc`，确保有`./toolchain/arm-none-eabi-gcc/bin/arm-none-eabi-gcc.exe`
2. Cmake复制到`./toolchain/cmake`，确保有`./toolchain/cmake/bin/cmake.exe`
3. Ninja复制到`./toolchain/ninja`，确保有`./toolchain/ninja/ninja.exe`
4. VsCode安装cmake_tool
5. 安装pyocd烧写环境

### 编译烧录
1. cmake_tool扩展选Release或Debug
2. 编译
3. 运行Task flash_boot
4. 运行Task flash_app_rtos

### 引用项目
1. CherryUSB https://github.com/cherry-embedded/CherryUSB
2. STM32 HAL https://github.com/STMicroelectronics/STM32CubeH7
3. CMSIS-DAP https://github.com/ARM-software/CMSIS-DAP
4. ThreadX https://github.com/eclipse-threadx/threadx

### 参与贡献

### Bug
1. 长时间运行后串口无法收发

