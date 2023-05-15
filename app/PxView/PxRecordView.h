#ifndef PxRecordView_h_
#define PxRecordView_h_

#include <CtrlLib/CtrlLib.h>
#include <GridCtrl/GridCtrl.h>

#include "PxSession.h"

enum filetype {
    csv = 1,
    json
};

class PxRecordView : public Upp::GridCtrl {
  public:
    PxRecordView();
    ~PxRecordView() override{};

  private:
    Upp::ParadoxSession px;
    bool modified = false;

    const int EditSizeHorz = 640;
    const int EditSizeVert = 72;
    const int InfoSizeHorz = 500;
    const int InfoSizeVert = 400;
    const int IndicatorSize = 12;

    Upp::Progress httpPI;
    Upp::HttpRequest httpClient;
    Upp::int64 httpLoaded = 0;
    Upp::FileOut httpOut;
    Upp::String httpPath;
    Upp::String httpFileName = "https_received_data.txt";

    Upp::FileAppend httpErrorLog;
    Upp::String httpErrorLogPath;
    Upp::String httpPIText = Upp::t_("HTTPS data transfer");

    void StatusMenuBar(Upp::Bar &bar);
    void ReadRecords(byte charset = 0);
    void EditData();
    void SaveAs(int fileType);

    void HttpStart();
    void HttpContent(const void *ptr, int size);
    void HttpShowProgress();

    void GetUrl(bool &upload, Upp::String &url, Upp::String &auth, bool &checkError);
    int SendData(const Upp::Json &data, const Upp::String &url, const Upp::String &auth, bool &checkError);

  public:
    Upp::Event<> WhenRemoveTab;
    void DoRemoveTab() const {
        WhenRemoveTab();
    };

    bool IsModified() const override {
        return modified;
    }

    bool OpenDB(const Upp::String &filePath);
    bool IsDBOpen() {
        return px.IsOpen();
    }
    void ShowInfo();
    void ChangeCharset();
    void DeleteRow();
    void ExportJson();
    void ExportAllJson();

    Upp::String GetFilePath() const {
        return px.GetFilePath();
    }
    Upp::String GetFileName() const {
        return px.GetFileName();
    }

    Upp::String AsText(Upp::String (*format)(const Upp::Value &),
                       const char *tab = "\t",
                       const char *row = "\r\n",
                       const char *hdrtab = "\t",
                       const char *hdrrow = "\r\n") const;
    Upp::String AsCsv(int sep = ';', bool hdr = true) const;
    Upp::String AsJson();

    Upp::Json GetJson(int row);
    Upp::Json GetJson() {
        return GetJson(GetCurrentRow());
    }

    int GetCountRows() {
        return GetVisibleCount();
    }

    void SaveAsCsv(const Upp::String &dirPath);
    void SaveAsJson(const Upp::String &dirPath);
};

#endif

// vim: ts=4 sw=4 expandtab
