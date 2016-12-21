################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/graph/Edge.cpp \
../src/treenet/graph/Vertice.cpp \
../src/treenet/graph/Graph.cpp \
../src/treenet/graph/SimpleGraph.cpp \
../src/treenet/graph/BipartiteGraph.cpp \
../src/treenet/graph/L2Device.cpp \
../src/treenet/graph/L3Device.cpp \
../src/treenet/graph/Neighborhood.cpp \
../src/treenet/graph/Subnet.cpp

OBJS += \
./src/treenet/graph/Edge.o \
./src/treenet/graph/Vertice.o \
./src/treenet/graph/Graph.o \
./src/treenet/graph/SimpleGraph.o \
./src/treenet/graph/BipartiteGraph.o \
./src/treenet/graph/L2Device.o \
./src/treenet/graph/L3Device.o \
./src/treenet/graph/Neighborhood.o \
./src/treenet/graph/Subnet.o

CPP_DEPS += \
./src/treenet/graph/Edge.d \
./src/treenet/graph/Vertice.d \
./src/treenet/graph/Graph.d \
./src/treenet/graph/SimpleGraph.d \
./src/treenet/graph/BipartiteGraph.d \
./src/treenet/graph/L2Device.d \
./src/treenet/graph/L3Device.d \
./src/treenet/graph/Neighborhood.d \
./src/treenet/graph/Subnet.d \


# Each subdirectory must supply rules for building sources it contributes
src/treenet/graph/%.o: ../src/treenet/graph/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


