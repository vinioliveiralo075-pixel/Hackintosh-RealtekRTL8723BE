#include "RTL8723BE.h"

#define super IO80211Controller
OSDefineMetaClassAndStructors(RTL8723BE, IO80211Controller)

bool RTL8723BE::init(OSDictionary *properties) {
    if (!super::init(properties)) return false;
    pciDevice = NULL;
    _fCommandGate = NULL;
    return true;
}

bool RTL8723BE::start(IOService *provider) {
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) return false;

    if (!super::start(provider)) return false;

    pciDevice->setMemoryEnable(true);
    pciDevice->setBusMasterEnable(true);

    _fCommandGate = IOCommandGate::commandGate(this);
    if (_fCommandGate) {
        getWorkLoop()->addEventSource(_fCommandGate);
    }

    IOLog("RTL8723BE: Controladora de Wi-Fi nativa iniciada com sucesso.\n");
    return true;
}

void RTL8723BE::stop(IOService *provider) {
    if (_fCommandGate) {
        getWorkLoop()->removeEventSource(_fCommandGate);
        _fCommandGate->release();
        _fCommandGate = NULL;
    }
    
    if (pciDevice) {
        pciDevice->setMemoryEnable(false);
        pciDevice->setBusMasterEnable(false);
    }
    super::stop(provider);
}

void RTL8723BE::free() {
    super::free();
}

IOReturn RTL8723BE::actionSetPowerOff(OSObject *owner, void *arg1, void *arg2, void *arg3, void *arg4) {
    RTL8723BE *self = OSDynamicCast(RTL8723BE, owner);
    if (self) self->setPowerStateOff();
    return kIOReturnSuccess;
}

IOReturn RTL8723BE::actionSetPowerOn(OSObject *owner, void *arg1, void *arg2, void *arg3, void *arg4) {
    RTL8723BE *self = OSDynamicCast(RTL8723BE, owner);
    if (self) self->setPowerStateOn();
    return kIOReturnSuccess;
}

IOReturn RTL8723BE::setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker) {
    if (!_fCommandGate) return IOPMAckImplied;

    if (powerStateOrdinal == 0) _fCommandGate->runAction(actionSetPowerOff);
    else _fCommandGate->runAction(actionSetPowerOn);
    
    return IOPMAckImplied;
}

void RTL8723BE::setPowerStateOff() { }
void RTL8723BE::setPowerStateOn() { }

UInt32 RTL8723BE::outputPacket(mbuf_t m, void *param) {
    if (!m) return kIOReturnOutputDropped;
    return kIOReturnOutputSuccess;
}

IO80211VirtualInterface* RTL8723BE::createVirtualInterface(ether_addr *ether, UInt role) {
    return NULL; 
}
