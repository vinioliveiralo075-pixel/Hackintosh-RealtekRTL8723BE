#ifndef _RTL8723BE_H_
#define _RTL8723BE_H_

// ESSAS 4 LINHAS PRECISAM SER AS PRIMEIRAS! 
// Elas impedem o erro "OSClassLoadInformation"
#ifndef KERNEL
#define KERNEL 1
#endif
#ifndef KERNEL_PRIVATE
#define KERNEL_PRIVATE 1
#endif

#include <libkern/c++/OSMetaClass.h>
#include <libkern/c++/OSObject.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IO80211Controller.h>
#include <IOKit/network/IO80211VirtualInterface.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <sys/kpi_mbuf.h> 
#include <net/ethernet.h>

class com_vini_driver_RTL8723BE : public IO80211Controller {
    OSDeclareDefaultStructors(com_vini_driver_RTL8723BE)

private:
    IOPCIDevice* pciDevice;
    IOMemoryMap* mmioMap;
    volatile uint8_t* mmioBase;
    
    IOCommandGate* _fCommandGate;
    IO80211VirtualInterface* fNetIf;
    IO80211VirtualInterface* fAWDLInterface;

    bool magicPacketEnabled;
    IOPMPowerState pmPowerState;
    IOService* pmPolicyMaker;
    thread_call_t powerOffThreadCall;
    thread_call_t powerOnThreadCall;

    static void handleSetPowerStateOff(thread_call_param_t param0, thread_call_param_t param1);
    static void handleSetPowerStateOn(thread_call_param_t param0, thread_call_param_t param1);
    void setPowerStateOff();
    void setPowerStateOn();

public:
    virtual bool init(OSDictionary *properties = 0) override;
    virtual void free() override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;

    // Métodos obrigatórios de Rede para não dar Kernel Panic
    virtual IOReturn enable(IOService *provider) override;
    virtual IOReturn disable(IOService *provider) override;

    // IO80211Controller Overrides
    virtual IOReturn setHardwareAddress(const IOEthernetAddress *addrP) override;
    virtual IOReturn getHardwareAddressForInterface(IO80211Interface *netif, IOEthernetAddress *addr) override;
    virtual UInt32 outputPacket(mbuf_t m, void *param) override;
    virtual int outputRaw80211Packet(IO80211Interface *interface, mbuf_t m) override;
    virtual int bpfOutputPacket(OSObject *object, UInt dltType, mbuf_t m) override;
    virtual void requestPacketTx(void *object, UInt action) override;
    virtual IO80211VirtualInterface *createVirtualInterface(ether_addr *ether, UInt role) override;
    virtual SInt32 enableVirtualInterface(IO80211VirtualInterface *interface) override;
    virtual SInt32 disableVirtualInterface(IO80211VirtualInterface *interface) override;
    virtual UInt32 getFeatures() const override;
    virtual IOReturn setPromiscuousMode(IOEnetPromiscuousMode mode) override;
    virtual IOReturn setMulticastMode(IOEnetMulticastMode mode) override;
    virtual IOReturn setMulticastList(IOEthernetAddress* addr, UInt32 len) override;
    virtual IOReturn getPacketFilters(const OSSymbol *group, UInt32 *filters) const override;
    virtual IOReturn setWakeOnMagicPacket(bool active) override;

    // Power Management
    virtual IOReturn registerWithPolicyMaker(IOService *policyMaker);
    virtual void unregistPM();
    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker) override;
    virtual IOReturn tsleepHandler(OSObject* owner, void* arg0, void* arg1, void* arg2, void* arg3);
};

#endif /* _RTL8723BE_H_ */
