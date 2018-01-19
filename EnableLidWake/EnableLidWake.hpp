/*
 * Copyright (c) 2017 syscl and corerobe. All rights reserved.
 *
 * Courtesy to vit9696's Lilu => https://github.com/vit9696/Lilu/
 */

#ifndef __ENABLE_LIDWAKE_HPP__
#define __ENABLE_LIDWAKE_HPP__

#include <Headers/kern_patcher.hpp>

#define kCurrentKextID "org.syscl.EnableLidWake"

/*
#define kHSW   0
#define kSKL   1
#define kKBL   2
*/
enum { kHSW = 0, kSKL, kKBL };

struct KextPatch {
    KernelPatcher::LookupPatch patch;
    uint32_t minKernel;
    uint32_t maxKernel;
};

class LidWake
{
public:
	bool init();
	void deinit();
	
private:
	/**
	 *  Patch kext if needed and prepare other patches
	 *
	 *  @param patcher KernelPatcher instance
	 *  @param index   kinfo handle
	 *  @param address kinfo load address
	 *  @param size    kinfo memory size
	 */
	void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
    
    /**
     *  Apply kext patches for loaded kext index
     *
     *  @param patcher    KernelPatcher instance
     *  @param index      kinfo index
     *  @param patches    patch list
     *  @param patchesNum patch number
     */
    void applyPatches(KernelPatcher &patcher, size_t index, const KextPatch *patches, size_t patchesNum);
	
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

#endif
