#pragma once
#include <Uefi.h>
#include <Protocol/SmmBase2.h>
#include <PiSmm.h>
#include <Protocol/SmmCpu.h>
typedef UINT64 (*KallsymsLookupName)(const char *name);
EFI_STATUS SetupLinux(EFI_SMM_CPU_PROTOCOL* cpu, EFI_SMM_SYSTEM_TABLE2* smst);

EFI_STATUS GetLinuxKernelBase(UINT64* base);
EFI_STATUS GetLinuxKernelCr3(UINT64* cr3);
KallsymsLookupName FindLinuxKallsymsLookupName(UINT64 base);
KallsymsLookupName GetKallsyms();
EFI_STATUS PatchRtmFunction(UINT64 phystarget, UINT64 dos);
