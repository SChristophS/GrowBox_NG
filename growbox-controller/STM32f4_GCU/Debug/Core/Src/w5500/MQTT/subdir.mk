################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/w5500/MQTT/MQTTClient.c \
../Core/Src/w5500/MQTT/mqtt_interface.c 

OBJS += \
./Core/Src/w5500/MQTT/MQTTClient.o \
./Core/Src/w5500/MQTT/mqtt_interface.o 

C_DEPS += \
./Core/Src/w5500/MQTT/MQTTClient.d \
./Core/Src/w5500/MQTT/mqtt_interface.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/w5500/MQTT/%.o Core/Src/w5500/MQTT/%.su Core/Src/w5500/MQTT/%.cyclo: ../Core/Src/w5500/MQTT/%.c Core/Src/w5500/MQTT/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Core/Src/lwjson -I../Core/Inc/lwjson -I../Core/Src/w5500/W5500 -I../Core/Src/w5500 -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-w5500-2f-MQTT

clean-Core-2f-Src-2f-w5500-2f-MQTT:
	-$(RM) ./Core/Src/w5500/MQTT/MQTTClient.cyclo ./Core/Src/w5500/MQTT/MQTTClient.d ./Core/Src/w5500/MQTT/MQTTClient.o ./Core/Src/w5500/MQTT/MQTTClient.su ./Core/Src/w5500/MQTT/mqtt_interface.cyclo ./Core/Src/w5500/MQTT/mqtt_interface.d ./Core/Src/w5500/MQTT/mqtt_interface.o ./Core/Src/w5500/MQTT/mqtt_interface.su

.PHONY: clean-Core-2f-Src-2f-w5500-2f-MQTT

