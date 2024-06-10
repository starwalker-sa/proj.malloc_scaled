
#include "build.h"
#include "lib_malloc.h"

#if PLATFORM_WIN
#include "win.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) 
{
    BOOL Ok = TRUE;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
#if !defined(MALLOC_DEBUG) && !defined(MALLOC_STATS) && !defined(MALLOC_TIME_STATS)
        Ok = InitMalloc();
#endif
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:      
        break;
    case DLL_PROCESS_DETACH:
#if !defined(MALLOC_DEBUG) && !defined(MALLOC_STATS) && !defined(MALLOC_TIME_STATS)
        ShutdownMalloc(); 
#endif
        break;
    }
    return Ok;  
}

#endif