#include "stdafx.h"
#include "winver.h"
#include "XMLDOM.h"
#include "XMLDOMLang.h"
#include "..\PluginSettings.hpp"

#define th (*static_cast<IXMLDOMDocument2Ptr*>(this))

const char CXMLFile::MSXML6[] = "MSXML6";
const char CXMLFile::MSXML4[] = "MSXML4";
const char CXMLFile::MSXML3[] = "MSXML3";

using namespace MSXML2;

int ErrorEdit(LPTSTR pMsg)
{
    static HANDLE hConsoleOutput;
    if(!hConsoleOutput) hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    //  if(!hConsoleOutput) hConsoleOutput = ;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);

    size_t w = sbi.dwSize.X - 14, iw=0;
    for(TCHAR* p = pMsg, *lastspace=0; *p; p++) {
        ++iw;
        switch(*p) {
	    case ' ': case '\t': lastspace = p; break;
	    case '\n': case '\r': iw = 0; lastspace = 0; break;
        }
        if(iw >= w && lastspace) {
	    *lastspace = '\n';
	    iw = p - lastspace;
	    lastspace = 0;
        }
    }
    vector<PCTSTR> items;
    items.reserve(6);
    items.push_back(GetMsg(MError));
    for(PTSTR p = _tcstok(pMsg,_T("\r\n")); p; p = _tcstok(NULL,_T("\r\n")))
        items.push_back(p);

    items.push_back(GetMsg(MOK));
    items.push_back(GetMsg(MEditFile));
    return Message(FMSG_WARNING, _T("WinError"), items, 2);
}

CXMLFile::CXMLFile(LPCTSTR lpFileName, LPCBYTE data)
{
    int version = 6;
    HRESULT hr = E_FAIL;

    // With engines newer than msxml3, we have the XPath problem descibed in http://support.microsoft.com/kb/313372:
    // if default NS is specified, XPath requires that NS will be specified too.
    // If we remove default namespace declaration in the loaded file and save it without this namespace, it will be corrupted.
    // If we restore it before saving, the engine will add "xmlns="" " attributes in the first-level subnodes,
    //   so this will corrupt them too.

    //if(!GetReg(_T("ForceMSXML3"), 0))
    //{
    //   hr = CreateInstance(*(clsid = &__uuidof(MSXML2::DOMDocument60)));
    //   if(FAILED(hr)) {
    //      hr = CreateInstance(*(clsid = &__uuidof(MSXML2::DOMDocument40)));
    //      version = 4;
    //      pXMLDOMDLL = MSXML4;
    //   }
    //   else
    //      pXMLDOMDLL = MSXML6;
    //}
    //if(FAILED(hr))
    {
        hr = CreateInstance(*(clsid = &__uuidof(MSXML2::DOMDocument30)));
        version = 3;
        pXMLDOMDLL = MSXML3;
    }
    if(FAILED(hr))
        _com_issue_error(hr);

    sMSXMLVersion.clear();
    extern TCHAR sValidateOnParse[], sResolveExternals[];
    USE_SETTINGS;
    th->validateOnParse = VARIANT_BOOL(settings.Get(sValidateOnParse, 0) ? VARIANT_TRUE : VARIANT_FALSE);
    th->resolveExternals = VARIANT_BOOL(settings.Get(sResolveExternals, 0) ? VARIANT_TRUE : VARIANT_FALSE);
    th->async = VARIANT_FALSE;
    //th->preserveWhiteSpace = VARIANT_TRUE;
    if(version == 6)
        th->setProperty(L"ProhibitDTD", VARIANT_FALSE);
    if(settings.Get(sValidateOnParse, 0)) {
        if(version > 4) {
            th->setProperty(L"UseInlineSchema", VARIANT_TRUE);
            th->setProperty(L"ResolveExternals", VARIANT_TRUE);
        }
    }
    /*else if(version > 3)
	th->setProperty(L"NewParser", VARIANT_TRUE);*/

#ifdef UNICODE
    LPCWSTR pwFileName = lpFileName;
#else
    DWORD l = _tcslen(lpFileName)+1;
    vector<wchar_t> pwFileName(l);
    OemToWchar(lpFileName, &pwFileName[0], l);
#endif
    while(1)
    {
        if(data && *(DWORD*)data == 0x3f003c) // L"<?" - unicode without BOM
        {            
            BYTE* buf;
            ReadBuffer(lpFileName, 0, 0, NULL, &buf);
            _bstr_t buffer((LPWSTR)buf);
            delete buf;
            hr = th->loadXML(buffer);
        }
        else
            hr = th->load(&pwFileName[0]);
        if(hr != VARIANT_TRUE)
        {
	    if(hr==0)
            {
	        auto errPtr = th->parseError;
                if(errPtr->errorCode == INET_E_OBJECT_NOT_FOUND)
                    _com_raise_error(ERROR_FILE_NOT_FOUND);
	        TCHAR buf[512];
	        _sntprintf(buf, _countof(buf), GetMsg(MParseError), (PCTSTR)errPtr->url,
		        errPtr->errorCode, errPtr->line, errPtr->linepos,
		        (PCTSTR)OEMString(errPtr->reason) );
		buf[_countof(buf) - 1] = 0;
                if(ErrorEdit(buf)==1)
                {
                   TCHAR abspath[MAX_PATH];
                   _tfullpath(abspath, lpFileName, _countof(abspath));
                   if(StartupInfo.Editor(abspath, lpFileName, 0,0,-1,-1, EF_ENABLE_F6, errPtr->line, errPtr->linepos
#ifdef FAR3
		    , CP_DEFAULT			
#elif UNICODE
                    , CP_AUTODETECT
#endif
                   )
                        ==EEC_MODIFIED)
                        continue;
                }
	        _com_raise_error(ERROR_CANCELLED);
	    }
        }
        else break;
    }
    GetMSXMLVersionInfo();
#if 0
    if(version >= 4)
    {
        // http://support.microsoft.com/kb/313372 - if default NS is specified, XPath requires NS too
        //      so we have to remove default namespace declaration
        /*NodePtr*/ defaultNamespace = th->documentElement->attributes->getNamedItem(L"xmlns");
        if(defaultNamespace != NULL)
        {
            th->documentElement->removeAttribute(defaultNamespace->nodeName);
            bstr_t xml = th->xml;
            PWSTR p = wcsstr(xml.GetBSTR(), defaultNamespace->xml.GetBSTR());
            if(p != NULL)
            {
                memmove(p, p+defaultNamespace->xml.length(), (xml.length()-(p-xml)-defaultNamespace->xml.length()+1)*sizeof(wchar_t));
                th->loadXML(xml);
                //th->documentElement->setAttribute(L"xmlns", defaultNamespace->text);
            }
        }
        /*else if(nsuri.length() > 0)
        {
            wchar_t buf[256];
	    PCWSTR pPfx = th->documentElement->prefix;
            _snwprintf(buf,_countof(buf),L"xmlns:%s='%s'",pPfx,(PCWSTR)nsuri);
            th->setProperty(L"SelectionNamespaces", buf);
	}*/
    }
#endif
    vector<_bstr_t> ns;
    for(int i = 0; i < th->documentElement->attributes->length; i++)
    {
        if( !memcmp(th->documentElement->attributes->item[i]->nodeName.GetBSTR(), L"xmlns", 10))
            ns.push_back(th->documentElement->attributes->item[i]->xml);
    }
    _bstr_t newSelNsp(th->getProperty(L"SelectionNamespaces").bstrVal);
    for each(_bstr_t item in ns)
    {
        newSelNsp += L" ";
        newSelNsp += item;
    }
    th->setProperty(L"SelectionNamespaces", newSelNsp);
}

NodePtr CreateNode(PCTSTR pFileName)
{
    return CXMLFile(pFileName)->documentElement;
}

void CXMLFile::GetMSXMLVersionInfo()
{
  static char SFI[] = "StringFileInfo";
  static wchar_t SFIW[] = L"StringFileInfo";

    OSVERSIONINFO vi; vi.dwOSVersionInfoSize = sizeof vi;
    GetVersionEx(&vi);
    bool NT = (vi.dwPlatformId==VER_PLATFORM_WIN32_NT);

    TCHAR FilePath[MAX_PATH];
    GetModuleFileName(GetModuleHandleA(pXMLDOMDLL), FilePath, sizeof FilePath);

    DWORD size = GetFileVersionInfoSize(FilePath, &size);
    if(!size) return;
    TCHAR* pBuffer = new TCHAR[size];
    if(!GetFileVersionInfo(FilePath, 0, size, pBuffer)) {
	delete pBuffer;
	return;
    }

    //Find StringFileInfo
    DWORD ofs;
    for(ofs = NT ? 92 : 70; ofs < size; ofs += *(WORD*)(pBuffer+ofs) )
    {
	if(
#ifndef UNICODE
            !NT && !_stricmp(pBuffer+ofs+6, SFI) ||  /*!!!*/
#endif
            NT && !_wcsicmp((LPCWSTR)(pBuffer+(ofs+6)/sizeof(TCHAR)), SFIW))
	    break;
    }
    if(ofs >= size)
    {
	delete pBuffer;
	return;
    }	
    TCHAR* langcode;
#ifndef UNICODE
    char lcode[10];
    if(NT)
    {
	WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)(pBuffer+ofs+42), -1, lcode, sizeof lcode, 0,0);
	langcode = lcode;
    }
    else 
#endif
	langcode = pBuffer + ofs /sizeof(TCHAR) + 21;

    TCHAR blockname[48];
    UINT uLen;

    TCHAR *pVersion, *pDesc;

    _sntprintf(blockname, sizeof blockname, _T("\\%s\\%s\\FileVersion"), EXP_NAME(SFI), langcode);
    if(!VerQueryValue(pBuffer, blockname, (void**)&pVersion, &uLen))
	pVersion = 0;

    _sntprintf(blockname, sizeof blockname, _T("\\%s\\%s\\FileDescription"), EXP_NAME(SFI), langcode);
    if(!VerQueryValue(pBuffer, blockname, (void**)&pDesc, &uLen))
	pDesc = 0;

    sMSXMLVersion = pDesc;
    sMSXMLVersion += ' ';
    sMSXMLVersion += pVersion;
    delete pBuffer;
}

void CXMLFile::Save(LPCTSTR filename)
{
    //if(defaultNamespace!=NULL)
    //    th->documentElement->attributes->setNamedItem(defaultNamespace);

#ifdef UNICODE
    th->save(filename);
#else
    vector<wchar_t> wfilename(strlen(filename)+1);
    MultiToWchar(filename, &wfilename[0], wfilename.size(), CP_OEMCP);
    th->save(variant_t(&wfilename[0]));
#endif

    //if(defaultNamespace!=NULL)
    //    th->documentElement->attributes->removeNamedItem(defaultNamespace->nodeName);
}
