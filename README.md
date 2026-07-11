# NotLink_v6

## 介绍
- STM32H750VB为主控的CMSIS-DAP调试器，使用SPI+TIMER模拟SWD和JTAG协议。
- 30M时钟下，读写RAM速度1600KB。
- SWD已充分测试。
- JTAG暂时不可用。

## 软件架构
- gcc
- cmake
- STM32 LL库、HAL库
- OpenOCD
- probe-rs
- pyOCD

## 安装编译环境
1. 编译器解压复制到`./toolchain/arm-none-eabi-gcc`，确保有`./toolchain/arm-none-eabi-gcc/bin/arm-none-eabi-gcc.exe`
2. Cmake复制到`./toolchain/cmake`，确保有`./toolchain/cmake/bin/cmake.exe`
3. Ninja复制到`./toolchain/ninja`，确保有`./toolchain/ninja/ninja.exe`
3. OpenOCD复制到`./toolchain/openocd`，确保有`./toolchain/openocd/bin/openocd.exe`
4. VsCode安装cmake_tool插件

## 编译烧录
1. cmake_tool配置Cmake项目
2. 编译
3. 运行Task: openocd_load_flash_app_rtos_without_boot

## 引用项目
1. CherryUSB https://github.com/cherry-embedded/CherryUSB
2. STM32 HAL https://github.com/STMicroelectronics/STM32CubeH7
3. CMSIS-DAP https://github.com/ARM-software/CMSIS-DAP
4. ThreadX https://github.com/eclipse-threadx/threadx

## 参与贡献

## Bug
1. 长时间运行后串口无法收发。
2. JTAG_Seq工作不正常，无法正常下载HPM单片机。
