################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../t_includes/I2C_core.c \
../t_includes/camera_func.c \
../t_includes/io.c \
../t_includes/mipi_bridge_config.c \
../t_includes/mipi_camera_config.c \
../t_includes/queue.c 

OBJS += \
./t_includes/I2C_core.o \
./t_includes/camera_func.o \
./t_includes/io.o \
./t_includes/mipi_bridge_config.o \
./t_includes/mipi_camera_config.o \
./t_includes/queue.o 

C_DEPS += \
./t_includes/I2C_core.d \
./t_includes/camera_func.d \
./t_includes/io.d \
./t_includes/mipi_bridge_config.d \
./t_includes/mipi_camera_config.d \
./t_includes/queue.d 


# Each subdirectory must supply rules for building sources it contributes
t_includes/%.o: ../t_includes/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler 4 [arm-linux-gnueabihf]'
	arm-linux-gnueabihf-gcc -Dsoc_cv_av -I"C:\altera\15.0\embedded\ip\altera\hps\altera_hps\hwlib\include" -I"C:\altera\15.0\embedded\ip\altera\hps\altera_hps\hwlib\include\soc_cv_av" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


