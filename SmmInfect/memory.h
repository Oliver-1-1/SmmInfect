#pragma once

#include <Uefi.h>
#include <Protocol/SmmBase2.h>
#include <PiSmm.h>
#include <Protocol/SmmCpu.h>

// #define Linux UNSUPPORTED

void* ZMemSet(void* ptr, int value, UINT64 num);
EFI_STATUS SetupMemoryMap();
BOOLEAN IsAddressValid(UINT64 address);
EFI_STATUS SetupMemory(EFI_SMM_SYSTEM_TABLE2* smst);
UINT64 TranslateVirtualToPhysical(UINT64 targetcr3, UINT64 address);
UINT64 TranslatePhysicalToVirtual(UINT64 targetcr3, UINT64 address);

UINT64 FindNearestCoffImage(UINT64 entry, UINT64 targetcr3);
void SetRwx(UINT64 address, UINT64 targetcr3);

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

typedef union
{
    struct
    {
        UINT64 reserved1 : 3;
        UINT64 page_level_write_through : 1;
        UINT64 page_level_cache_disable : 1;
        UINT64 reserved2 : 7;

        UINT64 address_of_page_directory : 36;
        UINT64 reserved3 : 16;
    }bits;

    UINT64 flags;
} cr3;

typedef union
{
    struct
    {
        UINT64 present : 1;
        UINT64 write : 1;
        UINT64 supervisor : 1;
        UINT64 page_level_write_through : 1;
        UINT64 page_level_cache_disable : 1;
        UINT64 accessed : 1;
        UINT64 reserved1 : 1;
        UINT64 must_be_zero : 1;
        UINT64 ignored_1 : 4;
        UINT64 page_frame_number : 36;
        UINT64 reserved2 : 4;
        UINT64 ignored_2 : 11;
        UINT64 execute_disable : 1;
    } bits;

    UINT64 flags;
} pml4e_64;

typedef union
{
    struct
    {
        UINT64 present : 1;
        UINT64 write : 1;
        UINT64 supervisor : 1;
        UINT64 page_level_write_through : 1;
        UINT64 page_level_cache_disable : 1;
        UINT64 accessed : 1;
        UINT64 reserved1 : 1;
        UINT64 large_page : 1;
        UINT64 ignored_1 : 4;
        UINT64 page_frame_number : 36;
        UINT64 reserved2 : 4;
        UINT64 ignored_2 : 11;
        UINT64 execute_disable : 1;
    } bits;

    UINT64 flags;
} pdpte_64;

typedef union
{
    struct
    {
        UINT64 present : 1;
        UINT64 write : 1;
        UINT64 supervisor : 1;
        UINT64 page_level_write_through : 1;
        UINT64 page_level_cache_disable : 1;
        UINT64 accessed : 1;
        UINT64 reserved1 : 1;
        UINT64 large_page : 1;
        UINT64 ignored_1 : 4;
        UINT64 page_frame_number : 36;
        UINT64 reserved2 : 4;
        UINT64 ignored_2 : 11;
        UINT64 execute_disable : 1;
    } bits;

    UINT64 flags;
} pde_64;

typedef union
{
    struct
    {
        UINT64 present : 1;
        UINT64 write : 1;
        UINT64 supervisor : 1;
        UINT64 page_level_write_through : 1;
        UINT64 page_level_cache_disable : 1;
        UINT64 accessed : 1;
        UINT64 dirty : 1;
        UINT64 pat : 1;
        UINT64 global : 1;
        UINT64 ignored_1 : 3;
        UINT64 page_frame_number : 36;
        UINT64 reserved1 : 4;
        UINT64 ignored_2 : 7;
        UINT64 protection_key : 4;
        UINT64 execute_disable : 1;
    } bits;
    UINT64 flags;
} pte_64;
