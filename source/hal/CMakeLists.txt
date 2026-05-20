# 项目名称
project("hal" C ASM)

# 寻找源文件
file(GLOB SOURCE_FILES "*.c")
file(GLOB HEADER_FILES "*.h")

# 添加源码
add_library(${PROJECT_NAME}
    INTERFACE
)

target_sources(${PROJECT_NAME} 
    INTERFACE
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

target_compile_definitions(${PROJECT_NAME}
    INTERFACE
    "STM32H750xx"
    "USE_FULL_LL_DRIVER"
    "USE_FULL_HAL_DRIVER"
    "USE_PWR_LDO_SUPPLY"
    "HSE_VALUE=24000000"
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    INTERFACE
    "cmsis"
)

# 头文件路径
target_include_directories(${PROJECT_NAME}
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
