/*
 * Copyright (c) 2017-2018 syscl and coderobe. All rights reserved.
 *
 * Courtesy to vit9696's Lilu => https://github.com/vit9696/Lilu
 *
 */

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/plugin_start.hpp>

#include <Headers/kern_iokit.hpp>

#include "EnableLidWake.hpp"

static const char *kextHSWFb[] { "/System/Library/Extensions/AppleIntelFramebufferAzul.kext/Contents/MacOS/AppleIntelFramebufferAzul" };
static const char *kextHSWFbId { "com.apple.driver.AppleIntelFramebufferAzul" };
static const char *kextSKLFb[] { "/System/Library/Extensions/AppleIntelSKLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelSKLGraphicsFramebuffer" };
static const char *kextSKLFbId { "com.apple.driver.AppleIntelSKLGraphicsFramebuffer" };
static const char *kextKBLFb[] { "/System/Library/Extensions/AppleIntelKBLGraphicsFramebuffer.kext/Contents/MacOS/AppleIntelKBLGraphicsFramebuffer" };
static const char *kextKBLFbId { "com.apple.driver.AppleIntelKBLGraphicsFramebuffer" };

static KernelPatcher::KextInfo kextList[] {
    { kextHSWFbId, kextHSWFb, arrsize(kextHSWFb), {true}, {}, KernelPatcher::KextInfo::Unloaded },
    { kextSKLFbId, kextSKLFb, arrsize(kextSKLFb), {true}, {}, KernelPatcher::KextInfo::Unloaded },
    { kextKBLFbId, kextKBLFb, arrsize(kextKBLFb), {true}, {}, KernelPatcher::KextInfo::Unloaded },
};

static size_t kextListSize = arrsize(kextList);

// methods that are implmented here

uint32_t LWEnabler::getIgPlatformId() const
{
    uint32_t platform = 0;
    const char *tree[] {"AppleACPIPCI", "IGPU"};
    auto sect = WIOKit::findEntryByPrefix("/AppleACPIPlatformExpert", "PCI", gIOServicePlane);
    for (size_t i = 0; sect && i < arrsize(tree); i++)
    {
        sect = WIOKit::findEntryByPrefix(sect, tree[i], gIOServicePlane);
        if (sect && i+1 == arrsize(tree))
        {
            if (WIOKit::getOSDataValue(sect, "AAPL,ig-platform-id", platform))
            {
                DBGLOG(kCurrentKextID, "found IGPU with ig-platform-id 0x%08x", platform);
                return platform;
            }
            else
            {
                SYSLOG(kCurrentKextID, "found IGPU with missing ig-platform-id, assuming old");
            }
        }
    }
    
    DBGLOG(kCurrentKextID, "failed to find IGPU ig-platform-id");
    return platform;
}

bool LWEnabler::init()
{
    LiluAPI::Error error = lilu.onKextLoad(kextList, kextListSize,
       [](void* user, KernelPatcher& patcher, size_t index, mach_vm_address_t address, size_t size) {
           LWEnabler* patch = static_cast<LWEnabler*>(user);
           patch->processKext(patcher, index, address, size);
           
       }, this);
    
    if (error != LiluAPI::Error::NoError)
    {
        SYSLOG(kCurrentKextID, "failed to register onPatcherLoad method %d", error);
        return false;
    }
    
    return true;
}

void LWEnabler::processKext(KernelPatcher& patcher, size_t index, mach_vm_address_t address, size_t size)
{
    // check if we already done here
    if (progressState == ProcessingState::EverythingDone) return;
    
    // we reach here, now let's get started
    const uint32_t gIgPlatformId = getIgPlatformId();
    
    for (size_t i = 0; i < kextListSize; i++)
    {
        if (kextList[i].loadIndex != index) continue;
        //
        // Enable lid wake for Haswell(Azul) platform
        //
        if (!(progressState & ProcessingState::EverythingDone) && !strcmp(kextList[i].id, kextHSWFbId))
        {
            SYSLOG(kCurrentKextID, "found %s", kextList[i].id);
            
            if (gIgPlatformId == static_cast<uint32_t>(0x0a2e0008))
            {
                // 10.10.1- do not require any internal display patches
                if (gKernMajorVersion == KernelVersion::Yosemite && gKernMinorVersion < 2) continue;
                
                // apply the patch for 0x0a2e0008
                uint8_t azul_find[] = { 0x40, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x05, 0x05, 0x09, 0x01 };
                //                                                       ^  <-- High Sierra differ here 0x02
                uint8_t azul_repl[] = { 0x40, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x05, 0x05, 0x09, 0x01 };
                //                                                ^      ^  <-- High Sierra differ here 0x1f and 0x02
                // correct patch for High Sierra
                if (gKernMajorVersion >= KernelVersion::HighSierra)
                {
                    // find pattern change
                    azul_find[5] = 0x02;
                    // replace pattern change
                    azul_repl[4] = 0x1f;
                    azul_repl[5] = 0x02;
                }
                
                KextPatch azul_patch_info {
                    { &kextList[i], azul_find, azul_repl, sizeof(azul_find), 1 },
                    KernelVersion::Yosemite, KernelVersion::HighSierra
                };
                
                applyPatches(patcher, index, &azul_patch_info, 1);
                SYSLOG(kCurrentKextID, "enable internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                progressState |= ProcessingState::EverythingDone;
                
            } else if (gIgPlatformId == static_cast<uint32_t>(0x0a2e000a) ||
                       gIgPlatformId == static_cast<uint32_t>(0x0a26000a)) {
                // ig-platform that are 0x0a2e000a and 0x0a26000a
                const uint8_t azul_find[] = { 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0xd6, 0x00, 0x00, 0x00, 0x05, 0x05, 0x00, 0x00 };
                const uint8_t azul_repl[] = { 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x05, 0x05, 0x00, 0x00 };
                KextPatch azul_patch_info1 {
                    { &kextList[i], azul_find, azul_repl, sizeof(azul_find), 1 },
                    KernelVersion::Yosemite, KernelVersion::HighSierra
                };
                
                applyPatches(patcher, index, &azul_patch_info1, 1);
                SYSLOG(kCurrentKextID, "enable internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                progressState |= ProcessingState::EverythingDone;
            }
        }
        
        //
        // Enable lid wake for Skylake(skl) platform
        //
        if (!(progressState & ProcessingState::EverythingDone) && !strcmp(kextList[i].id, kextSKLFbId))
        {
            SYSLOG(kCurrentKextID, "found %s", kextList[i].id);
            if (gIgPlatformId != static_cast<uint32_t>(0x19260004)) return;
            
            const uint8_t skl_find[] = { 0x0a, 0x0b, 0x03, 0x00, 0x00, 0x07, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };
            const uint8_t skl_repl[] = { 0x0f, 0x0b, 0x03, 0x00, 0x00, 0x07, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };
            KextPatch skl_patch_info {
                { &kextList[i], skl_find, skl_repl, sizeof(skl_find), 1 },
                KernelVersion::ElCapitan, KernelVersion::HighSierra
            };

            applyPatches(patcher, index, &skl_patch_info, 1);
            SYSLOG(kCurrentKextID, "enable internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
            
            progressState |= ProcessingState::EverythingDone;
            break;
        }
    }
    patcher.clearError();
}

void LWEnabler::applyPatches(KernelPatcher& patcher, size_t index, const KextPatch* patches, size_t patchNum)
{
    for (size_t p = 0; p < patchNum; p++)
    {
        auto &patch = patches[p];
        if (patch.patch.kext->loadIndex == index)
        {
            if (patcher.compatibleKernel(patch.minKernel, patch.maxKernel))
            {
                patcher.applyLookupPatch(&patch.patch);
                SYSLOG(kCurrentKextID, "patch %s (%ld/%ld).", patch.patch.kext->id, p+1, patchNum);
                patcher.clearError();
            }
        }
    }
}

