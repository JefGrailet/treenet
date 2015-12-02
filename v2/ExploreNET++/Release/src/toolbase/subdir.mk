################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/toolbase/ToolBase.cpp \
../src/toolbase/ExploreNETEnvironment.cpp

OBJS += \
./src/toolbase/ToolBase.o \
./src/toolbase/ExploreNETEnvironment.o

CPP_DEPS += \
./src/toolbase/ToolBase.d \
./src/toolbase/ExploreNETEnvironment.d


# Each subdirectory must supply rules for building sources it contributes
src/toolbase/%.o: ../src/toolbase/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


