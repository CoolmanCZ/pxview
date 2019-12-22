#include "PxView.h"

using namespace Upp;

PxRecordView::PxRecordView() {
	modified = false;

	WhenMenuBar = THISBACK(StatusMenuBar);
	WhenEnter = WhenLeftDouble = THISBACK(EditData);

	httpClient.MaxContentSize(INT_MAX);
	httpClient.WhenContent = THISBACK(HttpContent);
	httpClient.WhenWait = httpClient.WhenDo = THISBACK(HttpShowProgress);
	httpClient.WhenStart = THISBACK(HttpStart);

	httpLoaded = 0;
	httpFileName = "https_received_data.txt";
	httpErrorLogPath = AppendFileName(Nvl(GetDownloadFolder(), GetHomeDirFile("downloads")),
									  "https_error_log.txt");
	httpPIText = t_("HTTPS data transfer");

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
	bool enable = px.IsOpen();

	bar.Add(enable, t_("Show DB info"), THISBACK(ShowInfo));
	bar.Add(t_("Close this DB"), THISBACK(DoRemoveTab));
	bar.Separator();
	bar.Add(enable, t_("Change characters encoding"), THISBACK(ChangeCharset));
	bar.Separator();
	bar.Add(enable, t_("Delete current row"), THISBACK(DeleteRow));
	bar.Separator();
	bar.Add(enable, t_("Export DB as CSV"), THISBACK1(SaveAs, csv));
	bar.Add(enable, t_("Export DB as JSON"), THISBACK1(SaveAs, json));
	bar.Separator();
	bar.Add(enable, t_("Send current row using HTTPS (application/json)"), THISBACK(ExportJson));
	bar.Add(enable, t_("Send ALL rows using HTTPS (application/json)"), THISBACK(ExportAllJson));
}

bool PxRecordView::OpenDB(const String &filePath) {
	bool result = px.Open(filePath);

	if (result)
		ReadRecords();
	else
		Exclamation(Format("%s: %s", t_("Error during processing the file"), DeQtf(filePath)));

	return result;
}

void PxRecordView::ReadRecords(byte charset) {
	if (!px.IsOpen())
		return;

	Ready(false);
	Clear(true);

	Vector<SqlColumnInfo> columns = px.EnumColumns(nullptr, nullptr);
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
	if (!px.IsOpen())
		return;

	WithCharSetLayout<TopWindow> dlg;
	CtrlLayout(dlg, t_("Select encoding"));

	dlg.d.Add(CHARSET_UTF8, "UTF8");
	for (int i = 1; i < CharsetCount(); i++)
		dlg.d.Add(i, CharsetName(i));
	dlg.d.GoBegin();

	dlg.cancel <<= dlg.Breaker(IDCANCEL);
	dlg.ok <<= dlg.Breaker(IDOK);

	if (dlg.Run() == IDOK) {
		String charsetName = dlg.d.GetValue();
		ReadRecords(CharsetByName(charsetName));
	}
}

void PxRecordView::DeleteRow() {
	if (!px.IsOpen())
		return;

	int row = GetRowId();

	if (PromptOKCancel(t_("Delete current row from the DB?")) == IDOK) {
		if (px.DelRow(row))
			ReadRecords();
		else
			Exclamation(t_("Delete row has failed!"));
	}
}

void PxRecordView::EditData() {
	if (!px.IsOpen())
		return;

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
		Value newData = c->GetData();
		if (data != newData && px.SetRowCol(row, col, newData)) {
			Set(row, col, newData);
			modified = true;
		}
	}
}

void PxRecordView::ShowInfo() {
	if (!px.IsOpen())
		return;

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

static String sCsvString(const String &text) {
	String r;
	r << '\"';
	const char *s = text;
	while (*s) {
		if (*s == '\"')
			r << "\\\"";
		else
			r.Cat(*s);
		s++; // NOLINT: C code
	}
	r << '\"';
	return r;
}

static String sCsvFormat(const Value &v) {
	return (IsNumber(v) || IsVoid(v)) ? AsString(v) : CsvString(AsString(v));
}

String PxRecordView::AsCsv(int sep, bool hdr) {
	String h(0, 2);
	h.Set(0, sep);
	return AsText(sCsvFormat, h, "\r\n", hdr ? h : nullptr, "\r\n");
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

void PxRecordView::SaveAsCsv(const String &dirPath) {
	String fileName = px.GetFileName() + ".csv";
	String filePath = AppendFileName(dirPath, fileName);
	if (!SaveFile(filePath, AsCsv()))
		Exclamation("Error saving the CSV file");
	else
		Exclamation("Successfully saved the CSV file");
}

void PxRecordView::SaveAsJson(const String &dirPath) {
	String fileName = px.GetFileName() + ".json";
	String filePath = AppendFileName(dirPath, fileName);
	if (!SaveFile(filePath, AsJson()))
		Exclamation("Error saving the JSON file");
	else
		Exclamation("Successfully saved the JSON file");
}

void PxRecordView::GetUrl(bool &upload, String &url, String &auth, bool &checkError) {
	WithHttpSendLayout<TopWindow> ctrl;
	CtrlLayout(ctrl, t_("HTTPS data transfer"));

	ctrl.Acceptor(ctrl.ok, IDOK);
	ctrl.Rejector(ctrl.cancel, IDCANCEL);
	ctrl.WhenClose = ctrl.Rejector(IDCANCEL);
	ctrl.checkError <<= 1;

	ctrl.url.NullText("https://restapi.example.com/app/site/hosting/restlet.nl?script=11&deploy=1");
	ctrl.authorization.NullText("NLAuth nlauth_account=123456, nlauth_email=somobody@email.com, "
								"nlauth_signature=xxxxxxxx, nlauth_role=41");

	upload = false;
	if (ctrl.Execute() == IDOK) {
		url = ctrl.url.GetData();
		auth = ctrl.authorization.GetData();
		checkError = ctrl.checkError.GetData();
		upload = true;
	}
}

int PxRecordView::SendData(const Json &data, const String &url, const String &auth,
						   bool &checkError) {
	int result = IDOK;

	httpPath = AppendFileName(Nvl(GetDownloadFolder(), GetHomeDirFile("downloads")), httpFileName);
	httpPI.Reset();

	httpClient.New();
	httpClient.Authorization(auth);
	httpClient.ContentType("application/json");
	httpClient.Post(data);
	httpClient.Url(url).Execute();

	if (httpOut.IsOpen())
		httpOut.Close();

	if (!httpClient.IsSuccess()) {
		DeleteFile(httpPath);
		String error_msg = " => Sending data has failed.&\1" +
						   (httpClient.IsError() ? httpClient.GetErrorDesc()
												 : AsString(httpClient.GetStatusCode()) + ' ' +
													   httpClient.GetReasonPhrase());

		httpErrorLog.Open(httpErrorLogPath);
		httpErrorLog.Put(data.ToString());
		httpErrorLog.Put(error_msg);
		httpErrorLog.Put("\n");
		httpErrorLog.Flush();
		if (checkError)
			Exclamation(error_msg);
		result = IDCANCEL;
	}

	if (httpClient.IsAbort())
		checkError = true;

	return result;
}

void PxRecordView::ExportJson() {
	bool upload, checkError;
	String url, authorization;
	GetUrl(upload, url, authorization, checkError);

	if (FileExists(httpErrorLogPath))
		DeleteFile(httpErrorLogPath);

	if (upload) {
		httpPIText = t_("HTTPS data transfer");
		SendData(GetJson(), url, authorization, checkError);
	}
}

void PxRecordView::ExportAllJson() {
	bool upload, checkError;
	String url, authorization;
	GetUrl(upload, url, authorization, checkError);

	if (FileExists(httpErrorLogPath))
		DeleteFile(httpErrorLogPath);

	if (upload) {
		for (int i = 0; i < GetCount(); ++i) {
			httpPIText = Format(t_("HTTPS data transfer: %d/%d"), i + 1, GetCount());
			Json data = GetJson(i);
			if ((SendData(data, url, authorization, checkError) != IDOK) && (checkError))
				break;
		}
	}
}

void PxRecordView::HttpStart() {
	if (httpOut.IsOpen()) {
		httpOut.Close();
		DeleteFile(httpPath);
	}
	httpLoaded = 0;
}

void PxRecordView::HttpContent(const void *ptr, int size) {
	httpLoaded += size;
	if (!httpOut.IsOpen()) {
		RealizePath(httpPath);
		httpOut.Open(httpPath);
	}
	httpOut.Put(ptr, size);
}

void PxRecordView::HttpShowProgress() {
	if (httpClient.GetContentLength() >= 0) {
		httpPI.SetText(httpPIText);
		httpPI.Set((int)httpLoaded, (int)httpClient.GetContentLength());
	} else {
		httpPI.Set(0, 0);
		httpPI.SetText(httpClient.GetPhaseName());
	}

	if (httpPI.Canceled())
		httpClient.Abort();
}

// vim: ts=4
