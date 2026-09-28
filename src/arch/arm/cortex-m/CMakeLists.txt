# SPDX-License-Identifier: MIT
if(NOT EXISTS ${HOMECORE_ROOT}/external/cmsis/CMSIS/Core/Include/core_cm4.h)
    message(FATAL_ERROR "CMSIS-Core missing. Run: git submodule update --init --recursive")
endif()
target_include_directories(${PROJECT_NAME} PRIVATE
    ${HOMECORE_ROOT}/external/cmsis/CMSIS/Core/Include)
target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/arch.c
    ${CMAKE_CURRENT_LIST_DIR}/boot.c)
target_compile_definitions(${PROJECT_NAME} PRIVATE HOMECORE_ARCH=${HOMECORE_ARCH_ID})
