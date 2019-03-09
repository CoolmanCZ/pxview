#ifndef __FILEFORMAT_H__
#define __FILEFORMAT_H__

struct _TFldInfoRec {
	byte fType;
	byte fSize;
};

struct _TPxHeader {
	word recordSize;
	word headerSize;
	byte fileType;
	byte maxTableSize;
	longint numRecords;
	word nextBlock;
	word fileBlocks;
	word firstBlock;
	word lastBlock;
	word unknown12x13;
	byte modifiedFlags1;
	byte indexFieldNumber;
	pointer primaryIndexWorkspace;
	pointer unknownPtr1A;
	word indexRoot;
	byte numIndexLevels;
	integer numFields;
	integer primaryKeyFields;
	longint encryption1;
	byte sortOrder;
	byte modifiedFlags2;
	byte unknown2Bx2C[2];
	byte changeCount1;
	byte changeCount2;
	byte unknown2F;
	pointer tableNamePtrPtr;
	pointer fldInfoPtr;
	byte writeProtected;
	byte fileVersionID;
	word maxBlocks;
	byte unknown3C;
	byte auxPasswords;
	byte unknown3Ex3F[2];
	pointer cryptInfoStartPtr;
	pointer cryptInfoEndPtr;
	byte unknown48;
	longint autoInc;
	word firstFreeBlock;
	byte indexUpdateRequired;
	byte unknown50;
	word realHeaderSize;
	byte unknown53x54[2];
	byte refIntegrity;
	byte unknown56x57[2];
};

struct _TPxDataHeader {
	integer fileVerID3;
	integer fileVerID4;
	longint encryption2;
	longint fileUpdateTime;
	word hiFieldID;
	word hiFieldIDinfo;
	integer sometimesNumFields;
	word dosCodePage;
	byte unknown6Cx6F[4];
	integer changeCount4;
	byte unknown72x77[6];
};

struct _TDataBlock {
	word nextBlock;
	word prevBlock;
	integer addDataSize;
};

struct _TMbHeader {
	byte type;
	word blocksize;
	word modcount;
	byte unknown05;
	byte unknown06;
	byte unknown07;
	byte unknown08;
	byte unknown09;
	byte unknown0A;
	word basesize;
	word subblocksize;
	byte unknown0F;
	byte subchunksize;
	word suballoc;
	word subthresh;
};

struct _TMbBlockHeader2 {
	byte type;
	word numBlocks;
	longint blobLen;
	word modNr;
};

struct _TMbBlockHeader3 {
	byte type;
	word numBlocks;
	byte unknown[9];
};
struct _TMbBlockHeader3Table {
	byte offset;
	byte length;
	word modNr;
	byte lengthmod;
};

#endif
