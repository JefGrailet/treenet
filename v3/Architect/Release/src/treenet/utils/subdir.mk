################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/utils/SubnetParser.cpp \
../src/treenet/utils/IPDictionnaryParser.cpp

OBJS += \
./src/treenet/utils/SubnetParser.o \
./src/treenet/utils/IPDictionnaryParser.o

CPP_DEPS += \
./src/treenet/utils/SubnetParser.d \
./src/treenet/utils/IPDictionnaryParser.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/utils/%.o: ../src/treenet/utils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


