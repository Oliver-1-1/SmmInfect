#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winnt.h>
#include <winternl.h>
#include <windows.h>
#include <tbs.h>

#pragma comment(lib, "tbs.lib")
#pragma comment(lib, "ntdll.lib")

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
        //TriggerSmi(TpmAcpi);
        TriggerSmi(UefiRuntime);

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
    BYTE buffer[256] = { 0u };
    UINT32 size = sizeof(buffer);
    TBS_RESULT result = TBS_SUCCESS;

    PTBS_HCONTEXT context = new TBS_HCONTEXT;
    TBS_CONTEXT_PARAMS2 params = { 0u };

    HANDLE token = NULL;
    TOKEN_ELEVATION elevation;
    DWORD retlen;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        printf("Could not open process token\n");
        return;
    }

    BOOL isAdmin = FALSE;
    if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &retlen))
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

    params.version = TPM_VERSION_20;
    params.asUINT32 = 0;
    params.includeTpm20 = TRUE;

    // Create TBS contex
    result = Tbsi_Context_Create((PCTBS_CONTEXT_PARAMS)&params, context);
    if (result != TBS_SUCCESS)
    {
        printf("Could not create context: %x\n", result);
        return;
    }

    RtlZeroMemory(buffer, sizeof(buffer));
    buffer[0] = 0x00000005u;

    result = Tbsi_Physical_Presence_Command(*context, buffer, sizeof(DWORD), buffer, &size);

    if (result != TBS_SUCCESS)
    {
        printf("Command error: %x\n", result);
    }
}
