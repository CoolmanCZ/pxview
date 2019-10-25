#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "px_intern.h"
#include "paradox.h"

/* px_init_targetencoding() {{{
 */
void px_init_targetencoding(pxdoc_t *pxdoc) {
#if PX_USE_RECODE
	pxdoc->out_recode_outer = recode_new_outer(false);
	pxdoc->out_recode_request = recode_new_request(pxdoc->out_recode_outer);
#else
#if PX_USE_ICONV
	pxdoc->out_iconvcd = (iconv_t) -1;
#endif
#endif
}
/* }}} */

/* px_init_inputencoding() {{{
 */
void px_init_inputencoding(pxdoc_t *pxdoc) {
#if PX_USE_RECODE
	pxdoc->in_recode_outer = recode_new_outer(false);
	pxdoc->in_recode_request = recode_new_request(pxdoc->in_recode_outer);
#else
#if PX_USE_ICONV
	pxdoc->in_iconvcd = (iconv_t) -1;
#endif
#endif
}
/* }}} */

/* px_set_targetencoding() {{{
 */
int px_set_targetencoding(pxdoc_t *pxdoc) {
	if(pxdoc->targetencoding) {
#if PX_USE_RECODE
		char buffer[30];
		if(NULL == pxdoc->out_recode_outer)
			px_init_targetencoding(pxdoc);
		sprintf(buffer, "CP%d/CR-LF..%s", pxdoc->px_head->px_doscodepage, pxdoc->targetencoding);
		recode_scan_request(pxdoc->out_recode_request, buffer);
#else
#if PX_USE_ICONV
		char buffer[30];
		sprintf(buffer, "CP%d", pxdoc->px_head->px_doscodepage);
		if(pxdoc->out_iconvcd > 0)
			iconv_close(pxdoc->out_iconvcd);
		if((iconv_t)(-1) == (pxdoc->out_iconvcd = iconv_open(pxdoc->targetencoding, buffer))) {
			return -1;
		} else {
			return 0;
		}
#endif
#endif
	} else {
		return -1;
	}
	return 0;
}
/* }}} */

/* px_set_inputencoding() {{{
 */
int px_set_inputencoding(pxdoc_t *pxdoc) {
	if(pxdoc->inputencoding) {
#if PX_USE_RECODE
		char buffer[30];
		sprintf(buffer, "%s..CP%d/CR-LF", pxdoc->inputencoding, pxdoc->px_head->px_doscodepage);
		recode_scan_request(pxdoc->in_recode_request, buffer);
#else
#if PX_USE_ICONV
		char buffer[30];
		sprintf(buffer, "CP%d", pxdoc->px_head->px_doscodepage);
		if(pxdoc->in_iconvcd > 0)
			iconv_close(pxdoc->in_iconvcd);
		if((iconv_t)(-1) == (pxdoc->in_iconvcd = iconv_open(buffer, pxdoc->inputencoding))) {
			return -1;
		} else {
			return 0;
		}
#endif
#endif
	} else {
		return -1;
	}
	return 0;
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
