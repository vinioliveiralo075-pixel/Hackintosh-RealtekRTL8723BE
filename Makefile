MODULE_NAME = RTL8723BE
KEXT_BUNDLE = $(MODULE_NAME).kext
BUNDLE_ID = com.vini.driver.RTL8723BE

CXX = clang++

# Removemos o $(shell xcrun --show-sdk-path) para não usar o SDK da Apple
# Agora esperamos que a variável MAC_KERNEL_SDK seja passada via ambiente
SDKROOT ?= $(MAC_KERNEL_SDK)

CXXFLAGS = -mkernel -arch x86_64 -Wall -Wextra -std=c++14 \
           -DKERNEL=1 -DKERNEL_PRIVATE=1 \
           -fno-builtin -fno-exceptions -fno-rtti \
           -nostdinc -isysroot $(LEGACY_SDK) \
           -I$(LEGACY_SDK)/System/Library/Frameworks/Kernel.framework/Headers \
           -I$(LEGACY_SDK)/System/Library/Frameworks/Kernel.framework/Headers/bsd \
           -I$(LEGACY_SDK)/System/Library/Frameworks/Kernel.framework/Headers/IOKit/network \
           -I$(LEGACY_SDK)/System/Library/Extensions/IONetworkingFamily.kext/Contents/Headers

LDFLAGS = -Xlinker -kext -nostdlib -lkmod -lkmodc++ -lcc_kext

SRCS = RTL8723BE.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(KEXT_BUNDLE)

$(KEXT_BUNDLE): $(OBJS)
	@mkdir -p $(KEXT_BUNDLE)/Contents/MacOS
	$(CXX) $(LDFLAGS) $(OBJS) -o $(KEXT_BUNDLE)/Contents/MacOS/$(MODULE_NAME)
	@cp Info.plist $(KEXT_BUNDLE)/Contents/Info.plist
	@echo "SUCESSO! Kext compilada."

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf *.o $(KEXT_BUNDLE)