/*
 * pxversion.h : compile-time version informations for the pslib.
 *
 * See Copyright for the status of this software.
 *
 * Uwe@steinmann.cx
 */

#ifndef __PXLIB_VERSION_H__
#define __PXLIB_VERSION_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * use those to be sure nothing nasty will happen if
 * your library and includes mismatch
 */
#define PXLIB_DOTTED_VERSION "0.6.8"
#define PXLIB_MAJOR_VERSION 0
#define PXLIB_MINOR_VERSION 6
#define PXLIB_MICRO_VERSION 8

/*
 * Whether Debugging module is configured in
 */
#if 1
#define PXLIB_DEBUG_ENABLED
#else
#define PXLIB_DEBUG_DISABLED
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif


