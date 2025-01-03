#ifndef APP_VERSION_H
#define APP_VERSION_H

#define TO_STRING2(s) #s
#define TO_STRING(s) TO_STRING2(s)
#define TO_NUMBER(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

// clang-format off
#define APP_NAME_CZ   "PxView"
#define APP_TITLE_CZ  "Paradox database viewer"

#define APP_NAME      "PxView"
#define APP_TITLE     "Paradox database viewer"
#define APP_COPYRIGHT "Copyright (c) 2016-2025"

#define APP_MAJOR_VER 1
#define APP_MINOR_VER 1
#define APP_PATCH_VER 8
#define APP_BUILD_VER 0

#define APP_VERSION_STR "1.1.8"
#define APP_VERSION_NUM TO_NUMBER(APP_MAJOR_VER, APP_MINOR_VER, APP_PATCH_VER, APP_BUILD_VER)

#define APP_FILEVERSION     APP_MAJOR_VER, APP_MINOR_VER, APP_PATCH_VER, APP_BUILD_VER
#define APP_FILEVERSION_STR TO_STRING(APP_MAJOR_VER)     \
                            "." TO_STRING(APP_MINOR_VER) \
                            "." TO_STRING(APP_PATCH_VER) \
                            "." TO_STRING(APP_BUILD_VER) \

#define APP_PRODUCTVERSION     APP_FILEVERSION
#define APP_PRODUCTVERSION_STR APP_FILEVERSION_STR
// clang-format on

#endif // APP_VERSION_H

// vim: ts=4
