################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/subnetrefinement/SubnetRefiner.cpp \
../src/treenet/subnetrefinement/ProbesDispatcher.cpp \
../src/treenet/subnetrefinement/ProbeUnit.cpp 

OBJS += \
./src/treenet/subnetrefinement/SubnetRefiner.o \
./src/treenet/subnetrefinement/ProbesDispatcher.o \
./src/treenet/subnetrefinement/ProbeUnit.o 

CPP_DEPS += \
./src/treenet/subnetrefinement/SubnetRefiner.d \
./src/treenet/subnetrefinement/ProbesDispatcher.d \
./src/treenet/subnetrefinement/ProbeUnit.d 


# Each subdirectory must supply rules for building sources it contributes
src/treenet/subnetrefinement/%.o: ../src/treenet/subnetrefinement/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


