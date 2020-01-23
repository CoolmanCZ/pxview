#include "PxSession.h"

using namespace Upp;

ParadoxSession::ParadoxSession() {
	open = false;
	PX_boot();
	pxdoc = PX_new2(ErrorHandler, nullptr, nullptr, nullptr);
}

ParadoxSession::~ParadoxSession() {
	PX_delete(pxdoc);
	PX_shutdown();
}

bool ParadoxSession::Open(const char *filename) {
	filepath = filename;

	if (0 == PX_open_file(pxdoc, filepath)) {
		open = true;
		blobfilepath =
			AppendFileName(Upp::GetFileDirectory(filename), Upp::GetFileTitle(filename) + ".mb");
		// TODO: check proper work with the blob file
		// if (FileExists(blobfilepath) && (0 > PX_set_blob_file(pxdoc, blobfilepath))) {
		//	Exclamation(Format("%s: %s", t_("Could not open blob file"), DeQtf(blobfilepath)));
		//	return false;
		//}
		return true;
	}
	return false;
}

dword ParadoxSession::GetInfoType(char px_ftype) {
	switch (px_ftype) {
	case pxfAlpha:
		return STRING_V;
	case pxfDate:
		return DATE_V;
	case pxfShort:
		return INT_V;
	case pxfLong:
		return INT_V;
	case pxfCurrency:
		return DOUBLE_V;
	case pxfNumber:
		return DOUBLE_V;
	case pxfLogical:
		return BOOL_V;
	case pxfMemoBLOb:
		return VALUE_V;
	case pxfBLOb:
		return VALUE_V;
	case pxfFmtMemoBLOb:
		return VALUE_V;
	case pxfOLE:
		return VALUE_V;
	case pxfGraphic:
		return VALUE_V;
	case pxfTime:
		return STRING_V;
	case pxfTimestamp:
		return TIME_V;
	case pxfAutoInc:
		return INT_V;
	case pxfBCD:
		return STRING_V;
	case pxfBytes:
		return INT_V;
	default:
		return UNKNOWN_V;
	}
}

String ParadoxSession::GetFileTypeName() const {
	static VectorMap<int, String> type = {
		{0, "indexed .DB data file"},
		{1, "primary index .PX file"},
		{2, "non-indexed .DB data file"},
		{3, "non-incrementing secondary index .Xnn file"},
		{4, "secondary index .Ynn file (inc or non-inc)"},
		{5, "incrementing secondary index .Xnn file"},
		{6, "non-incrementing secondary index .XGn file"},
		{7, "secondary index .YGn file (inc or non inc)"},
		{8, "incrementing secondary index .XGn file"},
	};

	return type.Get(GetFileType(), "Unknown file type");
}

// NOLINTNEXTLINE
Vector<SqlColumnInfo> ParadoxSession::EnumColumns(String database, String table) {
	Vector<SqlColumnInfo> out;

	if (IsOpen()) {
		SqlColumnInfo info;
		pxfield_t *pxf = PX_get_fields(pxdoc);

		for (int i = 0; i < PX_get_num_fields(pxdoc); ++i) {
			info.scale = info.precision = 0;
			info.name = pxf->px_fname;
			info.type = GetInfoType(pxf->px_ftype);
			info.width = pxf->px_flen;
			out.Add(info);
			++pxf; // NOLINT: C code
		}
	}

	return out;
}

// NOLINTNEXTLINE
Vector<String> ParadoxSession::EnumTables(String database) {
	Vector<String> out;
	out.Add(GetTableName());
	return out;
}

Vector<Value> ParadoxSession::GetRow(int row, byte charset) {
	Vector<Value> record;

	if (row < 0 || row > GetNumRecords())
		return record;

	pxdatablockinfo_t pxdbinfo;
	int isdeleted = 0; // TODO: allow option to select deleted data
	int recordsize = PX_get_recordsize(pxdoc);

	StringBuffer data(recordsize);
	if (nullptr == PX_get_record2(pxdoc, row, data, &isdeleted, &pxdbinfo))
		return record;

	int offset = 0;
	pxfield_t *pxf = PX_get_fields(pxdoc);
	byte codepage = CharsetByName(GetCharsetName());
	if (charset)
		codepage = charset;

	for (int i = 0; i < PX_get_num_fields(pxdoc); ++i) {
		Value val;
		switch (pxf->px_ftype) {
		case pxfAlpha: {
			char *value;
			if (0 < PX_get_data_alpha(pxdoc, &data[offset], pxf->px_flen, &value)) {
				val = Upp::ToUnicode(value, codepage);
				pxdoc->free(pxdoc, value);
			}
			record.Add(val);
			break;
		}
		case pxfDate: {
			long value;
			if (0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
				char const *date_format = "d/m/Y";
				char *str =
					PX_timestamp2string(pxdoc, (double)value * 1000.0 * 86400.0, date_format);
				Date d;
				StrToDate("dmy", d, str);
				val = d;
				pxdoc->free(pxdoc, str);
			}
			record.Add(val);
			break;
		}
		case pxfShort: {
			short int value;
			if (0 < PX_get_data_short(pxdoc, &data[offset], pxf->px_flen, &value))
				val = value;
			record.Add(val);
			break;
		}
		case pxfAutoInc:
		case pxfLong: {
			long value;
			if (0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value))
				val = (int)value;
			record.Add(val);
			break;
		}
		case pxfTimestamp: {
			double value;
			if (0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value)) {
				char const *timestamp_format = "d/m/Y H:i:s";
				char *str = PX_timestamp2string(pxdoc, value, timestamp_format);
				Time t;
				StrToTime("dmy", t, str);
				val = t;
				pxdoc->free(pxdoc, str);
			}
			record.Add(val);
			break;
		}
		case pxfTime: {
			long value;
			if (0 < PX_get_data_long(pxdoc, &data[offset], pxf->px_flen, &value)) {
				char const *time_format = "H:i:s";
				char *str = PX_timestamp2string(pxdoc, (double)value, time_format);
				val = str;
				pxdoc->free(pxdoc, str);
			}
			record.Add(val);
			break;
		}
		case pxfCurrency:
		case pxfNumber: {
			double value;
			if (0 < PX_get_data_double(pxdoc, &data[offset], pxf->px_flen, &value))
				val = value;
			record.Add(val);
			break;
		}
		case pxfLogical: {
			char value;
			val = false;
			if (0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_flen, &value)) {
				if (value)
					val = true;
			}
			record.Add(val);
			break;
		}
		case pxfGraphic:
		case pxfBLOb:
		case pxfFmtMemoBLOb:
		case pxfMemoBLOb:
		case pxfOLE: {
			char *blobdata;
			int mod_nr, size, ret;

			if (pxf->px_ftype == pxfGraphic)
				ret = PX_get_data_graphic(pxdoc, &data[offset], pxf->px_flen, &mod_nr, &size,
										  &blobdata);
			else
				ret =
					PX_get_data_blob(pxdoc, &data[offset], pxf->px_flen, &mod_nr, &size, &blobdata);

			if ((ret > 0) && (blobdata)) {
				if (pxf->px_ftype == pxfFmtMemoBLOb || pxf->px_ftype == pxfMemoBLOb) {
					String out(blobdata, size);
					val = Upp::ToUnicode(out, codepage);
				} else {
					String blobprefix = GetTableName();
					String blobextension = "blob";
					String filename = Format("%s_%d.%s", blobprefix, mod_nr, blobextension);
					val = filename;
				}
				pxdoc->free(pxdoc, blobdata);
			}
			record.Add(val);
			break;
		}
		case pxfBytes: {
			char value;
			if (0 < PX_get_data_byte(pxdoc, &data[offset], pxf->px_fdc, &value)) {
				val = value;
			}
			record.Add(val);
			break;
		}
		case pxfBCD: {
			char *value;
			// NOLINTNEXTLINE: C code
			if (0 < PX_get_data_bcd(pxdoc, (unsigned char *)&data[offset], pxf->px_fdc, &value)) {
				val = value;
				pxdoc->free(pxdoc, value);
			}
			record.Add(val);
			break;
		}
		default:
			record.Add(val);
			break;
		}
		offset += pxf->px_flen;
		++pxf; // NOLINT: C code
	}
	return record;
}

bool ParadoxSession::DelRow(int row) {
	if (row < 0 || row > GetNumRecords())
		return false;

	pxdatablockinfo_t pxdbinfo;
	int isdeleted = 0;
	int recordsize = PX_get_recordsize(pxdoc);

	StringBuffer data(recordsize);
	if (nullptr == PX_get_record2(pxdoc, row, data, &isdeleted, &pxdbinfo))
		return false;

	bool result = false;
	int ret = PX_delete_record(pxdoc, row);
	if (ret > -1)
		result = true;

	return result;
}

bool ParadoxSession::SetRowCol(int row, int col, const Value &value) {
	if (col >= PX_get_num_fields(pxdoc) || row < 0 || row > GetNumRecords())
		return false;

	pxdatablockinfo_t pxdbinfo;
	int isdeleted = 0;
	int recordsize = PX_get_recordsize(pxdoc);

	StringBuffer data(recordsize);

	if (nullptr == PX_get_record2(pxdoc, row, data, &isdeleted, &pxdbinfo))
		return false;

	bool result = false;
	int offset = 0;
	pxfield_t *pxf = PX_get_fields(pxdoc);
	byte codepage = CharsetByName(GetCharsetName());

	for (int i = 0; i < PX_get_num_fields(pxdoc); ++i) {
		if (i < col) {
			offset += pxf->px_flen;
			++pxf; // NOLINT: C code
			continue;
		}
		if (i > col)
			break;

		switch (pxf->px_ftype) {
		case pxfAlpha: {
			String val = Upp::FromUnicode((WString)value, codepage);
			PX_put_data_alpha(pxdoc, &data[offset], pxf->px_flen, StringBuffer(val).Begin());
			break;
		}
		case pxfDate: {
			Date date;
			if (value.GetType() == DATE_V)
				date = value;
			else
				date = ScanDate(value.ToString());
			long val = PX_GregorianToSdn(date.year, date.month, date.day) - 1721425;
			PX_put_data_long(pxdoc, &data[offset], 4, val);
			break;
		}
		case pxfShort: {
			int rec;
			if (value.GetType() == INT_V)
				rec = value;
			else
				rec = ScanInt(value.ToString());
			PX_put_data_short(pxdoc, &data[offset], 2, short(rec));
			break;
		}
		case pxfAutoInc:
		case pxfLong: {
			int rec;
			if (value.GetType() == INT_V)
				rec = value;
			else
				rec = ScanInt(value.ToString());
			PX_put_data_long(pxdoc, &data[offset], 4, rec);
			break;
		}
		case pxfTimestamp: {
			Time time;
			if (value.GetType() == TIME_V)
				time = value;
			else
				time = ScanTime(value.ToString());
			long val = PX_GregorianToSdn(time.year, time.month, time.day) - 1721425;
			double rec = (double(val) * 86400.0 + time.hour * 3600 + time.minute * 60 + time.second) * 1000.0;
			PX_put_data_double(pxdoc, &data[offset], 8, rec);
			break;
		}
		case pxfTime: {
			Time time;
			if (value.GetType() == TIME_V)
				time = value;
			else
				time = ScanTime(value.ToString());
			long val = time.hour * 3600000 + time.minute * 60000 + time.second * 1000;
			PX_put_data_long(pxdoc, &data[offset], 4, val);
			break;
		}
		case pxfCurrency:
		case pxfNumber: {
			double rec;
			if (value.GetType() == DOUBLE_V)
				rec = value;
			else
				rec = ScanDouble(value.ToString());
			PX_put_data_double(pxdoc, &data[offset], 8, rec);
			break;
		}
		case pxfLogical: {
			char val = 0;
			if ((value.GetType() == BOOL_V) && (value == true))
				val = 1;
			else {
				String str = Upp::FromUnicode((WString)value, codepage);
				str = ToLower(TrimBoth(str));
				if (str.Compare("1") == 0 || str.Compare("true") || str.Compare("t"))
					val = 1;
			}
			PX_put_data_byte(pxdoc, &data[offset], 1, val);
			break;
		}
		case pxfFmtMemoBLOb:
		case pxfMemoBLOb: {
			String val = Upp::FromUnicode((WString)value, codepage);
			PX_put_data_blob(pxdoc, &data[offset], pxf->px_flen, StringBuffer(val).Begin(),
							 val.GetCount());
			break;
		}
		case pxfBytes: {
			String val = Upp::FromUnicode((WString)value, codepage);
			PX_put_data_bytes(pxdoc, &data[offset], val.GetCount(), StringBuffer(val).Begin());
			break;
		}
		case pxfBCD: {
			String val = Upp::FromUnicode((WString)value, codepage);
			PX_put_data_bcd(pxdoc, &data[offset], val.GetCount(), StringBuffer(val).Begin());
			break;
		}
		default:
			break;
		}
		offset += pxf->px_flen;
		++pxf; // NOLINT: C code
	}

	if (PX_put_recordn(pxdoc, data, row) > -1)
		result = true;

	return result;
}

// vim: ts=4
