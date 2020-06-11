#pragma once
#include <Windows.h>

void WMIHelperRelease();
HRESULT WMIHelperInitialize();
HRESULT WMIhelperGetIonicNetAdapters(std::wostream& outfile);
HRESULT WMIhelperGetLbfoInfo(std::wostream& outfile);

