# OCTOS

## About The Project

OCTOS is a compact real-time operating system (RTOS) featuring a preemptive kernel and an O(1) scheduler. 

>   Currently, it supports only the ARM Cortex-M4 processor, but the system is designed to be extensible (see `Core/Arch`).

## Features

*   **FreeRTOS-like preemptive scheduling policy**
    *   Round-Robin within priority level
    *   “Cooperative” between priority level

*   **Basic Task Management**
    *   `task_create`, `task_create_static`, `task_delete`
    *   `task_delay`, `task_abort_delay`
    *   `task_suspend`, `task_resume`, `task_resume_from_isr`
    *   `task_yield`, `task_yield_from_isr`
*   **Python-like Sync Primitives**
    *   `Sema_t` (ISR-compatible)
    *   `Mutex_t` (Support Priority Inheritance)
*   **Fexlible Inter-task Communication**
    *   *Lightweight Task Notification* (ISR-compatible)
    *   *Message Queue* (ISR-compatible)

## Usage

### Prerequisites

*   [GNU Arm Embedded Toolchain](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain)
*   [CMake](https://cmake.org)
*   [OpenOCD](https://openocd.org) (Optional)

### Build

```shell
# Start from project root directory
mkdir build
cd build
cmake ..

# or `cmake --build . --target flash` (OpenOCD and ST-LINK required)
cmake --build . --target octos
```

### Debugging

```shell
# Inside Build Directory
# Open a gdb server at :3333 (OpenOCD and ST-LINK required)
cmake --build . --target gdbhost

# Inside project root directory
# Try input `oinfo` and `otasks` in gdb
arm-none-eabi-gcc -x .gdbinit
```

## Credits

*   https://www.udemy.com/course/rtos-building-from-ground-up-on-arm-processors
*   https://jyywiki.cn/OS/2024
*   https://www.freertos.org
*   https://github.com/Eplankton/mos-stm32
*   https://github.com/FARLY7/embedded-cli
