#ifndef __PARADOX_MP_H__
#define __PARADOX_MP_H__

#include "paradox-gsf.h"

PXLIB_API void PXLIB_CALL
PX_mp_init(void);

PXLIB_API void * PXLIB_CALL
PX_mp_malloc(pxdoc_t *p, size_t size, const char *caller);

PXLIB_API void * PXLIB_CALL
PX_mp_realloc(pxdoc_t *p, void *mem, size_t size, const char *caller);

PXLIB_API void PXLIB_CALL
PX_mp_free(pxdoc_t *p, void *mem);

PXLIB_API void PXLIB_CALL
PX_mp_list_unfreed();

#endif
