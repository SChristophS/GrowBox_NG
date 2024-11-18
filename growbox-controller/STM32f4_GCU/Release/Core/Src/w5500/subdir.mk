################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/w5500/socket.c \
../Core/Src/w5500/wizchip_conf.c 

OBJS += \
./Core/Src/w5500/socket.o \
./Core/Src/w5500/wizchip_conf.o 

C_DEPS += \
./Core/Src/w5500/socket.d \
./Core/Src/w5500/wizchip_conf.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/w5500/%.o Core/Src/w5500/%.su Core/Src/w5500/%.cyclo: ../Core/Src/w5500/%.c Core/Src/w5500/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-w5500

clean-Core-2f-Src-2f-w5500:
	-$(RM) ./Core/Src/w5500/socket.cyclo ./Core/Src/w5500/socket.d ./Core/Src/w5500/socket.o ./Core/Src/w5500/socket.su ./Core/Src/w5500/wizchip_conf.cyclo ./Core/Src/w5500/wizchip_conf.d ./Core/Src/w5500/wizchip_conf.o ./Core/Src/w5500/wizchip_conf.su

.PHONY: clean-Core-2f-Src-2f-w5500

