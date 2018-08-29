EnableLidWake Changelog
==============
### v3.8

- Implemented lid wake for KabyLake (KBL) platform

### v3.7

- Use ```DBGLOG``` instead of ```SYSLOG``` for configIgPlatform

### v3.6

- Prevent configIgPlatform from invoking multiple times
- Simplified configIgPlatform

### v3.5

- Major clean up due to we get rid of old style (unreliable) matching against kernel version patch

### v3.4

- A more natural way to patch the framebuffer in memory (```copy back```) for ```Skylake``` platform

### v3.3

- A more natural way to patch the framebuffer in memory (```copy back```) for ```Haswell``` platform
- Get rid of ```applyLookupPatch()``` invoking

### v3.2

- Simplified the logic in ```configPlatform()```

### v3.1

- Dynamically generate patches from current loaded framebuffer (0x19260004) in memory
- Optimized speed a bit by using ```memcmp()``` instead of  ```strcmp()```

### v3.0, v2.9 and v2.8

- Unified and simplified the code blocks (speed up a bit)

### v2.7, v2.6 and v2.5
- Improved performance by  ```memset()``` and ```memcmp()``` which is usually highly optimized for the target architecture. It might be implemeted as a rep stosq loop, or use SSE2 (possibly using non-temporal writes for large chunks in order to avoid polluting caches).

### v2.4

- Append return for ```0x0a2e0008``` if the framebuffer has been patched already

### v2.3

- Dynamically generate patches from current loaded ```Haswell``` framebuffer in memory
- Code rework

### v2.2

- Minor cosmetic change

#### v2.1
- Correct ```0x0a2e0008``` patches on macOS earlier than ```High Sierra```

#### v2.0, v1.9 and v1.8
- Correct indentions

#### v1.7
- Variable ```gIgPlatformId``` cannot change

#### v 1.6
- Fixed ```as.vit9696.Lilu``` to 1.2.0
- Optimized using initialized lists
- Follow object-oriented programming style in ```C++```

#### v1.5
- Added platform id detection routine as a member 
- Speed up patches, more accuracy patching for each of the platform
- Added High Sierra support for ```0x0a2e0008```
- Major code style change to syscl's style (optimization)

#### v1.4
- Fixed compatible issue with Lilu v1.2 and later
- Fixed licensing issue
