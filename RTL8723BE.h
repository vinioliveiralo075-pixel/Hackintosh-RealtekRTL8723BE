#ifndef _RTL8723BE_H_
#define _RTL8723BE_H_

// =============================================================================
// FORÇAR MODO KERNEL (Resolve erros de DriverKit / IOKitUser e OSClassLoadInformation)
// =============================================================================
#ifndef KERNEL
#define KERNEL 1
#endif
#ifndef KERNEL_PRIVATE
#define KERNEL_PRIVATE 1
#endif
#ifndef NVRAM_PRIVATE
#define NVRAM_PRIVATE 1
#endif

// =============================================================================
// INCLUDES DO KERNEL (Ordem estrita para evitar dependências circulares)
// =============================================================================
#include <libkern/c++/OSMetaClass.h>
#include <libkern/c++/OSObject.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>

// Includes da IONetworkingFamily (Rede)
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>

// KPI do Kernel para manipulação de pacotes de rede
#include <sys/kpi_mbuf.h>

// =============================================================================
// DECLARAÇÃO DA CLASSE DO CONTROLADOR
// =============================================================================
class com_vini_driver_RTL8723BE : public IOEthernetController {
    OSDeclareDefaultStructors(com_vini_driver_RTL8723BE)

private:
    IOEthernetInterface* netInterface;

public:
    // Ciclo de vida básico da Kext
    virtual bool init(OSDictionary *properties = 0) override;
    virtual void free() override;
    
    // Gestão do ciclo de vida no IOKit
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
    // Controlo do Hardware de Rede
    virtual IOReturn enable(IOService *provider) override;
    virtual IOReturn disable(IOService *provider) override;
    
    // Funções obrigatórias da IOEthernetController para evitar erro de vtable
    virtual IOReturn getHardwareAddress(IOEthernetAddress *addr) override;
    virtual IOReturn outputPacket(mbuf_t packet, void *param) override;
};

#endif /* _RTL8723BE_H_ */