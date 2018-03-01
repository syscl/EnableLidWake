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
                DBGLOG(kThisKextID, "found IGPU with ig-platform-id 0x%08x", platform);
                return platform;
            }
            else
            {
                SYSLOG(kThisKextID, "found IGPU with missing ig-platform-id, assuming old");
            }
        }
    }
    
    DBGLOG(kThisKextID, "failed to find IGPU ig-platform-id");
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
        SYSLOG(kThisKextID, "failed to register onPatcherLoad method %d", error);
        return false;
    }
    
    return true;
}

void LWEnabler::configIgPlatform()
{
    gIgPlatformId = getIgPlatformId();
    if (gIgPlatformId == static_cast<uint32_t>(0x19260004)||
        gIgPlatformId == static_cast<uint32_t>(0x0a26000a)||
        gIgPlatformId == static_cast<uint32_t>(0x0a2e0008)||
        gIgPlatformId == static_cast<uint32_t>(0x0a2e000a)) {
        // It is a fixable platform
        isFixablePlatform = true;
        switch (gIgPlatformId) {
            case 0x19260004: {
                memset(rIgPlatformId+0, 0x04, sizeof(uint8_t));
                memset(rIgPlatformId+1, 0x00, sizeof(uint8_t));
                memset(rIgPlatformId+2, 0x26, sizeof(uint8_t));
                memset(rIgPlatformId+3, 0x19, sizeof(uint8_t));
                break;
            }
            case 0x0a26000a: {
                memset(rIgPlatformId+0, 0x0a, sizeof(uint8_t));
                memset(rIgPlatformId+1, 0x00, sizeof(uint8_t));
                memset(rIgPlatformId+2, 0x26, sizeof(uint8_t));
                memset(rIgPlatformId+3, 0x0a, sizeof(uint8_t));
                break;
            }
            case 0x0a2e0008: {
                memset(rIgPlatformId+0, 0x08, sizeof(uint8_t));
                memset(rIgPlatformId+1, 0x00, sizeof(uint8_t));
                memset(rIgPlatformId+2, 0x2e, sizeof(uint8_t));
                memset(rIgPlatformId+3, 0x0a, sizeof(uint8_t));
                break;
            }
            case 0x0a2e000a: {
                memset(rIgPlatformId+0, 0x0a, sizeof(uint8_t));
                memset(rIgPlatformId+1, 0x00, sizeof(uint8_t));
                memset(rIgPlatformId+2, 0x2e, sizeof(uint8_t));
                memset(rIgPlatformId+3, 0x0a, sizeof(uint8_t));
                break;
            }
        }
    } else {
        SYSLOG(kThisKextID, "0x%08x is not fixable, abort.", gIgPlatformId);
        return;
    }
}

void LWEnabler::processKext(KernelPatcher& patcher, size_t index, mach_vm_address_t address, size_t size)
{
    // set ig-platform information first
    // private member, not a setter
    configIgPlatform();
    // check if we already done here
    // or if the platform cannot be fixed
    if (progressState == ProcessingState::EverythingDone || !isFixablePlatform) return;
    
    for (size_t i = 0; i < kextListSize; i++)
    {
        if (kextList[i].loadIndex != index) continue;
        //
        // Enable lid wake for Haswell (Azul) platform
        //
        if (!(progressState & ProcessingState::EverythingDone) && !strcmp(kextList[i].id, kextHSWFbId))
        {
            SYSLOG(kThisKextID, "found %s", kextList[i].id);
            // 10.10.1- do not require any internal display patches for 0x0a2e0008
            // but it's unclear that if 0x0a26000a and 0x0a2e000a require this
            // type of skip or not
            if (gKernMajorVersion == KernelVersion::Yosemite && gKernMinorVersion < 2 &&
                gIgPlatformId == static_cast<uint32_t>(0x0a2e0008)) continue;
            mach_vm_address_t address = patcher.solveSymbol(index, "_ltDriveTable");
            if (address) {
                SYSLOG(kThisKextID, "obtained _ltDriveTable");
                patcher.clearError();
                
                // Lookup the ig-platform-id specific framebuffer data
                auto curOff = reinterpret_cast<uint8_t *>(address);
                // The real patch place should be very close
                // MaxSearchSize aka PAGE_SIZE is fairly enough
                auto endOff = curOff + PAGE_SIZE;
                // The framebuffer size
                static constexpr size_t MaxReplSize {16};
                // Search the specific ig-platform-id in the neighbourhood
                while (curOff < endOff && memcmp(curOff, rIgPlatformId, sizeof(rIgPlatformId)))
                    curOff++;
                // verify search
                if (curOff < endOff) {
                    DBGLOG(kThisKextID, "found platform-id (0x%08x) at framebuffer info data segment", gIgPlatformId);
                    // now let's generate the patch for it
                    uint8_t find[MaxReplSize] {};
                    uint8_t repl[MaxReplSize] {};
                    lilu_os_memcpy(find, curOff+84, MaxReplSize);
                    lilu_os_memcpy(repl, curOff+84, MaxReplSize);
                    DBGLOG(kThisKextID, "%u, %u, %u, %u, %u", *find, *(find+1), *(find+2), *(find+3), *(find+4));
                    // apply platform specific patch pattern
                    if (gIgPlatformId == static_cast<uint32_t>(0x0a2e0008)) {
                        // 0x0a2e0008 use 0x1f
                        memset(repl+4, 0x1f, sizeof(uint8_t));
                        if (memcmp(repl+4, find+4, sizeof(uint8_t)) == 0) {
                            // already patch? we should stop here due to the
                            // internal display has been enabled after sleep
                            SYSLOG(kThisKextID, "already enabled internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                            return;
                        }
                    } else {
                        // this two platforms share the same patch pattern 0x1e
                        memset(repl+4, 0x1e, sizeof(uint8_t));
                        if (memcmp(repl+4, find+4, sizeof(uint8_t)) == 0) {
                            // already patch? we should stop here due to the
                            // internal display has been enabled after sleep
                            SYSLOG(kThisKextID, "already enabled internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                            return;
                        }
                    }
                    SYSLOG(kThisKextID, "binary patches for internal display have been generated.");
                    KextPatch azul_patch_info {
                        { &kextList[i], find, repl, sizeof(find), 1 },
                        KernelVersion::Yosemite, KernelVersion::HighSierra
                    };
                    
                    applyPatches(patcher, index, &azul_patch_info, 1);
                    SYSLOG(kThisKextID, "enable internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                    progressState |= ProcessingState::EverythingDone;
                    
                } else {
                    SYSLOG(kThisKextID, "cannot find platform-id at %s", *kextHSWFb);
                    return;
                }
            } else {
                SYSLOG(kThisKextID, "cannot find _ltDriveTable");
                return;
            }
        }
        //
        // Enable lid wake for Skylake (skl) platform
        //
        if (!(progressState & ProcessingState::EverythingDone) && !strcmp(kextList[i].id, kextSKLFbId))
        {
            SYSLOG(kThisKextID, "found %s", kextList[i].id);
            // it must be 0x19260004 due to the previous isFixablePlatform checking
            const uint8_t skl_find[] = { 0x0a, 0x0b, 0x03, 0x00, 0x00, 0x07, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };
            const uint8_t skl_repl[] = { 0x0f, 0x0b, 0x03, 0x00, 0x00, 0x07, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 };
            KextPatch skl_patch_info {
                { &kextList[i], skl_find, skl_repl, sizeof(skl_find), 1 },
                KernelVersion::ElCapitan, KernelVersion::HighSierra
            };

            applyPatches(patcher, index, &skl_patch_info, 1);
            SYSLOG(kThisKextID, "enable internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
            
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
                SYSLOG(kThisKextID, "patch %s (%ld/%ld).", patch.patch.kext->id, p+1, patchNum);
                patcher.clearError();
            }
        }
    }
}

