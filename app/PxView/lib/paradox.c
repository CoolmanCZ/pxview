/*
 *    (c) Copyright 2003-2005  Uwe Steinmann.
 *    All rights reserved.
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 2 of the License, or (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the
 *    Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *    Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#if defined(WIN32) || defined(OS2)
#include <fcntl.h>
#endif
#include <time.h>

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#endif

#include "pxversion.h"
#include "px_intern.h"
#include "paradox-gsf.h"
#include "px_memory.h"
#include "px_head.h"
#include "px_io.h"
#include "px_error.h"
#include "px_misc.h"
#include "px_encode.h"
#include "px_crypt.h"

#ifndef WIN32
#define max(a,b) ((a)>(b) ? (a) : (b))
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif


/* PX_get_majorversion() {{{
 */
PXLIB_API int PXLIB_CALL
PX_get_majorversion(void) {
	return(PXLIB_MAJOR_VERSION);
}
/* }}} */

/* PX_get_minorversion() {{{
 */
PXLIB_API int PXLIB_CALL
PX_get_minorversion(void) {
	return(PXLIB_MINOR_VERSION);
}
/* }}} */

/* PX_get_subminorversion() {{{
 */
PXLIB_API int PXLIB_CALL
PX_get_subminorversion(void) {
	return(PXLIB_MICRO_VERSION);
}
/* }}} */

/* PX_has_recode_support() {{{
 */
PXLIB_API int PXLIB_CALL
PX_has_recode_support(void) {
#if PX_USE_RECODE
	return(1);
#else
#if PX_USE_ICONV
	return(2);
#endif
#endif
	return(0);
}
/* }}} */

/* PX_has_gsf_support() {{{
 */
PXLIB_API int PXLIB_CALL
PX_has_gsf_support(void) {
#if HAVE_GSF
	return(1);
#endif
	return(0);
}
/* }}} */

/* PX_is_bigendian() {{{
 */
PXLIB_API int PXLIB_CALL
PX_is_bigendian(void) {
#if WORDS_BIGENDIAN
	return(1);
#else
	return(0);
#endif
}
/* }}} */

/* PX_get_builddate() {{{
 */
PXLIB_API char * PXLIB_CALL
PX_get_builddate(void) {
#ifdef PXLIB_BUILD_DATE
	return(PXLIB_BUILD_DATE);
#else
	return("");
#endif
}
/* }}} */

/* PX_boot() {{{
 * Make some initial preparations for the whole library, e.g. set text domain.
 */
PXLIB_API void PXLIB_CALL
PX_boot(void) {
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#endif
}
/* }}} */

/* PX_shutdown() {{{
 * Make some final cleanup for the whole library. Counter part to PX_boot().
 */
PXLIB_API void PXLIB_CALL
PX_shutdown(void) {
}

/* }}} */

/* PX_new3() {{{
 * Create a new Paradox DB file and set memory management, error
 * handling functions and the user data passed to the error handler.
 * errorhandler can be NULL. If allocproc is NULL then none of the
 * memory management functions will be used.
 */
PXLIB_API pxdoc_t* PXLIB_CALL
PX_new3(void  (*errorhandler)(pxdoc_t *p, int type, const char *msg, void *data),
        void* (*allocproc)(pxdoc_t *p, size_t size, const char *caller),
        void* (*reallocproc)(pxdoc_t *p, void *mem, size_t size, const char *caller),
        void  (*freeproc)(pxdoc_t *p, void *mem),
		void* errorhandler_user_data) {
	pxdoc_t *pxdoc;

	if (errorhandler == NULL)
		errorhandler = px_errorhandler;

	if(allocproc == NULL) {
		allocproc = _px_malloc;
		reallocproc = _px_realloc;
		freeproc  = _px_free;
	} else if(allocproc != NULL && (reallocproc == NULL || freeproc == NULL)) {
		(*errorhandler)(NULL, PX_RuntimeError, _("Must set all memory management functions or none."), errorhandler_user_data);
		return(NULL);
	}

	if(NULL == (pxdoc = (pxdoc_t *) (* allocproc) (NULL, sizeof(pxdoc_t), "PX_new3: Allocate memory for px document."))) {
		(*errorhandler)(NULL, PX_MemoryError, _("Could not allocate memory for PX object."), errorhandler_user_data);
		return(NULL);
	}
	memset((void *)pxdoc, 0, (size_t) sizeof(pxdoc_t));
	pxdoc->errorhandler = errorhandler;
	pxdoc->errorhandler_user_data = errorhandler_user_data;
	pxdoc->malloc = allocproc;
	pxdoc->realloc = reallocproc;
	pxdoc->free = freeproc;
	pxdoc->px_stream = NULL;

	pxdoc->px_head = NULL;
	pxdoc->px_pindex = NULL;

	pxdoc->last_position = -1;
#if PX_USE_ICONV
	pxdoc->in_iconvcd = (iconv_t) -1;
	pxdoc->out_iconvcd = (iconv_t) -1;
#endif
	pxdoc->targetencoding = NULL;
	pxdoc->inputencoding = NULL;
	pxdoc->px_data = NULL;
	pxdoc->px_datalen = 0;
	pxdoc->curblocknr = 0;

	return pxdoc;
}
/* }}} */

/* PX_new2() {{{
 * Create a new Paradox DB file and set memory management and error
 * handling functions.
 */
PXLIB_API pxdoc_t* PXLIB_CALL
PX_new2(void  (*errorhandler)(pxdoc_t *p, int type, const char *msg, void *data),
        void* (*allocproc)(pxdoc_t *p, size_t size, const char *caller),
        void* (*reallocproc)(pxdoc_t *p, void *mem, size_t size, const char *caller),
        void  (*freeproc)(pxdoc_t *p, void *mem)) {
	return(PX_new3(errorhandler, allocproc, reallocproc, freeproc, NULL));
}
/* }}} */

/* PX_new() {{{
 * Create new Paradox DB file.
 * Use the default memory management and error handling functions.
 */
PXLIB_API pxdoc_t* PXLIB_CALL
PX_new(void) {
	return(PX_new3(NULL, NULL, NULL, NULL, NULL));
}
/* }}} */

/* PX_get_opaque() {{{
 * Returns the pointer on the user data as it is passed to each call
 * of the errorhandler.
 */
PXLIB_API void* PXLIB_CALL
PX_get_opaque(pxdoc_t *pxdoc) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return NULL;
	}
	return(pxdoc->errorhandler_user_data);
}
/* }}} */

/* PX_set_io_stream() {{{
 * Sets file access routines.
 */
PXLIB_API int PXLIB_CALL
PX_set_io_stream(pxdoc_t *pxdoc,
                 ssize_t (*readproc)(pxdoc_t *p, pxstream_t *stream, size_t numbytes, void *buffer),
                 ssize_t (*writeproc)(pxdoc_t *p, pxstream_t *stream, size_t len, void *data),
                 int (*seekproc)(pxdoc_t *p, pxstream_t *stream, long offset, int whence),
                 long (*tellproc)(pxdoc_t *p, pxstream_t *stream)
				 ) {
	pxstream_t *pxs;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(NULL == (pxs = px_stream_new(pxdoc))) {
		px_error(pxdoc, PX_MemoryError, _("Could not create new io stream."));
		return -1;
	}
	
	pxs->read = readproc;
	pxs->seek = seekproc;
	pxs->tell = tellproc;
	pxs->write = writeproc;

	pxdoc->px_stream = pxs;

	return(0);
}
/* }}} */

/* build_primary_index() {{{
 * Build a primary index.
 */
static int build_primary_index(pxdoc_t *pxdoc) {
	pxhead_t *pxh;
	pxstream_t *pxs;
	pxpindex_t *pindex;
	int blockcount, blocknumber, numrecords;

	pxh = pxdoc->px_head;
	pxs = pxdoc->px_stream;

	/* The internal list of index entries will only contain level 1
	 * entries. Whether we need level 2 entries depends on the size
	 * of the datablock in the primary index file. Level 2 entries
	 * will be created when the primary index file is written.
	 * Nevertheless the internal index entry has a field level, which
	 * is currently always set to 1.
	 */
	/* free an existing index before creating a new one */
	if(pxdoc->px_indexdata) {
		pxdoc->free(pxdoc, pxdoc->px_indexdata);
	}
	/* Allocate memory for internal list of index entries */
//	fprintf(stderr, "fileblocks = %d\n", pxh->px_fileblocks);
	if(NULL == (pindex = pxdoc->malloc(pxdoc, pxh->px_fileblocks*sizeof(pxpindex_t), _("Allocate memory for self build internal primary index.")))) {
		px_error(pxdoc, PX_MemoryError, _("Could not allocate memory for self build internal index."));
		return -1;
	}

	/* Build Index of Level 1 */
	pxdoc->px_indexdata = pindex;
	pxdoc->px_indexdatalen = pxh->px_fileblocks;
	blockcount = 0; /* Just a block counter */
	numrecords = 0;
	blocknumber = pxh->px_firstblock; /* Will be set to next block number */
	while((blockcount < pxh->px_fileblocks) && (blocknumber > 0)) {
		TDataBlock datablockhead;
		if(get_datablock_head(pxdoc, pxs, blocknumber, &datablockhead) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not get head of data block nr. %d."), blocknumber);
			pxdoc->free(pxdoc, pindex);
			return -1;
		}
		/* The data can be NULL because we don't support searching for field
		 * data yet. */
		pindex[blockcount].data = NULL;
		pindex[blockcount].blocknumber = blocknumber;
		pindex[blockcount].numrecords = (get_short_le((char *) &datablockhead.addDataSize)/pxh->px_recordsize)+1;

		numrecords += pindex[blockcount].numrecords;
		if(pindex[blockcount].numrecords == 0) {
			fprintf(stderr, _("Block with number %d has no records"), blocknumber);
			fprintf(stderr, "\n");
		}
		pindex[blockcount].myblocknumber = 0;
		pindex[blockcount].level = 1;
		blocknumber = get_short_le((const char *) &datablockhead.nextBlock);
		blockcount++;
	}
	/* Check if the number of records in the blocks sums up to number
	 * of records in the header
	 */
	if(numrecords != pxh->px_numrecords) {
		fprintf(stderr, _("Number of records counted in blocks does not match number of records in header (%d != %d)"), numrecords, pxh->px_numrecords);
		fprintf(stderr, "\n");
	}

	/* Read remaining blocks. This should not happen, but I've seen a database
	 * where it does happen. So better check for it.
	 */
	if(blocknumber != 0) {
		while(blocknumber > 0) {
			TDataBlock datablockhead;
//			fprintf(stderr, "next blocknumber after creating primary index: %d\n", blocknumber);
			if(get_datablock_head(pxdoc, pxs, blocknumber, &datablockhead) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not get head of data block nr. %d."), blocknumber);
				pxdoc->free(pxdoc, pindex);
				return -1;
			}
			/* The data can be NULL because we don't support searching for field
			 * data yet. */
/*			pindex[blockcount].data = NULL;
			pindex[blockcount].blocknumber = blocknumber;
			pindex[blockcount].numrecords = (get_short_le((char *) &datablockhead.addDataSize)/pxh->px_recordsize)+1;
			pindex[blockcount].myblocknumber = 0;
			pindex[blockcount].level = 1;
*/			blocknumber = get_short_le((const char *) &datablockhead.nextBlock);
			blockcount++;
		}
	}
	return 0;
}
/* }}} */

/* PX_open_stream() {{{
 * Read from a Paradox DB file, which has an already open stream.
 */
PXLIB_API int PXLIB_CALL
PX_open_stream(pxdoc_t *pxdoc, void *stream) {
	pxhead_t *pxh;
	pxstream_t *pxs;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_stream == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Paradox database has no stream."));
		return -1;
	}
	pxs = pxdoc->px_stream;

	pxdoc->px_stream->type = pxfIOStream;
	pxdoc->px_stream->mode = pxfFileRead;
	pxdoc->px_stream->close = px_false;
	pxdoc->px_stream->s.stream = stream;

	if((pxdoc->px_head = get_px_head(pxdoc, pxs)) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Unable to get header."));
		return -1;
	}

	/* Build primary index. This index misses all index blocks with a level
	 * greater than 1. Since they are not used currently this is of no harm.
	 */
	pxh = pxdoc->px_head;
	if(pxh->px_filetype == pxfFileTypIndexDB ||
	   pxh->px_filetype == pxfFileTypNonIndexDB ||
	   pxh->px_filetype == pxfFileTypNonIncSecIndex ||
	   pxh->px_filetype == pxfFileTypIncSecIndex ||
	   pxh->px_filetype == pxfFileTypNonIncSecIndexG ||
	   pxh->px_filetype == pxfFileTypIncSecIndexG) {
		if(build_primary_index(pxdoc) < 0) {
			return -1;
		}
	}
	return 0;
}
/* }}} */

#if HAVE_GSF
/* PX_open_gsf() {{{
 * Read from a Paradox DB file, which has already been opened with gsf.
 */
PXLIB_API int PXLIB_CALL
PX_open_gsf(pxdoc_t *pxdoc, GsfInput *gsf) {
	pxhead_t *pxh;
	pxstream_t *pxs;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(NULL == (pxs = px_stream_new_gsf(pxdoc, pxfFileRead, px_false, gsf))) {
		px_error(pxdoc, PX_MemoryError, _("Could not create new gsf io stream."));
		return -1;
	}
	pxdoc->px_stream = pxs;

	pxdoc->read = px_read;
	pxdoc->seek = px_seek;
	pxdoc->tell = px_tell;
	pxdoc->write = px_write;

	if((pxdoc->px_head = get_px_head(pxdoc, pxs)) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Unable to get header."));
		return -1;
	}

	/* Build primary index. This index misses all index blocks with a level
	 * greater than 1. Since they are not used currently this is of no harm.
	 */
	pxh = pxdoc->px_head;
	if(pxh->px_filetype == pxfFileTypIndexDB ||
	   pxh->px_filetype == pxfFileTypNonIndexDB ||
	   pxh->px_filetype == pxfFileTypNonIncSecIndex ||
	   pxh->px_filetype == pxfFileTypIncSecIndex ||
	   pxh->px_filetype == pxfFileTypNonIncSecIndexG ||
	   pxh->px_filetype == pxfFileTypIncSecIndexG) {
		if(build_primary_index(pxdoc) < 0) {
			return -1;
		}
	}
	return 0;
}
/* }}} */
#endif /* HAVE_GSF */

/* PX_open_fp() {{{
 * Read from a Paradox DB file, which has already been opend with fopen.
 */
PXLIB_API int PXLIB_CALL
PX_open_fp(pxdoc_t *pxdoc, FILE *fp) {
	pxhead_t *pxh;
	pxstream_t *pxs;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(NULL == (pxs = px_stream_new_file(pxdoc, pxfFileRead, px_false, fp))) {
		px_error(pxdoc, PX_MemoryError, _("Could not create new file io stream."));
		return -1;
	}

	pxdoc->px_stream = pxs;

	pxdoc->read = px_read;
	pxdoc->seek = px_seek;
	pxdoc->tell = px_tell;
	pxdoc->write = px_write;

	if((pxdoc->px_head = get_px_head(pxdoc, pxs)) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Unable to get header."));
		return -1;
	}
	pxdoc->last_position = pxdoc->px_head->px_numrecords-1;

	/* Build primary index. This index misses all index blocks with a level
	 * greater than 1. Since they are not used currently this is of no harm.
	 */
	pxh = pxdoc->px_head;
	if(pxh->px_filetype == pxfFileTypIndexDB ||
	   pxh->px_filetype == pxfFileTypNonIndexDB ||
	   pxh->px_filetype == pxfFileTypNonIncSecIndex ||
	   pxh->px_filetype == pxfFileTypIncSecIndex ||
	   pxh->px_filetype == pxfFileTypNonIncSecIndexG ||
	   pxh->px_filetype == pxfFileTypIncSecIndexG) {
		if(build_primary_index(pxdoc) < 0) {
			return -1;
		}
	}
	return 0;
}
/* }}} */

/* PX_open_file() {{{
 * Read from a Paradox DB file. Open the file itself. Use PX_open_fp()
 * if the file has been open already with fopen().
 */
PXLIB_API int PXLIB_CALL
PX_open_file(pxdoc_t *pxdoc, const char *filename) {
	FILE *fp;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if((fp = fopen(filename, "rb+")) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Could not open file of paradox database: %s"), strerror(errno));
		return -1;
	}

	if(0 > PX_open_fp(pxdoc, fp)) {
		px_error(pxdoc, PX_RuntimeError, _("Could not open paradox database."));
		fclose(fp);
		return -1;
	}

	pxdoc->px_name = px_strdup(pxdoc, filename);
	pxdoc->px_stream->close = px_true;
	return 0;
}
/* }}} */

/* PX_create_fp() {{{
 * Create a new paradox database.
 */
PXLIB_API int PXLIB_CALL
PX_create_fp(pxdoc_t *pxdoc, pxfield_t *fields, int numfields, FILE *fp, int type) {
	pxhead_t *pxh;
	pxfield_t *pxf;
	pxstream_t *pxs;
	int i, c, recordsize = 0;
	int approxheadersize = 0; /* The name indicates that this size not accurate,
							   * actually it is very precise. */

	pxf = fields;
	for(i=0, c=0; i<numfields; i++, pxf++) {
		if(pxf->px_ftype == pxfAutoInc)
			c++;
	}
	if(c > 1) {
		px_error(pxdoc, PX_Warning, _("Database has %d auto increment fields. The automatic incrementation works only with one field of that type."), c);
	}
	if((pxh = (pxhead_t *) pxdoc->malloc(pxdoc, sizeof(pxhead_t), _("Allocate memory for database header."))) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Could not allocate memory for database header."));
		return -1;
	}
	pxh->px_filetype = type; //pxfFileTypNonIndexDB;
	pxh->px_fileversion = 70;
	pxh->px_tablename = NULL;
	pxh->px_numrecords = 0;
	pxh->px_numfields = numfields;
	pxh->px_fields = fields;
	pxh->px_writeprotected = 0;
	pxh->px_indexfieldnumber = 0;
	pxh->px_numindexlevels = 0;
	pxh->px_indexroot = 0;
	pxh->px_headersize = 0x0800; /* default, will be recalculated below */
	pxh->px_fileblocks = 0;
	pxh->px_firstblock = 0;
	pxh->px_lastblock = 0;
	pxh->px_maxtablesize = 16;
	pxh->px_doscodepage = 1252;
	pxh->px_primarykeyfields = 0;
	pxh->px_autoinc = 0;
	if(type == pxfFileTypPrimIndex)
		pxh->px_autoinc = 1;
	pxh->px_sortorder = 0x62;
	pxh->px_encryption = 0;

	/* secondary index files have primarykeyfields always set to 2
	 * and sort order (stored in px_refintegrity is set to ascending
	 * by default.
	 */
	if((type == pxfFileTypIncSecIndex) ||
	   (type == pxfFileTypNonIncSecIndex) ||
	   (type == pxfFileTypIncSecIndexG) ||
	   (type == pxfFileTypNonIncSecIndexG)) {
		pxh->px_primarykeyfields = 2;
		pxh->px_refintegrity = 1;
	}

	/* Calculate record size and get an idea on how big the header might
	 * get due to the fieldnames, which is the major none fixed size. */
	pxf = pxh->px_fields;
	for(i=0; i<pxh->px_numfields; i++, pxf++) {
		recordsize += pxf->px_flen;
		if(pxf->px_fname)
			approxheadersize += strlen(pxf->px_fname) + 1;
	}
	if(type == pxfFileTypPrimIndex ||
	   type == pxfFileTypSecIndex ||
	   type == pxfFileTypSecIndexG)
		recordsize += 6;
	pxh->px_recordsize = recordsize;
	if(recordsize < 80) {
		pxh->px_maxtablesize = 2;
	} else if(recordsize < 140) {
		pxh->px_maxtablesize = 3;
	}

	/* add fixed size part of header */
	approxheadersize += sizeof(TPxHeader);
	/* The following files have a data header as well */
	if(((pxh->px_filetype == pxfFileTypIndexDB) ||
		  (pxh->px_filetype == pxfFileTypNonIndexDB) ||
		  (pxh->px_filetype == pxfFileTypNonIncSecIndex) ||
		  (pxh->px_filetype == pxfFileTypIncSecIndex) ||
		  (pxh->px_filetype == pxfFileTypNonIncSecIndexG) ||
		  (pxh->px_filetype == pxfFileTypIncSecIndexG)) &&
		  (pxh->px_fileversion >= 40)) {
		approxheadersize += sizeof(TPxDataHeader);
	}
	/* add size for field info records */
	approxheadersize += numfields * sizeof(TFldInfoRec);
	/* add size for tablename and tablename pointer */
	approxheadersize += 261 + sizeof(pchar);
	/* add size for field name pointers */
	if(((pxh->px_filetype == pxfFileTypIndexDB) ||
		  (pxh->px_filetype == pxfFileTypNonIndexDB) ||
		  (pxh->px_filetype == pxfFileTypNonIncSecIndex) ||
		  (pxh->px_filetype == pxfFileTypIncSecIndex) ||
		  (pxh->px_filetype == pxfFileTypNonIncSecIndexG) ||
		  (pxh->px_filetype == pxfFileTypIncSecIndexG)) &&
		  (pxh->px_fileversion >= 40)) {
		approxheadersize += numfields * sizeof(pchar);
		/* next would be the field names which has been added already */
		/* we don't need space for cryptInfo */
		/* add size of field numbers */
		approxheadersize += numfields * 2;
		/* add size for sort order */
		approxheadersize += 8;
	}

	/* calculate header size, which is always a multiple of 0x800 */
	pxh->px_headersize = ((approxheadersize / 0x800) + 1) * 0x0800;
//	fprintf(stderr, "Set headersize to 0x%X (%d)\n", pxh->px_headersize, approxheadersize);

	if(NULL == (pxs = px_stream_new_file(pxdoc, pxfFileWrite, px_false, fp))) {
		px_error(pxdoc, PX_MemoryError, _("Could not create new file io stream."));
		return -1;
	}

	pxdoc->px_stream = pxs;

	pxdoc->read = px_read;
	pxdoc->seek = px_seek;
	pxdoc->tell = px_tell;
	pxdoc->write = px_write;

	if(put_px_head(pxdoc, pxh, pxs) < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Unable to put header."));
		return -1;
	}

	pxdoc->px_head = pxh;
	return 0;
}
/* }}} */

/* PX_create_file() {{{
 * Create a Paradox DB file. Open the file itself. Use PX_create_fp()
 * if the file has been open already with fopen().
 */
PXLIB_API int PXLIB_CALL
PX_create_file(pxdoc_t *pxdoc, pxfield_t *fields, int numfields, const char *filename, int type) {
	FILE *fp;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if((fp = fopen(filename, "wb+")) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Could not create file for paradox database: %s"), strerror(errno));
		return -1;
	}

	if(0 > PX_create_fp(pxdoc, fields, numfields, fp, type)) {
		px_error(pxdoc, PX_RuntimeError, _("Could not open paradox database."));
		fclose(fp);
		return -1;
	}

//	pxh->px_tablename = px_strdup(pxdoc, filename);
	PX_set_tablename(pxdoc, filename);
	pxdoc->px_name = px_strdup(pxdoc, filename);
	pxdoc->px_stream->close = px_true;
	return 0;
}
/* }}} */

/* PX_set_value() {{{
 * Sets a numeric value
 */
PXLIB_API int PXLIB_CALL
PX_set_value(pxdoc_t *pxdoc, const char *name, float value) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(!(pxdoc->px_stream->mode & pxfFileWrite)) {
		px_error(pxdoc, PX_Warning, _("File is not writable. Setting '%s' has no effect."), name);
		return -1;
	}

	if(strcmp(name, "numprimkeys") == 0) {
		if(value < 0) {
			px_error(pxdoc, PX_Warning, _("Number of primary keys must be greater than or equal to 0."), name);
			return -1;
		}
		pxdoc->px_head->px_primarykeyfields = (int) value;
		/* FIXME: You can't change the file type if the number of
		 * primarykeyfields changes.
		 */
		if(value == 0) {
			pxdoc->px_head->px_filetype = pxfFileTypNonIndexDB;
		} else {
			pxdoc->px_head->px_filetype = pxfFileTypIndexDB;
		}
		if(put_px_head(pxdoc, pxdoc->px_head, pxdoc->px_stream) < 0) {
			return -1;
		}
	} else if(strcmp(name, "codepage") == 0) {
		if(value <= 0) {
			px_error(pxdoc, PX_Warning, _("codepage must be greater than 0."), name);
			return -1;
		}
		pxdoc->px_head->px_doscodepage = (int) value;
		if(put_px_head(pxdoc, pxdoc->px_head, pxdoc->px_stream) < 0) {
			return -1;
		}
	} else {
		px_error(pxdoc, PX_Warning, _("There is no such value like '%s' to set."), name);
		return -1;
	}
	return(0);
}
/* }}} */

/* PX_get_value() {{{
 * Gets a numeric value
 */
PXLIB_API int PXLIB_CALL
PX_get_value(pxdoc_t *pxdoc, const char *name, float *value) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(strcmp(name, "numprimkeys") == 0) {
		*value = (float) pxdoc->px_head->px_primarykeyfields;
		return(0);
	} else if(strcmp(name, "filetype") == 0) {
		*value = (float) pxdoc->px_head->px_filetype;
		return(0);
	} else if(strcmp(name, "numfields") == 0) {
		*value = (float) pxdoc->px_head->px_numfields;
		return(0);
	} else if(strcmp(name, "primarykeyfields") == 0) {
		*value = (float) pxdoc->px_head->px_primarykeyfields;
		return(0);
	} else if(strcmp(name, "numrecords") == 0) {
		*value = (float) pxdoc->px_head->px_numrecords;
		return(0);
	} else if(strcmp(name, "recordsize") == 0) {
		*value = (float) pxdoc->px_head->px_recordsize;
		return(0);
	} else if(strcmp(name, "theonumrecords") == 0) {
		*value = (float) pxdoc->px_head->px_theonumrecords;
		return(0);
	} else if(strcmp(name, "recordsperblock") == 0) {
		*value = (float) (pxdoc->px_head->px_maxtablesize*0x400-sizeof(TDataBlock)) / pxdoc->px_head->px_recordsize;
		return(0);
	} else if(strcmp(name, "fileversion") == 0) {
		*value = (float) pxdoc->px_head->px_fileversion/10.0;
		return(0);
	} else if(strcmp(name, "headersize") == 0) {
		*value = (float) pxdoc->px_head->px_headersize;
		return(0);
	} else if(strcmp(name, "maxtablesize") == 0) {
		*value = (float) pxdoc->px_head->px_maxtablesize;
		return(0);
	} else if(strcmp(name, "numblocks") == 0) {
		*value = (float) pxdoc->px_head->px_fileblocks;
		return(0);
	} else if(strcmp(name, "firstblock") == 0) {
		*value = (float) pxdoc->px_head->px_firstblock;
		return(0);
	} else if(strcmp(name, "lastblock") == 0) {
		*value = (float) pxdoc->px_head->px_lastblock;
		return(0);
	} else if(strcmp(name, "codepage") == 0) {
		*value = (float) pxdoc->px_head->px_doscodepage;
		return(0);
	} else if(strcmp(name, "autoinc") == 0) {
		*value = (float) pxdoc->px_head->px_autoinc;
		return(0);
	} else if(strcmp(name, "sortorder") == 0) {
		*value = (float) pxdoc->px_head->px_sortorder;
		return(0);
	} else if(strcmp(name, "encryption") == 0) {
		*value = (float) pxdoc->px_head->px_encryption;
		return(0);
	}
	px_error(pxdoc, PX_Warning, _("No such value name."));
	return(-2);
}
/* }}} */

/* PX_set_parameter() {{{
 * Sets a string value
 */
PXLIB_API int PXLIB_CALL
PX_set_parameter(pxdoc_t *pxdoc, const char *name, const char *value) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Header of file has not been read."));
		return -1;
	}

	if(strcmp(name, "tablename") == 0) {
		if(pxdoc->px_head->px_tablename)
			pxdoc->free(pxdoc, pxdoc->px_head->px_tablename);

		pxdoc->px_head->px_tablename = px_strdup(pxdoc, value);
		if(pxdoc->px_stream->mode & pxfFileWrite) {
			if(put_px_head(pxdoc, pxdoc->px_head, pxdoc->px_stream) < 0) {
				return -1;
			}
		} else {
			px_error(pxdoc, PX_Warning, _("File is not writable. Setting '%s' has no effect."), name);
			return -1;
		}
	} else if(strcmp(name, "password") == 0) {
		pxdoc->px_head->px_encryption = px_passwd_checksum(value);
		if(pxdoc->px_stream->mode & pxfFileWrite) {
			if(put_px_head(pxdoc, pxdoc->px_head, pxdoc->px_stream) < 0) {
				return -1;
			}
		} else {
			px_error(pxdoc, PX_Warning, _("File is not writable. Setting '%s' has no effect."), name);
			return -1;
		}
	} else if(strcmp(name, "targetencoding") == 0) {
		int codepage;
#if PX_USE_RECODE || PX_USE_ICONV
		if(pxdoc->targetencoding)
			pxdoc->free(pxdoc, pxdoc->targetencoding);
		pxdoc->targetencoding = px_strdup(pxdoc, value);
		if(0 > px_set_targetencoding(pxdoc)) {
			pxdoc->free(pxdoc, pxdoc->targetencoding);
			pxdoc->targetencoding = NULL;
			px_error(pxdoc, PX_RuntimeError, _("Target encoding could not be set."));
			return -1;
		}
#else
		px_error(pxdoc, PX_RuntimeError, _("Library has not been compiled with support for reencoding."));
#endif
		if(sscanf(value, "CP%d", &codepage)) {
			PX_set_value(pxdoc, "codepage", codepage);
		}
	} else if(strcmp(name, "inputencoding") == 0) {
#if PX_USE_RECODE || PX_USE_ICONV
		if(pxdoc->inputencoding)
			pxdoc->free(pxdoc, pxdoc->inputencoding);
		pxdoc->inputencoding = px_strdup(pxdoc, value);
		if(0 > px_set_inputencoding(pxdoc)) {
			pxdoc->free(pxdoc, pxdoc->inputencoding);
			pxdoc->inputencoding = NULL;
			px_error(pxdoc, PX_RuntimeError, _("Input encoding could not be set."));
			return -1;
		}
#else
		px_error(pxdoc, PX_RuntimeError, _("Library has not been compiled with support for reencoding."));
#endif
	} else if(strcmp(name, "warning") == 0) {
		if(strcmp(value, "true") == 0) {
			pxdoc->warnings = px_true;
		} else {
			pxdoc->warnings = px_false;
	    }
	}
	return 0;
}
/* }}} */

/* PX_get_parameter() {{{
 * Gets a string value
 */
PXLIB_API int PXLIB_CALL
PX_get_parameter(pxdoc_t *pxdoc, const char *name, char **value) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Header of file has not been read."));
		return -1;
	}

	if(strcmp(name, "tablename") == 0) {
		*value = pxdoc->px_head->px_tablename;
		return(0);
	} else if(strcmp(name, "targetencoding") == 0) {
		*value = pxdoc->targetencoding;
		return(0);
	} else if(strcmp(name, "inputencoding") == 0) {
		*value = pxdoc->inputencoding;
		return(0);
	}
	px_error(pxdoc, PX_Warning, _("No such parameter name."));
	return(-2);
}
/* }}} */

/* PX_add_primary_index() {{{
 * Use a primary index for an DB file. The index has to be opened before
 * with PX_open_fp() PX_open_file(). After adding an index it will be
 * used for accessing database records.
 * If this function has been called before for the same DB file, the
 * old index will be deleted first. Make sure to actually read the
 * index with PX_read_primary_index() and not just open it.
 */
PXLIB_API int PXLIB_CALL
PX_add_primary_index(pxdoc_t *pxdoc, pxdoc_t *pindex) {
	pxfield_t *pfielddb, *pfieldpx;
	pxpindex_t *pindex_data;
	int records, i;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Header of file has not been read."));
		return -1;
	}

	if(pxdoc->px_head->px_filetype != pxfFileTypIndexDB) {
		px_error(pxdoc, PX_RuntimeError, _("Cannot add a primary index to a database which is not of type 'IndexDB'."));
		return -1;
	}

	if(pindex == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox index file."));
		return -1;
	}
	if(pindex->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Header of index file has not been read."));
		return -1;
	}

	if(pindex->px_head->px_filetype != pxfFileTypPrimIndex) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox primary index file."));
		return -1;
	}

	if(pindex->px_data == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Primary index file has no index data."));
		return -1;
	}

	if(pindex->px_head->px_numfields != pxdoc->px_head->px_primarykeyfields) {
		px_error(pxdoc, PX_RuntimeError, _("Number of primary index fields in database and and number fields in primary index differ."));
		return -1;
	}

	for(i=0; i<pindex->px_head->px_numfields; i++) {
		pfielddb = PX_get_field(pxdoc, i);
		pfieldpx = PX_get_field(pindex, i);
		if(pfielddb->px_ftype != pfieldpx->px_ftype) {
			px_error(pxdoc, PX_RuntimeError, _("Type of primary key field '%s' in database differs from index file."), pfielddb->px_fname);
			return -1;
		}
		if(pfielddb->px_flen != pfieldpx->px_flen) {
			px_error(pxdoc, PX_RuntimeError, _("Length of primary key field '%s' in database differs from index file."), pfielddb->px_fname);
			return -1;
		}
	}

	/* Calculate the number of records covered by the index file and
	 * compare it with the number of records in the db file. They must
	 * be identical, otherwise the index file is probably does not belong to
	 * the db file. */
	records = 0;
	pindex_data = (pxpindex_t *) pindex->px_data;
	for(i=0; i<pindex->px_head->px_numrecords; i++) {
		if(pindex_data[i].level == 1)
			records += pindex_data[i].numrecords;
	}
	if(records != pxdoc->px_head->px_numrecords) {
		px_error(pxdoc, PX_RuntimeError, _("Index file is for database with %d records, but database has %d records."), records, pxdoc->px_head->px_numrecords);
		return -1;
	}

	/* Delete an existing primary index file */
	if(pxdoc->px_pindex) {
		PX_delete(pxdoc->px_pindex);
	}
	pxdoc->px_pindex = pindex;
	pxdoc->px_indexdata = pindex->px_data;
	pxdoc->px_indexdatalen = pindex->px_head->px_numrecords;

	return 0;
}
/* }}} */

/* PX_read_primary_index() {{{
 * Read the primary index completly into an internal array.
 */
PXLIB_API int PXLIB_CALL
PX_read_primary_index(pxdoc_t *pindex) {
	pxpindex_t *pindex_data;
	pxhead_t *pxh;
	pxfield_t *pxf;
	char *data;
	int i, j, datalen;

	if(pindex == NULL ||
	   pindex->px_head == NULL ||
	   pindex->px_head->px_filetype != pxfFileTypPrimIndex) {
		px_error(pindex, PX_RuntimeError, _("Did not pass a paradox primary index file."));
		return -1;
	}

	pxh = pindex->px_head;
	pindex->px_data = pindex->malloc(pindex, pxh->px_numrecords*sizeof(pxpindex_t), _("Allocate memory for primary index data."));
	if(!pindex->px_data) {
		px_error(pindex, PX_RuntimeError, _("Could not allocate memory for primary index data."));
		return -1;
	}
	pindex->px_datalen = pxh->px_numrecords;

	pindex_data = (pxpindex_t *) pindex->px_data;
	memset(pindex_data, 0, pxh->px_numrecords*sizeof(pxpindex_t));

	if((data = (char *) pindex->malloc(pindex, pxh->px_recordsize, _("Allocate memory for data of index record."))) == NULL) {
		px_error(pindex, PX_RuntimeError, _("Could not allocate memory for primary index data."));
		pindex->free(pindex, pindex->px_data);
		return -1;
	}

	/* Read over the field data.
	 * px_numfields does not count the fields with information about
	 * block position and num of records per block. It is only the
	 * number of index fields. */
	datalen = 0;
	pxf = pxh->px_fields;
	for(i=0; i<pxh->px_numfields; i++) {
		datalen += pxf->px_flen;
		pxf++;
	}
	if(datalen != pxh->px_recordsize-6) {
		px_error(pindex, PX_RuntimeError, _("Inconsistency in length of primary index record. Expected %d but calculated %d."), pxh->px_recordsize-6, datalen);
		pindex->free(pindex, data);
		pindex->free(pindex, pindex->px_data);
		pindex->px_data = NULL;
		return(-1);
	}
	for(j=0; j<pxh->px_numrecords; j++) {
		pxdatablockinfo_t pxdbinfo;
		int isdeleted=0;
		if(PX_get_record2(pindex, j, data, &isdeleted, &pxdbinfo)) {
			short int value;
			/* Copy the data part for later sorting */
			pindex_data[j].data = pindex->malloc(pindex, datalen, _("Allocate memory for data part of index record."));
			memcpy(pindex_data[j].data, data, datalen);
			/* Get the index data */
			PX_get_data_short(pindex, &data[datalen], 2, &value);
			pindex_data[j].blocknumber = value;
			PX_get_data_short(pindex, &data[datalen+2], 2, &value);
			pindex_data[j].numrecords = value;
			PX_get_data_short(pindex, &data[datalen+4], 2, &value);
			pindex_data[j].dummy = value;
			pindex_data[j].myblocknumber = pxdbinfo.number;
		} else {
			px_error(pindex, PX_RuntimeError, _("Could not read record no. %d of primary index data."), j);
			/* Free so far allocated data memory */
			for(j--; j>=0; j--)
				pindex->free(pindex, pindex_data->data);
			pindex->free(pindex, data);
			pindex->free(pindex, pindex->px_data);
			pindex->px_data = NULL;
			return -1;
		}
	}
	/* find level of index blocks. Index blocks of level 1 contain references
	 * to data blocks. Index blocks of level n+1 contain references to index
	 * blocks of level n. */
	/* If the number of data blocks is 1 then there will be no index blocks
	 * of level 2, and all blocks will be of level 1.
	 * In all other case we expect only blocks of level 2 and 1.
	 * This is an assumption which is only true for a
	 * certain number of records, which is usually quite high. If for example
	 * each data block in the database contains 10 records, and each level 1
	 * index block contains 50 block references, you will end up in 500
	 * records. The next index level will enlarge this to 25000 if only
	 * on block will be used in this level.
	 * For now this has to be sufficient. */
	if(pxh->px_fileblocks == 1) {
		for(j=0; j<pxh->px_numrecords; j++)
			pindex_data[j].level = 1;
	} else {
		int firstblock = pindex_data[0].myblocknumber;
		int numrecords = 0;
		for(j=0; j<pxh->px_numrecords && pindex_data[j].myblocknumber == firstblock; j++) {
			numrecords += pindex_data[j].numrecords;
			pindex_data[j].level = 2;
		}
		for(; j<pxh->px_numrecords; j++) {
			numrecords -= pindex_data[j].numrecords;
			pindex_data[j].level = 1;
		}
		if(numrecords != 0) {
			px_error(pindex, PX_Warning, _("The number of records coverd by index level 2 is unequal to level 1."));
		}
	}
//	for(j=0; j<pxh->px_numrecords-1; j++) {
//		printf("%d\t%d\n", pindex_data[j].myblocknumber, pindex_data[j].level);
//	}

	pindex->free(pindex, data);
	return 0;
}
/* }}} */

/* PX_write_primary_index() {{{
 * Write the primary index. This function calls build_primary_index()
 * if it has not been called before.
 */
PXLIB_API int PXLIB_CALL
PX_write_primary_index(pxdoc_t *pxdoc, pxdoc_t *pxindex) {
	pxpindex_t *indexdata;
	pxfield_t *pxf;
	pxhead_t *pxh, *pih;
	char *data;
	int i, j;
	int recordsize, indexdatalen, numrecords, recordnr;
	int recsperblock = 0;
	int blocknumber = 1;

	pxh = pxdoc->px_head;
	pxf = pxh->px_fields;
	pih = pxindex->px_head;

	/* Allocate memory for a complete data record. Actually it would be
	 * sufficient to just read the first n fields which make up the
	 * primary index fields, but it doesn't really harm to read the
	 * whole record.
	 * Anyway, there could be cases where the data in pxdoc needs less
	 * memory than a data record for pindexdoc. Thats why we use
	 * the max function for allocating memory.
	 */
	recordsize = pih->px_recordsize;
	if((data = (char *) pxindex->malloc(pxindex, max(pxh->px_recordsize, recordsize), _("Allocate memory for data of index record."))) == NULL) {
		px_error(pxindex, PX_RuntimeError, _("Could not allocate memory for primary index data."));
		return -1;
	}

	if(NULL == pxdoc->px_indexdata) {
		if(build_primary_index(pxdoc) < 0) {
			return -1;
		}
	}
	indexdata = pxdoc->px_indexdata;
	indexdatalen = pxdoc->px_indexdatalen;

	/* set default values for indexRoot and numIndexLevels */
	pih->px_indexroot = 1;
	pih->px_numindexlevels = 1;

	/* Check if we need level 2 index entries. If the space needed for
	 * all level 1 entries is larger than a datablock in the index file,
	 * we will need level 2 entries.
	 * There is currently no support for level 3 entries.
	 */
	if(pih->px_maxtablesize*0x400-(int)sizeof(TDataBlock) < indexdatalen*pih->px_recordsize) {
		pih->px_numindexlevels = 2;
		recsperblock = (pih->px_maxtablesize*0x400-sizeof(TDataBlock)) / pih->px_recordsize;
		blocknumber = 2; /* The first block contains the level 2 entries */
		recordnr = 0;
		for(i=0; i<indexdatalen; i++) {
			PX_get_record(pxdoc, recordnr, data);
//			fprintf(stderr, "Get record %d for level 2 entry\n", recordnr);

			/* Accumulate index entries until a data block is filled. */
			j = 0;
			numrecords = 0;
			while((j < recsperblock) && (i < indexdatalen)) {
				numrecords += indexdata[i].numrecords;
				j++; i++;
			}
			i--;

			PX_put_data_short(pxindex, &data[recordsize-6], 2, blocknumber);
			PX_put_data_short(pxindex, &data[recordsize-4], 2, numrecords);
			PX_put_data_short(pxindex, &data[recordsize-2], 2, 0);
			PX_put_record(pxindex, data);
			blocknumber++;
			recordnr += numrecords;
		}
	}
	/* Loop through the index. Each entry points to first record of a
	 * datablock.
	 */
	recordnr = 0;
	for(i=0; i<indexdatalen; i++) {
		PX_get_record(pxdoc, recordnr, data);
		PX_put_data_short(pxindex, &data[recordsize-6], 2, indexdata[i].blocknumber);
		PX_put_data_short(pxindex, &data[recordsize-4], 2, indexdata[i].numrecords);
		PX_put_data_short(pxindex, &data[recordsize-2], 2, 0);
		PX_put_recordn(pxindex, data, recsperblock+i);
		recordnr += indexdata[i].numrecords;
	}
	pxindex->free(pxindex, data);
	return(0);
}
/* }}} */

/* px_get_record_pos_with_index() {{{
 * Locates a database record by using the primary index.
 * The index is used by adding the number of records per block
 * until the block is found where the record with the given number
 * is stored. The function still disregards any sorting within the
 * index. The record number is not an absolut value. Accessing a
 * database file with and without the index may result in different
 * record numbers for the same record.
 * Returns 1 if record could be found, otherwise 0
 */
int
px_get_record_pos_with_index(pxdoc_t *pxdoc, int recno, int *deleted, pxdatablockinfo_t *pxdbinfo) {
	int j, numrecords, n, recsperdatablock;
//	pxdoc_t *pindexdoc;
	pxhead_t *pxh; //, *pxih;
	pxpindex_t *pindex_data;

	pxh = pxdoc->px_head;
//	pindexdoc = pxdoc->px_pindex;
//	pxih = pindexdoc->px_head;
//	pindex_data = pindexdoc->px_data;
	pindex_data = pxdoc->px_indexdata;

	if(!pindex_data) {
		px_error(pxdoc, PX_RuntimeError, _("Cannot search for free slot in block without an index."));
		return 0;
	}

	numrecords = 0 ;
	recsperdatablock = (pxh->px_maxtablesize*0x400-sizeof(TDataBlock))/pxh->px_recordsize;
	for(j=0; j<pxdoc->px_indexdatalen; j++) {
		/* We currently just take level 1 index blocks into account.
		 * This is only for large databases a speed disadvantage.
		 */
		if(pindex_data[j].level == 1) {
			n = pindex_data[j].numrecords;
			numrecords += n;
			if(recno >= n) {
				recno -= n;
			} else {
				int blocksize, ret;
				TDataBlock datablock;

				pxdbinfo->number = pindex_data[j].blocknumber;
				pxdbinfo->recno = recno;
				pxdbinfo->blockpos = pxh->px_headersize + (pxdbinfo->number-1)*pxh->px_maxtablesize*0x400;
				pxdbinfo->recordpos = pxdbinfo->blockpos + sizeof(TDataBlock) + recno*pxh->px_recordsize;

				/* Go to the start of the data block (skip the header) */
				if((ret = pxdoc->seek(pxdoc, pxdoc->px_stream, pxdbinfo->blockpos, SEEK_SET)) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not fseek start of first data block."));
					return 0;
				}

				/* Get the info about this data block */
				if((ret = pxdoc->read(pxdoc, pxdoc->px_stream, sizeof(TDataBlock), &datablock)) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not read datablock header."));
					return 0;
				}

				blocksize = get_short_le((char *) &datablock.addDataSize);

				pxdbinfo->prev = get_short_le((char *) &datablock.prevBlock);
				pxdbinfo->next = get_short_le((char *) &datablock.nextBlock);
				pxdbinfo->size = blocksize+pxh->px_recordsize;
				pxdbinfo->numrecords = pxdbinfo->size/pxh->px_recordsize;
				deleted = 0;
				return 1;
			}
		}
	}
	return 0;
}
/* }}} */

/* px_get_record_pos() {{{
 * Reads all data blocks until the requested recno is in the block.
 * This function doesn't use a primary index and is therefore far
 * from being efficient for large files.
 * Returns 1 if record could be found, otherwise 0
 */
int
px_get_record_pos(pxdoc_t *pxdoc, int recno, int *deleted, pxdatablockinfo_t *pxdbinfo) {
	int found, blockcount, blocknumber;
	TDataBlock datablock;
	pxhead_t *pxh;

	pxh = pxdoc->px_head;

	found = 0;
	blockcount = 0; /* Just a block counter */
	blocknumber = pxh->px_firstblock; /* Will be set to next block number */
	while(!found && (blockcount < pxh->px_fileblocks) && (blocknumber > 0)) {
		int datasize, blocksize;

		if(get_datablock_head(pxdoc, pxdoc->px_stream, blocknumber, &datablock) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not get head of data block nr. %d."), blocknumber);
			return 0;
		}

		/* if deleted is set, then we will disregard the block size in the
		 * data block header but take the maximum block size as indicated
		 * by pxh->px_maxtablesize. The variable blocksize is just to test
		 * whether a record is valid or not.
		 * If a block is not completely used the blocksize will be less than
		 * the theortical size of a block. If a block is not used at all
		 * its blocksize is usually much bigger than the maximal data block
		 * size. In the second case we set it -1.
		 */
		blocksize = get_short_le((char *) &datablock.addDataSize);
		if(!*deleted)
			datasize = blocksize; //get_short_le((char *) &datablock.addDataSize);
		else
			datasize = pxh->px_maxtablesize*0x400-sizeof(TDataBlock)-pxh->px_recordsize;
		if(blocksize > pxh->px_maxtablesize*0x400-(int)sizeof(TDataBlock)-pxh->px_recordsize) {
			/* setting blocksize to -1 means that this block contains no valid
			 * records. All records are deleted. The -1 is later used to set
			 * 'deleted' on the proper value.
			 */
			blocksize = -1;
		}

//		printf("datasize = %d, recno = %d, platz verbraucht = %d\n", datasize, recno, (recno+1)*pxh->px_recordsize);
		/* addDataSize is the number of bytes in this data block. It must
		 * be less then
		 * 'pxh->px_maxtablesize*0x400-sizeof(TDataBlock)-pxh->px_recordsize'
		 * and a multiple of pxh->recordsize. If this is not the case
		 * (especially if addDataSize is to big, then this data block
		 * does not contain any valid records. Actually you could read
		 * them, because the data is still there, but considered to be
		 * deleted.
		 */
		if ((datasize+pxh->px_recordsize) > (pxh->px_maxtablesize*0x400-(int)sizeof(TDataBlock))) {
//			printf("Size of data block %d as set in its header is to large: %d (%3.2f records)\n", get_short_le(&datablock.prevBlock), datasize, (float) datasize/pxh->px_recordsize + 1);
			/* Set the number of the next block */
			blocknumber = get_short_le((char *) &datablock.nextBlock);
		} else {
			if(recno*pxh->px_recordsize <= datasize) {
				found = 1;
				/* if we are within the range of valid data in the block,
				 * then set the deleted flag to 0
				 */
				if(recno*pxh->px_recordsize <= blocksize) {
					*deleted = 0;
				}
				if(pxdbinfo != NULL) {
					pxdbinfo->prev = get_short_le((char *) &datablock.prevBlock);
					pxdbinfo->next = get_short_le((char *) &datablock.nextBlock);
					pxdbinfo->number = blocknumber;
					pxdbinfo->size = datasize+pxh->px_recordsize;
					pxdbinfo->recno = recno;
					pxdbinfo->numrecords = pxdbinfo->size/pxh->px_recordsize;
					pxdbinfo->blockpos = pxdoc->tell(pxdoc, pxdoc->px_stream)-sizeof(TDataBlock);
					pxdbinfo->recordpos = pxdbinfo->blockpos + sizeof(TDataBlock) + recno*pxh->px_recordsize;
				}
			} else { /* skip rest of block */
				blocknumber = get_short_le((char *) &datablock.nextBlock);
			}
			recno -= (datasize/pxh->px_recordsize+1);
		}
		blockcount++;
	}
	return(found);
}
/* }}} */

/* px_find_slot_with_index() {{{
 * Searches for a free slot for a record by using the primary index.
 * Blocks are search for a free slot from the beginning to the end
 * of the file
 * Returns the record number if a free slot could be found, 0 if
 * none could be found * and -1 in case of an error. In the second
 * case the calling function has to create a new data block.
 * The record number starts at 1 because 0 is ambigous.
 */
int
px_find_slot_with_index(pxdoc_t *pxdoc, pxdatablockinfo_t *pxdbinfo) {
	int j, recsperdatablock;
	int reccount=0;
	pxhead_t *pxh; //, *pxih;
	pxpindex_t *pindex_data;

	pxh = pxdoc->px_head;
	pindex_data = pxdoc->px_indexdata;

	if(!pindex_data) {
		px_error(pxdoc, PX_RuntimeError, _("Cannot search for free slot in block without an index."));
		return -1;
	}

	recsperdatablock = (pxh->px_maxtablesize*0x400-sizeof(TDataBlock))/pxh->px_recordsize;
	for(j=0; j<pxdoc->px_indexdatalen; j++) {
		/* We currently just take level 1 index blocks into account.
		 * This is only for large databases a speed disadvantage.
		 */
		if(pindex_data[j].level == 1) {
			/* Is there a free slot in the block?j */
			if(pindex_data[j].numrecords < recsperdatablock) {
				int blocksize, ret;
				TDataBlock datablock;

				pxdbinfo->number = pindex_data[j].blocknumber;
				pxdbinfo->recno = pindex_data[j].numrecords;
				pxdbinfo->blockpos = pxh->px_headersize + (pxdbinfo->number-1)*pxh->px_maxtablesize*0x400;
				pxdbinfo->recordpos = pxdbinfo->blockpos + sizeof(TDataBlock) + pxdbinfo->recno*pxh->px_recordsize;

				/* Go to the start of the data block (skip the header) */
				if((ret = pxdoc->seek(pxdoc, pxdoc->px_stream, pxdbinfo->blockpos, SEEK_SET)) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not fseek start of first data block."));
					return -1;
				}

				/* Get the info about this data block */
				if((ret = pxdoc->read(pxdoc, pxdoc->px_stream, sizeof(TDataBlock), &datablock)) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not read datablock header."));
					return -1;
				}

				blocksize = get_short_le((char *) &datablock.addDataSize);

				pxdbinfo->prev = get_short_le((char *) &datablock.prevBlock);
				pxdbinfo->next = get_short_le((char *) &datablock.nextBlock);
				pxdbinfo->size = blocksize+pxh->px_recordsize;
				pxdbinfo->numrecords = pxdbinfo->size/pxh->px_recordsize;
				if(pindex_data[j].numrecords != pxdbinfo->numrecords) {
					px_error(pxdoc, PX_RuntimeError, _("Number of records of block stored in index (%d) is unequal to number of records stored in block header (%d)."), pindex_data[j].numrecords, pxdbinfo->numrecords);
					return -1;
				}
				return reccount+pindex_data[j].numrecords+1;
			} else {
				/* Just count the number of records found so far. It doesn't
				 * make a difference if we add recsperdatablock or
				 * pindex_data[j].numrecords because they equal anyway.
				 */
				reccount += recsperdatablock;
			}
		}
	}
	return 0;
}
/* }}} */

/* px_find_slot() {{{
 * Reads all data blocks until a block with a free slot is found.
 * This function doesn't use a primary index and is therefore far
 * from being efficient for large files.
 * Returns 1 if free slot could be found, otherwise 0, and -1
 * in case of error.
 */
int
px_find_slot(pxdoc_t *pxdoc, pxdatablockinfo_t *pxdbinfo) {
	int found, blockcount, blocknumber;
	TDataBlock datablock;
	pxhead_t *pxh;

	pxh = pxdoc->px_head;

	found = 0;
	blockcount = 0; /* Just a block counter */
	blocknumber = pxh->px_firstblock; /* Will be set to next block number */
	while(!found && (blockcount < pxh->px_fileblocks) && (blocknumber > 0)) {
		int datasize, blocksize;

		if(get_datablock_head(pxdoc, pxdoc->px_stream, blocknumber, &datablock) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not get head of data block nr. %d."), blocknumber);
			return -1;
		}

		/* if deleted is set, then we will disregard the block size in the
		 * data block header but take the maximum block size as indicated
		 * by pxh->px_maxtablesize. The variable blocksize is just to test
		 * whether a record is valid or not.
		 * If a block is not completely used the blocksize will be less than
		 * the theortical size of a block. If a block is not used at all
		 * its blocksize is usually much bigger than the maximal data block
		 * size. In the second case we set it -1.
		 */
		blocksize = get_short_le((char *) &datablock.addDataSize);
		datasize = blocksize; //get_short_le((char *) &datablock.addDataSize);
		if ((datasize+pxh->px_recordsize) < (pxh->px_maxtablesize*0x400-(int)sizeof(TDataBlock))) {
			found = 1;
			/* if we are within the range of valid data in the block,
			 * then set the deleted flag to 0
			 */
			if(pxdbinfo != NULL) {
				pxdbinfo->prev = get_short_le((char *) &datablock.prevBlock);
				pxdbinfo->next = get_short_le((char *) &datablock.nextBlock);
				pxdbinfo->number = blocknumber;
				pxdbinfo->size = datasize+pxh->px_recordsize;
				pxdbinfo->recno = pxdbinfo->size/pxh->px_recordsize;
				pxdbinfo->numrecords = pxdbinfo->size/pxh->px_recordsize;
				pxdbinfo->blockpos = pxdoc->tell(pxdoc, pxdoc->px_stream)-sizeof(TDataBlock);
				pxdbinfo->recordpos = pxdbinfo->blockpos + sizeof(TDataBlock) + pxdbinfo->recno*pxh->px_recordsize;
			}
		}
		blockcount++;
	}
	return(found);
}
/* }}} */

/* px_list_index() {{{
 * Reads all data blocks until a block with a free slot is found.
 * This function doesn't use a primary index and is therefore far
 * from being efficient for large files.
 * Returns 1 if free slot could be found, otherwise 0, and -1
 * in case of error.
 */
void
px_list_index(pxdoc_t *pxdoc) {
	pxpindex_t *pindex;
	int i;
	pindex = pxdoc->px_indexdata;
	fprintf(stdout, "    | blocknr | numrecs \n");
	fprintf(stdout, "------------------------\n");
	for(i=0; i<pxdoc->px_indexdatalen; i++) {
		fprintf(stdout, "%3d | %7d | %7d\n", i, pindex[i].blocknumber, pindex[i].numrecords);
	}
}
/* }}} */

/* px_find_blob_slot() {{{
 * Try to find block(s) in mb file which has/have enough space for the blob
 * blockinfo returns the pointer to the info block within the block info list
 * of the blob file or NULL.
 * Returns 1 if free slot could be found, otherwise 0, and -1
 * in case of error.
 */
int
px_find_blob_slot(pxblob_t *pxblob, int blobsize, pxmbblockinfo_t **blockinfo) {
	int numblocks, blockcount;
	char blocktype;

	if(pxblob->blocklist == NULL) {
		return -1;
	}

	if(blobsize > 2048) {
		blocktype = 2;
		numblocks = ((blobsize+sizeof(TMbBlockHeader2)-1) / 4096) + 1;
	} else {
		blocktype = 3;
		numblocks = ((blobsize-1) / 16) + 1;
	}

	blockcount = 0;
	while(blockcount < pxblob->blocklistlen) {
		/* Check for complete free blocks */
		if(blocktype == 2 && pxblob->blocklist[blockcount].type == 4) {
			/* We have found one free block but we need numblocks.
			 * Try to find more blocks if needed.
			 */
			int i = 1;
			while((pxblob->blocklist[blockcount+i].type == 4) &&
			      ((blockcount+i) < pxblob->blocklistlen) && 
				  (i < numblocks)) {
				i++;
			}
			if(i == numblocks) {
				*blockinfo = &pxblob->blocklist[blockcount];
				return 1;
			}
			blockcount += i;
		} else
		/* First check for exiting type 3 blocks with some free space */
		if(blocktype == 3 && pxblob->blocklist[blockcount].type == 3) {
			if((pxblob->blocklist[blockcount].numblobs < 64) &&
			   (pxblob->blocklist[blockcount].allocspace <= (235 - numblocks))) {
				*blockinfo = &pxblob->blocklist[blockcount];
				return 1;
			}
		}
		blockcount++;
	}

	/* Last change for blobs < 2048 byte. We haven't found an exiting block
	 * of type 3, maybe we find a completely free block which we can turn into
	 * a type 3 block. Still better than adding a new one at the end.
	 */
	if(blocktype == 3) {
		blockcount = 0;
		while(blockcount < pxblob->blocklistlen) {
			if(pxblob->blocklist[blockcount].type == 4) {
				*blockinfo = &pxblob->blocklist[blockcount];
				return 1;
			}
			blockcount++;
		}
	}

	return(0);
}
/* }}} */

/* px_convert_data() {{{
 * Takes a list of pointers, pointing towards the data of each
 * field and creates a record, which can be stored in the database
 * file.
 * Notice: This function modifies the header of the paradox database
 * if a field of type pxfAutoInc is affected and its value is NULL.
 *
 * Returns 0 on success or and -1 * in case of error.
 */
char *
px_convert_data(pxdoc_t *pxdoc, pxval_t **dataptr) {
	int numfields;
	pxhead_t *pxh;
	pxfield_t *pxf;
	int i, offset;
	char *data;

	pxh = pxdoc->px_head;

	if(NULL == (data = pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Allocate memory for data record.")))) {
		return NULL;
	}
	/* Initialize to 0, so null values are the default */
	memset(data, 0, pxh->px_recordsize);

	numfields = pxh->px_numfields;
	pxf = pxh->px_fields;
	offset = 0;
	for(i=0; i<numfields; i++) {
		/* No need to store a null value, unless a pxfAutoInc field */
		if(!dataptr[i]->isnull || pxf->px_ftype == pxfAutoInc) {
			switch(pxf->px_ftype) {
				case pxfAlpha:
					if(dataptr[i]->value.str.len > pxf->px_flen) {
						pxdoc->free(pxdoc, data);
						return NULL;
					}
					PX_put_data_alpha(pxdoc, &data[offset], pxf->px_flen, (char *) dataptr[i]->value.str.val);
					break;
				case pxfShort:
					PX_put_data_short(pxdoc, &data[offset], 2, (short int) dataptr[i]->value.lval);
					break;
				case pxfAutoInc:
					if(dataptr[i]->isnull) {
						pxh->px_autoinc++;
						PX_put_data_long(pxdoc, &data[offset], 4, (int) pxh->px_autoinc);
					} else {
						PX_put_data_long(pxdoc, &data[offset], 4, (int) dataptr[i]->value.lval);
					}
					break;
				case pxfLong:
				case pxfTime:
				case pxfDate:
					PX_put_data_long(pxdoc, &data[offset], 4, (int) dataptr[i]->value.lval);
					break;
				case pxfTimestamp:
				case pxfCurrency:
				case pxfNumber:
					PX_put_data_double(pxdoc, &data[offset], 8, dataptr[i]->value.dval);
					break;
				case pxfLogical: {
					char value;
					value = (char) dataptr[i]->value.lval;
					PX_put_data_byte(pxdoc, &data[offset], 1, value);
					break;
				}
				case pxfGraphic:
					break;
				case pxfBLOb:
					break;
				case pxfOLE:
					break;
				case pxfFmtMemoBLOb:
				case pxfMemoBLOb: {
					if(0 > PX_put_data_blob(pxdoc, &data[offset], pxf->px_flen, dataptr[i]->value.str.val, dataptr[i]->value.str.len)) {
						pxdoc->free(pxdoc, data);
						return NULL;
					}
					break;
				}
				case pxfBytes:
					PX_put_data_bytes(pxdoc, &data[offset], min(pxf->px_flen, dataptr[i]->value.str.len), dataptr[i]->value.str.val);
					break;
				case pxfBCD:
					PX_put_data_bcd(pxdoc, &data[offset], pxf->px_flen, dataptr[i]->value.str.val);
					break;
			}
		}
		offset += pxf->px_flen;
		pxf++;
	}
	return(data);
}
/* }}} */

/* PX_get_record() {{{
 * Reads one record from a Paradox file. This function can be used
 * for different types of Paradox files. This function will not
 * return any information about the datablock in which the record
 * was stored.
 */
PXLIB_API char* PXLIB_CALL
PX_get_record(pxdoc_t *pxdoc, int recno, char *data) {
	int d = 0;
	return(PX_get_record2(pxdoc, recno, data, &d, NULL));
}
/* }}} */

/* PX_get_record2() {{{
 * Reads one record from a Paradox file. This function can be used
 * for different types of Paradox files. The function will return
 * information about the datablock where the record is stored.
 * It also reports if a record is deleted. Read the man page of
 * this function to get an explanation on what it means if a record
 * is deleted.
 */
PXLIB_API char* PXLIB_CALL
PX_get_record2(pxdoc_t *pxdoc, int recno, char *data, int *deleted, pxdatablockinfo_t *pxdbinfo) {
	int ret, found;
	pxhead_t *pxh;
	pxdatablockinfo_t tmppxdbinfo;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return NULL;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return NULL;
	}
	pxh = pxdoc->px_head;

	/* Allow to read records up to the theoretical number of records
	 * in the file or the actual number of records depending on 'deleted'.
	 * If a primary index exists do not care about 'deleted' and read
	 * in any case only up to the actual number of records.
	 */
	if((recno < 0) ||
	   (*deleted && (recno >= pxh->px_theonumrecords)) ||
	   (pxdoc->px_pindex && (recno >= pxh->px_numrecords)) ||
	   (!*deleted && (recno >= pxh->px_numrecords))) {
		px_error(pxdoc, PX_RuntimeError, _("Record number out of range."));
		return NULL;
	}

	if(pxdoc->px_indexdata)
		found = px_get_record_pos_with_index(pxdoc, recno, deleted, &tmppxdbinfo);
	else
		found = px_get_record_pos(pxdoc, recno, deleted, &tmppxdbinfo);

	if(found) {
		if(pxdbinfo) {
			memcpy(pxdbinfo, &tmppxdbinfo, sizeof(pxdatablockinfo_t));
		}

		if((ret = pxdoc->seek(pxdoc, pxdoc->px_stream, tmppxdbinfo.recordpos, SEEK_SET)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not fseek start of record data."));
			return NULL;
		}
		if((ret = pxdoc->read(pxdoc, pxdoc->px_stream, pxh->px_recordsize, data)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not read data of record."));
			return NULL;
		}
		return data;
	} else {
		px_error(pxdoc, PX_RuntimeError, _("Could not find record in database."));
		return NULL;
	}
}
/* }}} */

/* PX_put_recordn() {{{
 * Store a record into the paradox file. The record can be saved at
 * any position. If the position is beyond the last datablock, then
 * new datablocks will be added until the position lies in a
 * datablock. You may use this function to end a datablock and
 * start a new one. If the position is in the middle of a data block
 * without any records before this position, the position will be
 * recalculated and the record is placed after the last record in
 * that datablock.
 * Returns the next postion or -1 in case of an error.
 */
PXLIB_API int PXLIB_CALL
PX_put_recordn(pxdoc_t *pxdoc, char *data, int recpos) {
	pxhead_t *pxh;
	int recsperdatablock, datablocknr, recdatablocknr;
	int itmp;
	int update; /* Will be set by px_add_data_to_block() if an existing
				   record is updated */

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return -1;
	}
	pxh = pxdoc->px_head;

//	fprintf(stderr, "Putting record at position %d\n", recpos);

	/* All the following calculation assume sequentially writing of
	 * records and filling a datablock first before starting a new one.
	 * This should be fixed. Better would be, if we keep record
	 * in px_data (like a primary index) which datablocks are available
	 * an how much space they still have.
	 */
	/* Check if record still fits into a existing data block */
	recsperdatablock = (pxh->px_maxtablesize*0x400-sizeof(TDataBlock)) / pxh->px_recordsize;
	/* Calculate the number of the data block for this record.
	 * Datablock numbers start at 1. */
	datablocknr = (recpos / recsperdatablock) + 1;
	/* Calculate the position within the datablock */
	recdatablocknr = recpos % recsperdatablock;

//	fprintf(stderr, "Data goes into block %d at record no %d (%d)\n", datablocknr, recdatablocknr, recsperdatablock);
	/* add as many datablocks as require to store the record at the
	 * desired position. */
	itmp = datablocknr;
	while(datablocknr > pxh->px_fileblocks) {
//		fprintf(stderr, "We need an new datablock\n");
		itmp = put_px_datablock(pxdoc, pxh, pxh->px_lastblock, pxdoc->px_stream);
		if(itmp < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not write new data block."));
			return -1;
		}
//		fprintf(stderr, "Added data block no. %d\n", itmp);
	}
	/* The datablock number return by px_put_datablock() should be
	 * the same as the calculated datablocknr after all datablocks
	 * has been added.
	 */
	if(datablocknr != itmp) {
		px_error(pxdoc, PX_RuntimeError, _("Inconsistency in writing data block. Expected data block nr. %d, but got %d."), datablocknr, itmp);
		return -1;
	}

	/* write data */
	itmp = px_add_data_to_block(pxdoc, pxh, datablocknr, recdatablocknr, data, pxdoc->px_stream, &update);

	/* The record number within the data block must be the same
	 * as the calculated one.
	 */
	if(itmp < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Inconsistency in writing record into data block. Expected record nr. %d, but got %d. %dth record. %dth data block. %d records per block."), recdatablocknr, itmp, pxh->px_numrecords+1, datablocknr, recsperdatablock);
		return -1;
	}
	
	if(itmp != recdatablocknr) {
		px_error(pxdoc, PX_Warning, _("Position of record has been recalculated. Requested position was %d, new position is %d."), recpos, (datablocknr-1) * recsperdatablock + itmp);
	}

	/* Update header:
	 * Modify the last_postion only if a record was added at the end.
	 * increment the record count only if a record was added not updated */
	if(recpos >= pxh->px_numrecords) {
		pxdoc->last_position = (datablocknr-1) * recsperdatablock + itmp;
	}
	if(update == 0) {
		pxh->px_numrecords++;
	}

	put_px_head(pxdoc, pxh, pxdoc->px_stream);
	return(pxdoc->last_position+1);
}
/* }}} */

/* PX_put_record() {{{
 * Stores a record into the paradox file. It uses the next free
 * slot in the database.
 * Returns the next postion (recpos+1) or -1 in case of an error.
 */
PXLIB_API int PXLIB_CALL
PX_put_record(pxdoc_t *pxdoc, char *data) {
	return(PX_put_recordn(pxdoc, data, pxdoc->last_position+1));
}
/* }}} */

/* PX_retrieve_record() {{{
 * Get a record from the paradox file.
 * Returns an array of *pxval_t or NULL in case of an error.
 */
PXLIB_API pxval_t ** PXLIB_CALL
PX_retrieve_record(pxdoc_t *pxdoc, int recno) {
	pxhead_t *pxh;
	char *data;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return NULL;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return NULL;
	}
	pxh = pxdoc->px_head;

	/* Allocate memory for record */
	if((data = (char *) pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Allocate memory for temporary record."))) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Could not allocate memory for temporary record."));
		return NULL;
	}

	if(NULL != PX_get_record(pxdoc, recno, data)) {
		int i, offset;
		pxval_t **dataptr;
		pxfield_t *pxf;

		/* Allocate memory for return record */
		if(NULL == (dataptr = (pxval_t **) pxdoc->malloc(pxdoc, pxh->px_numfields*sizeof(pxval_t *), _("Allocate memory for array of pointers to field values.")))) {
			px_error(pxdoc, PX_RuntimeError, _("Could not allocate memory for array of pointers to field values."));
			pxdoc->free(pxdoc, data);
			return NULL;
		}
		pxf = PX_get_fields(pxdoc);
		offset = 0;
		for(i=0; i<PX_get_num_fields(pxdoc); i++) {
			MAKE_PXVAL(pxdoc, dataptr[i]);
			dataptr[i]->type = pxf->px_ftype;
			switch(pxf->px_ftype) {
				case pxfAlpha: {
					char *value;
					int ret;
					if(0 < (ret = PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value))) {
						dataptr[i]->value.str.val = value;
						dataptr[i]->value.str.len = strlen(value);
					} else if(ret < 0) {
						dataptr[i]->isnull = 1;
						px_error(pxdoc, PX_RuntimeError, _("Could not read of field of type pxfAlpha."));
					} else {
						dataptr[i]->isnull = 1;
					}
					break;
				}
				case pxfShort: {
					short int value;
					if(0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value)) {
						dataptr[i]->value.lval = (long) value;
					} else {
						dataptr[i]->isnull = 1;
					}
					break;
					}
				case pxfDate:
				case pxfTime:
				case pxfAutoInc:
				case pxfLong: {
					long value;
					if(0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
						dataptr[i]->value.lval = value;
					} else {
						dataptr[i]->isnull = 1;
					}
					break;
					}
				case pxfTimestamp:
				case pxfCurrency:
				case pxfNumber: {
					double value;
					if(0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
						dataptr[i]->value.dval = value;
					} 
					break;
					} 
				case pxfLogical: {
					char value;
					if(0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_flen, &value)) {
						dataptr[i]->value.lval = (long) value;
					} else {
						dataptr[i]->isnull = 1;
					}
					break;
					}
				case pxfGraphic:
				case pxfBLOb:
				case pxfFmtMemoBLOb:
				case pxfMemoBLOb:
				case pxfOLE: {
					char *blobdata;
					int mod_nr, size, ret;
					if(pxf->px_ftype == pxfGraphic)
						ret = PX_get_data_graphic(pxdoc, &data[offset], pxf->px_flen, &mod_nr, &size, &blobdata);
					else
						ret = PX_get_data_blob(pxdoc, &data[offset], pxf->px_flen, &mod_nr, &size, &blobdata);
					if(ret > 0) {
						if(blobdata) {
							dataptr[i]->value.str.val = blobdata;
							dataptr[i]->value.str.len = size;
						} else {
							dataptr[i]->isnull = 1;
							px_error(pxdoc, PX_RuntimeError, _("Could not read blob data."));
						}
					} else if(ret == 0) {
						dataptr[i]->isnull = 1;
					} else {
						px_error(pxdoc, PX_RuntimeError, _("Could not read blob data."));
					}

					break;
				}
				case pxfBytes: {
					char *value;
					if(0 < PX_get_data_bytes(pxdoc, &data[offset], pxf->px_flen, &value)) {
						dataptr[i]->value.str.val = value;
						dataptr[i]->value.str.len = pxf->px_flen;
					} else {
						dataptr[i]->isnull = 1;
					}
					break;
					}
				case pxfBCD: {
					char *value;
					if(0 < PX_get_data_bcd(pxdoc, (unsigned char*) &data[offset], pxf->px_fdc, &value)) {
						dataptr[i]->value.str.val = value;
						dataptr[i]->value.str.len = strlen(value);
					} else {
						dataptr[i]->isnull = 1;
					}
					break;
				}
				default:
					dataptr[i]->isnull = 1;
					break;
			}
			offset += pxf->px_flen;
			pxf++;
		}
/*
		if(filetype == pxfFileTypPrimIndex) {
			short int value;
			if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
				fprintf(outfp, "%d", value);
			}
			offset += 2;
			if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
				fprintf(outfp, "%d", value);
				ireccounter += value;
			}
			offset += 2;
			if(0 < PX_get_data_short(pxdoc, &data[offset], 2, &value)) {
				fprintf(outfp, "%d", value);
			}
			fprintf(outfp, "%d", pxdbinfo.number);
		}
		if(markdeleted) {
			fprintf(outfp, "%d", isdeleted);
		}
*/
		pxdoc->free(pxdoc, data);
		return(dataptr);
	} else {
		px_error(pxdoc, PX_RuntimeError, _("Could not read data for record with number %d."), recno);
		pxdoc->free(pxdoc, data);
		return NULL;
	}
}
/* }}} */

/* PX_insert_record() {{{
 * Add a record to the paradox file. The record is saved in the first
 * free position found in the database. This doesn't have to be in
 * the last block. If records has been deleted before, this function
 * will try to reuse the space first.
 * Returns the record number starting from 0 or -1 in case of an error.
 */
PXLIB_API int PXLIB_CALL
PX_insert_record(pxdoc_t *pxdoc, pxval_t **dataptr) {
	pxhead_t *pxh;
	pxdatablockinfo_t tmppxdbinfo;
	char *data;
	int datablocknr;
	int itmp, found, recno, newrecpos;
	int update; /* Will be set by px_add_data_to_block() if an existing
				   record is updated */

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return -1;
	}
	pxh = pxdoc->px_head;

	/* Check for a free record in the exiting file */
	if(pxdoc->px_indexdata)
		found = px_find_slot_with_index(pxdoc, &tmppxdbinfo);
	else
		found = px_find_slot(pxdoc, &tmppxdbinfo);

	if(found < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Error while searching for free slot of new record."));
		return -1;
	}

	/* If not free space for the new record was found, then create a new block */
	if(found == 0) {
		pxpindex_t *pindex;

		datablocknr = put_px_datablock(pxdoc, pxh, pxh->px_lastblock, pxdoc->px_stream);
		if(datablocknr < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not write new data block."));
			return -1;
		}
		recno = 0;
		/* Rebuild the index */
		/* Allocate memory for internal list of index entries */
		if(NULL == (pindex = pxdoc->malloc(pxdoc, pxh->px_fileblocks*sizeof(pxpindex_t), _("Allocate memory for self build internal primary index.")))) {
			px_error(pxdoc, PX_MemoryError, _("Could not allocate memory for self build internal index."));
			return -1;
		}

		if(pxdoc->px_indexdata) {
			memcpy(pindex, pxdoc->px_indexdata, pxdoc->px_indexdatalen*sizeof(pxpindex_t));
			pxdoc->free(pxdoc, pxdoc->px_indexdata);
		}

		pxdoc->px_indexdata = pindex;

		pindex[pxdoc->px_indexdatalen].data = NULL;
		pindex[pxdoc->px_indexdatalen].blocknumber = datablocknr;
		pindex[pxdoc->px_indexdatalen].numrecords = 1;
		pindex[pxdoc->px_indexdatalen].myblocknumber = 0;
		pindex[pxdoc->px_indexdatalen].level = 1;
		pxdoc->px_indexdatalen++;
		newrecpos = pxh->px_numrecords;
	} else {
		pxpindex_t *pindex;
		pindex = pxdoc->px_indexdata;
		datablocknr = tmppxdbinfo.number;
		pindex[datablocknr-1].numrecords++;
		recno = tmppxdbinfo.recno;
		newrecpos = found-1;
	}
	/* The datablock number return by px_put_datablock() should be
	 * the same as the calculated datablocknr after all datablocks
	 * has been added.
	 */

	/* write data */
	data = px_convert_data(pxdoc, dataptr);
	itmp = px_add_data_to_block(pxdoc, pxh, datablocknr, recno, data, pxdoc->px_stream, &update);
	pxdoc->free(pxdoc, data);
	if(update == 1) {
		px_error(pxdoc, PX_RuntimeError, _("Request for inserting a new record turned out to be an update of an exiting record. This should not happen."));
		return -1;
	}

	/* The record number within the data block must be the same
	 * as the calculated one.
	 */
	if(itmp < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Error in writing record into data block."));
		return -1;
	}
	
	pxh->px_numrecords++;
	put_px_head(pxdoc, pxh, pxdoc->px_stream);
	return(newrecpos);
}
/* }}} */

/* px_delete_blobs() {{{
 */
int px_delete_blobs(pxdoc_t *pxdoc, int recordpos) {
	int i, offset;
	int leader;
	pxfield_t *pxf;
	pxhead_t *pxh;
	pxstream_t *pxs;
	pxblob_t *pxblob;
	char *recorddata = NULL;

	pxh = pxdoc->px_head;
	pxs = pxdoc->px_stream;
	pxblob = pxdoc->px_blob;

	/* Check if database has blob fields */
	offset = 0;
	pxf = pxh->px_fields;
	for(i=0; i<pxh->px_numfields; i++) {
		char *data;
		int hsize, size, blobsize, index, mod_nr, bloboffset;

		if(pxf[i].px_ftype == pxfMemoBLOb ||
		   pxf[i].px_ftype == pxfFmtMemoBLOb ||
		   pxf[i].px_ftype == pxfBLOb ||
		   pxf[i].px_ftype == pxfOLE ||
		   pxf[i].px_ftype == pxfGraphic) {

			if(pxf[i].px_ftype == pxfGraphic)
				hsize = 17;
			else
				hsize =9;

			if(NULL == recorddata) {
				if(NULL == (recorddata = pxdoc->malloc(pxdoc, pxh->px_recordsize, _("Allocate memory for temporary record data.")))) {
					px_error(pxdoc, PX_RuntimeError, _("Could not allocate memory for temporary record data.."));
					return -1;
				}
				if(pxdoc->seek(pxdoc, pxs, recordpos, SEEK_SET) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not fseek to start of old record."));
					pxdoc->free(pxdoc, recorddata);
					return -1;
				}

				/* Read the record data */
				if(pxdoc->read(pxdoc, pxs, pxh->px_recordsize, recorddata) < 1) {
					px_error(pxdoc, PX_RuntimeError, _("Could not read record."));
					pxdoc->free(pxdoc, recorddata);
					return -1;
				}
			}

			/* Let data point to the field data */
			data = &recorddata[offset];
			/* leader starts 10 bytes from the end of the field data */
			leader = pxf[i].px_flen - 10;

			/* FIXME: This is a quick hack because graphic blobs have some extra
			 * 8 Bytes before the data which is contained in the size
			 * The real size of the graphic is stored in the second long within
			 * the extra 8 bytes. But this value seems to be alwasy 8 smaller
			 * then the size at [leader+4].
			 */
			size = get_long_le(&data[leader+4]);
			if(hsize == 17)
				blobsize = size - 8;
			else
				blobsize = size;
			index = get_long_le(&data[leader]) & 0x000000ff;
			mod_nr = get_short_le(&data[leader+8]);

			if(blobsize <= 0) {
				continue;
			}

			/* First check if the blob data is included in the record itself */
			if(blobsize <= leader) {
				continue;
			} 

			/* Since the blob data is not in the record we will need a blob file */
			if(!pxblob || !pxblob->mb_stream) {
				px_error(pxdoc, PX_Warning, _("Blob data is not contained in record and a blob file is not set."));
				continue;
			}

			bloboffset = get_long_le(&data[leader]) & 0xffffff00;
			if(bloboffset == 0) {
				continue;
			}

			if(px_delete_blob_data(pxblob, hsize, size, bloboffset, index) > 0) {
				px_error(pxdoc, PX_RuntimeError, _("Deleting blob failed."));
				pxdoc->free(pxdoc, recorddata);
				return -1;
			}

		}
		offset += pxf[i].px_flen;
	}
	if(recorddata)
		pxdoc->free(pxdoc, recorddata);

	return 0;
}
/* }}} */

/* PX_update_record() {{{
 * Updates a record in the paradox file. The record number must
 * be between 0 and numrecords-1.
 * Returns -1 in case of an error.
 */
PXLIB_API int PXLIB_CALL
PX_update_record(pxdoc_t *pxdoc, pxval_t **dataptr, int recno) {
	pxhead_t *pxh;
	pxdatablockinfo_t tmppxdbinfo;
	char *data;
	int found;
	int deleted = 0;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return -1;
	}
	pxh = pxdoc->px_head;

	if((recno < 0) || (recno >= pxh->px_numrecords)) {
		px_error(pxdoc, PX_RuntimeError, _("Record number out of range."));
		return -1;
	}

	if(pxdoc->px_indexdata)
		found = px_get_record_pos_with_index(pxdoc, recno, &deleted, &tmppxdbinfo);
	else
		found = px_get_record_pos(pxdoc, recno, &deleted, &tmppxdbinfo);

	if(found) {
		int datablocknr;
		int ret, update;

		/* Delete all blobs associated with this record */
		if(px_delete_blobs(pxdoc, tmppxdbinfo.recordpos) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not delete blobs of record."));
			return -1;
		}

		/* We need to calculate the physical block number. tmppxdbinfo.number
		 * contains the logical number of the block as stored in the header of
		 * each block.
		 */
		datablocknr = ((tmppxdbinfo.blockpos - pxh->px_headersize) / (pxh->px_maxtablesize*0x400)) + 1;
		data = px_convert_data(pxdoc, dataptr);
		ret = px_add_data_to_block(pxdoc, pxh, datablocknr, tmppxdbinfo.recno, data, pxdoc->px_stream, &update);
		pxdoc->free(pxdoc, data);
		if(update != 1) {
			px_error(pxdoc, PX_RuntimeError, _("Expected record to be updated, but it was not."));
			return -1;
		}
		return(ret);
	} else {
		px_error(pxdoc, PX_RuntimeError, _("Could not find record for update."));
		return -1;
	}
}
/* }}} */

/* PX_delete_record() {{{
 * Deletes a record into the paradox file.
 */
PXLIB_API int PXLIB_CALL
PX_delete_record(pxdoc_t *pxdoc, int recno) {
	pxhead_t *pxh;
	pxstream_t *pxs;
	pxblob_t *pxblob;
	pxdatablockinfo_t tmppxdbinfo;
	int found;
	int deleted = 0;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return -1;
	}
	pxh = pxdoc->px_head;
	pxs = pxdoc->px_stream;
	pxblob = pxdoc->px_blob;

	if((recno < 0) || (recno >= pxh->px_numrecords)) {
		px_error(pxdoc, PX_RuntimeError, _("Record number out of range."));
		return -1;
	}

	if(pxdoc->px_indexdata)
		found = px_get_record_pos_with_index(pxdoc, recno, &deleted, &tmppxdbinfo);
	else
		found = px_get_record_pos(pxdoc, recno, &deleted, &tmppxdbinfo);

	if(found) {
		int ret, datablocknr;

		/* Delete all blobs associated with this record */
		if(px_delete_blobs(pxdoc, tmppxdbinfo.recordpos) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could delete blobs of record."));
			return -1;
		}

		/* We need to calculate the physical block number. tmppxdbinfo.number
		 * contains the logical number of the block as stored in the header of
		 * each block.
		 */
		datablocknr = ((tmppxdbinfo.blockpos - pxh->px_headersize) / (pxh->px_maxtablesize*0x400)) + 1;
		ret = px_delete_data_from_block(pxdoc, pxh, datablocknr, tmppxdbinfo.recno, pxdoc->px_stream);
		if(ret >= 0) {
			pxh->px_numrecords--;
			put_px_head(pxdoc, pxh, pxdoc->px_stream);

			/* Update the primary index */
			if(pxdoc->px_indexdata) {
				pxpindex_t *pindex = pxdoc->px_indexdata;
				pindex[datablocknr-1].numrecords = ret;
			}

		} else {
			px_error(pxdoc, PX_RuntimeError, _("Error while deleting record data. Error number %d."), ret);
		}
		return(ret);
	} else {
		px_error(pxdoc, PX_RuntimeError, _("Could not find record for deletion."));
		return -1;
	}
}
/* }}} */

/* PX_pack() {{{
 * Packs database into the smallest possible file size by filling
 * all datablocks to its maximum number of records and deleting
 * empty datablocks.
 */
PXLIB_API int PXLIB_CALL
PX_pack(pxdoc_t *pxdoc) {
	pxhead_t *pxh;
	pxstream_t *pxs;
	pxpindex_t *pindex_data;
	long blockpos, blockoutpos;
	long recordpos, recordoutpos;
	long blocknumber, blockoutnumber;
	int recsperblock;
	int i, j, n, jout, nout;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return -1;
	}
	pxh = pxdoc->px_head;
	pxs = pxdoc->px_stream;
	pindex_data = pxdoc->px_indexdata;
	recsperblock = (pxh->px_maxtablesize*0x400-sizeof(TDataBlock)) / pxh->px_recordsize;

	jout = 0;
	nout = 0;
	blockoutnumber = pindex_data[jout].blocknumber;
	blockoutpos = pxh->px_headersize + (blockoutnumber-1)*pxh->px_maxtablesize*0x400;
	for(j=0; j<pxdoc->px_indexdatalen; j++) {
		if(pindex_data[j].level == 1) {
			int n;
			blocknumber = pindex_data[j].blocknumber;
			blockpos = pxh->px_headersize + (blocknumber-1)*pxh->px_maxtablesize*0x400;
			n = pindex_data[j].numrecords;
			for(i=0; i<n; i++) {
				recordpos = blockpos + sizeof(TDataBlock) + i*pxh->px_recordsize;
				recordoutpos = blockoutpos + sizeof(TDataBlock) + nout*pxh->px_recordsize;
				if(recordpos != recordoutpos)
					fprintf(stdout, "copy record from 0x%lX (block %d) to 0x%lX (block %d)\n", recordpos, j, recordoutpos, jout);
				nout++;
				if(nout > recsperblock) {
					jout++;
					nout = 0;
					blockoutnumber = pindex_data[jout].blocknumber;
					blockoutpos = pxh->px_headersize + (blockoutnumber-1)*pxh->px_maxtablesize*0x400;
				}
			}
		}
	}
	if(nout == 0)
		jout--;
	if(pxdoc->px_indexdatalen > jout+1) {
		for(i=jout; i<pxdoc->px_indexdatalen; i++) {
			fprintf(stdout, "Block %d is empty\n", i);
		}
	}
	return 0;
}
/* }}} */

/* PX_close() {{{
 * Close a Paradox file, but only if it was opened with PX_open_file().
 * This function will not free any memory.
 */
PXLIB_API void PXLIB_CALL
PX_close(pxdoc_t *pxdoc) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return;
	}

	/* Write modified cache block */
	px_flush(pxdoc, pxdoc->px_stream);

	if(pxdoc->px_blob) {
		PX_delete_blob(pxdoc->px_blob);
		pxdoc->px_blob = NULL;
	}

	if(pxdoc->px_stream && pxdoc->px_stream->close && (pxdoc->px_stream->s.fp != NULL)){
		fclose(pxdoc->px_stream->s.fp);
	}

	/* Free memory for io stream */
	if(pxdoc->px_stream) {
		pxdoc->free(pxdoc, pxdoc->px_stream);
		pxdoc->px_stream = NULL;
	}

//	pxdoc->px_head = NULL;
}
/* }}} */

/* PX_delete() {{{
 * Frees all memory use by the Paradox file. If PX_close() had not
 * been called before, it will be now.
 * FIXME: Many calls of free should rather be done PX_close()
 */
PXLIB_API void PXLIB_CALL
PX_delete(pxdoc_t *pxdoc) {
	pxfield_t *pfield;
	int i;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return;
	}

	/* Make sure the files are closed. If they were already closed
	 * it is not problem to call the functions again.
	 */
	PX_close(pxdoc);

#if PX_USE_RECODE
	if(pxdoc->recode_outer)
		recode_delete_outer(pxdoc->recode_outer);

	if(pxdoc->out_recode_request)
		recode_delete_request(pxdoc->out_recode_request);
	if(pxdoc->in_recode_request)
		recode_delete_request(pxdoc->in_recode_request);
#else
#if PX_USE_ICONV
	if((int) pxdoc->out_iconvcd > 0)
		iconv_close(pxdoc->out_iconvcd);
	if((int) pxdoc->in_iconvcd > 0)
		iconv_close(pxdoc->in_iconvcd);
#endif
#endif

	if(pxdoc->targetencoding)
		pxdoc->free(pxdoc, pxdoc->targetencoding);
	if(pxdoc->inputencoding)
		pxdoc->free(pxdoc, pxdoc->inputencoding);
	if(pxdoc->px_name)
		pxdoc->free(pxdoc, pxdoc->px_name);

	if(pxdoc->px_head != NULL) {
		if(pxdoc->px_head->px_tablename) pxdoc->free(pxdoc, pxdoc->px_head->px_tablename);
		pfield = pxdoc->px_head->px_fields;
		if(pfield != NULL) {
			for(i=0; i<pxdoc->px_head->px_numfields; i++) {
				if(pfield->px_fname) pxdoc->free(pxdoc, pfield->px_fname);
				pfield++;
			}
			pxdoc->free(pxdoc, pxdoc->px_head->px_fields);
		}
		pxdoc->free(pxdoc, pxdoc->px_head);
	}
	if(pxdoc->px_data) {
		/* Free the data of the file. In case of an primary index file
		 * this is the index data
		 * FIXME: need to free the memory pointed to by px_data->data
		 */
		pxdoc->free(pxdoc, pxdoc->px_data);
		pxdoc->px_datalen = 0;
	}
	/* px_indexdata will be set if the index was read from an index file
	 * or build during PX_open_fp(). In the first case it is just a
	 * pointer to pxdoc->px_index->px_data and should not be freed
	 * because it is freed when the index file is deleted.
	 */
	if(pxdoc->px_indexdata && !pxdoc->px_pindex) {
		pxdoc->free(pxdoc, pxdoc->px_indexdata);
		pxdoc->px_indexdatalen = 0;
	}

	/* Free the memory for the block cache */
	if(pxdoc->curblock) {
		pxdoc->free(pxdoc, pxdoc->curblock);
	}

	pxdoc->free(pxdoc, pxdoc);
}
/* }}} */

/* PX_get_fields() {{{
 * Returns a pointer onto the first field specification. This is identical
 * to calling PX_get_field() with a recno of 0.
 */
PXLIB_API pxfield_t* PXLIB_CALL
PX_get_fields(pxdoc_t *pxdoc) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return NULL;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return NULL;
	}

	return(pxdoc->px_head->px_fields);
}
/* }}} */

/* PX_get_field() {{{
 * Returns a pointer onto a field (column) specification. The first
 * column/field has index 0.
 */
PXLIB_API pxfield_t* PXLIB_CALL
PX_get_field(pxdoc_t *pxdoc, int fieldno) {
	pxhead_t *pxh;
	pxfield_t *pfield;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return NULL;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return NULL;
	}

	pxh = pxdoc->px_head;

	if((fieldno < 0) || (fieldno >= pxh->px_numfields)) {
		px_error(pxdoc, PX_RuntimeError, _("Field number out of range."));
		return NULL;
	}

	pfield = pxh->px_fields;
	pfield += fieldno;

	return(pfield);
}
/* }}} */

/* PX_get_num_fields() {{{
 * Returns the number of fields/columns.
 */
PXLIB_API int PXLIB_CALL
PX_get_num_fields(pxdoc_t *pxdoc) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return -1;
	}

	return(pxdoc->px_head->px_numfields);
}
/* }}} */

/* PX_get_num_records() {{{
 * Returns the number of records in a Paradox file.
 */
PXLIB_API int PXLIB_CALL
PX_get_num_records(pxdoc_t *pxdoc) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return -1;
	}

	return(pxdoc->px_head->px_numrecords);
}
/* }}} */

/* PX_get_recordsize() {{{
 * Returns the number of bytes per records in a Paradox file.
 */
PXLIB_API int PXLIB_CALL
PX_get_recordsize(pxdoc_t *pxdoc) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("File has no header."));
		return -1;
	}

	return(pxdoc->px_head->px_recordsize);
}
/* }}} */

/* PX_set_targetencoding() {{{
 * Sets the encoding of the output data. This is one of
 * the encodings supported by iconv or recode.
 */
PXLIB_API int PXLIB_CALL
PX_set_targetencoding(pxdoc_t *pxdoc, const char *encoding) {
	int codepage;
#if PX_USE_RECODE || PX_USE_ICONV
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Header of file has not been read."));
		return -1;
	}

	if(pxdoc->targetencoding) {
		px_error(pxdoc, PX_RuntimeError, _("Target encoding already set."));
		return -1;
	}

	pxdoc->targetencoding = px_strdup(pxdoc, encoding);
	if(0 > px_set_targetencoding(pxdoc)) {
		pxdoc->free(pxdoc, pxdoc->targetencoding);
		pxdoc->targetencoding = NULL;
		px_error(pxdoc, PX_RuntimeError, _("Target encoding could not be set."));
		return -1;
	}
#else
	px_error(pxdoc, PX_RuntimeError, _("Library has not been compiled with support for reencoding."));
	return -2;
#endif
	if(sscanf(encoding, "CP%d", &codepage)) {
		PX_set_value(pxdoc, "codepage", codepage);
	}
	return 0;
}
/* }}} */

/* PX_set_inputencoding() {{{
 * Sets the encoding of the input data. This is one of
 * the encodings supported by iconv or recode. The input encoding
 * must be set before the target encoding to have effect. If the input
 * encoding is not set it will be taken from the paradox header.
 */
PXLIB_API int PXLIB_CALL
PX_set_inputencoding(pxdoc_t *pxdoc, const char *encoding) {
#if PX_USE_RECODE || PX_USE_ICONV
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Header of file has not been read."));
		return -1;
	}

	if(pxdoc->inputencoding) {
		px_error(pxdoc, PX_RuntimeError, _("Input encoding already set."));
		return -1;
	}

	pxdoc->inputencoding = px_strdup(pxdoc, encoding);
	if(0 > px_set_inputencoding(pxdoc)) {
		pxdoc->free(pxdoc, pxdoc->inputencoding);
		pxdoc->inputencoding = NULL;
		px_error(pxdoc, PX_RuntimeError, _("Input encoding could not be set."));
		return -1;
	}

#else
	px_error(pxdoc, PX_RuntimeError, _("Library has not been compiled with support for reencoding."));
	return -2;
#endif
	return 0;
}
/* }}} */

/* PX_set_tablename() {{{
 * Sets the name of the table as stored in database file.
 */
PXLIB_API int PXLIB_CALL
PX_set_tablename(pxdoc_t *pxdoc, const char *tablename) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_head == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Header of file has not been read."));
		return -1;
	}

	if(pxdoc->px_head->px_tablename)
		pxdoc->free(pxdoc, pxdoc->px_head->px_tablename);

	pxdoc->px_head->px_tablename = px_strdup(pxdoc, tablename);
	if(put_px_head(pxdoc, pxdoc->px_head, pxdoc->px_stream) < 0) {
		return -1;
	}
	return 0;
}
/* }}} */

/******* Function to access Blob files *******/

/* build_mb_block_list() {{{
 * Build a primary index.
 */
static int build_mb_block_list(pxblob_t *pxblob) {
	pxdoc_t *pxdoc;
	pxstream_t *pxs;
	int i;
	size_t filesize;
	int numblocks;
	pxmbblockinfo_t *blocklist;
	TMbBlockHeader2 mbblockhead;

	pxdoc = pxblob->pxdoc;
	pxs = pxblob->mb_stream;

	if(pxblob->seek(pxblob, pxs, 0, SEEK_END) < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Could not go to end of blob file."));
		return -1;
	}
	filesize = pxblob->tell(pxblob, pxs);
	if(filesize & 0x00000fff) {
		px_error(pxdoc, PX_RuntimeError, _("Size of blob file is not multiple of 4kB."));
		return -1;
	}

	if(pxblob->seek(pxblob, pxs, 0, SEEK_SET) < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Could not go to start of blob file."));
		return -1;
	}

	numblocks = filesize >> 12;
	if(NULL == (blocklist = pxdoc->malloc(pxdoc, numblocks*sizeof(pxmbblockinfo_t), _("Allocate memory for block info in blob file.")))) {
		return -1;
	}
	i = 0;
	for(i=0; i<numblocks; i++) {
		if(pxblob->seek(pxblob, pxs, i*4096, SEEK_SET) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not go to start of block in blob file."));
			pxdoc->free(pxdoc, blocklist);
			return -1;
		}

		if(pxblob->read(pxblob, pxs, sizeof(TMbBlockHeader3), &mbblockhead) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not read header of block in blob file."));
			pxdoc->free(pxdoc, blocklist);
			return -1;
		}
		blocklist[i].number = i;
		blocklist[i].type = mbblockhead.type;
		blocklist[i].numblocks = (int) (get_short_le((char *) &mbblockhead.numBlocks));
//		fprintf(stderr, "Block %d is of type %d\n", i, blocklist[i].type);
		if(blocklist[i].type == 3) {
			int j;
			blocklist[i].numblobs = 0;
			blocklist[i].allocspace = 0;

			for(j=0; j<64; j++) {
				TMbBlockHeader3Table mbbhtab;
				if(pxblob->read(pxblob, pxs, sizeof(TMbBlockHeader3Table), &mbbhtab) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not read blob pointer."));
					pxdoc->free(pxdoc, blocklist);
					return -1;
				}
				if(mbbhtab.offset != 0) {
//					fprintf(stderr, "  %d. Found blob with %d x 16 bytes\n", j, mbbhtab.length);
					blocklist[i].numblobs++;
					blocklist[i].allocspace += mbbhtab.length;
				}
			}
//			fprintf(stderr, "  Block of type 3 had %d blobs using %d from 235 x 16 bytes\n", blocklist[i].numblobs, blocklist[i].allocspace);
		} else {
			blocklist[i].numblobs = 1;
			blocklist[i].allocspace = 0;
		}

	}
	if(NULL != pxblob->blocklist) {
		pxdoc->free(pxdoc, pxblob->blocklist);
	}
	pxblob->blocklist = blocklist;
	pxblob->blocklistlen = numblocks;
	return 0;
}
/* }}} */

/* PX_new_blob() {{{
 * Create a new blob document
 */
PXLIB_API pxblob_t* PXLIB_CALL
PX_new_blob(pxdoc_t *pxdoc) {
	pxblob_t *pxblob;

	if(NULL == (pxblob = pxdoc->malloc(pxdoc, sizeof(pxblob_t), _("Allocate memory for blob.")))) {
		px_error(pxdoc, PX_RuntimeError, _("Could not allocate memory for blob."));
		return(NULL);
	}

	memset(pxblob, 0, sizeof(pxblob_t));
	pxblob->pxdoc = pxdoc;
	pxdoc->px_blob = pxblob;
	return(pxblob);
}
/* }}} */

/* PX_open_blob_fp() {{{
 * Opens a blob with a given already open file pointer
 */
PXLIB_API int PXLIB_CALL
PX_open_blob_fp(pxblob_t *pxblob, FILE *fp) {
	pxdoc_t *pxdoc;
	pxstream_t *pxs;

	if(NULL == (pxdoc = pxblob->pxdoc)) {
		px_error(pxdoc, PX_RuntimeError, _("No paradox document associated with blob file."));
		return -1;
	}

	if(NULL == (pxs = px_stream_new_file(pxdoc, pxfFileRead, px_false, fp))) {
		px_error(pxdoc, PX_MemoryError, _("Could not create new file io stream."));
		return -1;
	}

	pxblob->mb_stream = pxs;

	pxblob->read = px_mb_read;
	pxblob->seek = px_mb_seek;
	pxblob->tell = px_mb_tell;
	pxblob->write = px_mb_write;

	if((pxblob->mb_head = get_mb_head(pxblob, pxs)) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Unable to get header of blob file."));
		return -1;
	}

	build_mb_block_list(pxblob);
	pxblob->used_datablocks = pxblob->blocklistlen-1;

	return(0);
}
/* }}} */

/* PX_open_blob_file() {{{
 * Opens a file of a blob with the given filename
 */
PXLIB_API int PXLIB_CALL
PX_open_blob_file(pxblob_t *pxblob, const char *filename) {
	FILE *fp;

	if(!pxblob) {
		return(-1);
	}

	if((fp = fopen(filename, "rb+")) == NULL) {
		return -1;
	}

	if(0 > PX_open_blob_fp(pxblob, fp)) {
		fclose(fp);
		return -1;
	}

	pxblob->mb_name = px_strdup(pxblob->pxdoc, filename);
	pxblob->mb_stream->close = px_true;
	return 0;
}
/* }}} */

/* PX_create_blob_fp() {{{
 * Creates a blob with a given already open file pointer
 */
PXLIB_API int PXLIB_CALL
PX_create_blob_fp(pxblob_t *pxblob, FILE *fp) {
	pxdoc_t *pxdoc;
	pxstream_t *pxs;
	mbhead_t *mbh;

	if(NULL == (pxdoc = pxblob->pxdoc)) {
		px_error(pxdoc, PX_RuntimeError, _("No paradox document associated with blob file."));
		return -1;
	}

	if(NULL == (pxs = px_stream_new_file(pxdoc, pxfFileWrite, px_false, fp))) {
		px_error(pxdoc, PX_MemoryError, _("Could not create new file io stream."));
		return -1;
	}

	pxblob->mb_stream = pxs;

	pxblob->read = px_mb_read;
	pxblob->seek = px_mb_seek;
	pxblob->tell = px_mb_tell;
	pxblob->write = px_mb_write;

	if(NULL == (mbh = pxdoc->malloc(pxdoc, sizeof(mbhead_t), _("Allocate memory for header of blob file.")))) {
		px_error(pxdoc, PX_MemoryError, _("Could not allocate memory for header of blob file."));
		return -1;
	}
	memset(mbh, 0, sizeof(mbhead_t));
	if(put_mb_head(pxblob, mbh, pxs) < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Unable to put header."));
		return -1;
	}

	pxblob->mb_head = mbh;
	pxblob->used_datablocks = 0;

	return(0);
}
/* }}} */

/* PX_create_blob_file() {{{
 * Create a new file for blob data with the given filename
 */
PXLIB_API int PXLIB_CALL
PX_create_blob_file(pxblob_t *pxblob, const char *filename) {
	FILE *fp;
	pxdoc_t *pxdoc;

	if(!pxblob) {
		return(-1);
	}

	if(NULL == (pxdoc = pxblob->pxdoc)) {
		px_error(pxdoc, PX_RuntimeError, _("No paradox document associated with blob file."));
		return -1;
	}

	if((fp = fopen(filename, "wb+")) == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Could not open blob file '%s' for writing."), filename);
		return -1;
	}

	if(0 > PX_create_blob_fp(pxblob, fp)) {
		fclose(fp);
		return -1;
	}

	pxblob->mb_name = px_strdup(pxblob->pxdoc, filename);
	pxblob->mb_stream->close = px_true;

	return 0;
}
/* }}} */

/* PX_close_blob() {{{
 * Close a blob file
 */
PXLIB_API void PXLIB_CALL
PX_close_blob(pxblob_t *pxblob) {
	pxdoc_t *pxdoc;
	if(NULL == (pxdoc = pxblob->pxdoc)) {
		px_error(pxdoc, PX_RuntimeError, _("No paradox document associated with blob file."));
	}

	if(pxblob->mb_stream && pxblob->mb_stream->close && (pxblob->mb_stream->s.fp != NULL)){
		fclose(pxblob->mb_stream->s.fp);
		pxdoc->free(pxdoc, pxblob->mb_stream);
		pxblob->mb_stream = NULL;
		pxdoc->free(pxdoc, pxblob->mb_name);
		pxblob->mb_name = NULL;
		pxdoc->free(pxdoc, pxblob->mb_head);
		pxblob->mb_head = NULL;
	}
}
/* }}} */

/* PX_delete_blob() {{{
 * Deletes a blob object
 */
PXLIB_API void PXLIB_CALL
PX_delete_blob(pxblob_t *pxblob) {
	PX_close_blob(pxblob);
	if(pxblob->blockcache.data)
		pxblob->pxdoc->free(pxblob->pxdoc, pxblob->blockcache.data);
	if(pxblob->blocklist)
		pxblob->pxdoc->free(pxblob->pxdoc, pxblob->blocklist);
	pxblob->pxdoc->free(pxblob->pxdoc, pxblob);
}
/* }}} */

/* PX_set_blob_file() {{{
 * Sets the name of the file containing the blobs.
 */
PXLIB_API int PXLIB_CALL
PX_set_blob_file(pxdoc_t *pxdoc, const char *filename) {
	pxblob_t *pxblob;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_stream == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Paradox database has not been opened or created when setting the blob file."));
		return -1;
	}

	if(pxdoc->px_blob != NULL) {
		px_error(pxdoc, PX_Warning, _("Blob file has been set already. I will delete the existing one."));
		PX_delete_blob(pxdoc->px_blob);
		pxdoc->px_blob = NULL;
	}

	if(NULL == (pxblob = PX_new_blob(pxdoc))) {
		px_error(pxdoc, PX_RuntimeError, _("Could not create new blob file object."));
		return -1;
	}

	/* If the paradox database was opend for reading the blob will be too. */
	if(pxdoc->px_stream->mode == pxfFileRead) {
		if(0 > PX_open_blob_file(pxblob, filename)) {
			px_error(pxdoc, PX_RuntimeError, _("Could not open blob file."));
			return -1;
		}
	} else {
		if(0 > PX_create_blob_file(pxblob, filename)) {
			px_error(pxdoc, PX_RuntimeError, _("Could not create blob file."));
			return -1;
		}
	}

	pxdoc->px_blob = pxblob;

	return 0;
}
/* }}} */

/* PX_set_blob_fp() {{{
 * Sets the file pointer of the file containing the blobs.
 */
PXLIB_API int PXLIB_CALL
PX_set_blob_fp(pxdoc_t *pxdoc, FILE *fp) {
	pxblob_t *pxblob;

	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_stream == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Paradox database has not been opened or created when setting the blob file."));
		return -1;
	}

	if(pxdoc->px_blob != NULL) {
		px_error(pxdoc, PX_Warning, _("Blob file has been set already. I will delete the existing one."));
		PX_delete_blob(pxdoc->px_blob);
		pxdoc->px_blob = NULL;
	}

	if(NULL == (pxblob = PX_new_blob(pxdoc))) {
		px_error(pxdoc, PX_RuntimeError, _("Could not create new blob file object."));
		return -1;
	}

	/* If the paradox database was opend for reading the blob will be too. */
	if(pxdoc->px_stream->mode == pxfFileRead) {
		if(0 > PX_open_blob_fp(pxblob, fp)) {
			px_error(pxdoc, PX_RuntimeError, _("Could not open blob file."));
			return -1;
		}
	} else {
		if(0 > PX_create_blob_fp(pxblob, fp)) {
			px_error(pxdoc, PX_RuntimeError, _("Could not create blob file."));
			return -1;
		}
	}

	pxdoc->px_blob = pxblob;

	return 0;
}
/* }}} */

/* PX_has_blob_file() {{{
 * Returns 1 if a blob file has been set before, otherwise 0.
 */
PXLIB_API int PXLIB_CALL
PX_has_blob_file(pxdoc_t *pxdoc) {
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return -1;
	}

	if(pxdoc->px_blob != NULL)
		return 1;
	else
		return 0;
}
/* }}} */

/* _px_read_blobdata() {{{
 * Reads data of blob into memory and returns a pointer to it
 */
static char*
_px_read_blobdata(pxblob_t *pxblob, const char *data, int len, int hsize, int *mod, int *blobsize) {
	int ret;
	char *blobdata = NULL;
	unsigned char head[12];
	pxdoc_t *pxdoc = pxblob->pxdoc;
	size_t size, offset, mod_nr, index;
	int leader = len - 10;

	size = get_long_le(&data[leader+4]);
	if(hsize == 17)
		*blobsize = size - 8;
	else
		*blobsize = size;
	index = get_long_le(&data[leader]) & 0x000000ff;
	*mod = mod_nr = get_short_le(&data[leader+8]);
/*	fprintf(stderr, "index=%ld ", index); */
/*	fprintf(stderr, "size=%ld ", size); */
/*	fprintf(stderr, "mod_nr=%d \n", mod_nr); */

	if(!pxblob || !pxblob->mb_stream) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a blob file."));
		return(NULL);
	}

	if(*blobsize <= 0) {
		px_error(pxdoc, PX_RuntimeError, _("Makes no sense to read blob with 0 or less bytes."));
		return(NULL);
	}

	if(*blobsize <= leader) {
		if(NULL == (blobdata = pxdoc->malloc(pxblob->pxdoc, *blobsize, _("Allocate memory for blob.")))) {
			px_error(pxdoc, PX_MemoryError, _("Could not allocate memory for blob."));
			return(NULL);
		}
		memcpy(blobdata, data, *blobsize);
	} else {
		offset = get_long_le(&data[leader]) & 0xffffff00;
		if(offset == 0) {
			*blobsize = 0;
			return(NULL);
		}
//		fprintf(stderr, "offset=%ld ", offset);

		if((ret = pxblob->seek(pxblob, pxblob->mb_stream, offset, SEEK_SET)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not fseek start of blob."));
			return NULL;
		}

		/* Just read the first 3 Bytes because they are common for all block */
		if((ret = pxblob->read(pxblob, pxblob->mb_stream, 3, head)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not read head of blob data."));
			return NULL;
		}

		if(head[0] == 0) {
			px_error(pxdoc, PX_RuntimeError, _("Trying to read blob data from 'header' block."));
			return NULL;
		} else if(head[0] == 4) {
			px_error(pxdoc, PX_RuntimeError, _("Trying to read blob data from a 'free' block."));
			return NULL;
		}

		if(head[0] == 2) { /* Reading data from a block type 2 */
			if(index != 0xff) {
				px_error(pxdoc, PX_RuntimeError, _("Offset points to a single blob block but index field is not 0xff."));
				return NULL;
			}
			/* Read the remaining 6/14 bytes from the header */
			if((ret = pxblob->read(pxblob, pxblob->mb_stream, hsize-3, head)) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not read remaining head of single data block."));
				return NULL;
			}
			if(size != get_long_le((const char *) &head[0])) {
				px_error(pxdoc, PX_RuntimeError, _("Blob does not have expected size (%d != %d)."), size, get_long_le((const char *)&head[0]));
				return(NULL);
			}
			/* We may check for identical modificatio number as well, if it
			 * was passed to PX_read_blobdata()
			 */

			if(NULL == (blobdata = pxdoc->malloc(pxblob->pxdoc, *blobsize, _("Allocate memory for blob.")))) {
				px_error(pxdoc, PX_MemoryError, _("Could not allocate memory for blob."));
				return(NULL);
			}

			if((ret = pxblob->read(pxblob, pxblob->mb_stream, *blobsize, blobdata)) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not read all blob data."));
				return NULL;
			}
		} else if(head[0] == 3) { /* Reading data from a block type 3 */
			/* Read the remaining 9 bytes from the header */
			if((ret = pxblob->read(pxblob, pxblob->mb_stream, 9, head)) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not read remaining head of suballocated block."));
				return NULL;
			}
			/* Goto the blob pointer with the passed index */
			if((ret = pxblob->seek(pxblob, pxblob->mb_stream, offset+12+index*5, SEEK_SET)) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not fseek blob pointer."));
				return NULL;
			}
			/* Read the blob pointer */
			if((ret = pxblob->read(pxblob, pxblob->mb_stream, 5, head)) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not read blob pointer."));
				return NULL;
			}
			if(size != ((int)head[1]-1)*16+head[4]) {
				px_error(pxdoc, PX_RuntimeError, _("Blob does not have expected size (%d != %d)."), size, ((int)head[1]-1)*16+head[4]);
				return(NULL);
			}
			if(NULL == (blobdata = pxdoc->malloc(pxblob->pxdoc, size, _("Allocate memory for blob.")))) {
				px_error(pxdoc, PX_MemoryError, _("Could not allocate memory for blob."));
				return(NULL);
			}
			/* Goto the start of the blob */
			if((ret = pxblob->seek(pxblob, pxblob->mb_stream, offset+head[0]*16, SEEK_SET)) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not fseek start of blob."));
				return NULL;
			}
			if((ret = pxblob->read(pxblob, pxblob->mb_stream, size, blobdata)) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not read all blob data."));
				return NULL;
			}
		}
	}
	return(blobdata);
}
/* }}} */

/* PX_read_blobdata() {{{
 * Reads data of a blob into memory and returns a pointer to it
 */
PXLIB_API char* PXLIB_CALL
PX_read_blobdata(pxblob_t *pxblob, const char *data, int len, int *mod, int *blobsize) {
	return(_px_read_blobdata(pxblob, data, len, 9, mod, blobsize));
}
/* }}} */

/* PX_read_graphicdata() {{{
 * Reads data of a graphic into memory and returns a pointer to it
 */
PXLIB_API char* PXLIB_CALL
PX_read_graphicdata(pxblob_t *pxblob, const char *data, int len, int *mod, int *blobsize) {
	return(_px_read_blobdata(pxblob, data, len, 17, mod, blobsize));
}
/* }}} */

/******* Function to access record data ******/

/* PX_get_data_alpha() {{{
 * Extracts an alpha field value from a data block
 */
PXLIB_API int PXLIB_CALL
PX_get_data_alpha(pxdoc_t *pxdoc, char *data, int len, char **value) {
	char *buffer, *obuf = NULL;
	size_t olen;
	int res;

	if(data[0] == '\0') {
		*value = NULL;
		return 0;
	}

	if(pxdoc->targetencoding != NULL) {
#if PX_USE_RECODE
		int oallocated = 0;
		res = recode_buffer_to_buffer(pxdoc->out_recode_request, data, len, &obuf, &olen, &oallocated);
#else
#if PX_USE_ICONV
		size_t ilen;
		char *iptr, *optr;
		/* Worst case for length of output buffer. If conversion from 1 byte
		 * to 2 byte chars takes place
		 */
		olen = 2*len + 1;
		/* Do not pxdoc->malloc because the memory is freed with free
		 * We use free because the memory allocated by recode_buffer_to_buffer()
		 * is requested with malloc and must be freed with free.
		 */
		optr = obuf = (char *) malloc(olen);
		iptr = data;
		ilen = 0;
		while(iptr[ilen] != '\0' && ilen < (size_t) len)
			ilen++;
//		printf("data(%d) = '%s'\n", ilen, data);
//		printf("obuf(%d) = '%s'\n", olen, obuf);
		if(0 > (res = iconv(pxdoc->out_iconvcd, &iptr, &ilen, &optr, &olen))) {
			*value = NULL;
			free(obuf);
			return -1;
		}
		*optr = '\0';
//		printf("data(%d) = '%s'\n", ilen, data);
//		printf("obuf(%d) = '%s'\n", olen, obuf);
		olen = optr-obuf;
#endif
#endif
	} else {
		olen = len;
		obuf = data;
	}
	/* Copy the encoded string into memory which belongs to pxlib */
	buffer = (char *) pxdoc->malloc(pxdoc, olen+1, _("Allocate memory for field data."));
	if(!buffer) {
		if(pxdoc->targetencoding != NULL) {
			free(obuf);
		}
		*value = NULL;
		return -1;
	}
	memcpy(buffer, obuf, olen);
	buffer[olen] = '\0';
	*value = buffer;

	if(pxdoc->targetencoding != NULL) {
		free(obuf);
	}

	return 1;
}
/* }}} */

/* PX_get_data_bytes() {{{
 * Extracts a bytes field value from a data block
 * FIXME: This function should allocate the memory for the return value
 */
PXLIB_API int PXLIB_CALL
PX_get_data_bytes(pxdoc_t *pxdoc, char *data, int len, char **value) {
	char *buffer;
	if(data[0] == '\0') {
//		*value = NULL;
		return 0;
	}

	buffer = (char *) pxdoc->malloc(pxdoc, len, _("Allocate memory for field data."));
	if(!buffer) {
		*value = NULL;
		return -1;
	}
	memcpy(buffer, data, len);
	*value = buffer;

	return 1;
}
/* }}} */

/* PX_get_data_double() {{{
 * Extracts a double from a data block
 */
PXLIB_API int PXLIB_CALL
PX_get_data_double(pxdoc_t *pxdoc, char *data, int len, double *value) {
	char tmp[8];
	memcpy(&tmp, data, 8);
	if(tmp[0] & 0x80) {
		tmp[0] &= 0x7f;
	} else if(*((long int *)tmp) != 0) {
		int k;
		for(k=0; k<len; k++)
			tmp[k] = ~tmp[k];
	} else {
		*value = 0;
		return 0;
	}
	*value = get_double_be(tmp); // *((double *)tmp);
	return 1;
}
/* }}} */

/* PX_get_data_long() {{{
 * Extracts a long integer from a data block
 */
PXLIB_API int PXLIB_CALL
PX_get_data_long(pxdoc_t *pxdoc, char *data, int len, long *value) {
	char tmp[4];
	memcpy(&tmp, data, 4);
	if(tmp[0] & 0x80) {
		tmp[0] &= 0x7f;
	} else if(*((long int *)tmp) != 0) {
		tmp[0] |= 0x80;
	} else {
		*value = 0;
		return 0;
	}
	*value = get_long_be(tmp);
	return 1;
}
/* }}} */

/* PX_get_data_short() {{{
 * Extracts a short integer in a data block
 */
PXLIB_API int PXLIB_CALL
PX_get_data_short(pxdoc_t *pxdoc, char *data, int len, short int *value) {
	char tmp[2];
	memcpy(&tmp, data, 2);
	if(tmp[0] & 0x80) {
		tmp[0] &= 0x7f;
	} else if(*((short int *)tmp) != 0) {
		tmp[0] |= 0x80;
	} else {
		*value = 0;
		return 0;
	}
	*value = get_short_be(tmp);
	return 1;
}
/* }}} */

/* PX_get_data_byte() {{{
 * Extracts a byte in a data block
 */
PXLIB_API int PXLIB_CALL
PX_get_data_byte(pxdoc_t *pxdoc, char *data, int len, char *value) {
	if(data[0] & 0x80) {
		*value = data[0] & 0x7f;
		return 1;
	}
	if(*data != 0) {
		*value = data[0] | 0x80;
		return 1;
	}
	*value = *data;
	return 0;
}
/* }}} */

/* PX_get_data_bcd() {{{
 * Extracts a bcd number in a data block
 * len is the number decimal numbers
 */
PXLIB_API int PXLIB_CALL
PX_get_data_bcd(pxdoc_t *pxdoc, unsigned char *data, int len, char **value) {
	int i, j;
	unsigned char sign;
	unsigned char nibble;
	int size;
	int lz;   /* 1 as long as leading zeros are found */
	struct lconv *lc;
	char *buffer;

	if(data[0] == '\0') {
		*value = NULL;
		return 0;
	}
	buffer = (char *) pxdoc->malloc(pxdoc, 34+3, _("Allocate memory for field data."));
	if(!buffer) {
		*value = NULL;
		return -1;
	}

	j = 0;
	if(data[0] & 0x80) {
		sign = 0x00;
	} else  {
		buffer[j++] = '-';
		sign = 0x0F;
	}
	size = data[0] & 0x3f;
	if(size != len) {
		*value = NULL;
		return -1;
	}
	lz = 1;
	for(i=2; i<34-size; i++) {
		if(i%2)
			nibble = data[i/2] & 0x0f;
		else
			nibble = (data[i/2] >> 4) & 0x0f;
		if(lz && (nibble^sign))
			lz = 0;
		if(lz == 0)
			buffer[j++] = (nibble^sign)+48;
	}
	if(lz)
		buffer[j++] = '0';
#ifdef HAVE_LOCALE_H
	lc = localeconv();
	if(lc)
		buffer[j++] = lc->decimal_point[0];
	else
#endif
		buffer[j++] = '.';
	for(; i<34; i++) {
		if(i%2)
			nibble = data[i/2] & 0x0f;
		else
			nibble = (data[i/2] >> 4) & 0x0f;
		buffer[j++] = (nibble^sign)+48;
	}
	buffer[j] = '\0';
	*value = buffer;

	return 1;
}
/* }}} */

/* _px_get_data_blob() {{{
 * Reads data of blob or graphic into memory and returns a pointer to it.
 * The parameter hsize contains the length of the header right before
 * the blob/graphic in the .MB file. It is 17 Bytes for graphics and 9
 * for all other types of blobs (I'm not completely sure about OLE).
 */
static int
_px_get_data_blob(pxdoc_t *pxdoc, const char *data, int len, int hsize, int *mod, int *blobsize, char **value) {
	int ret;
	char *blobdata = NULL;
	unsigned char head[20];
	pxblob_t *pxblob = pxdoc->px_blob;
	size_t size, offset, mod_nr, index;
	int leader = len - 10;

	/* FIXME: This is a quick hack because graphic blobs have some extra
	 * 8 Bytes before the data which is contained in the size
	 * The real size of the graphic is stored in the second long within
	 * the extra 8 bytes. But this value seems to be alwasy 8 smaller
	 * then the size at [leader+4].
	 */
	size = get_long_le(&data[leader+4]);
	if(hsize == 17)
		*blobsize = size - 8;
	else
		*blobsize = size;
	index = get_long_le(&data[leader]) & 0x000000ff;
	*mod = mod_nr = get_short_le(&data[leader+8]);
/*	fprintf(stderr, "index=%ld ", index); */
/*	fprintf(stderr, "size=%ld ", size); */
/*	fprintf(stderr, "mod_nr=%d \n", mod_nr); */

	if(*blobsize <= 0) {
/*		px_error(pxdoc, PX_RuntimeError, _("Makes no sense to read blob with 0 or less bytes.")); */
		*value = NULL;
		return 0;
	}

	/* First check if the blob data is included in the record itself */
	if(*blobsize <= leader) {
		blobdata = pxdoc->malloc(pxdoc, *blobsize, _("Allocate memory for blob data."));
		if(!blobdata) {
			px_error(pxdoc, PX_RuntimeError, _("Could not allocate memory for blob data."));
			*value = NULL;
			return -1;
		}
		memcpy(blobdata, data, *blobsize);
		*value = blobdata;
		return(1);
	}

	/* Since the blob data is not in the record we will need a blob file */
	if(!pxblob || !pxblob->mb_stream) {
		px_error(pxdoc, PX_Warning, _("Blob data is not contained in record and a blob file is not set."));
		*value = NULL;
		return -1;
	}

	offset = get_long_le(&data[leader]) & 0xffffff00;
	if(offset == 0) {
		px_error(pxdoc, PX_Warning, _("Offset in blob file is unexpectedly zero."));
		*blobsize = 0;
		*value = NULL;
		return -1;
	}
//		fprintf(stderr, "offset=%ld ", offset);

	if((ret = pxblob->seek(pxblob, pxblob->mb_stream, offset, SEEK_SET)) < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Could not fseek start of blob."));
		*value = NULL;
		return -1;
	}

	/* Just read the first 3 Bytes because they are common for all block */
	if((ret = pxblob->read(pxblob, pxblob->mb_stream, 3, head)) < 0) {
		px_error(pxdoc, PX_RuntimeError, _("Could not read head of blob data."));
		*value = NULL;
		return -1;
	}

	if(head[0] == 0) {
		px_error(pxdoc, PX_RuntimeError, _("Trying to read blob data from 'header' block."));
		*value = NULL;
		return -1;
	} else if(head[0] == 4) {
		px_error(pxdoc, PX_RuntimeError, _("Trying to read blob data from a 'free' block."));
		*value = NULL;
		return -1;
	}

	if(head[0] == 2) { /* Reading data from a block type 2 */
		if(index != 0xff) {
			px_error(pxdoc, PX_RuntimeError, _("Offset points to a single blob block but index field is not 0xff."));
			*value = NULL;
			return -1;
		}
		/* Read the remaining 6 bytes from the header */
		if((ret = pxblob->read(pxblob, pxblob->mb_stream, hsize-3, head)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not read remaining head of single data block."));
			*value = NULL;
			return -1;
		}
		if(size != get_long_le((char*) &head[0])) {
			px_error(pxdoc, PX_RuntimeError, _("Blob does not have expected size (%d != %d)."), size, get_long_le((char *) &head[0]));
			*value = NULL;
			return -1;
		}
		/* We may check for identical modificatio number as well, if it
		 * was passed to PX_read_blobdata()
		 */

		blobdata = pxdoc->malloc(pxdoc, *blobsize, _("Allocate memory for blob data."));
		if(!blobdata) {
			px_error(pxdoc, PX_RuntimeError, _("Could not allocate memory for blob data."));
			*value = NULL;
			return -1;
		}

		if((ret = pxblob->read(pxblob, pxblob->mb_stream, *blobsize, blobdata)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not read all blob data."));
			*value = NULL;
			pxdoc->free(pxdoc, blobdata);
			return -1;
		}
	} else if(head[0] == 3) { /* Reading data from a block type 3 */
		/* Read the remaining 9 bytes from the header */
		if((ret = pxblob->read(pxblob, pxblob->mb_stream, 9, head)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not read remaining head of suballocated block."));
			*value = NULL;
			return -1;
		}
		/* Goto the blob pointer with the passed index */
		if((ret = pxblob->seek(pxblob, pxblob->mb_stream, offset+12+index*5, SEEK_SET)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not fseek blob pointer."));
			*value = NULL;
			return -1;
		}
		/* Read the blob pointer */
		if((ret = pxblob->read(pxblob, pxblob->mb_stream, 5, head)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not read blob pointer."));
			*value = NULL;
			return -1;
		}
		if(size != ((int)head[1]-1)*16+head[4]) {
			px_error(pxdoc, PX_RuntimeError, _("Blob does not have expected size (%d != %d)."), size, ((int)head[1]-1)*16+head[4]);
			*value = NULL;
			return -1;
		}
		blobdata = pxdoc->malloc(pxdoc, size, _("Allocate memory for blob data."));
		if(!blobdata) {
			px_error(pxdoc, PX_RuntimeError, _("Could not allocate memory for blob data."));
			*value = NULL;
			return -1;
		}
		/* Goto the start of the blob */
		if((ret = pxblob->seek(pxblob, pxblob->mb_stream, offset+head[0]*16, SEEK_SET)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not fseek start of blob."));
			*value = NULL;
			pxdoc->free(pxdoc, blobdata);
			return -1;
		}
		if((ret = pxblob->read(pxblob, pxblob->mb_stream, size, blobdata)) < 0) {
			px_error(pxdoc, PX_RuntimeError, _("Could not read all blob data."));
			*value = NULL;
			pxdoc->free(pxdoc, blobdata);
			return -1;
		}
	}

	*value = blobdata;
	return(1);
}
/* }}} */

/* PX_get_data_blob() {{{
 * Reads data of blob into memory and returns a pointer to it
 */
PXLIB_API int PXLIB_CALL
PX_get_data_blob(pxdoc_t *pxdoc, const char *data, int len, int *mod, int *blobsize, char **value) {
	return(_px_get_data_blob(pxdoc, data, len, 9, mod, blobsize, value));
}
/* }}} */

/* PX_get_data_graphic() {{{
 * Reads data of a graphic into memory and returns a pointer to it
 */
PXLIB_API int PXLIB_CALL
PX_get_data_graphic(pxdoc_t *pxdoc, const char *data, int len, int *mod, int *blobsize, char **value) {
	return(_px_get_data_blob(pxdoc, data, len, 17, mod, blobsize, value));
}
/* }}} */

/* PX_put_data_alpha() {{{
 * Stores a string in a data block.
 */
PXLIB_API void PXLIB_CALL
PX_put_data_alpha(pxdoc_t *pxdoc, char *data, int len, char *value) {
	char *obuf = NULL;
	size_t olen;
	int res;

	memset(data, 0, len);
	if((value == NULL) || (value[0] == '\0')) {
		return;
	}

	if(pxdoc->targetencoding != NULL) {
#if PX_USE_RECODE
		int oallocated = 0;
		res = recode_buffer_to_buffer(pxdoc->in_recode_request, value, strlen(value), &obuf, &olen, &oallocated);
#else
#if PX_USE_ICONV
		size_t ilen = strlen(value);
		char *iptr, *optr;
		olen = len + 1;
		/* Do not pxdoc->malloc because the memory is freed with free
		 * We use free because the memory allocated by recode_buffer_to_buffer()
		 * is requested with malloc and must be freed with free.
		 */
		optr = obuf = (char *) malloc(olen);
		iptr = value;
//		printf("value(%d) = '%s'\n", ilen, value);
//		printf("obuf(%d) = '%s'\n", olen, obuf);
		if(0 > (res = iconv(pxdoc->in_iconvcd, &iptr, &ilen, &optr, &olen))) {
			memset(data, 0, len);
			free(obuf);
			return;
		}
		*optr = '\0';
//		printf("value(%d) = '%s'\n", ilen, value);
//		printf("obuf(%d) = '%s'\n", olen, obuf);
		olen = optr-obuf;
#endif
#endif
	} else {
		olen = strlen(value);
		obuf = value;
	}

	memcpy(data, obuf, olen < (size_t) len ? olen : len);

	if(pxdoc->targetencoding != NULL) {
		free(obuf);
	}

}
/* }}} */

/* PX_put_data_bytes() {{{
 * Stores raw bytes in a data block.
 */
PXLIB_API void PXLIB_CALL
PX_put_data_bytes(pxdoc_t *pxdoc, char *data, int len, char *value) {
	memcpy(data, value, len);
}
/* }}} */

/* PX_put_data_double() {{{
 * Stores a double in a data block.
 * A len of 0 means to store a NULL value.
 */
PXLIB_API void PXLIB_CALL
PX_put_data_double(pxdoc_t *pxdoc, char *data, int len, double value) {
	if(len == 0) {
		memset(data, 0, 8);
	} else {
		put_double_be(data, value);
		if(value >= 0) {
			data[0] |= 0x80;
		} else {
			int k;
			for(k=0; k<len; k++)
				data[k] = ~data[k];
		}
	}
}
/* }}} */

/* PX_put_data_long() {{{
 * Stores an integer in a data block.
 * A len of 0 means to store a NULL value.
 */
PXLIB_API void PXLIB_CALL
PX_put_data_long(pxdoc_t *pxdoc, char *data, int len, int value) {
	if(len == 0) {
		memset(data, 0, 4);
	} else {
		put_long_be(data, value);
		if(value >= 0) {
			data[0] |= 0x80;
		} else {
			data[0] &= 0x7f;
		}
	}
}
/* }}} */

/* PX_put_data_short() {{{
 * Stores a short integer in a data block.
 * A len of 0 means to store a NULL value.
 */
PXLIB_API void PXLIB_CALL
PX_put_data_short(pxdoc_t *pxdoc, char *data, int len, short int value) {
	if(len == 0) {
		memset(data, 0, 2);
	} else {
		put_short_be(data, value);
		if(value >= 0) {
			data[0] |= 0x80;
		} else {
			data[0] &= 0x7f;
		}
	}
}
/* }}} */

/* PX_put_data_byte() {{{
 * Stores a byte in a data block.
 * A len of 0 means to store a NULL value.
 */
PXLIB_API void PXLIB_CALL
PX_put_data_byte(pxdoc_t *pxdoc, char *data, int len, char value) {
	if(len == 0) {
		memset(data, 0, 1);
	} else {
		*data = value;
		if(value >= 0) {
			data[0] |= 0x80;
		} else {
			data[0] &= 0x7f;
		}
	}
}
/* }}} */

/* PX_put_data_bcd() {{{
 * Stores bcd number bytes in a data block.
 * len is the number of decimal numbers.
 */
PXLIB_API void PXLIB_CALL
PX_put_data_bcd(pxdoc_t *pxdoc, char *data, int len, char *value) {
	unsigned char obuf[17];
	unsigned char sign;
	struct lconv *lc;
	char *dpptr;
	int i, j;

	memset(obuf, 0, 17);
	if(NULL != value) {
		j = 0;
		obuf[0] = 0xC0 + (unsigned char) len;
		sign = 0x00;
		if(value[0] == '-') {
			obuf[0] = 0x40 + (unsigned char) len;
			sign = 0x0f;
			memset(obuf+1, 255, 16);
		} 
#ifdef HAVE_LOCALE_H
		lc = localeconv();
		if(lc)
			dpptr = strchr(value, lc->decimal_point[0]);
		else
#endif
			dpptr = strchr(value, '.');
		if(dpptr) {
			j = dpptr-value+1;
			i = 0;
			while(i<len && value[j] != '\0') {
				char nibble;
				int index;
				nibble = value[j]-48;
				if((nibble >= 0) && (nibble < 10)) {
					index = (34-len+i)/2;
					if((34-len+i)%2)
						obuf[index] = (obuf[index] & 0xf0) | (nibble^sign) ;
					else
						obuf[index] = (obuf[index] & 0x0f) | ((nibble^sign) << 4);
					i++;
				}
				j++;
			}
		} else {
			dpptr = value + len;
		}
		j = dpptr-value-1;
		i = 34-len-1;
		while(i>1 && j>=0) {
			char nibble;
			int index;
			nibble = value[j]-48;
			if(nibble >= 0 && nibble < 10) {
				index = i/2;
				if(i%2)
					obuf[index] = (obuf[index] & 0xf0) | (nibble^sign) ;
				else
					obuf[index] = (obuf[index] & 0x0f) | ((nibble^sign) << 4);
				i--;
			}
			j--;
		}
	}

	memcpy(data, obuf, 17);
}
/* }}} */

/* _px_put_data_blob() {{{
 * Writes data of blob or graphic from memory into a file.
 * The parameter data is a pointer to the position in the data record.
 * len is the space available for the blob in the data record.
 * value is the pointer to the blob with length valuelen.
 * The function determines if the blob data fits into the data record.
 * If it does, it does not need an open blob file.
 */
static int
_px_put_data_blob(pxdoc_t *pxdoc, const char *data, int len, char *value, int valuelen) {
	pxblob_t *pxblob;
	pxstream_t *pxs;
	int leader;

	/* If the (field length - 10) is large enough to hold the blob data,
	 * we don't need to bother writing into the blob file. */
	leader = len - 10;
	if(valuelen > leader) {
		int found;
		pxmbblockinfo_t *blockinfoptr = NULL;
		pxblob = pxdoc->px_blob;
		if(!pxblob || !pxblob->mb_stream) {
			px_error(pxdoc, PX_RuntimeError, _("Paradox database has no blob file."));
			return(-1);
		}

		if((found = px_find_blob_slot(pxblob, valuelen, &blockinfoptr)) > 0) {
		}
		pxs = pxblob->mb_stream;
		if(valuelen > 2048) { /* Block of type 2 */
			TMbBlockHeader2 mbbh;
			int used_blocks;

			/* FIXME: need to add code which reuses free blocks for blocks of type 2*/
//			fprintf(stderr, "Blob goes into type 2 block\n");
			if(pxblob->seek(pxblob, pxs, (pxblob->used_datablocks+1)*4096, SEEK_SET) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not go to the beginning of the first free block in the blob file."));
				return -1;
			}
			/* Calculate how many blocks of 4K this blob will need */
			used_blocks = ((valuelen+sizeof(TMbBlockHeader2)-1) / 4096) + 1;

			/* Fill up the structure that precede the blob in the mb file. */
			mbbh.type = 2;
			put_short_le((char *) &mbbh.numBlocks, used_blocks);
			put_long_le((char *) &mbbh.blobLen, valuelen);
			put_short_le((char *) &mbbh.modNr, ++pxblob->mb_head->modcount);

			/* Write the header of the blob */
			if(pxblob->write(pxblob, pxs, sizeof(TMbBlockHeader2), &mbbh) < 1) {
				px_error(pxdoc, PX_RuntimeError, _("Could not write header of blob data to file."));
				return -1;
			}
			/* Write the blob itself */
			if(pxblob->write(pxblob, pxs, valuelen, value) < 1) {
				px_error(pxdoc, PX_RuntimeError, _("Could not write blob data to file."));
				return -1;
			}
			put_long_le((char *) &data[leader], (pxblob->used_datablocks+1)*4096 + 0xff);
			put_short_le((char *) &data[leader+8], pxblob->mb_head->modcount);
			pxblob->used_datablocks += used_blocks;
		} else { /* Block of type 3 */
			TMbBlockHeader3Table mbbhtab;
			int j;
//			fprintf(stderr, "Blob goes into type 3 block\n");
			if(found > 0) {
				pxblob->subblockoffset = blockinfoptr->number;
				pxblob->subblockblobcount = blockinfoptr->numblobs;
				pxblob->subblockfree = 235-blockinfoptr->allocspace;
			}
			/* Do we have subblock already? Does the block have enough space? */
			if(pxblob->subblockoffset == 0 || (pxblob->subblockblobcount > 63) || ((pxblob->subblockfree*16) < valuelen)) {
				TMbBlockHeader3 mbbh;
				int i, nullint=0;

				if(pxblob->seek(pxblob, pxs, (pxblob->used_datablocks+1)*4096, SEEK_SET) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not go to the beginning of the first free block in the blob file."));
					return -1;
				}

				memset(&mbbh, 0, sizeof(TMbBlockHeader3));
				mbbh.type = 3;
				put_short_le((char *) &mbbh.numBlocks, 1);
				/* Write the header of the blob */
				if(pxblob->write(pxblob, pxs, sizeof(TMbBlockHeader3), &mbbh) < 1) {
					px_error(pxdoc, PX_RuntimeError, _("Could not write header of blob data to file."));
					return -1;
				}
				for(i=0; i<4096-sizeof(TMbBlockHeader3); i++) {
					if(pxblob->write(pxblob, pxs, 1, &nullint) < 1) {
						px_error(pxdoc, PX_RuntimeError, _("Could not write remaining of a type 3 block."));
						return -1;
					}
				}
				pxblob->used_datablocks++;
				pxblob->subblockoffset = pxblob->used_datablocks;
				pxblob->subblockblobcount = 0;
				pxblob->subblockfree = 4096/16 - 21;
			}

			/* At this point a block of type 3 should be available and has enough
			 * space to store the blob data.
			 * First write the table entry pointing to the blob data. The table is
			 * filled from the end to the beginning.
			 * FIXME: The assumption that all table entries are filled starting from
			 * the end isn't true anymore. PX_delete_record() could have left empty
			 * slots in the table. Loop through the index table from the end to the
			 * start and use the first free slot.
			 */
			if(pxblob->seek(pxblob, pxs, (pxblob->subblockoffset)*4096+sizeof(TMbBlockHeader3)+63*sizeof(TMbBlockHeader3Table), SEEK_SET) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not go to last table entry for the blob data."));
				return -1;
			}
			for(j=63; j>=0; j--) {
				if(pxblob->read(pxblob, pxblob->mb_stream, sizeof(TMbBlockHeader3Table), &mbbhtab) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not read entry in index table of type 3 block."));
					return -1;
				}
				if(mbbhtab.offset == 0) {
					if(pxblob->seek(pxblob, pxs, -sizeof(TMbBlockHeader3Table), SEEK_CUR) < 0) {
						px_error(pxdoc, PX_RuntimeError, _("Could not go to table entry for the blob data."));
						return -1;
					}
					break;
				}
				if(pxblob->seek(pxblob, pxs, -2*sizeof(TMbBlockHeader3Table), SEEK_CUR) < 0) {
					px_error(pxdoc, PX_RuntimeError, _("Could not go to table entry for the blob data."));
					return -1;
				}
			}
			mbbhtab.offset = (4096/16)-pxblob->subblockfree; /* offset/16 to blob data */
			mbbhtab.length = (valuelen-1)/16 + 1;

			/* FIXME: Using subblockblobcount is probably not sufficient. It
			 * maybe a counter over the whole file and not just the block.
			 * Uwe 17.12.2004: Tried (pxblob->mb_head->modcount+1) instead of
			 * (pxblob->subblockblobcount+1)
			 */
			put_short_le((char *) &mbbhtab.modNr, pxblob->mb_head->modcount+1);
			mbbhtab.lengthmod = (valuelen % 16) == 0 ? 16 : (valuelen % 16);
			/* Write the blob table entry */
			if(pxblob->write(pxblob, pxs, sizeof(TMbBlockHeader3Table), &mbbhtab) < 1) {
				px_error(pxdoc, PX_RuntimeError, _("Could not write table entry for blob data to file."));
				return -1;
			}
			/* Write the blob itself */
			if(pxblob->seek(pxblob, pxs, pxblob->subblockoffset*4096+mbbhtab.offset*16, SEEK_SET) < 0) {
				px_error(pxdoc, PX_RuntimeError, _("Could not go to the beginning of the slot for the blob."));
				return -1;
			}
			if(pxblob->write(pxblob, pxs, valuelen, value) < 1) {
				px_error(pxdoc, PX_RuntimeError, _("Could not write blob data to file."));
				return -1;
			}
			pxblob->subblockfree -= mbbhtab.length;
			pxblob->subblockblobcount++;

			put_long_le((char *) &data[leader], (pxblob->subblockoffset)*4096 + j);
			put_short_le((char *) &data[leader+8], ++pxblob->mb_head->modcount);
		}
	} else { /* blob fits in db file */
		put_long_le((char *) &data[leader], 0);
		put_short_le((char *) &data[leader+8], 0);
	}
	put_long_le((char *) &data[leader+4], valuelen);

	/* Write the info about the blob into the db file.
	 * The field value always starts with the blob data followed by a
	 * 10 Byte section. */
	if(leader) {
		if(leader <= valuelen)
			memcpy((char *) data, value, leader);
		else
			memcpy((char *) data, value, valuelen);
	}
	return 0;
}
/* }}} */

/* PX_put_data_blob() {{{
 * Stores bcd number bytes in a data block.
 * len is the number of decimal numbers.
 */
PXLIB_API int PXLIB_CALL
PX_put_data_blob(pxdoc_t *pxdoc, char *data, int len, char *value, int valuelen) {
	return(_px_put_data_blob(pxdoc, data, len, value, valuelen));
}
/* }}} */

/******* Function for data formating ******/

/* PX_make_time() {{{
 * Creates a value representing the internal paradox format of a time.
 * The return value can written with PX_put_data_long() into the record.
 */
PXLIB_API pxval_t* PXLIB_CALL
PX_make_time(pxdoc_t *pxdoc, int hour, int minute, int second) {
	pxval_t *pxval;
	MAKE_PXVAL(pxdoc, pxval);
	pxval->value.lval = hour*3600000 + minute*60000 + second*1000;
	return(pxval);
}
/* }}} */

/* PX_make_date() {{{
 * Creates a value representing the internal paradox format of a date.
 * The return value can written with PX_put_data_long() into the record.
 */
PXLIB_API pxval_t* PXLIB_CALL
PX_make_date(pxdoc_t *pxdoc, int year, int month, int day) {
	pxval_t *pxval;
	int value;
	MAKE_PXVAL(pxdoc, pxval);
	if(0 != (value = PX_GregorianToSdn(year, month, day))) {
		pxval->value.lval = value-1721425;
	} else {
		pxval->isnull = 1;
	}
	return(pxval);
}
/* }}} */

/* PX_make_timestamp() {{{
 * Creates a value representing the internal paradox format of a timestamp.
 * The return value can written with PX_put_data_double() into the record.
 */
PXLIB_API pxval_t* PXLIB_CALL
PX_make_timestamp(pxdoc_t *pxdoc, int year, int month, int day, int hour, int minute, int second) {
	double value;
	pxval_t *pxval;
	MAKE_PXVAL(pxdoc, pxval);
	if(0.0 != (value = (double) PX_GregorianToSdn(year, month, day))) {

		pxval->value.dval = ((value - 1721425.0) * 86400.0 + hour * 3600 + minute * 60 + second) * 1000.0;
	} else {
		pxval->isnull = 1;
	}
	return(pxval);
}
/* }}} */

#define isleap(year) ((((year) % 4) == 0 && ((year) % 100) != 0) || ((year) % 400)==0)
/* PX_timestamp2string() {{{
 * Converts a timestamp as stored in the paradox database into
 * a string as specified by the format string.
 */
PXLIB_API char * PXLIB_CALL
PX_timestamp2string(pxdoc_t *pxdoc, double value, const char *format) {
	char tmp_buff[32], *str;
	int i, size=0;
	struct tm ta;
	int secs, days;

	value = value / 1000.0;
	days = (int) (value / 86400);
	secs = (int) fmod(value, 86400);
	PX_SdnToGregorian(days+1721425, &ta.tm_year, &ta.tm_mon, &ta.tm_mday);
	ta.tm_mon--;
	ta.tm_hour = secs/3600;
	ta.tm_min = secs/60%60;
	ta.tm_sec = secs%60;

	for (i = 0; i < strlen(format); i++) {
		switch(format[i]) {
			case 'Y':       /* year, numeric, 4 digits */
				size += 6;  /* Allow years to be 6 digits long */
				break;
			case 'y':		/* year, numeric, 2 digits */
			case 'm':		/* month, numeric */
			case 'n':		/* month, numeric, no leading zeroes */
			case 'd':		/* day of the month, numeric */
			case 'j':		/* day of the month, numeric, no leading zeros */
			case 'H':		/* hour, numeric, 24 hour format */
			case 'h':		/* hour, numeric, 12 hour format */
			case 'G':		/* hour, numeric, 24 hour format, no leading zeroes */
			case 'g':		/* hour, numeric, 12 hour format, no leading zeroes */
			case 'i':		/* minutes, numeric */
			case 's':		/* seconds, numeric */
			case 'A':		/* AM/PM */
			case 'a':		/* am/pm */
			case 'S':		/* standard english suffix for the day of the month (e.g. 3rd, 2nd, etc) */
				size += 2;
				break;
			case 'L':		/* boolean for leap year */
				size++;
				break;
			default:
				size++;
				break;

		}
	}
	if(NULL == (str = pxdoc->malloc(pxdoc, size+1, _("Allocate memory for timestamp string.")))) {
		px_error(pxdoc, PX_MemoryError, _("Could not allocate memory for timestamp string."));
		return NULL;
	}
	str[0] = '\0';
	for (i = 0; i < strlen(format); i++) {
		switch(format[i]) {
			case '\\':
				if (i < strlen(format) - 1) {
					char ch[2];
					ch[0] = format[++i];
					ch[1] = '\0';
					strcat(str, ch);
				}
			case 'Y':		/* year, numeric, 4 digits */
				sprintf(tmp_buff, "%04d", ta.tm_year);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'z':		/* day (of the year) */
				sprintf(tmp_buff, "%d", ta.tm_yday);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'y':		/* year, numeric, 2 digits */
				sprintf(tmp_buff, "%02d", ((ta.tm_year)%100));  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'm':		/* month, numeric */
				sprintf(tmp_buff, "%02d", ta.tm_mon + 1);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'n':      /* month, numeric, no leading zeros */
				sprintf(tmp_buff, "%d", ta.tm_mon + 1);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'd':		/* day of the month, numeric */
				sprintf(tmp_buff, "%02d", ta.tm_mday);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'j':
				sprintf(tmp_buff, "%d", ta.tm_mday); /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'H':		/* hour, numeric, 24 hour format */
				sprintf(tmp_buff, "%02d", ta.tm_hour);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'h': {		/* hour, numeric, 12 hour format */
				int h = ta.tm_hour % 12; if (h==0) h = 12;
				sprintf(tmp_buff, "%02d", h);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			}
			case 'G':      /* hour, numeric, 24 hour format, no leading zeros */
				sprintf(tmp_buff, "%d", ta.tm_hour);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'g': {     /* hour, numeric, 12 hour format, no leading zeros */
				int h = ta.tm_hour % 12; if (h==0) h = 12;
				sprintf(tmp_buff, "%d", h);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			}
			case 'i':		/* minutes, numeric */
				sprintf(tmp_buff, "%02d", ta.tm_min);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 's':		/* seconds, numeric */
				sprintf(tmp_buff, "%02d", ta.tm_sec);  /* SAFE */
				strcat(str, tmp_buff);
				break;
			case 'A':		/* AM/PM */
				strcat(str, (ta.tm_hour >= 12 ? "PM" : "AM"));
				break;
			case 'a':		/* am/pm */
				strcat(str, (ta.tm_hour >= 12 ? "pm" : "am"));
				break;
			case 'S':		/* standard english suffix, e.g. 2nd/3rd for the day of the month */
				if (ta.tm_mday >= 10 && ta.tm_mday <= 19) {
					strcat(str, "th");
				} else {
					switch (ta.tm_mday % 10) {
						case 1:
							strcat(str, "st");
							break;
						case 2:
							strcat(str, "nd");
							break;
						case 3:
							strcat(str, "rd");
							break;
						default:
							strcat(str, "th");
							break;
					}
				}
				break;
			case 'L':		/* boolean for leapyear */
				sprintf(tmp_buff, "%d", (isleap((ta.tm_year)) ? 1 : 0 ) );
				strcat(str, tmp_buff);
				break;
			default: {
				char ch[2];
				ch[0] = format[i];
				ch[1] = '\0';
				strcat(str, ch);
			}
		}
	}
	return(str);
}
/* }}} */

/* PX_date2string() {{{
 * Converts a date as stored in the paradox database into
 * a string as specified by the format string.
 */
PXLIB_API char * PXLIB_CALL
PX_date2string(pxdoc_t *pxdoc, long value, const char *format) {
	return(PX_timestamp2string(pxdoc, value*1000.0*86400.0, format));
}
/* }}} */

/* PX_time2string() {{{
 * Converts a time as stored in the paradox database into
 * a string as specified by the format string.
 */
PXLIB_API char * PXLIB_CALL
PX_time2string(pxdoc_t *pxdoc, long value, const char *format) {
	return(PX_timestamp2string(pxdoc, (double) value, format));
}
/* }}} */

/******* Function for memory management ******/

/* PX_strdup() {{{
 * Same as strdup but uses the memory management functions of
 * the paradox document as set with PX_new2()
 */
PXLIB_API char * PXLIB_CALL
PX_strdup(pxdoc_t *pxdoc, const char *str) {
	return(px_strdup(pxdoc, str));
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
