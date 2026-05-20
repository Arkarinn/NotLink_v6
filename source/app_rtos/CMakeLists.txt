# 最低版本限制
cmake_minimum_required(VERSION 3.24)

# 引入自定义CMAKE函数
include("${CMAKE_SOURCE_DIR}/scripts/cmake/set_target_linker.cmake")
include("${CMAKE_SOURCE_DIR}/scripts/cmake/generate_hex.cmake")
include("${CMAKE_SOURCE_DIR}/scripts/cmake/generate_bin.cmake")
include("${CMAKE_SOURCE_DIR}/scripts/cmake/print_all_variables.cmake")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h"
    @ONLY
)

# 项目名称
project("app_rtos" C ASM)

# 寻找源文件
file(GLOB SOURCE_FILES "*.c")

# 添加源码
add_executable(${PROJECT_NAME}
)

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCE_FILES}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    "cmsis"
    "hal"
    "bsp"
    "threadx"
    "cherryusb"
    "dap"
)

# 头文件路径
target_include_directories(${PROJECT_NAME}
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# 添加链接脚本
set_target_linker(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/scripts/ldscripts/STM32H750VBTX_APP_SPI.ld)

# 生成二进制文件
generate_hex()
generate_bin()


# 项目名称
project("app_rtos_without_boot" C ASM)

# 寻找源文件
file(GLOB SOURCE_FILES "*.c")

# 添加源码
add_executable(${PROJECT_NAME}
)

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCE_FILES}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    "cmsis"
    "hal"
    "bsp"
    "threadx"
    "cherryusb"
    "dap"
)

# 头文件路径
target_include_directories(${PROJECT_NAME}
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    "CONFIG_APP_WITHOUT_BOOT"
    "CONFIG_OPTIMIZE_TMS_HARD_OE"
)

# 添加链接脚本
set_target_linker(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/scripts/ldscripts/STM32H750VBTX_FLASH.ld)

# 生成二进制文件
generate_hex()
generate_bin()
