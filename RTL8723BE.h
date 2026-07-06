#ifndef _RTL8723BE_H
#define _RTL8723BE_H

#include <IOKit/network/IO80211Controller.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOLib.h>

class RTL8723BE : public IO80211Controller {
    OSDeclareDefaultStructors(RTL8723BE)

public:
    virtual bool init(OSDictionary *properties = 0) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    virtual void free() override;

    static IOReturn actionSetPowerOff(OSObject *owner, void *arg1, void *arg2, void *arg3, void *arg4);
    static IOReturn actionSetPowerOn(OSObject *owner, void *arg1, void *arg2, void *arg3, void *arg4);

    virtual UInt32 outputPacket(mbuf_t m, void *param) override;
    virtual IO80211VirtualInterface* createVirtualInterface(ether_addr *ether, UInt role) override;

protected:
    IOPCIDevice *pciDevice;
    IOCommandGate *_fCommandGate;
    
    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker) override;
    void setPowerStateOff();
    void setPowerStateOn();
};

#endif
