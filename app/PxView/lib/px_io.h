#ifndef __PX_IO_H__
#define __PX_IO_H__
pxstream_t *px_stream_new(pxdoc_t *pxdoc);
#if HAVE_GSF
pxstream_t *px_stream_new_gsf(pxdoc_t *pxdoc, int mode, int close, GsfInput *gsf);
#endif
pxstream_t *px_stream_new_file(pxdoc_t *pxdoc, int mode, int close, FILE *fp);

ssize_t px_read(pxdoc_t *p, pxstream_t *dummy, size_t len, void *buffer);
int px_seek(pxdoc_t *p, pxstream_t *dummy, long offset, int whence);
long px_tell(pxdoc_t *p, pxstream_t *dummy);
ssize_t px_write(pxdoc_t *p, pxstream_t *dummy, size_t len, void *buffer);
int px_flush(pxdoc_t *p, pxstream_t *dummy);

ssize_t px_mb_read(pxblob_t *p, pxstream_t *dummy, size_t len, void *buffer);
int px_mb_seek(pxblob_t *p, pxstream_t *dummy, long offset, int whence);
long px_mb_tell(pxblob_t *p, pxstream_t *dummy);
ssize_t px_mb_write(pxblob_t *p, pxstream_t *dummy, size_t len, void *buffer);

ssize_t px_fread(pxdoc_t *p, pxstream_t *stream, size_t len, void *buffer);
int px_fseek(pxdoc_t *p, pxstream_t *stream, long offset, int whence);
long px_ftell(pxdoc_t *p, pxstream_t *stream);
ssize_t px_fwrite(pxdoc_t *p, pxstream_t *stream, size_t len, void *buffer);

#ifdef HAVE_GSF
ssize_t px_gsfread(pxdoc_t *p, pxstream_t *stream, size_t len, void *buffer);
int px_gsfseek(pxdoc_t *p, pxstream_t *stream, long offset, int whence);
long px_gsftell(pxdoc_t *p, pxstream_t *stream);
ssize_t px_gsfwrite(pxdoc_t *p, pxstream_t *stream, size_t len, void *buffer);
#endif

#endif
