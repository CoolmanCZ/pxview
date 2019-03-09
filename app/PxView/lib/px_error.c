#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "px_intern.h"
#include "paradox.h"

/* px_errorhandler() {{{
 * Default error handler if not set by application
 */
void px_errorhandler(pxdoc_t *p, int error, const char *str, void *data) {
	if(error != PX_Warning || p->warnings == px_true)
		printf("PXLib: %s\n", str);
}
/* }}} */

/* px_error() {{{
 * Issue an error from within the library by using the error handler
 */
void px_error(pxdoc_t *p, int type, const char *fmt, ...) {
	char msg[256];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);

	if(p->errorhandler)
		(p->errorhandler)(p, type, msg, p->errorhandler_user_data);

	va_end(ap);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
