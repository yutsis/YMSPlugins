#include "..\YMSPlugin.h"

//#ifdef UNICODE
    typedef UTF16<> DocType;
/*#else
    typedef UTF8<> DocType;
#endif*/

class JsonPlugin : public YMSPlugin {
    tstring sHostFile;
    FILE* f;
    vector<char> buf;
    GenericDocument<DocType> doc;
    GenericValue<DocType>* curObject;
    BOOL bDisplayAttrsAsNames, bShowNodeDesc;
    tstring sPanelTitle;
    bool bChanged;
    int iStartPanelMode;
    PCTSTR pExtOnGet;
    bool bSourceHasBOM;
    UTFType sourceUTFType;

    static /*??*/ BOOL bExportUTF, bBakFiles, bStartUnsorted;

    virtual void ExportItem(PluginPanelItem& item, PCTSTR filename, bool bAppend) override;
    virtual PCTSTR GetFilesHistoryId() override { return _T("JsonCopy"); }
    virtual PCTSTR DescHistoryId() override { return _T("JsonDesc"); }
    BOOL GoToPath(PCTSTR path);

public:
    JsonPlugin(LPCTSTR pFileName, LPCBYTE data=NULL);
    ~JsonPlugin();
    virtual void GetOpenPluginInfo(OpenPluginInfo& Info) override;
    virtual BOOL GetFindData(PluginPanelItem*& PanelItems, int& itemCount, int OpMode) override;
    virtual BOOL SetDirectory(PCTSTR Dir, int iOpMode=0) override;
    virtual BOOL ProcessKey(int Key, unsigned int ControlState) override;
#ifdef FAR3
    static int WINAPI ConfigureW(const ConfigureInfo *info);
    static int WINAPI ProcessDialogEventW(const ProcessDialogEventInfo *Info);
#else
    static int WINAPI EXP_NAME(Configure)(int);
#endif
    virtual int DeleteFiles(PluginPanelItem *PanelItem, size_t itemCount, int OpMode) override;
    virtual int PutFiles(PluginPanelItem *PanelItem, size_t itemCount, int Move, int OpMode, PCTSTR SrcPath=NULL) override;
    virtual int ProcessEvent(int Event, void *Param) override;
    virtual int MakeDirectory(WCONST WTYPE Name, int OpMode) override;
    virtual void GetShortcutData(tstring& data) override;
#ifdef UNICODE
    virtual void FreeFileNames(PluginPanelItem& item) override;
#endif
    virtual bool IsFolder(PluginPanelItem& item) override { return (Type)(int)item.USERDATA == kArrayType || (Type)(int)item.USERDATA == kObjectType; }
    virtual void OnMakeFileName(PluginPanelItem& item) override;
    virtual PCTSTR GetFileExt() override { return pExtOnGet; }
};
