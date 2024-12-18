cmake_minimum_required(VERSION 3.22)

project(stm32cubemx)
add_library(stm32cubemx INTERFACE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

target_compile_definitions(stm32cubemx INTERFACE
    STM32F429xx
    $<$<CONFIG:Debug>:DEBUG>
)

# Find ARM GCC installation
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-sysroot
    OUTPUT_VARIABLE ARM_TOOLCHAIN_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_include_directories(stm32cubemx INTERFACE
    ../../Core/Inc
    ../../CMSIS/Device/ST/STM32F4xx/Include
    ../../CMSIS/Include
    ../../Drivers/Inc
    ${ARM_TOOLCHAIN_DIR}/include
    ${ARM_TOOLCHAIN_DIR}/lib
)


target_sources(stm32cubemx INTERFACE
    ../../Core/Src/main.c
    ../../Core/Src/stm32f4xx_it.c
    ../../Core/Src/system_stm32f4xx.c
    ../../Core/Src/sysmem.c
    ../../Core/Src/syscall.c
    ../../startup_stm32f429xx.s
    ../../Drivers/Src/led.c
    ../../Drivers/Src/uart.c
)

target_link_directories(stm32cubemx INTERFACE
)

target_link_libraries(stm32cubemx INTERFACE
)

# Validate that STM32CubeMX code is compatible with C standard
if(CMAKE_C_STANDARD LESS 11)
    message(ERROR "Generated code requires C11 or higher")
endif()


