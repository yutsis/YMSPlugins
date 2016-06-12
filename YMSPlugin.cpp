static bool CheckForEsc()
{
    static HANDLE hConInp = GetStdHandle(STD_INPUT_HANDLE);

    bool rc = false;
    while(1)
    {
        INPUT_RECORD rec;
        DWORD dwReadCount;
        PeekConsoleInput(hConInp,&rec,1,&dwReadCount);
        if(!dwReadCount)
            break;
        ReadConsoleInput(hConInp,&rec,1,&dwReadCount);
        if(rec.EventType==KEY_EVENT &&
            rec.Event.KeyEvent.bKeyDown &&
            rec.Event.KeyEvent.wVirtualKeyCode==VK_ESCAPE)
            rc = true;
    }
    return rc;
}

void ProgressAction::ShowProgress()
{
    DWORD dwTicks1 = GetTickCount();
    if(dwTicks1-dwTicks > TICK_INTERVAL_TO_SHOW)
    {
        TCHAR buf[80];
        ProgressMessage(buf);
        const TCHAR *Items[]={ProgressTitle(),buf};
        Message(0,NULL,Items,_countof(Items),0);
        if(CheckForEsc())
	{
            AbortAction();
            SetLastError(ERROR_CANCELLED);
            throw WinExcept(ERROR_CANCELLED);
        }
        dwTicks = dwTicks1;
    }
}

void YMSPlugin::DeleteKeyBarTitles()
{
    if(pKeyBarTitles != NULL)
    {
#ifdef FAR3
	delete[] pKeyBarTitles->Labels;
#endif
	delete pKeyBarTitles;
    }
}

YMSPlugin::~YMSPlugin()
{
    DeleteKeyBarTitles();
}

int YMSPlugin::WinError(LPCTSTR pMsg, LPCSTR pSourceModule)
{
    LPTSTR lpMsgBuf; bool bAllocated = false;
    UINT uErr = GetLastError() & (pSourceModule ? 0xffffffff:0x3fffffff);
    if(pMsg /*&& GetLastError()==0*/)
        lpMsgBuf = (LPTSTR)pMsg;
    else
    {
        HMODULE hModule = 0;
        if(pSourceModule)
	{
            hModule = GetModuleHandleA(pSourceModule);
            if(!hModule)
                hModule = LoadLibraryA(pSourceModule);
        }
        FormatMessage( hModule ?
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE
            : FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            hModule, uErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf, 0, NULL );
        bAllocated = true;
#ifndef UNICODE
        ToOem(lpMsgBuf);
#endif
    }

    static HANDLE hConsoleOutput;
    if(!hConsoleOutput) hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    //  if(!hConsoleOutput) hConsoleOutput = ;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    GetConsoleScreenBufferInfo(hConsoleOutput, &sbi);

    DWORD w = sbi.dwSize.X - 14;
    size_t iw=0;
    for(TCHAR* p = lpMsgBuf, *lastspace=0; *p; p++)
    {
        ++iw;
        switch(*p)
	{
          case ' ': case '\t': lastspace = p; break;
          case '\n': case '\r': iw = 0; lastspace = 0; break;
        }
        if(iw >= w && lastspace)
	{
            *lastspace = '\n';
            iw = p - lastspace;
            lastspace = 0;
        }
    }
    int rc;
#ifndef FAR3
    if(HIWORD(GetFarVersion())<591)
    {
        static LPCTSTR items[]={0,0,0,0};
        items[0] = GetMsg(MError); items[3] = GetMsg(MOK);
        items[1] = _tcstok(lpMsgBuf,_T("\r\n"));
        items[2] = _tcstok(NULL,_T("\r\n")); if(!items[2]) items[2] = items[3];
        rc = Message(FMSG_WARNING,_T("WinError"),items,_countof(items)-(items[2]==items[3]),1);
    } else
#endif
	{
        size_t l;
        TCHAR* buf = new TCHAR[l=_tcslen(lpMsgBuf)+_tcslen(GetMsg(MError))+2];
        _sntprintf(buf, l, _T("%s\n%s"), GetMsg(MError), lpMsgBuf);
        TCHAR*d=buf;
        for(TCHAR*s=buf; *s; s++)
	{
            *d = *s;
            if(!(*s=='\r' && s[1]=='\n' || *s=='\n' && s[1]=='\r'))
                d++;	  
        }
        *d = 0;
        /*char* p = buf + strlen(buf) - 1;
        if(p[-1]=='\r' && *p=='\n') p[-1] = 0;*/
        rc = Message(FMSG_WARNING|FMSG_ALLINONE|FMSG_MB_OK, _T("WinError"), (PCTSTR*)buf, 0, 0);
        delete[] buf;
    }
    if(bAllocated) LocalFree( lpMsgBuf );
    return rc;
}

int YMSPlugin::WinError(DWORD error)
{
    if(error) ::SetLastError(error);
    return WinError();
}

#ifndef NOPANELS
static TCHAR invalid_chars[] = _T(":*?\\/\"<>;|");

inline TCHAR hexdigit(_TUCHAR c)
{
    return c<10 ? c+'0' : c+55;
}

TCHAR* YMSPlugin::MakeFileName(LPCTSTR DestPath, LPCTSTR fname,int)
{
    TCHAR* fmt;
    switch(DestPath[_tcslen(DestPath)-1])
    {
        case '\\': case'/': case':':
            fmt = _T("%s%s%s"); break;
        default: fmt = _T("%s\\%s%s");
    }

    TCHAR name[MAX_PATH];
    bool bTwoDots = false;
    if(!_tcscmp(fname, _T("..")))
    {
        // make name out of parent name
        TCHAR* p = _tcsrchr(CurDir, '\\');
        if(!p) p = CurDir;
        if(!*p) p = _T("(root)");
        fname = p;
        bTwoDots = true;
    }
    bool bAllDots = true;
    TCHAR* maxName = name + MAX_PATH-1 - _tcslen(GetFileExt()) - _tcslen(DestPath)-2;
    _TUCHAR* q = (_TUCHAR*)name;
    for(_TUCHAR*p = (_TUCHAR*)fname; *p; p++)
    {
        if(_tcschr(invalid_chars, *p) || *p<' ')
	{
            *q++ = '#'; *q++ = hexdigit(*p>>4); *q++ = hexdigit(*p&15);
        }
        else
            *q++ = *p;
        if(*p!='.' && *p!=' ') bAllDots = false;
        if(q >= (_TUCHAR*)maxName)
            break;
    }
    if(q > (_TUCHAR*)maxName) q = (_TUCHAR*)maxName;
    *q = 0;
    //Fix some other "bad" names
    if(!_tcsicmp(name,_T("con")) || !_tcsicmp(name,_T("aux")) || !_tcsicmp(name,_T("prn")) || 
        !_memicmp(name,_T("com"),3) && isdigit(name[3]) && name[4]==0 || 
        !_memicmp(name,_T("lpt"),3) && isdigit(name[3]) && name[4]==0 || 
        bAllDots)
    {
        TCHAR* pEnd = name + _tcslen(name);
        _TUCHAR c = pEnd[-1];
        pEnd[-1] = '%'; pEnd[0] = hexdigit(c>>4);
        pEnd[1] = hexdigit(c&15); pEnd[2] = 0;
    }
    _sntprintf(LastMadeName, _countof(LastMadeName), fmt, DestPath, name, GetFileExt());
    return LastMadeName;
}

// Returns: -1=cancel, 0=skip, 1=overwrite, 2=append
int ConfirmOverwrite(LPCTSTR filename, bool& bOverAll, bool& bSkipAll, bool bAppend)
// bAppend - show Append button;   filename is in OEM
{
    if(bOverAll || GetFileAttributes(filename)==0xffffffff)
        return 1;
    if(bSkipAll) return 0;
    LPCTSTR items[9] = { GetMsg(MWarning), GetMsg(MFileExists), filename,
        GetMsg(MOverwrite), GetMsg(MAll), GetMsg(MSkip), GetMsg(MSkipAll),
        bAppend ? GetMsg(MAppend) : GetMsg(MACancel), GetMsg(MACancel)
    };
    switch(Message(FMSG_WARNING, 0, items, bAppend? 9:8, bAppend? 6:5))
    {
        case -1: return -1;
        case 0: return 1;
        case 1: bOverAll = true; return 1;
        case 2: return 0;
        case 3: bSkipAll = true; return 0;
        case 4: return bAppend ? 2 : -1;
    }
    return 0;
}

void StripQuotes(TCHAR* str)
{    
    TCHAR* p;
    for(p=str; *p==' '; p++)
        ;
    if(*p!='"') return;
    TCHAR* pDest=str;    
    for(p++; *p && *p!='"'; p++)
        if(*p!='"')
            *pDest++ = *p;
    *pDest = 0;
}

// Checks destination path passed to GetFiles. May modify content of DestPath.
YMSPlugin::DESTPATH_RESULT YMSPlugin::CheckDestPath(TCHAR* pDestPath, PluginPanelItem *PanelItem,size_t nItems, PCTSTR ext)
{
    if(_tcschr(pDestPath,'*') || _tcschr(pDestPath, '?'))
    {
        SetLastError(0);
        WinError(GetMsg(MWildcardsNotAllowed));
        return DESTPATH_ERROR;
    }

    size_t l = _tcslen(pDestPath)-1;
    bool bIsFileName = false;
    // trim final backslash; if present, assume it's dir
    if(pDestPath[l]=='\\' && pDestPath[l-1]!=':')
        pDestPath[l] = '\0';
    else if (pDestPath[l-1]!=':')
    {
        // no backslash or colon; if not exist or is not directory, it's file
        DWORD attr = GetFileAttributes(pDestPath);
        if(attr==-1 || !(attr&FILE_ATTRIBUTE_DIRECTORY) )
            bIsFileName = true;
    }
    if(bIsFileName) // Add ext unless it contains it already
    {
        PCTSTR p = _tcsrchr(pDestPath,'\\');
        if(!p) p = pDestPath;
        if(!_tcschr(p,'.') && ext) // if doesn't contain extension, and extension given
            _tcscat(pDestPath, ext);
    }
    return bIsFileName ? DESTPATH_FILE : DESTPATH_DIR;
}

int YMSPlugin::GetFiles(PluginPanelItem *PanelItem,size_t nItems,int /*Move*/,WCONST WTYPE DestPath1,int OpMode,bool bGetGroup)
{
    if (nItems==0) return FALSE;

    OnEnterGetFiles();

    TCHAR DestPath[MAX_PATH]; _tcsncpy(DestPath,WDEREF DestPath1, _countof(DestPath));
    for(TCHAR*p = DestPath; *p; p++) if(*p=='/') *p = '\\';
    bool bOverAll=0, bSkipAll=0;
    int nTotalExportedKeys = 0;
    int nTotalExportedValues = 0;

    size_t l = _tcslen(DestPath)-1;
    if(DestPath[l]!='\\' && DestPath[l]!=':')
    {
        DestPath[l+1] = '\\';
        DestPath[l+2] = '\0';
    }

    const bool bSilent = (OpMode&(OPM_SILENT|OPM_FIND))!=0;
    DWORD dwDialogRet = 0;

    if(!bSilent)
    {
        dwDialogRet = GetFilesDialog(DestPath, _countof(DestPath), PanelItem->FileName, nItems);
        if(!(dwDialogRet&1))
            return FALSE;
    }

    bool bIsFileName;
    switch(CheckDestPath(DestPath, PanelItem, nItems, GetFileExt()))
    {
        // in reg plugin, GetFileExt() after GetFilesDialog will return either ".reg" or ""
        // hive and "alldirs" logic put somewhere there
        case DESTPATH_ERROR: return FALSE;
        case DESTPATH_DIR: bIsFileName = false; break;
        case DESTPATH_FILE: bIsFileName = true; break;
    }
    if(bSilent)
        bIsFileName = false;
    /*if(OpMode&OPM_EDIT)
        bIsFileName = true;*/

    int rc = TRUE;

    for(size_t i=0; i<nItems; i++)
    {
        PluginPanelItem& item = PanelItem[i];

        try
	{
            // OnMakeFileName tells GetFileExt what to return
            OnMakeFileName(item);

            TCHAR* filename = LastMadeName;
            if(!bGetGroup || nTotalExportedKeys==0)
	    {
                filename = bIsFileName ? DestPath :
                    MakeFileName(DestPath, item.FileName, OpMode);
            }
            int iConf = bSilent ? 1 :
                ConfirmOverwrite(filename, bOverAll, bSkipAll, false);
            if(iConf==-1)
	    {
                rc = -1;
                break;
            }
            if(iConf==0)
	    {
                rc = FALSE;
                continue;
            }

            OnGetItem(item);
            ExportItem(item, filename, bGetGroup&&i!=0);
            OnAfterGetItem(item);
            item.Flags&=~PPIF_SELECTED;
            nTotalExportedKeys++;
        } //try 
        catch(WinExcept ret)
	{
            OnAfterGetItem(item);
            if(!(OpMode&OPM_FIND) && !ret.GetSilent())
                WinError(ret);
            rc = (ret==ERROR_CANCELLED);
            break;
        }
        catch(ActionException e)
	{
            OnAfterGetItem(item);
            rc = FALSE;
            if(e) { rc=-1; break; }
        }
    } // for
    OnAfterGetFiles();
    return bSkipAll ? -1 : rc;
}

#ifdef UNICODE
#define DataPtr(i) ((LPCTSTR)StartupInfo.SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
#else
#define DataPtr(i) CopyDlgItems[i].Data
#endif

#ifdef FAR3
// {54780B3C-1ECE-41BE-8E7C-786A7F51492D}
DEFINE_GUID(CopyDialogGuid, 0x54780b3c, 0x1ece, 0x41be, 0x8e, 0x7c, 0x78, 0x6a, 0x7f, 0x51, 0x49, 0x2d);
#endif

DWORD YMSPlugin::GetFilesDialog(TCHAR* DestPath, DWORD cchSize, LPCTSTR pInitialName, size_t nItems)
{
	FarDialogItem CopyDlgItems[] = {
		DIALOG_ITEM(DI_DOUBLEBOX, 3,1, 72,6, 0,0)
		DIALOG_ITEM_D(DI_TEXT, 5,2, 0,0, 0,0)
		DIALOG_ITEM_F(DI_EDIT, 5,3, 70,0, GetFilesHistoryId(),DIF_HISTORY)
		DIALOG_ITEM(DI_TEXT,4,4,0,0,0,DIF_SEPARATOR)
		DIALOG_ITEM_D(DI_BUTTON,0,5,0,0,0,DIF_CENTERGROUP)
		DIALOG_ITEM(DI_BUTTON,0,5,0,0,0,DIF_CENTERGROUP)
	};
    SetData(CopyDlgItems[0], GetMsg(MCopy));
    TCHAR buf[512];
    if(nItems>1) 
        _sntprintf(buf, _countof(buf), GetMsg(MCopyItemsToDir), nItems);
    else
	_sntprintf(buf, _countof(buf), GetMsg(MCopyToDir),
            !_tcscmp(pInitialName, _T("..")) ? (*CurDir?CurDir:_T("\\")) : pInitialName );
    SetData(CopyDlgItems[1], buf);
    SetData(CopyDlgItems[2], DestPath, cchSize);
    SetData(CopyDlgItems[4], GetMsg(MOK));
    SetData(CopyDlgItems[5], GetMsg(MCancel));
    RUN_DIALOG(CopyDialogGuid,-1,-1,76,8,_T("CopyMove"),CopyDlgItems,_countof(CopyDlgItems));
    if(DIALOG_RESULT==-1)
        return 0;
    _tcsncpy(DestPath,GetItemText(CopyDlgItems, 2), cchSize-1);
    StripQuotes(DestPath);
    return 1;
}

void GetCurrentDescription(PluginPanelItem& item, TCHAR* desc, DWORD count)
{
    if(!item.Description)
        *desc = 0;
    else
    {
        _tcsncpy(desc, item.Description, count);
        desc[count-1]=0;
    }
}
#endif

void YMSPlugin::PutToCmdLine(PCTSTR tmp, bool bSpace)
{
    size_t l = _tcslen(tmp);
    TCHAR* tmp1 = 0;
    if(_tcscspn(tmp,_T(" &^"))!=l)
    {
        tmp1 = new TCHAR[l+3];
        memcpy(tmp1+1,tmp,l*sizeof(TCHAR));
        tmp1[0] = tmp1[l+1] = '\"';
        tmp1[l+2] = '\0';
        tmp = tmp1;
    }
    TCHAR cmdbuf[1024];
#ifdef UNICODE
    Control(FCTL_GETCMDLINE, _countof(cmdbuf), cmdbuf);
#else
    Control(FCTL_GETCMDLINE, cmdbuf);
#endif
    _tcscat(cmdbuf, tmp);
    if(bSpace) _tcscat(cmdbuf, _T(" "));
    Control(FCTL_SETCMDLINE, cmdbuf);
    delete[] tmp1;
}

void YMSPlugin::CreateTmpDir(TCHAR* TmpPath)
{
    TCHAR RootTmpPath[MAX_PATH];
    if(GetTempPath(_countof(RootTmpPath), RootTmpPath)==0)
        *RootTmpPath = 0;
    else
    {
        TCHAR* p;
        if(p=_tcschr(RootTmpPath,';')) *p = 0;
        if(RootTmpPath[_tcslen(RootTmpPath)-1]=='\0')
            _tcscat(RootTmpPath, _T("\\"));
    }
    _tcscat(RootTmpPath, _T("far%d"));
    for(int i=0; i<=999; i++) 
    {
        _sntprintf(TmpPath, _countof(RootTmpPath), RootTmpPath, i);
        if(GetFileAttributes(TmpPath)==0xffffffff) break;
    }
    CreateDirectory(TmpPath,0);
}

#ifdef FAR3
// {B0F0C168-3B47-41D9-826B-008F6797D5B1}
DEFINE_GUID(DescDialogGuid, 0xb0f0c168, 0x3b47, 0x41d9, 0x82, 0x6b, 0x0, 0x8f, 0x67, 0x97, 0xd5, 0xb1);
#endif

#ifndef NOPANELS
BOOL YMSPlugin::ProcessKey(int key, unsigned int controlState)
{
    if(!bKeysAsDirs && (key==VK_RETURN && controlState==0 || key==VK_DOWN && controlState==PKF_CONTROL))
    {
        SetDirectory(
#ifdef UNICODE
	    unique_ptr<PluginPanelItem>
#endif
		(GetCurrentItem())->FileName);
        Reread();
        return TRUE;
    }
    if(key==VK_F7 && controlState==PKF_SHIFT)
    {
        bKeysAsDirs^=1;
        Reread();
        return TRUE;
    }
    if((key=='G' || key=='Z') && controlState==PKF_CONTROL)
    {
        CURRENTITEM_PTR(item);
        TCHAR desc[1024];
        GetCurrentDescription(*item, desc, _countof(desc));
        if(key=='G')
	{
            if(*desc)
                PutToCmdLine(desc);
            return TRUE;
        }
        else
	{
            if(!_tcscmp(item->FileName, _T("..")))
                return TRUE;
            FarDialogItem Items[] = {
                DIALOG_ITEM(DI_DOUBLEBOX, 3,1, 72,4, 0,0)
                DIALOG_ITEM_D(DI_TEXT, 5,2, 18,2, 0,0)
                DIALOG_ITEM_F(DI_EDIT, 5,3, 70,3, DescHistoryId(),DIF_HISTORY)
            };
            SetData(Items[0], GetMsg(MCtrlZTitle));
            SetData(Items[1], GetCtrlZPrompt());
            SetData(Items[2], desc);
            while(1)
	    {
                RUN_DIALOG(DescDialogGuid,-1,-1,76,6,_T("CtrlZ"),Items,_countof(Items));				
                if(DIALOG_RESULT==-1)
                    break;
                ToAnsi((LPTSTR)GetItemText(Items,2));
                if(!ChangeDesc(*item, GetItemText(Items,2))) //error
                    WinError(GetMsg(MCtrlZIncorrectData));
                else
		{
                    Reread();
                    break;
                }
            }
        }
        return TRUE;
    }
    if(key==VK_CLEAR)
        key = VK_F3;
    if((key==VK_F3||key==VK_F4) && (
        controlState==0 || controlState==(PKF_SHIFT|PKF_ALT) )
        )
    {
        PluginPanelItem item;
        PluginPanelItem* items = 0;
        size_t nItems;
        vector<vector<BYTE>> item_buf;

        if(controlState) // Alt-Shift...
	{
            PanelInfo panel;
            Control(FCTL_GETPANELINFO, &panel);
            nItems = panel.SelectedItemsNumber;
            if(!nItems)
                return FALSE;
#ifdef UNICODE
            items = new PluginPanelItem[nItems];
            item_buf.resize(nItems);
            for(size_t i=0; i<nItems; i++)
                memcpy(&items[i], GetSelectedPanelItem(i, item_buf[i]), sizeof items[i]);
        }
	else
	{
            items = GetCurrentItem();
#else
            items = panel.SelectedItems;
            if(!items)
                return FALSE;
        }
	else
	{
            item = *GetCurrentItem();
            items = &item;
#endif
            // Let FAR process it, if it's not dir, and we're not in "keys as dirs" mode
            if((!bKeysAsDirs || !IsFolder(*items)) && _tcscmp(items->FileName,_T("..")))
            {
#ifdef UNICODE
                delete[] items;
#endif
                return FALSE;
            }
            //in "keys as files", we process only ".."
            nItems = 1;
        }

        TCHAR TmpPath[MAX_PATH];
        CreateTmpDir(TmpPath);

        int hRes=1;
        int mode = OPM_SILENT;
        if(key==VK_F3) mode |= OPM_VIEW;
        if(key==VK_F4) mode |= OPM_EDIT;
        WCONST TCHAR* tmpPath = TmpPath;
        if(GetFiles(items, nItems, 0, WADDR tmpPath, mode, controlState!=0) != 1 )
            goto _1;
        OpenPluginInfo info;
        TCHAR Title[256]; Title[_countof(Title)-1]=0;
        GetOpenPluginInfo(info);
        int ret;
        if(controlState || !_tcscmp(items->FileName,_T("..")))
	{
            ret=_sntprintf(Title, _countof(Title)-1, /*PFX*/_T("%s"), info.CurDir);
            mode |= OPM_TWODOTS;
        }
        else
            ret=_sntprintf(Title,_countof(Title)-1,*info.CurDir ? /*PFX*/_T("%s\\%s"):/*PFX*/_T("%s%s"),info.CurDir,items->FileName);
        if(ret==-1)
            _tcscpy(Title+_countof(Title)-4,_T("..."));
        if(key==VK_F4)
	{
            if(Editor(LastMadeName,Title,0/*EF_IMMEDIATERETURN|EF_NONMODAL|EF_ENABLE_F6|EF_DELETEONCLOSE*/)==1)
	    {
                /*EditorInfo ei;
                GetEditorInfo(ei);*/
                //TCHAR CurPath[MAX_PATH];
                //GetCurrentDirectory(_countof(CurPath), CurPath);
                //SetCurrentDirectory(TmpPath);
                PluginPanelItem item;
                memset(&item, 0, sizeof item);
#ifdef UNICODE
                WIN32_FIND_DATA wfd;
                HANDLE hTmp = FindFirstFile(LastMadeName, &wfd);
#ifdef FAR3
		item.FileAttributes = wfd.dwFileAttributes;
		item.CreationTime = wfd.ftCreationTime;
		item.LastAccessTime = wfd.ftLastAccessTime;
		item.LastWriteTime = wfd.ftLastWriteTime;
		item.AlternateFileName = wfd.cAlternateFileName;
#else
                memcpy(&item.FindData, &wfd, offsetof(WIN32_FIND_DATA, nFileSizeHigh));
#endif
                *(DWORD*)&item.FileSize = wfd.nFileSizeLow;
                *(DWORD*)(&item.FileSize+1) = wfd.nFileSizeHigh;
                item.FileName = wfd.cFileName;
#else
                HANDLE hTmp = FindFirstFile(LastMadeName, &item.FindData);
#endif
                hRes = PutFiles(&item, 1, 0, mode, TmpPath);
                FindClose(hTmp);
                //SetCurrentDirectory(CurPath);
                Reread();
            }
        }
        else
            Viewer(LastMadeName,Title);
        if(hRes>0)
            DeleteFile(LastMadeName);
_1:
#ifdef UNICODE
        if(items != &item)
            delete[] items;
#endif
        RemoveDirectory(TmpPath);
        return TRUE;
    }
    return FALSE;
}

int YMSPlugin::ProcessEvent(int Event, void *Param)
{
    if(Event==FE_CLOSE)
        return CloseEvent();
    return FALSE;
}

void WriteBufToFile(PCTSTR filename, BYTE* data, DWORD cbData, bool bAppend)
{
    DWORD nb, sz = cbData==-1 ? (DWORD)_tcslen((TCHAR*)data)*sizeof(TCHAR) : cbData;
    HANDLE hFile;
    if( (hFile=CreateFile(filename, GENERIC_WRITE,FILE_SHARE_READ,0,bAppend?OPEN_ALWAYS:CREATE_ALWAYS,
        FILE_FLAG_SEQUENTIAL_SCAN,0))==INVALID_HANDLE_VALUE ||
        bAppend && !SetFilePointer(hFile,0,0,FILE_END) ||
        !WriteFile(hFile, data, sz, &nb, 0) || !CloseHandle(hFile) || nb!=sz )
        throw FileExcept(GetLastError());
}

void YMSPlugin::FreeFindData(PluginPanelItem* PanelItem, int count)
{
    for(int i=0; i<count; i++)
    {
        PluginPanelItem& item = PanelItem[i];
        FreeFileNames(item);
        delete[] item.Owner;
        delete[] item.Description;
    }
    delete[] PanelItem;
}
#endif

void YMSPlugin::Reread(bool bOther/*unused*/, bool bKeepPosition, bool bKeepSelection)
{
    PanelInfo panel;
    Control(FCTL_GETPANELINFO, &panel);
#ifdef UNICODE
    Control(FCTL_UPDATEPANEL, bKeepSelection ? 1 : 0, NULL);
#else
    Control(FCTL_UPDATEPANEL, bKeepSelection ? (void*)1 : NULL);
#endif
    ControlRedraw(bKeepPosition ? panel.CurrentItem : 0, bKeepPosition ? panel.TopPanelItem : 0);
}

void YMSPlugin::GetOpenPluginInfo(OpenPluginInfo& info)
{
    info.StructSize = sizeof info;
    _tcsncpy(CurDir, CurrentDir, _countof(CurDir)-1);
    CurDir[_countof(CurDir)-1] = 0;
    info.CurDir = CurDir;

    sShortcutData.clear();
    GetShortcutData(sShortcutData);
    info.ShortcutData = sShortcutData.empty() ? NULL : sShortcutData.c_str();
}

PluginStartupInfo StartupInfo;
static FARSTANDARDFUNCTIONS FSF;

#include "PluginSettings.hpp"

#ifndef FAR3
TCHAR PluginRootKey[MAX_PATH];
#endif

void YMSPlugin::SetStartupInfo(PluginStartupInfo* Info, PCTSTR RegRoot)
{
    StartupInfo = *Info;
    FSF = *Info->FSF;
    StartupInfo.FSF = &FSF;
#ifndef FAR3
    _sntprintf(PluginRootKey, _countof(PluginRootKey), _T("%s\\%s"), Info->RootKey, RegRoot);
#endif
}

TCHAR* MakeItemDesc(PCTSTR pText, DWORD dwLimit)
{
    size_t l = (DWORD)_tcslen(pText);
    if(l)
    {
        if(dwLimit && l>dwLimit-1) l = dwLimit-1;
        TCHAR* pDesc = new TCHAR[l+1];
        _tcsncpy(pDesc, pText, l);
        pDesc[l] = 0;
        ToOem(pDesc);
        return pDesc;
    } else
        return NULL;
}
TCHAR* MakeItemDesc(DWORD dwValue, int iRadix)
{
    TCHAR* pDesc = new TCHAR[12];
    _ultot(dwValue, pDesc, iRadix);
    return pDesc;
}
#ifndef UNICODE
TCHAR* MakeItemDesc(PCWSTR pText, DWORD dwLimit)
{
    DWORD l = WideCharToMultiByte(CP_OEMCP, 0, pText, -1, 0, 0, 0, 0);
    //DWORD l = (DWORD)wcslen(pText);
    if(l)
    {
        if(dwLimit && l>dwLimit) l = dwLimit;
        char* pDesc = new char[l+1];
        WideCharToMultiByte(CP_OEMCP, 0, pText, -1, pDesc, l+1, 0, 0);
        pDesc[l] = 0;
        return pDesc;
    } else
        return NULL;
}
#endif
bool ReadBuffer(PCTSTR pFile, BYTE* buf, DWORD dwSize, DWORD* pdwRead, BYTE** pNewBuf)
{
    SetLastError(0);
    HANDLE hFile;
    if((hFile=CreateFile(pFile, GENERIC_READ,FILE_SHARE_READ,0,
        OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0))==INVALID_HANDLE_VALUE)
        throw WinExcept();

    if(buf==0)
    {
        dwSize = GetFileSize(hFile, 0);
        buf = new BYTE[dwSize+2];
        buf[dwSize] = buf[dwSize+1] = 0; // two zeroes for unicode buffer
        *pNewBuf = buf;
    }
    else if(GetFileSize(hFile, 0)<dwSize) // size is less than required
    {
        if(!pdwRead) 	    // no way to return this fact
	{
            CloseHandle(hFile);
            return false;
        }
        else
            dwSize = GetFileSize(hFile, 0); // read less
    }    

    DWORD nRead;
    if(!pdwRead)
        pdwRead = &nRead;
    if(ReadFile(hFile, buf, dwSize, pdwRead, 0)==FALSE || *pdwRead!=dwSize)
    {
        DWORD err = GetLastError();
        CloseHandle(hFile);
        throw WinExcept(err);
    }
    CloseHandle(hFile);
    return true;
}

/*#ifdef FAR3
static PluginSettings* Settings = NULL;
static void InitSettings()
{
    if(Settings == NULL)
    {
	Settings = new PluginSettings(PluginGuid, StartupInfo.SettingsControl);
    }
}
void CloseSettings()
{
    delete Settings;
    Settings = NULL;
}
#endif

DWORD GetReg(PCTSTR name, DWORD deflt)
{
#ifdef FAR3
    InitSettings();
    return Settings->Get(0, name, deflt);
#else
    HKEY hKey; DWORD Type; DWORD val, size=sizeof val;
    DWORD dwRet = deflt;
    if(RegOpenKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, KEY_QUERY_VALUE, &hKey)==ERROR_SUCCESS)
    {
        if(RegQueryValueEx(hKey, name, 0, &Type, (PBYTE)&val, &size)==ERROR_SUCCESS)
            dwRet = val;
        RegCloseKey(hKey);
    }
    return dwRet;
#endif
}

TCHAR* GetReg(PCTSTR name, TCHAR* buf, DWORD cchSize, PCTSTR deflt)
{
#ifdef FAR3
    InitSettings();
    Settings->Get(0, name, buf, cchSize, deflt);
    return buf;
#else
    HKEY hKey=0; DWORD Type; DWORD size=cchSize*sizeof(TCHAR);
    if(RegOpenKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, KEY_QUERY_VALUE, &hKey)!=ERROR_SUCCESS ||
        RegQueryValueEx(hKey, name, 0, &Type, (BYTE*)buf, &size)!=ERROR_SUCCESS)
        _tcsncpy(buf, deflt, cchSize);
    RegCloseKey(hKey);
#endif
    return buf;
}

tstring GetRegS(LPCTSTR name, PCTSTR dflt)
{
#ifdef FAR3
    InitSettings();
    return Settings->Get(0, name, dflt);
#else
    tstring str;
    HKEY hKey=0; DWORD Type; DWORD size=0;
    if(RegOpenKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, KEY_QUERY_VALUE, &hKey)!=ERROR_SUCCESS ||
       RegQueryValueEx(hKey, name, 0, &Type, NULL, &size)!=ERROR_SUCCESS)
    {
        str = dflt;
    }
    else
    {
	str.reserve(size/sizeof(TCHAR));
	str.resize(size/sizeof(TCHAR));
	if(RegQueryValueEx(hKey, name, 0, &Type, (BYTE*)str.c_str(), &size)!=ERROR_SUCCESS)
	    str = dflt;
	else // remove trailing \0
	    str.resize((size-1)/sizeof(TCHAR));
    }
    RegCloseKey(hKey);
    return str;
#endif
}

void SetReg(PCTSTR name, BYTE* val, DWORD bufsize, DWORD Type)
{
#ifdef FAR3
	InitSettings();
	Settings->Set(0, name, val, bufsize);
#else
    HKEY hKey; DWORD disp;
    if(RegCreateKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, 0, 0, KEY_SET_VALUE, 0, &hKey, &disp)!=ERROR_SUCCESS) return;
    RegSetValueEx(hKey, name, 0, Type, val, bufsize);
    RegCloseKey(hKey);
#endif
}

#ifdef FAR3
void SetReg(LPCTSTR name, LPCWSTR  buf)
{
    InitSettings();
    Settings->Set(0, name, buf);
}
void SetReg(LPCTSTR name, DWORD wrd)
{
    InitSettings();
    Settings->Set(0, name, wrd);
}
#endif
*/
FarDialogItem* MakeDialogItems(FarDialogItemID *ItemID, int count)
{
    FarDialogItem* item = new FarDialogItem[count];
    memset(item, 0, sizeof *item * count);
    for(int i=0; i<count; i++)
    {
        item[i].Type = 
#ifdef FAR3
            (FARDIALOGITEMTYPES)
#endif
                    ItemID[i].Type;
        item[i].X1 = ItemID[i].X1;
        item[i].Y1 = ItemID[i].Y1;
        item[i].X2 = ItemID[i].X2;
        item[i].Y2 = ItemID[i].Y2;
        item[i].Selected = (int)ItemID[i].Selected;
        item[i].Flags = ItemID[i].Flags;
#ifdef FAR3
	if(ItemID[i].Focus)
	    item[i].Flags |= DIF_FOCUS;
	if(ItemID[i].DefaultButton)
	    item[i].Flags |= DIF_DEFAULTBUTTON;
#else
        item[i].Focus = ItemID[i].Focus;
        item[i].DefaultButton = ItemID[i].DefaultButton;
#endif
        if(ItemID[i].Flags & DIF_HISTORY)
            item[i].History = (PCTSTR)ItemID[i].Selected;
        if(ItemID[i].ID)
            SetData(item[i], GetMsg(ItemID[i].ID));
    }
    return item;
}

KeyBarTitles* YMSPlugin::MakeKeyBarTitles(KeyBarItem* items, int count)
{
    DeleteKeyBarTitles();
    pKeyBarTitles = new KeyBarTitles;
#ifdef FAR3
    pKeyBarTitles->CountLabels = count;
    pKeyBarTitles->Labels = new KeyBarLabel[count];
    for(int i=0; i<count; i++)
    {
	pKeyBarTitles->Labels[i].Key.VirtualKeyCode = items[i].VirtualKeyCode;
	pKeyBarTitles->Labels[i].Key.ControlKeyState = items[i].ControlKeyState;
	pKeyBarTitles->Labels[i].Text = pKeyBarTitles->Labels[i].LongText = items[i].MsgID == -1 ? _T("") : GetMsg(items[i].MsgID);
    }
#else
    memset(pKeyBarTitles, 0, sizeof *pKeyBarTitles);
    for(int i=0; i<count; i++)
    {
	LPTSTR* p = NULL;
	switch(items[i].ControlKeyState)
	{
	    case 0:
		p = pKeyBarTitles->Titles;
		break;
	    case LEFT_CTRL_PRESSED:
		p = pKeyBarTitles->CtrlTitles;
		break;
	    case LEFT_ALT_PRESSED:
		p = pKeyBarTitles->AltTitles;
		break;
	    case SHIFT_PRESSED:
		p = pKeyBarTitles->ShiftTitles;
		break;
	    case LEFT_CTRL_PRESSED|SHIFT_PRESSED:
		p = pKeyBarTitles->CtrlShiftTitles;
		break;
	    case LEFT_ALT_PRESSED|SHIFT_PRESSED:
		p = pKeyBarTitles->AltShiftTitles;
		break;
	    case LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED:
		p = pKeyBarTitles->CtrlAltTitles;
		break;
	}
	p[items[i].VirtualKeyCode - VK_F1] = items[i].MsgID == -1 ? _T("") : GetMsg(items[i].MsgID);
    }
#endif
    return pKeyBarTitles;
}

#ifdef UNICODE
PluginPanelItem* YMSPlugin::GetSelectedPanelItem(size_t index, vector<BYTE>& item_buf)
{
    int sz = (int)Control(FCTL_GETSELECTEDPANELITEM, index, NULL);
    item_buf.resize(sz);
#ifdef FAR3
    FarGetPluginPanelItem fgppi = { sizeof fgppi, sz, (PluginPanelItem*)&item_buf[0] };
    Control(FCTL_GETSELECTEDPANELITEM, index, &fgppi);
#else
    Control(FCTL_GETSELECTEDPANELITEM, index, &item_buf[0]);
#endif
    return (PluginPanelItem*)&item_buf[0];
}

PluginPanelItem* YMSPlugin::GetPanelItem(size_t index, vector<BYTE>& item_buf)
{
    intptr_t sz = Control(FCTL_GETPANELITEM, index, NULL);
    item_buf.resize(sz);
#ifdef FAR3
    FarGetPluginPanelItem fgppi = { sizeof fgppi, sz, (PluginPanelItem*)&item_buf[0] };
    Control(FCTL_GETPANELITEM, index, &fgppi);
#else
    Control(FCTL_GETPANELITEM, index, &item_buf[0]);
#endif
    return (PluginPanelItem*)&item_buf[0];
}

#endif

#ifndef FAR3
int YMSPlugin::GetFarVersion()
{
    static int iFarVersion = -1;
    if(iFarVersion==-1)
        iFarVersion = StartupInfo.StructSize < offsetof(PluginStartupInfo, AdvControl) ?
            0 : (int)AdvControl(ACTL_GETFARVERSION, 0);
    return iFarVersion;
};
#endif

#ifndef UNICODE
// Compare strings using CompareStringW or CompareString if it's missing.
int mystrcmpi(PCTSTR str1, PCTSTR str2, PCWSTR wstr1)
{
  static int supportedUnicode = -1;

    if(supportedUnicode == -1) // check once for lifetime
	supportedUnicode = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, L"a", -1, L"A", -1) == CSTR_EQUAL;

    return supportedUnicode ?
		lstrcmpiW(wstr1 ? wstr1 : WideFromOem(str1), WideFromOem(str2)) :
		lstrcmpiA(str1, str2);
}
#endif
