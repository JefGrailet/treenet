################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/graph/type-ers/proj-r/RouterGraph.cpp

OBJS += \
./src/treenet/graph/type-ers/proj-r/RouterGraph.o

CPP_DEPS += \
./src/treenet/graph/type-ers/proj-r/RouterGraph.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/graph/type-ers/proj-r/%.o: ../src/treenet/graph/type-ers/proj-r/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


