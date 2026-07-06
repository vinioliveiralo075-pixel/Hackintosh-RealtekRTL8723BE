#include "RTL8723BE.h"

#define super IO80211Controller
OSDefineMetaClassAndStructors(com_vini_driver_RTL8723BE, IO80211Controller)

// =============================================================================
// 1. CICLO DE VIDA E INICIALIZAÇÃO PCI
// =============================================================================

bool com_vini_driver_RTL8723BE::init(OSDictionary *properties) {
    if (!super::init(properties)) return false;
    
    pciDevice = NULL;
    mmioMap = NULL;
    mmioBase = NULL;
    _fCommandGate = NULL;
    fNetIf = NULL;
    fAWDLInterface = NULL;
    pmPowerState = kPowerStateOff;
    powerOffThreadCall = NULL;
    powerOnThreadCall = NULL;
    magicPacketEnabled = false;
    
    return true;
}

bool com_vini_driver_RTL8723BE::start(IOService *provider) {
    if (!super::start(provider)) return false;

    IOLog("RTL8723BE: Tentando inicializar kext Wi-Fi...\n");

    // 1. Mapeia o dispositivo PCI
    pciDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDevice) {
        IOLog("RTL8723BE: Falha ao obter provedor IOPCIDevice!\n");
        return false;
    }

    // 2. Habilita o barramento PCI
    pciDevice->setMemoryEnable(true);
    pciDevice->setBusMasterEnable(true);

    // 3. Mapeamento de Memória Física (Crucial para falar com a placa)
    mmioMap = pciDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!mmioMap) {
        IOLog("RTL8723BE: Falha ao mapear memoria PCI!\n");
        return false;
    }
    mmioBase = (volatile uint8_t*)mmioMap->getVirtualAddress();

    // 4. Inicializa a CommandGate com proteção de WorkLoop nulo
    _fCommandGate = IOCommandGate::commandGate(this);
    if (!_fCommandGate) {
        IOLog("RTL8723BE: Falha ao criar CommandGate!\n");
        return false;
    }
    
    IOWorkLoop *workLoop = getWorkLoop();
    if (!workLoop) {
        IOLog("RTL8723BE: Falha ao obter WorkLoop!\n");
        _fCommandGate->release();
        return false;
    }
    workLoop->addEventSource(_fCommandGate);

    // Registra o gerenciamento de energia
    registerWithPolicyMaker(this);
    
    IOLog("RTL8723BE: Inicializacao base concluida com sucesso.\n");
    return true;
}

void com_vini_driver_RTL8723BE::stop(IOService *provider) {
    IOLog("RTL8723BE: Parando controladora...\n");
    
    unregistPM();
    
    if (_fCommandGate) {
        IOWorkLoop *workLoop = getWorkLoop();
        if (workLoop) workLoop->removeEventSource(_fCommandGate);
        _fCommandGate->release();
        _fCommandGate = NULL;
    }

    if (mmioMap) {
        mmioMap->release();
        mmioMap = NULL;
    }

    if (pciDevice) {
        pciDevice->setMemoryEnable(false);
        pciDevice->setBusMasterEnable(false);
    }
    
    super::stop(provider);
}

void com_vini_driver_RTL8723BE::free() {
    super::free();
}

// =============================================================================
// 1.5 METODOS OBRIGATORIOS DE REDE (Ligam e Desligam a interface)
// =============================================================================
IOReturn com_vini_driver_RTL8723BE::enable(IOService *provider) {
    return kIOReturnSuccess;
}

IOReturn com_vini_driver_RTL8723BE::disable(IOService *provider) {
    return kIOReturnSuccess;
}

// =============================================================================
// 2. ENDEREÇO MAC
// =============================================================================

IOReturn com_vini_driver_RTL8723BE::setHardwareAddress(const IOEthernetAddress *addrP) {
    if (!fNetIf || !addrP) return kIOReturnError;
    return kIOReturnSuccess;
}

IOReturn com_vini_driver_RTL8723BE::getHardwareAddressForInterface(IO80211Interface *netif, IOEthernetAddress *addr) {
    if (!addr) return kIOReturnBadArgument;
    // MAC Address temporário seguro para testes
    addr->bytes[0] = 0x00; addr->bytes[1] = 0x11; addr->bytes[2] = 0x22;
    addr->bytes[3] = 0x33; addr->bytes[4] = 0x44; addr->bytes[5] = 0x55;
    return kIOReturnSuccess;
}

// =============================================================================
// 3. FLUXO DE TRANSMISSÃO E BPF (Corrigido para evitar Kernel Panic)
// =============================================================================

UInt32 com_vini_driver_RTL8723BE::outputPacket(mbuf_t m, void *param) {
    if (m == NULL) return kIOReturnOutputDropped;
    mbuf_freem(m); // Correção nativa do macOS
    return kIOReturnOutputSuccess;
}

int com_vini_driver_RTL8723BE::outputRaw80211Packet(IO80211Interface *interface, mbuf_t m) {
    if (m) mbuf_freem(m); // Correção nativa do macOS
    return kIOReturnOutputDropped;
}

int com_vini_driver_RTL8723BE::bpfOutputPacket(OSObject *object, UInt dltType, mbuf_t m) {
    if (m) mbuf_freem(m);
    return 1;
}

void com_vini_driver_RTL8723BE::requestPacketTx(void *object, UInt action) {
    // Logica reservada
}

// =============================================================================
// 4. INTERFACES VIRTUAIS
// =============================================================================

IO80211VirtualInterface *com_vini_driver_RTL8723BE::createVirtualInterface(ether_addr *ether, UInt role) {
    if (role - 1 > 3) return super::createVirtualInterface(ether, role);
        
    IO80211VirtualInterface *inf = new IO80211VirtualInterface;
    if (inf) {
        if (inf->init(this, ether, role, role == APPLE80211_VIF_AWDL ? "awdl" : "p2p")) {
            if (role == APPLE80211_VIF_AWDL) fAWDLInterface = inf;
            return inf;
        }
        inf->release();
    }
    return NULL;
}

SInt32 com_vini_driver_RTL8723BE::enableVirtualInterface(IO80211VirtualInterface *interface) {
    SInt32 ret = super::enableVirtualInterface(interface);
    if (!ret) {
#if __IO80211_TARGET >= __MAC_13_0
        interface->setEnabledBySystem(true);
#endif
        interface->setLinkState(kIO80211NetworkLinkUp, 0);
        interface->postMessage(APPLE80211_M_LINK_CHANGED);
        return kIOReturnSuccess;
    }
    return ret;
}

SInt32 com_vini_driver_RTL8723BE::disableVirtualInterface(IO80211VirtualInterface *interface) {
    SInt32 ret = super::disableVirtualInterface(interface);
    if (!ret) {
        interface->setLinkState(kIO80211NetworkLinkDown, 0);
        interface->postMessage(APPLE80211_M_LINK_CHANGED);
        return kIOReturnSuccess;
    }
    return ret;
}

// =============================================================================
// 5. FILTROS E CAPACIDADES
// =============================================================================

UInt32 com_vini_driver_RTL8723BE::getFeatures() const { return 0; }
IOReturn com_vini_driver_RTL8723BE::setPromiscuousMode(IOEnetPromiscuousMode mode) { return kIOReturnSuccess; }
IOReturn com_vini_driver_RTL8723BE::setMulticastMode(IOEnetMulticastMode mode) { return kIOReturnSuccess; }
IOReturn com_vini_driver_RTL8723BE::setMulticastList(IOEthernetAddress* addr, UInt32 len) { return kIOReturnSuccess; }

IOReturn com_vini_driver_RTL8723BE::getPacketFilters(const OSSymbol *group, UInt32 *filters) const {
    if (group == gIOEthernetWakeOnLANFilterGroup && magicPacketEnabled)
        *filters = kIOEthernetWakeOnMagicPacket;
    else if (group == gIONetworkFilterGroup)
        *filters = kIOPacketFilterMulticast | kIOPacketFilterPromiscuous;
    else
        return super::getPacketFilters(group, filters);
    return kIOReturnSuccess;
}

IOReturn com_vini_driver_RTL8723BE::setWakeOnMagicPacket(bool active) {
    magicPacketEnabled = active;
    return kIOReturnSuccess;
}

// =============================================================================
// 6. GERENCIAMENTO DE ENERGIA
// =============================================================================

static IOPMPowerState powerStateArray[kPowerStateCount] = {
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

IOReturn com_vini_driver_RTL8723BE::registerWithPolicyMaker(IOService *policyMaker) {
    pmPowerState = kPowerStateOn;
    pmPolicyMaker = policyMaker;
    powerOffThreadCall = thread_call_allocate((thread_call_func_t)handleSetPowerStateOff, (thread_call_param_t)this);
    powerOnThreadCall  = thread_call_allocate((thread_call_func_t)handleSetPowerStateOn, (thread_call_param_t)this);
    return pmPolicyMaker->registerPowerDriver(this, powerStateArray, kPowerStateCount);
}

void com_vini_driver_RTL8723BE::unregistPM() {
    if (powerOffThreadCall) { thread_call_free(powerOffThreadCall); powerOffThreadCall = NULL; }
    if (powerOnThreadCall) { thread_call_free(powerOnThreadCall); powerOnThreadCall = NULL; }
}

IOReturn com_vini_driver_RTL8723BE::setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker) {
    if (pmPowerState == powerStateOrdinal) return IOPMAckImplied;
    IOReturn result = IOPMAckImplied;
        
    switch (powerStateOrdinal) {
        case kPowerStateOff:
            if (powerOffThreadCall) { retain(); if (thread_call_enter(powerOffThreadCall)) release(); result = 5000000; }
            break;
        case kPowerStateOn:
            if (powerOnThreadCall) { retain(); if (thread_call_enter(powerOnThreadCall)) release(); result = 5000000; }
            break;
    }
    return result;
}

void com_vini_driver_RTL8723BE::handleSetPowerStateOff(thread_call_param_t param0, thread_call_param_t param1) {
    com_vini_driver_RTL8723BE *self = (com_vini_driver_RTL8723BE *)param0;
    if (param1 == 0) self->getCommandGate()->runAction((IOCommandGate::Action)handleSetPowerStateOff, (void *)1);
    else { self->setPowerStateOff(); self->release(); }
}

void com_vini_driver_RTL8723BE::handleSetPowerStateOn(thread_call_param_t param0, thread_call_param_t param1) {
    com_vini_driver_RTL8723BE *self = (com_vini_driver_RTL8723BE *)param0;
    if (param1 == 0) self->getCommandGate()->runAction((IOCommandGate::Action)handleSetPowerStateOn, (void *)1);
    else { self->setPowerStateOn(); self->release(); }
}

void com_vini_driver_RTL8723BE::setPowerStateOff() {
    pmPowerState = kPowerStateOff;
    pmPolicyMaker->acknowledgeSetPowerState();
}

void com_vini_driver_RTL8723BE::setPowerStateOn() {
    pmPowerState = kPowerStateOn;
    pmPolicyMaker->acknowledgeSetPowerState();
}

IOReturn com_vini_driver_RTL8723BE::tsleepHandler(OSObject* owner, void* arg0, void* arg1, void* arg2, void* arg3) {
    com_vini_driver_RTL8723BE* dev = OSDynamicCast(com_vini_driver_RTL8723BE, owner);
    if (!dev) return kIOReturnError;
    if (arg1 == 0) return (_fCommandGate->commandSleep(arg0, THREAD_INTERRUPTIBLE) == THREAD_AWAKENED) ? kIOReturnSuccess : kIOReturnTimeout;
    
    AbsoluteTime deadline;
    clock_interval_to_deadline((*(int*)arg1), kNanosecondScale, reinterpret_cast<uint64_t*>(&deadline));
    return (_fCommandGate->commandSleep(arg0, deadline, THREAD_INTERRUPTIBLE) == THREAD_AWAKENED) ? kIOReturnSuccess : kIOReturnTimeout;
}
