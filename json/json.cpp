// XMLDOM.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "json.h"
#include "jsonlang.h"
#include "guids.h"
#include "version.h"
#include "..\PluginSettings.hpp"

static bool bInitCalled;

#define PARAM(_) TCHAR s##_[] = _T(#_);
PARAM(BrowseXMLFiles)
PARAM(StartUnsorted)

#ifdef FAR3
void WINAPI GetGlobalInfoW(GlobalInfo *info)
{
  info->StructSize = sizeof *info;
  info->MinFarVersion = MAKEFARVERSION(3,0,0,4242,VS_RELEASE);
  info->Version = MAKEFARVERSION(VERSIONMAJOR,VERSIONMINOR,0,0,VS_RELEASE);
  info->Guid = PluginGuid;
  info->Title = L"Json";
  info->Description = L"JSON Browser for FAR Manager";
  info->Author = L"Michael Yutsis <yutsis@gmail.com>";
}
#endif

void WINAPI EXP_NAME(SetStartupInfo)(PluginStartupInfo *Info)
{
    YMSPlugin::SetStartupInfo(Info, _T("YMS\\JSON"));
}

HANDLE WINAPI EXP_NAME(OpenFilePlugin)(LPCTSTR name, LPCBYTE data, int dataSize
#ifdef UNICODE
                    , int OpMode
#endif
                    )
{
    /*if(!SETTINGS_GET(sBrowseXMLFiles, 1))
	return INVALID_HANDLE_VALUE;*/

/*    int sz = dataSize;
    while(sz) {
	if(strchr(" \t\r\n", *data)) {
	    data++;
	    sz--;
	    if(*data != '{' && 
	}
	break;
    }*/

    {
        MemoryStream ms((LPCSTR)data, dataSize);
        AutoUTFInputStream<unsigned, MemoryStream> is(ms);
        GenericDocument<DocType> d;
        d.ParseStream(is);
        auto err = d.GetParseError();
        auto ofs= d.GetErrorOffset();
        if(err != kParseErrorNone && ofs < (unsigned)dataSize-4)
            return INVALID_HANDLE_VALUE;
    }
    return new JsonPlugin(name, data);
}

HANDLE WINAPI EXP_NAME(OpenPlugin)(int openFrom, int item)
{
    PCTSTR filePath;
    auto_ptr<wchar_t> cmdLine;
    auto_ptr<PluginPanelItem> panelItem;
    switch(openFrom) {
        case OPEN_COMMANDLINE:
            {
            if(!item)
	        return INVALID_HANDLE_VALUE;
#ifdef FAR3
            cmdLine.reset(_wcsdup(((OpenCommandLineInfo*)item)->CommandLine));
            filePath  = cmdLine.get();
#else
            filePath = (PCTSTR)item;
#endif
            if(!filePath || !*filePath)
	        return INVALID_HANDLE_VALUE;

            if(*filePath == '\"') {
                PTSTR p1 = const_cast<PTSTR>(filePath) + _tcslen(filePath) - 1;
                if(*p1 == '\"')
                    *p1 = 0;
                memmove(const_cast<PTSTR>(filePath), filePath + 1, (p1 - filePath + 1) * sizeof(TCHAR));
            }
            }
            break;

        case OPEN_PLUGINSMENU:
            {
            panelItem.reset(GetCurrentItem());
            filePath = panelItem->FileName;
            if(!filePath || !*filePath)
	        return INVALID_HANDLE_VALUE;
            }
            break;

#ifdef FAR3
        case OPEN_SHORTCUT:
            if(!item)
	        return INVALID_HANDLE_VALUE;
	    filePath = ((OpenShortcutInfo*)item)->HostFile;
            break;
#endif
        default:
	    return INVALID_HANDLE_VALUE;
    }

	//SaveScreen ss;
	//PCTSTR Items[]={_T(""),GetMsg(MLoading)};
	//Message(0,NULL,Items,_countof(Items),0);

    // These command lines are possible:
    //   file.json
    //   c:\dir\file.json
    //???   http://site.com/url

    tstring sFilePath( 
#ifdef FAR3
        openFrom == OPEN_SHORTCUT ? ((OpenShortcutInfo*)item)->HostFile : 
#endif
                    filePath);
    tstring sSubDir;
    if(openFrom == OPEN_COMMANDLINE) {
        tstring sCommandLine(filePath);
        size_t colon = sCommandLine.find(':');
        if(colon != tstring::npos) {
            tstring s1 = sCommandLine.substr(0, colon);
            if(GetFileAttributes(s1.c_str()) != 0xffffffff) { // file before 1st colon exists, get the rest 
                sFilePath = s1;
                sSubDir = sCommandLine.substr(colon+1);
            } else {
                colon = sCommandLine.find(':', colon+1);
                sFilePath = sCommandLine.substr(0, colon);
                if(colon != tstring::npos) // second colon exists, get the rest
                    sSubDir = sCommandLine.substr(colon+1);
            }
        }
        TCHAR absPath[MAX_PATH];
#ifdef UNICODE
        if(!wcschr(sFilePath.c_str(), ':')) // if rel path, convert it
        {
            StartupInfo.FSF->ConvertPath(CPM_FULL, sFilePath.c_str(), absPath, _countof(absPath));
#else
        if(GetFileAttributes(sFilePath.c_str()) != 0xffffffff) { // if local file exists, use its abs path
	    _tfullpath(absPath, sFilePath.c_str(), _countof(absPath));
#endif
            sFilePath = absPath;
        }
    }
    JsonPlugin* plugin;
    try
    {
        plugin = new JsonPlugin(sFilePath.c_str());
    }
    catch(WinExcept ex)
    {
        WinError(ex);
        return INVALID_HANDLE_VALUE;
    }
    if(!sSubDir.empty())
	try {
            plugin->SetDirectory(sSubDir.c_str(),0);
	}
	catch(...) {}
    return plugin;
}

#ifdef FAR3
HANDLE WINAPI AnalyseW(const AnalyseInfo *info)
{
    HANDLE h = OpenFilePluginW(info->FileName, (BYTE*)info->Buffer, (int)info->BufferSize, (int)info->OpMode);
    return h == INVALID_HANDLE_VALUE ? NULL : h;
}
#endif

void WINAPI EXP_NAME(GetPluginInfo)(PluginInfo *Info)
{
  Info->StructSize = sizeof *Info;
  Info->CommandPrefix = _T("json");

#ifdef FAR3
  static PluginMenuItem pluginMenuItem, configMenuItem;
//  static PCWSTR pluginMenuString;
//  pluginMenuString = GetMsg(MXMLBrwsr);

/*  if(SETTINGS_GET(sAddPluginsMenu, 0)) {
      pluginMenuItem.Guids = &MenuGuid;
      pluginMenuItem.Strings = &pluginMenuString;
      pluginMenuItem.Count = 1;
      Info->PluginMenu = pluginMenuItem;
  }*/

  configMenuItem.Guids = &MenuGuid;
  //configMenuItem.Strings = &pluginMenuString;
  //configMenuItem.Count = 1;

  Info->PluginConfig = configMenuItem;
#else  
  static PCTSTR menu;

  /*menu = GetMsg(MXMLBrwsr);
  Info->PluginConfigStrings = &menu;
  Info->PluginConfigStringsNumber = 1;*/

  /*if(SETTINGS_GET(sAddPluginsMenu, 0)) {
    Info->PluginMenuStrings = &menu;
    Info->PluginMenuStringsNumber = 1;
  }*/
#endif
}

#ifdef FAR3
int WINAPI JsonPlugin::ConfigureW(const ConfigureInfo *info)
{
    if(info->StructSize > sizeof *info /*|| memcmp(info->Guid, &MenuGuid, sizeof MenuGuid)*/)
        return FALSE;
#else
int WINAPI JsonPlugin::EXP_NAME(Configure)(int)
{
#endif

#if 0
 static FarDialogItemID DialogItems[]={
    /*00*/{DI_DOUBLEBOX,3,1,72,13,0,0,0,0,MOptions},
    /*01*/{DI_TEXT,5,2,0,0,0,0,0,0,MExportFormat},
    /*02*/{DI_RADIOBUTTON,5,3,0,0,0,0,DIF_GROUP,0,MExportUTF8},
    /*03*/{DI_RADIOBUTTON,5,4,0,0,0,0,0,0, MExportWin},
    /*04*/{DI_TEXT,4,5,0,0,0,0,DIF_SEPARATOR},
    /*05*/{DI_CHECKBOX,5,6,0,0,0,0,0,0,MAddPluginsMenu},
    /*06*/{DI_CHECKBOX,5,7,0,0,0,0,0,0,MEnableFileOpening},
    /*07*/{DI_CHECKBOX,5,8,0,0,0,0,0,0,MValidateOnParse},
    /*08*/{DI_CHECKBOX,40,8,0,0,0,0,0,0,MResolveExternals},    
    /*09*/{DI_CHECKBOX,5,9,0,0,0,0,0,0,MStartUnsorted},
    /*10*/{DI_TEXT,5,10,0,0,0,0,0,0,MNameAttributes},
    /*11*/{DI_EDIT,40,10,70},
    /*12*/{DI_TEXT,4,11,0,0,0,0,DIF_SEPARATOR},
    /*13*/{DI_BUTTON,0,12,0,0,0,0,DIF_CENTERGROUP,1,MOK},
    /*14*/{DI_BUTTON,0,12,0,0,0,0,DIF_CENTERGROUP,0,MCancel},
    /*15*/{DI_BUTTON,0,12,0,0,0,0,DIF_BTNNOCLOSE|DIF_CENTERGROUP,0,MOverrideColumns},
  };
  extern TCHAR sExportUTF[];
  USE_SETTINGSW;
  bExportUTF = settings.Get(sExportUTF, 1);
  bStartUnsorted = settings.Get(sStartUnsorted, 1);
  DialogItems[2].Selected = bExportUTF;
  DialogItems[3].Selected = !bExportUTF;
  DialogItems[5].Selected = settings.Get(sAddPluginsMenu, 0);
  DialogItems[6].Selected = settings.Get(sBrowseXMLFiles, 1);
  DialogItems[7].Selected = settings.Get(sValidateOnParse, 0);
  DialogItems[8].Selected = settings.Get(sResolveExternals, 0);  
  DialogItems[9].Selected = settings.Get(sStartUnsorted, 1);  

#ifdef FAR3
  PanelInfo panel;
  StartupInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &panel);
  int itemCount = _countof(DialogItems) - (panel.PluginHandle == 0);
#else
  int itemCount = _countof(DialogItems) - 1;
#endif
  FarDialogItem* ItemsStr = MakeDialogItems(DialogItems, itemCount);

  extern TCHAR sIdAttributes[], sDefIdAttributes[];
#ifdef UNICODE
  wchar_t attrlist[512];
  DWORD attrsize = _countof(attrlist)-1;
  attrlist[attrsize] = 0;
#else
  char* attrlist = ItemsStr[11].Data;
  DWORD attrsize = sizeof ItemsStr[11].Data-1;
#endif
  SetData(ItemsStr[11], attrlist, attrsize);
  settings.Get(sIdAttributes, attrlist, attrsize, sDefIdAttributes);
  ToOem(ItemsStr[10].Data);
  ItemsStr[11].X1 = ItemsStr[10].X1 + _tcslen(ItemsStr[10].ITEMDATA)+1;

  RUN_DIALOG(ConfigDialogGuid, -1, -1, 76, 15, _T("Config"), ItemsStr, itemCount);
  if(DIALOG_RESULT != 13 /*OK*/) {
    delete ItemsStr;
    return FALSE;
  }
  bExportUTF = GetSelected(ItemsStr,2);
  bStartUnsorted = GetSelected(ItemsStr,9);
  ToAnsi(ItemsStr[11].Data);
  settings.Set(sIdAttributes, GetItemText(ItemsStr,11));
  settings.Set(sExportUTF, bExportUTF);
  settings.Set(sAddPluginsMenu, GetSelected(ItemsStr,5));
  settings.Set(sBrowseXMLFiles, GetSelected(ItemsStr,6));
  settings.Set(sValidateOnParse, GetSelected(ItemsStr,7));
  settings.Set(sResolveExternals, GetSelected(ItemsStr,8));
  settings.Set(sStartUnsorted, bStartUnsorted);

  XMLPlugin::LoadIdAttributes();
  delete ItemsStr;

  return TRUE;
#endif

  return FALSE;
}

int
#ifndef FAR3
    WINAPI
#endif
           EXP_NAME(Compare)(HANDLE hPlugin,const struct PluginPanelItem *Item1,const struct PluginPanelItem *Item2,
                                                    unsigned int Mode)
{
    switch(Mode)
    {
        case SM_NAME:
        case SM_EXT:
            {
#ifndef UNICODE
            return mystrcmpi(Item1->FindData.cFileName, Item2->FindData.cFileName);
#else
            PCWSTR s1 = Item1->FileName;
            PCWSTR s2 = Item2->FileName;
            return lstrcmpi(s1,s2);
#endif
            }
        default:
            return -2;
    }
}

#ifdef FAR3
intptr_t WINAPI CompareW(const CompareInfo *info)
{
    if(info->StructSize < sizeof(CompareInfo))
        return -2;
    return CompareW(info->hPanel, info->Item1, info->Item2, info->Mode);
}
#endif

#ifdef FAR3
int WINAPI JsonPlugin::EXP_NAME(ProcessDialogEvent)(const ProcessDialogEventInfo *info)
{
#if 0
    if(info->StructSize != sizeof(ProcessDialogEventInfo) || info->Event != DE_DLGPROCINIT)
        return FALSE;
    FarDialogEvent& evt = *(FarDialogEvent *)info->Param;
    if(evt.Msg == DN_BTNCLICK && evt.Param1 == 15/*Override button*/)
    {
        PanelInfo panel;
        Control(FCTL_GETPANELINFO, 0, &panel);
        if(!panel.PluginHandle || panel.OwnerGuid != PluginGuid)
            return FALSE;
        ((JsonPlugin*)panel.PluginHandle)->EditSpecialDocs();
        return TRUE;
    }
#endif
    return FALSE;
}
#endif
