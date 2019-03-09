#ifndef __PARADOX_H__
#define __PARADOX_H__

#define PX_USE_RECODE 0
#define PX_USE_ICONV 0

#include <stdio.h>
#if PX_USE_RECODE
#include <recode.h>
#else
#if PX_USE_ICONV
#include <iconv.h>
#endif
#endif

#ifdef WIN32

#define PXLIB_CALL __cdecl

#ifdef PXLIB_EXPORTS
#define PXLIB_API __declspec(dllexport) /* prepare a DLL (internal use only) */
#elif defined(PXLIB_DLL)
#define PXLIB_API __declspec(dllimport) /* PXlib clients: import PXlib DLL */
#else /* !PXLIB_DLL */
#define PXLIB_API /* */  /* default: generate or use static library */

#endif  /* !PXLIB_DLL */

#endif /* !WIN32 */

#ifndef PXLIB_CALL
#define PXLIB_CALL
#endif
#ifndef PXLIB_API
#define PXLIB_API
#endif

#define px_true 1
#define px_false 0

/* Error codes */
#define PX_MemoryError 1
#define PX_IOError 2
#define PX_RuntimeError 3
#define PX_Warning 100

/* IO Stream types */
#define pxfIOFile 1
/* pxfIOGsf is defined as 2 in paradox-gsf.h */
#define pxfIOStream 3

/* Field types */
#define pxfAlpha        0x01
#define pxfDate         0x02
#define pxfShort        0x03
#define pxfLong         0x04
#define pxfCurrency     0x05
#define pxfNumber       0x06
#define pxfLogical      0x09
#define pxfMemoBLOb     0x0C
#define pxfBLOb         0x0D
#define pxfFmtMemoBLOb  0x0E
#define pxfOLE          0x0F
#define pxfGraphic      0x10
#define pxfTime         0x14
#define pxfTimestamp    0x15
#define pxfAutoInc      0x16
#define pxfBCD          0x17
#define pxfBytes        0x18
#define pxfNumTypes     0x18

/* File types */
#define pxfFileTypIndexDB         0   /* .DB */
#define pxfFileTypPrimIndex       1   /* .PX */
#define pxfFileTypNonIndexDB      2   /* .DB */
#define pxfFileTypNonIncSecIndex  3   /* .Xnn */
#define pxfFileTypSecIndex        4   /* .Ynn */
#define pxfFileTypIncSecIndex     5   /* .Xnn */
#define pxfFileTypNonIncSecIndexG 6   /* .XGn */
#define pxfFileTypSecIndexG       7   /* .YGn */
#define pxfFileTypIncSecIndexG    8   /* .XGn */

/* File modes */
#define pxfFileRead   0x1
#define pxfFileWrite  0x2

struct px_field {
	char *px_fname;
	char px_ftype;
	int px_flen;
	int px_fdc;
};

struct px_val {
	char isnull;
	int type;
	union {
		long lval;
		double dval;
		struct {
			char *val;
			int len;
		} str;
	} value;
};

struct px_head {
	char *px_tablename;
	int px_recordsize;
	char px_filetype;
	int px_fileversion;
	int px_numrecords;           // number of records as in header
	int px_theonumrecords;       // number of records that fit in data blocks
	int px_numfields;
	int px_maxtablesize;
	int px_headersize;
	unsigned int px_fileblocks;
	unsigned int px_firstblock;  // block number of first block
	unsigned int px_lastblock;   // block number of last block
	int px_indexfieldnumber;
	int px_indexroot;            // number of root index block (only px files)
	int px_numindexlevels;       // number of index levels (only px files)
	int px_writeprotected;
	int px_doscodepage;
	int px_primarykeyfields;
	char px_modifiedflags1;
	char px_modifiedflags2;
	char px_sortorder;
	int px_autoinc;
	int px_fileupdatetime;
	char px_refintegrity;
	struct px_field *px_fields;
	unsigned long px_encryption;
};

typedef struct px_doc pxdoc_t;
typedef struct px_datablockinfo pxdatablockinfo_t;
typedef struct px_blob pxblob_t;
typedef struct px_head pxhead_t;
typedef struct px_field pxfield_t;
typedef struct px_pindex pxpindex_t;
typedef struct px_stream pxstream_t;
typedef struct px_val pxval_t;
typedef struct mb_head mbhead_t;

struct px_stream {
	int type;        /* set to pxfIOFile | pxfIOGsf | pxfIOStream*/
	int mode;        /* set to pxfFileRead | pxfFileWrite */
	int close;       /* set to true if stream must be closed */
	union {
		FILE *fp;
		void *stream;
#if HAVE_GSF
		GsfInput *gsfin;
		GsfOutput *gsfout;
#endif
	} s;
	ssize_t (*read)(pxdoc_t *p, pxstream_t *stream, size_t numbytes, void *buffer);
	int (*seek)(pxdoc_t *p, pxstream_t *stream, long offset, int whence);
	long (*tell)(pxdoc_t *p, pxstream_t *stream);
	ssize_t (*write)(pxdoc_t *p, pxstream_t *stream, size_t numbytes, void *buffer);
};

struct px_doc {
	/* database file */
//	FILE *px_fp;       /* File pointer of file */
	pxstream_t *px_stream; /* input stream to read file from */
	char *px_name;     /* Name of file */
	int px_close_fp;   /* set to true if file pointer must be closed, deprecated */
//	int px_filemode;   /* set to pxfFileRead|pxfFileWrite */
	pxhead_t *px_head; /* Pointer to header of file */
	void *px_data;     /* Pointer to data of file (used in prim. index for
						* index data, used in db files for an self build
						* index) */
	int px_datalen;    /* length of data field in number of units */
	void *px_indexdata;/* Pointer to index data */
	int px_indexdatalen; /* number of index data records */

	/* primary index file */
	pxdoc_t *px_pindex;

	/* blob file */
	pxblob_t *px_blob;

	/* record accounting */
	int last_position; /* Position (starting at 0) where last record was put. */

	int warnings;      /* Turn of/off output of warnings */

	/* output function */
	size_t (*writeproc)(pxdoc_t *p, void *data, size_t size);

	/* error handler function */
	void (*errorhandler)(pxdoc_t *p, int level, const char* msg, void *data);

	/* user data passed to error handler */
	void *errorhandler_user_data;

	/* Memory allocation functions */
	void *(*malloc)(pxdoc_t *p, size_t size, const char *caller);
	void *(*calloc)(pxdoc_t *p, size_t size, const char *caller);
	void *(*realloc)(pxdoc_t *p, void *mem, size_t size, const char *caller);
	void  (*free)(pxdoc_t *p, void *mem);

	/* input stream functions */
	ssize_t (*read)(pxdoc_t *p, pxstream_t *stream, size_t numbytes, void *buffer);
	int (*seek)(pxdoc_t *p, pxstream_t *stream, long offset, int whence);
	long (*tell)(pxdoc_t *p, pxstream_t *stream);
	ssize_t (*write)(pxdoc_t *p, pxstream_t *stream, size_t numbytes, void *buffer);

	char *targetencoding;
	char *inputencoding;
#if PX_USE_RECODE
	RECODE_OUTER recode_outer;
	RECODE_REQUEST out_recode_request; /* Encoding of written data */
	RECODE_REQUEST in_recode_request; /* Encoding of read data */
#else
#if PX_USE_ICONV
	iconv_t out_iconvcd;  /* Encoding of written data */
	iconv_t in_iconvcd;   /* Encoding of read data */
#endif
#endif

	long curblocknr;      /* Number of current block in cache (0-n) */
	int curblockdirty;    /* Set to px_true if the block needs to be written */
	unsigned char *curblock;       /* Data of block in read cache */
};

struct px_blockcache {
	long start;
	size_t size;
	unsigned char *data;
};
typedef struct px_blockcache pxblockcache_t;

struct px_mbblockinfo {
	int number;
	char type;
	char numblobs;
	int numblocks;
	int allocspace;
};
typedef struct px_mbblockinfo pxmbblockinfo_t;

struct px_blob {
	char *mb_name;
	pxdoc_t *pxdoc;
	pxstream_t *mb_stream; /* input stream to read file from */
	mbhead_t *mb_head; /* Pointer to header of file */
	int used_datablocks;
	int subblockoffset;
	int subblockinneroffset;
	int subblockfree;
	int subblockblobcount;
	/* input stream functions */
	ssize_t (*read)(pxblob_t *p, pxstream_t *stream, size_t numbytes, void *buffer);
	int (*seek)(pxblob_t *p, pxstream_t *stream, long offset, int whence);
	long (*tell)(pxblob_t *p, pxstream_t *stream);
	ssize_t (*write)(pxblob_t *p, pxstream_t *stream, size_t numbytes, void *buffer);
	/* Cache for the last read block */
	pxblockcache_t blockcache;
	/* Index of all blocks in the blob file */
	pxmbblockinfo_t *blocklist;
	int blocklistlen;
};

struct mb_head {
	int modcount;
};

struct px_datablockinfo {
	long blockpos;     /* the start of the block in the file */
	long recordpos;    /* the start of the record in the file */
	int size;          /* the size of the block */
	int recno;         /* the recno with the block starting with 0 */
	int numrecords;    /* the number of records in the block */
	int prev;          /* the number of the previous block */
	int next;          /* the number of the next block */
	int number;        /* the block number count (first block is 1) */
};

struct px_pindex {
	char *data;
	int blocknumber;   /* Block number of referenced block */
	int numrecords;
	int dummy;
	int myblocknumber; /* Number of block this record is stored in */
	int level;         /* level of index block, level 1 blocks point to
						* data block in the database file. */
};

#define MAKE_PXVAL(pxdoc, pxval) \
	(pxval) = (pxval_t *) (pxdoc)->malloc((pxdoc), sizeof(pxval_t), "Allocate memory for pxval_t"); \
	memset((void *) (pxval), 0, sizeof(pxval_t));

#define FREE_PXVAL(pxdoc, pxval) \
	(pxdoc)->free((pxdoc), (pxval));

PXLIB_API int PXLIB_CALL
PX_get_majorversion(void);

PXLIB_API int PXLIB_CALL
PX_get_minorversion(void);

PXLIB_API int PXLIB_CALL
PX_get_subminorversion(void);

PXLIB_API int PXLIB_CALL
PX_has_recode_support(void);

PXLIB_API int PXLIB_CALL
PX_has_gsf_support(void);

PXLIB_API int PXLIB_CALL
PX_is_bigendian(void);

PXLIB_API char * PXLIB_CALL
PX_get_builddate(void);

PXLIB_API void PXLIB_CALL
PX_boot(void);

PXLIB_API void PXLIB_CALL
PX_shutdown(void);

PXLIB_API pxdoc_t* PXLIB_CALL
PX_new3(void  (*errorhandler)(pxdoc_t *p, int type, const char *msg, void *data),
        void* (*allocproc)(pxdoc_t *p, size_t size, const char *caller),
        void* (*reallocproc)(pxdoc_t *p, void *mem, size_t size, const char *caller),
        void  (*freeproc)(pxdoc_t *p, void *mem),
		void* errorhandler_user_data);

PXLIB_API pxdoc_t* PXLIB_CALL
PX_new2(void  (*errorhandler)(pxdoc_t *p, int type, const char *msg, void *data),
        void* (*allocproc)(pxdoc_t *p, size_t size, const char *caller),
        void* (*reallocproc)(pxdoc_t *p, void *mem, size_t size, const char *caller),
        void  (*freeproc)(pxdoc_t *p, void *mem));

PXLIB_API pxdoc_t* PXLIB_CALL
PX_new(void);

PXLIB_API int PXLIB_CALL
PX_open_fp(pxdoc_t *pxdoc, FILE *fp);

PXLIB_API int PXLIB_CALL
PX_open_file(pxdoc_t *pxdoc, const char *filename);

PXLIB_API int PXLIB_CALL
PX_create_file(pxdoc_t *pxdoc, pxfield_t *pxf, int numfields, const char *filename, int type);

PXLIB_API int PXLIB_CALL
PX_create_fp(pxdoc_t *pxdoc, pxfield_t *pxf, int numfields, FILE *fp, int type);

PXLIB_API void* PXLIB_CALL
PX_get_opaque(pxdoc_t *pxdoc);

PXLIB_API int PXLIB_CALL
PX_write_primary_index(pxdoc_t *pxdoc, pxdoc_t *pxindex);

PXLIB_API int PXLIB_CALL
PX_read_primary_index(pxdoc_t *pindex);

PXLIB_API int PXLIB_CALL
PX_add_primary_index(pxdoc_t *pxdoc, pxdoc_t *pindex);

PXLIB_API char * PXLIB_CALL
PX_get_record(pxdoc_t *pxdoc, int recno, char *data);

PXLIB_API char * PXLIB_CALL
PX_get_record2(pxdoc_t *pxdoc, int recno, char *data, int *deleted, pxdatablockinfo_t *pxdbinfo);

PXLIB_API int PXLIB_CALL
PX_put_recordn(pxdoc_t *pxdoc, char *data, int recpos);

PXLIB_API int PXLIB_CALL
PX_put_record(pxdoc_t *pxdoc, char *data);

PXLIB_API int PXLIB_CALL
PX_insert_record(pxdoc_t *pxdoc, pxval_t **dataptr);

PXLIB_API int PXLIB_CALL
PX_update_record(pxdoc_t *pxdoc, pxval_t **dataptr, int recno);

PXLIB_API int PXLIB_CALL
PX_delete_record(pxdoc_t *pxdoc, int recno);

PXLIB_API pxval_t ** PXLIB_CALL
PX_retrieve_record(pxdoc_t *pxdoc, int recno);

PXLIB_API void PXLIB_CALL
PX_close(pxdoc_t *pxdoc);

PXLIB_API void PXLIB_CALL
PX_delete(pxdoc_t *pxdoc);

PXLIB_API int PXLIB_CALL
PX_pack(pxdoc_t *pxdoc);

PXLIB_API pxfield_t* PXLIB_CALL
PX_get_fields(pxdoc_t *pxdoc);

PXLIB_API pxfield_t* PXLIB_CALL
PX_get_field(pxdoc_t *pxdoc, int i);

PXLIB_API int PXLIB_CALL
PX_get_num_fields(pxdoc_t *pxdoc);

PXLIB_API int PXLIB_CALL
PX_get_num_records(pxdoc_t *pxdoc);

PXLIB_API int PXLIB_CALL
PX_get_recordsize(pxdoc_t *pxdoc);

PXLIB_API int PXLIB_CALL
PX_set_parameter(pxdoc_t *pxdoc, const char *name, const char *value);

PXLIB_API int PXLIB_CALL
PX_get_parameter(pxdoc_t *pxdoc, const char *name, char **value);

PXLIB_API int PXLIB_CALL
PX_set_value(pxdoc_t *pxdoc, const char *name, float value);

PXLIB_API int PXLIB_CALL
PX_get_value(pxdoc_t *pxdoc, const char *name, float *value);

PXLIB_API int PXLIB_CALL
PX_set_targetencoding(pxdoc_t *pxdoc, const char *encoding);

PXLIB_API int PXLIB_CALL
PX_set_inputencoding(pxdoc_t *pxdoc, const char *encoding);

PXLIB_API int PXLIB_CALL
PX_set_tablename(pxdoc_t *pxdoc, const char *tablename);

PXLIB_API int PXLIB_CALL
PX_set_blob_file(pxdoc_t *pxdoc, const char *filename);

PXLIB_API int PXLIB_CALL
PX_set_blob_fp(pxdoc_t *pxdoc, FILE *fp);

PXLIB_API int PXLIB_CALL
PX_has_blob_file(pxdoc_t *pxdoc);

PXLIB_API pxblob_t* PXLIB_CALL
PX_new_blob(pxdoc_t *pxdoc);

PXLIB_API int PXLIB_CALL
PX_open_blob_fp(pxblob_t *pxdoc, FILE *fp);

PXLIB_API int PXLIB_CALL
PX_open_blob_file(pxblob_t *pxdoc, const char *filename);

PXLIB_API int PXLIB_CALL
PX_create_blob_fp(pxblob_t *pxdoc, FILE *fp);

PXLIB_API int PXLIB_CALL
PX_create_blob_file(pxblob_t *pxblob, const char *filename);

PXLIB_API void PXLIB_CALL
PX_close_blob(pxblob_t *pxdoc);

PXLIB_API void PXLIB_CALL
PX_delete_blob(pxblob_t *pxblob);

PXLIB_API char* PXLIB_CALL
PX_read_blobdata(pxblob_t *pxblob, const char *data, int len, int *mod, int *blobsize);

PXLIB_API char* PXLIB_CALL
PX_read_graphicdata(pxblob_t *pxblob, const char *data, int len, int *mod, int *blobsize);

PXLIB_API char* PXLIB_CALL
PX_read_grahicdata(pxblob_t *pxblob, const char *data, int len, int *mod, int *blobsize);

/* Data conversion functions */
/* Functions to read data from a record */
PXLIB_API int PXLIB_CALL
PX_get_data_alpha(pxdoc_t *pxdoc, char *data, int len, char **value);

PXLIB_API int PXLIB_CALL
PX_get_data_bytes(pxdoc_t *pxdoc, char *data, int len, char **value);

PXLIB_API int PXLIB_CALL
PX_get_data_double(pxdoc_t *pxdoc, char *data, int len, double *value);

PXLIB_API int PXLIB_CALL
PX_get_data_long(pxdoc_t *pxdoc, char *data, int len, long *value);

PXLIB_API int PXLIB_CALL
PX_get_data_short(pxdoc_t *pxdoc, char *data, int len, short int *value);

PXLIB_API int PXLIB_CALL
PX_get_data_byte(pxdoc_t *pxdoc, char *data, int len, char *value);

PXLIB_API int PXLIB_CALL
PX_get_data_bcd(pxdoc_t *pxdoc, unsigned char *data, int len, char **value);

PXLIB_API int PXLIB_CALL
PX_get_data_blob(pxdoc_t *pxdoc, const char *data, int len, int *mod, int *blobsize, char **value);

PXLIB_API int PXLIB_CALL
PX_get_data_graphic(pxdoc_t *pxdoc, const char *data, int len, int *mod, int *blobsize, char **value);

/* Funktion to add data to a record */
PXLIB_API void PXLIB_CALL
PX_put_data_alpha(pxdoc_t *pxdoc, char *data, int len, char *value);

PXLIB_API void PXLIB_CALL
PX_put_data_bytes(pxdoc_t *pxdoc, char *data, int len, char *value);

PXLIB_API void PXLIB_CALL
PX_put_data_double(pxdoc_t *pxdoc, char *data, int len, double value);

PXLIB_API void PXLIB_CALL
PX_put_data_long(pxdoc_t *pxdoc, char *data, int len, int value);

PXLIB_API void PXLIB_CALL
PX_put_data_short(pxdoc_t *pxdoc, char *data, int len, short int value);

PXLIB_API void PXLIB_CALL
PX_put_data_byte(pxdoc_t *pxdoc, char *data, int len, char value);

PXLIB_API void PXLIB_CALL
PX_put_data_bcd(pxdoc_t *pxdoc, char *data, int len, char *value);

PXLIB_API int PXLIB_CALL
PX_put_data_blob(pxdoc_t *pxdoc, char *data, int len, char *value, int valuelen);

PXLIB_API void PXLIB_CALL
PX_SdnToGregorian(long int sdn, int *pYear, int *pMonth, int *pDay);

PXLIB_API long int PXLIB_CALL
PX_GregorianToSdn(int year, int month, int day);

PXLIB_API pxval_t* PXLIB_CALL
PX_make_time(pxdoc_t *pxdoc, int hour, int minute, int second);

PXLIB_API pxval_t* PXLIB_CALL
PX_make_date(pxdoc_t *pxdoc, int year, int month, int day);

PXLIB_API pxval_t* PXLIB_CALL
PX_make_timestamp(pxdoc_t *pxdoc, int year, int month, int day, int hour, int minute, int second);

PXLIB_API char * PXLIB_CALL
PX_timestamp2string(pxdoc_t *pxdoc, double value, const char *format);

PXLIB_API char * PXLIB_CALL
PX_time2string(pxdoc_t *pxdoc, long value, const char *format);

PXLIB_API char * PXLIB_CALL
PX_date2string(pxdoc_t *pxdoc, long value, const char *format);

PXLIB_API char * PXLIB_CALL
PX_strdup(pxdoc_t *pxdoc, const char *str);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
