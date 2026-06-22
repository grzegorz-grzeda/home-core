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


add_executable(${PROJECT_NAME})

add_subdirectory(src)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE homecore_board
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -ffreestanding
    -fno-builtin
    -mcpu=cortex-m3
    -mthumb
)

target_link_options(${PROJECT_NAME} PRIVATE
    -T${CMAKE_SOURCE_DIR}/src/board/${HOMECORE_BOARD}/linker.ld
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME} ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}
)