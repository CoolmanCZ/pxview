#ifndef PxView_h_
#define PxView_h_

#include <CtrlLib/CtrlLib.h>

#include "Version.h"
#include "PxRecordView.h"

namespace Upp {
#define LAYOUTFILE <PxView/PxView.lay>
#include <CtrlCore/lay.h>
} // namespace Upp

#define TFILE <PxView/PxView.t>
#include <Core/t.h>

#define IMAGECLASS PxViewImg
#define IMAGEFILE <PxView/PxView.iml>
#include <Draw/iml_header.h>

class PxView : public Upp::WithPxViewLayout<Upp::TopWindow> {
  public:
	PxView();
	~PxView() override{};

	void ShowInfo();
	void ChangeCharset();
	void DeleteRow();
	void ExportJson();
	void ExportAllJson();

  private:
	Upp::Array<PxRecordView> pxArray;

	Upp::FileSel fileSel;
	Upp::String filePattern;

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
