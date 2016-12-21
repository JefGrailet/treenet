################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/treenet/aliasresolution/Fingerprint.cpp \
../src/treenet/aliasresolution/AliasResolver.cpp

OBJS += \
./src/treenet/aliasresolution/Fingerprint.o \
./src/treenet/aliasresolution/AliasResolver.o

CPP_DEPS += \
./src/treenet/aliasresolution/Fingerprint.d \
./src/treenet/aliasresolution/AliasResolver.d


# Each subdirectory must supply rules for building sources it contributes
src/treenet/aliasresolution/%.o: ../src/treenet/aliasresolution/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


