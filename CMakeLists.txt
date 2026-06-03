cmake_minimum_required(VERSION 3.15)

project(rtos C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")

# Diretório de includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Arquivos fonte do kernel
set(RTOS_SOURCES
    src/kernel/kernel.c
    src/scheduler/scheduler.c
    src/scheduler/task.c
    src/sync/semaphore.c
    src/sync/mutex.c
    src/sync/queue.c
    src/memory/memory.c
    src/isr/isr.c
    src/platform/esp32/timer.c
    src/platform/esp32/context.c
)

# Criar biblioteca RTOS
add_library(rtos STATIC ${RTOS_SOURCES})

# Diretório de exemplos
add_subdirectory(examples)
