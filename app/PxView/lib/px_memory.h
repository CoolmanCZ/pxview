#ifndef __PX_MEMORY_H__
#define __PX_MEMORY_H__
void *_px_malloc(pxdoc_t *p, size_t len, const char *caller);
void *_px_realloc(pxdoc_t *p, void *mem, size_t len, const char *caller);
void _px_free(pxdoc_t *p, void *ptr);
size_t px_strlen(const char *str);
char *px_strdup(pxdoc_t *p, const char *str);
#endif
