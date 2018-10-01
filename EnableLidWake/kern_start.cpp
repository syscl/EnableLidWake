/*
 * Copyright (c) 2017 syscl and coderobe. All rights reserved.
 *
 * Courtesy to vit9696's Lilu => https://github.com/vit9696/Lilu/
 */

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "EnableLidWake.hpp"

static LWEnabler elw;

const char* kBootArgvDisable[] = { "-elwoff"  };
const char* kBootArgvDebug[]   = { "-elwdbg"  };
const char* kBootArgvBeta[]    = { "-elwbeta" };

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery,
    kBootArgvDisable,
    arrsize(kBootArgvDisable),
    kBootArgvDebug,
    arrsize(kBootArgvDebug),
    kBootArgvBeta,
    arrsize(kBootArgvBeta),
    // minKernel: 10.10 - maxKernel: 10.14+
    KernelVersion::Yosemite,
    KernelVersion::Mojave,
    // now let's get start
    []() {
        elw.init();
    }
};
