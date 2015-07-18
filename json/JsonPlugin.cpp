#include "StdAfx.h"
#include "version.h"
#include "json.h"
#include "jsonlang.h"

#include "..\ymsplugin.cpp"

static const TCHAR sJsonExt[] = _T(".json");
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

JsonPlugin::JsonPlugin(LPCTSTR lpFileName, LPCBYTE data) : buf(10000),
            sHostFile(lpFileName), pExtOnGet(sJsonExt)
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
    if(f == NULL)
        throw WinExcept();
    FileReadStream fs(f, &buf[0], buf.size());
    AutoUTFInputStream<unsigned, FileReadStream> autoStream(fs);
    doc.ParseStream<0, AutoUTF<unsigned>>(autoStream);
    bSourceHasBOM = autoStream.HasBOM();
    sourceUTFType = autoStream.GetType();

    curObject = &doc;    
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
    VK_F7, 0, -1,
    VK_F8, 0, -1,
    VK_F8, SHIFT_PRESSED, -1,
    VK_F3, LEFT_ALT_PRESSED|SHIFT_PRESSED, MViewGr,
//    VK_F4, LEFT_ALT_PRESSED|SHIFT_PRESSED, MViewGr,//MEditGr,
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
    
    auto fillItem = [this](PluginPanelItem& item, PCWSTR name, GenericValue<DocType>& value)
    {
        item.Owner = 0;
#ifdef UNICODE
        item.FileName = _wcsdup(name);
#else
        strncpy(item.FindData.cFileName, OEMString(name), sizeof item.FindData.cFileName);
#endif
        item.USERDATA = 
#ifdef FAR3
                (void*)
#else
                (DWORD_PTR)
#endif
                        value.GetType();
        DescStringBuffer sbuf(1000);
        PrettyWriter<DescStringBuffer, DocType, UTF16<> > pwriter(sbuf);
        pwriter.SetIndent(' ', 0);
        /*auto o =*/ value.Accept(pwriter);
        item.Description = MakeItemDesc(sbuf.GetString(), 1000);
        switch(value.GetType())
        {
            /*case Type::kStringType:
                item.FileSize = value.GetStringLength();
                break;*/
            case kArrayType:
            case kObjectType:
                if(bKeysAsDirs)
                    item.FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                break;
            default:
#ifdef UNICODE
                item.FileSize
#else
                item.FindData.nFileSizeLow
#endif
                        = _tcslen(item.Description);
                break;
        }
    };

    if(curObject->IsArray())
    {
        for(int i=0; i < itemCount; i++)
        {
            WCHAR name[10];
            _itow(i, name, 10);
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


void JsonPlugin::OnMakeFileName(PluginPanelItem& item)
{
    pExtOnGet = IsFolder(item) ? sJsonExt : _T("");
}

BOOL JsonPlugin::GoToPath(PCTSTR path)
{
    GenericPointer<GenericValue<DocType>> p((PWSTR)WideFromOem(path));
    auto v = p.Get(doc);
    if(!p.IsValid())
        return FALSE;

    if(v == NULL || !v->IsArray() && !v->IsObject())
        return FALSE;

    curObject = v;
    _tcsncpy(CurrentDir, path, _countof(CurrentDir));
    CurrentDir[_countof(CurrentDir) - 1] = 0;
    return TRUE;
}

void EncodeToBuf(PTSTR buf, PCTSTR dir, size_t size)
{
    //_tcsncpy(buf, dir, size);
    TCHAR* p = buf;
    for(size_t i = 0; i < size; i++)
    {
        char c = dir[i];
        switch(c)
        {
            case '/':
                *p++ = '~'; *p++ = '1';
                break;
            case '~':
                *p++ = '~'; *p++ = '0';
                break;
            default:
                *p++ = c;
                if(c == 0)
                    return;
                break;
        }
    }
}

BOOL JsonPlugin::SetDirectory(PCTSTR dir, int iOpMode)
{
    tstring path;
    if(!_tcscmp(dir, _T(".."))) // go to updir (..)
    {
        path = CurrentDir;
        TCHAR* pSlash = _tcsrchr(CurrentDir, '/');
        if(pSlash)
        {
            path = path.substr(0, pSlash - CurrentDir);
        }
        return GoToPath(path.c_str());
    }

    // First, try the name immediately
    if(curObject->IsObject())
    {
        auto mem = curObject->FindMember((PWSTR)WideFromOem(dir));
        if(mem != curObject->MemberEnd())
        {
            if(!mem->value.IsObject() && !mem->value.IsArray())
                return FALSE;
            curObject = &mem->value;
            _tcsncat(CurrentDir, _T("/"), _countof(CurrentDir));
            size_t l = _tcslen(CurrentDir);
            EncodeToBuf(CurrentDir + l, dir, _countof(CurrentDir) - l);
            CurrentDir[_countof(CurrentDir) - 1] = 0;
            return TRUE;
        }
    }
    else if(curObject->IsArray()) //try the value immediately
    {
        PTSTR end;
        long index = _tcstol(dir, &end, 10);
        if(*end == 0)
        {
            auto val = &(*curObject)[index];
            if(!val->IsObject() && !val->IsArray())
                return FALSE;
            curObject = val;
            _tcsncat(CurrentDir, _T("/"), _countof(CurrentDir));
            _tcsncat(CurrentDir, dir, _countof(CurrentDir));
            CurrentDir[_countof(CurrentDir) - 1] = 0;
            return TRUE;
        }
    }

    // Then, try without converting backslashes to slashes (there may be a name with backslashes)
    if(dir[0] != '/') //relative path, add CurrentDir first
    {
        path = CurrentDir;
        path += _T("/");
    }

    if(_tcscmp(dir, _T("/")))
        path += dir;

    if(GoToPath(path.c_str()))
        return TRUE;

    // Convert \ to / and retry
    path.clear();
    tstring tdir(dir);

    for(size_t i = 0; i < tdir.size(); i++)
        if(tdir[i] == '\\')
            tdir[i] = '/';

    if(tdir[0] != '/') //relative path, add CurrentDir first
    {
        path = CurrentDir;
        path += _T("/");
    }
    if(_tcscmp(tdir.c_str(), _T("/")))
        path += tdir;

    return GoToPath(path.c_str());
} //SetDirectory

void JsonPlugin::ExportItem(PluginPanelItem& item, PCTSTR filename, bool bAppend)
{
    GenericValue<DocType>::MemberIterator obj;
    if(_tcscmp(item.FileName, _T("..")) && curObject->IsObject())
    {
        obj = curObject->FindMember((PCWSTR)WideFromOem(item.FileName));
        if (obj == curObject->MemberEnd())
            throw ActionException();
    }

    FILE* f = _tfopen(filename, bAppend ? _T("a") : _T("w"));
    if(f == NULL)
        throw WinExcept();
    char buf[1000];
    FileWriteStream fstream(f, buf, sizeof buf);
    
    typedef AutoUTFOutputStream<unsigned, FileWriteStream> OutputStream;
    OutputStream os(fstream, sourceUTFType, /*bSourceHasBOM*/true);

    if(bAppend)
        fstream.Put('\n');
    
    auto value = (_tcscmp(item.FileName, _T("..")) ?
            (curObject->IsObject() ? &obj->value : &(*curObject)[_ttoi(item.FileName)]) :
             curObject);
    if(sourceUTFType == kUTF8)
        value->Accept(PrettyWriter<OutputStream, DocType, UTF8 <>>(os));
    else
        value->Accept(PrettyWriter<OutputStream, DocType, UTF16<>>(os));
    fstream.Flush();
    fclose(f);
}

BOOL JsonPlugin::ProcessKey(int key, unsigned int controlState)
{
    if(key == VK_F4 && controlState==(PKF_SHIFT|PKF_ALT) ) // don't pass AltShiftF4 for editing in this version
        return FALSE;

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
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
