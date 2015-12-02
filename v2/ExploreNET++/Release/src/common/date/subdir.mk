################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/common/date/TimeVal.cpp 

OBJS += \
./src/common/date/TimeVal.o 

CPP_DEPS += \
./src/common/date/TimeVal.d 


# Each subdirectory must supply rules for building sources it contributes
src/common/date/%.o: ../src/common/date/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


