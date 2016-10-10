################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/tree/growth/classic/ClassicGrower.cpp \
../src/treenet/tree/growth/classic/ParisTracerouteTask.cpp

OBJS += \
./src/treenet/tree/growth/classic/ClassicGrower.o \
./src/treenet/tree/growth/classic/ParisTracerouteTask.o

CPP_DEPS += \
./src/treenet/tree/growth/classic/ClassicGrower.d \
./src/treenet/tree/growth/classic/ParisTracerouteTask.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/tree/growth/classic/%.o: ../src/treenet/tree/growth/classic/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


