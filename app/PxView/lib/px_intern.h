#ifndef __PX_INTERN_H__
#define __PX_INTERN_H__

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(a) dgettext(GETTEXT_PACKAGE, a)
#else
#define _(a) a
#endif

typedef unsigned char byte;
//typedef unsigned short int integer;
typedef struct { char c[2]; } integer;
typedef struct { char c[4]; } longint;
typedef struct { char c[2]; } word;
typedef char *pchar;
typedef struct { char c[4]; } pointer;

#include "fileformat.h"

typedef struct _TFldInfoRec TFldInfoRec;
typedef TFldInfoRec *PFldInfoRec;

typedef struct _TPxHeader TPxHeader;
typedef TPxHeader *PPxHeader;

typedef struct _TPxDataHeader TPxDataHeader;
typedef TPxDataHeader *PPxDataHeader;

typedef struct _TDataBlock TDataBlock;
typedef TDataBlock *PDataBlock;

typedef struct _TMbHeader TMbHeader;
typedef TMbHeader *PMbHeader;

typedef struct _TMbBlockHeader2 TMbBlockHeader2;
typedef TMbBlockHeader2 *PMbBlockHeader2;

typedef struct _TMbBlockHeader3 TMbBlockHeader3;
typedef TMbBlockHeader3 *PMbBlockHeader3;

typedef struct _TMbBlockHeader3Table TMbBlockHeader3Table;
typedef TMbBlockHeader3Table *PMbBlockHeader3Table;

#endif
