/*
 * Copyright (c) 2017-2018 syscl and coderobe. All rights reserved.
 *
 * Courtesy to vit9696's Lilu => https://github.com/vit9696/Lilu/
 */

#ifndef EnableLidWake_hpp
#define EnableLidWake_hpp

#include <Headers/kern_patcher.hpp>

#define kThisKextID "org.syscl.EnableLidWake"

struct KextPatch {
    KernelPatcher::LookupPatch patch;
    uint32_t minKernel;
    uint32_t maxKernel;
};

class LWEnabler
{
public:
    // default constructor
    LWEnabler() : gKernMajorVersion(getKernelVersion()), gKernMinorVersion(getKernelMinorVersion()), isFixablePlatform(true) { }
    
    // destructor
    ~LWEnabler() { }
    // methods that are used to process patching
    bool init();
	
private:
    const KernelVersion gKernMajorVersion;
    const KernelMinorVersion gKernMinorVersion;
    uint32_t gIgPlatformId;
    // reverse order of ig-platform-id
    uint8_t rIgPlatformId[4] {};
    bool isFixablePlatform;

    /**
     * Obtain current ig-platform-id
     *
     * @return current ig-platform-id
     */
    uint32_t getIgPlatformId(void) const;
    
    /**
     * Set up ig-platform-id information
     */
    void configIgPlatform(void);
    
    
	/**
	 *  Patch framebuffer in memory
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void frameBufferPatch(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
    
    /**
     *  Current progress mask
     */
    struct ProcessingState {
        enum {
            NothingReady = 0,
            EverythingDone = 1,
        };
    };
    int progressState {ProcessingState::NothingReady};
};

#endif /* EnableLidWake_hpp */
