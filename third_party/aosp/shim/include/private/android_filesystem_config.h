// Subset of AID_* UIDs SF references. Layerviewer never does real uid
// checks; the values are used in trace metadata / equality compares.
#pragma once

#define AID_ROOT 0
#define AID_SYSTEM 1000
#define AID_GRAPHICS 1003
#define AID_INPUT 1004
#define AID_MEDIA 1013
#define AID_SDCARD_RW 1015
#define AID_SHELL 2000
#define AID_NOBODY 9999
#define AID_APP_START 10000
#define AID_APP_END 19999
#define AID_CACHE_GID_START 20000
#define AID_CACHE_GID_END 29999
#define AID_USER_OFFSET 100000
