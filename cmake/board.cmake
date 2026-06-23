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
set(HOMECORE_BOARD_DIR ${HOMECORE_ROOT}/src/board/${HOMECORE_BOARD})

if(NOT EXISTS ${HOMECORE_BOARD_DIR}/board.cmake)
    message(STATUS "------------------------------------------------")
    message(STATUS "Unknown board: ${HOMECORE_BOARD}. Missing: ${HOMECORE_BOARD_DIR}/board.cmake")

    file(GLOB BOARD_DIRS RELATIVE ${HOMECORE_ROOT}/src/board ${HOMECORE_ROOT}/src/board/*)

    message(STATUS "Available boards:")
    foreach(board_dir ${BOARD_DIRS})
        if(EXISTS ${HOMECORE_ROOT}/src/board/${board_dir}/board.cmake)
            message(STATUS "  ${board_dir}")
        endif()
    endforeach()
    message(STATUS "------------------------------------------------")
    message(FATAL_ERROR "Terminating due to error.  Please select a valid board using -DHOMECORE_BOARD=<board_name>")
endif()

include(${HOMECORE_BOARD_DIR}/board.cmake)

if(NOT DEFINED HOMECORE_SOC)
    message(FATAL_ERROR "Board ${HOMECORE_BOARD} did not set SOC")
endif()
