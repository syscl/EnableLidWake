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
    const char *tree[] { "AppleACPIPCI", "IGPU" };
    auto sect = WIOKit::findEntryByPrefix("/AppleACPIPlatformExpert", "PCI", gIOServicePlane);
    for (size_t i = 0; sect && i < arrsize(tree); i++) {
        sect = WIOKit::findEntryByPrefix(sect, tree[i], gIOServicePlane);
        if (sect && i+1 == arrsize(tree)) {
            if (WIOKit::getOSDataValue(sect, "AAPL,ig-platform-id", platform)) {
                DBGLOG(kThisModule, "found IGPU with ig-platform-id 0x%08x", platform);
                return platform;
            } else {
                SYSLOG(kThisModule, "found IGPU with missing ig-platform-id, assuming old");
            }
        }
    }
    
    DBGLOG(kThisModule, "failed to find IGPU ig-platform-id");
    return platform;
}

bool LWEnabler::init()
{
    auto error = lilu.onKextLoad(kextList, kextListSize,
       [](void* user, KernelPatcher& patcher, size_t index, mach_vm_address_t address, size_t size) {
           LWEnabler* patch = static_cast<LWEnabler*>(user);
           patch->frameBufferPatch(patcher, index, address, size);
       }, this);

    if (error != LiluAPI::Error::NoError)
    {
        SYSLOG(kThisModule, "failed to register onPatcherLoad method %d", error);
        return false;
    }
    
    return true;
}

void LWEnabler::configIgPlatform()
{
    // Once ig-platform has set, we will not
    // invoke this routine again
    isIgPlatformSet = true;
    gIgPlatformId = getIgPlatformId();
    switch (gIgPlatformId) {
        case 0x19260004:
        case 0x0a26000a:
        case 0x0a2e0008:
        case 0x0d26000f:
        case 0x0a2e000a:
        case 0x59260002: {
            lilu_os_memcpy(reinterpret_cast<uint32_t *>(rIgPlatformId), &gIgPlatformId, sizeof(uint32_t));
            DBGLOG(kThisModule, "reverse order of ig-platform-id: 0x%02x, 0x%02x, 0x%02x, 0x%02x", *rIgPlatformId, *(rIgPlatformId+1), *(rIgPlatformId+2), *(rIgPlatformId+3));
            break;
        }
        default: {
            SYSLOG(kThisModule, "0x%08x is not fixable, abort.", gIgPlatformId);
            isFixablePlatform = false;
            break;
        }
    }
}

void LWEnabler::frameBufferPatch(KernelPatcher& patcher, size_t index, mach_vm_address_t address, size_t size)
{
    // set ig-platform information first
    // private member, not a setter
    if (!isIgPlatformSet)
        configIgPlatform();
    
    // check if we already done here
    // or if the platform cannot be fixed
    if (progressState == ProcessingState::EverythingDone || !isFixablePlatform) return;
    
    for (size_t i = 0; i < kextListSize; i++)
    {
        if (kextList[i].loadIndex != index) continue;

        // Enable lid wake for Haswell (Azul) platform
        if (!(progressState & ProcessingState::EverythingDone) &&
            memcmp(kextList[i].id, kextHSWFbId, strlen(kextHSWFbId)) == 0) {
            SYSLOG(kThisModule, "found %s", kextList[i].id);
            
            mach_vm_address_t address = patcher.solveSymbol(index, "_gPlatformInformationList");
            if (address) {
                SYSLOG(kThisModule, "obtained _gPlatformInformationList");
                patcher.clearError();
                
                // Lookup the ig-platform-id specific framebuffer data
                auto curOff = reinterpret_cast<uint8_t *>(address);
                // The real patch place should be very close
                // MaxSearchSize aka PAGE_SIZE is fairly enough
                auto endOff = curOff + PAGE_SIZE;
                // Max replace size
                static constexpr size_t MaxReplSize {sizeof(uint8_t)};
                // Search the specific ig-platform-id in the neighbourhood
                while (curOff < endOff && memcmp(curOff, rIgPlatformId, sizeof(rIgPlatformId)))
                    curOff++;
                // verify search
                if (curOff < endOff) {
                    DBGLOG(kThisModule, "found platform-id (0x%08x) at framebuffer info data segment", gIgPlatformId);
                    // now let's generate the patch for it
                    // target offset should @ +88
                    curOff += 88;
                    uint8_t repl[MaxReplSize] {};
                    lilu_os_memcpy(repl, curOff, MaxReplSize);
                    DBGLOG(kThisModule, "%u, %u, %u, %u, %u", *curOff, *(curOff+1), *(curOff+2), *(curOff+3), *(curOff+4));
                    // apply platform specific patch pattern
                    // 0x0a2e0008 and 0x0d26000f use 0x1f
                    // 0x0a2e000a and 0x0a26000a use 0x1e
                    memset(repl, (gIgPlatformId == 0x0a2e0008 || gIgPlatformId == 0x0d26000f) ? 0x1f : 0x1e, MaxReplSize);
                    if (memcmp(repl, curOff, MaxReplSize) == 0) {
                        // already patch due to the kext has been
                        // patched in the cache? we should stop here
                        SYSLOG(kThisModule, "already enabled internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                        return;
                    }
                    SYSLOG(kThisModule, "binary patches for internal display have been generated.");
                    SYSLOG(kThisModule, "attempting to copy back...");
                    // a more nature way to fix the lid wake issue by
                    // copying back to the framebuffer in memory instead of
                    // invoking the applyLookupPatch()
                    // note: we didn't require to check kernel version
                    // due to the fact that we don't care about which
                    // version of kext will be loaded
                    lilu_os_memcpy(curOff, repl, MaxReplSize);
                    SYSLOG(kThisModule, "enable internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                    progressState |= ProcessingState::EverythingDone;
                    break;
                } else {
                    SYSLOG(kThisModule, "cannot find platform-id at %s", *kextHSWFb);
                    return;
                }
            } else {
                SYSLOG(kThisModule, "cannot find _gPlatformInformationList");
                return;
            }
        }

        // Enable lid wake for Skylake (skl) platform
        if (!(progressState & ProcessingState::EverythingDone) &&
            memcmp(kextList[i].id, kextSKLFbId, strlen(kextSKLFbId)) == 0) {
            SYSLOG(kThisModule, "found %s", kextList[i].id);
            // it must be 0x19260004 due to the previous configPlatform's check
            mach_vm_address_t address = patcher.solveSymbol(index, "_gPlatformInformationList");
            if (address) {
                SYSLOG(kThisModule, "obtained _gPlatformInformationList");
                patcher.clearError();
                // Lookup the ig-platform-id specific framebuffer data
                auto curOff = reinterpret_cast<uint8_t *>(address);
                // The real patch place should be very close
                // MaxSearchSize aka PAGE_SIZE is fairly enough
                auto endOff = curOff + PAGE_SIZE;
                // Max replace size
                static constexpr size_t MaxReplSize {sizeof(uint8_t)};
                // Search the specific ig-platform-id in the neighbourhood
                while (curOff < endOff && memcmp(curOff, rIgPlatformId, sizeof(rIgPlatformId)))
                    curOff++;
                // verify search
                if (curOff < endOff) {
                    DBGLOG(kThisModule, "found platform-id (0x%08x) at framebuffer info data segment", gIgPlatformId);
                    // now let's generate the patch for it
                    // target offset should @ +97
                    curOff += 97;
                    uint8_t repl[MaxReplSize] {};
                    lilu_os_memcpy(repl, curOff, MaxReplSize);
                    DBGLOG(kThisModule, "%u, %u, %u, %u, %u", *curOff, *(curOff+1), *(curOff+2), *(curOff+3), *(curOff+4));
                    // apply platform specific patch pattern
                    // 0x19260004 uses 0x0f
                    memset(repl, 0x0f, MaxReplSize);
                    if (memcmp(repl, curOff, MaxReplSize) == 0) {
                        // already patch due to the kext has been
                        // patched in the cache? we should stop here
                        SYSLOG(kThisModule, "already enabled internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                        return;
                    }
                    SYSLOG(kThisModule, "binary patches for internal display have been generated.");
                    // a more nature way to fix the lid wake issue by
                    // copying back to the framebuffer in memory instead of
                    // invoking the applyLookupPatch()
                    // note: we didn't require to check kernel version
                    // due to the fact that we don't care about which
                    // version of kext will be loaded
                    lilu_os_memcpy(curOff, repl, MaxReplSize);
                    SYSLOG(kThisModule, "enable internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                    progressState |= ProcessingState::EverythingDone;
                    break;
                } else {
                    SYSLOG(kThisModule, "cannot find platform-id at %s", *kextHSWFb);
                    return;
                }
            } else {
                SYSLOG(kThisModule, "cannot find _gPlatformInformationList");
                return;
            }
        }
        
        // Enable lid wake for KabyLake (kbl) platform
        if (!(progressState & ProcessingState::EverythingDone) &&
            memcmp(kextList[i].id, kextKBLFbId, strlen(kextKBLFbId)) == 0) {
            SYSLOG(kThisModule, "found %s", kextList[i].id);
            // it must be 0x59260002 due to the previous configPlatform's check
            mach_vm_address_t address = patcher.solveSymbol(index, "_gPlatformInformationList");
            if (address) {
                SYSLOG(kThisModule, "obtained _gPlatformInformationList");
                patcher.clearError();
                // Lookup the ig-platform-id specific framebuffer data
                auto curOff = reinterpret_cast<uint8_t *>(address);
                // The real patch place should be very close
                // MaxSearchSize aka PAGE_SIZE is fairly enough
                auto endOff = curOff + PAGE_SIZE;
                // Max replace size
                static constexpr size_t MaxReplSize {sizeof(uint8_t)};
                // Search the specific ig-platform-id in the neighbourhood
                while (curOff < endOff && memcmp(curOff, rIgPlatformId, sizeof(rIgPlatformId)))
                curOff++;
                // verify search
                if (curOff < endOff) {
                    DBGLOG(kThisModule, "found platform-id (0x%08x) at framebuffer info data segment", gIgPlatformId);
                    // now let's generate the patch for it
                    // target offset should @ +97
                    curOff += 97;
                    uint8_t repl[MaxReplSize] {};
                    lilu_os_memcpy(repl, curOff, MaxReplSize);
                    DBGLOG(kThisModule, "%u, %u, %u, %u, %u", *curOff, *(curOff+1), *(curOff+2), *(curOff+3), *(curOff+4));
                    // apply platform specific patch pattern
                    // 0x19260004 uses 0x0f
                    memset(repl, 0x0f, MaxReplSize);
                    if (memcmp(repl, curOff, MaxReplSize) == 0) {
                        // already patch due to the kext has been
                        // patched in the cache? we should stop here
                        SYSLOG(kThisModule, "already enabled internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                        return;
                    }
                    SYSLOG(kThisModule, "binary patches for internal display have been generated.");
                    // a more nature way to fix the lid wake issue by
                    // copying back to the framebuffer in memory instead of
                    // invoking the applyLookupPatch()
                    // note: we didn't require to check kernel version
                    // due to the fact that we don't care about which
                    // version of kext will be loaded
                    lilu_os_memcpy(curOff, repl, MaxReplSize);
                    SYSLOG(kThisModule, "enable internal display after sleep for ig-platform-id: 0x%08x", gIgPlatformId);
                    progressState |= ProcessingState::EverythingDone;
                    break;
                } else {
                    SYSLOG(kThisModule, "cannot find platform-id at %s", *kextHSWFb);
                    return;
                }
            } else {
                SYSLOG(kThisModule, "cannot find _gPlatformInformationList");
                return;
            }
        }
    }
    patcher.clearError();
}
