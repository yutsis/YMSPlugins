#pragma once

#define _FAR_USE_WIN32_FIND_DATA // only for FAR1 is used
#include <plugin.hpp> // exact path for FAR1/2/3 is defined in project, property manager or environment

#include "tchar.h"
#include <memory>
#include <string>
#include <vector>

// Typecasts for WIN64
#define _tsclen (DWORD)tsclen
#define strlen (DWORD)strlen
#define wcslen (DWORD)wcslen

using namespace std;

// YMS plugin framework ;)

#ifdef UNICODE
typedef wstring tstring;
#define EXP_NAME(p) p##W
#define WCONST const
#define WTYPE wchar_t**
#define WDEREF *
#define WADDR &
#else
#define EXP_NAME(p) p
typedef string tstring;
#define WCONST
#define WTYPE char*
#define WDEREF
#define WADDR
#define INT_PTR int
#endif

#define TICK_INTERVAL_TO_SHOW 300

extern PluginStartupInfo StartupInfo;

#ifdef FAR3
#include <InitGuid.h>
extern const GUID PluginGuid;
#define OpenPluginInfo OpenPanelInfo
#define FCTL_CLOSEPLUGIN FCTL_CLOSEPANEL
#define DIALOG_ITEM(_type,_x1,_y1,_x2,_y2,_hist,_opt) { _type,_x1,_y1,_x2,_y2,0,_hist,0,_opt, _T("") },
#define DIALOG_ITEM_F(_type,_x1,_y1,_x2,_y2,_hist,_opt) { _type,_x1,_y1,_x2,_y2,0,_hist,0,_opt|DIF_FOCUS, _T("") },
#define DIALOG_ITEM_D(_type,_x1,_y1,_x2,_y2,_hist,_opt) { _type,_x1,_y1,_x2,_y2,0,_hist,0,_opt|DIF_DEFAULTBUTTON, _T("") },

enum
{
	PKF_CONTROL     = 0x00000001,
	PKF_ALT         = 0x00000002,
	PKF_SHIFT       = 0x00000004,
	PKF_PREPROCESS  = 0x00080000, // for "Key", function ProcessKey()
};
#define USERDATA UserData.Data
#else

#ifdef UNICODE
#define FileName FindData.lpwszFileName
#define AlternateFileName FindData.lpwszAlternateFileName
#else
#define FileName FindData.cFileName
#define AlternateFileName FindData.cAlternateFileName
#endif
#define FileSize FindData.nFileSize
#define FileAttributes FindData.dwFileAttributes
#define CreationTime FindData.ftCreationTime
#define LastAccessTime FindData.ftLastAccessTime
#define LastWriteTime FindData.ftLastWriteTime
#define PanelControl(_1, _2, _3, _4) Control(_1, _2, _3, (LONG_PTR)(_4))
#define DIALOG_ITEM(_type,_x1,_y1,_x2,_y2,_hist,_opt) { _type,_x1,_y1,_x2,_y2,0,(DWORD_PTR)_hist,_opt,0, _T("") },
#define DIALOG_ITEM_F(_type,_x1,_y1,_x2,_y2,_hist,_opt) { _type,_x1,_y1,_x2,_y2,1,(DWORD_PTR)_hist,_opt,0, _T("") },
#define DIALOG_ITEM_D(_type,_x1,_y1,_x2,_y2,_hist,_opt) { _type,_x1,_y1,_x2,_y2,0,(DWORD_PTR)_hist,_opt,1, _T("") },
#define USERDATA UserData
#endif


// Save screen automatically. Screen is saved when SaveScreen variable is 
//   declared and restored at block exit.
class SaveScreen {
    HANDLE hScreen;
  public:
    SaveScreen(int X1=0,int Y1=0,int X2=-1,int Y2=-1) {
	hScreen = StartupInfo.SaveScreen(X1,Y1,X2,Y2);
    }
    ~SaveScreen() { StartupInfo.RestoreScreen(hScreen); }
};

// Inline wrappers for FAR service API
inline int Message(unsigned flags, PCTSTR helpTopic, PCTSTR* items, int nItems, int nButtons) {
    return (int)StartupInfo.Message(
#ifdef FAR3
		&PluginGuid, NULL,
#else
		StartupInfo.ModuleNumber,
#endif
		flags, helpTopic, items, nItems, nButtons);
}

inline int Message(unsigned flags, PCTSTR helpTopic, vector<PCTSTR> items, int nButtons) {
    return Message(flags, helpTopic, &items[0], (int)items.size() , nButtons);
}

#define snprintf StartupInfo.FSF->snprintf

#ifdef FAR3
inline void Text(int X,int Y,const FarColor *Color, PTSTR Str) { StartupInfo.Text(X,Y,Color,Str); }
#else
inline void Text(int X,int Y,int Color, PTSTR Str) { StartupInfo.Text(X,Y,Color,Str); }
#endif

/*
inline PCTSTR GetItemText(FarDialogItem& item) {
#ifdef UNICODE
    return item.PtrData;
#else
    return item.Data;
#endif
}
*/
inline void SetInfoData(InfoPanelLine& infoPanelLine, PCTSTR source)
{
#ifdef UNICODE
    infoPanelLine.Data = source;
#else
    strncpy(infoPanelLine.Data, source, sizeof infoPanelLine.Data);
#endif
}

inline void SetInfoText(InfoPanelLine& infoPanelLine, PCTSTR source)
{
#ifdef UNICODE
    infoPanelLine.Text = source;
#else
    strcpy(infoPanelLine.Text, source);
#endif
}

#ifdef UNICODE
class Dialog
{
   HANDLE hDlg;
   int retCode;
public:
    Dialog(
#ifdef FAR3
		const GUID* dialogGuid,
#endif
		int X1, int Y1, int X2, int Y2, PCWSTR HelpTopic,
        FarDialogItem *items, int itemCount) : retCode(0)
    {
        hDlg = StartupInfo.DialogInit(
#ifdef FAR3
		&PluginGuid, dialogGuid,
#else
		StartupInfo.ModuleNumber,
#endif
			X1,Y1,X2,Y2,
	    (PWSTR)HelpTopic,items,itemCount, 0,0,NULL,0);
        if(hDlg == INVALID_HANDLE_VALUE)
              retCode = -1;
        else
            retCode = (int)StartupInfo.DialogRun(hDlg);
    }
    ~Dialog() { StartupInfo.DialogFree(hDlg); }
    int GetResult() { return retCode; }
    HANDLE GetHDlg() { return hDlg; }
};

#ifdef FAR3
#define RUN_DIALOG(_GUID, _X1,_Y1,_X2,_Y2,_HelpTopic,_items,_itemCount) Dialog theDialog(&_GUID, _X1,_Y1,_X2,_Y2,_HelpTopic,_items,_itemCount);
#else
#define RUN_DIALOG(_GUID, _X1,_Y1,_X2,_Y2,_HelpTopic,_items,_itemCount) Dialog theDialog(_X1,_Y1,_X2,_Y2,_HelpTopic,_items,_itemCount);
#endif

#define DIALOG_RESULT theDialog.GetResult()
inline void SetData(FarDialogItem& item, PCWSTR str, DWORD cchMaxLen=0) {
#ifdef FAR3    
#define ITEMDATA Data
	item.Data = str; item.MaxLength = cchMaxLen/*?cchMaxLen : wcslen(str)*/;
#else
#define ITEMDATA PtrData
	item.PtrData = str; item.MaxLen = cchMaxLen/*?cchMaxLen : wcslen(str)*/;
#endif
}
#define GetItemText(_item,_i) ((PCWSTR)StartupInfo.SendDlgMessage(theDialog.GetHDlg(),DM_GETCONSTTEXTPTR,_i,0))
#define GetSelected(_item,_i) ((int)StartupInfo.SendDlgMessage(theDialog.GetHDlg(),DM_GETCHECK,_i,0)!=0)
#else
inline int Dialog(int X1, int Y1, int X2, int Y2, PTSTR HelpTopic,
  	FarDialogItem *Item, int ItemsNumber) {
        return StartupInfo.Dialog(StartupInfo.ModuleNumber,X1,Y1,X2,Y2,
	    HelpTopic,Item,ItemsNumber);
}
#define RUN_DIALOG(_GUID, _X1,_Y1,_X2,_Y2,_HelpTopic,_items,_itemCount) int dialogRes = Dialog(_X1,_Y1,_X2,_Y2,_HelpTopic,_items,_itemCount);
#define DIALOG_RESULT dialogRes
#define ITEMDATA Data
inline void SetData(FarDialogItem& item, PCSTR str, DWORD notused=0) {
    if(str != NULL)
        strncpy(item.Data, str, sizeof item.Data-1);
}
inline PCSTR GetItemText(FarDialogItem* items, int i) {
    return items[i].Data;
}
inline bool GetSelected(FarDialogItem* items, int i) {
    return *(bool*)&items[i].Selected;
}
#endif

inline int Viewer(PCTSTR fileName, PCTSTR Title,int X1=0,int Y1=0,int X2=-1,int Y2=-1) {
    return (int)StartupInfo.Viewer(fileName,Title,X1,Y1,X2,Y2,VF_NONMODAL|VF_DELETEONCLOSE
#ifdef FAR3
					, CP_DEFAULT			
#elif UNICODE
                    , CP_AUTODETECT
#endif
        );
}
inline int Editor (PCTSTR pFileName, PCTSTR pTitle, DWORD dwFlags=0) {
    return (int)StartupInfo.Editor(pFileName, pTitle,0,0,-1,-1, dwFlags, 0,0
#ifdef FAR3
					, CP_DEFAULT			
#elif UNICODE
                    , CP_AUTODETECT
#endif
        );
}
inline PCTSTR GetMsg(int MsgId) {
    return StartupInfo.GetMsg(
#ifdef FAR3
		&PluginGuid,
#else
		StartupInfo.ModuleNumber,
#endif
		MsgId);
}

inline int Message(unsigned flags, PCTSTR helpTopic, int* itemsIDs, int nItems, int nButtons) {
    vector<PCTSTR> items(nItems);
    for(int i=0; i<nItems; i++)
        items[i] = GetMsg(itemsIDs[i]);
    return Message(flags, helpTopic, items, nButtons);
}

inline int Control(int Command, void *Param) {
#ifdef FAR3
    return (int)StartupInfo.PanelControl(PANEL_ACTIVE, (FILE_CONTROL_COMMANDS)Command, 0, Param);
#elif UNICODE
    return StartupInfo.Control(PANEL_ACTIVE, Command, 0, (LONG_PTR)Param);
#else
    return StartupInfo.Control(INVALID_HANDLE_VALUE, Command, Param);
#endif
}

inline int GetEditorInfo(EditorInfo& editorInfo) {
#ifdef FAR3
    return (int)StartupInfo.EditorControl(-1, ECTL_GETINFO, 0, &editorInfo);
#else
    return StartupInfo.EditorControl(ECTL_GETINFO, &editorInfo);
#endif
}

inline PluginPanelItem* GetCurrentItem() // caller must delete buffer in UNICODE version!! or use auto_ptr
{
#ifdef UNICODE
    int bufsize = Control(FCTL_GETCURRENTPANELITEM, 0);
        if(!bufsize) return NULL;
    char* buf = new char[bufsize];
#ifdef FAR3
	FarGetPluginPanelItem fgppi = { sizeof fgppi, bufsize, (PluginPanelItem*)buf };
    Control(FCTL_GETCURRENTPANELITEM, &fgppi);
#else
    Control(FCTL_GETCURRENTPANELITEM, buf);
#endif
    return (PluginPanelItem*)buf;
#define CURRENTITEM_PTR(_item) auto_ptr<PluginPanelItem> _item(GetCurrentItem())
#else //!UNICODE
    PanelInfo panel;
    Control(FCTL_GETPANELINFO, &panel);
    return &panel.PanelItems[panel.CurrentItem];
#define CURRENTITEM_PTR(_item) PluginPanelItem* _item = GetCurrentItem()
#endif
}

// Exception classes
//  WinExcept - generic Windows error exception. Contains GetLastError code.
class WinExcept {
    DWORD err;
    DWORD bSilent;
  public:
    WinExcept(DWORD e) { err=e; bSilent = FALSE;}
    WinExcept() { err=GetLastError(); bSilent = FALSE;}
    operator DWORD() { return err; }
    void SetSilent(bool b=true) { bSilent = b; }
    DWORD GetSilent() { return bSilent; }
    void SetErr(DWORD e) { err = e; }
};

// FileExcept class; uses when needed to distinguish file errors from other errors
class FileExcept: public WinExcept { public: FileExcept(DWORD e):WinExcept(e){} };

// User action exception class. Used in YMSPlugin loops to break them.
//  throw it to take actions: break or continue
class ActionException {
    bool bBreak;
    public:
	ActionException(bool bBrk=false) { bBreak = bBrk; }
	operator bool() { return bBreak; }
};

// ProgressAction class is used to display progress indicators
class ProgressAction {
    DWORD dwTicks;
  protected:
    virtual PCTSTR ProgressTitle() { return _T(""); }
    virtual PTSTR ProgressMessage(PTSTR buf)=0;
    virtual void AbortAction() {}
  public:
    ProgressAction() { ResetTicks(); }
    void ResetTicks() { dwTicks=GetTickCount(); }
    void ShowProgress();
};

enum { OPM_TWODOTS = 0x800000 } ;

// Redefine FAR structures with automatic filling of StructSize
#define DEF_STRUCT(_name) \
      struct _name : public ::_name \
      { \
          _name() { StructSize = sizeof (::_name); } \
      };

struct KeyBarItem
{
  WORD VirtualKeyCode;
  WORD ControlKeyState;
  short MsgID;
};

class YMSPlugin
{
    friend class YMSExport;
  tstring sShortcutData;
  void DeleteKeyBarTitles();

 protected:
  BOOL bKeysAsDirs;
  TCHAR LastMadeName[MAX_PATH];
  // actual current dir
  TCHAR CurrentDir[2048];
  // the dir we return in Info.CurDir
  TCHAR CurDir[MAX_PATH];
  KeyBarTitles* pKeyBarTitles;
  KeyBarTitles* MakeKeyBarTitles(KeyBarItem* items, int count);

  int Control(int Command, void *Param) {
#ifdef FAR3
      return (int)StartupInfo.PanelControl(PANEL_ACTIVE, (FILE_CONTROL_COMMANDS)Command, 0, Param);
#elif UNICODE
      return StartupInfo.Control(PANEL_ACTIVE, Command, 0, (LONG_PTR)Param);
#else
      return StartupInfo.Control(this, Command, Param);
#endif
  }

#ifdef UNICODE
  static intptr_t Control(int Command, intptr_t Param1, void *Param2) {
#ifdef FAR3
      return StartupInfo.PanelControl(PANEL_ACTIVE, (FILE_CONTROL_COMMANDS)Command, Param1, Param2);
#else
      return StartupInfo.Control(PANEL_ACTIVE, Command, (int)Param1, (LONG_PTR)Param2);
#endif
  }
#endif

  int ControlRedraw(/*PanelRedrawInfo& pri*/size_t currentItem, size_t topPanelItem = 0)
  {
      PanelRedrawInfo pri = {
#ifdef FAR3
          sizeof pri,
#endif
          (int)currentItem, (int)topPanelItem };
      return Control(FCTL_REDRAWPANEL, &pri);
  }
#ifdef FAR3
  static INT_PTR AdvControl(ADVANCED_CONTROL_COMMANDS command, int param1, void *param2);
#else
  static INT_PTR AdvControl(int command, void *param);
#endif

  enum DESTPATH_RESULT {
      DESTPATH_ERROR = 0, DESTPATH_DIR = 1, DESTPATH_FILE = 2
  };
  
  DESTPATH_RESULT CheckDestPath(TCHAR* DestPath, PluginPanelItem *PanelItem,size_t ItemsNumber, PCTSTR ext);
  void CreateTmpDir(TCHAR* TmpPath);

#ifndef NOPANELS
  //GetFiles-related methods
  virtual PTSTR MakeFileName(PCTSTR DestPath, PCTSTR fname,int OpMode=0);
  virtual void ExportItem(PluginPanelItem& item, PCTSTR filename, bool bAppend=false) = 0;
  virtual DWORD GetFilesDialog(TCHAR* DestPath, DWORD cchSize, PCTSTR pInitialName, size_t nItems);
  virtual PCTSTR GetFilesHistoryId()=0;
  virtual PCTSTR DescHistoryId()=0;
  virtual PCTSTR GetFileExt() { return _T(""); }
  virtual void OnEnterGetFiles() {}
  virtual void OnMakeFileName(PluginPanelItem& item) {}
  virtual void OnAfterGetFiles() {}
  virtual void OnGetItem(PluginPanelItem &item) {}
  virtual void OnAfterGetItem(PluginPanelItem &item) {}

  //GetFiles is virtual temporarily, only for REG plugin!!!
  virtual int GetFiles(PluginPanelItem *PanelItem, size_t ItemsNumber,int Move,
	          WCONST WTYPE DestPath, int OpMode, bool bGetGroup=false);
  virtual int PutFiles(PluginPanelItem *PanelItem,size_t ItemsNumber,int Move,int OpMode,PCTSTR SrcPath=NULL) { return FALSE; }
  virtual int DeleteFiles(PluginPanelItem *PanelItem, size_t ItemsNumber, int OpMode)=0;
  virtual BOOL SetDirectory(PCTSTR Dir, int iOpMode=0)=0;
  virtual int MakeDirectory(WCONST WTYPE Name, int OpMode)=0;
  virtual BOOL GetFindData(PluginPanelItem*& PanelItem, int& ItemsNumber, int OpMode)=0;
  virtual void FreeFindData(PluginPanelItem* pPanelItem, int itemCount);
  virtual BOOL ProcessKey(int Key, unsigned int ControlState);
  virtual int ProcessEvent(int Event, void *Param);
#endif

  virtual void GetOpenPluginInfo(OpenPluginInfo& info);

  virtual void Reread(bool bOther=false, bool bKeepPosition=true, bool bKeepSelection=true);
  virtual bool ChangeDesc(PluginPanelItem& item, PCTSTR pNewDesc) { return true; }
  virtual TCHAR* GetCtrlZPrompt() {return _T("");}
  virtual BOOL CloseEvent() { return FALSE; }

  // called by FreeFindData; overridable because RegFilePlugin already has the names and does not need to reallocate them
  virtual void FreeFileNames(PluginPanelItem& item) {}

  // used in F3/F4 only
  virtual bool IsFolder(PluginPanelItem& item) { return item.USERDATA !=0 ; }
  virtual void GetShortcutData(tstring& data) { }

  void PutToCmdLine(PCTSTR tmp, bool bSpace=true);

#ifdef FAR3
  static int GetFarVersion() { return 3; }
#else  
  static int GetFarVersion();
#endif

 public:
  YMSPlugin() { bKeysAsDirs = true; *CurrentDir = 0; pKeyBarTitles = NULL; }
  virtual ~YMSPlugin();

  static void SetStartupInfo(PluginStartupInfo* Info, PCTSTR pRegRoot);

#ifdef UNICODE
  static PluginPanelItem* GetSelectedPanelItem(size_t index, vector<BYTE>& item_buf);
  static PluginPanelItem* GetPanelItem(size_t index, vector<BYTE>& item_buf);
#endif

  static int WinError(PCTSTR msg = NULL, PCSTR module = NULL);
  static int WinError(DWORD error);

#ifdef FAR3
  protected:
      DEF_STRUCT(PanelInfo)
#endif
};

#ifdef FAR3
inline INT_PTR YMSPlugin::AdvControl(ADVANCED_CONTROL_COMMANDS command, int param1, void *param2)
{
	return StartupInfo.AdvControl(&PluginGuid, command, param1, param2);
#else
inline INT_PTR YMSPlugin::AdvControl(int command, void *param)
{
	return StartupInfo.AdvControl(StartupInfo.ModuleNumber, command, param);
#endif
}

#ifdef UNICODE
#define ToOem(s)
#define ToAnsi(s)
#else
inline void ToOem(char*str) { CharToOemA(str,str); }
inline void ToAnsi(char*str) { OemToCharA(str,str); }
#endif
inline int MultiToWchar(PCSTR src, wchar_t* dest, int cchWideChar, UINT CodePage, int cbMultiByte=-1)
{
    return MultiByteToWideChar(CodePage, CodePage>50000||CodePage==CP_SYMBOL ? 0 : MB_PRECOMPOSED, src, cbMultiByte, dest, cchWideChar);
}
inline void OemToWchar(PCSTR src, wchar_t* dest, int cchWideChar) { MultiToWchar(src, dest, cchWideChar, CP_OEMCP); }
inline void AnsiToWchar(PCSTR src, wchar_t* dest, int cchWideChar) { MultiToWchar(src, dest, cchWideChar, CP_THREAD_ACP); }
void WriteBufToFile(PCTSTR filename, BYTE* data, DWORD cbData=-1, bool bAppend=false);
inline int WinError(PCTSTR msg = NULL, PCSTR module = NULL) { return YMSPlugin::WinError(msg, module); }
inline int WinError(tstring msg) { return YMSPlugin::WinError(msg.c_str()); }
inline int WinError(DWORD error) { return YMSPlugin::WinError(error); }
inline bool IsWhitespace(wchar_t c) { return c==L' ' || c==L'\t'; }
inline bool IsWhitespace(char c) { return c==' ' || c=='\t'; }
TCHAR* MakeItemDesc(PCSTR pText, DWORD dwLimit=0);
TCHAR* MakeItemDesc(DWORD dwValue, int iRadix=10);
TCHAR* MakeItemDesc(PCWSTR pText, DWORD dwLimit=0);

#ifdef UNICODE
#define OEMString(s) (s)
#define WideFromOem(s) (s)
#else
class OEMString {
    char* str;
  public:
      OEMString() { str = NULL; }
      OEMString(PCWSTR pwStr) { str = MakeItemDesc(pwStr); }
      ~OEMString() { delete str; }
      operator char*() const { return str; }
      OEMString& operator =(PCWSTR pwStr) { delete str; str = MakeItemDesc(pwStr); return *this; }
};

class WideFromOem {
    wchar_t* str;
  public:
      WideFromOem(PCSTR pStr) {
          int len = strlen(pStr)+1;
          str = new wchar_t[len];
          OemToWchar(pStr, str, len);
      }
      ~WideFromOem() { delete str; }
      operator wchar_t*() const { return str; }
};
#endif

int mystrcmpi(PCTSTR str1, PCTSTR str2, PCWSTR wstr1=NULL);

#ifdef UNICODE
inline int mystrcmpi(PCTSTR str1, PCTSTR str2, PCWSTR) { return lstrcmpiW(str1, str2); }
#endif

bool ReadBuffer(PCTSTR pFile, BYTE* buf, DWORD dwMaxSize=0, DWORD* pdwRead=NULL, BYTE** pNewBuf=NULL);
/*TCHAR* GetReg(PCTSTR name, TCHAR* buf, DWORD count, PCTSTR dflt);
tstring GetRegS(PCTSTR name, PCTSTR dflt);
DWORD GetReg(PCTSTR name, DWORD deflt=0);
void SetReg(PCTSTR name, BYTE* val, DWORD bufsize, DWORD Type);
void SetReg(PCTSTR name, PCSTR  buf);
void SetReg(PCTSTR name, PCWSTR buf);
void SetReg(PCTSTR name, DWORD wrd);

#ifndef FAR3
inline void SetReg(PCTSTR name, PCSTR  buf) { SetReg(name, (BYTE*)buf, strlen(buf)+1, REG_SZ); }
inline void SetReg(PCTSTR name, PCWSTR buf) { SetReg(name, (BYTE*)buf, (wcslen(buf)+1)*sizeof(wchar_t), REG_SZ); }
inline void SetReg(PCTSTR name, DWORD wrd) { SetReg(name, (BYTE*)&wrd, sizeof wrd, REG_DWORD); }
#define CloseSettings()
#else
void CloseSettings();
#endif*/

struct FarDialogItemID
{
  int Type, X1,Y1,X2,Y2, Focus;
  INT_PTR Selected;
  unsigned int Flags;
  int DefaultButton, ID;
};
FarDialogItem* MakeDialogItems(FarDialogItemID *items, int count);

struct MenuItem : FarMenuItem
{
    MenuItem(PCTSTR text, bool bSeparator = false)
    {
	memset(this, 0, sizeof FarMenuItem);
	if(bSeparator)
#ifdef FAR3
	    Flags = MIF_SEPARATOR;
#else
	    Separator = 1;
#endif
	if(text != NULL)	
#ifdef UNICODE
	    Text = _tcsdup(text);
    }
    ~MenuItem() { free((void*)Text); }
    MenuItem(const MenuItem& item2) { *this = item2; }
    void operator=(const MenuItem& item2) { memcpy(this, &item2, sizeof *this); Text = _tcsdup(item2.Text); }
#else
	{
	    strncpy(Text, text, sizeof Text - 1);
	    Text[sizeof Text - 1] = 0;
	}
    }
#endif

    void Select() {
#ifdef FAR3
	Flags |= MIF_SELECTED;
#else
	Selected = 1;
#endif
    }

    bool IsSeparator() { return 
#ifdef FAR3
	(Flags & MIF_SEPARATOR)
#else
	Separator 
#endif
			!= 0; }
};


#ifdef FAR3
#define MENUGUIDPARAM const GUID& menuGuid,
#define BREAK_KEY_TYPE FarKey
#else
#define MENUGUIDPARAM 
#define BREAK_KEY_TYPE int // the caller should define two different arrays for FAR3 and older FARs
#endif

inline int Menu(MENUGUIDPARAM int x, int y, int maxHeight, DWORD flags,
            int titleID, int bottomID, LPCTSTR helpTopic, const BREAK_KEY_TYPE *breakKeys,
            int *brkCode, const FarMenuItem *item0, int itemCount)
{
    #if defined(FAR3) && !defined(_X86_)
            intptr_t
    #else
            int // bug in far2 w64 api
    #endif
            breakCode = *brkCode;
    int ret = (int)StartupInfo.Menu(
    #ifdef FAR3
                &PluginGuid, &menuGuid,
    #else
                StartupInfo.ModuleNumber,
    #endif
                    x, y, maxHeight, flags,
                    titleID<0 ? NULL : GetMsg(titleID), bottomID<0 ? NULL : GetMsg(bottomID),
                    helpTopic, breakKeys, &breakCode, item0, itemCount);
    *brkCode = (int)breakCode;
    return ret;
}

inline int Menu(MENUGUIDPARAM int x, int y, int maxHeight, DWORD flags,
            int titleID, int bottomID, LPCTSTR helpTopic, const BREAK_KEY_TYPE *breakKeys,
            int *breakCode, vector<MenuItem> menuItems)
{
    return Menu(
#ifdef FAR3
            menuGuid,
#endif
            x, y, maxHeight, flags, titleID, bottomID, helpTopic, breakKeys, breakCode,
                            menuItems.empty() ? NULL : &menuItems[0], (int)menuItems.size());
}

#define DECLPROCADDR(rettype, procname, paramlist) \
	typedef rettype (WINAPI *P##procname)paramlist; \
	P##procname p##procname = (P##procname)-1; // not static, because the library may be freed

#define GETPROCADDR(_hlib, procname) {\
	if(p##procname == (P##procname)-1) \
			p##procname = (P##procname)GetProcAddress(_hlib, #procname); \
	}

#ifdef UNICODE
#define GETPROCADDRT(_hlib, procname) {\
	if(p##procname == (P##procname)-1) \
	p##procname = (P##procname)GetProcAddress(_hlib, #procname "W"); \
	}
#else
#define GETPROCADDRT(_hlib, procname) {\
	if(p##procname == (P##procname)-1) \
	p##procname = (P##procname)GetProcAddress(_hlib, #procname "A"); \
	}
#endif


#ifdef FAR3
#define USE_SETTINGS  PluginSettings settings(StartupInfo.SettingsControl, PluginGuid)
#define USE_SETTINGSW PluginSettings settings(StartupInfo.SettingsControl, PluginGuid)
#define SETTINGS_GET PluginSettings(StartupInfo.SettingsControl, PluginGuid).Get
#define SETTINGS_SET PluginSettings(StartupInfo.SettingsControl, PluginGuid).Set
#else
#define USE_SETTINGS  PluginSettings settings(false)
#define USE_SETTINGSW PluginSettings settings(true)
#define SETTINGS_GET PluginSettings(false).Get
#define SETTINGS_SET PluginSettings(true).Set
#endif

// Export functions calling instance protected virtual methods
class YMSExport
{
#ifdef FAR3
  static void WINAPI ClosePanelW(const ClosePanelInfo *info);
  static int  WINAPI DeleteFilesW(const DeleteFilesInfo *info);
  static void WINAPI FreeFindDataW(GetFindDataInfo *info);
  static int  WINAPI GetFilesW(GetFilesInfo *info);
  static int  WINAPI GetFindDataW(GetFindDataInfo *info);
  static void WINAPI GetOpenPanelInfoW(OpenPluginInfo *info);
  static int  WINAPI MakeDirectoryW(MakeDirectoryInfo *info);
  static HANDLE WINAPI OpenW(const OpenInfo *info);
  static int  WINAPI ProcessEditorEventW(const ProcessEditorEventInfo *info); // placeholder, not implemented!
  static int  WINAPI ProcessPanelEventW(const ProcessPanelEventInfo *info);
  static int  WINAPI ProcessPanelInputW(const ProcessPanelInputInfo *info);
  static int  WINAPI PutFilesW(const PutFilesInfo *info);
  static int  WINAPI SetDirectoryW(const SetDirectoryInfo *info);
#else
  static void WINAPI EXP_NAME(ClosePlugin)(HANDLE hPlugin);
  static int  WINAPI EXP_NAME(DeleteFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
  static void WINAPI EXP_NAME(FreeFindData)(HANDLE hPlugin, PluginPanelItem *PanelItem,int ItemsNumber);
  static int  WINAPI EXP_NAME(GetFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem,
					    int ItemsNumber, int Move, WCONST WTYPE DestPath,int OpMode);
  static int  WINAPI EXP_NAME(GetFindData)(HANDLE hPlugin, PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
  static void WINAPI EXP_NAME(GetOpenPluginInfo)(HANDLE hPlugin, OpenPluginInfo *Info);
  static int  WINAPI EXP_NAME(MakeDirectory)(HANDLE hPlugin, WCONST WTYPE Name, int OpMode);
  static int  WINAPI EXP_NAME(ProcessEditorEvent)(int Event, void *Param); // placeholder, not implemented!
  static int  WINAPI EXP_NAME(ProcessEvent)(HANDLE hPlugin,int Event,void *Param);
  static int  WINAPI EXP_NAME(ProcessKey)(HANDLE hPlugin, int Key, unsigned ControlState);
  static int  WINAPI EXP_NAME(PutFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move, 
#if UNICODE
				PCWSTR SrcPath, 
#endif
				int OpMode);
  static int  WINAPI EXP_NAME(SetDirectory)(HANDLE hPlugin, PCTSTR Dir, int iOpMode);
#endif
};
