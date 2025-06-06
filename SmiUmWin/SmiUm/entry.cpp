#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winnt.h>
#include <winternl.h>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "wbemuuid.lib")

#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof((s)[0]), sizeof(s), (PWSTR)s }
#define EFI_VARIABLE_NON_VOLATILE                          0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                    0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS                        0x00000004

extern "C"
{
    NTSYSAPI NTSTATUS NTAPI RtlAdjustPrivilege(ULONG Privilege, BOOLEAN Enable, BOOLEAN Client, PBOOLEAN WasEnabled);
    NTSYSCALLAPI NTSTATUS NTAPI NtSetSystemEnvironmentValueEx(PUNICODE_STRING VariableName, LPGUID VendorGuid, PVOID Value, ULONG ValueLength, ULONG Attributes);
}

#pragma pack(1) // Ensure that alignment is 1-byte
typedef struct _SmmCommunicationProtocol
{
    UINT8 magic = 'i'; // Magic identifier 
    UINT8 process_name[30] = { 0 };
    UINT16 module_name[30] = { 0 };
    UINT64 offset = 0;
    UINT64 read_size = 0;
    UINT8 read_buffer[30] = { 0 };
    UINT64 smi_count = 2976579765;

} SmmCommunicationProtocol, * PSmmCommunicationProtocol;
#pragma pack() // reset

// Allocate a section so the SMM driver knows what section the communication payload is in.
#pragma section(".ZEPTA", read, write)
__declspec(allocate(".ZEPTA")) volatile SmmCommunicationProtocol protocol;

enum SmiType
{
    UefiRuntime,
    TpmAcpi
};

void TriggerSmi(SmiType type);
void TriggerSmiUefiRuntimeVariable();
void TriggerSmiTpmAcpi();
void main();


void main()
{
    printf("Size of protocol %x\n", (INT)sizeof(SmmCommunicationProtocol)); // 6b
    printf("Address of protocol %x\n", (INT)&protocol);

    while (true)
    {
        //Read the first 15 bytes of explorer.exe. This will include DOS header if the SMM module is setup correctly.
        strcpy_s((char*)protocol.process_name, sizeof(protocol.process_name), "explorer.exe");
        wcscpy_s((wchar_t*)protocol.module_name, sizeof(protocol.module_name) / sizeof(UINT16), L"explorer.exe");
        protocol.offset = 0;//= 0x2F000;
        protocol.read_size = 15;
        memset((void*)protocol.read_buffer, 0, sizeof(protocol.read_buffer));

        // Trigger a SMI and the driver will find this process.
        TriggerSmi(TpmAcpi);
        //TriggerSmi(UefiRuntime);

        // Print out the bytes the SMM driver read for us.
        printf("Smi count: %llu\n", protocol.smi_count);

        for (int i = 0; i < protocol.read_size; i++)
        {
            printf("%02X ", protocol.read_buffer[i]);
        }

        Sleep(1000);
    }
}

void TriggerSmi(SmiType type)
{
    if (type == UefiRuntime)
    {
        TriggerSmiUefiRuntimeVariable();
    }
    else if (type == TpmAcpi)
    {
        TriggerSmiTpmAcpi();
    }
    else
    {
        printf("Unknown SMI trigger\n");
    }
}

void TriggerSmiUefiRuntimeVariable()
{
    BOOLEAN e = false;
    //Get the right privileges to call NtSetSystemEnvironmentValueEx. ( SeSystemEnvironmentPrivilege )
    const NTSTATUS status = RtlAdjustPrivilege(22, true, false, &e);

    if (!NT_SUCCESS(status))
    {
        // We need admin privileges!
        printf("No suitable permission! Open as admin!\n");
        return;
    }

    GUID guid = { 0 };
    // Try to get a variable that doesn't exist so we don't trigger a runtime cache hit
    UNICODE_STRING name = RTL_CONSTANT_STRING(L"ZeptaVar");
    char buffer[8];
    NtSetSystemEnvironmentValueEx(&name, &guid, buffer, sizeof(buffer), EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS);
}

//The TPM2 definition block in ACPI table clearly defines how to trigger a SMI.
//Windows exposes a WMI interface to the TPM that allows us to trigger the SMI.
//This can also be done by opening tpm.msc and press Prepare the TPM or Clear TPM...
//https://github.com/tianocore/edk2/blob/master/SecurityPkg/Tcg/Tcg2Acpi/Tpm.asl
void TriggerSmiTpmAcpi()
{

    HANDLE token = NULL;
    TOKEN_ELEVATION elevation;
    DWORD size;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        printf("Could not open process token\n");
        return;
    }

    BOOL isAdmin = FALSE;
    if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
    {
        isAdmin = elevation.TokenIsElevated;
    }
    else
    {
        printf("Could not get token information\n");
        CloseHandle(token);
        return;
    }

    if (!isAdmin)
    {
        printf("No suitable permission! Open as admin!\n");
        return;
    }

    HRESULT res;

    res = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(res))
    {
        printf("COM initialize for thread failed\n");
        return;
    }

    // Set security
    res = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

    if (FAILED(res))
    {
        printf("Security set failed\n");
        CoUninitialize();
        return;
    }

    // Connect to WMI
    IWbemLocator* loc = NULL;
    res = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&loc);

    if (FAILED(res))
    {
        printf("Could not create instance\n");
        CoUninitialize();
        return;
    }

    IWbemServices* svc = NULL;
    res = loc->ConnectServer(_bstr_t(L"ROOT\\CIMV2\\Security\\MicrosoftTpm"), NULL, NULL, 0, NULL, 0, 0, &svc);

    if (FAILED(res))
    {
        printf("Could not connect to server\n");
        loc->Release();
        CoUninitialize();
        return;
    }

    // Set security levels on the proxy
    res = CoSetProxyBlanket(svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    if (FAILED(res))
    {
        printf("Could not set proxy blanket\n");
        svc->Release();
        loc->Release();
        CoUninitialize();
        return;
    }

    // Get TPM class
    IEnumWbemClassObject* enume = NULL;
    res = svc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_Tpm"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enume);

    if (FAILED(res))
    {
        printf("WMI query failed\n");
        svc->Release();
        loc->Release();
        CoUninitialize();
        return;
    }

    // Get the TPM object
    IWbemClassObject* tpm = NULL;
    ULONG ret = 0;

    res = enume->Next(WBEM_INFINITE, 1, &tpm, &ret);
    if (ret == 0)
    {
        printf("No TPM found\n");
        enume->Release();
        svc->Release();
        loc->Release();
        CoUninitialize();
        return;
    }

    // Call SetPhysicalPresenceRequest method
    // https://learn.microsoft.com/en-us/windows/win32/secprov/setphysicalpresencerequest-win32-tpm
    VARIANT request;
    VariantInit(&request);
    request.vt = VT_I4;
    request.intVal = 6; // 6 = Enable and activate the TPM. 5 should work here aswell.

    IWbemClassObject* params = NULL;
    IWbemClassObject* wclass = NULL;
    IWbemClassObject* oparams = NULL;
    IWbemClassObject* iparams = NULL;

    res = svc->GetObjectW((BSTR)L"Win32_Tpm", 0, NULL, &wclass, NULL);
    if (FAILED(res))
    {
        printf("Failed to get object\n");
        goto Clean;
    }

    res = wclass->GetMethod(L"SetPhysicalPresenceRequest", 0, &params, NULL);
    if (FAILED(res))
    {
        printf("Failed to get method\n");
        goto Clean;
    }

    res = params->SpawnInstance(0, &iparams);
    if (FAILED(res))
    {
        printf("Failed spawn\n");
        goto Clean;
    }

    res = iparams->Put(L"Request", 0, &request, 0);
    if (FAILED(res))
    {
        printf("Failed to set request\n");
        goto Clean;
    }

    //Get-WmiObject -Namespace root\cimv2\Security\MicrosoftTpm -Class Win32_Tpm in powershell and __PATH. This may vary for you.
    // \\DESKTOP-XXXXXX\root\cimv2\Security\MicrosoftTpm:Win32_Tpm=@
    res = svc->ExecMethod(bstr_t(L"Win32_Tpm=@"), bstr_t(L"SetPhysicalPresenceRequest"), 0, NULL, iparams, &oparams, NULL);

    if (FAILED(res))
    {
        printf("Failed to execute\n");
    }
    else
    {
        //printf("SMI triggered.\n");
    }

Clean:

    VariantClear(&request);
    if (oparams) oparams->Release();
    if (iparams) iparams->Release();
    if (wclass) wclass->Release();
    if (tpm) tpm->Release();
    if (enume) enume->Release();
    if (svc) svc->Release();
    if (loc) loc->Release();
    CoUninitialize();
}


