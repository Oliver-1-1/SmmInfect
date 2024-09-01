#pragma once
#include "memory.h"
#include "string.h"

UINT64 ZGetProcAddressX64(UINT64 cr3, UINT64 base, const char* export_name);
UINT64 GetEProcess(const char* process_name);
UINT64 GetBaseAddressModuleX64(UINT64 eprocess, unsigned short* process_name);
UINT64 GetSectionBaseAddressX64(UINT64 eprocess, UINT64 module_base, unsigned char* section_name);

UINT64 GetKernelBase();
UINT64 GetKernelCr3();

UINT64 GetProcessCr3(UINT64 eprocess);
UINT64 GetProcessPEB(UINT64 eprocess);
UINT64 GetProcessBaseAddress(UINT64 eprocess);
