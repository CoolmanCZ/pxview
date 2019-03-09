#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "px_intern.h"
#include "paradox-gsf.h"
#include "px_error.h"

void *_px_malloc(pxdoc_t *p, size_t len, const char *caller) {
	return((void *) malloc(len));
}

void *_px_realloc(pxdoc_t *p, void *mem, size_t len, const char *caller) {
	return((void *) realloc(mem, len));
}

void _px_free(pxdoc_t *p, void *ptr) {
	free(ptr);
	ptr = NULL;
}

size_t px_strlen(const char *str) {
	return(strlen(str));
}

char *px_strdup(pxdoc_t *p, const char *str) {
	size_t len;
	char *buf;

	if (str == NULL) {
		px_error(p, PX_Warning, _("Passed NULL string to px_strdup()."));
		return(NULL);
	}
	len = px_strlen(str)+1;
	if(NULL == (buf = (char *) p->malloc(p, len, "px_strdup"))) {
		px_error(p, PX_MemoryError, _("Could not allocate memory for string."));
		return(NULL);
	}
	memcpy(buf, str, len);
	return(buf);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
