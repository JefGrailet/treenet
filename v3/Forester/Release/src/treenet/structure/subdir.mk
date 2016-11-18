################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/structure/IPTableEntry.cpp \
../src/treenet/structure/IPLookUpTable.cpp \
../src/treenet/structure/SubnetSiteNode.cpp \
../src/treenet/structure/RouteInterface.cpp \
../src/treenet/structure/SubnetSite.cpp \
../src/treenet/structure/SubnetSiteSet.cpp \
../src/treenet/structure/RouterInterface.cpp \
../src/treenet/structure/Router.cpp

OBJS += \
./src/treenet/structure/IPTableEntry.o \
./src/treenet/structure/IPLookUpTable.o \
./src/treenet/structure/SubnetSiteNode.o \
./src/treenet/structure/RouteInterface.o \
./src/treenet/structure/SubnetSite.o \
./src/treenet/structure/SubnetSiteSet.o \
./src/treenet/structure/RouterInterface.o \
./src/treenet/structure/Router.o

CPP_DEPS += \
./src/treenet/structure/IPTableEntry.d \
./src/treenet/structure/IPLookUpTable.d \
./src/treenet/structure/SubnetSiteNode.d \
./src/treenet/structure/RouteInterface.d \
./src/treenet/structure/SubnetSite.d \
./src/treenet/structure/SubnetSiteSet.d \
./src/treenet/structure/RouterInterface.d \
./src/treenet/structure/Router.d

# Each subdirectory must supply rules for building sources it contributes
src/treenet/structure/%.o: ../src/treenet/structure/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


