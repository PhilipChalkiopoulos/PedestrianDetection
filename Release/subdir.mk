################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../fbdraw.cpp \
../main.cpp 

OBJS += \
./fbdraw.o \
./main.o 

CPP_DEPS += \
./fbdraw.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler 4 [arm-linux-gnueabihf]'
	arm-linux-gnueabihf-g++ -std=c++0x -Dsoc_cv_av -I/opt/arm-opencv/include/opencv -I/home/ellabuser/altera/15.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av -I/home/ellabuser/altera/15.1/embedded/ip/altera/hps/altera_hps/hwlib/include -I/opt/arm-opencv/include -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


