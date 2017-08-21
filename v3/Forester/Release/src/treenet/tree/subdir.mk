################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/tree/Aggregate.cpp \
../src/treenet/tree/InvalidSubnetException.cpp \
../src/treenet/tree/NetworkTreeNode.cpp \
../src/treenet/tree/NetworkTree.cpp \
../src/treenet/tree/SubnetMapEntry.cpp \
../src/treenet/tree/Soil.cpp

OBJS += \
./src/treenet/tree/Aggregate.o \
./src/treenet/tree/InvalidSubnetException.o \
./src/treenet/tree/NetworkTreeNode.o \
./src/treenet/tree/NetworkTree.o \
./src/treenet/tree/SubnetMapEntry.o \
./src/treenet/tree/Soil.o

CPP_DEPS += \
./src/treenet/tree/Aggregate.d \
./src/treenet/tree/InvalidSubnetException.d \
./src/treenet/tree/NetworkTreeNode.d \
./src/treenet/tree/NetworkTree.d \
./src/treenet/tree/SubnetMapEntry.d \
./src/treenet/tree/Soil.d

# Each subdirectory must supply rules for building sources it contributes
src/treenet/tree/%.o: ../src/treenet/tree/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


