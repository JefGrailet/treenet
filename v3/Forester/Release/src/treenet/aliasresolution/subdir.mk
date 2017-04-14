################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/aliasresolution/AliasHintCollector.cpp \
../src/treenet/aliasresolution/IPIDTuple.cpp \
../src/treenet/aliasresolution/IPIDUnit.cpp \
../src/treenet/aliasresolution/UDPUnreachablePortUnit.cpp \
../src/treenet/aliasresolution/TimestampCheckUnit.cpp \
../src/treenet/aliasresolution/ReverseDNSUnit.cpp \
../src/treenet/aliasresolution/Fingerprint.cpp \
../src/treenet/aliasresolution/AliasResolver.cpp

OBJS += \
./src/treenet/aliasresolution/AliasHintCollector.o \
./src/treenet/aliasresolution/IPIDTuple.o \
./src/treenet/aliasresolution/IPIDUnit.o \
./src/treenet/aliasresolution/UDPUnreachablePortUnit.o \
./src/treenet/aliasresolution/TimestampCheckUnit.o \
./src/treenet/aliasresolution/ReverseDNSUnit.o \
./src/treenet/aliasresolution/Fingerprint.o \
./src/treenet/aliasresolution/AliasResolver.o

CPP_DEPS += \
./src/treenet/aliasresolution/AliasHintCollector.d \
./src/treenet/aliasresolution/IPIDTuple.d \
./src/treenet/aliasresolution/IPIDUnit.d \
./src/treenet/aliasresolution/UDPUnreachablePortUnit.d \
./src/treenet/aliasresolution/TimestampCheckUnit.d \
./src/treenet/aliasresolution/ReverseDNSUnit.d \
./src/treenet/aliasresolution/Fingerprint.d \
./src/treenet/aliasresolution/AliasResolver.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/aliasresolution/%.o: ../src/treenet/aliasresolution/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


