#ifndef _PxView_PxView_h_
#define _PxView_PxView_h_

#include "PxSession.h"

#include <Core/Core.h>
#include <CtrlLib/CtrlLib.h>

#include <CtrlLib/CtrlLib.h>
#include <GridCtrl/GridCtrl.h>

namespace Upp {
#define LAYOUTFILE <PxView/PxView.lay>
#include <CtrlCore/lay.h>

#define TFILE <PxView/PxView.t>
#include <Core/t.h>

#define IMAGECLASS PxViewImg
#define IMAGEFILE <PxView/PxView.iml>
#include <Draw/iml_header.h>
}

enum filetype { csv = 1, json };

class PxView : public Upp::WithPxViewLayout<Upp::TopWindow> {
  private:
	Upp::FileSel file;
	Upp::String filepattern;

	Upp::String m_version;

	Upp::MenuBar menubar;
	Upp::StatusBar statusbar;
	int m_current_lang;

	void Exit();
	void MakeMenu();
	void MenuMain(Upp::Bar &menu);
	void MenuFile(Upp::Bar &menu);
	void MenuDB(Upp::Bar &menu);

	void OpenFile();
	void OpenDirectory();
	void LoadFile(const Upp::String &filename);

	void SaveAs(const int filetype);
	void SaveAllAs(const int filetype);

	void ToggleLang();
	void RemoveTab();
	void CountRows();

  public:
    void ShowInfo();
    void ChangeCharset();
    void DeleteRow();
	void ExportJson();
	void ExportAllJson();

//	void HttpExport();

	typedef PxView CLASSNAME;

	PxView();
	virtual ~PxView();
};

class PxRecordView : public Upp::GridCtrl {
  private:
	Upp::ParadoxSession px;
	bool modified;

	Upp::Progress http_pi;
	Upp::HttpRequest http_client;
	Upp::int64 http_loaded;
	Upp::FileOut http_out;
	Upp::String http_path;
	Upp::String http_file_name;

	Upp::FileAppend http_error_log;
	Upp::String http_error_log_path;
	Upp::String http_pi_text;

	void StatusMenuBar(Upp::Bar &bar);
	void ReadRecords(byte charset = 0);
	void EditData();
	void SaveAs(const int filetype);

	void HttpStart();
	void HttpContent(const void *ptr, int size);
	void HttpShowProgress();

	void GetUrl(bool &upload, Upp::String &url, Upp::String &auth, bool &checkerror);
	int SendData(Upp::Json data, const Upp::String &url, const Upp::String &auth, bool &checkerror);

  public:
	Upp::Event<> WhenRemoveTab;
	void DoRemoveTab() { WhenRemoveTab(); };

	bool OpenDB(const Upp::String &filename);
	void ShowInfo();
	void ChangeCharset();
	void DeleteRow();
	void ExportJson();
	void ExportAllJson();

	Upp::String GetFilePath() const { return px.GetFilePath(); }
	Upp::String GetFileName() const { return px.GetFileName(); }
	bool IsModified() const { return modified; }

	Upp::String AsText(Upp::String (*format)(const Upp::Value &), const char *tab = "\t", const char *row = "\r\n",
				  const char *hdrtab = "\t", const char *hdrrow = "\r\n") const;
	Upp::String AsCsv(int sep = ';', bool hdr = true);
	Upp::String AsJson();

	Upp::Json GetJson(int row);
	Upp::Json GetJson() { return GetJson(GetCurrentRow()); }

	int GetCountRows() { return GetVisibleCount(); }

	void SaveAsCsv(const Upp::String &dirpath);
	void SaveAsJson(const Upp::String &dirpath);

	typedef PxRecordView CLASSNAME;

	PxRecordView();
	virtual ~PxRecordView(){};
};

#endif

// vim: ts=4
