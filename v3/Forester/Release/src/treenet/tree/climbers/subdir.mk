################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/tree/climbers/Climber.cpp \
../src/treenet/tree/climbers/Robin.cpp \
../src/treenet/tree/climbers/Magpie.cpp \
../src/treenet/tree/climbers/Sparrow.cpp \
../src/treenet/tree/climbers/Cuckoo.cpp \
../src/treenet/tree/climbers/Crow.cpp \
../src/treenet/tree/climbers/Cat.cpp \
../src/treenet/tree/climbers/Termite.cpp

OBJS += \
./src/treenet/tree/climbers/Climber.o \
./src/treenet/tree/climbers/Robin.o \
./src/treenet/tree/climbers/Magpie.o \
./src/treenet/tree/climbers/Sparrow.o \
./src/treenet/tree/climbers/Cuckoo.o \
./src/treenet/tree/climbers/Crow.o \
./src/treenet/tree/climbers/Cat.o \
./src/treenet/tree/climbers/Termite.o

CPP_DEPS += \
./src/treenet/tree/climbers/Climber.d \
./src/treenet/tree/climbers/Magpie.d \
./src/treenet/tree/climbers/Sparrow.d \
./src/treenet/tree/climbers/Robin.d \
./src/treenet/tree/climbers/Cuckoo.d \
./src/treenet/tree/climbers/Crow.d \
./src/treenet/tree/climbers/Cat.d \
./src/treenet/tree/climbers/Termite.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/tree/climbers/%.o: ../src/treenet/tree/climbers/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


