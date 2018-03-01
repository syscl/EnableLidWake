EnableLidWake
====

This kext (actually Lilu friend) provides you the ability to fix the lid wake issue after resuming from sleep on macOS. Comparing the previous binary patch, new ```EnableLidWake (v3.4+) ``` gives you a more natural and effective way to fix the internal display sleep issue in memory.


####  Requriments
Lilu.kext ```1.2.0``` and later
macOS version that is greater than 10.10.2 (because previous OS X do not have this issue)
Inject correct ig-platform-id for your system.

####  Usage
Place it under any bootloader injection folder or ```/Library/Extensions``` then reboot.

####  Credits
- vit9696 for his Lilu - a kext that patches kext and processes.
- coderobe for his ```xcodeproj``` file linkage to Lilu's API
- syscl for his dynamic lid wake patches and natural ```put back``` method

Copyright (c) 2017-2018 syscl and coderobe. All rights reserved.
