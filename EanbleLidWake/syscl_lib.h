//
//  syscl_lib.h
//  EnableLidWake
//
//  Created by syscl on 6/11/17.
//  Copyright © 2017 syscl. All rights reserved.
//
// Yating Zhou(aka syscl)'s lib
//

#ifndef __SYSCL_LIB_H__
#define __SYSCL_LIB_H__

#include <stdbool.h>
#include <string.h>

#if kDEBUG
#define DBG_PREFIX "Debug: "
#define DBG(args...) do { printf(DBG_PREFIX args); } while(kFALSE)
//
// getFunctionName: retrieve function's name
//
#define getFunctionName(args...) do { printf(DBG_PREFIX "%s()", __func__); } while(kFALSE)
//
// list init information for specific function
//
#define InitDBG(args...) do { printf(DBG_PREFIX "%s() ===>\n", __func__); } while(kFALSE)
//
// list return information for specific function
//
// usage: e.g. ReturnDBG("%s\n", string); ReturnDBG("%d\n", ret);
//
#define ReturnDBG(args...) do { printf(DBG_PREFIX "%s() <==", __func__); printf("=" args); } while(kFALSE)
#else
#define DBG(args...)
#define getFunctionName(args...)
#define InitDBG(args...)
#define ReturnDBG(args...)
#endif

#define getArrayLength(Array) ((sizeof(Array)/sizeof(0[Array])) / ((size_t)(!(sizeof(Array) % sizeof(0[Array])))))

#endif
