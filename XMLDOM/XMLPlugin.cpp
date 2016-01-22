#include "StdAfx.h"
#include <unordered_map>
#include "xmldom.h"
#include "xmldomlang.h"
#include "version.h"
#include "guids.h"

#include "..\ymsplugin.cpp"
#include <time.h>

static const TCHAR XMLEXT[] = _T(".xml");
static const TCHAR sNodesAsDirs[] = _T("NodesAsDirs");
TCHAR sExportUTF[] = _T("ExportUTF");
BOOL XMLPlugin::bExportUTF;
TCHAR sBakFiles[] = _T("BakFiles");
BOOL XMLPlugin::bBakFiles;
BOOL XMLPlugin::bStartUnsorted;
vector<_bstr_t> XMLPlugin::IdAttributes;

//char XMLPlugin::IdAttributes[256];
#define PARAM(_) TCHAR s##_[] = _T(#_);
PARAM(IdAttributes)
    TCHAR sDefIdAttributes[] = _T("Name,name,Id,id");
PARAM(StartPanelMode)
    PARAM(DisplayAttrsAsNames)
    PARAM(ShowNodeDesc)
    extern TCHAR sStartUnsorted[];

void XMLPlugin::LoadIdAttributes()
{
    TCHAR attrsBuf[256];
    PTSTR attrs = SETTINGS_GET(sIdAttributes, attrsBuf, _countof(attrsBuf), sDefIdAttributes);
    IdAttributes.clear();
    for(PTSTR p = _tcstok(attrs, _T(" \t,;")); p; p =_tcstok(NULL, _T(" \t,;")))
    {
        IdAttributes.push_back(p);
    }
}

InfoPanelLine fixedInfoPanelLine[] = {
    { _T(""), _T(""), 1 },
    { _T("        XML Browser ")TVERSION, _T(""), 0},
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
    { _T("  :) Michael Yutsis, 2003-2014"), _T(""), 0 },
    { _T(""), _T(""), 1 },
    { _T("XML DOM:"), _T(""), 0 },
};
/*
void XMLPlugin::GetRegCols()
{
GetReg("ColTypes5", ColTypes5, sizeof ColTypes5, "O,N,S,Z");
GetReg("ColWidths5", ColWidths5, sizeof ColWidths5, "10,16,4,0");
}*/

XMLPlugin::XMLPlugin(LPCTSTR lpFileName, LPCBYTE data) : XmlFile(lpFileName, data), sHostFile(lpFileName)
{
    bLocalHostFile = GetFileAttributes(sHostFile.c_str()) != 0xffffffff;
    bChanged = false;
    iUpDir = -1;
    iXPathSubDir = -1;
    pExtOnGet = XMLEXT;
    {
    USE_SETTINGS;
    bKeysAsDirs = settings.Get(sNodesAsDirs, 1);
    bExportUTF = settings.Get(sExportUTF, 1);
    bBakFiles = settings.Get(sBakFiles, 1);
    bDisplayAttrsAsNames = settings.Get(sDisplayAttrsAsNames, 1);
    bShowNodeDesc = settings.Get(sShowNodeDesc, 1);
    bStartUnsorted = settings.Get(sStartUnsorted, 1);
    iStartPanelMode = settings.Get(sStartPanelMode, 6) + '0';
    }
    if(iStartPanelMode>'9')
        iStartPanelMode = '6';
    LoadIdAttributes();
    // GetRegCols();

    CurNode = XmlFile;

    iFarVersion = GetFarVersion();
    LoadSpecialDocs();
#ifdef FAR3
    if(mapSpecialDocs.empty())
        LoadSpecialDocsFromReg(L"Software\\Far Manager\\Plugins\\YMS\\XMLDOM");    
    if(mapSpecialDocs.empty())
        LoadSpecialDocsFromReg(L"Software\\Far2\\Plugins\\YMS\\XMLDOM");
    if(mapSpecialDocs.empty())
        LoadSpecialDocsFromReg(L"Software\\Far\\Plugins\\YMS\\XMLDOM");
#endif

    // Add root attributes to infoPanelLine.
    NamedNodeMapPtr nodemap;
    long nAttrs=0;
    try
    {
        nodemap = XmlFile->documentElement->attributes;
        nAttrs = nodemap->length;
    }
    catch(...) {}
#define NFIXEDLINES _countof(fixedInfoPanelLine)
    nInfoLines = NFIXEDLINES + nAttrs + (nAttrs!=0);
    infoPanelLine = new InfoPanelLine[nInfoLines];
    memcpy(infoPanelLine, fixedInfoPanelLine, sizeof fixedInfoPanelLine);
    SetInfoData(infoPanelLine[5], XmlFile.GetMSXMLVersion());
    if(nAttrs)
    {
        memset(infoPanelLine+NFIXEDLINES, 0, sizeof *infoPanelLine * (nInfoLines-NFIXEDLINES));
#ifdef FAR3
        infoPanelLine[NFIXEDLINES].Flags = IPLFLAGS_SEPARATOR;
#else
        infoPanelLine[NFIXEDLINES].Separator = 1;
#endif
        SetInfoText(infoPanelLine[NFIXEDLINES], GetMsg(MRootAttributes)); 
        try
        {
            for(long i=0; i<nAttrs; i++)
            {
                NodePtr node = nodemap->item[i];
                /*_tcsncpy(infoPanelLine[NFIXEDLINES+1+i].Text,
                (PCTSTR)node->nodeName, _countof infoPanelLine->Text-1);
                _tcsncpy(infoPanelLine[NFIXEDLINES+1+i].Data,
                (PCTSTR)node->text, _countof infoPanelLine->Data-1);*/
#ifdef UNICODE
#define DUPIFUNICODE(s) _wcsdup(s)
#else
#define DUPIFUNICODE(s) s
#endif
                SetInfoText(infoPanelLine[NFIXEDLINES+1+i], DUPIFUNICODE(node->nodeName));
                SetInfoData(infoPanelLine[NFIXEDLINES+1+i], DUPIFUNICODE(node->text));
            }
        }
        catch(...) {}
    }
}
XMLPlugin::~XMLPlugin()
{
    SETTINGS_SET(sNodesAsDirs, bKeysAsDirs);
#ifdef UNICODE
    for(int i=NFIXEDLINES+1; i<nInfoLines; i++)
    {
        delete[] infoPanelLine[i].Text;
        delete[] infoPanelLine[i].Data;
    }
#endif
    delete[] infoPanelLine;
}

KeyBarItem KeyBarItems[] = {
    VK_F6, 0, MCopy,
    //VK_F7, 0, MMkKey,
    VK_F6, LEFT_ALT_PRESSED, -1,
    VK_F2, SHIFT_PRESSED, MSave,
    VK_F3, SHIFT_PRESSED, MViewDef,
    VK_F6, SHIFT_PRESSED, MCopy,
    VK_F7, SHIFT_PRESSED, MToggle,
    VK_F3, LEFT_ALT_PRESSED|SHIFT_PRESSED, MViewGr,
    VK_F4, LEFT_ALT_PRESSED|SHIFT_PRESSED, MViewGr,//MEditGr,
    VK_F11, LEFT_ALT_PRESSED|SHIFT_PRESSED, MSpecial,
    VK_F5, LEFT_CTRL_PRESSED|SHIFT_PRESSED, MDup,
};

//static PanelMode panelModes[10];

void XMLPlugin::GetOpenPluginInfo(OpenPluginInfo& info)
{
    YMSPlugin::GetOpenPluginInfo(info);

    if(iXPathSubDir==0) // prevent from exiting on XPath from root
        _tcsncpy(CurDir, sCurXPath.c_str(), _countof(CurDir));
    info.Format = _T("Xml");
    info.HostFile = bLocalHostFile ? sHostFile.c_str() : NULL;
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

    sPanelTitle = _T("Xml:");
    if(bChanged) sPanelTitle += '*';
    sPanelTitle += sHostFile;
    if(bChanged) sPanelTitle += '*';
    if(iXPathSubDir == 0) {
        sPanelTitle += ':';
        sPanelTitle += sCurXPath;
    } else if(*CurDir) {
        sPanelTitle += ':';
        sPanelTitle += CurDir;
    }
    info.PanelTitle = sPanelTitle.c_str();
    info.InfoLines = infoPanelLine;
    info.InfoLinesNumber = nInfoLines;
    if(bStartUnsorted)
    {
        info.StartSortMode = SM_UNSORTED;
        info.StartSortOrder = 0;
    }
    info.StartPanelMode = iStartPanelMode;
    /*panelModes[5].ColumnTypes = ColTypes5;
    panelModes[5].ColumnWidths = ColWidths5;
    panelModes[5].FullScreen = 1;
    info.PanelModesArray = panelModes;
    info.PanelModesNumber = _countof(panelModes);*/

    info.KeyBar = MakeKeyBarTitles(KeyBarItems, _countof(KeyBarItems));
}

void XMLPlugin::GetShortcutData(tstring& data)
{
    data.assign(sHostFile);
}

bool XMLPlugin::GetNodeNameFromAttr(NodePtr& node, _bstr_t& sValue)
{
    NamedNodeMapPtr nodemap = node->attributes;
    for(DWORD i=0; i<IdAttributes.size(); i++) {
        NodePtr nameItem = nodemap->getNamedItem(IdAttributes[i]);
        if(nameItem) {
            sValue = nameItem->nodeValue;
            return true;
        }
    }
    return false;
}

class NodesProgressAction : public ProgressAction, SaveScreen
{
    int msgid;
public:
    DWORD dwRead, dwTotal;
    NodesProgressAction(int id=MLoading) { msgid = id; }
    virtual PTSTR ProgressMessage(PTSTR buf)
    {
#ifdef UNICODE
        wmemset(buf,L'\x2591',30);
        wmemset(buf,L'\x2588', dwRead*30/dwTotal);
#else
        memset(buf,'°',30);
        memset(buf,'Û', dwRead*30/dwTotal);
#endif
        buf[30] = 0;
        return buf;
    }
    virtual LPCTSTR ProgressTitle() { return GetMsg(msgid); }
};

static void ParseTime(PCWSTR sTime, FILETIME& ft)
{
    // Check time_t
    wchar_t* pEnd;
    __time64_t t = _wcstoi64(sTime, &pEnd, 10);
    if(*pEnd == 0 && pEnd > sTime) // succeeded
    {
        int fs = 0;
        if(t > 0x1FFFFFFFF)
        { // assume it includes milliseconds
            fs = t % 1000;
            t /= 1000;
        }
        tm* tm = _localtime64(&t);
        SYSTEMTIME st = { tm->tm_year + 1900,tm->tm_mon + 1,tm->tm_wday,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec,fs};
        SystemTimeToFileTime(&st, &ft);
        return;
    }

    // Check time_t in milliseconds

    int d=-1,mon=-1,y=-1,h=-1,m=-1,s=-1,dow=-1;

    wchar_t month[4],tz[6],dayofweek[4];
    swscanf(sTime, L"%3s, %d %3s %d %d:%d:%d %5s", &dayofweek,&d,&month,&y,&h,&m,&s, &tz);
    wchar_t months[12][4] = {L"Jan",L"Feb",L"Mar",L"Apr",L"May",L"Jun",L"Jul",L"Aug",L"Sep",L"Oct",L"Nov",L"Dec"};
    wchar_t days[7][4] = {L"Sun",L"Mon",L"Tue",L"Wed",L"Thu",L"Fri",L"Sat"};
    for(int i=0; i<12; i++)
        if(!wcscmp(month,months[i])) { mon=i+1; break; }
    if(mon >= 0)
    {
        for(int ii=0; ii<7; ii++)
            if(!wcscmp(dayofweek,days[ii])) { dow = ii; break; }
        SYSTEMTIME st = { y,mon,dow,d,h,m,s,0}/*, stloc*/;
        //SystemTimeToTzSpecificLocalTime(0, &st, &stloc);
        SystemTimeToFileTime(&st, &ft);
    }
    else
    {
        //try another format: "2009-08-05T07:07:45Z"
        float fs;
        swscanf(sTime, L"%4d-%2d-%2dT%d:%d:%fZ", &y, &mon, &d, &h, &m, &fs);
        if(fs < 0 || fs > 60)
            fs = 0;
        SYSTEMTIME st = { y,mon,0,d,h,m,(WORD)fs,(WORD)((fs-(WORD)fs)*1000)};
        SystemTimeToFileTime(&st, &ft);
    }
}

#ifdef UNICODE
void XMLPlugin::FreeFileNames(PluginPanelItem& item)
{
    delete[] item.FileName;
}
#endif

BOOL XMLPlugin::GetFindData(PluginPanelItem*& PanelItem, int& itemCount, int OpMode)
{
    BOOL bRet = TRUE;
    PanelItem = NULL;
    try
    {
        //Get list of nodes
        NodeListPtr childList = iXPathSubDir ? CurNode->childNodes : XPathSelection;
        //node.Release();
        long nNodes = childList->length;

        long nAttrs=0;
        NamedNodeMapPtr nodemap;

        if(iXPathSubDir!=0)
            // Get list of attributes
            try {
                nodemap = CurNode->attributes;
                nAttrs = nodemap == NULL ? 0 : nodemap->length;
        } catch(_com_error) {}
        PanelItem = new PluginPanelItem[nNodes+nAttrs];
        memset(PanelItem, 0, (nNodes+nAttrs)*sizeof PluginPanelItem);

        long i;
        // Attributes first...
        for (i=0; i<nAttrs; i++)
        {
            PluginPanelItem& item = PanelItem[i];
            NodePtr node = nodemap->item[i];
            _bstr_t name = node->nodeName;
#ifdef UNICODE
            item.FileName = _wcsdup(name);
#else
            strncpy(item.FindData.cFileName, (PCSTR)name, sizeof PanelItem->FindData.cFileName);
#endif
            item.USERDATA = 
#ifdef FAR3
                (void*)
#endif
                i; // store attr number
            _bstr_t val = node->nodeValue;
            if(val.length()) {
#ifdef UNICODE
                item.FileSize = val.length();
#else
                item.FindData.nFileSizeLow = val.length();
#endif
                item.Description = MakeItemDesc(val.GetBSTR());
            }
            ToOem(item.FindData.cFileName);
        }
        // Then nodes
        NodesProgressAction npa;
        npa.dwTotal = nNodes;
        if(nNodes > 0)
        {
            childList->reset();
            tstring sCurPath;
            GetNodeXPath(CurNode, sCurPath);
            map<_bstr_t, DocumentPtr> xsltDocMap;
            
            unordered_map<wstring, pair<int, PluginPanelItem*>> nameMap;
            
            for (i=0; i<nNodes; i++)
            {
                PluginPanelItem& item = PanelItem[nAttrs+i];
                npa.dwRead = i;
                try {
                    npa.ShowProgress();
                } catch(WinExcept) {
                    WinError();
                    nNodes = i;
                    break;
                }
                NodePtr node = childList->nextNode();
                if(node==NULL) {
                    nNodes = i;
                    break;
                }

                NodeType type = node->nodeType;
                bool bTrueNode = type!=MSXML2::NODE_COMMENT && type!=MSXML2::NODE_TEXT && type!=MSXML2::NODE_CDATA_SECTION && type!=MSXML2::NODE_ATTRIBUTE
                    && type!=MSXML2::NODE_PROCESSING_INSTRUCTION && type!=MSXML2::NODE_NOTATION && type!=MSXML2::NODE_DOCUMENT_TYPE;
                bool bIndexedNode = bTrueNode || type==MSXML2::NODE_COMMENT;

                _bstr_t name = node->nodeName;
                wstring wsname(name.GetBSTR());
                auto res = nameMap.find(wsname);
                if(res == nameMap.end())
                {
                    nameMap[wsname] = pair<int, PluginPanelItem*>(bIndexedNode ? 1 : 0, bIndexedNode ? &item : NULL);
                }
                else
                {
                    int count = res->second.first;
                    if(count < 2) // Append index to [0]
                    {
                        auto item0 = res->second.second;
                        if(item0 != NULL)
                        {
                            tstring name0(item0->Owner);
                            bool bNameToo = _tcscmp(item0->FileName, item0->Owner) == 0;
                            delete[] item0->Owner;
                            item0->Owner = MakeItemDesc((name0 + _T("[0]")).c_str());
                            if(bNameToo)
                            {
#ifdef UNICODE
                                delete[] item0->FileName;
                                item0->FileName = _tcsdup(item0->Owner);
#else
                                strncpy(item0->FindData.cFileName, item0->Owner, sizeof item0->FindData.cFileName);
#endif
                            }
                        }
                    }

                    wchar_t buf[16];
                    _snwprintf(buf, _countof(buf), L"[%d]", count);
                    if(bIndexedNode)
                        res->second.first++;
                    wsname += buf;
                }
                item.Owner = MakeItemDesc(wsname.c_str());
                // first check if name is overridden; if not, we'll assign it to FileName

                const SpecialFields* special=0;
                auto it = mapSpecialDocs.find(node->nodeName);
                if(it != mapSpecialDocs.end())
                    special = &it->second;

                item.USERDATA =
#ifdef FAR3
                    (void*)
#endif
                        (i | USERDATA_DIR);

                if(bTrueNode && bKeysAsDirs)
                    item.FileAttributes = FILE_ATTRIBUTE_DIRECTORY;

                if(bDisplayAttrsAsNames && bTrueNode)
                try
                {
                    _bstr_t sNameValue;
                    bool bOK=false;
                    if(special && special->sNameFld.length()) {
                        sNameValue = EvalXPath(node, special->sNameFld, xsltDocMap);
                        if(sNameValue.length() > 0)
                            bOK = true;
                    }
                    if(!bOK && GetNodeNameFromAttr(node, sNameValue))
                        bOK = true;
                    if(bOK) {
#ifdef UNICODE
                        item.FileName = _wcsdup(sNameValue);
#else
                        strncpy(item.FindData.cFileName, (PCSTR)sNameValue, sizeof item.FindData.cFileName);
                        ToOem(item.FindData.cFileName);
#endif
                    }
                } catch(...) { }
#ifdef UNICODE
                if(!item.FileName)
                    item.FileName = _wcsdup(item.Owner);
#else
                if(!*item.FileName)
                    strncpy(item.FindData.cFileName, item.Owner, sizeof item.FindData.cFileName-1);
#endif
                if(bShowNodeDesc || nNodes<NNODES_HIDE_DESC)
                    try {
                        _bstr_t text;
                        if(bDisplayAttrsAsNames && special && special->sDescriptionFld.length())
                            try {
                                text = EvalXPath(node, special->sDescriptionFld, xsltDocMap);
                        }
                        catch(_com_error err) {
                            PCTSTR dest = err.Description();
                        }
                        if(!text.length())
                            text = type==MSXML2::NODE_DOCUMENT_TYPE ? node->xml : node->text;
                        if(text.length())
                        {
                            item.Description = MakeItemDesc(text.GetBSTR(), 256);
                            if(!bTrueNode)
                            {
#ifdef UNICODE
                                item.FileSize = text.length();
#else
                                item.FindData.nFileSizeLow = text.length();
#endif
                            }
                        }
                        if(bDisplayAttrsAsNames && special && special->sOwnerFld.length()) {
                            _bstr_t text1;
                            try {
                                text1 = EvalXPath(node, special->sOwnerFld, xsltDocMap);
                            } catch(...) {}
                            if(text1.length())
                            {
                                delete[] item.Owner;
                                item.Owner = MakeItemDesc(text1.GetBSTR(), 256);
                            }
                        }
                } catch(...) {}
                if(special && special->sDateFld.length())
                {
                    try
                    {
                        // Fill in the date
                        _bstr_t text = EvalXPath(node, special->sDateFld, xsltDocMap);
                        if(text.length() > 0)
                            ParseTime(text, item.LastWriteTime);
                    } catch(...) {}
                }
            }//for (i=0; i<nNodes; i++)
        }//if(nNodes > 0) 
        itemCount = nNodes+nAttrs;
    }
    catch(_com_error e) {
        WinError(e);
        delete[] PanelItem;
        bRet = FALSE;
    }
    return bRet;
}//GetFindData

void XMLPlugin::SetCurNode(NodePtr& newnode, PCTSTR Dir)
{
    CurNode.Release();
    CurNode = newnode;
    newnode.Detach();
    if(Dir && !sCurXPath.empty())
        iXPathSubDir++;
    tstring s;
    GetNodeXPath(CurNode, s);
    _tcsncpy(CurrentDir, s.c_str(), _countof(CurrentDir));
    /*if(!*CurrentDir && *Dir && CurNode==XmlFile)
    strcpy(CurrentDir, "/");*/
}

bool XMLPlugin::SelectXPath(PCTSTR pXPath, bool bCD)
    // bCD = true if we invoke this from SetDirectory
    //   i.e. when there's only one item, enter into it
{
    if(pXPath)
    {
        try
        {
            SelectionPtr XPathSel = CurNode->selectNodes((PWSTR)WideFromOem(pXPath));
            if(bCD)
                switch(XPathSel->length)
                {
                    case 0:
                        return false; // empty selection, do nothing
                    case 1:
                        SelectXPath(NULL);
                        SetCurNode(XPathSel->nextNode());
                        return true;
                }
            if(XPathSelection)
                XPathSelection.Release();
            XPathSelection = XPathSel;
            sCurXPath = pXPath;
            iXPathSubDir = 0;
        }
        catch(_com_error e)
        {
            _bstr_t bstrResult = EvalXPath(CurNode, pXPath, map<_bstr_t, DocumentPtr>());
            if(bstrResult.length() == 0)
                YMSPlugin::WinError(GetMsg(MXpathEvalResultIsEmpty));
            else
            {
                PCTSTR items[] = { GetMsg(MXpathEvalResult), _T(""), bstrResult, _T(""), GetMsg(MOK) };
                Message(0, NULL, items, _countof(items), 1);
            }
            return false;
        }
    }
    else
    {
        if(XPathSelection)
            XPathSelection.Release();
        sCurXPath.clear();
        iXPathSubDir = -1;
    }
    return true;
}

BOOL XMLPlugin::SetDirectory(PCTSTR Dir, int iOpMode)
{
    if(!_tcscmp(Dir, _T("..")))
    {
        if(!sCurXPath.empty()) {
            if(iXPathSubDir==0)
            {
                SetCurNode(XPathSelection->context);
                SelectXPath(NULL);		
                return TRUE;
            }
            if(--iXPathSubDir==0) // up to selection root
            {
                XPathSelection->reset();
                iUpDir = -1;
                for(int i=0; ; i++)
                {
                    NodePtr node = XPathSelection->nextNode();
                    if(node==NULL)
                        break;
                    if(node==CurNode) {
                        iUpDir = i; // now iUpDir points in XPath selection set
                        break;
                    }
                }
                return TRUE;
            }
        }
        NodePtr parent = CurNode->parentNode;
        // What is the number of CurNode in parent?
        NodeListPtr childList = parent->childNodes;
        for(int i=0; ; i++)
        {
            NodePtr node = childList->nextNode();
            if(node==NULL)
                break;
            if(node==CurNode)
                iUpDir = i;
        }
        SetCurNode(parent);
#ifdef FAR3
        if(!(iOpMode & (OPM_SILENT|OPM_FIND)))
            Redraw(dwParentItem);
#endif
        /*char*p = strrchr(CurrentDir, '\\');
        if(p) *p = 0;
        else *CurrentDir = 0;*/
        return TRUE;
    }
    dwParentItem = 0xffffffff;
    if(!_tcscmp(Dir, _T("\\")))
    {
        SetCurNode(static_cast<NodePtr>(XmlFile));
        *CurrentDir = 0;
        SelectXPath(0);
        return TRUE;
    }
    if(*Dir=='\\')
        return SetDirectory(_T("\\"), iOpMode) && SetDirectory(Dir+1, iOpMode);

    if(_tcschr(Dir, '\\'))
    {
        // Dir==token1\token2\token3
        for(PTSTR token = _tcstok(const_cast<PTSTR>(Dir), _T("\\")); token; token = _tcstok(NULL, _T("\\")))
            SetDirectory(token, iOpMode);
        return TRUE;
    }

    // search for node named Dir
    NodeListPtr childList = iXPathSubDir ? CurNode->childNodes : XPathSelection;
    //long nNodes = childList->length;

    // Check if current item on the panel is the same as Dir.
    PluginPanelItem* CurItem = GetCurrentItem();
    if(CurItem != NULL)
    {
        if(((DWORD)CurItem->USERDATA & USERDATA_DIR) && !_tcscmp(CurItem->FileName, Dir)) {
            //if yes, use index stored in it
            DWORD iItem = (DWORD)CurItem->USERDATA & ~USERDATA_DIR;
            SetCurNode(childList->item[iItem], Dir);
            dwParentItem = (DWORD)CurItem->USERDATA;
            return TRUE;
        }
    }
    NodePtr node;
    for(childList->reset(); (node = childList->nextNode()) != NULL; node.Release())
    {
        // match by attr "name" or "id"
        try
        {
            _bstr_t sNameValue;
            if(GetNodeNameFromAttr(node, sNameValue) && 
                //vNameValue.vt!=VT_EMPTY && 
                !_tcsicmp((PCTSTR)sNameValue, Dir)) {
                    SetCurNode(node,Dir);
                    return TRUE;
            }
        } catch(...) { }
        // match by tag
        _bstr_t name = node->nodeName;
        if(!_tcsicmp((PCTSTR)name, Dir))
        {
            SetCurNode(node,Dir);
            return TRUE;
        }	
    }
    // maybe, it's XPath?
    if(!_tcschr(Dir,'/') && !_tcschr(Dir,'['))
        return FALSE; //just name, we've checked it
    try {
        return SelectXPath(Dir, true);
    } catch(_com_error e) {
        if(!(iOpMode & OPM_SILENT))
            WinError(e);
        return FALSE;
    }
} //SetDirectory

PTSTR XMLPlugin::MakeFileName(PCTSTR DestPath, PCTSTR fname, int OpMode)
{
    return YMSPlugin::MakeFileName(DestPath, (OpMode & OPM_SILENT) ? ExportNameSuffix : fname, OpMode);
}

void XMLPlugin::WriteXML(PCTSTR filename, _bstr_t& xml, bool bAppend)
{
    PWCHAR p = xml.GetBSTR();
    bool bIntlChars = false;
    while(*p) if(*p++ > L'\x7e') { bIntlChars = true; break; }
    if(!bAppend && (bExportUTF || !bIntlChars))
    {
        PWCHAR wxml = xml.GetBSTR();;
        DWORD l = (wcslen(wxml)+1)*2;
        char* pOut = new char[l+3];
        *(DWORD*)pOut = 0xbfbbef; //byte-order mark
        WideCharToMultiByte(CP_UTF8, 0, wxml, -1, pOut + (bIntlChars?3:0), l, 0,0);
        WriteBufToFile(filename, (BYTE*)pOut, strlen(pOut));
        delete[] pOut;
    }
    else
    {
        char* pXml = (char*)xml;
        if(bAppend)
            WriteBufToFile(filename, (BYTE*)"\r\n", 2, true);
        else 
            if(_memicmp(pXml,"<?xml ",6)) {
                char buf[60];
                wsprintfA(buf, "<?xml version=\"1.0\" encoding=\"windows-%d\"?>\r\n", GetACP());
                WriteBufToFile(filename, (BYTE*)buf, strlen(buf));
                bAppend = true;
            }
            WriteBufToFile(filename, (BYTE*)pXml, strlen(pXml), bAppend);
    }    
}

bool isRawExport(NodeType type)
{
    return type==MSXML2::NODE_COMMENT || type==MSXML2::NODE_TEXT ||
        type==MSXML2::NODE_CDATA_SECTION || type==MSXML2::NODE_ATTRIBUTE ;
}

void XMLPlugin::ExportItem(PluginPanelItem& item, PCTSTR filename, bool bAppend)
{
    try
    {
        if(!_tcscmp(item.FileName, _T("..")))
        {
            if(iXPathSubDir==0)
            {
                long nNodes = XPathSelection->length;
                NodesProgressAction npa(MExporting);
                npa.dwTotal = nNodes;
                XPathSelection->reset();
                for (int i=0; i<nNodes; i++)
                {
                    npa.dwRead = i;
                    try {
                        npa.ShowProgress();
                    } catch(WinExcept) {
                        WinError();
                        break;
                    }
                    NodePtr node = XPathSelection->nextNode();
                    if(node==NULL)
                        break;
                    if(isRawExport(node->nodeType))
                    {
                        _bstr_t text = node->text;
                        text+=L"\r\n";
                        WriteBufToFile(filename, (BYTE*)(PCTSTR)text, text.length(), i>0);
                    }
                    else
                        WriteXML(filename, node->xml, i>0);
                }
            }
            else if(!*CurrentDir)
                XmlFile.Save(filename);
            else
                //export current node
                WriteXML(filename, CurNode->xml);
        }
        else
        {
            NodeListPtr childList = iXPathSubDir ? CurNode->childNodes : XPathSelection;
            int iItem = (int)item.USERDATA & ~USERDATA_DIR;
            NodePtr node;
            if((DWORD)item.USERDATA & USERDATA_DIR)
            { // node
                node = childList->item[iItem];
                if(isRawExport(node->nodeType))
                {
		    _bstr_t text;
                    if(bAppend)
                        text = L"\r\n" + node->text;
                    else
#ifdef UNICODE
                        text = L"\xfeff" + node->text;
#else
                        text = node->text;
#endif
                    WriteBufToFile(filename, (BYTE*)(PCTSTR)text, text.length()*sizeof(TCHAR), bAppend);
                }
                else
                    WriteXML(filename, node->xml, bAppend);
            }
            else
            { //attribute
                _bstr_t text = CurNode->attributes->item[iItem]->nodeValue;
                if(bAppend)
                    text = L"\r\n" + text;
#ifdef UNICODE
                else
                    text = L"\xfeff" + text;
#endif
                WriteBufToFile(filename, (BYTE*)(PCTSTR)text, text.length()*sizeof(TCHAR), bAppend);
            }
        }
    }
    catch(_com_error e) {
        WinError(e);
        throw ActionException();
    }
}

void XMLPlugin::GetItemFullPath(NodeListPtr& list, PluginPanelItem& item, tstring& s)
{
    if((DWORD)item.USERDATA & USERDATA_DIR) {
        NodePtr node = list->item[(DWORD)item.USERDATA & ~USERDATA_DIR];
        GetNodeXPath(node, s);
    } else { //attr
        s = CurrentDir;
        s += _T("/@");
        s += item.FileName;
    }
}

#ifdef UNICODE
inline PluginPanelItem *GetPanelItem(size_t i, vector<BYTE>& vpi)
{
    size_t sz = StartupInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, (int)i, NULL);
    if(sz >= vpi.size())
        vpi.resize(sz);
    PluginPanelItem *ppi = (PluginPanelItem*)&vpi[0];
#ifdef FAR3
    FarGetPluginPanelItem fgppi = { sizeof fgppi, sz, ppi };
    StartupInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, (int)i, &fgppi);
#else
    StartupInfo.PanelControl(PANEL_ACTIVE, FCTL_GETPANELITEM, (int)i, ppi);
#endif
    return ppi;
}
#endif

void SetOrDelValue(PluginSettings& settings, KEY_TYPE key, PCWSTR valName, PCWSTR str)
{
    if(str && *str)
        settings.Set(valName, str, key);
    else
        settings.DeleteValue(valName, key);
}

void XMLPlugin::Redraw(DWORD currentID)
{
    // Keep cursor position
    DWORD dwKeepUD = currentID == 0xffffffff ? (DWORD)GetCurrentItem()->USERDATA : currentID;

#ifdef UNICODE
    Control(FCTL_UPDATEPANEL, 1, NULL);
#else
    Control(FCTL_UPDATEPANEL, (void*)1);
#endif

    // Redraw and restore position (FAR can't do it if several similar items are present)
    PanelInfo panel;
    Control(FCTL_GETPANELINFO, &panel);
    size_t currentItem = panel.CurrentItem;
#ifdef UNICODE
    vector<BYTE> vpi;
#endif
    for(size_t i=0; i<(size_t)panel.ItemsNumber; i++)
    {
#ifdef UNICODE
        if((DWORD)GetPanelItem(i, vpi)->USERDATA==dwKeepUD) {
#else
        if(panel.PanelItems[i].USERDATA==dwKeepUD) {
#endif
            currentItem = i;
            break;
        }
    }
    ControlRedraw(currentItem, panel.TopPanelItem);
}

#ifdef UNICODE
#define OemBStr _bstr_t
#else
#define OemBStr OEMString
#endif

BOOL XMLPlugin::ProcessKey(int Key, unsigned int ControlState)
{
    if(Key == VK_F4 && ControlState == (PKF_SHIFT|PKF_ALT))
        Key = VK_F3;

    try
    {
        if(YMSPlugin::ProcessKey(Key, ControlState))
            return TRUE;

        if(Key==VK_F2 && ControlState==PKF_SHIFT)
        {
            if(!bChanged) return TRUE;
            //Save
            if(bBakFiles)
            {
                tstring bakname = sHostFile + _T(".bak");
                DeleteFile(bakname.c_str());
                if(!MoveFile(sHostFile.c_str(), bakname.c_str()))
                    throw WinExcept();
            }
            {
                SaveScreen ss;
                PCTSTR Items[]={_T(""),GetMsg(MSaving)};
                Message(0,NULL,Items,_countof(Items),0);
                XmlFile.Save(sHostFile.c_str());
                bChanged = false;
            }
            Reread();
            return TRUE;
        }
        if(Key==VK_F3 && ControlState==PKF_SHIFT)
        {
            PluginPanelItem& currentItem = *GetCurrentItem();
            DWORD dwUD = (DWORD)currentItem.USERDATA;
            NodePtr node;
            if(dwUD&USERDATA_DIR)
            {
                NodePtr node1 = (iXPathSubDir ? CurNode->childNodes : XPathSelection)
                    ->item[dwUD&~USERDATA_DIR];
                node = node1->definition;
                if(node==0 /*&& node1==XmlFile->documentElement*/)
                    node = XmlFile->doctype;
                if(node==0)
                {
                    _bstr_t ns = node1->namespaceURI;
                    if(ns.length() > 0)
                        node = XmlFile->namespaces->get(ns);
                }
                if(node==0)
                {
                    _bstr_t ns = XmlFile->documentElement->namespaceURI;
                    if(ns.length() > 0)
                        node = XmlFile->namespaces->get(ns);
                }
            }
            else
                if(iXPathSubDir && _tcscmp(currentItem.FileName, _T("..")))
                    node = CurNode->attributes->item[dwUD]->definition;
            if(node!=0)
            {
                TCHAR TmpPath[MAX_PATH];
                CreateTmpDir(TmpPath);
                WriteXML(MakeFileName(TmpPath, currentItem.Owner,0), node->xml, false);
                Viewer(LastMadeName, currentItem.Owner);
            }
            return TRUE;
        }
        if(Key==VK_F5 && ControlState==(PKF_SHIFT | PKF_CONTROL))
        {
            PanelInfo panel;
            Control(FCTL_GETPANELINFO, &panel);
            NodeListPtr list = iXPathSubDir ? CurNode->childNodes : XPathSelection;
            vector<NodePtr> selectedNodes;
#ifdef UNICODE
            vector<vector<BYTE>> item_buf((size_t)panel.SelectedItemsNumber);
#endif
            for(size_t i=0; i<(size_t)panel.SelectedItemsNumber; i++)
            {
#ifdef UNICODE
                PluginPanelItem& item = *GetSelectedPanelItem(i, item_buf[i]);
#else
                PluginPanelItem& item = panel.SelectedItems[i];
#endif
                if(((DWORD)item.USERDATA & USERDATA_DIR) == 0)
                    continue;
                NodePtr node = list->item[(DWORD)item.USERDATA & ~USERDATA_DIR];
                selectedNodes.push_back(node);
            }

            if(selectedNodes.size() == 0)
                return TRUE;

            TCHAR MsgN[256];
            LPCTSTR MsgItems[] = { NULL, MsgN, GetMsg(MOK), GetMsg(MCancel) };
            if(selectedNodes.size() == 1)
                _sntprintf(MsgN, _countof(MsgN), GetMsg(MDupConfirm1), (PWSTR)WideFromOem(selectedNodes[0]->nodeName));
            else
                _sntprintf(MsgN, _countof(MsgN), GetMsg(MDupConfirmN), selectedNodes.size());
            if (Message(0,NULL,MsgItems,_countof(MsgItems),2)!=0)
                return TRUE;

            for each(auto node in selectedNodes) // if you want to compile this outside VC++, it’s *your* problem
            {
                auto clone = node->cloneNode(VARIANT_TRUE);
                node->parentNode->insertBefore(clone, _variant_t(node, true));
            }

            bChanged = true;
            Reread(false, true, false);
            return TRUE;
        }
        if(Key==VK_F11 && ControlState==(PKF_SHIFT | PKF_ALT))
        {
            EditSpecialDocs();
            return TRUE;
        }
        if(Key=='N' && ControlState==PKF_CONTROL) {
            bDisplayAttrsAsNames ^= 1;
            Redraw();
            return TRUE;
        }
        if(Key=='H' && ControlState==PKF_CONTROL)
        {
            bShowNodeDesc ^= 1;
            SETTINGS_SET(sShowNodeDesc, bShowNodeDesc);
            Reread();
            return TRUE;
        }
        if(Key==VK_INSERT && ControlState==(PKF_SHIFT | PKF_ALT) && iFarVersion>-1)
        {
            PanelInfo panel;
            Control(FCTL_GETPANELINFO, &panel);
            NodeListPtr list = iXPathSubDir ? CurNode->childNodes : XPathSelection;
            tstring str;
            for(size_t i=0; i<(size_t)panel.SelectedItemsNumber; i++)
            {
                tstring s;
#ifdef UNICODE
                vector<BYTE> selItem;
                GetItemFullPath(list, *GetSelectedPanelItem(0, selItem), s);
#else
                GetItemFullPath(list, panel.SelectedItems[i], s);
#endif
                str += s;
                str += _T("\r\n");
            }
            StartupInfo.FSF->CopyToClipboard(
#ifdef FAR3
                FCT_STREAM,
#endif
                str.c_str());
            return TRUE;
        }
        if(Key=='F' && ControlState==PKF_CONTROL)
        {
            tstring s;
            GetItemFullPath(iXPathSubDir ? CurNode->childNodes : XPathSelection,
                *GetCurrentItem(), s);
            PutToCmdLine(s.c_str());
            return TRUE;
        }
        if(Key=='I' && ControlState==PKF_CONTROL) {
            return TRUE;
        }
        if(Key=='J' && ControlState==PKF_CONTROL)
        {
            // Enter XPath dialog
            static FarDialogItemID iditems[] = {
                { DI_DOUBLEBOX, 3,1, 72,4, 0,0,0,0, MXPath },
                { DI_TEXT, 5,2,  0,0, 0,0,0,1, MSelectUsingXPath },
                { DI_EDIT, 5,3, 70,3, 1,(INT_PTR)_T("XMLDOMXPath"),DIF_HISTORY,0, 0 },
            };
            FarDialogItem* items = MakeDialogItems(iditems, _countof(iditems));
            SetData(items[2], sCurXPath.c_str());
            while(1)
            {
                RUN_DIALOG(XPathDialogGuid, -1,-1,76,6,_T("XPath"),items,_countof(iditems));
                if(DIALOG_RESULT == -1)
                    break;
                LPCTSTR pxpath = GetItemText(items,2);
                if(*pxpath)
                    try {
                        SelectXPath(pxpath);
                } catch(_com_error e) {
                    WinError(e);
                    continue;
                }
                Reread();
                break;
            }
            delete[] items;
            return TRUE;
        }
        if(Key==VK_RETURN && ControlState==PKF_SHIFT)
        {
            PluginPanelItem& item = *GetCurrentItem();
            if((!bKeysAsDirs || !IsFolder(item)) && _tcscmp(item.FileName, _T("..")))
                return FALSE; //if not dir, let FAR process it;
            //in "keys as files", we process only ".."

            TCHAR TmpPath[MAX_PATH];
            //CreateTmpDir(TmpPath);
            GetTempPath(_countof(TmpPath), TmpPath);
            WCONST TCHAR* pTmpPath = TmpPath;

            int mode = OPM_SILENT | OPM_VIEW;
            if(GetFiles(&item, 1, 0, WADDR pTmpPath, mode, ControlState!=0) == 1 )
            {
                OpenPluginInfo info; TCHAR Title[256]; Title[_countof(Title)-1]=0;
                GetOpenPluginInfo(info);

                int ret;
                if(ControlState || !_tcscmp(item.FileName, _T("..")))
                {
                    ret=_sntprintf(Title, _countof(Title)-1, /*PFX*/_T("%s"), info.CurDir);
                    mode |= OPM_TWODOTS;
                }
                else
                    ret = _sntprintf(Title,_countof(Title)-1,*info.CurDir ? /*PFX*/_T("%s\\%s"):/*PFX*/_T("%s%s"),info.CurDir,item.FileName);
                if(ret==-1)
                    _tcscpy(Title+_countof(Title)-4, _T("..."));

                if((int)ShellExecute(0, 0, LastMadeName, 0, 0, SW_SHOW)<=32)
                    DeleteFile(LastMadeName);
            }
            //RemoveDirectory(TmpPath);
            return TRUE;
        }
        if((Key == VK_UP || Key == VK_DOWN || Key == VK_NUMPAD8 || Key == VK_NUMPAD2) && ControlState == (PKF_SHIFT | PKF_ALT))
        {
            PanelInfo panel;
            Control(FCTL_GETPANELINFO, &panel);
            if(panel.SortMode != SM_UNSORTED)
                return TRUE;

            if(Key == VK_NUMPAD8) Key = VK_UP;
            if(Key == VK_NUMPAD2) Key = VK_DOWN;
            if(panel.Flags & PFLAGS_REVERSESORTORDER) {
                if(Key == VK_UP) Key = VK_DOWN;
                else Key = VK_UP;
            }
            if(iXPathSubDir == 0)
                return TRUE;
            PluginPanelItem& currentItem = *GetCurrentItem();
            if(!((DWORD)currentItem.USERDATA & USERDATA_DIR))
                return TRUE;
            DWORD index = (DWORD)currentItem.USERDATA & ~USERDATA_DIR;
            if(index == 0 && Key == VK_UP || index == CurNode->childNodes->length - 1 && Key == VK_DOWN)
                return TRUE;
            NodePtr node = CurNode->childNodes->item[index];
            CurNode->removeChild(node);
            if(index == CurNode->childNodes->length - 1 && Key == VK_DOWN) {
                CurNode->appendChild(node);
                index = CurNode->childNodes->length - 1;
            }
            else
                CurNode->insertBefore(node, _variant_t(CurNode->childNodes->item[Key == VK_DOWN ? ++index : --index], true));

            bChanged = true;
            Redraw(index | ((DWORD)currentItem.USERDATA & USERDATA_DIR));
            return TRUE;
        }
    }
    catch(WinExcept) {
        WinError();
        return TRUE;
    }
    catch(_com_error e) {
        WinError(e);
        return TRUE;
    }
    return FALSE;
}
int XMLPlugin::ProcessEvent(int event, void *param)
{
    if(YMSPlugin::ProcessEvent(event, param))
        return TRUE;
    if(event==FE_REDRAW && iUpDir!=-1) {
        // Position cursor currently when going up-dir
        int iPrevUpDir = iUpDir;
        iUpDir = -1;
        PanelInfo panel;
        Control(FCTL_GETPANELINFO, &panel);
#ifdef UNICODE
        vector<BYTE> vpi;
#endif
        for(size_t i=0; i<(size_t)panel.ItemsNumber; i++) {
#ifdef UNICODE
            DWORD dwUD = (DWORD)GetPanelItem(i, vpi)->USERDATA;
#else
            DWORD dwUD = panel.PanelItems[i].USERDATA;
#endif
            if( (dwUD&USERDATA_DIR) && (dwUD & ~USERDATA_DIR)==iPrevUpDir) {
                ControlRedraw(i, panel.TopPanelItem);
                return TRUE;
            }		    
        }
    }
    return FALSE;
}
int XMLPlugin::DeleteFiles(PluginPanelItem *PanelItem, size_t itemCount, int OpMode)
{
    int rc=TRUE;
    if (itemCount==0) return FALSE;

    if (!(OpMode & OPM_SILENT))
    {
        int MsgIDs[] = { MWarning, MDoYouWantDel, MDelete, MCancel };
        LPCTSTR MsgItems[_countof(MsgIDs)];
        for(int i=0; i<_countof(MsgIDs); i++)
            MsgItems[i] = GetMsg(MsgIDs[i]);

        TCHAR MsgN[256];
        if (itemCount>1)
        {
            _sntprintf(MsgN, _countof(MsgN), GetMsg(MDoYouWantDelN), itemCount);
            MsgItems[1]=MsgN;
        }
        if (Message(FMSG_WARNING,NULL,MsgItems,_countof(MsgItems),2)!=0)
            return FALSE;
    }
    NodeListPtr childList;
    if(itemCount>1 || ((DWORD)PanelItem[0].USERDATA & USERDATA_DIR) || !iXPathSubDir)
        childList = iXPathSubDir ? CurNode->childNodes : XPathSelection;
    for(int i=0; i<(int)itemCount; i++)
    {
        PluginPanelItem& item = PanelItem[i];
        try {
            DWORD iItem = (int)item.USERDATA & ~USERDATA_DIR;
            if((DWORD)item.USERDATA & USERDATA_DIR) { // node
                NodePtr node = childList->item[iItem];
                (iXPathSubDir ? CurNode : node->parentNode ) ->removeChild(node).Release();
                if(iXPathSubDir)
                    // indices move!
                    for(int ii=i+1; ii<(int)itemCount; ii++) {
                        PluginPanelItem& item1 = PanelItem[ii];
                        if((DWORD)item1.USERDATA & USERDATA_DIR) { // node too
                            if(((DWORD)item1.USERDATA & ~USERDATA_DIR) > iItem)
                                --*(DWORD*)&item1.USERDATA;
                        }
                    }
            } else
                CurNode->attributes->removeNamedItem(item.FileName).Release();
            bChanged = true;
        } catch(WinExcept) {
            if(!OpMode) WinError();
            rc = FALSE;
        }
        catch(_com_error e) {
            if(!OpMode) WinError(e);
            rc = FALSE;
        }
    }
    if(!sCurXPath.empty()) // Reread XPath selection
        SelectXPath(sCurXPath.c_str());
    return rc;
}

#ifdef _X86_
TCHAR fmtSuffix[] = _T("_%08x_%04x_%d");
#else
TCHAR fmtSuffix[] = _T("_%016llx_%04x_%d");
#endif

void XMLPlugin::OnMakeFileName(PluginPanelItem& item)
{
    pExtOnGet = ((DWORD)item.USERDATA&USERDATA_DIR) ?
XMLEXT : _T("");
    _sntprintf(ExportNameSuffix, _countof(ExportNameSuffix), fmtSuffix,
        iXPathSubDir ? (void*)CurNode : (void*)XPathSelection, (DWORD)item.USERDATA&0xffff,
        ((DWORD)item.USERDATA&USERDATA_DIR)!=0 );
}
void XMLPlugin::OnAfterGetItem(PluginPanelItem&)
{
    pExtOnGet = XMLEXT;
}
PCTSTR XMLPlugin::GetFileExt()
{
    return pExtOnGet;
}
bool XMLPlugin::ChangeDesc(PluginPanelItem& item, LPCTSTR pNewDesc)
{
    try {
        if(!((DWORD)item.USERDATA&USERDATA_DIR)) { //attribute
            (iXPathSubDir ? CurNode->attributes : XPathSelection)
                ->item[(DWORD)item.USERDATA&~USERDATA_DIR]->nodeValue = _variant_t(pNewDesc);
            bChanged = true;
        } else { // node
            NodePtr node = (iXPathSubDir ? CurNode->childNodes : XPathSelection)
                ->item[(DWORD)item.USERDATA&~USERDATA_DIR];
            NodeType type = node->nodeType;
            if(type==MSXML2::NODE_COMMENT || type==MSXML2::NODE_TEXT || type==MSXML2::NODE_ATTRIBUTE)
            {
                node->nodeValue = _variant_t(pNewDesc);
                bChanged = true;
            }
        }
    } catch(_com_error e) {
        WinError(e);
    }
    return true;
}

int XMLPlugin::PutFiles(PluginPanelItem *PanelItem,size_t itemCount,int Move,int OpMode,PCTSTR SrcPath)
{
    for(size_t i=0; i<itemCount; i++)
    {
        PluginPanelItem& item = PanelItem[i];
        try {
            // replaceChild, file name must contain CurNode and item #
            void* pCurNode; int iItemUserData;
            BOOL bTrueNode=0;
            _stscanf(item.FileName, fmtSuffix, (INT_PTR)&pCurNode, &iItemUserData, &bTrueNode);
            // is this the node/selection we exported?
            if( iXPathSubDir && pCurNode==(void*)CurNode ||
                !iXPathSubDir && pCurNode==(void*)XPathSelection && !(OpMode&OPM_TWODOTS)) {

                    NodePtr node = 
                        iXPathSubDir ?
                        OpMode&OPM_TWODOTS ? CurNode :
                        bTrueNode ? CurNode->childNodes->item[iItemUserData] : CurNode->attributes->item[iItemUserData]
                    : XPathSelection->item[iItemUserData] ;

                    TCHAR absPath[MAX_PATH];
                    if(SrcPath)
                        _sntprintf(absPath, _countof(absPath), _T("%s\\%s"), SrcPath, item.FileName);
                    else
                        _tfullpath(absPath, item.FileName, _countof(absPath));

                    if(isRawExport(node->nodeType)) {
                        BYTE* text;
                        DWORD dwRead=0;
                        ReadBuffer(absPath, 0,0,&dwRead,&text);
                        if(dwRead > 2 && *(wchar_t*)text==0xfeff)
                            node->text = (wchar_t*)(text+2);
                        else
                            node->text = (char*)text;
                        delete text;
                    } else { //node, import xml
                        if(!*CurrentDir && iXPathSubDir && (OpMode&OPM_TWODOTS)) {
                            //reload the whole file
                            CXMLFile newfile(absPath);
                            XmlFile = newfile;
                            SetCurNode(static_cast<NodePtr>(newfile));
                        } else {
                            NodePtr newNode = CreateNode(absPath);
                            if(OpMode&OPM_TWODOTS) {			
                                NodePtr parent;
                                //NodeType type = CurNode->nodeType;
                                CurNode->parentNode->replaceChild(newNode, node).Release();
                                SetCurNode(newNode);
                            }
                            else {
                                (iXPathSubDir ? CurNode : node->parentNode)
                                    ->replaceChild(newNode, node).Release();
                            }
                        }
                    }
            }
            else
                _com_raise_error(E_NOTIMPL);
        } catch(WinExcept) {
            WinError();
            return 0;
        } catch(_com_error e) {
            WinError(e);
            return 0;
        }
        bChanged = true;
    }
    if(!sCurXPath.empty()) // Reread XPath selection
        SelectXPath(sCurXPath.c_str());
    return 1;
}
int XMLPlugin::MakeDirectory(WCONST WTYPE Name, int OpMode)
{
    if(*CurrentDir==0 || iXPathSubDir==0) return TRUE;
    static FarDialogItemID MkkeyItems[] = {
        { DI_DOUBLEBOX, 3,1, 72,4, 0,0,0,0, MCreate},
        { DI_TEXT, 5,2, 70,2, 0,0,0,1, MCreateElement},
        { DI_EDIT, 5,3, 70,3, 1,0,0,0, 0},
    };
    FarDialogItem* items = MakeDialogItems(MkkeyItems, _countof(MkkeyItems));
    SetData(items[2], WDEREF Name);
    _bstr_t newName(WDEREF Name);
    if(!(OpMode&OPM_SILENT)) {
        RUN_DIALOG(CreateNewDialogGuid, -1,-1,76,6,_T("CreateNew"),items,_countof(MkkeyItems));
        if(DIALOG_RESULT == -1) {
            delete[] items;
            return TRUE;
        }
        newName = GetItemText(items, 2);
    }
    int rc = TRUE;
    try
    {
        wchar_t* pColon = wcschr(newName.GetBSTR(), ':');
        _bstr_t nmspace(pColon ? wstring(newName).substr(0, pColon-newName).c_str() : L"");
        NodePtr newNode = XmlFile->createNode(_variant_t(MSXML2::NODE_ELEMENT), newName, nmspace);
        CurNode->appendChild(newNode);
#ifdef UNICODE
        strCreatedDir = newName;
#endif
        bChanged = true;
    }
    catch(_com_error e)
    {
        if(!(OpMode&OPM_SILENT))
            WinError(e);
        else
            rc = FALSE;
    }
    if(rc)
#ifdef UNICODE
        *Name = strCreatedDir.c_str();
#else
        strcpy(Name, items[2].Data);
#endif
    delete[] items;
    return rc;
}
BOOL XMLPlugin::CloseEvent()
{
    PCTSTR items[] = {_T(""),GetMsg(MFileModified), GetMsg(MSaveButton), GetMsg(MDontSave), GetMsg(MContEditing)};
    if(bChanged)
        switch(Message(FMSG_WARNING,NULL,items,_countof(items),3)) {
        case 0: ProcessKey(VK_F2, PKF_SHIFT);
        case 1: break;
        default: return TRUE; // Cancel
    }	
    PanelInfo pi;
    Control(FCTL_GETPANELINFO, &pi);
    USE_SETTINGSW;
    settings.Set(sStartPanelMode, (DWORD)pi.ViewMode);
    settings.Set(sDisplayAttrsAsNames, bDisplayAttrsAsNames);
    return FALSE;
}
void XMLPlugin::WinError(_com_error& e)
{
    _bstr_t s = e.Error()==E_FAIL ? e.Description() : e.ErrorMessage();
    YMSPlugin::WinError(OEMString(s));
}

void XMLPlugin::GetNodeXPath(NodePtr& node, tstring& sPath)
{
    NodePtr parent = node->parentNode;
    if(parent==NULL)
        return;
    GetNodeXPath(parent, sPath);
    NodeListPtr childList = parent->childNodes;
    NodeType type = node->nodeType;

    _bstr_t name = node->nodeName;
    int idx=-1, total = 0;
    while(1) {
        NodePtr node_i = childList->nextNode();
        if(node_i==NULL)
            break;
        if(type==MSXML2::NODE_DOCUMENT_TYPE || 
            !wcscmp(node_i->nodeName, name) && type==node_i->nodeType ) {	    
                if(node_i==node) {
                    idx = total;
                    if(!XmlFile.IsMSXML3()) idx++;
                }
                if(++total>1 && idx>=0) // we need to know if total number of same items is >1
                    break;
        }
    }
    TCHAR buf[256];

    PCTSTR p = 0;
    switch(type) {
    case MSXML2::NODE_COMMENT:
        p = _T("comment()"); break;
    case MSXML2::NODE_TEXT:
        p = _T("text()"); break;
    case MSXML2::NODE_CDATA_SECTION :
        p = _T("cdata()"); break;
    case MSXML2::NODE_DOCUMENT_TYPE:
        p = _T("node()"); break;
    case MSXML2::NODE_PROCESSING_INSTRUCTION:
        p = _T("processing-instruction()"); break;
        //	case MSXML2::NODE_NOTATION:
    }
    _sntprintf(buf, _countof(buf), total>1?_T("%s[%d]"):_T("%s"), p?p:OEMString(name), idx);
    sPath += _T("/");
    sPath += buf;
}

#define SPECIALROOT _T("Special")

#ifdef FAR3
void XMLPlugin::LoadSpecialDocsFromReg(PCWSTR rootKey)
{
    // Read SpecialDocs from registry
    wstring keyName(rootKey);
    keyName.append(L"\\" SPECIALROOT L"\\");
    keyName.append(XmlFile->documentElement->nodeName);

    HKEY hKey;
    DWORD nSpecialTags;
    if(RegOpenKeyExW(HKEY_CURRENT_USER, keyName.c_str(), 0, KEY_READ, &hKey)==ERROR_SUCCESS &&
        RegQueryInfoKeyW(hKey, 0, 0, 0, &nSpecialTags,0,0,0,0,0,0,0)==ERROR_SUCCESS) {
            for(DWORD id=0; id<nSpecialTags; id++) {
                WCHAR tag[128]; HKEY h;
                SpecialFields elem;
                DWORD sz = _countof(tag);
                if(RegEnumKeyExW(hKey, id, tag, &sz, 0,0,0,0)!=ERROR_SUCCESS ||
                    RegOpenKeyExW(hKey, tag, 0, KEY_QUERY_VALUE, &h)!=ERROR_SUCCESS )
                    break;
                DWORD size;
                TCHAR val[128];
                size = _countof(val);
                if(RegQueryValueExW(h, L"name", 0, NULL, (PBYTE)&val, &size)==ERROR_SUCCESS)
                    elem.sNameFld = val;
                size = _countof(val);
                if(RegQueryValueExW(h, L"datetime", 0, NULL, (PBYTE)&val, &size)==ERROR_SUCCESS)
                    elem.sDateFld = val;
                size = _countof(val);
                if(RegQueryValueExW(h, L"desc", 0, NULL, (PBYTE)&val, &size)==ERROR_SUCCESS)
                    elem.sDescriptionFld = val;
                if(RegQueryValueExW(h, L"owner", 0, NULL, (PBYTE)&val, &size)==ERROR_SUCCESS)
                    elem.sOwnerFld = val;
                mapSpecialDocs[tag] = elem;
                RegCloseKey(h);
            }
            RegCloseKey(hKey);
    }
}
#endif

void XMLPlugin::LoadSpecialDocs()
{
    USE_SETTINGS;
    KEY_TYPE idspec = settings.OpenSubKey(SPECIALROOT);
    if(idspec == 0)
        return;

    vector<tstring> docs = settings.EnumKeys(idspec);
    for each(auto docName in docs)
    {
        KEY_TYPE iddoc = settings.OpenSubKey(docName.c_str(), idspec);
        if(iddoc == 0)
            continue;
        vector<tstring> entries = settings.EnumKeys(iddoc);
        for each(auto entry in entries)
        {
            KEY_TYPE identry = settings.OpenSubKey(entry.c_str(), iddoc);
            if(identry == 0)
                continue;
            SpecialFields elem;
            TCHAR buffer[256];
            PCTSTR str = settings.Get(_T("name"), buffer, _countof(buffer), NULL, identry);
            if(str && *str)
                elem.sNameFld = str;
            str = settings.Get(_T("datetime"), buffer, _countof(buffer), NULL, identry);
            if(str && *str)
                elem.sDateFld = str;
            str = settings.Get(_T("desc"), buffer, _countof(buffer), NULL, identry);
            if(str && *str)
                elem.sDescriptionFld = str;
            str = settings.Get(_T("owner"), buffer, _countof(buffer), NULL, identry);
            if(str && *str)
                elem.sOwnerFld = str;
            mapSpecialDocs[entry.c_str()] = elem;
        }
    }
}

void XMLPlugin::SaveSpecialDocs(bool bDelete, PCTSTR docName, PCTSTR entry, PCTSTR name, PCTSTR datetime, PCTSTR desc, PCTSTR owner)
{
    USE_SETTINGSW;
    KEY_TYPE idspec = settings.CreateSubKey(WideFromOem(SPECIALROOT));
    if(idspec == 0)
        return;
    KEY_TYPE iddoc = settings.CreateSubKey(WideFromOem(docName), idspec);
    if(iddoc == 0)
        return;

    // Save in registry or delete from it
    if(bDelete) {
#ifdef FAR3
        KEY_TYPE hkey = settings.CreateSubKey(WideFromOem(entry), iddoc);
        if(hkey != 0)
            settings.DeleteSubKey(hkey);
#else
        settings.DeleteSubKey(WideFromOem(entry));
#endif
        mapSpecialDocs.erase((PWSTR)WideFromOem(entry));
    } else {
        KEY_TYPE hkey = settings.CreateSubKey(WideFromOem(entry), iddoc);
        if(hkey != 0)
        {
            SetOrDelValue(settings, hkey, L"name"    , WideFromOem(name));
            SetOrDelValue(settings, hkey, L"datetime", WideFromOem(datetime));
            SetOrDelValue(settings, hkey, L"desc"    , WideFromOem(desc));
            SetOrDelValue(settings, hkey, L"owner"   , WideFromOem(owner));
        }
    }
}

_bstr_t XMLPlugin::EvalXPath(NodePtr node, _bstr_t xpath, map<_bstr_t, DocumentPtr>& xsltDocMap)
{
    DocumentPtr xsltDoc = xsltDocMap[xpath];
    if(xsltDoc == NULL) {
        try {
            if(node == NULL)
                _com_issue_error(E_FAIL);
            NodePtr n = node->selectSingleNode(xpath);
            return n == NULL ? _bstr_t() : n->text;
        } catch(_com_error) {
            // eval scalar XPath
            static TCHAR part1[] = 
                _T("<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" ");
            static TCHAR part1a[] = 
                _T(" version=\"1.0\">")
                _T("<xsl:output method=\"text\" encoding=\"utf-16\" />")
                _T("<xsl:template match=\"*\"><xsl:value-of select=\"");
            static TCHAR part2[] = _T("\" /></xsl:template></xsl:stylesheet>");

            _bstr_t xslt(part1);
            xslt += XmlFile->getProperty(L"SelectionNamespaces").bstrVal;
            xslt += part1a;

            tstring xp(xpath);
            for(size_t i = 0; i < xp.length(); i++)
                if(xp[i]=='\"')
                    const_cast<PTSTR>(xp.c_str())[i] = '\'';
            xslt += xp.c_str();
            xslt += part2;

            xsltDoc.CreateInstance(XmlFile.GetCLSID());
            if(!xsltDoc->loadXML(xslt))
                return _bstr_t();
            xsltDocMap[xpath] = xsltDoc;
        }
    }
    _bstr_t res = node->transformNode(xsltDoc);
    return res;
}

void XMLPlugin::EditSpecialDocs()
{
    static FarDialogItemID DialogItems[]={
        /*00*/{DI_DOUBLEBOX,3,1,72,11,0,0,0,0,MSpecialDocDefinition},
        /*01*/{DI_TEXT,5,2,0,0,0,0,0,0,MRootTag},
        /*02*/{DI_EDIT,30,2,70,0, 0, (INT_PTR)_T("XMLDocTag"), DIF_HISTORY},
        /*03*/{DI_TEXT,5,3,0,0,0,0,0,0,MItemTag},
        /*04*/{DI_EDIT,30,3,70,0, 0, (INT_PTR)_T("XMLItemTag"), DIF_HISTORY},
        /*05*/{DI_TEXT,5,5,0,0,0,0,0,0,MNameXPath},
        /*06*/{DI_EDIT,30,5,70,0, 1, (INT_PTR)_T("XMLNameXPath"), DIF_HISTORY},
        /*07*/{DI_TEXT,5,6,0,0,0,0,0,0,MDateTimeXPath},
        /*08*/{DI_EDIT,30,6,70,0, 0, (INT_PTR)_T("XMLTimeXPath"), DIF_HISTORY},
        /*09*/{DI_TEXT,5,7,0,0,0,0,0,0,MDescXPath},
        /*10*/{DI_EDIT,30,7,70,0, 0, (INT_PTR)_T("XMLNameXPath"), DIF_HISTORY},
        /*11*/{DI_TEXT,5,8,0,0,0,0,0,0,MOwnerXPath},
        /*12*/{DI_EDIT,30,8,70,0, 0, (INT_PTR)_T("XMLNameXPath"), DIF_HISTORY},
        /*13*/{DI_TEXT,4,9,0,0,0,0,DIF_SEPARATOR},
        /*14*/{DI_BUTTON,0,10,0,0,0,0,DIF_CENTERGROUP,1,MOK},
        /*15*/{DI_BUTTON,0,10,0,0,0,0,DIF_CENTERGROUP,0,MCancel}
    };
    FarDialogItem* ItemsStr = MakeDialogItems(DialogItems, _countof(DialogItems));
    for(int i=2; i<=12; i+=2)
        ItemsStr[i].X1 = ItemsStr[i-1].X1 + _tcslen(ItemsStr[i-1].ITEMDATA);

    OemBStr docName(XmlFile->documentElement->nodeName);
    SetData(ItemsStr[2], (LPCTSTR)docName);

    PluginPanelItem& currentItem = *GetCurrentItem();
    DWORD dwUD = (DWORD)currentItem.USERDATA;
    NodePtr node = (iXPathSubDir ? CurNode->childNodes : XPathSelection)->item[dwUD&~USERDATA_DIR];

    OemBStr nodeName, nameField, dateField, descField, ownerField;
    if(IsFolder(currentItem)) {
        nodeName = node->nodeName;
        SetData(ItemsStr[4], nodeName);
        auto iter = mapSpecialDocs.find(ItemsStr[4].ITEMDATA);
        if(iter != mapSpecialDocs.end()) {
            nameField = iter->second.sNameFld;
            SetData(ItemsStr[6], nameField);
            dateField = iter->second.sDateFld;
            SetData(ItemsStr[8], dateField);
            descField = iter->second.sDescriptionFld;
            SetData(ItemsStr[10], descField);
            ownerField = iter->second.sOwnerFld;
            SetData(ItemsStr[12], ownerField);
        }
    }
_run:
    RUN_DIALOG(SpecialDialogGuid, -1, -1, 76, 13, _T("Special"), ItemsStr, _countof(DialogItems));
    if(DIALOG_RESULT == _countof(DialogItems)-2) {
        if(*GetItemText(ItemsStr, 2) == 0 || *GetItemText(ItemsStr, 4)==0) {
            LPCTSTR MsgItems[] = { GetMsg(MError), GetMsg(MEmptyTags), GetMsg(MOK) };
            Message(FMSG_WARNING,_T("Special"),MsgItems,_countof(MsgItems),1);
            goto _run;
        }
        bool bDelete = true;
        for(int i=6; i<=12; i+=2) {
            LPCTSTR txt = GetItemText(ItemsStr, i);
            if(txt && *txt) { bDelete= false; break; }
        }

        SaveSpecialDocs(bDelete, GetItemText(ItemsStr, 2), GetItemText(ItemsStr, 4), GetItemText(ItemsStr, 6),
            GetItemText(ItemsStr, 8), GetItemText(ItemsStr, 10), GetItemText(ItemsStr, 12));

        if(!_tcscmp(GetItemText(ItemsStr, 2), docName)) { // defined for THIS document
            if(!bDelete) {
                SpecialFields& fields = mapSpecialDocs[GetItemText(ItemsStr, 4)];
                fields.sNameFld         = WideFromOem(GetItemText(ItemsStr,  6));
                fields.sDateFld         = WideFromOem(GetItemText(ItemsStr,  8));
                fields.sDescriptionFld  = WideFromOem(GetItemText(ItemsStr, 10));
                fields.sOwnerFld        = WideFromOem(GetItemText(ItemsStr, 12));
            }
            Redraw();
        }
    }
    delete[] ItemsStr;
}