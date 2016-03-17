################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/aliasresolution/AliasHintCollector.cpp \
../src/aliasresolution/IPIDTuple.cpp \
../src/aliasresolution/IPIDUnit.cpp \
../src/aliasresolution/IPIDCollector.cpp \
../src/aliasresolution/ReverseDNSUnit.cpp \
../src/aliasresolution/Fingerprint.cpp \
../src/aliasresolution/AliasResolver.cpp

OBJS += \
./src/aliasresolution/AliasHintCollector.o \
./src/aliasresolution/IPIDTuple.o \
./src/aliasresolution/IPIDUnit.o \
./src/aliasresolution/IPIDCollector.o \
./src/aliasresolution/ReverseDNSUnit.o \
./src/aliasresolution/Fingerprint.o \
./src/aliasresolution/AliasResolver.o

CPP_DEPS += \
./src/aliasresolution/AliasHintCollector.d \
./src/aliasresolution/IPIDTuple.d \
./src/aliasresolution/IPIDUnit.d \
./src/aliasresolution/IPIDCollector.d \
./src/aliasresolution/ReverseDNSUnit.d \
./src/aliasresolution/Fingerprint.d \
./src/aliasresolution/AliasResolver.d


# Each subdirectory must supply rules for building sources it contributes
src/aliasresolution/%.o: ../src/aliasresolution/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


