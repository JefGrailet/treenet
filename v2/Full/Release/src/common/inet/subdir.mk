################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/common/inet/InetAddress.cpp \
../src/common/inet/InetAddressException.cpp \
../src/common/inet/InetAddressSet.cpp \
../src/common/inet/NetworkAddress.cpp \
../src/common/inet/NetworkAddressSet.cpp 

OBJS += \
./src/common/inet/InetAddress.o \
./src/common/inet/InetAddressException.o \
./src/common/inet/InetAddressSet.o \
./src/common/inet/NetworkAddress.o \
./src/common/inet/NetworkAddressSet.o 

CPP_DEPS += \
./src/common/inet/InetAddress.d \
./src/common/inet/InetAddressException.d \
./src/common/inet/InetAddressSet.d \
./src/common/inet/NetworkAddress.d \
./src/common/inet/NetworkAddressSet.d 


# Each subdirectory must supply rules for building sources it contributes
src/common/inet/%.o: ../src/common/inet/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -m32 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


