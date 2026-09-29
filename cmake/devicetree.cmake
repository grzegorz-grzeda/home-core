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
# Compile-time device description. Merges the SoC and board descriptions and
# optional overlays, validates them against driver bindings, and generates
# constants, driver instances, the linker MEMORY block, and the list of driver
# sources. Only drivers of enabled devices are compiled.

set(HOMECORE_DT_OVERLAYS "" CACHE STRING
    "Device-description overlays applied after the board, in order (semicolon-separated)")

set(HOMECORE_DT_SOC ${HOMECORE_SOC_DIR}/soc.yaml)
set(HOMECORE_DT_BOARD ${HOMECORE_BOARD_DIR}/board.yaml)
foreach(description ${HOMECORE_DT_SOC} ${HOMECORE_DT_BOARD})
    if(NOT EXISTS ${description})
        message(FATAL_ERROR "Missing device description: ${description}")
    endif()
endforeach()

set(HOMECORE_DT_GENERATED ${CMAKE_BINARY_DIR}/generated)
set(HOMECORE_DT_OVERLAY_ARGS)
foreach(overlay ${HOMECORE_DT_OVERLAYS})
    get_filename_component(overlay ${overlay} ABSOLUTE BASE_DIR ${HOMECORE_ROOT})
    list(APPEND HOMECORE_DT_OVERLAY_ARGS --overlay ${overlay})
    list(APPEND HOMECORE_DT_INPUTS ${overlay})
endforeach()

execute_process(
    COMMAND
        ${Python3_EXECUTABLE} ${HOMECORE_ROOT}/scripts/devicetree_generate.py
        --soc ${HOMECORE_DT_SOC}
        --board ${HOMECORE_DT_BOARD}
        ${HOMECORE_DT_OVERLAY_ARGS}
        --bindings ${HOMECORE_ROOT}/src/drivers
        --header ${HOMECORE_BINARY_INCLUDE_DIR}/homecore/devicetree.h
        --source ${HOMECORE_DT_GENERATED}/devicetree.c
        --memory ${HOMECORE_DT_GENERATED}/memory.ld
        --cmake ${HOMECORE_DT_GENERATED}/devicetree.cmake
    WORKING_DIRECTORY ${HOMECORE_ROOT}
    RESULT_VARIABLE HOMECORE_DT_RESULT
)
if(NOT HOMECORE_DT_RESULT EQUAL 0)
    message(FATAL_ERROR "Invalid device description for ${HOMECORE_BOARD} (see message above)")
endif()

# Rerun configuration when a description, binding, or the generator changes.
file(GLOB_RECURSE HOMECORE_DT_BINDINGS ${HOMECORE_ROOT}/src/drivers/*.yaml)
set_property(DIRECTORY ${HOMECORE_ROOT} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${HOMECORE_DT_SOC} ${HOMECORE_DT_BOARD} ${HOMECORE_DT_INPUTS} ${HOMECORE_DT_BINDINGS}
    ${HOMECORE_ROOT}/scripts/devicetree_generate.py
)

include(${HOMECORE_DT_GENERATED}/devicetree.cmake)
file(READ ${HOMECORE_DT_GENERATED}/memory.ld HOMECORE_BOARD_MEMORY)

target_sources(${PROJECT_NAME} PRIVATE ${HOMECORE_DT_GENERATED}/devicetree.c ${HOMECORE_DT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${HOMECORE_DT_INCLUDE_DIRS})
