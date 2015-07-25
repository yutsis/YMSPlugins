// XMLDOM.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "XMLDOM.h"
#include "XMLDOMlang.h"
#include "guids.h"
#include "version.h"
#include "..\PluginSettings.hpp"

static bool bInitCalled;

//static char IgnoredExtensions[] = {"html,htm,asp"};

#define PARAM(_) TCHAR s##_[] = _T(#_);
PARAM(BrowseXMLFiles)
PARAM(AddPluginsMenu)
PARAM(ValidateOnParse)
PARAM(ResolveExternals)
PARAM(StartUnsorted)

#ifdef FAR3
void WINAPI GetGlobalInfoW(GlobalInfo *info)
{
  info->StructSize = sizeof *info;
  info->MinFarVersion = MAKEFARVERSION(3,0,0,2927,VS_RELEASE);
  info->Version = MAKEFARVERSION(VERSIONMAJOR,VERSIONMINOR,0,0,VS_RELEASE);
  info->Guid = PluginGuid;
  info->Title = L"Xml";
  info->Description = L"XML Browser for FAR Manager";
  info->Author = L"Michael Yutsis <yutsis@gmail.com>";
}
#endif

void WINAPI EXP_NAME(SetStartupInfo)(PluginStartupInfo *Info)
{
    YMSPlugin::SetStartupInfo(Info, _T("YMS\\XMLDOM"));
}

HANDLE WINAPI EXP_NAME(OpenFilePlugin)(LPCTSTR name, LPCBYTE data, int dataSize
#ifdef UNICODE
                    , int OpMode
#endif
                    )
{
    if(!SETTINGS_GET(sBrowseXMLFiles, 1))
	return INVALID_HANDLE_VALUE;

    // Skip irrelevant data
    int sz = dataSize;
    while(sz) {
	if(strchr(" \t\r\n", *data)) {
	    data++;
	    sz--;
	    continue;
	}
	if(!_memicmp(data,"<!--", 4)) {
	    BYTE*p = (BYTE*)strstr((PCSTR)data+4, "-->");
	    if(!p) break;
	    p+=3;
	    sz -= (int)(p-data);
	    if(sz<=0)
		return INVALID_HANDLE_VALUE;
	    data = p;
	    continue;
	}
	break;
    }
    if(sz<=0)
	return INVALID_HANDLE_VALUE;
	
    // avoid opening html
    if(*data!='<' && *(DWORD*)data!=0x3cbfbbef && *(DWORD*)data!=0x003cfeff && *(DWORD*)data!=0x3c00fffe
	|| _memicmp(data,"<?xml", 5) && 
	(!_memicmp(data,"<html", 5) || !_memicmp(data,"<!DOCTYPE HTML", 13)
	|| *(WORD*)data==0x253c /*<% ASP*/)
	)
	return INVALID_HANDLE_VALUE;

    if(!bInitCalled)
	CoInitialize(0);
    try {
	return new XMLPlugin(name, data);
    }
    catch(_com_error e) {
        if(e.Error() && e.Error() != ERROR_CANCELLED)
            XMLPlugin::WinError(e);
	return INVALID_HANDLE_VALUE;
    }
}

HANDLE WINAPI EXP_NAME(OpenPlugin)(int OpenFrom,int Item)
{
    PCTSTR filePath;
    auto_ptr<wchar_t> cmdLine;
    auto_ptr<PluginPanelItem> panelItem;
    switch(OpenFrom) {
        case OPEN_COMMANDLINE:
            {
            if(!Item)
	        return INVALID_HANDLE_VALUE;
#ifdef FAR3
            cmdLine.reset(_wcsdup(((OpenCommandLineInfo*)Item)->CommandLine));
            filePath  = cmdLine.get();
#else
            filePath = (PCTSTR)Item;
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
            if(!Item)
	        return INVALID_HANDLE_VALUE;
	    filePath = ((OpenShortcutInfo*)Item)->HostFile;
            break;
#endif
        default:
	    return INVALID_HANDLE_VALUE;
    }

    if(!bInitCalled)
	    CoInitialize(0);
    try {
	    SaveScreen ss;
	    PCTSTR Items[]={_T(""),GetMsg(MLoading)};
	    Message(0,NULL,Items,_countof(Items),0);

        // These command lines are possible:
        //   file.xml:xpath
        //   c:\dir\file.xml:xpath
        //   http://site.com/file.xml://xpath

        tstring sFilePath( 
#ifdef FAR3
            OpenFrom == OPEN_SHORTCUT ? ((OpenShortcutInfo*)Item)->ShortcutData : 
#endif
                        filePath);
        tstring sSubDir;
        if(OpenFrom == OPEN_COMMANDLINE) {
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
	XMLPlugin* plugin = new XMLPlugin(sFilePath.c_str());
        if(!sSubDir.empty())
	    try {
                plugin->SetDirectory(sSubDir.c_str(),0);
	    }
	    catch(...) {}
	return plugin;
    }
    catch(_com_error e) {
	if(e.Error()) XMLPlugin::WinError(e);
	return INVALID_HANDLE_VALUE;
    }
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
  Info->CommandPrefix = _T("xml");

#ifdef FAR3
  static PluginMenuItem pluginMenuItem, configMenuItem;
  static PCWSTR pluginMenuString;
  pluginMenuString = GetMsg(MXMLBrwsr);

  if(SETTINGS_GET(sAddPluginsMenu, 0)) {
      pluginMenuItem.Guids = &MenuGuid;
      pluginMenuItem.Strings = &pluginMenuString;
      pluginMenuItem.Count = 1;
      Info->PluginMenu = pluginMenuItem;
  }

  configMenuItem.Guids = &MenuGuid;
  configMenuItem.Strings = &pluginMenuString;
  configMenuItem.Count = 1;

  Info->PluginConfig = configMenuItem;
#else  
  static PCTSTR menu;

  menu = GetMsg(MXMLBrwsr);
  Info->PluginConfigStrings = &menu;
  Info->PluginConfigStringsNumber = 1;

  if(SETTINGS_GET(sAddPluginsMenu, 0)) {
    Info->PluginMenuStrings = &menu;
    Info->PluginMenuStringsNumber = 1;
  }
#endif
}

#ifdef FAR3
int WINAPI XMLPlugin::ConfigureW(const ConfigureInfo *info)
{
    if(info->StructSize > sizeof *info /*|| memcmp(info->Guid, &MenuGuid, sizeof MenuGuid)*/)
        return FALSE;
#else
int WINAPI XMLPlugin::EXP_NAME(Configure)(int)
{
#endif
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
            // If params are equal names with indices, compare indices
            LPCTSTR br1 = _tcschr(Item1->FileName, '[');
            if(br1 != NULL)
            {
                LPCTSTR br2 = _tcschr(Item2->FileName, '[');
                if(br2 != NULL && br1 - Item1->FileName == br2 - Item2->FileName &&
                    !memcmp(Item1->FileName, Item2->FileName, br1 - Item1->FileName))
                {
                    int i1, i2;
                    if(_stscanf(br1+1, _T("%d]"), &i1) == 1 && _stscanf(br2+1, _T("%d]"), &i2) == 1)
                        return i1 - i2;
                }
            }
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
int WINAPI XMLPlugin::EXP_NAME(ProcessDialogEvent)(
                    const ProcessDialogEventInfo *info)
{
    if(info->StructSize != sizeof(ProcessDialogEventInfo) || info->Event != DE_DLGPROCINIT)
        return FALSE;
    FarDialogEvent& evt = *(FarDialogEvent *)info->Param;
    if(evt.Msg == DN_BTNCLICK && evt.Param1 == 15/*Override button*/)
    {
        PanelInfo panel;
        Control(FCTL_GETPANELINFO, 0, &panel);
        if(!panel.PluginHandle || panel.OwnerGuid != PluginGuid)
            return FALSE;
        ((XMLPlugin*)panel.PluginHandle)->EditSpecialDocs();
        return TRUE;
    }
    return FALSE;
}
#endif
