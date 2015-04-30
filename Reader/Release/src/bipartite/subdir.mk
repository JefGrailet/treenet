################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/bipartite/BipartiteGraph.cpp \
../src/bipartite/BipartiteRouter.cpp \
../src/bipartite/BipartiteSubnet.cpp \
../src/bipartite/BipartiteSwitch.cpp \
../src/bipartite/LinkRouterSubnet.cpp \
../src/bipartite/LinkSwitchRouter.cpp

OBJS += \
./src/bipartite/BipartiteGraph.o \
./src/bipartite/BipartiteRouter.o \
./src/bipartite/BipartiteSubnet.o \
./src/bipartite/BipartiteSwitch.o \
./src/bipartite/LinkRouterSubnet.o \
./src/bipartite/LinkSwitchRouter.o

CPP_DEPS += \
./src/bipartite/BipartiteGraph.d \
./src/bipartite/BipartiteRouter.d \
./src/bipartite/BipartiteSubnet.d \
./src/bipartite/BipartiteSwitch.d \
./src/bipartite/LinkRouterSubnet.d \
./src/bipartite/LinkSwitchRouter.d

# Each subdirectory must supply rules for building sources it contributes
src/toolbase/structure/%.o: ../src/structure/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


