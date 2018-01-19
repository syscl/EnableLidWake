EnableLidWake
====

This kext (actually Lilu friend) provides you the ability to fix the lid wake issue after resuming from sleep on macOS.

####  Requriments
Lilu.kext 1.2.0 and later
macOS version that is greater than 10.10.2 (because previous OS X do not have this issue)
Inject correct ig-platform-id for your system.

####  Usage
Place it under any bootloader injection folder or ```/Library/Extensions``` then reboot.

####  Credits
- vit9696 for his Lilu - a kext that patch kext and processes.
- coderobe for his codebase
- syscl for his lid wake patch

Copyright (c) 2018 syscl and coderobe. All rights reserved.
