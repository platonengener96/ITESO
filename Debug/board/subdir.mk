################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/board.c \
../board/clock_config.c \
../board/pin_mux.c 

C_DEPS += \
./board/board.d \
./board/clock_config.d \
./board/pin_mux.d 

OBJS += \
./board/board.o \
./board/clock_config.o \
./board/pin_mux.o 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c board/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DPRINTF_ADVANCED_ENABLE=1 -DPRINTF_FLOAT_ENABLE=1 -DFREESCALE_KSDK_BM -DMBEDTLS_CONFIG_FILE='"ksdk_mbedtls_config.h"' -DFRDM_K64F -DFREEDOM -DSERIAL_PORT_TYPE_UART=1 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\source" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\drivers" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\mmcau" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\mbedtls\include" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\mbedtls\library" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\mbedtls\port\ksdk" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\utilities" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\device" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\component\uart" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\component\serial_manager" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\component\lists" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\CMSIS" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\board" -I"D:\Documentos\MCUXpressoIDE_11.6.0_8187\workspace\frdmk64f_mbedtls_benchmark\frdmk64f\mbedtls_examples\mbedtls_benchmark" -O0 -fno-common -g3 -fomit-frame-pointer -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-board

clean-board:
	-$(RM) ./board/board.d ./board/board.o ./board/clock_config.d ./board/clock_config.o ./board/pin_mux.d ./board/pin_mux.o

.PHONY: clean-board

