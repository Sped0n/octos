cmake_minimum_required(VERSION 3.22)

project(stm32cubemx)
add_library(stm32cubemx INTERFACE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

target_compile_definitions(stm32cubemx INTERFACE
    USE_FULL_LL_DRIVER
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
    ../../Libs/CMSIS/Device/ST/STM32F4xx/Include
    ../../Libs/CMSIS/Include
    ../../Libs/STM32F4xx_HAL_Driver/Inc
    ../../Libs/STM32F4xx_HAL_Driver/Inc/Legacy
    ../../Drivers/Inc
    ../../Core/Inc
    ../../App/Inc
    ../../
    ${ARM_TOOLCHAIN_DIR}/include
    ${ARM_TOOLCHAIN_DIR}/lib
)

file(GLOB DRIVER_SOURCES CONFIGURE_DEPENDS "../../Drivers/Src/*.c")
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "../../Core/Src/*.c")
file(GLOB APP_SOURCES CONFIGURE_DEPENDS "../../App/Src/*.c")

target_sources(stm32cubemx INTERFACE
    ../../startup_stm32f429xx.s
    ../../Libs/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_gpio.c
    ../../Libs/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usart.c
    ${DRIVER_SOURCES}
    ${CORE_SOURCES}
    ${APP_SOURCES}
)

target_link_directories(stm32cubemx INTERFACE
)

target_link_libraries(stm32cubemx INTERFACE
)

# Validate that STM32CubeMX code is compatible with C standard
if(CMAKE_C_STANDARD LESS 11)
    message(ERROR "Generated code requires C11 or higher")
endif()


