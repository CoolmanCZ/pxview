#ifndef _PxView_PxView_h_
#define _PxView_PxView_h_

#include <CtrlLib/CtrlLib.h>

#include "PxRecordView.h"

using namespace Upp;

#define LAYOUTFILE <PxView/PxView.lay>
#include <CtrlCore/lay.h>

#define TFILE <PxView/PxView.t>
#include <Core/t.h>

#define IMAGECLASS PxViewImg
#define IMAGEFILE <PxView/PxView.iml>
#include <Draw/iml_header.h>

class PxView : public WithPxViewLayout<Upp::TopWindow> {
  public:
	typedef PxView CLASSNAME;
	PxView();
	virtual ~PxView(){};

	void ShowInfo();
	void ChangeCharset();
	void DeleteRow();
	void ExportJson();
	void ExportAllJson();

	//	void HttpExport();

  private:
	Upp::Array<PxRecordView> pxArray;

	Upp::FileSel fileSel;
	Upp::String filePattern;

	Upp::String version;

	Upp::MenuBar menuBar;
	Upp::StatusBar statusBar;
	int currentLang;

	void Exit();
	void MakeMenu();
	void MenuMain(Upp::Bar &menu);
	void MenuFile(Upp::Bar &menu);
	void MenuDB(Upp::Bar &menu);

	void OpenFile();
	void OpenDirectory();
	void LoadFile(const Upp::String &filePath);

	void SaveAs(const int fileType);
	void SaveAllAs(const int fileType);

	void ToggleLang();
	void RemoveTab();
	void CountRows();

	int FindPxRecordView(const Upp::String &filePath);
	void RemovePxRecordView(const Upp::String &filePath);
	PxRecordView *GetPxRecordView(const Upp::String &filePath);
};

#endif

// vim: ts=4
