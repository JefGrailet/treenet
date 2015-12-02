################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/toolbase/structure/IPTableEntry.cpp \
../src/toolbase/structure/IPLookUpTable.cpp \
../src/toolbase/structure/RouterInterface.cpp \
../src/toolbase/structure/InferredRouter.cpp \
../src/toolbase/structure/Router.cpp

OBJS += \
./src/toolbase/structure/IPTableEntry.o \
./src/toolbase/structure/IPLookUpTable.o \
./src/toolbase/structure/RouterInterface.o \
./src/toolbase/structure/InferredRouter.o \
./src/toolbase/structure/Router.o

CPP_DEPS += \
./src/toolbase/structure/IPTableEntry.d \
./src/toolbase/structure/IPLookUpTable.d \
./src/toolbase/structure/RouterInterface.d \
./src/toolbase/structure/InferredRouter.d \
./src/toolbase/structure/Router.d

# Each subdirectory must supply rules for building sources it contributes
src/toolbase/structure/%.o: ../src/toolbase/structure/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


