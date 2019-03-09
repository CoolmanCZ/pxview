#include "PxView.h"

using namespace Upp;

#define IMAGECLASS PxViewImg
#define IMAGEFILE <PxView/PxView.iml>
#include <Draw/iml_source.h>

GUI_APP_MAIN { PxView().Sizeable().Zoomable().Run(); }

PxView::PxView() {
	Icon(PxViewImg::AppLogo());

	m_version = "v1.0.0";

	CtrlLayout(*this);
	this->WhenClose = THISBACK(Exit);

	AddFrame(menubar);
	AddFrame(statusbar);

	tab.WhenSet = THISBACK(CountRows);
	lang.WhenPush = THISBACK(ToggleLang);

	file.Type(t_("table files (*.db)"), "*.db")
		.Type(t_("index information files (*.px, *.x*, *.y*)"), "*.px, *.x*, *.y*")
		.Type(t_("all files"), "*")
		.ActiveDir(GetHomeDirectory());

	filepattern = "*.db;*.px;*.x*;*.y*";

	m_current_lang = GetCurrentLanguage();
	ToggleLang();

	statusbar.SetDefault("");
	statusbar = "";
}

PxView::~PxView() {
	for (int i = 0; i < tab.GetCount(); ++i) {
		TabCtrl::Item &myTab = tab.GetItem(i);
		PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
		delete (px);
	}
}

void PxView::Exit() {
	if (PromptOKCancel(t_("Exit Paradox database viewer?")))
		Close();
}

void PxView::MakeMenu() {
	menubar.Set(THISBACK(MenuMain));
	menubar.WhenHelp = statusbar;
}

void PxView::MenuMain(Bar &menu) {
	menu.Add(t_("Files"), THISBACK(MenuFile));
	menu.Add(t_("Database"), THISBACK(MenuDB));
}

void PxView::MenuFile(Bar &menu) {
	bool enable = tab.GetCount();

	menu.Add(t_("&Open DB file"), CtrlImg::open(), THISBACK(OpenFile))
		.Key(K_CTRL_O)
		.Help(t_("Open DB file for view"));
	menu.Add(t_("Open &directory"), CtrlImg::open(), THISBACK(OpenDirectory))
		.Key(K_CTRL_D)
		.Help(t_("Open all DB files in the selected directory for view"));
	menu.Add(enable, t_("Close DB file"), CtrlImg::open(), THISBACK(RemoveTab))
		.Key(K_CTRL_Q)
		.Help(t_("Close the currently displayed DB file"));
	menu.Add(t_("E&xit"), THISBACK(Exit)).Key(K_CTRL_X).Help(t_("Quit the application"));
}

void PxView::MenuDB(Bar &menu) {
	bool enable = tab.GetCount();

	menu.Add(enable, t_("Show DB info"), THISBACK(ShowInfo));
	menu.Separator();
	menu.Add(enable, t_("Change characters encoding"), THISBACK(ChangeCharset));
	menu.Separator();
	menu.Add(enable, t_("Delete current row"), THISBACK(DeleteRow));
	menu.Separator();
	menu.Add(enable, t_("Export current DB to CSV"), CtrlImg::save(), THISBACK1(SaveAs, csv))
		.Help(t_("Save current DB file in the CSV format to the directory..."));
	menu.Add(enable, t_("Export all DBs to CSV"), CtrlImg::save(), THISBACK1(SaveAllAs, csv))
		.Help(t_("Save all opened DB files in the CSV format to the directory..."));
	menu.Separator();
	menu.Add(enable, t_("Export current DB to JSON"), CtrlImg::save(), THISBACK1(SaveAs, json))
		.Help(t_("Save current DB file in the JSON format to the directory..."));
	menu.Add(enable, t_("Export all DBs to JSON"), CtrlImg::save(), THISBACK1(SaveAllAs, json))
		.Help(t_("Save all opened DB files in the JSON format to the directory..."));
	menu.Separator();
	menu.Add(enable, t_("Send current row using HTTPS (application/json)"), THISBACK(ExportJson));
	menu.Add(enable, t_("Send ALL rows using HTTPS (application/json)"), THISBACK(ExportAllJson));
}

void PxView::OpenFile() {
	if (!file.ExecuteOpen(t_("Select file to open")))
		return;

	String filename = file.Get();
	if (PatternMatchMulti(filepattern, Upp::GetFileName(filename)))
		LoadFile(filename);
}

void PxView::OpenDirectory() {
	file.ClearFiles();
	if (!file.ExecuteSelectDir(t_("Select directory to open files")))
		return;

	String path = AppendFileName(file.Get(), "*");

	for (FindFile ff(path); ff; ff.Next()) {
		if (ff.IsFile()) {
			String filename = ff.GetPath();
			if (PatternMatchMulti(filepattern, Upp::GetFileName(filename)))
				LoadFile(filename);
		}
		Ctrl::ProcessEvents();
	}
}

void PxView::LoadFile(const String &filename) {
	for (int i = 0; i < tab.GetCount(); ++i) {
		TabCtrl::Item &myTab = tab.GetItem(i);
		PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
		if (px && filename.Compare(px->GetFilePath()) == 0) {
			tab.Set(i);
			return;
		}
	}

	PxRecordView *px = new PxRecordView();
	if (px->OpenDB(filename)) {
		tab.Add(px->SizePos(), px->GetFileName());
		tab.Set(tab.GetCount() - 1);
		px->WhenRemoveTab = THISBACK(RemoveTab);
		px->WhenSearchCursor = THISBACK(CountRows);
	} else {
		delete (px);
	}
}

void PxView::ShowInfo() {
	int curtab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curtab);
	PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->ShowInfo();
}

void PxView::ChangeCharset() {
	int curtab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curtab);
	PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->ChangeCharset();
}

void PxView::DeleteRow() {
	int curtab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curtab);
	PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->DeleteRow();
}

void PxView::SaveAs(const int filetype) {
	file.ClearFiles();
	if (!file.ExecuteSelectDir(t_("Select directory to save the file")))
		return;

	int curtab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curtab);
	PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px) {
		if (filetype == csv)
			px->SaveAsCsv(file.Get());
		else
			px->SaveAsJson(file.Get());
	}
}

void PxView::SaveAllAs(const int filetype) {
	file.ClearFiles();
	if (!file.ExecuteSelectDir(t_("Select directory to save the files")))
		return;

	for (int i = 0; i < tab.GetCount(); ++i) {
		TabCtrl::Item &myTab = tab.GetItem(i);
		PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
		if (px) {
			if (filetype == csv)
				px->SaveAsCsv(file.Get());
			else
				px->SaveAsJson(file.Get());
		}
	}
}

void PxView::ExportJson() {
	int curtab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curtab);
	PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->ExportJson();
}

void PxView::ExportAllJson() {
	int curtab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curtab);
	PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->ExportAllJson();
}

void PxView::ToggleLang() {
	Size lang_size = lang.GetSize();

	if (m_current_lang == LNG_('C', 'S', 'C', 'Z')) {
		m_current_lang = LNG_('E', 'N', 'U', 'S');
		lang.SetImage(Rescale(PxViewImg::cz(), lang_size.cx, lang_size.cy));
	} else {
		m_current_lang = LNG_('C', 'S', 'C', 'Z');
		lang.SetImage(Rescale(PxViewImg::gb(), lang_size.cx, lang_size.cy));
	}
	SetLanguage(m_current_lang);

	Title(Format("%s - %s", t_("Paradox database viewer"), m_version));
	numrows_text.SetText(t_("Visible rows:"));

	MakeMenu();
}

void PxView::RemoveTab() {
	int curtab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curtab);
	PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px) {
		if (px->IsModified())
			Exclamation(t_("DB has been modified!"));
		delete (px);
	}

	tab.Remove(curtab);
}

void PxView::CountRows() {
	int curtab = tab.Get();
	int rows = 0;
	if (curtab > -1) {
		TabCtrl::Item &myTab = tab.GetItem(curtab);
		PxRecordView *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
		if (px)
			rows = px->GetCountRows();
	}
	numrows.SetText(AsString(rows));
}

// vim: ts=4
