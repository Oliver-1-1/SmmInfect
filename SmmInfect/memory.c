#include "memory.h"
#include "windows.h"
#include <Library/UefiBootServicesTableLib.h>

static UINT64 map_begin = 0x1000;
static UINT64 map_end = 0;
static BOOLEAN setup_done = FALSE;
static EFI_SMM_CPU_PROTOCOL* cpu = NULL;
static EFI_SMM_SYSTEM_TABLE2* GSmst2 = NULL;

static UINT64 ScanNtosKernel(UINT64 cr3, UINT64 entry);

UINT8* ReadPhysical(UINT64 address, UINT8* buffer, UINT64 length)
{
    if (address == 0)
        return NULL;

    for (UINT64 i = 0; i < length; i++)
    {
        buffer[i] = *(UINT8*)(address + i);
    }

    return buffer;
}

UINT8 ReadPhysical8(UINT64 address)
{
    return !address ? 0 : *(UINT8*)address;
}

UINT16 ReadPhysical16(UINT64 address)
{
    return !address ? 0 : *(UINT16*)address;
}

UINT32 ReadPhysical32(UINT64 address)
{
    return !address ? 0 : *(UINT32*)address;
}

UINT64 ReadPhysical64(UINT64 address)
{
    return !address ? 0 : *(UINT64*)address;
}

void* ZMemSet(void* ptr, int value, UINT64 num)
{
    unsigned char* p = (unsigned char*)ptr;
    while (num--)
    {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

// Clamp the virtual read to bounderies of page size.
UINT8* ReadVirtual(UINT64 address, UINT64 cr3, UINT8* buffer, UINT64 length)
{
    if (buffer == NULL)
        return NULL;

    UINT64 remaining_size = length;
    UINT64 offset = 0;

    while (remaining_size > 0)
    {
        UINT64 physical_address = TranslateVirtualToPhysical(cr3, address + offset);
        UINT64 page_offset = physical_address & 0xFFF;
        UINT64 current_size = 0x1000 - page_offset;

        if (current_size > remaining_size)
        {
            current_size = remaining_size;
        }

        if (!physical_address)
        {
            ZMemSet(buffer + offset, 0, current_size);
        }
        else
        {
            if (!ReadPhysical(physical_address, buffer + offset, current_size))
            {
                break;
            }
        }

        offset += current_size;
        remaining_size -= current_size;
    }

    return buffer;
}

UINT8 ReadVirtual8(UINT64 address, UINT64 cr3)
{
    UINT8 value = 0;
    ReadVirtual(address, cr3, &value, sizeof(value));
    return value;
}

UINT16 ReadVirtual16(UINT64 address, UINT64 cr3)
{
    UINT16 value = 0;
    ReadVirtual(address, cr3, (UINT8*)&value, sizeof(value));
    return value;
}

UINT32 ReadVirtual32(UINT64 address, UINT64 cr3)
{
    UINT32 value = 0;
    ReadVirtual(address, cr3, (UINT8*)&value, sizeof(value));
    return value;
}
UINT64 ReadVirtual64(UINT64 address, UINT64 cr3)
{
    UINT64 value = 0;
    ReadVirtual(address, cr3, (UINT8*)&value, sizeof(value));
    return value;
}

EFI_STATUS SetupMemoryMap()
{
    UINT32 descriptor_version;
    UINTN memory_map_size = 0;
    EFI_MEMORY_DESCRIPTOR* memory_map = NULL;
    UINTN map_key;
    UINTN descriptor_size;
    EFI_MEMORY_DESCRIPTOR* last;
    EFI_STATUS status;
    UINT64 end;

    status = gBS->GetMemoryMap(&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
    if (status == EFI_INVALID_PARAMETER || status == EFI_SUCCESS)
    {
        return EFI_ACCESS_DENIED;
    }

    status = gBS->AllocatePool(EfiBootServicesData, memory_map_size, (VOID**)&memory_map);

    if (EFI_ERROR(status))
    {
        return status;
    }

    status = gBS->GetMemoryMap(&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
    if (EFI_ERROR(status))
    {
        gBS->FreePool(memory_map);
    }

    last = (EFI_MEMORY_DESCRIPTOR*)((INT8*)memory_map + (((UINT32)memory_map_size / (UINT32)descriptor_size) * descriptor_size));
    end = last->PhysicalStart + (last->NumberOfPages * 0x1000);
    gBS->FreePool(memory_map);
    map_end = end;
    return EFI_SUCCESS;
}

BOOLEAN IsAddressValid(UINT64 address)
{
    return !(address < map_begin || address > map_end);
}

// credits ekknod
UINT64 TranslateVirtualToPhysical(UINT64 cr3, UINT64 address)
{
    UINT64 v2;
    UINT64 v3;
    UINT64 v5;
    UINT64 v6;

    v2 = ReadPhysical64(8 * ((address >> 39) & 0x1FF) + cr3);
    if (!v2)
        return 0;

    if ((v2 & 1) == 0)
        return 0;

    v3 = ReadPhysical64((v2 & 0xFFFFFFFFF000) + 8 * ((address >> 30) & 0x1FF));
    if (!v3 || (v3 & 1) == 0)
        return 0;

    if ((v3 & 0x80u) != 0)
        return (address & 0x3FFFFFFF) + (v3 & 0xFFFFFFFFF000);

    v5 = ReadPhysical64((v3 & 0xFFFFFFFFF000) + 8 * ((address >> 21) & 0x1FF));
    if (!v5 || (v5 & 1) == 0)
        return 0;

    if ((v5 & 0x80u) != 0)
        return (address & 0x1FFFFF) + (v5 & 0xFFFFFFFFF000);

    v6 = ReadPhysical64((v5 & 0xFFFFFFFFF000) + 8 * ((address >> 12) & 0x1FF));
    if (v6 && (v6 & 1) != 0)
        return (address & 0xFFF) + (v6 & 0xFFFFFFFFF000);

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
    cpu->ReadSaveState(cpu, sizeof(tempcr3), EFI_SMM_SAVE_STATE_REGISTER_CR3, GSmst2->CurrentlyExecutingCpu, (VOID*)&tempcr3);
    cpu->ReadSaveState(cpu, sizeof(rip), EFI_SMM_SAVE_STATE_REGISTER_RIP, GSmst2->CurrentlyExecutingCpu, (VOID*)&rip);

    UINT64 kernel_entry = rip & ~(SIZE_2MB - 1);

    for (UINT16 i = 0; i < 0x30; i++)
    {
        UINT64 address = kernel_entry - (i * SIZE_2MB);
        UINT64 address2 = kernel_entry + (i * SIZE_2MB);

        UINT64 translated = TranslateVirtualToPhysical(tempcr3, address);
        UINT64 translated2 = TranslateVirtualToPhysical(tempcr3, address2);

        if (translated && *(UINT16*)translated == 23117)
        {
            *cr3 = tempcr3;
            return EFI_SUCCESS;
        }

        if (translated2 && *(UINT16*)translated2 == 23117)
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
    cpu->ReadSaveState(cpu, sizeof(cr3), EFI_SMM_SAVE_STATE_REGISTER_CR3, GSmst2->CurrentlyExecutingCpu, (VOID*)&cr3);
    cpu->ReadSaveState(cpu, sizeof(rip), EFI_SMM_SAVE_STATE_REGISTER_RIP, GSmst2->CurrentlyExecutingCpu, (VOID*)&rip);

    UINT64 kernel_entry = rip & ~(SIZE_2MB - 1);

    for (UINT16 i = 0; i < 0x30; i++)
    {
        UINT64 address = kernel_entry - (i * SIZE_2MB);
        UINT64 address2 = kernel_entry + (i * SIZE_2MB);

        UINT64 translated = TranslateVirtualToPhysical(cr3, address);
        UINT64 translated2 = TranslateVirtualToPhysical(cr3, address2);

        if (translated && *(UINT16*)translated == 23117)
        {
            *base = address;
            return EFI_SUCCESS;
        }

        if (translated2 && *(UINT16*)translated2 == 23117)
        {
            *base = address2;
            return EFI_SUCCESS;
        }
    }

    return EFI_NOT_FOUND;
}

UINT64 ScanNtosKernel(UINT64 cr3, UINT64 entry)
{
    UINT64 a3 = entry & ~(SIZE_2MB - 1);

    for (UINT16 i = 0; i < 0x30; i++)
    {
        UINT64 a1 = TranslateVirtualToPhysical(cr3, a3 + (i * SIZE_2MB));
        UINT64 a2 = TranslateVirtualToPhysical(cr3, a3 - (i * SIZE_2MB));
        if (a1 && *(UINT16*)a1 == 23117)
            // if (IsAddressValid(a1) && *(UINT16*)a1 == 23117 && *(UINT32*)(*(UINT32*)(a1 + 60) + a1) != 17744 && ZGetProcAddressX64(cr3, a3 + (i * SIZE_2MB), "NtClose"))
        {
            return a3 + (i * SIZE_2MB);
        }
        if (a2 && *(UINT16*)a2 == 23117)
        {
            return a3 - (i * SIZE_2MB);
        }
    }

    return 0;
}

EFI_STATUS SetupMemory(EFI_SMM_CPU_PROTOCOL* c, EFI_SMM_SYSTEM_TABLE2* g)
{
    if (setup_done == TRUE)
    {
        return EFI_SUCCESS;
    }

    if (c == NULL || g == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    cpu = c;
    GSmst2 = g;
    setup_done = TRUE;
    return EFI_SUCCESS;
}
