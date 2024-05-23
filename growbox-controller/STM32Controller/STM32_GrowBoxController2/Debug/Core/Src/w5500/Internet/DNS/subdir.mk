################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/w5500/Internet/DNS/dns.c 

OBJS += \
./Core/Src/w5500/Internet/DNS/dns.o 

C_DEPS += \
./Core/Src/w5500/Internet/DNS/dns.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/w5500/Internet/DNS/%.o Core/Src/w5500/Internet/DNS/%.su Core/Src/w5500/Internet/DNS/%.cyclo: ../Core/Src/w5500/Internet/DNS/%.c Core/Src/w5500/Internet/DNS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Core/Src/w5500 -I../Core/Src/w5500/Internet/DHCP -I../Core/Src/w5500/Internet/DNS -I../Core/Src/w5500/Internet/MQTT -I../Core/Src/w5500/Internet/MQTT/MQTTPacket -I../Core/Src/w5500/W5500 -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-w5500-2f-Internet-2f-DNS

clean-Core-2f-Src-2f-w5500-2f-Internet-2f-DNS:
	-$(RM) ./Core/Src/w5500/Internet/DNS/dns.cyclo ./Core/Src/w5500/Internet/DNS/dns.d ./Core/Src/w5500/Internet/DNS/dns.o ./Core/Src/w5500/Internet/DNS/dns.su

.PHONY: clean-Core-2f-Src-2f-w5500-2f-Internet-2f-DNS

