#include "communication.h"
#include "memory.h"
#include "windows.h"
#include "linux.h"
#include "serial.h"
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SmmCpu.h>

static EFI_SMM_BASE2_PROTOCOL* SmmBase2;
static EFI_SMM_SYSTEM_TABLE2* GSmst2;
static BOOLEAN OS = FALSE;
static EFI_SMM_CPU_PROTOCOL* Cpu = NULL;

VOID EFIAPI ClearCache();

EFI_STATUS EFIAPI SmiHandler(EFI_HANDLE dispatch, CONST VOID* context, VOID* buffer, UINTN* size)
{
    GSmst2->SmmLocateProtocol(&gEfiSmmCpuProtocolGuid, NULL, (VOID**)&Cpu);
    SERIAL_INIT();
    SERIAL_PRINT("Handler called!\r\n");

    if (!EFI_ERROR(SetupWindows(Cpu, GSmst2)))
    {
        OS = TRUE;
        if (!EFI_ERROR(PerformCommunication()))
        {
            return EFI_SUCCESS;
        }
    }

    if (!EFI_ERROR(SetupLinux(Cpu, GSmst2)))
    {
        OS = TRUE;
        // Linux hook not implemented yet
    }

    // Make sure we are not running into a cache side channel attack. When the system leaves SMM it might clear cache.
    //ClearCache();

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE image, IN EFI_SYSTEM_TABLE* table)
{
    gRT = table->RuntimeServices;
    gBS = table->BootServices;
    gST = table;
    SERIAL_INIT();

    if (EFI_ERROR(gBS->LocateProtocol(&gEfiSmmBase2ProtocolGuid, 0, (void**)&SmmBase2)))
    {
        SERIAL_PRINT("Failed to find SmmBase!\r\n");
        return EFI_SUCCESS;
    }

    if (EFI_ERROR(SmmBase2->GetSmstLocation(SmmBase2, &GSmst2)))
    {
        SERIAL_PRINT("Failed to find smst!\r\n");
        return EFI_SUCCESS;
    }

    //Register root handler
    EFI_HANDLE handle;
    GSmst2->SmiHandlerRegister(&SmiHandler, NULL, &handle);

    if (EFI_ERROR(SetupMemory(GSmst2)))
    {
        SERIAL_PRINT("Failed to setup memory\r\n");
        return EFI_ERROR_MAJOR;
    }

    SERIAL_PRINT("Handler registered!\r\n");

    return  EFI_SUCCESS;
}
