# 项目名称
project("threadx" C ASM)

# 添加源码
add_library(${PROJECT_NAME}
    INTERFACE
)

target_sources(${PROJECT_NAME}
    INTERFACE
)

target_compile_definitions(${PROJECT_NAME}
    INTERFACE
    "TX_INCLUDE_USER_DEFINE_FILE"
)

add_subdirectory("common")
add_subdirectory("ports/cortex_m7/gnu")

# 链接库
target_link_libraries(${PROJECT_NAME}
    INTERFACE
    "bsp"
)

# 头文件路径
target_include_directories(${PROJECT_NAME}
    INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
