# 项目名称
project("dap" C ASM)

# 寻找源文件
file(GLOB SOURCE_FILES "*.c")

# 添加源码
add_library(${PROJECT_NAME}
    INTERFACE
)

target_sources(${PROJECT_NAME}
    INTERFACE
    ${SOURCE_FILES}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    INTERFACE
    "bsp"
    "cmsis"
    "ulog"
)

# 头文件路径
target_include_directories(${PROJECT_NAME}
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
