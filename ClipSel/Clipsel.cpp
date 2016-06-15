#include "stdafx.h"

#define NOPANELS

#include "..\YMSplugin.h"
#include "version.h"

// Lang constants
enum {
    MOK=1,
    MCancel,
    MError,

    MMenu,
    MOptions,
    MAddSeparators,
    MCaseSensitive,
    MTrimSpaces
};

// {8482FA99-CFEC-404F-97D0-F37307C672A6}
DEFINE_GUID(PluginGuid, 0x8482fa99, 0xcfec, 0x404f, 0x97, 0xd0, 0xf3, 0x73, 0x7, 0xc6, 0x72, 0xa6);
// {D2AE8CAB-382F-4088-BC3C-3DAF8C225E5B}
DEFINE_GUID(ConfigDialogGuid, 0xd2ae8cab, 0x382f, 0x4088, 0xbc, 0x3c, 0x3d, 0xaf, 0x8c, 0x22, 0x5e, 0x5b);
// {0AC11A6F-5BEC-48AE-9536-8DBE69FFD958}
DEFINE_GUID(MenuGuid, 0xac11a6f, 0x5bec, 0x48ae, 0x95, 0x36, 0x8d, 0xbe, 0x69, 0xff, 0xd9, 0x58);

#include "..\YMSplugin.cpp"

typedef unsigned char uchar;

void WINAPI EXP_NAME(SetStartupInfo)(PluginStartupInfo *Info)
{
    YMSPlugin::SetStartupInfo(Info, _T("YMS\\ClipSel"));
}

TCHAR DefaultSeparators[] = _T("\x1\x2\x3\x4\x5\x6\x7\x8\x9\xa\xb\xc\xd\xe\xf")
			   _T("\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f");

TCHAR keyAddSeparators[] = _T("AddSeparators");
TCHAR keyCaseSensitive[] = _T("CaseSensitive");
TCHAR keyTrimSpaces[] = _T("TrimSpaces");

#ifdef UNICODE
#define CLIPBD_FORMAT CF_UNICODETEXT
#else
#define CLIPBD_FORMAT CF_OEMTEXT
#endif

#ifdef FAR3
void WINAPI GetGlobalInfoW(GlobalInfo *info)
{
  info->StructSize = sizeof *info;
  info->MinFarVersion = MAKEFARVERSION(3,0,0,4242,VS_RELEASE);
  info->Version = MAKEFARVERSION(VERSIONMAJOR,VERSIONMINOR,0,0,VS_RELEASE);
  info->Guid = PluginGuid;
  info->Title = L"Select-from-Clipboard";
  info->Description = L"Select from Clipboard plugin for FAR Manager";
  info->Author = L"Michael Yutsis <yutsis@gmail.com>";
}
#endif

HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom, INT_PTR Item)
{
    PanelInfo panel;
    Control(FCTL_GETPANELINFO, &panel);
    if(panel.PanelType != PTYPE_FILEPANEL)
	return INVALID_HANDLE_VALUE;

    if(!OpenClipboard(0))
	return INVALID_HANDLE_VALUE;

    PCTSTR pClipboardText = (PCTSTR)GetClipboardData(CLIPBD_FORMAT);
    
    vector<tstring> files;

    if(!pClipboardText)
    {
	HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP);
	if (hDrop != NULL)
	{
	    int count = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	    if(count)
	    {
		bool bFilenamesOnly =
#ifdef FAR3
		    !(panel.Flags&PFLAGS_PLUGIN);
#else
		    !panel.Plugin;
#endif

		TCHAR szFile[MAX_PATH];
		for(int i=0; i<count; i++)
		{
		    DragQueryFile(hDrop, i, szFile, _countof(szFile));
		    TCHAR* pname = szFile;
		    // if it's regular panel, store filename only
		    if(bFilenamesOnly)
		    {			
			pname = _tcsrchr(szFile, '\\');
			if(!pname || !*++pname)
			    pname = szFile;
		    }
		    ToOem(pname);
		    files.push_back(pname);
		}
	    }
	}
    }
    CloseClipboard();

    USE_SETTINGS;

    if(pClipboardText)
    {
	tstring separators = settings.Get(keyAddSeparators, _T("")) + DefaultSeparators;
	tstring qseparators(DefaultSeparators); qseparators += '"'; // default separators and quotes
	tstring sseparators(separators); sseparators += ' '; // all defined separators and space

	BOOL bTrimSpaces = settings.Get(keyTrimSpaces, TRUE);

	//  2345|/" 346436"/345634564/54757\nert6y
	size_t l = _tcslen(pClipboardText);
	for(size_t i=0; i<l; )
	{
	    i += _tcsspn(pClipboardText+i, bTrimSpaces ? sseparators.c_str() : separators.c_str());
	    size_t start = i;
	    bool bSkipQuote = false;
	    if(pClipboardText[i] == '"')
	    {
		start = ++i;
		i += _tcscspn(pClipboardText+i, qseparators.c_str()); // i points to the first separator char after the string, or the quote
		if(pClipboardText[i] == '"')
		{
		    bSkipQuote = true;
		}
	    }
	    else
	    {
		i += _tcscspn(pClipboardText+i, separators.c_str()); // i points to the first separator char after the string
		if(bTrimSpaces)
		    while(i > start && pClipboardText[i-1] == ' ')
			--i;
	    }
	    if(start == i)
		continue; // maybe separator after quote, etc.
	    files.push_back(tstring(pClipboardText+start, pClipboardText+i));
	    if(bSkipQuote)
		++i;
	}
    }

#ifdef UNICODE
    Control(FCTL_BEGINSELECTION, 0);
#endif

    BOOL bCaseSensitive = settings.Get(keyCaseSensitive, FALSE);

#ifdef UNICODE // external loop is on panels because we allocate item_buf each time
    for(int i=0; i<(int)panel.ItemsNumber; i++)
    {
	vector<BYTE> item_buf;
        PluginPanelItem& item = *YMSPlugin::GetPanelItem(i, item_buf);

	for each(wstring filename in files)
	{
	    PCWSTR p = filename.c_str();
	    PCWSTR pw = p;
	    bool bBackslash = wcschr(p,'\\') != NULL;

#else  // if OEM, external loop is on file list because of WideFromOem; memory for panel items is not allocated

    for each(string filename in files)
    {
	PCSTR p = filename.c_str();
	WideFromOem w(p);
	PCWSTR pw = w;
	bool bBackslash = strchr(p,'\\') != NULL;

	for(int i=0; i<(int)panel.ItemsNumber; i++)
	{
	    PluginPanelItem& item = panel.PanelItems[i];
#endif

	    PCTSTR pName = bBackslash ? NULL : _tcsrchr(item.FileName, '\\');
    	    if(pName) pName++;
      	    else pName = item.FileName;

	    bool bSelect = bCaseSensitive ? lstrcmp(p, pName) == 0 :
					    mystrcmpi(p, pName, pw)==0 || mystrcmpi(p, item.AlternateFileName, pw)==0;
	    if(bSelect)
	    {
#ifdef UNICODE
		StartupInfo.PanelControl(PANEL_ACTIVE, FCTL_SETSELECTION, i, (void*)TRUE);
#else
		item.Flags |= PPIF_SELECTED;
#endif
	    }
        }
    }
#ifdef UNICODE
    Control(FCTL_ENDSELECTION, 0);
#else
    Control(FCTL_SETSELECTION, &panel);
#endif
    Control(FCTL_REDRAWPANEL, 0);

    return INVALID_HANDLE_VALUE;
}

void WINAPI EXP_NAME(GetPluginInfo)(PluginInfo *Info)
{
  static PCTSTR pluginMenuString;
    Info->StructSize = sizeof *Info;
    pluginMenuString = GetMsg(MMenu);

#ifdef FAR3
  static PluginMenuItem pluginMenuItem, configMenuItem;

    pluginMenuItem.Guids = &MenuGuid;
    pluginMenuItem.Strings = &pluginMenuString;
    pluginMenuItem.Count = 1;
    Info->PluginMenu = pluginMenuItem;

    configMenuItem.Guids = &MenuGuid;
    configMenuItem.Strings = &pluginMenuString;
    configMenuItem.Count = 1;
    Info->PluginConfig = configMenuItem;
#else
    Info->PluginMenuStrings = &pluginMenuString;
    Info->PluginMenuStringsNumber = 1;
    Info->PluginConfigStrings = &pluginMenuString;
    Info->PluginConfigStringsNumber = 1;
#endif
}


#ifdef FAR3
intptr_t WINAPI ConfigureW(const ConfigureInfo *info)
{
    if(info->StructSize > sizeof *info /*|| memcmp(info->Guid, &MenuGuid, sizeof MenuGuid)*/)
        return FALSE;
#else
int WINAPI EXP_NAME(Configure)(int ItemNumber)
{
  if(ItemNumber!=0)  return FALSE;
#endif

  static FarDialogItemID DialogItems[]={
    {DI_DOUBLEBOX,3,1,40,9,0,0,0,0,MOptions},
    {DI_TEXT,5,2,0,0,0,0,0,0, MAddSeparators},
    {DI_EDIT,5,3,38,0,1},
    {DI_TEXT,4,4,0,0,0,0,DIF_SEPARATOR},
    {DI_CHECKBOX,5,5,0,0,0,0,0,0,MCaseSensitive},
    {DI_CHECKBOX,5,6,0,0,0,0,0,0,MTrimSpaces},
    {DI_TEXT,4,7,0,0,0,0,DIF_SEPARATOR},
    {DI_BUTTON,0,8,0,0,0,0,DIF_CENTERGROUP,1,MOK},
    {DI_BUTTON,0,8,0,0,0,0,DIF_CENTERGROUP,0,MCancel},
  };

    FarDialogItem* ItemsStr = MakeDialogItems(DialogItems, _countof(DialogItems));

    TCHAR sep[256];
    {
    USE_SETTINGS;
    SetData(ItemsStr[2], settings.Get(keyAddSeparators, sep, _countof(sep), _T("")), _countof(sep));
    ItemsStr[4].Selected = settings.Get(keyCaseSensitive, FALSE);
    ItemsStr[5].Selected = settings.Get(keyTrimSpaces, TRUE);
    }

    RUN_DIALOG(ConfigDialogGuid, -1,-1,44,11,/*_T("ClpSelCfg")*/NULL, ItemsStr, _countof(DialogItems))
    if(DIALOG_RESULT != _countof(DialogItems)-2 )
    {
	delete ItemsStr;
	return FALSE;
    }

    {
    USE_SETTINGSW;
    settings.Set(keyCaseSensitive, (DWORD)GetSelected(ItemsStr, 4));
    settings.Set(keyTrimSpaces, (DWORD)GetSelected(ItemsStr, 5));
    settings.Set(keyAddSeparators, GetItemText(ItemsStr,2));
    }
    delete ItemsStr;
    return TRUE;
}
