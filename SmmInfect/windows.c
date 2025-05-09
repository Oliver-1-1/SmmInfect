#include <Library/BaseLib.h>
#include "windows.h"
#include "memory.h"
#include "string.h"

static UINT64 KernelCr3 = 0;
static UINT64 KernelBase = 0;
static UINT64 PsInitialSystemProcess;
static UINT64 PsGetProcessExitProcessCalled = 0;
static UINT64 PsGetProcessImageFileName = 0;
static UINT64 ActiveProcessLinks = 0;
static UINT64 PsGetProcessPeb = 0;
static UINT64 PsGetProcessSectionBaseAddress = 0;
static BOOLEAN SetupDone = FALSE;
static EFI_SMM_CPU_PROTOCOL* Cpu;
static EFI_SMM_SYSTEM_TABLE2* GSmst2;
static EFI_STATUS MemGetKernelCr3(UINT64* cr3);
static EFI_STATUS MemGetKernelBase(UINT64* base);

UINT64 ZGetWindowsProcAddressX64(UINT64 cr3, UINT64 base, const char* export_name)
{
    INT64 v4;
    UINT32 v5;
    UINT32* v6;
    INT64 v7;
    UINT32* v8;
    UINT32 v9;
    INT64 v10;

    if (ReadVirtual16(base, cr3) != 23117)
    {
        return 0;
    }

    v4 = ReadVirtual32(base + 60, cr3);
    if (ReadVirtual32(v4 + base, cr3) != 17744)
    {
        return 0;
    }

    v5 = 0;
    v6 = (UINT32*)(base + ReadVirtual32(v4 + base + 136, cr3));
    v7 = base + (UINT32)ReadVirtual32((UINT64)v6 + sizeof(UINT32) * 7, cr3);
    v8 = (UINT32*)(base + (UINT32)ReadVirtual32((UINT64)v6 + sizeof(UINT32) * 8, cr3));
    v9 = ReadVirtual32((UINT64)v6 + sizeof(UINT32) * 6, cr3);
    v10 = base + (UINT32)ReadVirtual32((UINT64)v6 + sizeof(UINT32) * 9, cr3);
    if (!v9)
    {
        return 0;
    }

    unsigned char name[50] = { 0 };

    ReadVirtual(base + ReadVirtual32((UINT64)v8, cr3), cr3, (UINT8*)name, (UINT64)sizeof(name));
    name[49] = 0;

    while (StrCmpi(export_name, (char*)name))
    {
        ++v5;
        ++v8;
        if (v5 >= v9)
        {
            return 0;
        }

        ReadVirtual(base + (UINT64)ReadVirtual32((UINT64)v8, cr3), cr3, (UINT8*)name, (UINT64)sizeof(name));
        name[49] = 0;
    }
    return base + ReadVirtual32(v7 + 4 * ReadVirtual16(v10 + 2 * (INT32)v5, cr3), cr3);
}

UINT64 GetWindowsBaseAddressModuleX64(UINT64 eprocess, unsigned short* process_name)
{
    UINT64* v3;
    UINT64* i;

    UINT64 cr3 = GetWindowsProcessCr3(eprocess);
    UINT64 a1 = GetWindowsProcessPEB(eprocess);

    v3 = (UINT64*)((UINT64)ReadVirtual64(a1 + 24, cr3) + 32);
    for (i = (UINT64*)ReadVirtual64((UINT64)v3, cr3);; i = (UINT64*)ReadVirtual64((UINT64)i, cr3))
    {
        if (i == v3)
        {
            return 0;
        }

        unsigned short name[30];
        ReadVirtual(ReadVirtual64((UINT64)i + (UINT64)sizeof(UINT64) * 10, cr3), cr3, (UINT8*)name, sizeof(name));

        if (!(unsigned int)WcsCmpi(process_name, name))
        {
            break;
        }
    }

    return ReadVirtual64((UINT64)i + sizeof(UINT64) * 4, cr3);
}

UINT64 GetWindowsSectionBaseAddressX64(UINT64 eprocess, UINT64 module_base, unsigned char* section_name)
{
    INT64 v4;
    INT32 v5;
    INT32 v6;
    INT64 v7;
    UINT64 cr3 = GetWindowsProcessCr3(eprocess);

    if (ReadVirtual16(module_base, cr3) != 23117)
        return 0;
    v4 = module_base + ReadVirtual32(module_base + 60, cr3);
    if (ReadVirtual32(v4, cr3) != 17744)
        return 0;
    v5 = 0;
    v6 = ReadVirtual16(v4 + 6, cr3);
    v7 = v4 + ReadVirtual16(v4 + 20, cr3) + 24;
    if (!ReadVirtual16(v4 + 6, cr3))
        return 0;

    char* name[8] = { 0 };
    ReadVirtual(v7, cr3, (UINT8*)name, sizeof(name));
    while (StrCmpi((const char*)section_name, (const char*)name))
    {
        ++v5;
        v7 += 40;
        if (v5 >= v6)
            return 0;

        ReadVirtual(v7, cr3, (UINT8*)name, sizeof(name));
    }
    return module_base + ReadVirtual32(v7 + 12, cr3);
}

UINT64 GetWindowsKernelBase()
{
    if (EFI_ERROR(SetupWindows(Cpu, GSmst2)))
    {
        return 0;
    }

    return KernelBase;
}

UINT64 GetWindowsKernelCr3()
{
    if (EFI_ERROR(SetupWindows(Cpu, GSmst2)))
    {
        return 0;
    }

    return KernelCr3;
}

UINT64 GetWindowsProcessCr3(UINT64 eprocess)
{
    // _EPROCESS -> _KPROCESS -> 0x28;
    return ReadVirtual64(eprocess + 0x28, KernelCr3);
}

UINT64 GetWindowsProcessPEB(UINT64 eprocess)
{
    return ReadVirtual64(eprocess + PsGetProcessPeb, KernelCr3);
}

UINT64 GetWindowsProcessBaseAddress(UINT64 eprocess)
{
    return ReadVirtual64(eprocess + PsGetProcessSectionBaseAddress, KernelCr3);
}

EFI_STATUS SetupWindows(EFI_SMM_CPU_PROTOCOL* cpu, EFI_SMM_SYSTEM_TABLE2* smst)
{
    if (SetupDone == TRUE)
    {
        return EFI_SUCCESS;
    }

    if (cpu == NULL || smst == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }
    Cpu = cpu;
    GSmst2 = smst;

    EFI_STATUS status = MemGetKernelCr3(&KernelCr3);
    if (EFI_ERROR(status))
    {
        return EFI_NOT_FOUND;
    }

    status = MemGetKernelBase(&KernelBase);
    if (EFI_ERROR(status))
    {
        return EFI_NOT_FOUND;
    }

    PsInitialSystemProcess = ZGetWindowsProcAddressX64(KernelCr3, KernelBase, "PsInitialSystemProcess");
    if (PsInitialSystemProcess == 0)
    {
        return EFI_NOT_FOUND;
    }

    PsGetProcessSectionBaseAddress = ReadVirtual32(ZGetWindowsProcAddressX64(KernelCr3, KernelBase, "PsGetProcessSectionBaseAddress") + 3, KernelCr3);
    PsGetProcessExitProcessCalled = ReadVirtual32(ZGetWindowsProcAddressX64(KernelCr3, KernelBase, "PsGetProcessExitProcessCalled") + 2, KernelCr3);
    PsGetProcessImageFileName = ReadVirtual32(ZGetWindowsProcAddressX64(KernelCr3, KernelBase, "PsGetProcessImageFileName") + 3, KernelCr3);
    ActiveProcessLinks = ReadVirtual32(ZGetWindowsProcAddressX64(KernelCr3, KernelBase, "PsGetProcessId") + 3, KernelCr3) + 8;
    PsGetProcessPeb = ReadVirtual32(ZGetWindowsProcAddressX64(KernelCr3, KernelBase, "PsGetProcessPeb") + 3, KernelCr3);

    if (!PsInitialSystemProcess || !PsGetProcessExitProcessCalled || !PsGetProcessImageFileName || !ActiveProcessLinks || !PsGetProcessPeb || !PsGetProcessSectionBaseAddress)
    {
        return EFI_NOT_FOUND;
    }

    SetupDone = TRUE;
    return EFI_SUCCESS;
}

// credits ekknod
UINT64 GetWindowsEProcess(const char* process_name)
{

    if (EFI_ERROR(SetupWindows(Cpu, GSmst2)))
    {
        return 0;
    }

    UINT64 entry;
    char name[15] = { 0 };

    UINT64 proc = ReadVirtual64(PsInitialSystemProcess, KernelCr3);
    entry = proc;
    do
    {
        ReadVirtual(entry + PsGetProcessImageFileName, KernelCr3, (UINT8*)name, 15);
        name[14] = 0;

        UINT32 exitcalled = ReadVirtual32(entry + PsGetProcessExitProcessCalled, KernelCr3);
        exitcalled = exitcalled >> 2;
        exitcalled = exitcalled & 1;

        if (!exitcalled && !StrCmpi(process_name, name))
            return entry;

        entry = ReadVirtual64(entry + ActiveProcessLinks, KernelCr3);
        if (entry == 0)
            break;

        entry = entry - ActiveProcessLinks;

    } while (entry != proc);
    return 0;
}

EFI_STATUS MemGetKernelCr3(UINT64* cr3)
{
    if (cr3 == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    UINT64 rip;
    UINT64 tempcr3;
    Cpu->ReadSaveState(Cpu, sizeof(tempcr3), EFI_SMM_SAVE_STATE_REGISTER_CR3, GSmst2->CurrentlyExecutingCpu, (VOID*)&tempcr3);
    Cpu->ReadSaveState(Cpu, sizeof(rip), EFI_SMM_SAVE_STATE_REGISTER_RIP, GSmst2->CurrentlyExecutingCpu, (VOID*)&rip);

    if (tempcr3 == AsmReadCr3() || (tempcr3 & 0xFFF) != 0) {
        return EFI_NOT_FOUND;
    }

    if (rip < 0xFFFF800000000000) {
        return EFI_NOT_FOUND;
    } 

    UINT64 kernel_entry = rip & ~(SIZE_2MB - 1);

    for (UINT16 i = 0; i < 0x30; i++)
    {
        UINT64 address = kernel_entry - (i * SIZE_2MB);
        UINT64 address2 = kernel_entry + (i * SIZE_2MB);

        if (ReadVirtual16(address, tempcr3) == 23117)
        {
            *cr3 = tempcr3;
            return EFI_SUCCESS;
        }

        if (ReadVirtual16(address2, tempcr3) == 23117)
        {
            *cr3 = tempcr3;
            return EFI_SUCCESS;
        }
    }

    return EFI_NOT_FOUND;
}

EFI_STATUS MemGetKernelBase(UINT64* base)
{
    if (base == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    UINT64 rip;
    UINT64 cr3;
    Cpu->ReadSaveState(Cpu, sizeof(cr3), EFI_SMM_SAVE_STATE_REGISTER_CR3, GSmst2->CurrentlyExecutingCpu, (VOID*)&cr3);
    Cpu->ReadSaveState(Cpu, sizeof(rip), EFI_SMM_SAVE_STATE_REGISTER_RIP, GSmst2->CurrentlyExecutingCpu, (VOID*)&rip);

    UINT64 kernel_entry = rip & ~(SIZE_2MB - 1);

    for (UINT16 i = 0; i < 0x30; i++)
    {
        UINT64 address = kernel_entry - (i * SIZE_2MB);
        UINT64 address2 = kernel_entry + (i * SIZE_2MB);

        if (ReadVirtual16(address, cr3) == 23117)
        {
            *base = address;
            return EFI_SUCCESS;
        }

        if (ReadVirtual16(address2, cr3) == 23117)
        {
            *base = address2;
            return EFI_SUCCESS;
        }
    }

    return EFI_NOT_FOUND;
}
