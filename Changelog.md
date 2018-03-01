EnableLidWake Changelog
==============
### v2.3

- Dynamically generate patches from current loaded framebuffer in memory
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
