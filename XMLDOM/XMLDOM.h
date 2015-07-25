#include "..\YMSPlugin.h"

using namespace std;

//Cannot use namespace MSXML2 in VC6 due to compiler errors

#define DEFPTR(obj) typedef MSXML2::IXMLDOM##obj##Ptr obj##Ptr;
DEFPTR(Document)
DEFPTR(Node)
DEFPTR(NodeList)
DEFPTR(NamedNodeMap)
DEFPTR(Element)
DEFPTR(Selection)
typedef MSXML2::DOMNodeType NodeType;

#define NNODES_HIDE_DESC 500 // Number of nodes until which descriptions are always shown

extern PluginStartupInfo StartupInfo;

struct SpecialFields {
    _bstr_t sNameFld, sDateFld, sDescriptionFld, sOwnerFld;
};
struct SpecialDoc : public map<_bstr_t, SpecialFields> {
    bool bDisabled;
    SpecialDoc() { bDisabled = false; }
};

class CXMLFile : public MSXML2::IXMLDOMDocument2Ptr {
    tstring sMSXMLVersion;
    PCSTR pXMLDOMDLL;
    const CLSID* clsid;
    void GetMSXMLVersionInfo();
    //NodePtr defaultNamespace;

  static const char MSXML6[], MSXML4[], MSXML3[];
  public:
    CXMLFile(PCTSTR pFileName, LPCBYTE data=NULL);
    PCTSTR GetMSXMLVersion() const { return sMSXMLVersion.c_str(); }
    bool IsMSXML3() { return pXMLDOMDLL==MSXML3; }
    void Save(PCTSTR filename);
    REFIID GetCLSID() const { return *clsid; }
};

NodePtr CreateNode(PCTSTR pFileName);

#define USERDATA_DIR 0x80000000

class XMLPlugin : public YMSPlugin {
    CXMLFile XmlFile;
    NodePtr CurNode;
    SelectionPtr XPathSelection;
    tstring sHostFile;
    bool bLocalHostFile;
    tstring sPanelTitle;
    tstring sCurXPath;
    int iXPathSubDir; // -1 = no xpath selection, 0 = we're in xpath root, 1 = we're under subdir of xpath selection
    int iStartPanelMode;
    DWORD dwParentItem;
    int iFarVersion;
    InfoPanelLine *infoPanelLine;
    int nInfoLines;
#ifdef UNICODE
    wstring strCreatedDir;
#endif

    static BOOL bExportUTF, bBakFiles, bStartUnsorted;

    bool bChanged;
    int iUpDir;
    BOOL bDisplayAttrsAsNames, bShowNodeDesc;

    virtual PCTSTR GetFileExt() override;
    virtual void ExportItem(PluginPanelItem& item, PCTSTR filename, bool bAppend) override;
    virtual PCTSTR GetFilesHistoryId() override { return _T("XMLDOMCopy"); }
    virtual PCTSTR DescHistoryId() override { return _T("XMLDOMDesc"); }

    virtual void OnMakeFileName(PluginPanelItem& item) override;
    virtual void OnAfterGetItem(PluginPanelItem& item) override;

    void WriteXML(PCTSTR filename, _bstr_t& xml, bool bAppend=false);
    virtual bool ChangeDesc(PluginPanelItem& item, PCTSTR pNewDesc) override;
    virtual PTSTR MakeFileName(PCTSTR DestPath, PCTSTR fname,int OpMode) override;
    virtual BOOL CloseEvent() override;
    void GetNodeXPath(NodePtr& pNode, tstring& sPath);
    virtual void GetShortcutData(tstring& data) override;

  private:
    PCTSTR pExtOnGet;
    TCHAR ExportNameSuffix[26];
    //char ColTypes5[22], ColWidths5[22];

    void SetCurNode(NodePtr& newnode, PCTSTR Dir=0);
    bool SelectXPath(PCTSTR pXPath, bool bCD=false);
    void GetItemFullPath(NodeListPtr& list, PluginPanelItem& item, tstring& s);
    void GetRegCols();
    void Redraw(DWORD currentID = 0xffffffff);

    void EditSpecialDocs();
    void LoadSpecialDocs();
#ifdef FAR3
    void LoadSpecialDocsFromReg(PCWSTR rootKey);
#endif
    void SaveSpecialDocs(bool bDelete, PCTSTR docName, PCTSTR entry, PCTSTR name, PCTSTR datetime, PCTSTR desc, PCTSTR owner);
    _bstr_t EvalXPath(NodePtr node, _bstr_t xpath, map<_bstr_t, DocumentPtr>& xsltDocMap);

    static bool GetNodeNameFromAttr(NodePtr& node, /*_variant_t& vValue*/ _bstr_t& sValue);
        
    static vector<_bstr_t> IdAttributes;    
    SpecialDoc mapSpecialDocs;

  public:
    XMLPlugin(LPCTSTR pFileName, LPCBYTE data=NULL);
    ~XMLPlugin();
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
#ifdef UNICODE
    virtual void FreeFileNames(PluginPanelItem& item) override;
#endif
    /*virtual bool IsFolder(PluginPanelItem& item) override {
        return (item.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; //((DWORD)item.USERDATA & USERDATA_DIR) != 0;
        // should we also do it in the base class? not virtual at all??
    }*/

    static void LoadIdAttributes();
    static void WinError(_com_error& e);
    static void WinError();
};

inline void XMLPlugin::WinError() { YMSPlugin::WinError(); }
