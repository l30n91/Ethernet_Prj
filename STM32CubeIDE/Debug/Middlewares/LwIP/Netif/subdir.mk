################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
E:/T1893PFW_00/STM32CubeF4/Middlewares/Third_Party/LwIP/src/netif/ethernet.c \
E:/T1893PFW_00/STM32CubeF4/Middlewares/Third_Party/LwIP/system/OS/sys_arch.c 

OBJS += \
./Middlewares/LwIP/Netif/ethernet.o \
./Middlewares/LwIP/Netif/sys_arch.o 

C_DEPS += \
./Middlewares/LwIP/Netif/ethernet.d \
./Middlewares/LwIP/Netif/sys_arch.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/LwIP/Netif/ethernet.o: E:/T1893PFW_00/STM32CubeF4/Middlewares/Third_Party/LwIP/src/netif/ethernet.c Middlewares/LwIP/Netif/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F4XX_NUCLEO_144 -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../../Inc -I../../Src -I../../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../../Drivers/BSP/STM32F4xx_Nucleo_144 -I../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../Middlewares/Third_Party/LwIP/src/include -I../../../../../../../Middlewares/Third_Party/LwIP/system -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../../../../../../Utilities -I../../../../../../../Utilities/Log -I../../../../../../../Drivers/CMSIS/Include -I"E:/T1893PFW_00/STM32CubeF4/Projects/STM32F429ZI-Nucleo/Applications/LwIP/LwIP_HTTP_Server_Netconn_RTOS/tcp" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Middlewares/LwIP/Netif/sys_arch.o: E:/T1893PFW_00/STM32CubeF4/Middlewares/Third_Party/LwIP/system/OS/sys_arch.c Middlewares/LwIP/Netif/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_STM32F4XX_NUCLEO_144 -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../../Inc -I../../Src -I../../../../../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../../../../../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../../../../../../Drivers/BSP/STM32F4xx_Nucleo_144 -I../../../../../../../Drivers/BSP/Components/Common -I../../../../../../../Middlewares/Third_Party/LwIP/src/include -I../../../../../../../Middlewares/Third_Party/LwIP/system -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../../../../../../../Utilities -I../../../../../../../Utilities/Log -I../../../../../../../Drivers/CMSIS/Include -I"E:/T1893PFW_00/STM32CubeF4/Projects/STM32F429ZI-Nucleo/Applications/LwIP/LwIP_HTTP_Server_Netconn_RTOS/tcp" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-LwIP-2f-Netif

clean-Middlewares-2f-LwIP-2f-Netif:
	-$(RM) ./Middlewares/LwIP/Netif/ethernet.d ./Middlewares/LwIP/Netif/ethernet.o ./Middlewares/LwIP/Netif/ethernet.su ./Middlewares/LwIP/Netif/sys_arch.d ./Middlewares/LwIP/Netif/sys_arch.o ./Middlewares/LwIP/Netif/sys_arch.su

.PHONY: clean-Middlewares-2f-LwIP-2f-Netif

