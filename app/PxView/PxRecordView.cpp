#include "PxView.h"

using namespace Upp;

PxRecordView::PxRecordView() {
	modified = false;

	WhenMenuBar = THISBACK(StatusMenuBar);
	WhenEnter = WhenLeftDouble = THISBACK(EditData);

	http_client.MaxContentSize(INT_MAX);
	http_client.WhenContent = THISBACK(HttpContent);
	http_client.WhenWait = http_client.WhenDo = THISBACK(HttpShowProgress);
	http_client.WhenStart = THISBACK(HttpStart);

	http_file_name = "https_received_data.txt";
	http_error_log_path =
		AppendFileName(Nvl(GetDownloadFolder(), GetHomeDirFile("downloads")), "https_error_log.txt");
	http_pi_text = t_("HTTPS data transfer");

	Absolute()
		.Editing()
		.EditCell()
		.Accepting()
		.Canceling()
		.ColorRows()
		.Navigating()
		.Searching()
		.SetToolBar()
		.SelectRow()
		.MultiSelect()
		.Indicator(true, 12);
}

void PxRecordView::StatusMenuBar(Bar &bar) {
	bar.Add(t_("Show DB info"), THISBACK(ShowInfo));
	bar.Add(t_("Close this DB"), THISBACK(DoRemoveTab));
	bar.Separator();
	bar.Add(t_("Change characters encoding"), THISBACK(ChangeCharset));
	bar.Separator();
	bar.Add(t_("Delete current row"), THISBACK(DeleteRow));
	bar.Separator();
	bar.Add(t_("Export DB as CSV"), THISBACK1(SaveAs, csv));
	bar.Add(t_("Export DB as JSON"), THISBACK1(SaveAs, json));
	bar.Separator();
	bar.Add(t_("Send current row using HTTPS (application/json)"), THISBACK(ExportJson));
	bar.Add(t_("Send ALL rows using HTTPS (application/json)"), THISBACK(ExportAllJson));
}

bool PxRecordView::OpenDB(const String &filename) {
	bool result = px.Open(filename);

	if (result)
		ReadRecords();
	else
		Exclamation(Format("%s: %s", t_("Error during processing the file"), DeQtf(filename)));

	return result;
}

void PxRecordView::ReadRecords(byte charset) {
	Ready(false);
	Clear(true);

	Vector<SqlColumnInfo> columns = px.EnumColumns(NULL, NULL);
	for (int i = 0; i < columns.GetCount(); ++i)
		AddColumn(static_cast<Id>(columns[i].name), columns[i].name);

	Vector<Value> row;
	for (int i = 0; i < px.GetNumRecords(); ++i) {
		row = px.GetRow(i, charset);
		Add(row);
	}

	Ready(true);
}

void PxRecordView::ChangeCharset() {
	WithCharSetLayout<TopWindow> dlg;
	CtrlLayout(dlg, t_("Select encoding"));

	dlg.d.Add(CHARSET_UTF8, "UTF8");
	for (int i = 1; i < CharsetCount(); i++)
		dlg.d.Add(i, CharsetName(i));
	dlg.d.GoBegin();

	dlg.cancel <<= dlg.Breaker(IDCANCEL);
	dlg.ok <<= dlg.Breaker(IDOK);

	if (dlg.Run() == IDOK) {
		String charsetname = dlg.d.GetValue();
		ReadRecords(CharsetByName(charsetname));
	}
}

void PxRecordView::DeleteRow() {
	int row = GetRowId();

	if (PromptOKCancel(t_("Delete current row from the DB?")) == IDOK) {
		if (px.DelRow(row))
			ReadRecords();
		else
			Exclamation(t_("Delete row has failed!"));
	}
}

void PxRecordView::EditData() {
	int row = GetRowId();
	int col = GetColId();
	Value data = Get(row, col);

	Button ok, cancel;

	TopWindow editcolumn;
	editcolumn.Title(t_("Chage column data"));
	editcolumn.SetRect(0, 0, 640, 72);
	editcolumn.Sizeable().Zoomable();

	editcolumn.Add(ok.SetLabel(t_("OK")).RightPosZ(4, 56).BottomPosZ(4, 20));
	editcolumn.Add(cancel.SetLabel(t_("Cancel")).RightPosZ(64, 56).BottomPosZ(4, 20));

	editcolumn.Acceptor(ok, IDOK);
	editcolumn.Rejector(cancel, IDCANCEL);
	editcolumn.WhenClose = editcolumn.Rejector(IDCANCEL);

	// set edit layout based on value type
	Ctrl *c;
	EditInt ei;
	EditString es;
	EditDouble ed;
	DropDate edate;
	DropTime etime;

	dword type = data.GetType();
	if (type == INT_V)
		c = &ei;
	else if (type == BOOL_V)
		c = &es;
	else if (type == DOUBLE_V)
		c = &ed;
	else if (type == DATE_V)
		c = &edate;
	else if (type == TIME_V)
		c = &etime;
	else
		c = &es;

	editcolumn.Add(*c);
	c->HSizePosZ(4, 4);
	c->VSizePosZ(4, 28);
	c->SetData(data);

	if (editcolumn.Execute() == IDOK) {
		Value newdata = c->GetData();
		if (data != newdata && px.SetRowCol(row, col, newdata)) {
			Set(row, col, newdata);
			modified = true;
		}
	}
}

void PxRecordView::ShowInfo() {
	TopWindow w;
	ArrayCtrl info;

	info.AddColumn("parameter");
	info.AddColumn("value");
	info.NoHeader().AutoHideSb().OddRowColor();

	info.Add("File version", px.GetFileVersion());
	info.Add("File type", px.GetFileTypeName());
	info.Add("Table name", px.GetTableName());

	info.Add("Update time", px.GetUpdateTime());

	info.Add("Number of records", px.GetNumRecords());
	info.Add("Theoretical number of records", px.GetNumRecordsAll());
	info.Add("Number of fields", px.GetNumFields());
	info.Add("Headers size", px.GetHeaderSize());
	info.Add("Record size", px.GetRecordSize());
	info.Add("Max. table size", px.GetMaxTableSize());
	info.Add("Number of data blocks", px.GetNumBlocks());
	info.Add("Number of first data block", px.GetFirstBlock());
	info.Add("Number of last data block", px.GetLastBlock());

	if ((px.GetFileType() == pxfFileTypNonIncSecIndex) ||
		(px.GetFileType() == pxfFileTypIncSecIndex)) {
		info.Add("Number of index field", px.GetIndexFieldNumber());
		info.Add("Sort order of field", px.GetRefIntegrity());
	}

	if ((px.GetFileType() == pxfFileTypIndexDB) || (px.GetFileType() == pxfFileTypNonIndexDB))
		info.Add("Number of primary key fields", px.GetPrimaryKeyField());

	if (px.GetFileType() == pxfFileTypPrimIndex) {
		info.Add("Root index block number", px.GetAutoInc());
		info.Add("Number of index levels", px.GetNumIndexLevels());
	}

	info.Add("Write protected", px.IsWriteProtected());
	info.Add("Codepage", px.GetCharsetName());
	info.Add("Encryption", px.GetEncryption());

	info.Add("Sort order", px.GetSortOrder());
	info.Add("Auto increment", px.GetNextAutoInc());
	info.Add("Modified Flags 1", px.GetModFlags1());
	info.Add("Modified Flags 2", px.GetModFlags2());

	w.SetRect(0, 0, 500, 400);
	w.Sizeable();
	w.Add(info.SizePos());

	w.Run();
}

String PxRecordView::AsText(String (*format)(const Value &), const char *tab, const char *row,
							const char *hdrtab, const char *hdrrow) const {
	String txt;
	if (hdrtab) {
		for (int i = 0; i < GetColumnCount(); ++i) {
			if (i)
				txt << hdrtab;
			txt << (*format)(GetFixed(0, i));
		}
		if (hdrrow)
			txt << hdrrow;
	}
	bool next = false;
	for (int r = 0; r < GetCount(); ++r) {
		if (next)
			txt << row;
		for (int i = 0; i < GetColumnCount(); ++i) {
			if (i)
				txt << tab;
			txt << (*format)(Get(r, i));
		}
		next = true;
	}
	return txt;
}

static String sCsvString(const String& text)
{
	String r;
	r << '\"';
	const char *s = text;
	while(*s) {
		if(*s == '\"')
			r << "\\\"";
		else
			r.Cat(*s);
		s++;
	}
	r << '\"';
	return r;
}

static String sCsvFormat(const Value &v) {
	return (IsNumber(v) || IsVoid(v)) ? AsString(v) : CsvString(AsString(v));
}

String PxRecordView::AsCsv(int sep, bool hdr) {
	char h[2] = {(char)sep, 0};
	return AsText(sCsvFormat, h, "\r\n", hdr ? h : NULL, "\r\n");
}

String PxRecordView::AsJson() {
	JsonArray data;

	for (int r = 0; r < GetCount(); ++r)
		data << GetJson(r);

	return data.ToString();
}

Json PxRecordView::GetJson(int row) {
	Json json;

	for (int i = 0; i < GetColumnCount(); ++i) {
		String val = Get(row, i).ToString();
		if (val.GetCount()) {
			val.Replace("\r", "");
			val.Replace("\n", "\\n");
		}
		json.CatRaw(GetFixed(0, i).ToString(), sCsvString(val));
	}

	return json;
}

void PxRecordView::SaveAs(const int filetype) {
	FileSel file;
	if (!file.ExecuteSelectDir(t_("Select directory to save the file")))
		return;

	if (filetype == csv)
		SaveAsCsv(file.Get());
	else
		SaveAsJson(file.Get());
}

void PxRecordView::SaveAsCsv(const String &dirpath) {
	String filename = px.GetFileName() + ".csv";
	String filepath = AppendFileName(dirpath, filename);
	if (!SaveFile(filepath, AsCsv()))
		Exclamation("Error saving the CSV file");
	else
		Exclamation("Successfully saved the CSV file");
}

void PxRecordView::SaveAsJson(const String &dirpath) {
	String filename = px.GetFileName() + ".json";
	String filepath = AppendFileName(dirpath, filename);
	if (!SaveFile(filepath, AsJson()))
		Exclamation("Error saving the JSON file");
	else
		Exclamation("Successfully saved the JSON file");
}

void PxRecordView::GetUrl(bool &upload, String &url, String &auth, bool &checkerror) {
	WithHttpSendLayout<TopWindow> ctrl;
	CtrlLayout(ctrl, t_("HTTPS data transfer"));

	ctrl.Acceptor(ctrl.ok, IDOK);
	ctrl.Rejector(ctrl.cancel, IDCANCEL);
	ctrl.WhenClose = ctrl.Rejector(IDCANCEL);
	ctrl.checkerror <<= 1;

	ctrl.url.NullText(
		"https://restapi.example.com/app/site/hosting/restlet.nl?script=11&deploy=1");
	ctrl.authorization.NullText("NLAuth nlauth_account=123456, nlauth_email=somobody@email.com, "
							   "nlauth_signature=xxxxxxxx, nlauth_role=41");

	upload = false;
	if (ctrl.Execute() == IDOK) {
		url = ctrl.url.GetData();
		auth = ctrl.authorization.GetData();
		checkerror = ctrl.checkerror.GetData();
		upload = true;
	}
}

int PxRecordView::SendData(Json data, const String &url, const String &auth, bool &checkerror) {
	int result = IDOK;

	http_path =
		AppendFileName(Nvl(GetDownloadFolder(), GetHomeDirFile("downloads")), http_file_name);
	http_pi.Reset();

	http_client.New();
	http_client.Authorization(auth);
	http_client.ContentType("application/json");
	http_client.Post(data);
	http_client.Url(url).Execute();

	if (http_out.IsOpen())
		http_out.Close();

	if (!http_client.IsSuccess()) {
		DeleteFile(http_path);
		String error_msg = " => Sending data has failed.&\1" +
						   (http_client.IsError() ? http_client.GetErrorDesc()
												  : AsString(http_client.GetStatusCode()) + ' ' +
														http_client.GetReasonPhrase());

		http_error_log.Open(http_error_log_path);
		http_error_log.Put(data.ToString());
		http_error_log.Put(error_msg);
		http_error_log.Put("\n");
		http_error_log.Flush();
		if (checkerror)
			Exclamation(error_msg);
		result = IDCANCEL;
	}

	if (http_client.IsAbort())
		checkerror = true;

	return result;
}

void PxRecordView::ExportJson() {
	bool upload, checkerror;
	String url, authorization;
	GetUrl(upload, url, authorization, checkerror);

	if (FileExists(http_error_log_path))
		DeleteFile(http_error_log_path);

	if (upload) {
		http_pi_text = t_("HTTPS data transfer");
		SendData(GetJson(), url, authorization, checkerror);
	}
}

void PxRecordView::ExportAllJson() {
	bool upload, checkerror;
	String url, authorization;
	GetUrl(upload, url, authorization, checkerror);

	if (FileExists(http_error_log_path))
		DeleteFile(http_error_log_path);

	if (upload) {
		for (int i = 0; i < GetCount(); ++i) {
			http_pi_text = Format(t_("HTTPS data transfer: %d/%d"), i + 1, GetCount());
			Json data = GetJson(i);
			if ((SendData(data, url, authorization, checkerror) != IDOK) && (checkerror))
				break;
		}
	}
}

void PxRecordView::HttpStart() {
	if (http_out.IsOpen()) {
		http_out.Close();
		DeleteFile(http_path);
	}
	http_loaded = 0;
}

void PxRecordView::HttpContent(const void *ptr, int size) {
	http_loaded += size;
	if (!http_out.IsOpen()) {
		RealizePath(http_path);
		http_out.Open(http_path);
	}
	http_out.Put(ptr, size);
}

void PxRecordView::HttpShowProgress() {
	if (http_client.GetContentLength() >= 0) {
		http_pi.SetText(http_pi_text);
		http_pi.Set((int)http_loaded, (int)http_client.GetContentLength());
	} else {
		http_pi.Set(0, 0);
		http_pi.SetText(http_client.GetPhaseName());
	}

	if (http_pi.Canceled())
		http_client.Abort();
}

// vim: ts=4
