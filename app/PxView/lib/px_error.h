#ifndef __PX_ERROR_H__
#define __PX_ERROR_H__
void px_errorhandler(pxdoc_t *p, int error, const char *str, void *data);
void px_error(pxdoc_t *p, int type, const char *fmt, ...);
#endif
