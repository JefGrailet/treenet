################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/graph/type-ers/ERGraph.cpp \
../src/treenet/graph/type-ers/RSGraph.cpp \
../src/treenet/graph/type-ers/ERSProcesser.cpp

OBJS += \
./src/treenet/graph/type-ers/ERGraph.o \
./src/treenet/graph/type-ers/RSGraph.o \
./src/treenet/graph/type-ers/ERSProcesser.o

CPP_DEPS += \
./src/treenet/graph/type-ers/ERGraph.d \
./src/treenet/graph/type-ers/RSGraph.d \
./src/treenet/graph/type-ers/ERSProcesser.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/graph/type-ers/%.o: ../src/treenet/graph/type-ers/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


