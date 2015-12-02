################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/aliasresolution/AliasResolver.cpp \
../src/aliasresolution/IPIDResolverUnit.cpp \
../src/aliasresolution/ReverseDNSResolverUnit.cpp

OBJS += \
./src/aliasresolution/AliasResolver.o \
./src/aliasresolution/IPIDResolverUnit.o \
./src/aliasresolution/ReverseDNSResolverUnit.o

CPP_DEPS += \
./src/aliasresolution/AliasResolver.d \
./src/aliasresolution/IPIDResolverUnit.d \
./src/aliasresolution/ReverseDNSResolverUnit.d


# Each subdirectory must supply rules for building sources it contributes
src/aliasresolution/%.o: ../src/aliasresolution/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


