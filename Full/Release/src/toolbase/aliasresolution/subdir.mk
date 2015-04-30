################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/toolbase/aliasresolution/AliasResolver.cpp \
../src/toolbase/aliasresolution/IPIDResolverUnit.cpp \
../src/toolbase/aliasresolution/ReverseDNSResolverUnit.cpp

OBJS += \
./src/toolbase/aliasresolution/AliasResolver.o \
./src/toolbase/aliasresolution/IPIDResolverUnit.o \
./src/toolbase/aliasresolution/ReverseDNSResolverUnit.o

CPP_DEPS += \
./src/toolbase/aliasresolution/AliasResolver.d \
./src/toolbase/aliasresolution/IPIDResolverUnit.d \
./src/toolbase/aliasresolution/ReverseDNSResolverUnit.d


# Each subdirectory must supply rules for building sources it contributes
src/toolbase/aliasresolution/%.o: ../src/toolbase/aliasresolution/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


