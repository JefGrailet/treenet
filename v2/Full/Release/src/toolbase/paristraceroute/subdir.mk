################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/toolbase/paristraceroute/ParisTracerouteTask.cpp

OBJS += \
./src/toolbase/paristraceroute/ParisTracerouteTask.o

CPP_DEPS += \
./src/toolbase/paristraceroute/ParisTracerouteTask.d


# Each subdirectory must supply rules for building sources it contributes
src/toolbase/paristraceroute/%.o: ../src/toolbase/paristraceroute/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


