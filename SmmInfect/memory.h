#pragma once

#include <Uefi.h>
#include <Protocol/SmmBase2.h>

// #define Linux UNSUPPORTED

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
UINT64 TranslateVirtualToPhysical(UINT64 cr3, UINT64 address);
EFI_STATUS MemGetKernelBase(UINT64* base);
EFI_STATUS MemGetKernelCr3(UINT64* cr3);

//
// Linux kernel.
//
#ifdef Linux
UINT64 TranslateVirtualToPhysical(UINT64 cr3, UINT64 address);
EFI_STATUS GetKernelBase();
#endif
