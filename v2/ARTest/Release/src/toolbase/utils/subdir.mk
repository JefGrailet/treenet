################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/toolbase/utils/TargetParser.cpp \
../src/toolbase/utils/IPDictionnaryParser.cpp

OBJS += \
./src/toolbase/utils/TargetParser.o \
./src/toolbase/utils/IPDictionnaryParser.o

CPP_DEPS += \
./src/toolbase/utils/TargetParser.d \
./src/toolbase/utils/IPDictionnaryParser.d


# Each subdirectory must supply rules for building sources it contributes
src/toolbase/utils/%.o: ../src/toolbase/utils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


