################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectClient.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectServer.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTDeserializePublish.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTFormat.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTPacket.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSerializePublish.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeClient.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeServer.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeClient.c \
../Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeServer.c 

OBJS += \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectClient.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectServer.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTDeserializePublish.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTFormat.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTPacket.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSerializePublish.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeClient.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeServer.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeClient.o \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeServer.o 

C_DEPS += \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectClient.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectServer.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTDeserializePublish.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTFormat.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTPacket.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSerializePublish.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeClient.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeServer.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeClient.d \
./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeServer.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/w5500/MQTT/MQTTPacket/src/%.o Core/Src/w5500/MQTT/MQTTPacket/src/%.su Core/Src/w5500/MQTT/MQTTPacket/src/%.cyclo: ../Core/Src/w5500/MQTT/MQTTPacket/src/%.c Core/Src/w5500/MQTT/MQTTPacket/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-w5500-2f-MQTT-2f-MQTTPacket-2f-src

clean-Core-2f-Src-2f-w5500-2f-MQTT-2f-MQTTPacket-2f-src:
	-$(RM) ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectClient.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectClient.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectClient.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectClient.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectServer.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectServer.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectServer.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTConnectServer.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTDeserializePublish.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTDeserializePublish.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTDeserializePublish.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTDeserializePublish.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTFormat.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTFormat.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTFormat.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTFormat.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTPacket.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTPacket.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTPacket.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTPacket.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSerializePublish.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSerializePublish.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSerializePublish.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSerializePublish.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeClient.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeClient.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeClient.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeClient.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeServer.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeServer.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeServer.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTSubscribeServer.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeClient.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeClient.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeClient.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeClient.su ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeServer.cyclo ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeServer.d ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeServer.o ./Core/Src/w5500/MQTT/MQTTPacket/src/MQTTUnsubscribeServer.su

.PHONY: clean-Core-2f-Src-2f-w5500-2f-MQTT-2f-MQTTPacket-2f-src

