#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "px_intern.h"
#include "paradox-mp.h"
#include "px_error.h"

#define MAXMEM 10000

struct mem {
	void *ptr;
	int size;
	char *caller;
};

static struct mem memlist[MAXMEM];
static int peakmem = 0;
static int summem = 0;

PXLIB_API void PXLIB_CALL
PX_mp_init() {
	memset(memlist, 0, MAXMEM*sizeof(struct mem));
}

PXLIB_API void * PXLIB_CALL
PX_mp_malloc(pxdoc_t *p, size_t size, const char *caller) {
	void *a;
	int i;
	a = (void *) malloc(size);
	i = 0;
	while((i < MAXMEM) && (memlist[i].ptr != NULL)) {
		i++;
	}
	if(i == MAXMEM) {
		fprintf(stderr, _("Aiii, no more space for new memory block."));
		fprintf(stderr, "\n");
	}
	memlist[i].ptr = a;
	memlist[i].size = size;
	summem += size;
	peakmem = (summem > peakmem) ? summem : peakmem;
	memlist[i].caller = strdup(caller);
	return(a);
}

PXLIB_API void * PXLIB_CALL
PX_mp_realloc(pxdoc_t *p, void *mem, size_t size, const char *caller) {
	void *a;
	int i;
	a = realloc(mem, size);
	for(i=0; i<MAXMEM; i++) {
		if(memlist[i].ptr == mem) {
			memlist[i].ptr = a;
			summem -= memlist[i].size;
			summem += size;
			memlist[i].size = size;
			free(memlist[i].caller);
			memlist[i].caller = strdup(caller);
		}
	}
	if(i == MAXMEM) {
		fprintf(stderr, _("Aiii, did not find memory block at 0x%X to enlarge."), (unsigned int) mem);
		fprintf(stderr, "\n");
	}
	return(a);
}

PXLIB_API void PXLIB_CALL
PX_mp_free(pxdoc_t *p, void *mem) {
	int i;
	i = 0;
	while((i < MAXMEM) && (memlist[i].ptr != mem)) {
		i++;
	}
	if(i == MAXMEM) {
		fprintf(stderr, _("Aiii, did not find memory block at 0x%X to free."), (unsigned int) mem);
		fprintf(stderr, "\n");
	} else {
		memlist[i].ptr = NULL;
		summem -= memlist[i].size;
		memlist[i].size = 0;
		free(memlist[i].caller);
	}
	free(mem);
}

PXLIB_API void PXLIB_CALL
PX_mp_list_unfreed() {
	int i, j;
	i = j = 0;
	while(i < MAXMEM) {
		if(memlist[i].ptr) {
			fprintf(stderr, _("%d. Memory at address 0x%X (%d) not freed: '%s'."), j, (unsigned int) memlist[i].ptr, memlist[i].size, memlist[i].caller);
			fprintf(stderr, "\n");
			j++;
		}
		i++;
	}
	fprintf(stderr, _("Remaining unfreed memory: %d Bytes."), summem);
	fprintf(stderr, "\n");
	fprintf(stderr, _("Max. amount of memory used: %d Bytes."), peakmem);
	fprintf(stderr, "\n");
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
