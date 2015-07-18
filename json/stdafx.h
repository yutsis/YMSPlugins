// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#pragma pack(2)
#define _WINCON_
#include <windows.h>
#undef _WINCON_
#pragma pack(8)
#include <wincon.h>
#include <stdio.h>
#include <use_ansi.h>
#include <vector>
#include <map>

#include <rapidjson.h>
#include <memorystream.h>
#include <stringbuffer.h>
#include <memorybuffer.h>
#include <document.h>
#include <encodedstream.h>
#include <filereadstream.h>
#include <filewritestream.h>
#include <prettywriter.h>
#include <pointer.h>

using namespace std;
using namespace rapidjson;

