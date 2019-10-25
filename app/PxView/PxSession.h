#ifndef _plugin_pxview_h_
#define _plugin_pxview_h_

#include <CtrlLib/CtrlLib.h>
#include <Sql/Sql.h>

extern "C" {
#include "lib/paradox.h"
}

static const char *pxfiletypename[] = {"indexed .DB data file",
									   "primary index .PX file",
									   "non-indexed .DB data file",
									   "non-incrementing secondary index .Xnn file",
									   "secondary index .Ynn file (inc or non-inc)",
									   "incrementing secondary index .Xnn file",
									   "non-incrementing secondary index .XGn file",
									   "secondary index .YGn file (inc or non inc)",
									   "incrementing secondary index .XGn file",
									   "Unknown file type"};

namespace Upp {

class ParadoxSession {
  public:
	ParadoxSession();
	virtual ~ParadoxSession();

	virtual bool IsOpen() const {
		return NULL != pxdoc->px_stream ||
			   (NULL != pxdoc->px_blob && NULL != pxdoc->px_blob->mb_stream);
	}

	virtual Vector<String> EnumUsers() {
		NEVER();
		return Vector<String>();
	} // No users in paradox!
	virtual Vector<String> EnumDatabases() {
		NEVER();
		return Vector<String>();
	} // No databases in paradox!
	virtual Vector<String> EnumTables(String database);
	virtual Vector<String> EnumViews(String database) {
		NEVER();
		return Vector<String>();
	} // No views in paradox!
	virtual Vector<SqlColumnInfo> EnumColumns(String database, String table);

	virtual bool IsBlobOpen() const {
		return (NULL != pxdoc->px_blob && NULL != pxdoc->px_blob->mb_stream);
	}

	virtual bool IsBlob() const { return NULL != pxdoc->px_blob; }

  private:
	pxdoc_t *pxdoc;

	String filepath;
	String blobfilepath;

	static void ErrorHandler(pxdoc_t *p, int error, const char *str, void *data) {
		Exclamation(Format("PXLib: %s", str));
	}
	dword GetInfoType(char px_ftype);

  public:
	String GetFilePath() const { return filepath; }
	String GetFileName() const { return Upp::GetFileName(filepath); }

	int GetNumRecords() const { return PX_get_num_records(pxdoc); }
	int GetNumFields() const { return PX_get_num_fields(pxdoc); }

	String GetTableName() const {
		char *str;
		PX_get_parameter(pxdoc, "tablename", &str);
		return AsString(str);
	}
	int GetCodepage() const {
		float number;
		PX_get_value(pxdoc, "codepage", &number);
		return (int)number;
	}
	int GetNumRecordsAll() const {
		float number;
		PX_get_value(pxdoc, "theonumrecords", &number);
		return (int)number;
	}
	int GetNumBlocks() const {
		float number;
		PX_get_value(pxdoc, "numblocks", &number);
		return (int)number;
	}
	int GetFirstBlock() const {
		float number;
		PX_get_value(pxdoc, "firstblock", &number);
		return (int)number;
	}
	int GetLastBlock() const {
		float number;
		PX_get_value(pxdoc, "lastblock", &number);
		return (int)number;
	}
	int GetHeaderSize() const {
		float number;
		PX_get_value(pxdoc, "headersize", &number);
		return (int)number;
	}
	int GetRecordSize() const {
		float number;
		PX_get_value(pxdoc, "recordsize", &number);
		return (int)number;
	}
	int GetMaxTableSize() const {
		float number;
		PX_get_value(pxdoc, "maxtablesize", &number);
		return (int)number;
	}
	int GetPrimaryKeyField() const {
		float number;
		PX_get_value(pxdoc, "primarykeyfields", &number);
		return (int)number;
	}
	int GetAutoInc() const {
		float number;
		PX_get_value(pxdoc, "autoinc", &number);
		return (int)number;
	}
	int GetFileType() const {
		float number;
		PX_get_value(pxdoc, "filetype", &number);
		return (int)number;
	}

	String GetFileTypeName() const { return pxfiletypename[GetFileType()]; }
	String GetCharsetName() const { return Format("%s%d", "cp", GetCodepage()); }
	String GetUpdateTime() const {
		return Upp::Format(Upp::TimeFromUTC(pxdoc->px_head->px_fileupdatetime));
	}

	int GetNumIndexLevels() const { return pxdoc->px_head->px_numindexlevels; }
	int GetIndexFieldNumber() const { return pxdoc->px_head->px_indexfieldnumber; }
	int GetNextAutoInc() const { return pxdoc->px_head->px_autoinc; }
	int GetRefIntegrity() const { return pxdoc->px_head->px_refintegrity; }
	char GetSortOrder() const { return pxdoc->px_head->px_sortorder; }
	char GetModFlags1() const { return pxdoc->px_head->px_modifiedflags1; }
	char GetModFlags2() const { return pxdoc->px_head->px_modifiedflags2; }
	bool IsWriteProtected() const { return pxdoc->px_head->px_writeprotected; }
	int64 GetEncryption() const { return pxdoc->px_head->px_encryption; }
	double GetFileVersion() const { return pxdoc->px_head->px_fileversion / 10.0; }

	void Close() { PX_close(pxdoc); }
	bool Open(const char *filename);

	Vector<Value> GetRow(int row, byte charset = 0);
	bool DelRow(int row);
	bool SetRowCol(int row, int col, const Value &value);

	operator pxdoc_t *() { return pxdoc; }
};

} // namespace Upp
#endif

// vim: ts=4
