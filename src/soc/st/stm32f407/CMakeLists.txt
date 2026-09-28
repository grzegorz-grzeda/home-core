target_sources(${PROJECT_NAME} PRIVATE soc.c interrupts.c)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    ${HOMECORE_ROOT}/external/stm32f4/Include)
target_compile_definitions(${PROJECT_NAME} PRIVATE STM32F407xx)
