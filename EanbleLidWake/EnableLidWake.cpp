/*
 * Copyright (c) 2017 syscl/lighting/Yating Zhou. All rights reserved.
 *
 * Courtesy to vit9696's Lilu => https://github.com/vit9696/Lilu
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial
 * 4.0 Unported License => http://creativecommons.org/licenses/by-nc/4.0
 */

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <Library/LegacyIOService.h>

#include <mach/vm_map.h>
#include <IOKit/IORegistryEntry.h>

#include "EnableLidWake.hpp"

KernelVersion KernelCheck = getKernelVersion();

static const char *GraphicsKextCFBundleIdentifier[] = {
    "com.apple.driver.AppleIntelFramebufferAzul",
    "com.apple.driver.AppleIntelSKLGraphicsFramebuffer"
};

static const char *GraphicKextPath[] = {
    "/System/Library/Extensions/AppleIntelFramebufferAzul.kext/Contents/MacOS/AppleIntelFramebufferAzul",
    "/System/Library/Extensions/AppleIntelSKLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelSKLGraphicsFramebuffer"
};

static KernelPatcher::KextInfo kextList[] {
    { GraphicsKextCFBundleIdentifier[kHSW], &GraphicKextPath[kHSW], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded },
    { GraphicsKextCFBundleIdentifier[kSKL], &GraphicKextPath[kSKL], 1, {true}, {}, KernelPatcher::KextInfo::Unloaded }
};

static size_t kextListSize = getArrayLength(kextList);


bool LidWake::init()
{
	LiluAPI::Error error = lilu.onKextLoad(kextList, kextListSize,
    [](void* user, KernelPatcher& patcher, size_t index, mach_vm_address_t address, size_t size) {
        LidWake* patch = static_cast<LidWake*>(user);
		patch->processKext(patcher, index, address, size);
	}, this);
	
	if (error != LiluAPI::Error::NoError)
    {
		SYSLOG(kCurrentKextID, "failed to register onPatcherLoad method %d", error);
		return false;
	}
	
	return true;
}

void LidWake::deinit() {}

void LidWake::processKext(KernelPatcher& patcher, size_t index, mach_vm_address_t address, size_t size)
{
    KextPatch *patch_info;
    if (progressState != ProcessingState::EverythingDone)
    {
        for (size_t i = 0; i < kextListSize; i++)
        {
            if (kextList[i].loadIndex == index)
            {
                //
                // Enable lid wake for Haswell(Azul) platform
                //
                if (!(progressState & ProcessingState::EverythingDone) && !strcmp(kextList[i].id, GraphicsKextCFBundleIdentifier[kHSW]))
                {
                    SYSLOG(kCurrentKextID, ": found %s", kextList[i].id);
                    
                    const uint8_t azul_find[]    = { 0x40, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x05, 0x05, 0x09, 0x01 };
                    const uint8_t azul_replace[] = { 0x40, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x05, 0x05, 0x09, 0x01 };
                    KextPatch azul_patch_info {
                        { &kextList[i], azul_find, azul_replace, sizeof(azul_find), 1 },
                        KernelVersion::Yosemite, KernelVersion::HighSierra
                    };
                    
                    patch_info = &azul_patch_info;
                    applyPatches(patcher, index, patch_info, 1);
                    SYSLOG(kCurrentKextID, ": Enable internal display after sleep for Haswell 1");
                    
                    
                    const uint8_t azul_find1[]    = { 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0xd6, 0x00, 0x00, 0x00, 0x05, 0x05, 0x00, 0x00 };
                    const uint8_t azul_replace1[] = { 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x05, 0x05, 0x00, 0x00 };
                    KextPatch azul_patch_info1 {
                        { &kextList[i], azul_find1, azul_replace1, sizeof(azul_find1), 1 },
                        KernelVersion::Yosemite, KernelVersion::HighSierra
                    };
                    
                    
                    patch_info = &azul_patch_info1;
                    applyPatches(patcher, index, patch_info, 1);
                    SYSLOG(kCurrentKextID, ": Enable internal display after sleep for Haswell 2");
                    
                    progressState |= ProcessingState::EverythingDone;
                    break;
                }
                
                //
                // Enable lid wake for Skylake(skl) platform
                //
                if (!(progressState & ProcessingState::EverythingDone) && !strcmp(kextList[i].id, GraphicsKextCFBundleIdentifier[kSKL]))
                {
                    SYSLOG(kCurrentKextID, ": found %s", kextList[i].id);
                    
                    const uint8_t skl_find[]    = { 0x0a, 0x0b, 0x03, 0x00, 0x00, 0x07, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };
                    const uint8_t skl_replace[] = { 0x0f, 0x0b, 0x03, 0x00, 0x00, 0x07, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };
                    KextPatch skl_patch_info {
                        { &kextList[i], skl_find, skl_replace, sizeof(skl_find), 1 },
                        KernelVersion::ElCapitan, KernelVersion::HighSierra
                    };
                    
                    patch_info = &skl_patch_info;
                    applyPatches(patcher, index, patch_info, 1);
                    SYSLOG(kCurrentKextID, ": Enable Lidwake for Skylake Platform");
                    
                    progressState |= ProcessingState::EverythingDone;
                    break;
                }
            }
        }
    }
	patcher.clearError();
}

void LidWake::applyPatches(KernelPatcher& patcher, size_t index, const KextPatch* patches, size_t patchNum)
{
    for (size_t p = 0; p < patchNum; p++)
    {
        auto &patch = patches[p];
        if (patch.patch.kext->loadIndex == index)
        {
            if (patcher.compatibleKernel(patch.minKernel, patch.maxKernel))
            {
                SYSLOG(kCurrentKextID, ": patching %s (%ld/%ld)...", patch.patch.kext->id, p+1, patchNum);
                patcher.applyLookupPatch(&patch.patch);
                patcher.clearError();
            }
        }
    }
}

