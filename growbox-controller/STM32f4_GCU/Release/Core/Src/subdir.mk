################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/DS3231.c \
../Core/Src/at24cxx.c \
../Core/Src/cJSON.c \
../Core/Src/controller_state.c \
../Core/Src/eeprom.c \
../Core/Src/freertos.c \
../Core/Src/hardware.c \
../Core/Src/helper_websocket.c \
../Core/Src/main.c \
../Core/Src/sha1.c \
../Core/Src/state_manager.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/task_alive.c \
../Core/Src/task_hardware.c \
../Core/Src/task_light_controller.c \
../Core/Src/task_network.c \
../Core/Src/task_sensor.c \
../Core/Src/task_watcher.c \
../Core/Src/task_water_controller.c \
../Core/Src/time_utils.c \
../Core/Src/uart_redirect.c \
../Core/Src/wizchip_init.c 

OBJS += \
./Core/Src/DS3231.o \
./Core/Src/at24cxx.o \
./Core/Src/cJSON.o \
./Core/Src/controller_state.o \
./Core/Src/eeprom.o \
./Core/Src/freertos.o \
./Core/Src/hardware.o \
./Core/Src/helper_websocket.o \
./Core/Src/main.o \
./Core/Src/sha1.o \
./Core/Src/state_manager.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/task_alive.o \
./Core/Src/task_hardware.o \
./Core/Src/task_light_controller.o \
./Core/Src/task_network.o \
./Core/Src/task_sensor.o \
./Core/Src/task_watcher.o \
./Core/Src/task_water_controller.o \
./Core/Src/time_utils.o \
./Core/Src/uart_redirect.o \
./Core/Src/wizchip_init.o 

C_DEPS += \
./Core/Src/DS3231.d \
./Core/Src/at24cxx.d \
./Core/Src/cJSON.d \
./Core/Src/controller_state.d \
./Core/Src/eeprom.d \
./Core/Src/freertos.d \
./Core/Src/hardware.d \
./Core/Src/helper_websocket.d \
./Core/Src/main.d \
./Core/Src/sha1.d \
./Core/Src/state_manager.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/task_alive.d \
./Core/Src/task_hardware.d \
./Core/Src/task_light_controller.d \
./Core/Src/task_network.d \
./Core/Src/task_sensor.d \
./Core/Src/task_watcher.d \
./Core/Src/task_water_controller.d \
./Core/Src/time_utils.d \
./Core/Src/uart_redirect.d \
./Core/Src/wizchip_init.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/DS3231.cyclo ./Core/Src/DS3231.d ./Core/Src/DS3231.o ./Core/Src/DS3231.su ./Core/Src/at24cxx.cyclo ./Core/Src/at24cxx.d ./Core/Src/at24cxx.o ./Core/Src/at24cxx.su ./Core/Src/cJSON.cyclo ./Core/Src/cJSON.d ./Core/Src/cJSON.o ./Core/Src/cJSON.su ./Core/Src/controller_state.cyclo ./Core/Src/controller_state.d ./Core/Src/controller_state.o ./Core/Src/controller_state.su ./Core/Src/eeprom.cyclo ./Core/Src/eeprom.d ./Core/Src/eeprom.o ./Core/Src/eeprom.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/hardware.cyclo ./Core/Src/hardware.d ./Core/Src/hardware.o ./Core/Src/hardware.su ./Core/Src/helper_websocket.cyclo ./Core/Src/helper_websocket.d ./Core/Src/helper_websocket.o ./Core/Src/helper_websocket.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/sha1.cyclo ./Core/Src/sha1.d ./Core/Src/sha1.o ./Core/Src/sha1.su ./Core/Src/state_manager.cyclo ./Core/Src/state_manager.d ./Core/Src/state_manager.o ./Core/Src/state_manager.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.cyclo ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/task_alive.cyclo ./Core/Src/task_alive.d ./Core/Src/task_alive.o ./Core/Src/task_alive.su ./Core/Src/task_hardware.cyclo ./Core/Src/task_hardware.d ./Core/Src/task_hardware.o ./Core/Src/task_hardware.su ./Core/Src/task_light_controller.cyclo ./Core/Src/task_light_controller.d ./Core/Src/task_light_controller.o ./Core/Src/task_light_controller.su ./Core/Src/task_network.cyclo ./Core/Src/task_network.d ./Core/Src/task_network.o ./Core/Src/task_network.su ./Core/Src/task_sensor.cyclo ./Core/Src/task_sensor.d ./Core/Src/task_sensor.o ./Core/Src/task_sensor.su ./Core/Src/task_watcher.cyclo ./Core/Src/task_watcher.d ./Core/Src/task_watcher.o ./Core/Src/task_watcher.su ./Core/Src/task_water_controller.cyclo ./Core/Src/task_water_controller.d ./Core/Src/task_water_controller.o ./Core/Src/task_water_controller.su ./Core/Src/time_utils.cyclo ./Core/Src/time_utils.d ./Core/Src/time_utils.o ./Core/Src/time_utils.su ./Core/Src/uart_redirect.cyclo ./Core/Src/uart_redirect.d ./Core/Src/uart_redirect.o ./Core/Src/uart_redirect.su ./Core/Src/wizchip_init.cyclo ./Core/Src/wizchip_init.d ./Core/Src/wizchip_init.o ./Core/Src/wizchip_init.su

.PHONY: clean-Core-2f-Src

