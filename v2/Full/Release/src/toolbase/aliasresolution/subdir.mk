################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/toolbase/aliasresolution/AliasHintCollector.cpp \
../src/toolbase/aliasresolution/IPIDTuple.cpp \
../src/toolbase/aliasresolution/IPIDUnit.cpp \
../src/toolbase/aliasresolution/IPIDCollector.cpp \
../src/toolbase/aliasresolution/ReverseDNSUnit.cpp \
../src/toolbase/aliasresolution/Fingerprint.cpp \
../src/toolbase/aliasresolution/AliasResolver.cpp

OBJS += \
./src/toolbase/aliasresolution/AliasHintCollector.o \
./src/toolbase/aliasresolution/IPIDTuple.o \
./src/toolbase/aliasresolution/IPIDUnit.o \
./src/toolbase/aliasresolution/IPIDCollector.o \
./src/toolbase/aliasresolution/ReverseDNSUnit.o \
./src/toolbase/aliasresolution/Fingerprint.o \
./src/toolbase/aliasresolution/AliasResolver.o

CPP_DEPS += \
./src/toolbase/aliasresolution/AliasHintCollector.d \
./src/toolbase/aliasresolution/IPIDTuple.d \
./src/toolbase/aliasresolution/IPIDUnit.d \
./src/toolbase/aliasresolution/IPIDCollector.d \
./src/toolbase/aliasresolution/ReverseDNSUnit.d \
./src/toolbase/aliasresolution/Fingerprint.d \
./src/toolbase/aliasresolution/AliasResolver.d


# Each subdirectory must supply rules for building sources it contributes
src/toolbase/aliasresolution/%.o: ../src/toolbase/aliasresolution/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


