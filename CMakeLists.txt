# MIT License
#
# Copyright (c) 2026 Grzegorz Grzęda
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
cmake_minimum_required(VERSION 3.25)
project(homecore VERSION 0.0.1 LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(HOMECORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(HOMECORE_BINARY_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(HOMECORE_KCONFIG_CMAKE ${CMAKE_CURRENT_BINARY_DIR}/kconfig.cmake)
set(HOMECORE_BOARD "" CACHE STRING "Target board")

if(NOT HOMECORE_BOARD)
    message(FATAL_ERROR "Use: cmake -B build -DHOMECORE_BOARD=bluepill")
endif()

set(HOMECORE_DEFCONFIG
    ${HOMECORE_ROOT}/configs/homecore_defconfig
    CACHE FILEPATH "HomeCore defconfig file"
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND
        ${Python3_EXECUTABLE}
        ${HOMECORE_ROOT}/scripts/kconfig_generate.py
        --kconfig ${HOMECORE_ROOT}/Kconfig
        --defconfig ${HOMECORE_DEFCONFIG}
        --config-out ${CMAKE_CURRENT_BINARY_DIR}/.config
        --header-out ${HOMECORE_BINARY_INCLUDE_DIR}/homecore/autoconf.h
        --cmake-out ${HOMECORE_KCONFIG_CMAKE}
    WORKING_DIRECTORY ${HOMECORE_ROOT}
    RESULT_VARIABLE HOMECORE_KCONFIG_RESULT
)

if(NOT HOMECORE_KCONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate HomeCore Kconfig files")
endif()
include(${HOMECORE_KCONFIG_CMAKE})

add_custom_target(homecore_config
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/.config
        ${HOMECORE_BINARY_INCLUDE_DIR}/homecore/autoconf.h
)

add_executable(${PROJECT_NAME})

include(${HOMECORE_ROOT}/cmake/board.cmake)
include(${HOMECORE_ROOT}/cmake/soc.cmake)
include(${HOMECORE_ROOT}/cmake/arch.cmake)

add_dependencies(${PROJECT_NAME} homecore_config)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${HOMECORE_BINARY_INCLUDE_DIR}
)

add_subdirectory(src)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -ffreestanding
    -fno-builtin
    -mcpu=cortex-m3
    -mthumb
)

target_link_options(${PROJECT_NAME} PRIVATE
    -T${CMAKE_SOURCE_DIR}/src/board/${HOMECORE_BOARD}/board.ld
    -nostartfiles
    -specs=nano.specs
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,--defsym=__stack_size=${CONFIG_HOMECORE_KERNEL_MAIN_STACK_SIZE}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME} ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}
)