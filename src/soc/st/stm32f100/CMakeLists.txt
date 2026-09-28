target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common/soc.c interrupts.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    ${HOMECORE_ROOT}/external/stm32f1/Include)
target_compile_definitions(${PROJECT_NAME} PRIVATE STM32F100xB)
