#pragma once

#include "plugin.hpp"

class PluginSettings;

class PluginSettings
{
#ifdef FAR3
    HANDLE handle;
    FARAPISETTINGSCONTROL SettingsControl;
#define KEY_TYPE int
#else
    HKEY hKey;
    vector<HKEY> subKeys;
#define KEY_TYPE HKEY
#endif

public:
    PluginSettings(
#ifdef FAR3
        FARAPISETTINGSCONTROL settingsControl, const GUID &guid)
    {
        SettingsControl = settingsControl;
	handle = INVALID_HANDLE_VALUE;

	FarSettingsCreate settings = {sizeof settings,guid,handle};
	if(SettingsControl(INVALID_HANDLE_VALUE,SCTL_CREATE,PSL_ROAMING,&settings))
	    handle = settings.Handle;
#else
        bool bWrite)
    {
        extern TCHAR PluginRootKey[];
        DWORD disp;
        if((bWrite ?
            RegCreateKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, 0, 0, KEY_WRITE|KEY_READ, 0, &hKey, &disp) :
            RegOpenKeyEx(HKEY_CURRENT_USER, PluginRootKey, 0, KEY_READ, &hKey))
                !=ERROR_SUCCESS)
                hKey = 0;
#endif
    }

    ~PluginSettings()
    {
#ifdef FAR3
	SettingsControl(handle,SCTL_FREE,0,0);
#else
        for each(HKEY hk in subKeys)
            RegCloseKey(hk);
        subKeys.clear();
        if(hKey)
            RegCloseKey(hKey);
        hKey = 0;
#endif
    }

    KEY_TYPE CreateSubKey(PCWSTR Name, KEY_TYPE root = 0)
    {
#ifdef FAR3
	FarSettingsValue value = {sizeof value, root, Name};
	return (KEY_TYPE)SettingsControl(handle,SCTL_CREATESUBKEY,0,&value);
#else
        if(root == 0) root = hKey;
        HKEY hk;
        if(RegCreateKeyW(root, Name, &hk) != ERROR_SUCCESS)
            return 0;
        subKeys.push_back(hk);
        return hk;
#endif
    }

    KEY_TYPE OpenSubKey(PCTSTR szName, KEY_TYPE root = 0
#ifndef FAR3
        , bool bWrite = false
#endif
        )
    {
#ifdef FAR3
	FarSettingsValue value = {sizeof value, root, szName};
	return (KEY_TYPE)SettingsControl(handle,SCTL_OPENSUBKEY,0,&value);
#else
        if(root == 0) root = hKey;
        HKEY hk;
        if(RegOpenKeyEx(root, szName, 0, bWrite ? KEY_WRITE|KEY_READ : KEY_READ, &hk) != ERROR_SUCCESS)
            return 0;
         subKeys.push_back(hk);
         return hk;
#endif
    }

    bool DeleteSubKey(
#ifdef FAR3
                KEY_TYPE root)
    {
	FarSettingsValue value = {sizeof value, root, NULL};
	return SettingsControl(handle,SCTL_DELETE,0,&value) != 0;
#else
                PCWSTR szName, KEY_TYPE root = 0)
    {
        if(root == 0) root = hKey;
        return RegDeleteKeyW(root, szName) == ERROR_SUCCESS;
#endif
    }

    bool DeleteValue(PCTSTR szName, KEY_TYPE root = 0)
    {
#ifdef FAR3
	FarSettingsValue value = {sizeof value, root, szName};
	return SettingsControl(handle,SCTL_DELETE,0,&value) != 0;
#else
        if(root == 0) root = hKey;
        return RegDeleteValue(root, szName) == ERROR_SUCCESS;
#endif
    }

#ifndef UNICODE
    bool DeleteValue(PCWSTR szName, KEY_TYPE root = 0)
    {
        if(root == 0) root = hKey;
        return RegDeleteValueW(root, szName) == ERROR_SUCCESS;
    }
#endif


   vector<tstring> EnumKeys(KEY_TYPE root = 0)
   {
      vector<tstring> keys;
#ifdef FAR3
      FarSettingsEnum fse = { sizeof fse, root };
      if (SettingsControl(handle, SCTL_ENUM, 0, &fse)) {
         keys.reserve(fse.Count);
         for(size_t i = 0; i < fse.Count; i++)
            if (fse.Items[i].Type == FST_SUBKEY)
               keys.push_back(fse.Items[i].Name);
      }
#else
     if(root == 0) root = hKey;
      DWORD cSubKeys, cMaxSubKeyLen;
      if(RegQueryInfoKey(root, 0, 0, 0, &cSubKeys,&cMaxSubKeyLen,0,0,0,0,0,0)==ERROR_SUCCESS) {
         keys.reserve(cSubKeys);
         vector<TCHAR> keyName(cMaxSubKeyLen + 1);
         for(DWORD iKey=0; iKey<cSubKeys; iKey++) {
            cMaxSubKeyLen = (DWORD)keyName.size();
            if(RegEnumKeyEx(root, iKey, &keyName[0], &cMaxSubKeyLen, 0,0,0,0)!=ERROR_SUCCESS)
               break;
            keys.push_back(&keyName[0]);
         }
      }
#endif
      return keys;
   }

    PTSTR Get(PCTSTR szName, PTSTR buf, DWORD cchSize, PCTSTR Default, KEY_TYPE root = 0)
    {
        if(!Default)
            Default = _T("");
#ifdef FAR3
	FarSettingsItem item = {sizeof item, root,szName,FST_STRING};
        _tcsncpy(buf,
	    SettingsControl(handle,SCTL_GET,0,&item) ? item.String : Default,
            cchSize);
#else
        if(root == 0) root = hKey;
        DWORD dwSize = cchSize*sizeof(TCHAR);
        LONG res = RegQueryValueEx(root, szName, 0, NULL, (BYTE*)buf, &dwSize);
        if(res != ERROR_SUCCESS)
            _tcsncpy(buf, Default, cchSize);
#endif
        return buf;
    }
/*
#ifdef FAR3
    void Get(PCTSTR szName, PTSTR Value, size_t Size, PCTSTR Default, KEY_TYPE root = 0)
    {
	lstrcpyn(Value, Get(szName,Value,Size,Default,root), (int)Size);
    }
#endif*/

    unsigned __int64 Get(PCTSTR szName, unsigned __int64 Default, KEY_TYPE root = 0)
    {
#ifdef FAR3
	FarSettingsItem item={sizeof item, root,szName,FST_QWORD};
	if (SettingsControl(handle,SCTL_GET,0,&item))
	{
	    return item.Number;
	}
	return Default;
#else
        if(root == 0) root = hKey;
        unsigned __int64 val = 0;
        DWORD dwSize = sizeof val;
        return RegQueryValueEx(hKey, szName, 0, NULL, (PBYTE)&val, &dwSize) == ERROR_SUCCESS ?
            val : Default;
#endif
    }

    __int64      Get(PCTSTR szName, __int64 Default, KEY_TYPE root = 0) { return (__int64)Get(szName,(unsigned __int64)Default, root); }
    int          Get(PCTSTR szName, int Default, KEY_TYPE root = 0)  { return (int)Get(szName,(unsigned __int64)Default, root); }
    unsigned int Get(PCTSTR szName, unsigned int Default, KEY_TYPE root = 0) { return (unsigned int)Get(szName,(unsigned __int64)Default, root); }
    DWORD        Get(PCTSTR szName, DWORD Default, KEY_TYPE root = 0) { return (DWORD)Get(szName,(unsigned __int64)Default, root); }
    bool         Get(PCTSTR szName, bool Default, KEY_TYPE root = 0) { return Get(szName,Default?1ull:0ull,root)?true:false; }

    size_t Get(PCTSTR szName, void *buf, size_t size, KEY_TYPE root = 0)
    {
#ifdef FAR3
	FarSettingsItem item={sizeof item, root,szName,FST_DATA};
	if (SettingsControl(handle,SCTL_GET,0,&item))
	{
            if(item.Data.Size < size)
	        size = item.Data.Size;
	    memcpy(buf, item.Data.Data, size);
	    return size;
	}
#else
        if(root == 0) root = hKey;
        DWORD dwSize = (DWORD)size;
        if(size > MAXDWORD)
            dwSize = MAXDWORD;
        if(RegQueryValueEx(root, szName, 0, NULL, (BYTE*)buf, &dwSize) == ERROR_SUCCESS)
            return dwSize;
#endif
	return 0;
    }

#ifndef FAR3
    private:
    bool SetReg(PCTSTR szName, const void* val, DWORD bufsize, DWORD dwType, KEY_TYPE root)
    {
        if(root == 0) root = hKey;
        return RegSetValueEx(root, szName, 0, dwType, (BYTE*)val, bufsize) == ERROR_SUCCESS;
    }
    public:
#endif

    bool Set(PCTSTR szName, PCTSTR Value, KEY_TYPE root = 0)
    {
#ifdef FAR3
	FarSettingsItem item={sizeof item, root,szName,FST_STRING};
	item.String=Value;
	return SettingsControl(handle,SCTL_SET,0,&item)!=FALSE;
#else
        return SetReg(szName, Value, (_tcslen(Value)+1) * sizeof(TCHAR), REG_SZ, root);
#endif
    }

#ifndef UNICODE
    bool Set(PCWSTR szName, PCWSTR Value, KEY_TYPE root = 0)
    {
        if(root == 0) root = hKey;
        return RegSetValueExW(root, szName, 0, REG_SZ, (BYTE*)Value, (wcslen(Value)+1) * sizeof(WCHAR)) == ERROR_SUCCESS;
    }
#endif

    bool Set(PCTSTR szName, unsigned __int64 Value, KEY_TYPE root = 0)
    {
#ifdef FAR3
	FarSettingsItem item={sizeof item, root,szName,FST_QWORD};
	item.Number=Value;
	return SettingsControl(handle,SCTL_SET,0,&item)!=FALSE;
#else
        return SetReg(szName, &Value, sizeof Value, REG_QWORD, root);
#endif
    }

    bool Set(PCTSTR Name, __int64 Value, KEY_TYPE root = 0) { return Set(Name,(unsigned __int64)Value, root); }
#ifdef FAR3
    bool Set(PCTSTR Name, int Value, KEY_TYPE root = 0) { return Set(Name,(unsigned __int64)Value, root); }
    bool Set(PCTSTR Name, unsigned int Value, KEY_TYPE root = 0) { return Set(Name,(unsigned __int64)Value, root); }
    bool Set(PCTSTR Name, DWORD Value, KEY_TYPE root = 0) { return Set(Name,(unsigned __int64)Value, root); }
    bool Set(PCTSTR Name, bool Value, KEY_TYPE root = 0) { return Set(Name,Value?1ull:0ull, root); }
#else
    bool Set(PCTSTR Name, DWORD Value, KEY_TYPE root = 0)
    {
        return SetReg(Name, &Value, sizeof Value, REG_DWORD, root);
    }
    bool Set(PCTSTR Name, int Value, KEY_TYPE root = 0) { return Set(Name,(DWORD)Value, root); }
    bool Set(PCTSTR Name, unsigned int Value, KEY_TYPE root = 0) { return Set(Name,(DWORD)Value, root); }
    bool Set(PCTSTR Name, bool Value, KEY_TYPE root = 0) { return Set(Name,Value?1:0, root); }
#endif

    bool Set(PCTSTR Name, const void *Value, size_t Size, KEY_TYPE root = 0)
    {
#ifdef FAR3
	FarSettingsItem item={sizeof item, root,Name,FST_DATA};
	item.Data.Size=Size;
	item.Data.Data=Value;
	return SettingsControl(handle,SCTL_SET,0,&item)!=FALSE;
#else
        return SetReg(Name, Value, (DWORD)Size, REG_BINARY, root);
#endif
    }
};
