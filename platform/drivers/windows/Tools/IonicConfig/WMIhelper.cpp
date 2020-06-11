#define _WIN32_DCOM
#include <iostream>
#include <fstream>
#include <iomanip>

#include <comdef.h>
#include <Wbemidl.h>
#include "WMIHelper.h"

#pragma comment(lib, "wbemuuid.lib")

IWbemLocator* g_pLocatorStdCIMv2 = NULL;
IWbemServices* g_pServicesStdCIMv2 = NULL;

IWbemLocator* g_pLocatorCIMv2 = NULL;
IWbemServices* g_pServicesCIMv2 = NULL;

HRESULT WMIHelperInitialize() {
    HRESULT hres;

    // Initialize COM.
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        std::cerr << "Failed to initialize COM library. Error code = 0x" << std::hex << hres << std::endl;
        return hres; 
    }

    // Initialize 
    hres = CoInitializeSecurity( NULL,
                                 -1,                           // COM negotiates service                  
                                 NULL,                         // Authentication services
                                 NULL,                         // Reserved
                                 RPC_C_AUTHN_LEVEL_DEFAULT,    // authentication
                                 RPC_C_IMP_LEVEL_IMPERSONATE,  // Impersonation
                                 NULL,                         // Authentication info 
                                 EOAC_NONE,                    // Additional capabilities
                                 NULL                          // Reserved
                                );
    if (FAILED(hres)) {
        std::cerr << "Failed to initialize security. Error code = 0x" << std::hex << hres << std::endl;
        goto ERROR_EXIT;
    }

    // Obtain the initial locator to Windows Management
    hres = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&g_pLocatorStdCIMv2);
    if (FAILED(hres)) {
        std::cerr << "Failed to create IWbemLocator object. Error code = 0x" << std::hex << hres << std::endl;
        goto ERROR_EXIT;
    }
    // Connect to the root\cimv2 namespace with the current user and obtain pointer g_pServices
    hres = g_pLocatorStdCIMv2->ConnectServer(_bstr_t(L"ROOT\\STANDARDCIMV2"),  // WMI namespace
                                              NULL,                            // User name
                                              NULL,                            // User password
                                              0,                               // Locale
                                              NULL,                            // Security flags                 
                                              0,                               // Authority       
                                              0,                               // Context object
                                              &g_pServicesStdCIMv2             // IWbemServices proxy
                                            );
    if (FAILED(hres)) {
        std::cerr << "Could not connect. Error code = 0x" << std::hex << hres << std::endl;
        goto ERROR_EXIT;
    }

    // Set the IWbemServices proxy so that impersonation of the user (client) occurs.
    hres = CoSetProxyBlanket( g_pServicesStdCIMv2,          // the proxy to set
                              RPC_C_AUTHN_WINNT,            // authentication service
                              RPC_C_AUTHZ_NONE,             // authorization service
                              NULL,                         // Server principal name
                              RPC_C_AUTHN_LEVEL_CALL,       // authentication level
                              RPC_C_IMP_LEVEL_IMPERSONATE,  // impersonation level
                              NULL,                         // client identity 
                              EOAC_NONE                     // proxy capabilities     
                            );
    if (FAILED(hres)) {
        std::cerr << "Could not set proxy blanket. Error code = 0x" << std::hex << hres << std::endl;
        goto ERROR_EXIT;
    }

    // Obtain the initial locator to Windows Management
    hres = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&g_pLocatorCIMv2);
    if (FAILED(hres)) {
        std::cerr << "Failed to create IWbemLocator object. Error code = 0x" << std::hex << hres << std::endl;
        goto ERROR_EXIT;
    }
    // Connect to the root\cimv2 namespace with the current user and obtain pointer g_pServices
    hres = g_pLocatorCIMv2->ConnectServer(_bstr_t(L"ROOT\\CIMV2"),     // WMI namespace
                                              NULL,                    // User name
                                              NULL,                    // User password
                                              0,                       // Locale
                                              NULL,                    // Security flags                 
                                              0,                       // Authority       
                                              0,                       // Context object
                                              &g_pServicesCIMv2        // IWbemServices proxy
                                            );
    if (FAILED(hres)) {
        std::cerr << "Could not connect. Error code = 0x" << std::hex << hres << std::endl;
        goto ERROR_EXIT;
    }

    // Set the IWbemServices proxy so that impersonation of the user (client) occurs.
    hres = CoSetProxyBlanket( g_pServicesCIMv2,          // the proxy to set
                              RPC_C_AUTHN_WINNT,            // authentication service
                              RPC_C_AUTHZ_NONE,             // authorization service
                              NULL,                         // Server principal name
                              RPC_C_AUTHN_LEVEL_CALL,       // authentication level
                              RPC_C_IMP_LEVEL_IMPERSONATE,  // impersonation level
                              NULL,                         // client identity 
                              EOAC_NONE                     // proxy capabilities     
                            );
    if (FAILED(hres)) {
        std::cerr << "Could not set proxy blanket. Error code = 0x" << std::hex << hres << std::endl;
        goto ERROR_EXIT;
    }

    return hres;

ERROR_EXIT:
    
    WMIHelperRelease();

    return hres;
}

void WMIHelperRelease() {
    // Cleanup
    // ========
    if (g_pServicesStdCIMv2) {
        g_pServicesStdCIMv2->Release();
        g_pServicesStdCIMv2 = NULL;
    }
    if (g_pServicesCIMv2) {
        g_pServicesCIMv2->Release();
        g_pServicesCIMv2 = NULL;
    }
    if (g_pLocatorStdCIMv2) {
        g_pLocatorStdCIMv2->Release();
        g_pLocatorStdCIMv2 = NULL;
    }
    if (g_pLocatorCIMv2) {
        g_pLocatorCIMv2->Release();
        g_pLocatorCIMv2 = NULL;
    }

    CoUninitialize();

}

void PrintVariant(VARIANT& Variant, std::wostream& outfile) {
    if ((Variant.vt == VT_EMPTY) || (Variant.vt == VT_NULL)) {
        return;
    }
    switch (Variant.vt) {
    case VT_I1: outfile << Variant.cVal; break;
    case VT_I2: outfile << Variant.iVal; break;
    case VT_I4: outfile << Variant.lVal; break;
    case VT_I8: outfile << Variant.llVal; break;
    case VT_UI1: outfile << Variant.bVal; break;
    case VT_UI2: outfile << Variant.uiVal; break;
    case VT_UI4: outfile << Variant.ulVal; break;
    case VT_UI8: outfile << Variant.ullVal; break;
    case VT_INT: outfile << Variant.intVal; break;
    case VT_UINT: outfile << Variant.uintVal; break;
    case VT_BOOL: outfile << ((Variant.boolVal) ? L"TRUE" : L"FALSE"); break;
    case VT_BSTR: outfile << Variant.bstrVal; break;
    case (VT_BSTR | VT_ARRAY): 
        {
            SAFEARRAY* pSafeArray = V_ARRAY(&Variant);
            LONG lowerBound, upperBound;
            bstr_t OutputStr = L"";
            SafeArrayGetLBound(pSafeArray, 1, &lowerBound);
            SafeArrayGetUBound(pSafeArray, 1, &upperBound);
            LONG cnt_elements = upperBound - lowerBound + 1;
            if (cnt_elements) {
                OutputStr += bstr_t(L"{");
                for (LONG i = 0; i < cnt_elements; i++) {
                    BSTR element;
                    element = bstr_t(L"");
                    SafeArrayGetElement(pSafeArray, &i, (void*)&element);
                    OutputStr += ((0 == i) ? bstr_t(L"") : bstr_t(L", ")) + bstr_t(element);
                }
                OutputStr += bstr_t(L"}");
            }
            outfile << OutputStr;
            break;
        }

    default:
        outfile << L"NI"; break;
    }
}

HRESULT WMIhelperGetLbfoInfo(std::wostream& outfile) {
    HRESULT hres;
    IEnumWbemClassObject* pLbfoEnum = NULL;
    IEnumWbemClassObject* pLbfoTeamMemberEnum = NULL;
    IEnumWbemClassObject* pLbfoTeamNicEnum = NULL;

    if ((NULL == g_pServicesStdCIMv2) || (NULL == g_pServicesCIMv2)) {
        return CO_E_NOTINITIALIZED;
    }

    std::wcout << L"Collecting LBFO info" << std::endl;

    bstr_t NetLBFOQuery("SELECT * FROM MSFT_NetLbfoTeam");
    bstr_t NetLBFOTeamMemberQueryPre("SELECT * FROM MSFT_NetLbfoTeamMember WHERE Team = '");
    bstr_t NetLBFOTeamNicQueryPre("SELECT * FROM MSFT_NetLbfoTeamNic WHERE Team = '");

    hres = g_pServicesStdCIMv2->ExecQuery(bstr_t("WQL"), NetLBFOQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pLbfoEnum);
    if (FAILED(hres)) {
        std::cerr << "Query for LBFO failed. Error code = 0x" << std::hex << hres << std::endl;
        return hres;
    }
    else {
        IWbemClassObject* pclsObj;
        ULONG uReturn = 0;
        bstr_t NetLBFOTeamMemberQuery;
        bstr_t NetLBFOTeamNicQuery;

        while (pLbfoEnum) {
            hres = pLbfoEnum->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn) {
                break;
            }

            VARIANT vtProp;

            outfile << std::endl << L"LBFO team NIC found: " << std::endl;
            outfile << L"====================" << std::endl;
            // Get the value of the Name property
            hres = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
            outfile << L"Name: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            NetLBFOTeamMemberQuery = NetLBFOTeamMemberQueryPre + vtProp.bstrVal + bstr_t("'");
            NetLBFOTeamNicQuery = NetLBFOTeamNicQueryPre + vtProp.bstrVal + bstr_t("'");
            VariantClear(&vtProp);
            // Get the value of the InstanceID property
            hres = pclsObj->Get(L"InstanceID", 0, &vtProp, 0, 0);
            outfile << L"InstanceID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the TeamingMode property
            hres = pclsObj->Get(L"TeamingMode", 0, &vtProp, 0, 0);
            outfile << L"TeamingMode: "; PrintVariant(vtProp, outfile); 
            if (VT_I4 == vtProp.vt) {
                switch (vtProp.lVal) {
                case 0: outfile << L" (Static)"; break;
                case 1: outfile << L" (SwitchIndependent)"; break;
                case 2: outfile << L" (Lacp)"; break;
                }
            }
            outfile << std::endl;
            VariantClear(&vtProp);
            // Get the value of the LoadBalancingAlgorithm property
            hres = pclsObj->Get(L"LoadBalancingAlgorithm", 0, &vtProp, 0, 0);
            outfile << L"LoadBalancingAlgorithm: "; PrintVariant(vtProp, outfile); 
            if (VT_I4 == vtProp.vt) {
                switch (vtProp.lVal) {
                case 0: outfile << L" (TransportPorts)"; break;
                case 2: outfile << L" (IPAddresses)"; break;
                case 3: outfile << L" (MacAddresses)"; break;
                case 4: outfile << L" (HyperVPort)"; break;
                case 5: outfile << L" (Dynamic)"; break;
                }
            }
            outfile << std::endl;
            VariantClear(&vtProp);
            // Get the value of the Status property
            hres = pclsObj->Get(L"Status", 0, &vtProp, 0, 0);
            outfile << L"Status: "; PrintVariant(vtProp, outfile);
            if (VT_I4 == vtProp.vt) {
                switch (vtProp.lVal) {
                case 0: outfile << L" (Up)"; break;
                case 1: outfile << L" (Down)"; break;
                case 2: outfile << L" (Degraded)"; break;
                }
            }
            outfile << std::endl;
            VariantClear(&vtProp);

            pclsObj->Release();
            pclsObj = NULL;

            std::wcout << L".";

            hres = g_pServicesStdCIMv2->ExecQuery(bstr_t("WQL"), NetLBFOTeamMemberQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pLbfoTeamMemberEnum);
            if (FAILED(hres)) {
                std::cerr << "Query for LBFO Team Member failed. Error code = 0x" << std::hex << hres << std::endl;
                // don't break or return here, continue with more info
            }
            else {
                outfile << L"Members: ";
                while (pLbfoTeamMemberEnum) {
                    hres = pLbfoTeamMemberEnum->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                    if (0 == uReturn) {
                        break;
                    }
                    // Get the value of the Name property
                    hres = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
                    outfile << std::endl << L"\tName: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the InterfaceDescription property
                    hres = pclsObj->Get(L"InterfaceDescription", 0, &vtProp, 0, 0);
                    outfile << L"\tInterfaceDescription: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the InstanceID property
                    hres = pclsObj->Get(L"InstanceID", 0, &vtProp, 0, 0);
                    outfile << L"\tInstanceID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the Team property
                    hres = pclsObj->Get(L"Team", 0, &vtProp, 0, 0);
                    outfile << L"\tTeam: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the TransmitLinkSpeed property
                    hres = pclsObj->Get(L"TransmitLinkSpeed", 0, &vtProp, 0, 0);
                    outfile << L"\tTransmitLinkSpeed: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the ReceiveLinkSpeed property
                    hres = pclsObj->Get(L"ReceiveLinkSpeed", 0, &vtProp, 0, 0);
                    outfile << L"\tReceiveLinkSpeed: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the FailureReason property
                    hres = pclsObj->Get(L"FailureReason", 0, &vtProp, 0, 0);
                    outfile << L"\tFailureReason: "; PrintVariant(vtProp, outfile); 
                    if (VT_I4 == vtProp.vt) {
                        switch (vtProp.lVal) {
                        case 0: outfile << L" (NoFailure)"; break;
                        case 1: outfile << L" (AdministrativeDecision)"; break;
                        case 3: outfile << L" (PhysicalMediaDisconnected)"; break;
                        case 4: outfile << L" (WaitingForStableConnectivity)"; break;
                        case 5: outfile << L" (LacpNegotiationIssue)"; break;
                        case 6: outfile << L" (NicNotPresent)"; break;
                        case 7: outfile << L" (UnknownFailure)"; break;
                        case 9: outfile << L" (VirtualSwitchLacksExternalConnectivity)"; break;
                        }
                    }
                    outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the AdministrativeMode property
                    hres = pclsObj->Get(L"AdministrativeMode", 0, &vtProp, 0, 0);
                    outfile << L"\tAdministrativeMode: "; PrintVariant(vtProp, outfile); 
                    if (VT_I4 == vtProp.vt) {
                        switch (vtProp.lVal) {
                        case 0: outfile << L" (Active)"; break;
                        case 1: outfile << L" (Standby)"; break;
                        }
                    }
                    outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the OperationalMode property
                    hres = pclsObj->Get(L"OperationalMode", 0, &vtProp, 0, 0);
                    outfile << L"\tOperationalMode: "; PrintVariant(vtProp, outfile); 
                    if (VT_I4 == vtProp.vt) {
                        switch (vtProp.lVal) {
                        case 0: outfile << L" (Active)"; break;
                        case 1: outfile << L" (Standby)"; break;
                        case 4096: outfile << L" (Failed)"; break;
                        }
                    }
                    outfile << std::endl;
                    VariantClear(&vtProp);

                    pclsObj->Release();
                    pclsObj = NULL;

                }

                pLbfoTeamMemberEnum->Release();
            }

            std::wcout << L".";


            // Get the value of the TeamNics property

            hres = g_pServicesStdCIMv2->ExecQuery(bstr_t("WQL"), NetLBFOTeamNicQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pLbfoTeamNicEnum);
            if (FAILED(hres)) {
                std::cerr << "Query for LBFO Team Member failed. Error code = 0x" << std::hex << hres << std::endl;
                // don't break or return here, continue with more info
            }
            else {
                outfile << L"Team Interfaces: ";
                while (pLbfoTeamNicEnum) {
                    hres = pLbfoTeamNicEnum->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                    if (0 == uReturn) {
                        break;
                    }
                    // Get the value of the Name property
                    hres = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
                    outfile << std::endl << L"\tName: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the InterfaceDescription property
                    hres = pclsObj->Get(L"InterfaceDescription", 0, &vtProp, 0, 0);
                    outfile << L"\tInterfaceDescription: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the InstanceID property
                    hres = pclsObj->Get(L"InstanceID", 0, &vtProp, 0, 0);
                    outfile << L"\tInstanceID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the Team property
                    hres = pclsObj->Get(L"Team", 0, &vtProp, 0, 0);
                    outfile << L"\tTeam: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the VlanID property
                    hres = pclsObj->Get(L"VlanID", 0, &vtProp, 0, 0);
                    outfile << L"\tVlanID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the Primary property
                    hres = pclsObj->Get(L"Primary", 0, &vtProp, 0, 0);
                    outfile << L"\tPrimary: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the Default property
                    hres = pclsObj->Get(L"Default", 0, &vtProp, 0, 0);
                    outfile << L"\tDefault: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the TransmitLinkSpeed property
                    hres = pclsObj->Get(L"TransmitLinkSpeed", 0, &vtProp, 0, 0);
                    outfile << L"\tTransmitLinkSpeed: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the ReceiveLinkSpeed property
                    hres = pclsObj->Get(L"ReceiveLinkSpeed", 0, &vtProp, 0, 0);
                    outfile << L"\tReceiveLinkSpeed: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);

                    pclsObj->Release();
                    pclsObj = NULL;

                }

                pLbfoTeamNicEnum->Release();
            }

            std::wcout << L".";
            outfile << L"====================================================================" << std::endl << std::endl;

        }

        pLbfoEnum->Release();
    }

    std::wcout << std::endl << L"Finished collecting LBFO info" << std::endl;

    return hres;
}


HRESULT WMIhelperGetIonicNetAdapters(std::wostream& outfile) {
    HRESULT hres;
    IEnumWbemClassObject* pNetAdapterEnum = NULL;
    IEnumWbemClassObject* pNetAdapterAdvPropEnum = NULL;
    IEnumWbemClassObject* pNetAdapterHwInfoEnum = NULL;
    IEnumWbemClassObject* pNetAdapterRssEnum = NULL;
    IEnumWbemClassObject* pNetAdapterConfigEnum = NULL;
    
    if ((NULL == g_pServicesStdCIMv2)||(NULL == g_pServicesCIMv2)) {
        return CO_E_NOTINITIALIZED;
    }

    std::wcout << L"Collecting Ionic NetAdapter info" << std::endl;

    bstr_t NetAdapterQuery("SELECT * FROM MSFT_NetAdapter WHERE PnPDeviceID LIKE '%VEN_1DD8%'");
    //bstr_t NetAdapterQuery("SELECT * FROM MSFT_NetAdapter WHERE PnPDeviceID LIKE 'PCI%'");
    bstr_t NetAdapterAdvPropQueryPre("SELECT * FROM MSFT_NetAdapterAdvancedPropertySettingData WHERE Name = '");
    bstr_t NetAdapterHwInfoQueryPre("SELECT * FROM MSFT_NetAdapterHardwareInfoSettingData WHERE Name = '");
    bstr_t NetAdapterRssQueryPre("SELECT * FROM MSFT_NetAdapterRssSettingData WHERE Name = '");
    bstr_t NetAdapterConfigQueryPre("SELECT * FROM Win32_NetworkAdapterConfiguration WHERE SettingID = '");

    hres = g_pServicesStdCIMv2->ExecQuery(bstr_t("WQL"), NetAdapterQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pNetAdapterEnum);
    if (FAILED(hres)) {
        std::cerr << "Query for NetAdapter failed. Error code = 0x" << std::hex << hres << std::endl;
        return hres;
    } else {
        IWbemClassObject* pclsObj;
        ULONG uReturn = 0;
        bstr_t NetAdapterAdvPropQuery;
        bstr_t NetAdapterHwInfoQuery;
        bstr_t NetAdapterRssQuery;
        bstr_t NetAdapterConfigQuery;
    
        while (pNetAdapterEnum) {
            hres = pNetAdapterEnum->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn) {
                break;
            }

            VARIANT vtProp;
            VARIANT vtVDV;
            VARIANT vtVRV;

            outfile << std::endl << L"Ionic Network Adapter found: " << std::endl;
            outfile << L"============================ " << std::endl;
            // Get the value of the Name property
            hres = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
            outfile << L"Name: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            NetAdapterAdvPropQuery = NetAdapterAdvPropQueryPre + vtProp.bstrVal + bstr_t("'");
            NetAdapterHwInfoQuery = NetAdapterHwInfoQueryPre + vtProp.bstrVal + bstr_t("'");
            NetAdapterRssQuery = NetAdapterRssQueryPre + vtProp.bstrVal + bstr_t("'");
            //outfile << NetAdapterAdvPropQuery << std::endl;
            VariantClear(&vtProp);

            // Get the value of the InterfaceDescription property
            hres = pclsObj->Get(L"InterfaceDescription", 0, &vtProp, 0, 0);
            outfile << L"Device Name: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the PnPDeviceID property
            hres = pclsObj->Get(L"PnPDeviceID", 0, &vtProp, 0, 0);
            outfile << L"PnPDeviceID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the ComponentID property
            hres = pclsObj->Get(L"ComponentID", 0, &vtProp, 0, 0);
            outfile << L"ComponentID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the DeviceID property
            hres = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
            outfile << L"DeviceID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            NetAdapterConfigQuery = NetAdapterConfigQueryPre + vtProp.bstrVal + bstr_t("'");
            VariantClear(&vtProp);

            // Get the value of the DeviceName property
            hres = pclsObj->Get(L"DeviceName", 0, &vtProp, 0, 0);
            outfile << L"DeviceName: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the MtuSize property
            hres = pclsObj->Get(L"MtuSize", 0, &vtProp, 0, 0);
            outfile << L"MtuSize: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the ActiveMaximumTransmissionUnit property
            hres = pclsObj->Get(L"ActiveMaximumTransmissionUnit", 0, &vtProp, 0, 0);
            outfile << L"Active MTU: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the InterfaceName property
            hres = pclsObj->Get(L"InterfaceName", 0, &vtProp, 0, 0);
            outfile << L"InterfaceName: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the InterfaceGuid property
            hres = pclsObj->Get(L"InterfaceGuid", 0, &vtProp, 0, 0);
            outfile << L"InterfaceGuid: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the InterfaceIndex property
            hres = pclsObj->Get(L"InterfaceIndex", 0, &vtProp, 0, 0);
            outfile << L"InterfaceIndex: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the NetLuid property
            hres = pclsObj->Get(L"NetLuid", 0, &vtProp, 0, 0);
            outfile << L"NetLuid: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the NetLuidIndex property
            hres = pclsObj->Get(L"NetLuidIndex", 0, &vtProp, 0, 0);
            outfile << L"NetLuidIndex: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the InterfaceType property
            hres = pclsObj->Get(L"InterfaceType", 0, &vtProp, 0, 0);
            outfile << L"InterfaceType: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the Speed property
            hres = pclsObj->Get(L"Speed", 0, &vtProp, 0, 0);
            outfile << L"Speed: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the TransmitLinkSpeed property
            hres = pclsObj->Get(L"TransmitLinkSpeed", 0, &vtProp, 0, 0);
            outfile << L"TransmitLinkSpeed: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the ReceiveLinkSpeed property
            hres = pclsObj->Get(L"ReceiveLinkSpeed", 0, &vtProp, 0, 0);
            outfile << L"ReceiveLinkSpeed: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the MediaConnectState property
            hres = pclsObj->Get(L"MediaConnectState", 0, &vtProp, 0, 0);
            outfile << L"MediaConnectState: " << ((0 == vtProp.lVal)?L"Unknown":((1 == vtProp.lVal)?L"Connected":((2 == vtProp.lVal)?L"Disconnected":L""  )  )) << std::endl;
            VariantClear(&vtProp);

            // Get the value of the MediaDuplexState property
            hres = pclsObj->Get(L"MediaDuplexState", 0, &vtProp, 0, 0);
            outfile << L"MediaDuplexState: " << ((0 == vtProp.lVal)?L"Unknown":((1 == vtProp.lVal)?L"Half":((2 == vtProp.lVal)?L"Full":L""  )  )) << std::endl;
            VariantClear(&vtProp);

            // Get the value of the InterfaceAdminStatus property
            hres = pclsObj->Get(L"InterfaceAdminStatus", 0, &vtProp, 0, 0);
            outfile << L"InterfaceAdminStatus: " << ((1 == vtProp.lVal)?L"Up":((2 == vtProp.lVal)?L"Down":((3 == vtProp.lVal)?L"Testing":L""  )  )) << std::endl;
            VariantClear(&vtProp);

            // Get the value of the State property
            hres = pclsObj->Get(L"State", 0, &vtProp, 0, 0);
            outfile << L"Interface PnP State: " << ((0 == vtProp.lVal) ? L"Unknown" : ((1 == vtProp.lVal) ? L"Present" : ((2 == vtProp.lVal) ? L"Started" : ((3 == vtProp.lVal) ? L"Disabled" : L"")))) << std::endl;
            VariantClear(&vtProp);

            // Get the value of the PromiscuousMode property <- this could be confused with our advanced registry property
            //hres = pclsObj->Get(L"PromiscuousMode", 0, &vtProp, 0, 0);
            //outfile << L"PromiscuousMode: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            //VariantClear(&vtProp);

            // Get the value of the VlanID property
            hres = pclsObj->Get(L"VlanID", 0, &vtProp, 0, 0);
            outfile << L"VlanID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the PermanentAddress property
            hres = pclsObj->Get(L"PermanentAddress", 0, &vtProp, 0, 0);
            outfile << L"PermanentAddress: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the NetworkAddresses property
            hres = pclsObj->Get(L"NetworkAddresses", 0, &vtProp, 0, 0);
            outfile << L"NetworkAddresses: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the LastErrorCode property
            hres = pclsObj->Get(L"LastErrorCode", 0, &vtProp, 0, 0);
            outfile << L"LastErrorCode: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the ErrorDescription property
            hres = pclsObj->Get(L"ErrorDescription", 0, &vtProp, 0, 0);
            outfile << L"ErrorDescription: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the ErrorCleared property
            hres = pclsObj->Get(L"ErrorCleared", 0, &vtProp, 0, 0);
            outfile << L"ErrorCleared: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the ConfigManagerErrorCode property
            hres = pclsObj->Get(L"ConfigManagerErrorCode", 0, &vtProp, 0, 0);
            outfile << L"ConfigManagerErrorCode: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            // Get the value of the SystemName property
            hres = pclsObj->Get(L"SystemName", 0, &vtProp, 0, 0);
            outfile << L"SystemName: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            std::wcout << L".";

            outfile << L"DriverInfo: " << std::endl;
            // Get the value of the DriverDescription property
            hres = pclsObj->Get(L"DriverDescription", 0, &vtProp, 0, 0);
            outfile << L"\tDriverDescription: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);
            // Get the value of the DriverName property
            hres = pclsObj->Get(L"DriverName", 0, &vtProp, 0, 0);
            outfile << L"\tDriverName: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);
            // Get the value of the DriverProvider property
            hres = pclsObj->Get(L"DriverProvider", 0, &vtProp, 0, 0);
            outfile << L"\tDriverProvider: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);
            // Get the value of the DriverDate property
            hres = pclsObj->Get(L"DriverDate", 0, &vtProp, 0, 0);
            outfile << L"\tDriverDate: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);
            // Get the value of the DriverVersionString property
            hres = pclsObj->Get(L"DriverVersionString", 0, &vtProp, 0, 0);
            outfile << L"\tDriverVersionString: "; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);
            // Get the value of the Driver NDIS version property
            hres = pclsObj->Get(L"DriverMajorNdisVersion", 0, &vtProp, 0, 0);
            outfile << L"\tDriverNdisVersion: " << vtProp.bVal;
            VariantClear(&vtProp);
            hres = pclsObj->Get(L"DriverMinorNdisVersion", 0, &vtProp, 0, 0);
            outfile << L"."; PrintVariant(vtProp, outfile); outfile << std::endl;
            VariantClear(&vtProp);

            pclsObj->Release();
            pclsObj = NULL;

            std::wcout << L".";

            hres = g_pServicesStdCIMv2->ExecQuery(bstr_t("WQL"), NetAdapterHwInfoQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pNetAdapterHwInfoEnum);
            if (FAILED(hres)) {
                std::cerr << "Query for NetAdapterHardwareInfo failed. Error code = 0x" << std::hex << hres << std::endl;
                // don't break or return here, continue with more info
            }
            else {
                outfile << L"Hardware Info:" << std::endl;
                while (pNetAdapterHwInfoEnum) {
                    hres = pNetAdapterHwInfoEnum->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                    if (0 == uReturn) {
                        break;
                    }
                    // Get the value of the BusNumber property
                    hres = pclsObj->Get(L"BusNumber", 0, &vtProp, 0, 0);
                    outfile << L"\tBusNumber: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DeviceNumber property
                    hres = pclsObj->Get(L"DeviceNumber", 0, &vtProp, 0, 0);
                    outfile << L"\tDeviceNumber: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the FunctionNumber property
                    hres = pclsObj->Get(L"FunctionNumber", 0, &vtProp, 0, 0);
                    outfile << L"\tFunctionNumber: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the SegmentNumber property
                    hres = pclsObj->Get(L"SegmentNumber", 0, &vtProp, 0, 0);
                    outfile << L"\tSegmentNumber: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the SlotNumber property
                    hres = pclsObj->Get(L"SlotNumber", 0, &vtProp, 0, 0);
                    outfile << L"\tSlotNumber: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the NumaNode property
                    hres = pclsObj->Get(L"NumaNode", 0, &vtProp, 0, 0);
                    outfile << L"\tNumaNode: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the NumMsiMessages property
                    hres = pclsObj->Get(L"NumMsiMessages", 0, &vtProp, 0, 0);
                    outfile << L"\tNumMsiMessages: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the NumMsixTableEntries property
                    hres = pclsObj->Get(L"NumMsixTableEntries", 0, &vtProp, 0, 0);
                    outfile << L"\tNumMsixTableEntries: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciCurrentSpeedAndMode property
                    hres = pclsObj->Get(L"PciCurrentSpeedAndMode", 0, &vtProp, 0, 0);
                    outfile << L"\tPciCurrentSpeedAndMode: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciDeviceLabelID property
                    hres = pclsObj->Get(L"PciDeviceLabelID", 0, &vtProp, 0, 0);
                    outfile << L"\tPciDeviceLabelID: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciDeviceLabelString property
                    hres = pclsObj->Get(L"PciDeviceLabelString", 0, &vtProp, 0, 0);
                    outfile << L"\tPciDeviceLabelString: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciDeviceType property
                    hres = pclsObj->Get(L"PciDeviceType", 0, &vtProp, 0, 0);
                    outfile << L"\tPciDeviceType: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciExpressCurrentLinkSpeedEncoded property
                    hres = pclsObj->Get(L"PciExpressCurrentLinkSpeedEncoded", 0, &vtProp, 0, 0);
                    outfile << L"\tPciExpressCurrentLinkSpeedEncoded: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciExpressCurrentLinkWidth property
                    hres = pclsObj->Get(L"PciExpressCurrentLinkWidth", 0, &vtProp, 0, 0);
                    outfile << L"\tPciExpressCurrentLinkWidth: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciExpressCurrentPayloadSize property
                    hres = pclsObj->Get(L"PciExpressCurrentPayloadSize", 0, &vtProp, 0, 0);
                    outfile << L"\tPciExpressCurrentPayloadSize: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciExpressMaxLinkSpeedEncoded property
                    hres = pclsObj->Get(L"PciExpressMaxLinkSpeedEncoded", 0, &vtProp, 0, 0);
                    outfile << L"\tPciExpressMaxLinkSpeedEncoded: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciExpressMaxLinkWidth property
                    hres = pclsObj->Get(L"PciExpressMaxLinkWidth", 0, &vtProp, 0, 0);
                    outfile << L"\tPciExpressMaxLinkWidth: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciExpressMaxPayloadSize property
                    hres = pclsObj->Get(L"PciExpressMaxPayloadSize", 0, &vtProp, 0, 0);
                    outfile << L"\tPciExpressMaxPayloadSize: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciExpressMaxReadRequestSize property
                    hres = pclsObj->Get(L"PciExpressMaxReadRequestSize", 0, &vtProp, 0, 0);
                    outfile << L"\tPciExpressMaxReadRequestSize: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciExpressVersion property
                    hres = pclsObj->Get(L"PciExpressVersion", 0, &vtProp, 0, 0);
                    outfile << L"\tPciExpressVersion: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the PciXCurrentSpeedAndMode property
                    hres = pclsObj->Get(L"PciXCurrentSpeedAndMode", 0, &vtProp, 0, 0);
                    outfile << L"\tPciXCurrentSpeedAndMode: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);

                    pclsObj->Release();
                    pclsObj = NULL;

                }

                pNetAdapterHwInfoEnum->Release();
            }

            std::wcout << L".";

            hres = g_pServicesStdCIMv2->ExecQuery(bstr_t("WQL"), NetAdapterAdvPropQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pNetAdapterAdvPropEnum);
            if (FAILED(hres)) {
                std::cerr << "Query for NetAdapterAdvProperty failed. Error code = 0x" << std::hex << hres << std::endl;
                // don't break or return here, continue with the next adapter
                continue;
            }
            else {
                outfile << L"Advanced Properties:" << std::endl;
                while (pNetAdapterAdvPropEnum) {
                    hres = pNetAdapterAdvPropEnum->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                    if (0 == uReturn) {
                        break;
                    }
                    // Get the value of the RegistryKeyword property
                    hres = pclsObj->Get(L"RegistryKeyword", 0, &vtProp, 0, 0);
                    outfile << L"\t" << vtProp.bstrVal << L" = ";
                    VariantClear(&vtProp);
                    // Get the value of the RegistryValue property
                    hres = pclsObj->Get(L"RegistryValue", 0, &vtProp, 0, 0);
                    PrintVariant(vtProp, outfile);
                    VariantClear(&vtProp);

                    // Get the value of the ValidDisplayValues property
                    hres = pclsObj->Get(L"ValidDisplayValues", 0, &vtVDV, 0, 0);
                    hres = pclsObj->Get(L"ValidRegistryValues", 0, &vtVRV, 0, 0);
                    bstr_t DispValues = L" ";
                    if ((vtVDV.vt == (VT_BSTR | VT_ARRAY))&&(vtVRV.vt == (VT_BSTR | VT_ARRAY))) {
                        SAFEARRAY* pSafeArrayD = V_ARRAY(&vtVDV);
                        SAFEARRAY* pSafeArrayR = V_ARRAY(&vtVRV);
                        LONG lowerBoundD, upperBoundD;
                        LONG lowerBoundR, upperBoundR;
                        SafeArrayGetLBound(pSafeArrayD, 1, &lowerBoundD);
                        SafeArrayGetUBound(pSafeArrayD, 1, &upperBoundD);
                        SafeArrayGetLBound(pSafeArrayR, 1, &lowerBoundR);
                        SafeArrayGetUBound(pSafeArrayR, 1, &upperBoundR);
                        if ((upperBoundD - lowerBoundD == upperBoundR - lowerBoundR)&&(0 != (upperBoundD - lowerBoundD))) {
                            DispValues += bstr_t(L"Valid Values:{");
                            LONG cnt_elements = upperBoundD - lowerBoundD + 1;
                            for (LONG i = 0; i < cnt_elements; i++) {
                                BSTR elementD, elementR;
                                elementD = bstr_t(L"");
                                elementR = bstr_t(L"");
                                SafeArrayGetElement(pSafeArrayD, &i, (void*)&elementD);
                                SafeArrayGetElement(pSafeArrayR, &i, (void*)&elementR);
                                DispValues += ((0 == i) ? bstr_t(L"") : bstr_t(L", ")) + bstr_t(elementD) + bstr_t(L"=") + bstr_t(elementR);
                            }
                            DispValues += bstr_t(L"}");
                        }
                        outfile << DispValues;
                    }
                    outfile << std::endl;

                    VariantClear(&vtVDV);
                    VariantClear(&vtVRV);

                    pclsObj->Release();
                    pclsObj = NULL;

                }
                pNetAdapterAdvPropEnum->Release();
            }

            std::wcout << L".";

            hres = g_pServicesStdCIMv2->ExecQuery(bstr_t("WQL"), NetAdapterRssQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pNetAdapterRssEnum);
            if (FAILED(hres)) {
                std::cerr << "Query for NetAdapterRss failed. Error code = 0x" << std::hex << hres << std::endl;
                // don't break or return here, continue with more info
            }
            else {
                outfile << L"Rss:" << std::endl;
                while (pNetAdapterRssEnum) {
                    hres = pNetAdapterRssEnum->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                    if (0 == uReturn) {
                        break;
                    }
                    // Get the value of the Enabled property
                    hres = pclsObj->Get(L"Enabled", 0, &vtProp, 0, 0);
                    outfile << L"\tEnabled: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the NumberOfReceiveQueues property
                    hres = pclsObj->Get(L"NumberOfReceiveQueues", 0, &vtProp, 0, 0);
                    outfile << L"\tNumberOfReceiveQueues: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the Profile property
                    hres = pclsObj->Get(L"Profile", 0, &vtProp, 0, 0);
                    outfile << L"\tProfile: "; PrintVariant(vtProp, outfile);
                    switch (vtProp.lVal) {
                    case 1: outfile << L" ( Closest Processor )"; break;
                    case 2: outfile << L" ( Closest Processor Static )"; break;
                    case 3: outfile << L" ( NUMA Scaling )"; break;
                    case 4: outfile << L" ( NUMA Scaling Static )"; break;
                    case 5: outfile << L" ( Conservative Scaling )"; break;
                    }
                    outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the BaseProcessor property
                    hres = pclsObj->Get(L"BaseProcessorGroup", 0, &vtProp, 0, 0);
                    outfile << L"\tBaseProcessor[Group:Number]: "; PrintVariant(vtProp, outfile);
                    VariantClear(&vtProp);
                    hres = pclsObj->Get(L"BaseProcessorNumber", 0, &vtProp, 0, 0);
                    outfile << L":"; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the MaxProcessor property
                    hres = pclsObj->Get(L"MaxProcessorGroup", 0, &vtProp, 0, 0);
                    outfile << L"\tMaxProcessor[Group:Number]: "; PrintVariant(vtProp, outfile);
                    VariantClear(&vtProp);
                    hres = pclsObj->Get(L"MaxProcessorNumber", 0, &vtProp, 0, 0);
                    outfile << L":"; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the MaxProcessors property
                    hres = pclsObj->Get(L"MaxProcessors", 0, &vtProp, 0, 0);
                    outfile << L"\tMaxProcessors: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);

                    // Get the value of the RssProcessorArraySize property
                    hres = pclsObj->Get(L"RssProcessorArraySize", 0, &vtProp, 0, 0);
                    LONG ArrayElemCount = vtProp.lVal;
                    VariantClear(&vtProp);\
                    // Get the value of the RssProcessorArray property
                    hres = pclsObj->Get(L"RssProcessorArray", 0, &vtProp, 0, 0);
                    if (vtProp.vt == (VT_ARRAY | VT_UNKNOWN)) {
                        outfile << L"\tRssProcessorArray[Group:Number/NUMA Distance]:" << std::endl;
                        for (LONG i = 0; i < ArrayElemCount; i++) {
                            SAFEARRAY* pSafeArray = V_ARRAY(&vtProp);
                            IUnknown* pUnk = NULL;
                            SafeArrayGetElement(pSafeArray, &i, (void*)&pUnk);
                            if (pUnk) {
                                IWbemClassObject* pWbemClsObj = NULL;
                                pUnk->QueryInterface(IID_IWbemClassObject, (LPVOID*)&pWbemClsObj);
                                if (pWbemClsObj) {
                                    VARIANT vtPropRssProc;
                                    // Get the value of the ProcessorGroup property
                                    hres = pWbemClsObj->Get(L"ProcessorGroup", 0, &vtPropRssProc, 0, 0);
                                    outfile << ((0==i) ? L"\t\t" : L"  "); PrintVariant(vtPropRssProc, outfile);
                                    VariantClear(&vtPropRssProc);
                                    // Get the value of the ProcessorNumber property
                                    hres = pWbemClsObj->Get(L"ProcessorNumber", 0, &vtPropRssProc, 0, 0);
                                    outfile << L":"; PrintVariant(vtPropRssProc, outfile);
                                    VariantClear(&vtPropRssProc);
                                    // Get the value of the PreferenceIndex property
                                    hres = pWbemClsObj->Get(L"PreferenceIndex", 0, &vtPropRssProc, 0, 0);
                                    outfile << L"/"; PrintVariant(vtPropRssProc, outfile);
                                    VariantClear(&vtPropRssProc);

                                    pWbemClsObj->Release();
                                }
                            }
                        }
                        outfile << std::endl;
                    }
                    VariantClear(&vtProp);

                    // Get the value of the IndirectionTableEntryCount property
                    hres = pclsObj->Get(L"IndirectionTableEntryCount", 0, &vtProp, 0, 0);
                    ArrayElemCount = vtProp.lVal;
                    VariantClear(&vtProp);
                    // Get the value of the IndirectionTable property
                    hres = pclsObj->Get(L"IndirectionTable", 0, &vtProp, 0, 0);
                    if (vtProp.vt == (VT_ARRAY | VT_UNKNOWN)) {
                        outfile << L"\tIndirectionTable: [Group:Number]:";
                        for (LONG i = 0; i < ArrayElemCount; i++) {
                            SAFEARRAY* pSafeArray = V_ARRAY(&vtProp);
                            IUnknown* pUnk = NULL;
                            SafeArrayGetElement(pSafeArray, &i, (void*)&pUnk);
                            if (pUnk) {
                                IWbemClassObject* pWbemClsObj = NULL;
                                pUnk->QueryInterface(IID_IWbemClassObject, (LPVOID*)&pWbemClsObj);
                                if (pWbemClsObj) {
                                    VARIANT vtPropRssProc;
                                    // Get the value of the ProcessorGroup property
                                    hres = pWbemClsObj->Get(L"ProcessorGroup", 0, &vtPropRssProc, 0, 0);
                                    if (0 == i % 8) {
                                        outfile << std::endl;
                                    }
                                    outfile << ((0 == i%8) ? L"\t\t" : L"    "); PrintVariant(vtPropRssProc, outfile);
                                    VariantClear(&vtPropRssProc);
                                    // Get the value of the ProcessorNumber property
                                    hres = pWbemClsObj->Get(L"ProcessorNumber", 0, &vtPropRssProc, 0, 0);
                                    outfile << L":"; PrintVariant(vtPropRssProc, outfile);
                                    VariantClear(&vtPropRssProc);
                                    pWbemClsObj->Release();
                                }
                            }
                        }
                        outfile << std::endl;
                    }
                    VariantClear(&vtProp);

                    pclsObj->Release();
                    pclsObj = NULL;

                }

                pNetAdapterRssEnum->Release();
            }

            std::wcout << L".";

            hres = g_pServicesCIMv2->ExecQuery(bstr_t("WQL"), NetAdapterConfigQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pNetAdapterConfigEnum);
            if (FAILED(hres)) {
                std::cerr << "Query for NetAdapterAdvProperty failed. Error code = 0x" << std::hex << hres << std::endl;
                // don't break or return here, continue with the next adapter
                continue;
            }
            else {
                outfile << L"Network Adapter Configuration:" << std::endl;
                while (pNetAdapterConfigEnum) {
                    hres = pNetAdapterConfigEnum->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                    if (0 == uReturn) {
                        break;
                    }
                    // Get the value of the IPAddress property
                    hres = pclsObj->Get(L"IPAddress", 0, &vtProp, 0, 0);
                    outfile << L"\tIPAddress: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the IPSubnet property
                    hres = pclsObj->Get(L"IPSubnet", 0, &vtProp, 0, 0);
                    outfile << L"\tIPSubnet:  "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DefaultIPGateway property
                    hres = pclsObj->Get(L"DefaultIPGateway", 0, &vtProp, 0, 0);
                    outfile << L"\tDefaultIPGateway: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DNSHostName property
                    hres = pclsObj->Get(L"DNSHostName", 0, &vtProp, 0, 0);
                    outfile << L"\tDNSHostName: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DNSDomain property
                    hres = pclsObj->Get(L"DNSDomain", 0, &vtProp, 0, 0);
                    outfile << L"\tDNSDomain: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DNSServerSearchOrder property
                    hres = pclsObj->Get(L"DNSServerSearchOrder", 0, &vtProp, 0, 0);
                    outfile << L"\tDNSServerSearchOrder: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DNSDomainSuffixSearchOrder property
                    hres = pclsObj->Get(L"DNSDomainSuffixSearchOrder", 0, &vtProp, 0, 0);
                    outfile << L"\tDNSDomainSuffixSearchOrder: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DHCPEnabled property
                    hres = pclsObj->Get(L"DHCPEnabled", 0, &vtProp, 0, 0);
                    outfile << L"\tDHCPEnabled: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DHCPServer property
                    hres = pclsObj->Get(L"DHCPServer", 0, &vtProp, 0, 0);
                    outfile << L"\tDHCPServer: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DHCPLeaseObtained property
                    hres = pclsObj->Get(L"DHCPLeaseObtained", 0, &vtProp, 0, 0);
                    outfile << L"\tDHCPLeaseObtained: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);
                    // Get the value of the DHCPLeaseExpires property
                    hres = pclsObj->Get(L"DHCPLeaseExpires", 0, &vtProp, 0, 0);
                    outfile << L"\tDHCPLeaseExpires: "; PrintVariant(vtProp, outfile); outfile << std::endl;
                    VariantClear(&vtProp);

                    pclsObj->Release();
                    pclsObj = NULL;

                }


                pNetAdapterConfigEnum->Release();
            }

            std::wcout << L".";
            outfile << L"====================================================================" << std::endl << std::endl;

        }
        pNetAdapterEnum->Release();
    }

    std::wcout << std::endl << L"Finished collecting Ionic NetAdapter info" << std::endl;

    return hres;
}
