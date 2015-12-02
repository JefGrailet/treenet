################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/toolbase/prescanning/NetworkPrescanningUnit.cpp \
../src/toolbase/prescanning/NetworkPrescanner.cpp

OBJS += \
./src/toolbase/prescanning/NetworkPrescanningUnit.o \
./src/toolbase/prescanning/NetworkPrescanner.o

CPP_DEPS += \
./src/toolbase/prescanning/NetworkPrescanningUnit.d \
./src/toolbase/prescanning/NetworkPrescanner.d


# Each subdirectory must supply rules for building sources it contributes
src/toolbase/prescanning/%.o: ../src/toolbase/prescanning/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


