#include <iostream>
#include <fstream>
#include <iomanip>
#include <Windows.h>

#include "EvtLogHelper.h"

#pragma comment(lib, "wevtapi.lib")

#define ARRAY_SIZE 10



// Gets the specified message string from the event. If the event does not
// contain the specified message, the function returns NULL.
LPWSTR GetMessageString(EVT_HANDLE hMetadata, EVT_HANDLE hEvent, EVT_FORMAT_MESSAGE_FLAGS FormatId) {
    LPWSTR pBuffer = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwBufferUsed = 0;
    DWORD status = 0;

    if (!EvtFormatMessage(hMetadata, hEvent, 0, 0, NULL, FormatId, dwBufferSize, pBuffer, &dwBufferUsed)) {
        status = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == status) {
            // For keyords string list, terminate the list with a second null terminator character 
            if ((EvtFormatMessageKeyword == FormatId))
                dwBufferSize = dwBufferUsed + sizeof(WCHAR);
            else
                dwBufferSize = dwBufferUsed;

            pBuffer = (LPWSTR)malloc(dwBufferSize * sizeof(WCHAR));

            if (pBuffer) {
                EvtFormatMessage(hMetadata, hEvent, 0, 0, NULL, FormatId, dwBufferSize, pBuffer, &dwBufferUsed);
                // Add the second null terminator character.
                if ((EvtFormatMessageKeyword == FormatId)) {
                    pBuffer[dwBufferUsed - 1] = L'\0';
                }
            }
            else {
                std::wcerr << L"malloc failed." << std::endl;
            }
        }
        else if ((ERROR_EVT_MESSAGE_NOT_FOUND != status) && (ERROR_EVT_MESSAGE_ID_NOT_FOUND != status)) {
            std::wcerr << L"EvtFormatMessage failed with error: " << status << std::endl;
        }
    }

    return pBuffer;
}

void PrintHeader(std::wostream& outfile) {
    outfile << L"Event No., Time, Level, Type, Provider, EventID, Message, ProcessID, ThreadID, Channel, Computer" << std::endl;
}


DWORD PrintEventData(EVT_HANDLE hEvent, std::wostream& outfile)
{
    DWORD status = ERROR_SUCCESS;
    EVT_HANDLE hContext = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwBufferUsed = 0;
    DWORD dwPropertyCount = 0;
    PEVT_VARIANT pRenderedValues = NULL;

    ULONGLONG ullTimeStamp = 0;
    ULONGLONG ullNanoseconds = 0;
    SYSTEMTIME st;
    FILETIME ft;
    DWORD EventID;
    EVT_HANDLE hProviderMeta = NULL;
    LPWSTR pwsMessage = NULL;

    // Identify the components of the event that you want to render. 
    // In this case, render the system section of the event.
    hContext = EvtCreateRenderContext(0, NULL, EvtRenderContextSystem);
    if (NULL == hContext) {
        status = GetLastError();
        std::wcerr << L"EvtCreateRenderContext failed with error: " << status << std::endl;
        goto cleanup;
    }

    if (!EvtRender(hContext, hEvent, EvtRenderEventValues, dwBufferSize, pRenderedValues, &dwBufferUsed, &dwPropertyCount)) {
        status = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == status) {
            dwBufferSize = dwBufferUsed;
            pRenderedValues = (PEVT_VARIANT)malloc(dwBufferSize);
            if (pRenderedValues) {
                // call EvtRender again with allocated buffer
                if (!EvtRender(hContext, hEvent, EvtRenderEventValues, dwBufferSize, pRenderedValues, &dwBufferUsed, &dwPropertyCount)) {
                    status = GetLastError();
                    std::wcerr << L"EvtRender failed with error: " << status << std::endl;
                    goto cleanup;
                }
            }
            else {
                std::wcerr << L"malloc failed." << std::endl;
                status = ERROR_OUTOFMEMORY;
                goto cleanup;
            }
        }
    }
    // Get the handle to the provider's metadata that contains the message strings.
    hProviderMeta = EvtOpenPublisherMetadata(NULL, pRenderedValues[EvtSystemProviderName].StringVal, NULL, 0, 0);
    if (NULL == hProviderMeta) {
        status = GetLastError();
        std::wcerr << L"EvtOpenPublisherMetadata failed with error: " << status << std::endl;
        goto cleanup;
    }

    // Print the values from the System section of the element.
    // ===== Event No.====
    outfile << pRenderedValues[EvtSystemEventRecordId].UInt64Val;

    // ===== Time ====
    ullTimeStamp = pRenderedValues[EvtSystemTimeCreated].FileTimeVal;
    ft.dwHighDateTime = (DWORD)((ullTimeStamp >> 32) & 0xFFFFFFFF);
    ft.dwLowDateTime = (DWORD)(ullTimeStamp & 0xFFFFFFFF);

    FileTimeToSystemTime(&ft, &st);
    ullNanoseconds = (ullTimeStamp % 10000000) * 100; // Display nanoseconds instead of milliseconds for higher resolution
    outfile << L", ";
    outfile << std::setfill(L'0') << std::setw(4) << st.wYear << L"/";
    outfile << std::setfill(L'0') << std::setw(2) << st.wMonth << L"/";
    outfile << std::setfill(L'0') << std::setw(2) << st.wDay << L" ";
    outfile << std::setfill(L'0') << std::setw(2) << st.wHour << L":";
    outfile << std::setfill(L'0') << std::setw(2) << st.wMinute << L":";
    outfile << std::setfill(L'0') << std::setw(2) << st.wSecond << L".";
    outfile << ullNanoseconds;

    // ==== Level =====
    outfile << L", " << ((EvtVarTypeNull == pRenderedValues[EvtSystemLevel].Type) ? 0 : pRenderedValues[EvtSystemLevel].ByteVal);

    // ==== Type ====
    pwsMessage = GetMessageString(hProviderMeta, hEvent, EvtFormatMessageLevel);
    if (pwsMessage) {
        outfile << L", " << pwsMessage;
        free(pwsMessage);
        pwsMessage = NULL;
    }
    else {
        std::wcerr << L", ";
    }

    // ==== Provider ====
    outfile << L", " << pRenderedValues[EvtSystemProviderName].StringVal;

    // ==== EventID ====
    EventID = pRenderedValues[EvtSystemEventID].UInt16Val;
    if (EvtVarTypeNull != pRenderedValues[EvtSystemQualifiers].Type) {
        EventID = MAKELONG(pRenderedValues[EvtSystemEventID].UInt16Val, pRenderedValues[EvtSystemQualifiers].UInt16Val);
    }
    outfile << L", " << EventID;

    // ==== Message ====
    pwsMessage = GetMessageString(hProviderMeta, hEvent, EvtFormatMessageEvent);
    if (pwsMessage) {
        outfile << L", " << pwsMessage;
        free(pwsMessage);
        pwsMessage = NULL;
    }
    else {
        std::wcerr << L", ";
    }

    //outfile << L"Version: " <<  ((EvtVarTypeNull == pRenderedValues[EvtSystemVersion].Type) ? 0 : pRenderedValues[EvtSystemVersion].ByteVal) << std::endl;
    //outfile << L"Task: " <<  ((EvtVarTypeNull == pRenderedValues[EvtSystemTask].Type) ? 0 : pRenderedValues[EvtSystemTask].UInt16Val) << std::endl;
    //outfile << L"Opcode: " <<  ((EvtVarTypeNull == pRenderedValues[EvtSystemOpcode].Type) ? 0 : pRenderedValues[EvtSystemOpcode].ByteVal) << std::endl;
    //outfile << L"Keywords: 0x" <<  pRenderedValues[EvtSystemKeywords].UInt64Val << std::endl;

    // ==== ProcessID ====
    outfile << L", " << pRenderedValues[EvtSystemProcessID].UInt32Val;
    // ==== ThreadID ====
    outfile << L", " << pRenderedValues[EvtSystemThreadID].UInt32Val;
    // ==== Channel ====
    outfile << L", " << ((EvtVarTypeNull == pRenderedValues[EvtSystemChannel].Type) ? L"" : pRenderedValues[EvtSystemChannel].StringVal);
    // ==== Computer ====
    outfile << L", " << pRenderedValues[EvtSystemComputer].StringVal;
    outfile << std::endl;

    status = ERROR_SUCCESS;

cleanup:

    if (hContext)
        EvtClose(hContext);

    if (pRenderedValues)
        free(pRenderedValues);

    if (hProviderMeta)
        EvtClose(hProviderMeta);

    return status;
}



// Enumerate all the events in the result set. 
DWORD PrintResults(EVT_HANDLE hResults, std::wostream& outfile) {
    DWORD status = ERROR_SUCCESS;
    EVT_HANDLE hEvents[ARRAY_SIZE];
    DWORD dwReturned = 0;

    while (true) {
        // Get a block of events from the result set.
        if (!EvtNext(hResults, ARRAY_SIZE, hEvents, INFINITE, 0, &dwReturned)) {
            if (ERROR_NO_MORE_ITEMS != (status = GetLastError())) {
                wprintf(L"EvtNext failed with %lu\n", status);
            }
            goto cleanup;
        }
        for (DWORD i = 0; i < dwReturned; i++) {
            if (ERROR_SUCCESS == (status = PrintEventData(hEvents[i], outfile))) {
                EvtClose(hEvents[i]);
                hEvents[i] = NULL;
            }
            else {
                goto cleanup;
            }
        }
        std::wcout << L".";
        //break;

    }

cleanup:
    for (DWORD i = 0; i < dwReturned; i++) {
        if (NULL != hEvents[i])
            EvtClose(hEvents[i]);
    }

    return status;
}


HRESULT EvtLogHelperGetEvtLogs(LPCWSTR Query, std::wostream& outfile) {
    HRESULT hRes = S_OK;
    EVT_HANDLE hResults = NULL;
    std::wcout << L"Collecting EventLogs" << std::endl;
    hResults = EvtQuery(NULL, NULL, Query, EvtQueryChannelPath | EvtQueryForwardDirection);
    if (NULL == hResults)
    {
        hRes = E_FAIL;
        std::wcout << L"Collecting EventLogs failed." << std::endl;
        goto cleanup;
    }

    PrintHeader(outfile);
    PrintResults(hResults, outfile);

    std::wcout << std::endl << L"Collecting EventLogs succeeded." << std::endl;

cleanup:

    if (hResults) {
        EvtClose(hResults);
    }

    return hRes;
}

