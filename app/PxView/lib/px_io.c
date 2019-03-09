#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "px_intern.h"
#include "paradox-gsf.h"
#include "px_error.h"
#include "px_crypt.h"
#include "px_io.h"

/* px_stream_new() {{{
 *
 * Create a new stream
 */
pxstream_t *px_stream_new(pxdoc_t *pxdoc) {
	pxstream_t *pxs;
	if(pxdoc == NULL) {
		px_error(pxdoc, PX_RuntimeError, _("Did not pass a paradox database."));
		return NULL;
	}

	if(NULL == (pxs = pxdoc->malloc(pxdoc, sizeof(pxstream_t), _("Allocate memory for io stream.")))) {
		px_error(pxdoc, PX_MemoryError, _("Could not allocate memory for io stream."));
		return NULL;
	}
	
	return(pxs);
}
/* }}} */

#if HAVE_GSF
/* px_stream_new_gsf() {{{
 *
 * Create a gsf stream
 */
pxstream_t *px_stream_new_gsf(pxdoc_t *pxdoc, int mode, int close, GsfInput *gsf) {
	pxstream_t *pxs;

	if(NULL == (pxs = px_stream_new(pxdoc)))
		return(NULL);
	
	pxs->type = pxfIOGsf;
	pxs->mode = mode;
	pxs->close = close;
	pxs->s.gsfin = gsf;

	pxs->read = px_gsfread;
	pxs->seek = px_gsfseek;
	pxs->tell = px_gsftell;
	pxs->write = px_gsfwrite;
	return(pxs);
}
/* }}} */
#endif

/* px_stream_new_file() {{{
 *
 * Create a file stream
 */
pxstream_t *px_stream_new_file(pxdoc_t *pxdoc, int mode, int close, FILE *fp) {
	pxstream_t *pxs;

	if(NULL == (pxs = px_stream_new(pxdoc)))
		return(NULL);

	pxs->type = pxfIOFile;
	pxs->mode = mode;
	pxs->close = close;
	pxs->s.fp = fp;

	pxs->read = px_fread;
	pxs->seek = px_fseek;
	pxs->tell = px_ftell;
	pxs->write = px_fwrite;
	return(pxs);
}
/* }}} */

/* Generic file access functions for .db and .px files */
/* px_read() {{{
 *
 * Generic read function doing decryption if needed.
 * It calls the read function from px_stream_t to actually get the
 * file data.
 */
ssize_t px_read(pxdoc_t *p, pxstream_t *dummy, size_t len, void *buffer) {
	size_t ret;
	long blocknr, blockpos, curpos, blocksize;
	pxhead_t *pxh;
	pxstream_t *pxs;

	pxh = p->px_head;
	pxs = p->px_stream;

	curpos = pxs->tell(p, pxs);
	if(pxh != NULL && curpos >= pxh->px_headersize) {
		blocksize = pxh->px_maxtablesize * 0x400;
		blocknr = ((curpos - pxh->px_headersize) / blocksize) + 1;
		blockpos = (curpos - pxh->px_headersize) % blocksize;
//		fprintf(stderr, "reading from block %d:%d\n", blocknr, blockpos);
		if(blockpos+len > blocksize) {
			px_error(p, PX_RuntimeError, _("Trying to read data from file exceeds block boundary."));
			return(0);
		}
		if(p->curblock == NULL) {
//			fprintf(stderr, "Allocate memory for cache block.\n");
			p->curblock = p->malloc(p, blocksize, _("Allocate memory for block cache."));
			if(p->curblock == NULL) {
				return(0);
			}
			
		}
		if(p->curblocknr != blocknr) {
//			fprintf(stderr, "Read block %d into cache.\n", blocknr);
			if(p->curblockdirty == px_true) {
				pxs->seek(p, pxs, pxh->px_headersize + ((p->curblocknr-1)*blocksize), SEEK_SET);
				if(pxh->px_encryption != 0) {
	//				fprintf(stderr, "Encrypting block %d\n", p->curblocknr);
					px_encrypt_db_block(p->curblock, p->curblock, pxh->px_encryption, blocksize, p->curblocknr);
				}
				pxs->write(p, pxs, blocksize, p->curblock);
			}
			memset(p->curblock, 0, blocksize);
			pxs->seek(p, pxs, pxh->px_headersize + ((blocknr-1)*blocksize), SEEK_SET);
			pxs->read(p, pxs, blocksize, p->curblock);
			p->curblocknr = blocknr;
			if(pxh->px_encryption != 0) {
//				fprintf(stderr, "Decrypting block %d\n", blocknr);
				px_decrypt_db_block(p->curblock, p->curblock, pxh->px_encryption, blocksize, blocknr);
			}
		} else {
//			fprintf(stderr, "block %d already in cache.\n", blocknr);
		}
		memcpy(buffer, p->curblock+blockpos, len);
		pxs->seek(p, pxs, curpos+len, SEEK_SET);
		ret = len;
	} else {
		ret = pxs->read(p, pxs, len, buffer);
	}
	return(ret);
}
/* }}} */

/* px_seek() {{{
 */
int px_seek(pxdoc_t *p, pxstream_t *dummy, long offset, int whence) {
	return(p->px_stream->seek(p, p->px_stream, offset, whence));
}
/* }}} */

/* px_tell() {{{
 */
long px_tell(pxdoc_t *p, pxstream_t *dummy) {
	return(p->px_stream->tell(p, p->px_stream));
}
/* }}} */

/* px_write() {{{
 */
ssize_t px_write(pxdoc_t *p, pxstream_t *dummy, size_t len, void *buffer) {
	size_t ret;
	long blocknr, blockpos, curpos, blocksize;
	pxhead_t *pxh;
	pxstream_t *pxs;

	pxh = p->px_head;
	pxs = p->px_stream;
	curpos = pxs->tell(p, pxs);
	if(pxh != NULL && curpos >= pxh->px_headersize) {
		blocksize = pxh->px_maxtablesize * 0x400;
		blocknr = ((curpos - pxh->px_headersize) / blocksize) + 1;
		blockpos = (curpos - pxh->px_headersize) % blocksize; 
//		fprintf(stderr, "writing to block %d:%d\n", blocknr, blockpos);
		if(blockpos+len > blocksize) {
			px_error(p, PX_RuntimeError, _("Trying to write data to file exceeds block boundary: %d + %d > %d."), blockpos, len, blocksize);
			return(0);
		}
		if(p->curblock == NULL) {
//			fprintf(stderr, "Allocate memory for cache block.\n");
			p->curblock = p->malloc(p, blocksize, _("Allocate memory for block cache."));
			if(p->curblock == NULL) {
				return(0);
			}
		}
		/* Write last accessed block to disk if the write operation modifies
		 * a new block.
		 * No need to write, if this is the first time a write operation
		 * modifies a block. Blocks will be written to the file, when
		 * a new block is accessed.
		 */
		if(p->curblocknr != blocknr && p->curblocknr != 0) {
//			fprintf(stderr, "Write block %d from cache into file.\n", p->curblocknr);
			if(p->curblockdirty == px_true) {
				pxs->seek(p, pxs, pxh->px_headersize + ((p->curblocknr-1)*blocksize), SEEK_SET);
				if(pxh->px_encryption != 0) {
	//				fprintf(stderr, "Encrypting block %d\n", p->curblocknr);
					px_encrypt_db_block(p->curblock, p->curblock, pxh->px_encryption, blocksize, p->curblocknr);
				}
				pxs->write(p, pxs, blocksize, p->curblock);
			}
			memset(p->curblock, 0, blocksize);
			/* Read the new block, just in case it has been in the file already */
			pxs->seek(p, pxs, pxh->px_headersize + ((blocknr-1)*blocksize), SEEK_SET);
			pxs->read(p, pxs, blocksize, p->curblock);
			if(pxh->px_encryption != 0) {
				px_decrypt_db_block(p->curblock, p->curblock, pxh->px_encryption, blocksize, blocknr);
			}
		} else {
//			fprintf(stderr, "block %d already in cache.\n", blocknr);
		}
		p->curblocknr = blocknr;
		p->curblockdirty = px_true;
		memcpy(p->curblock+blockpos, buffer, len);
		pxs->seek(p, pxs, curpos+len, SEEK_SET);
		ret = len;
	} else {
		ret = pxs->write(p, pxs, len, buffer);
	}
	return(ret);
}
/* }}} */

/* px_flush() {{{
 */
int px_flush(pxdoc_t *p, pxstream_t *dummy) {
	long blocksize;
	pxhead_t *pxh;
	pxstream_t *pxs;

	pxh = p->px_head;
	pxs = p->px_stream;
	if(pxh != NULL) {
		blocksize = pxh->px_maxtablesize * 0x400;
		if(p->curblockdirty) {
//			fprintf(stderr, "Write block %d from cache into file.\n", p->curblocknr);
			pxs->seek(p, pxs, pxh->px_headersize + ((p->curblocknr-1)*blocksize), SEEK_SET);
			if(pxh->px_encryption != 0) {
//				fprintf(stderr, "Encrypting block %d\n", p->curblocknr);
				px_encrypt_db_block(p->curblock, p->curblock, pxh->px_encryption, blocksize, p->curblocknr);
			}
			pxs->write(p, pxs, blocksize, p->curblock);
			p->curblockdirty = px_false;
		}
	}
	return(0);
}
/* }}} */

/* Generic file access functions for .mb */
/* px_mb_read() {{{
 *
 * Generic read function doing decryption if needed.
 * It calls the read function from px_stream_t to actually get the
 * file data.
 */
#define BLOCKSIZEEXP 8 /* Each encrypted block has 2^BLOCKSIZEEXP bytes */
ssize_t px_mb_read(pxblob_t *p, pxstream_t *dummy, size_t len, void *buffer) {
	pxdoc_t *pxdoc;
	pxhead_t *pxh;
	pxstream_t *pxs;
	long pos;
	int ret;
	unsigned char *tmpbuf = NULL;
	unsigned int blockslen, blockoffset;

	pxdoc = p->pxdoc;
	pxh = pxdoc->px_head;
	pxs = p->mb_stream;

	if (pxh->px_encryption == 0)
		return pxs->read(pxdoc, pxs, len, buffer);

	pos = pxs->tell(pxdoc, pxs);
	if (pos < 0) {
		return pos;
	}

	/* pos can be in the middle of a 2^BLOCKSIZEEXP bytes block.
	 * Make sure we start reading at the beginning of the block.
	 */
	blockoffset = (pos >> BLOCKSIZEEXP) << BLOCKSIZEEXP;
	/* We need to read at least chunk from the blockoffset till the
	 * desired postion and the data itself which has len bytes.
	 * e.g. if we want to read 20 bytes starting at position 300 in the
	 * file, we will need to read 44+20 bytes starting at position 256. 
	 */
	blockslen = len + pos - blockoffset;
	/* Check if the end of the data is within a 2^BLOCKSIZEEXP bytes block.
	 * If that is the case, we will need to read the remainder of the
	 * 2^BLOCKSIZEEXP bytes block as well. In the above example, we
	 * will have to read 256 bytes instead of just 64.
	 */
	if(blockslen & 0xff)
		blockslen = ((blockslen >> BLOCKSIZEEXP) + 1) << BLOCKSIZEEXP;

	assert(blockslen >= len);
	assert(blockoffset <= (unsigned long)pos);
	assert((blockoffset+blockslen) >= (pos+len));

	ret = pxs->seek(pxdoc, pxs, blockoffset, SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	if(NULL == p->blockcache.data) {
		p->blockcache.data = (unsigned char *) malloc(blockslen);
	} else {
		if(blockoffset == p->blockcache.start && blockslen <= p->blockcache.size) {
//			fprintf(stderr, "Reading block at position 0x%X from cache.\n", blockoffset);
			memcpy(buffer, p->blockcache.data + (pos - blockoffset), len);
			ret = pxs->seek(pxdoc, pxs, pos + len, SEEK_SET);
			if (ret < 0) {
				return ret;
			}
			return len;
		}
		p->blockcache.data = (unsigned char *) realloc(p->blockcache.data, blockslen);
	}
	if (p->blockcache.data == NULL) {
		return -ENOMEM;
	}
//	fprintf(stderr, "Reading block at position 0x%X from file.\n", blockoffset);
	tmpbuf = p->blockcache.data;

	ret = pxs->read(pxdoc, pxs, blockslen, tmpbuf);
	if (ret <= 0) {
		free(tmpbuf);
		p->blockcache.data = NULL;
		return ret;
	}
	px_decrypt_mb_block(tmpbuf, tmpbuf, pxh->px_encryption, blockslen);
	memcpy(buffer, tmpbuf + (pos - blockoffset), len);
	p->blockcache.start = blockoffset;
	p->blockcache.size = blockslen;
//	free(tmpbuf);

	ret = pxs->seek(pxdoc, pxs, pos + len, SEEK_SET);
	if (ret < 0) {
		return ret;
	}

	return len;

}
/* }}} */

/* px_mb_seek() {{{
 */
int px_mb_seek(pxblob_t *p, pxstream_t *dummy, long offset, int whence) {
	return(p->mb_stream->seek(p->pxdoc, p->mb_stream, offset, whence));
}
/* }}} */

/* px_mb_tell() {{{
 */
long px_mb_tell(pxblob_t *p, pxstream_t *dummy) {
	return(p->mb_stream->tell(p->pxdoc, p->mb_stream));
}
/* }}} */

/* px_mb_write() {{{
 */
ssize_t px_mb_write(pxblob_t *p, pxstream_t *dummy, size_t len, void *buffer) {
	return(p->mb_stream->write(p->pxdoc, p->mb_stream, len, buffer));
}
/* }}} */

/* regular file pointer */
/* px_fread() {{{
 */
ssize_t px_fread(pxdoc_t *p, pxstream_t *stream, size_t len, void *buffer) {
	return(fread(buffer, 1, len, stream->s.fp));
}
/* }}} */

/* px_fseek() {{{
 */
int px_fseek(pxdoc_t *p, pxstream_t *stream, long offset, int whence) {
	return(fseek(stream->s.fp, offset, whence));
}
/* }}} */

/* px_ftell() {{{
 */
long px_ftell(pxdoc_t *p, pxstream_t *stream) {
	return(ftell(stream->s.fp));
}
/* }}} */

/* px_fwrite() {{{
 */
ssize_t px_fwrite(pxdoc_t *p, pxstream_t *stream, size_t len, void *buffer) {
	return(fwrite(buffer, 1, len, stream->s.fp));
}
/* }}} */

/* gsf */
#if HAVE_GSF
/* px_gsfread() {{{
 */
ssize_t px_gsfread(pxdoc_t *p, pxstream_t *stream, size_t len, void *buffer) {
	return((int) gsf_input_read(stream->s.gsfin, len, buffer));
}
/* }}} */

/* px_gsfseek() {{{
 */
int px_gsfseek(pxdoc_t *p, pxstream_t *stream, long offset, int whence) {
	GSeekType gsfwhence = G_SEEK_SET;

	switch(whence) {
		case SEEK_CUR: gsfwhence = G_SEEK_CUR; break;
		case SEEK_END: gsfwhence = G_SEEK_END; break;
		case SEEK_SET: gsfwhence = G_SEEK_SET; break;
	}
	return(gsf_input_seek(stream->s.gsfin, offset, gsfwhence));
}
/* }}} */

/* px_gsftell() {{{
 */
long px_gsftell(pxdoc_t *p, pxstream_t *stream) {
	return(gsf_input_tell(stream->s.gsfin));
}
/* }}} */

/* px_gsfwrite() {{{
 */
ssize_t px_gsfwrite(pxdoc_t *p, pxstream_t *stream, size_t len, void *buffer) {
	return(gsf_output_write(stream->s.gsfout, len, buffer));
}
/* }}} */
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
