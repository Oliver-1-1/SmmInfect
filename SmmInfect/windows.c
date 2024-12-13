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
static EFI_STATUS SetupWindows();

UINT64 ZGetProcAddressX64(UINT64 cr3, UINT64 base, const char* export_name)
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

    while (StrCmpi((char*)name, export_name))
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

UINT64 GetBaseAddressModuleX64(UINT64 eprocess, unsigned short* process_name)
{
    UINT64* v3;
    UINT64* i;

    UINT64 cr3 = GetProcessCr3(eprocess);
    UINT64 a1 = GetProcessPEB(eprocess);

    v3 = (UINT64*)((UINT64)ReadVirtual64(a1 + 24, cr3) + 32);
    for (i = (UINT64*)ReadVirtual64((UINT64)v3, cr3);; i = (UINT64*)ReadVirtual64((UINT64)i, cr3))
    {
        if (i == v3)
        {
            return 0;
        }

        unsigned short name[30];
        ReadVirtual(ReadVirtual64((UINT64)i + (UINT64)sizeof(UINT64) * 10, cr3), cr3, (UINT8*)name, sizeof(name));

        if (!(unsigned int)WcsCmpi(name, process_name))
        {
            break;
        }
    }

    return ReadVirtual64((UINT64)i + sizeof(UINT64) * 4, cr3);
}

UINT64 GetSectionBaseAddressX64(UINT64 eprocess, UINT64 module_base, unsigned char* section_name)
{
    INT64 v4;
    INT32 v5;
    INT32 v6;
    INT64 v7;
    UINT64 cr3 = GetProcessCr3(eprocess);

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
    while (StrCmpi((const char*)name, (const char*)section_name))
    {
        ++v5;
        v7 += 40;
        if (v5 >= v6)
            return 0;

        ReadVirtual(v7, cr3, (UINT8*)name, sizeof(name));
    }
    return module_base + ReadVirtual32(v7 + 12, cr3);
}

UINT64 GetKernelBase()
{
    if (EFI_ERROR(SetupWindows()))
    {
        return 0;
    }

    return KernelBase;
}

UINT64 GetKernelCr3()
{
    if (EFI_ERROR(SetupWindows()))
    {
        return 0;
    }

    return KernelCr3;
}

UINT64 GetProcessCr3(UINT64 eprocess)
{
    // _EPROCESS -> _KPROCESS -> 0x28;
    return ReadVirtual64(eprocess + 0x28, KernelCr3);
}

UINT64 GetProcessPEB(UINT64 eprocess)
{
    return ReadVirtual64(eprocess + PsGetProcessPeb, KernelCr3);
}

UINT64 GetProcessBaseAddress(UINT64 eprocess)
{
    return ReadVirtual64(eprocess + PsGetProcessSectionBaseAddress, KernelCr3);
}

EFI_STATUS SetupWindows()
{
    if (SetupDone == TRUE)
    {
        return EFI_SUCCESS;
    }

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

    PsInitialSystemProcess = ZGetProcAddressX64(KernelCr3, KernelBase, "PsInitialSystemProcess");
    if (PsInitialSystemProcess == 0)
    {
        return EFI_NOT_FOUND;
    }

    PsGetProcessSectionBaseAddress = ReadVirtual32(ZGetProcAddressX64(KernelCr3, KernelBase, "PsGetProcessSectionBaseAddress") + 3, KernelCr3);
    PsGetProcessExitProcessCalled = ReadVirtual32(ZGetProcAddressX64(KernelCr3, KernelBase, "PsGetProcessExitProcessCalled") + 2, KernelCr3);
    PsGetProcessImageFileName = ReadVirtual32(ZGetProcAddressX64(KernelCr3, KernelBase, "PsGetProcessImageFileName") + 3, KernelCr3);
    ActiveProcessLinks = ReadVirtual32(ZGetProcAddressX64(KernelCr3, KernelBase, "PsGetProcessId") + 3, KernelCr3) + 8;
    PsGetProcessPeb = ReadVirtual32(ZGetProcAddressX64(KernelCr3, KernelBase, "PsGetProcessPeb") + 3, KernelCr3);

    if (!PsInitialSystemProcess || !PsGetProcessExitProcessCalled || !PsGetProcessImageFileName || !ActiveProcessLinks || !PsGetProcessPeb || !PsGetProcessSectionBaseAddress)
    {
        return EFI_NOT_FOUND;
    }

    SetupDone = TRUE;
    return EFI_SUCCESS;
}

// credits ekknod
UINT64 GetEProcess(const char* process_name)
{

    if (EFI_ERROR(SetupWindows()))
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

        if (!exitcalled && !StrCmpi(name, process_name))
            return entry;

        entry = ReadVirtual64(entry + ActiveProcessLinks, KernelCr3);
        if (entry == 0)
            break;

        entry = entry - ActiveProcessLinks;

    } while (entry != proc);
    return 0;
}
