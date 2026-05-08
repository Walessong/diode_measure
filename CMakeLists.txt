cmake_minimum_required(VERSION 3.22)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    find_program(ARM_GCC arm-none-eabi-gcc)
    find_program(ARM_GXX arm-none-eabi-g++)
    find_program(ARM_ASM arm-none-eabi-gcc)
    find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
    find_program(ARM_SIZE_UTIL arm-none-eabi-size)
    find_program(ARM_NINJA ninja)

    if (ARM_GCC AND ARM_GXX AND ARM_ASM)
        set(CMAKE_SYSTEM_NAME Generic CACHE STRING "" FORCE)
        set(CMAKE_SYSTEM_PROCESSOR arm CACHE STRING "" FORCE)
        set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "" FORCE)
        set(CMAKE_C_COMPILER "${ARM_GCC}" CACHE FILEPATH "" FORCE)
        set(CMAKE_CXX_COMPILER "${ARM_GXX}" CACHE FILEPATH "" FORCE)
        set(CMAKE_ASM_COMPILER "${ARM_ASM}" CACHE FILEPATH "" FORCE)
        if (ARM_NINJA)
            set(CMAKE_MAKE_PROGRAM "${ARM_NINJA}" CACHE FILEPATH "" FORCE)
        endif()
        if (ARM_OBJCOPY)
            set(CMAKE_OBJCOPY "${ARM_OBJCOPY}" CACHE FILEPATH "" FORCE)
        endif()
        if (ARM_SIZE_UTIL)
            set(CMAKE_SIZE "${ARM_SIZE_UTIL}" CACHE FILEPATH "" FORCE)
        endif()
    endif()
endif()

project(diode_measure C ASM)

if (NOT CMAKE_C_COMPILER MATCHES "arm-none-eabi")
    message(FATAL_ERROR
        "This project must be configured with the ARM embedded toolchain.\n"
        "Current compiler: ${CMAKE_C_COMPILER}\n"
        "In CLion, select the CMake preset 'stm32-debug' or configure with "
        "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(PROJECT_SOURCES
    Core/Src/main.c
    Core/Src/syscalls.c
    Core/Src/stm32f1xx_hal_msp.c
    Core/Src/stm32f1xx_it.c
    Core/Src/app.c
    Core/Src/bsp_adc.c
    Core/Src/bsp_key.c
    Core/Src/bsp_oled.c
    Core/Src/bsp_relay.c
    Core/Src/flash_storage.c
    Core/Src/identify.c
    Core/Src/learn.c
    Core/Src/measure.c
    Core/Src/ui.c
    Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_adc_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_i2c.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c
    startup/startup_stm32f103xb.s
)

add_executable(${PROJECT_NAME}.elf ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    Core/Inc
    Drivers/CMSIS/Include
    Drivers/CMSIS/Device/ST/STM32F1xx/Include
    Drivers/STM32F1xx_HAL_Driver/Inc
    Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    STM32F103xB
    USE_HAL_DRIVER
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    $<$<COMPILE_LANGUAGE:C>:-O2>
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp>
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F103C8TX_FLASH.ld
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -u
    _printf_float
)

target_link_libraries(${PROJECT_NAME}.elf m)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
)

find_program(STM32_PROGRAMMER_CLI STM32_Programmer_CLI)

if (STM32_PROGRAMMER_CLI)
    add_custom_target(flash
        COMMAND ${STM32_PROGRAMMER_CLI} -c port=SWD mode=UR -w $<TARGET_FILE:${PROJECT_NAME}.elf> -v -rst
        DEPENDS ${PROJECT_NAME}.elf
        USES_TERMINAL
        COMMENT "Flashing ${PROJECT_NAME}.elf via ST-Link"
    )

    add_custom_target(flash_hex
        COMMAND ${STM32_PROGRAMMER_CLI} -c port=SWD mode=UR -w ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex -v -rst
        DEPENDS ${PROJECT_NAME}.elf
        USES_TERMINAL
        COMMENT "Flashing ${PROJECT_NAME}.hex via ST-Link"
    )
endif()
