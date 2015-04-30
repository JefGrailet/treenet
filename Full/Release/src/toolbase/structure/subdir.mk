################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/toolbase/structure/SubnetSiteNode.cpp \
../src/toolbase/structure/SubnetSite.cpp \
../src/toolbase/structure/SubnetSiteSet.cpp \
../src/toolbase/structure/InvalidSubnetException.cpp \
../src/toolbase/structure/NetworkTreeNode.cpp \
../src/toolbase/structure/NetworkTree.cpp \
../src/toolbase/structure/Router.cpp

OBJS += \
./src/toolbase/structure/SubnetSiteNode.o \
./src/toolbase/structure/SubnetSite.o \
./src/toolbase/structure/SubnetSiteSet.o \
./src/toolbase/structure/InvalidSubnetException.o \
./src/toolbase/structure/NetworkTreeNode.o \
./src/toolbase/structure/NetworkTree.o \
./src/toolbase/structure/Router.o

CPP_DEPS += \
./src/toolbase/structure/SubnetSiteNode.d \
./src/toolbase/structure/SubnetSite.d \
./src/toolbase/structure/SubnetSiteSet.d \
./src/toolbase/structure/InvalidSubnetException.d \
./src/toolbase/structure/NetworkTreeNode.d \
./src/toolbase/structure/NetworkTree.d \
./src/toolbase/structure/Router.d

# Each subdirectory must supply rules for building sources it contributes
src/toolbase/structure/%.o: ../src/toolbase/structure/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


