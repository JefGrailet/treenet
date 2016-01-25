################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/structure/IPTableEntry.cpp \
../src/structure/IPLookUpTable.cpp \
../src/structure/SubnetSiteNode.cpp \
../src/structure/SubnetSite.cpp \
../src/structure/SubnetSiteSet.cpp \
../src/structure/InvalidSubnetException.cpp \
../src/structure/NetworkTreeNode.cpp \
../src/structure/NetworkTree.cpp \
../src/structure/RouterInterface.cpp \
../src/structure/Router.cpp

OBJS += \
./src/structure/IPTableEntry.o \
./src/structure/IPLookUpTable.o \
./src/structure/SubnetSiteNode.o \
./src/structure/SubnetSite.o \
./src/structure/SubnetSiteSet.o \
./src/structure/InvalidSubnetException.o \
./src/structure/NetworkTreeNode.o \
./src/structure/NetworkTree.o \
./src/structure/RouterInterface.o \
./src/structure/Router.o

CPP_DEPS += \
./src/structure/IPTableEntry.d \
./src/structure/IPLookUpTable.d \
./src/structure/SubnetSiteNode.d \
./src/structure/SubnetSite.d \
./src/structure/SubnetSiteSet.d \
./src/structure/InvalidSubnetException.d \
./src/structure/NetworkTreeNode.d \
./src/structure/NetworkTree.d \
./src/structure/RouterInterface.d \
./src/structure/Router.d

# Each subdirectory must supply rules for building sources it contributes
src/toolbase/structure/%.o: ../src/structure/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


