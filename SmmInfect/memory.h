#pragma once

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmPeriodicTimerDispatch2.h>
#include <Protocol/SmmUsbDispatch2.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <IndustryStandard/PeImage.h>
#include <Guid/GlobalVariable.h>

EFI_RUNTIME_SERVICES* gRT;
EFI_BOOT_SERVICES* gBS;
EFI_SYSTEM_TABLE* gST;

#define Windows
// #define Linux UNSUPPORTED
EFI_SMM_SYSTEM_TABLE2* gSmst2;

void* ZMemSet(void* ptr, int value, UINT64 num);
EFI_STATUS SetupMemoryMap();
BOOLEAN IsAddressValid(UINT64 address);

//
// Physical memory
//
UINT8* ReadPhysical(UINT64 address, UINT8* buffer, UINT64 length);
UINT8 ReadPhysical8(UINT64 address);
UINT16 ReadPhysical16(UINT64 address);
UINT32 ReadPhysical32(UINT64 address);
UINT64 ReadPhysical64(UINT64 address);

//
// Virtual memory
//
UINT8* ReadVirtual(UINT64 address, UINT64 cr3, UINT8* buffer, UINT64 length);
UINT8 ReadVirtual8(UINT64 address, UINT64 cr3);
UINT16 ReadVirtual16(UINT64 address, UINT64 cr3);
UINT32 ReadVirtual32(UINT64 address, UINT64 cr3);
UINT64 ReadVirtual64(UINT64 address, UINT64 cr3);

//
// Windows kernel
//
#ifdef Windows
UINT64 TranslateVirtualToPhysical(UINT64 cr3, UINT64 address);
EFI_STATUS MemGetKernelBase(UINT64* base);
EFI_STATUS MemGetKernelCr3(UINT64* cr3);
#endif

//
// Linux kernel.
//
#ifdef Linux
UINT64 TranslateVirtualToPhysical(UINT64 cr3, UINT64 address);
EFI_STATUS GetKernelBase();
#endif
