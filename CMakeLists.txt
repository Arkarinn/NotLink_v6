# 最低版本限制
cmake_minimum_required(VERSION 3.24)

MESSAGE(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

set(CMAKE_C_STANDARD 11)			# 使用C语言标准
set(CMAKE_C_STANDARD_REQUIRED ON)	# 强制使用语言标准，不支持则报错
set(CMAKE_C_EXTENSIONS ON)			# 使用GCC对标准C语言的扩展

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE) # 编译时输出命令

add_compile_definitions(
	"$<$<CONFIG:Debug>:DEBUG>"	# 自动添加DEBUG宏定义
)

# 目标
project("NotLink_v6")

# 添加子文件夹
add_subdirectory("source")
