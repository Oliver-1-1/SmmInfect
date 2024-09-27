#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winnt.h>
#include <winternl.h>
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

void TriggerSmi()
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

void main()
{
  printf("Size of protocol %x\n", (INT)sizeof(SmmCommunicationProtocol)); // 6b
  printf("Address of protocol %x\n", (INT)&protocol);

  while (true)
  {
    //Read the first 15 bytes of explorer.exe. This will include DOS header if the SMM module is setup correctly.
    strcpy((char*)protocol.process_name, "explorer.exe");
    wcscpy((wchar_t*)protocol.module_name, L"explorer.exe");
    protocol.offset = 0;//= 0x2F000;
    protocol.read_size = 15;
    memset((void*)protocol.read_buffer, 0, sizeof(protocol.read_buffer));

    // Trigger a SMI and the driver will find this process.
    TriggerSmi();

    // Print out the bytes the SMM driver read for us.
    printf("Smi count: %llu\n", protocol.smi_count);

    for (int i = 0; i < protocol.read_size; i++)
    {
      printf("%02X ", protocol.read_buffer[i]);
    }
    
    Sleep(50);
  }
}



