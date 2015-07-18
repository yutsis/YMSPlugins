#include "StdAfx.h"
#include "version.h"
#include "json.h"
#include "jsonlang.h"

#include "..\ymsplugin.cpp"

static const TCHAR JSONEXT[] = _T(".json");
static const TCHAR sObjectsAsDirs[] = _T("ObjectsAsDirs");
TCHAR sExportUTF[] = _T("ExportUTF");
BOOL JsonPlugin::bExportUTF;
TCHAR sBakFiles[] = _T("BakFiles");
BOOL JsonPlugin::bBakFiles;
BOOL JsonPlugin::bStartUnsorted;

#define PARAM(_) TCHAR s##_[] = _T(#_);
PARAM(StartPanelMode)
extern TCHAR sStartUnsorted[];

InfoPanelLine infoPanelLines[] = {
    { _T(""), _T(""), 1 },
    { _T("        JSON Browser ")TVERSION, _T(""), 0},
#ifdef UNICODE
# ifdef _X86_
#  ifdef FAR3
    { _T("Unicode version (32-bit) for FAR 3"), _T(""), 0},
#  else
    { _T("Unicode version (32-bit) for FAR 2"), _T(""), 0},
#  endif
# else
#  ifdef FAR3
    { _T("Unicode version (64-bit) for FAR 3"), _T(""), 0},
#  else
    { _T("Unicode version (64-bit) for FAR 2"), _T(""), 0},
#  endif
# endif
#else
    { _T("       ANSI version (32-bit)     "), _T(""), 0},
#endif
    { _T("     :) Michael Yutsis, 2015"), _T(""), 0 },
};

JsonPlugin::JsonPlugin(LPCTSTR lpFileName, LPCBYTE data) : buf(10000), sHostFile(lpFileName), pCurrent(_T(""))
{
    bChanged = false;
    {
    USE_SETTINGS;
    bKeysAsDirs = settings.Get(sObjectsAsDirs, 1);
    bBakFiles = settings.Get(sBakFiles, 1);
    bStartUnsorted = settings.Get(sStartUnsorted, 1);
    iStartPanelMode = settings.Get(sStartPanelMode, 6) + '0';
    }
    //buf.resize(buf.capacity());
    f = _tfopen(lpFileName, _T("r"));
    FileReadStream fs(f, &buf[0], buf.size());
    doc.ParseStream(AutoUTFInputStream<unsigned, FileReadStream>(fs));
    curObject = &doc;
    pExtOnGet = JSONEXT;
}
JsonPlugin::~JsonPlugin()
{
    if(f)
        fclose(f);
    f = NULL;
}

KeyBarItem KeyBarItems[] = {
    VK_F6, 0, MCopy,
    //VK_F7, 0, MMkKey,
    VK_F6, LEFT_ALT_PRESSED, -1,
    VK_F2, SHIFT_PRESSED, MSave,
    VK_F6, SHIFT_PRESSED, MCopy,
    VK_F7, SHIFT_PRESSED, MToggle,
    VK_F3, LEFT_ALT_PRESSED|SHIFT_PRESSED, MViewGr,
    VK_F4, LEFT_ALT_PRESSED|SHIFT_PRESSED, MViewGr,//MEditGr,
};

//static PanelMode panelModes[10];

void JsonPlugin::GetOpenPluginInfo(OpenPluginInfo& info)
{
    YMSPlugin::GetOpenPluginInfo(info);
    info.Format = _T("JSON");
    info.HostFile = sHostFile.c_str();
    info.Flags = OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE
#ifndef UNICODE
        | OPIF_FINDFOLDERS
#endif
#ifdef FAR3
        | OPIF_SHORTCUT
#else
        | OPIF_USEHIGHLIGHTING
#endif
        ;
    sPanelTitle = _T("json:");
    if(bChanged) sPanelTitle += '*';
    sPanelTitle += sHostFile;
    if(bChanged) sPanelTitle += '*';
    if(*CurDir) {
        sPanelTitle += ':';
        sPanelTitle += CurDir;
    }
    info.PanelTitle = sPanelTitle.c_str();
    info.InfoLines = infoPanelLines;
    info.InfoLinesNumber = _countof(infoPanelLines);
    if(bStartUnsorted)
    {
        info.StartSortMode = SM_UNSORTED;
        info.StartSortOrder = 0;
    }
    info.StartPanelMode = iStartPanelMode;
    info.KeyBar = MakeKeyBarTitles(KeyBarItems, _countof(KeyBarItems));
}

#ifdef UNICODE
void JsonPlugin::FreeFileNames(PluginPanelItem& item)
{
    //delete item.FileName;
    free((void*)item.FileName);
}
#endif

class DescStringBuffer : public GenericStringBuffer<UTF16<> >
{
    typedef GenericStringBuffer<UTF16<> > Base;

public:
    DescStringBuffer(size_t capacity) : Base(0, capacity) {}

    void Put(Ch c)
    {
        if(c != '\n')
            Base::Put(c);
    }
};

void JsonPlugin::GetShortcutData(tstring& data)
{
    data.assign(sHostFile);
}

BOOL JsonPlugin::GetFindData(PluginPanelItem*& panelItems, int& itemCount, int OpMode)
{
    if(doc.HasParseError())
        return FALSE;

    if(curObject->IsArray())
    {
        itemCount = curObject->Size();
    }
    else if(curObject->IsObject())
    {
        itemCount = 0;
        for(auto m = curObject->MemberBegin(); m != curObject->MemberEnd(); ++m)
        {
            itemCount++;
        }
    }
    else
        return FALSE;

    panelItems = new PluginPanelItem[itemCount];
    memset(panelItems, 0, itemCount * sizeof PluginPanelItem);
    
    auto fillItem = [this](PluginPanelItem& item, PCTSTR name, GenericValue<DocType>& value)
    {
        item.Owner = 0;
#ifdef UNICODE
        item.FileName = _wcsdup(name);
#else
        strncpy(item.FindData.cFileName, name, sizeof item.FindData.cFileName);
#endif
        if(bKeysAsDirs && (value.IsArray() || value.IsObject()))
            item.FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        DescStringBuffer sbuf(1000);
        PrettyWriter<DescStringBuffer, DocType, UTF16<> > pwriter(sbuf);
        pwriter.SetIndent(' ', 0);
        /*auto o =*/ value.Accept(pwriter);
        item.Description = MakeItemDesc(sbuf.GetString(), 1000);
    };

    if(curObject->IsArray())
    {
        for(int i=0; i < itemCount; i++)
        {
            TCHAR name[10];
            _itot(i, name, 10);
            fillItem(panelItems[i], name, (*curObject)[i]);
        }
    }
    else if(curObject->IsObject())
    {
        int i = 0;
        for(auto m = curObject->MemberBegin(); m != curObject->MemberEnd(); ++m)
        {
            fillItem(panelItems[i++], m->name.GetString(), m->value);
        }
    }
    else
        return FALSE;

    return TRUE;
}//GetFindData

BOOL JsonPlugin::SetDirectory(PCTSTR dir, int iOpMode)
{
    tstring tdir(dir);
    tstring path;
    if(!_tcscmp(dir, _T("..")))
    {
        path = CurrentDir;
        TCHAR* pSlash = _tcsrchr(CurrentDir, '/');
        if(pSlash)
        {
            path = path.substr(0, pSlash - CurrentDir);
        }
    }
    else
    {
        for(size_t i = 0; i < tdir.size(); i++)
            if(tdir[i] == '\\')
                tdir[i] = '/';

        if(tdir[0] != '/') //relative
        {
            path = CurrentDir;
            path += _T("/");
        }
        if(_tcscmp(tdir.c_str(), _T("/")))
            path += tdir;
    }
    GenericPointer<GenericValue<DocType>> p(path.c_str());
    auto v = p.Get(doc);
    if(!p.IsValid())
        return FALSE;

    if(v == NULL || !v->IsArray() && !v->IsObject())
        return FALSE;

    curObject = v;
    _tcsncpy(CurrentDir, path.c_str(), _countof(CurrentDir));
    return TRUE;
} //SetDirectory

void JsonPlugin::ExportItem(PluginPanelItem& item, PCTSTR filename, bool bAppend)
{
    FILE* f = _tfopen(filename, bAppend ? _T("a") : _T("w"));
    char buf[1000];
    FileWriteStream fstream(f, buf, sizeof buf);
    PrettyWriter<FileWriteStream, DocType, DocType> pwriter(fstream);
    if(bAppend)
        fstream.Put('\n');
    /*auto o =*/ 
        (_tcscmp(item.FileName, _T("..")) ?
             (curObject->IsObject() ? (*curObject)[item.FileName] : (*curObject)[_ttoi(item.FileName)]) :
             *curObject)
             .Accept(pwriter);
    fstream.Flush();
    fclose(f);
}

BOOL JsonPlugin::ProcessKey(int key, unsigned int controlState)
{
    return YMSPlugin::ProcessKey(key, controlState);
}
int JsonPlugin::ProcessEvent(int event, void *param)
{
    return YMSPlugin::ProcessEvent(event, param);
}
int JsonPlugin::DeleteFiles(PluginPanelItem *PanelItem, size_t itemCount, int OpMode)
{
    return FALSE;
}

int JsonPlugin::PutFiles(PluginPanelItem *PanelItem,size_t itemCount,int Move,int OpMode,PCTSTR SrcPath)
{
    return FALSE;
}
int JsonPlugin::MakeDirectory(WCONST WTYPE Name, int OpMode)
{
    return FALSE;
}
