# Makefile otimizado para RTL8723BE Native Wi-Fi
CC = clang++
ARCH = -arch x86_64
SDK = /Users/runner/work/Hackintosh-RealtekRTL8723BE/Hackintosh-RealtekRTL8723BE/MacOSX10.15.sdk
KERNEL_HEADERS = $(SDK)/System/Library/Frameworks/Kernel.framework/Headers

CFLAGS = -mkernel -std=c++14 -fno-builtin -fno-exceptions -fno-rtti -nostdinc -Wall -Wextra

INCLUDES = -I$(KERNEL_HEADERS) \
           -I$(KERNEL_HEADERS)/bsd \
           -I$(KERNEL_HEADERS)/IOKit \
           -I$(KERNEL_HEADERS)/libkern \
           -I./Apple80211

SRC = RTL8723BE.cpp
OBJ = RTL8723BE.o

all: prepare $(OBJ)

prepare:
	@echo "Baixando Headers Privados de Wi-Fi da Apple..."
	@mkdir -p Apple80211/IOKit/network
	@curl -sL https://raw.githubusercontent.com/OpenIntelWireless/itlwm/master/Dependencies/Apple80211/IO80211Controller.h -o Apple80211/IOKit/network/IO80211Controller.h
	@curl -sL https://raw.githubusercontent.com/OpenIntelWireless/itlwm/master/Dependencies/Apple80211/IO80211Interface.h -o Apple80211/IOKit/network/IO80211Interface.h
	@curl -sL https://raw.githubusercontent.com/OpenIntelWireless/itlwm/master/Dependencies/Apple80211/IO80211VirtualInterface.h -o Apple80211/IOKit/network/IO80211VirtualInterface.h
	@curl -sL https://raw.githubusercontent.com/OpenIntelWireless/itlwm/master/Dependencies/Apple80211/apple80211_var.h -o Apple80211/IOKit/network/apple80211_var.h

$(OBJ): $(SRC)
	$(CC) $(ARCH) $(CFLAGS) $(INCLUDES) -c $(SRC) -o $(OBJ)

clean:
	rm -f *.o
	rm -rf Apple80211
