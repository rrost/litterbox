#pragma once

#ifdef _WIN32

#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif // DBG_NEW
#endif // _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

inline bool crtLeakDetectionInit()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    return true;
};

#define ENABLE_LEAK_DETECTION static const bool crtLeakDetectionEnabled = crtLeakDetectionInit();

#else // Not Visual Studio

#define ENABLE_LEAK_DETECTION static const bool crtLeakDetectionEnabled = false;

#endif // _WIN32

