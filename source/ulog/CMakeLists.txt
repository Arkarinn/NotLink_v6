# 项目名称
project("ulog" C)

# 添加库
add_library(${PROJECT_NAME}
    INTERFACE
)

# 源码
target_sources(${PROJECT_NAME}
    INTERFACE
    "ulog.c"
)

# 头文件路径
target_include_directories(${PROJECT_NAME}
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
