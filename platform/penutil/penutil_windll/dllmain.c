// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
#include "ionic_spp.h"
#include "version.h"
#include <stdio.h>
#include <string.h>

HMODULE g_hInst = NULL;
PENUTIL_VERSION_INFO gVerInfo;

VOID InitPenutilVersionInfo(PPENUTIL_VERSION_INFO pVersionInfo)
{
    pVersionInfo->VerMaj = PENUTIL_MAJOR_VERSION;
    pVersionInfo->VerMin = PENUTIL_MINOR_VERSION;
    pVersionInfo->VerSP = PENUTIL_SP_VERSION;
    pVersionInfo->VerBuild = PENUTIL_BUILD_VERSION;
    snprintf(pVersionInfo->VerString, REVISION_MAX_STR_SIZE, "%s-%s", PENUTIL_VERSION_STRING, PENUTIL_VERSION_EXTENSION);
}

const char* __stdcall ionic_get_dll_ver() {
    return gVerInfo.VerString;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hModule;
        InitPenutilVersionInfo(&gVerInfo);
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_PROCESS_DETACH:
        break;

    default:
        break;

    }
    return TRUE;
}
