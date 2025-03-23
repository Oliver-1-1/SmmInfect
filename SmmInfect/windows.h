#pragma once
#include <Uefi.h>
#include <Protocol/SmmBase2.h>
#include <PiSmm.h>
#include <Protocol/SmmCpu.h>

EFI_STATUS SetupWindows(EFI_SMM_CPU_PROTOCOL* cpu, EFI_SMM_SYSTEM_TABLE2* smst);

UINT64 ZGetWindowsProcAddressX64(UINT64 cr3, UINT64 base, const char* export_name);
UINT64 GetWindowsEProcess(const char* process_name);
UINT64 GetWindowsBaseAddressModuleX64(UINT64 eprocess, unsigned short* process_name);
UINT64 GetWindowsSectionBaseAddressX64(UINT64 eprocess, UINT64 module_base, unsigned char* section_name);

UINT64 GetWindowsKernelBase();
UINT64 GetWindowsKernelCr3();

UINT64 GetWindowsProcessCr3(UINT64 eprocess);
UINT64 GetWindowsProcessPEB(UINT64 eprocess);
UINT64 GetWindowsProcessBaseAddress(UINT64 eprocess);
