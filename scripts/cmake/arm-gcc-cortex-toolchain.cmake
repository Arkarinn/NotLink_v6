set(CMAKE_SYSTEM_NAME Generic)  # 系统载体，嵌入式没有OS，Generic
set(CMAKE_SYSTEM_PROCESSOR arm) # 处理器类型

set(TARGET_TRIPLET "arm-none-eabi-")# 编译器版本前缀

# WIN系统下，添加".exe"扩展名后缀
if(WIN32)
    set(TOOLCHAIN_EXT ".exe")
else()
    set(TOOLCHAIN_EXT "")
endif(WIN32)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)# 使用静态库执行编译器测试

if(ARM_TOOLCHAIN_PATH)
    set(ARM_TOOLCHAIN_PATH "${ARM_TOOLCHAIN_PATH}/")
endif()

# 指定编译器
set(CMAKE_C_COMPILER    ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}gcc${TOOLCHAIN_EXT})
set(CMAKE_CXX_COMPILER  ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}g++${TOOLCHAIN_EXT})
set(CMAKE_ASM_COMPILER  ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}gcc${TOOLCHAIN_EXT})
set(CMAKE_LINKER        ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}g++${TOOLCHAIN_EXT})
set(CMAKE_SIZE_UTIL     ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}size${TOOLCHAIN_EXT})
set(CMAKE_OBJCOPY       ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}objcopy${TOOLCHAIN_EXT})
set(CMAKE_OBJDUMP       ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}objdump${TOOLCHAIN_EXT})
set(CMAKE_NM_UTIL       ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}gcc-nm${TOOLCHAIN_EXT})
set(CMAKE_AR            ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}gcc-ar${TOOLCHAIN_EXT})
set(CMAKE_RANLIB        ${ARM_TOOLCHAIN_PATH}${TARGET_TRIPLET}gcc-ranlib${TOOLCHAIN_EXT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)# 不查找
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

# 编译器选项
set(CMAKE_COMMON_FLAGS "-Wall -Wextra -Wshadow -Wpedantic -Wdouble-promotion")
set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -Wno-unused-parameter -Wno-unused-function")
set(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -mno-unaligned-access -fno-builtin")

set(CMAKE_C_FLAGS 	"${MCPU_FLAGS} ${VFP_FLAGS} ${CMAKE_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -lstdc++ -lsupc++ -fno-rtti -fno-exceptions -fno-threadsafe-statics")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")

set(CMAKE_C_LINK_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} --specs=nano.specs")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")

set(CMAKE_C_FLAGS_DEBUG "-Og -g -ggdb3")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g -ggdb3")
set(CMAKE_ASM_FLAGS_DEBUG "-g -ggdb3")

set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_ASM_FLAGS_RELEASE "")
