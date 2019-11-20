#include "PxView.h"

using namespace Upp;

#define IMAGECLASS PxViewImg
#define IMAGEFILE <PxView/PxView.iml>
#include <Draw/iml_source.h>

GUI_APP_MAIN { PxView().Sizeable().Zoomable().Run(); }

PxView::PxView() {
	Icon(PxViewImg::AppLogo());

	version = "v1.1.1";

	CtrlLayout(*this);
	this->WhenClose = THISBACK(Exit);

	AddFrame(menuBar);
	AddFrame(statusBar);

	tab.WhenSet = THISBACK(CountRows);
	lang.WhenPush = THISBACK(ToggleLang);

	fileSel.Type(t_("table files (*.db)"), "*.db")
		.Type(t_("index information files (*.px, *.x*, *.y*)"), "*.px, *.x*, *.y*")
		.Type(t_("all files"), "*")
		.ActiveDir(GetHomeDirectory());

	filePattern = "*.db;*.px;*.x*;*.y*";

	currentLang = GetCurrentLanguage();
	ToggleLang();

	statusBar.SetDefault("");
	statusBar = "";
}

void PxView::Exit() {
	if (PromptOKCancel(t_("Exit Paradox database viewer?")))
		Close();
}

void PxView::MakeMenu() {
	menuBar.Set(THISBACK(MenuMain));
	menuBar.WhenHelp = statusBar;
}

void PxView::MenuMain(Bar &menu) {
	menu.Add(t_("Files"), THISBACK(MenuFile));
	menu.Add(t_("Database"), THISBACK(MenuDB));
}

void PxView::MenuFile(Bar &menu) {
	bool enable = tab.GetCount();

	menu.Add(t_("Open DB file"), CtrlImg::open(), THISBACK(OpenFile))
		.Key(K_CTRL_O)
		.Help(t_("Open DB file for view"));
	menu.Add(t_("Open directory"), CtrlImg::open(), THISBACK(OpenDirectory))
		.Key(K_CTRL_D)
		.Help(t_("Open all DB files in the selected directory for view"));
	menu.Add(enable, t_("Close DB file"), CtrlImg::open(), THISBACK(RemoveTab))
		.Key(K_CTRL_Q)
		.Help(t_("Close the currently displayed DB file"));
	menu.Add(t_("Exit"), THISBACK(Exit)).Key(K_CTRL_X).Help(t_("Quit the application"));
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
	if (!fileSel.ExecuteOpen(t_("Select file to open")))
		return;

	String filePath = fileSel.Get();
	if (PatternMatchMulti(filePattern, Upp::GetFileName(filePath)))
		LoadFile(filePath);
}

void PxView::OpenDirectory() {
	fileSel.ClearFiles();
	if (!fileSel.ExecuteSelectDir(t_("Select directory to open files")))
		return;

	String path = AppendFileName(fileSel.Get(), "*");

	for (FindFile ff(path); ff; ff.Next()) {
		if (ff.IsFile()) {
			String filePath = ff.GetPath();
			if (PatternMatchMulti(filePattern, Upp::GetFileName(filePath)))
				LoadFile(filePath);
		}
		Ctrl::ProcessEvents();
	}
}

void PxView::LoadFile(const String &filePath) {
	for (int i = 0; i < tab.GetCount(); ++i) {
		TabCtrl::Item &myTab = tab.GetItem(i);
		auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
		if (px && px->GetFilePath().IsEqual(filePath)) {
			tab.Set(i);
			return;
		}
	}

	PxRecordView *px = GetPxRecordView(filePath);
	if (px) {
		tab.Add(px->SizePos(), px->GetFileName());
		tab.Set(tab.GetCount() - 1);
		px->WhenRemoveTab = THISBACK(RemoveTab);
		px->WhenSearchCursor = THISBACK(CountRows);
	}
}

void PxView::ShowInfo() {
	int curTab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curTab);
	auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->ShowInfo();
}

void PxView::ChangeCharset() {
	int curTab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curTab);
	auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->ChangeCharset();
}

void PxView::DeleteRow() {
	int curTab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curTab);
	auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->DeleteRow();
}

void PxView::SaveAs(const int fileType) {
	fileSel.ClearFiles();
	if (!fileSel.ExecuteSelectDir(t_("Select directory to save the file")))
		return;

	int curTab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curTab);
	auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px) {
		if (fileType == csv)
			px->SaveAsCsv(fileSel.Get());
		else
			px->SaveAsJson(fileSel.Get());
	}
}

void PxView::SaveAllAs(const int fileType) {
	fileSel.ClearFiles();
	if (!fileSel.ExecuteSelectDir(t_("Select directory to save the files")))
		return;

	for (int i = 0; i < tab.GetCount(); ++i) {
		TabCtrl::Item &myTab = tab.GetItem(i);
		auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
		if (px) {
			if (fileType == csv)
				px->SaveAsCsv(fileSel.Get());
			else
				px->SaveAsJson(fileSel.Get());
		}
	}
}

void PxView::ExportJson() {
	int curTab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curTab);
	auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->ExportJson();
}

void PxView::ExportAllJson() {
	int curTab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curTab);
	auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		px->ExportAllJson();
}

void PxView::ToggleLang() {
	Size langSize = lang.GetSize();

	if (currentLang == LNG_('C', 'S', 'C', 'Z')) {
		currentLang = LNG_('E', 'N', 'U', 'S');
		lang.SetImage(Rescale(PxViewImg::cz(), langSize));
	} else {
		currentLang = LNG_('C', 'S', 'C', 'Z');
		lang.SetImage(Rescale(PxViewImg::gb(), langSize));
	}
	SetLanguage(currentLang);

	Title(Format("%s - %s", t_("Paradox database viewer"), version));
	numrows_text.SetText(t_("Visible rows:"));

	MakeMenu();
}

void PxView::RemoveTab() {
	int curTab = tab.Get();
	TabCtrl::Item &myTab = tab.GetItem(curTab);
	auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
	if (px)
		RemovePxRecordView(px->GetFilePath());

	tab.Remove(curTab);
}

void PxView::CountRows() {
	int curTab = tab.Get();
	int rows = 0;
	if (curTab > -1) {
		TabCtrl::Item &myTab = tab.GetItem(curTab);
		auto *px = dynamic_cast<PxRecordView *>(myTab.GetSlave());
		if (px)
			rows = px->GetCountRows();
	}
	numrows.SetText(AsString(rows));
}

int PxView::FindPxRecordView(const Upp::String &filePath) {
	int index = -1;
	for (int i = 0; i < pxArray.GetCount(); ++i) {
		if (pxArray[i].GetFilePath().IsEqual(filePath)) {
			index = i;
			break;
		}
	}

	return index;
}

void PxView::RemovePxRecordView(const Upp::String &filePath) {
	int index = FindPxRecordView(filePath);
	if (index != -1) {
		pxArray.Remove(index);
		pxArray.Shrink();
	}
};

PxRecordView *PxView::GetPxRecordView(const Upp::String &filePath) {
	int index = FindPxRecordView(filePath);
	if (index == -1) {
		pxArray.Create<PxRecordView>().OpenDB(filePath);
		index = FindPxRecordView(filePath);
	}
	if (index > -1 && index < pxArray.GetCount())
		return &pxArray[index];

	return nullptr;
};

// vim: ts=4
