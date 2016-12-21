################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/graph/type-ns/proj-n/NeighborhoodGraph.cpp

OBJS += \
./src/treenet/graph/type-ns/proj-n/NeighborhoodGraph.o

CPP_DEPS += \
./src/treenet/graph/type-ns/proj-n/NeighborhoodGraph.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/graph/type-ns/proj-n/%.o: ../src/treenet/graph/type-ns/proj-n/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


