################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/prescanning/NetworkPrescanningUnit.cpp \
../src/treenet/prescanning/NetworkPrescanner.cpp

OBJS += \
./src/treenet/prescanning/NetworkPrescanningUnit.o \
./src/treenet/prescanning/NetworkPrescanner.o

CPP_DEPS += \
./src/treenet/prescanning/NetworkPrescanningUnit.d \
./src/treenet/prescanning/NetworkPrescanner.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/prescanning/%.o: ../src/treenet/prescanning/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


