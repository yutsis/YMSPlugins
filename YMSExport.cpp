#include "YMSPlugin.h"

#ifdef FAR3

void WINAPI YMSExport::ClosePanelW(const ClosePanelInfo *info)
{
    if(info->hPanel != INVALID_HANDLE_VALUE)
	delete (YMSPlugin*)info->hPanel;
    _CrtDumpMemoryLeaks();
}

int WINAPI YMSExport::DeleteFilesW(const DeleteFilesInfo *info)
{
    return ((YMSPlugin *)info->hPanel)->DeleteFiles(info->PanelItem, (int)info->ItemsNumber, (int)info->OpMode);
}

void WINAPI YMSExport::FreeFindDataW(GetFindDataInfo *info)
{
    ((YMSPlugin *)info->hPanel)->FreeFindData(info->PanelItem, (int&)info->ItemsNumber);
}

int WINAPI YMSExport::GetFilesW(GetFilesInfo *info)
{
    return ((YMSPlugin *)info->hPanel)->GetFiles(info->PanelItem, info->ItemsNumber,info->Move, &info->DestPath, (int)info->OpMode);
}

int WINAPI YMSExport::GetFindDataW(GetFindDataInfo *info)
{
    return info->hPanel == INVALID_HANDLE_VALUE ? 0 :
	((YMSPlugin *)info->hPanel)->GetFindData(info->PanelItem, (int&)info->ItemsNumber, (int)info->OpMode);
}

void WINAPI YMSExport::GetOpenPanelInfoW(OpenPluginInfo *info)
{
    if(info->hPanel != INVALID_HANDLE_VALUE)
	((YMSPlugin *)info->hPanel) -> GetOpenPluginInfo(*info);
}

int WINAPI YMSExport::MakeDirectoryW(MakeDirectoryInfo *info)
{
    return ((YMSPlugin *)info->hPanel)->MakeDirectory(&info->Name, (int)info->OpMode);
}

HANDLE WINAPI YMSExport::OpenW(const OpenInfo *info)
{
    if(info->StructSize < sizeof *info)
	return NULL;

    if(info->OpenFrom == OPEN_ANALYSE)
	return ((OpenAnalyseInfo*)info->Data)->Handle;
    else
    {
	extern HANDLE WINAPI OpenPluginW(int OpenFrom,int Item);
	HANDLE h = OpenPluginW((int)info->OpenFrom, (int)info->Data);
	return h == INVALID_HANDLE_VALUE ? NULL : h;
    }

    return NULL;
}

int WINAPI YMSExport::ProcessPanelEventW(const ProcessPanelEventInfo *info)
{
    return info->StructSize < sizeof(ProcessPanelEventInfo) || info->hPanel == INVALID_HANDLE_VALUE ? 0 :
        ((YMSPlugin *)info->hPanel)->ProcessEvent((int)info->Event, info->Param);
}

int WINAPI YMSExport::ProcessPanelInputW(const ProcessPanelInputInfo *info)
{
    if(info->StructSize < sizeof *info || info->Rec.EventType != KEY_EVENT)
	    return FALSE;

    int pkf_flags = 0;
    if(info->Rec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
	    pkf_flags |= PKF_CONTROL;
    if(info->Rec.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
	    pkf_flags |= PKF_ALT;
    if(info->Rec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
	    pkf_flags |= PKF_SHIFT;

    return ((YMSPlugin *)info->hPanel)->ProcessKey(info->Rec.Event.KeyEvent.wVirtualKeyCode, pkf_flags);
}

int WINAPI YMSExport::PutFilesW(const PutFilesInfo *info)
{
    return ((YMSPlugin *)info->hPanel)->PutFiles(info->PanelItem,(int)info->ItemsNumber,info->Move,
        (int)info->OpMode, info->SrcPath);
}

int WINAPI YMSExport::SetDirectoryW(const SetDirectoryInfo *info)
{
    try {
        return ((YMSPlugin *)info->hPanel)->SetDirectory(info->Dir, (int)info->OpMode);
    } catch(...) { return FALSE; }
}

#else

void WINAPI YMSExport::EXP_NAME(ClosePlugin)(HANDLE hPlugin)
{
  delete (YMSPlugin*)hPlugin;
  _CrtDumpMemoryLeaks();
}

int WINAPI YMSExport::EXP_NAME(DeleteFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem,int ItemsNumber,int OpMode)
{
  return ((YMSPlugin *)hPlugin)->DeleteFiles(PanelItem,ItemsNumber,OpMode);
}

void WINAPI YMSExport::EXP_NAME(FreeFindData)(HANDLE hPlugin, PluginPanelItem *PanelItem,int ItemsNumber)
{
  ((YMSPlugin *)hPlugin)->FreeFindData(PanelItem,ItemsNumber);
}

int WINAPI YMSExport::EXP_NAME(GetFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem,
                   int ItemsNumber, int Move, WCONST WTYPE DestPath,int OpMode)
{
  return ((YMSPlugin *)hPlugin)->GetFiles(PanelItem,ItemsNumber,Move,DestPath,OpMode);
}

int WINAPI YMSExport::EXP_NAME(GetFindData)(HANDLE hPlugin, PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode)
{
  return ((YMSPlugin *)hPlugin)->GetFindData(*pPanelItem, *pItemsNumber, OpMode);
}

void WINAPI YMSExport::EXP_NAME(GetOpenPluginInfo)(HANDLE hPlugin, OpenPluginInfo *Info)
{
  ((YMSPlugin *)hPlugin) -> GetOpenPluginInfo(*Info);
}

int WINAPI YMSExport::EXP_NAME(MakeDirectory)(HANDLE hPlugin, WCONST WTYPE Name, int OpMode)
{
   return ((YMSPlugin *)hPlugin)->MakeDirectory (Name, OpMode);
}

int WINAPI YMSExport::EXP_NAME(ProcessEvent)(HANDLE hPlugin,int Event,void *Param)
{
   return ((YMSPlugin *)hPlugin)->ProcessEvent(Event, Param);
}

int WINAPI YMSExport::EXP_NAME(ProcessKey)(HANDLE hPlugin, int Key, unsigned int ControlState)
{
   return ((YMSPlugin *)hPlugin)->ProcessKey(Key, ControlState);
}

int WINAPI YMSExport::EXP_NAME(PutFiles)(HANDLE hPlugin, PluginPanelItem *PanelItem, int ItemsNumber, int Move,
#if UNICODE
		        PCWSTR SrcPath,
#endif
		        int OpMode)
{
  return ((YMSPlugin *)hPlugin)->PutFiles(PanelItem, ItemsNumber, Move, OpMode
#if UNICODE
			, SrcPath
#endif
	    );
}

int WINAPI YMSExport::EXP_NAME(SetDirectory)(HANDLE hPlugin, PCTSTR Dir, int iOpMode)
{
    try {
        return ((YMSPlugin *)hPlugin)->SetDirectory(Dir, iOpMode);
    } catch(...) { return FALSE; }
}

#endif
