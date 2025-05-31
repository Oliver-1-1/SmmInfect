#include "memory.h"
#include "windows.h"
#include "serial.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>

static UINT64 map_begin = 0;
static UINT64 map_end = 0;
static BOOLEAN setup_done = FALSE;
static EFI_SMM_SYSTEM_TABLE2* GSmst2;
static EFI_PHYSICAL_ADDRESS pml4_phys, pdpt_phys, pd_phys, pt_phys;
BOOLEAN MapPhysicalMemory(UINT64 address);

UINT8* ReadPhysical(UINT64 address, UINT8* buffer, UINT64 length)
{
    if (!IsAddressValid(address))
        return NULL;

    if(!MapPhysicalMemory(address))
    {
      return NULL;
    }

    for (UINT64 i = 0; i < length; ++i)
    {
        buffer[i] = *(UINT8*)(address + i);
    }

    return buffer;
}

UINT8 ReadPhysical8(UINT64 address)
{
    return !IsAddressValid(address) ? 0 : *(UINT8*)address;
}

UINT16 ReadPhysical16(UINT64 address)
{
    return !IsAddressValid(address) ? 0 : *(UINT16*)address;
}

UINT32 ReadPhysical32(UINT64 address)
{
    return !IsAddressValid(address) ? 0 : *(UINT32*)address;
}

UINT64 ReadPhysical64(UINT64 address)
{
    return !IsAddressValid(address) ? 0 : *(UINT64*)address;
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

void ZMemCpy(UINT8* src, UINT8* dst, UINT64 len)
{
  for(UINT64 i = 0; i < len; ++i)
  {
    dst[i] = src[i];
  }
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
    EFI_STATUS status;

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

    UINT64 max_end = 0;
    EFI_MEMORY_DESCRIPTOR* desc = memory_map;

    for (UINT64 i = 0; i < memory_map_size / descriptor_size; ++i) 
    {
        UINT64 region_end = desc->PhysicalStart + (desc->NumberOfPages * 0x1000);
        if (region_end > max_end) 
        {
            max_end = region_end;
        }

        desc = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)desc + descriptor_size);
    }

    gBS->FreePool(memory_map);
    map_end = max_end;
    return EFI_SUCCESS;
}

BOOLEAN IsAddressValid(UINT64 address)
{
    return !(address < map_begin || address > map_end);
}

UINT64 TranslateVirtualToPhysical(UINT64 targetcr3, UINT64 address)
{
    UINT16 pml4_idx = ((UINT64)address >> 39) & 0x1FF;
    UINT16 pdpt_idx = ((UINT64)address >> 30) & 0x1FF;
    UINT16 pd_idx   = ((UINT64)address >> 21) & 0x1FF;
    UINT16 pt_idx   = ((UINT64)address >> 12) & 0x1FF;

    pml4e_64* pml4_arr = (pml4e_64*)(targetcr3 + 8 * pml4_idx);
    if(!pml4_arr->bits.present) return 0;
    pdpte_64* pdpt_arr = (pdpte_64*)((pml4_arr->bits.page_frame_number << 12) + 8 * pdpt_idx);
    if(!pdpt_arr->bits.present) return 0;
    pde_64* pd_arr = (pde_64*)((pdpt_arr->bits.page_frame_number << 12) + 8 * pd_idx);
    if(!pd_arr->bits.present) return 0;
    if(pd_arr->bits.large_page) return ((UINT64)(address & 0x1FFFFF) + (*(UINT64*)pd_arr & 0xFFFFFFFFF000));
    pte_64* pt_arr = (pte_64*)((pd_arr->bits.page_frame_number << 12) + 8 * pt_idx);
    if(!pt_arr->bits.present) return 0;
    return (address & 0xFFF) + (*(UINT64*)pt_arr & 0xFFFFFFFFF000);
}

//From SmmInfect fork: https://github.com/r1cky33/SmmInfect/blob/85a2c170e0a8bed8fffb7d4f9ce766f602a070b6/SmmInfect/memory.c#L428
BOOLEAN MapPhysicalMemory(UINT64 address)
{
    UINT64 PMASK =  (~0xfull << 8) & 0xfffffffffull
    address &= PMASK;

    UINT16 pml4_idx = ((UINT64)address >> 39) & 0x1FF;
    UINT16 pdpt_idx = ((UINT64)address >> 30) & 0x1FF;
    UINT16 pd_idx   = ((UINT64)address >> 21) & 0x1FF;
    UINT16 pt_idx   = ((UINT64)address >> 12) & 0x1FF;
    UINT64* pml4    = (UINT64*)AsmReadCr3();

    if (!(pml4[pml4_idx] & 0x1)) 
    {
        EFI_PHYSICAL_ADDRESS pdpt;
        GSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &pdpt);
        ZMemSet((void*)pdpt, 0, SIZE_4KB);
        pml4[pml4_idx] = pdpt | 0x3;
    }

    UINT64* pdpt = (UINT64*)(pml4[pml4_idx] & PMASK);

    if (!(pdpt[pdpt_idx] & 0x1)) 
    {
        EFI_PHYSICAL_ADDRESS pd;
        GSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &pd);
        ZMemSet((void*)pd, 0, SIZE_4KB);
        pdpt[pdpt_idx] = pd | 0x3;
    } 
    else 
    {
        if (pdpt[pdpt_idx] & 0x80) 
            return TRUE; 
    }

    UINT64* pd = (UINT64*)(pdpt[pdpt_idx] & PMASK);

    if (!(pd[pd_idx] & 0x1)) {
        EFI_PHYSICAL_ADDRESS pt;
        GSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData, 1, &pt);

        ZMemSet((void*)pt, 0, SIZE_4KB);
        pd[pd_idx] = pt | 0x3;
    } 
    else 
    {
        if (pd[pd_idx] & 0x80)
            return TRUE;
    }

    UINT64* pte = (UINT64*)(pd[pd_idx] & PMASK);

    if (!(pte[pt_idx] & 0x1)) {
        pte[pt_idx] = address | 0x3;
    } 

    return TRUE;
}

//This function does not currently work. It works on linx and windows but when porting to smm it fails.
UINT64 TranslatePhysicalToVirtual(UINT64 targetcr3, UINT64 address)
{
    if(targetcr3 == AsmReadCr3()) return 0;

    cr3 ctx;

    ctx.flags = targetcr3;
    UINT64 physical_addr = ctx.bits.address_of_page_directory << 12;

    pml4e_64* pml4 = (pml4e_64*)pml4_phys;
    pdpte_64* pdpt = (pdpte_64*)pdpt_phys;
    pde_64* pd = (pde_64*)pd_phys;
    pte_64* pt = (pte_64*)pt_phys;

    ReadPhysical(physical_addr, (UINT8*)pml4, 512 * sizeof(pml4e_64));

    for (UINT64 pml4_i = 0; pml4_i < 512; pml4_i++) {
        if (!pml4[pml4_i].bits.present) continue;

        physical_addr = pml4[pml4_i].bits.page_frame_number << 12;
        ReadPhysical(physical_addr, (UINT8*)pdpt, 512 * sizeof(pdpte_64));

        for (UINT64 pdpt_i = 0; pdpt_i < 512; pdpt_i++) {
            if (!pdpt[pdpt_i].bits.present || pdpt[pdpt_i].bits.large_page) continue;

            physical_addr = pdpt[pdpt_i].bits.page_frame_number << 12;
            ReadPhysical(physical_addr, (UINT8*)pd, 512 * sizeof(pde_64));

            for (UINT64 pde_i = 0; pde_i < 512; pde_i++) {
                if (!pd[pde_i].bits.present || pd[pde_i].bits.large_page) continue;

                physical_addr = pd[pde_i].bits.page_frame_number << 12;
                ReadPhysical(physical_addr, (UINT8*)pt, 512 * sizeof(pte_64));

                for (UINT64 pte_i = 0; pte_i < 512; pte_i++) {
                    if (!pt[pte_i].bits.present) continue;

                    UINT64 page_phys = pt[pte_i].bits.page_frame_number << 12;
                    UINT64 virtual = (pml4_i << 39) | (pdpt_i << 30) | (pde_i << 21) | (pte_i << 12);

                    if (virtual & (1ull << 47)) virtual |= 0xFFFF000000000000ull;

                    if ((address & ~0xFFF) == (page_phys & ~0xFFF)) {
                        SERIAL_PRINT("Translation worked!\r\n");

                        return virtual + (address & 0xFFF);
                    }
                }
            }
        }
    }

    return 0;
}


void SetRwx(UINT64 address, UINT64 targetcr3)
{
  UINT16 pml4_idx = ((UINT64)address >> 39) & 0x1FF;
  UINT16 pdpt_idx = ((UINT64)address >> 30) & 0x1FF;
  UINT16 pd_idx   = ((UINT64)address >> 21) & 0x1FF;
  UINT16 pt_idx   = ((UINT64)address >> 12) & 0x1FF;

  pml4e_64* pml4_arr = (pml4e_64*)(targetcr3 + 8 * pml4_idx);
  if(!pml4_arr->bits.present) return;
  pdpte_64* pdpt_arr = (pdpte_64*)((pml4_arr->bits.page_frame_number << 12) + 8 * pdpt_idx);
  if(!pdpt_arr->bits.present) return;
  pde_64* pd_arr = (pde_64*)((pdpt_arr->bits.page_frame_number << 12) + 8 * pd_idx);
  if(!pd_arr->bits.present) return;
  if(pd_arr->bits.large_page)
  {
    pd_arr->bits.write = 1;
    pd_arr->bits.execute_disable = 0;
    return;
  } 

  pte_64* pt_arr = (pte_64*)((pd_arr->bits.page_frame_number << 12) + 8 * pt_idx);
  if(!pt_arr->bits.present) return;
  pt_arr->bits.write = 1;
  pt_arr->bits.execute_disable = 0;
}


EFI_STATUS SetupMemory(EFI_SMM_SYSTEM_TABLE2* smst)
{
    if (setup_done == TRUE)
    {
        return EFI_SUCCESS;
    }

    if (smst == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }
    GSmst2 = smst;

    if(SetupMemoryMap() != EFI_SUCCESS)
    {
      return EFI_NOT_FOUND;
    }

    EFI_STATUS status;
    status = GSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData,
                                      EFI_SIZE_TO_PAGES(512 * sizeof(pml4e_64)), &pml4_phys);
    if (status != EFI_SUCCESS) {
        SERIAL_PRINT("Failed alloc pml4: %llx\r\n", status);
        while(1){}; // Something is wrong outside of our control. Make pc not startable
    }

    status = GSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData,
                                      EFI_SIZE_TO_PAGES(512 * sizeof(pdpte_64)), &pdpt_phys);
    if (status != EFI_SUCCESS) {
        SERIAL_PRINT("Failed alloc pdpt: %llx\r\n", status);
        while(1){}; // Something is wrong outside of our control. Make pc not startable
    }

    status = GSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData,
                                      EFI_SIZE_TO_PAGES(512 * sizeof(pde_64)), &pd_phys);
    if (status != EFI_SUCCESS) {
        SERIAL_PRINT("Failed alloc pd: %llx\r\n", status);
        while(1){}; // Something is wrong outside of our control. Make pc not startable
    }

    status = GSmst2->SmmAllocatePages(AllocateAnyPages, EfiRuntimeServicesData,
                                      EFI_SIZE_TO_PAGES(512 * sizeof(pte_64)), &pt_phys);
    if (status != EFI_SUCCESS) {
        SERIAL_PRINT("Failed alloc pte: %llx\r\n", status);
        while(1){}; // Something is wrong outside of our control. Make pc not startable
    }
    
    setup_done = TRUE;

    return EFI_SUCCESS;
}

UINT64 FindNearestCoffImage(UINT64 entry, UINT64 targetcr3)
{
  entry = entry & ~(SIZE_4KB -1);

  UINTN i = 0;
  for(i = 0; i < 500; ++i)
  {
    UINT16 magic = ReadVirtual16(entry - (SIZE_4KB * i), targetcr3);

    if(magic == 0x5A4D)
    {
      return (entry - (SIZE_4KB * i));
    }
  }

  return 0;
}

UINT64 FindNearestCoffImagePhys(UINT64 entry)
{
  entry = entry & ~(SIZE_4KB -1);

  UINTN i = 0;
  for(i = 0; i < 500; ++i)
  {
    UINT16 magic = ReadPhysical16(entry + (SIZE_4KB * i));

    if(magic == 0x5A4D)
    {
      return (entry + (SIZE_4KB * i));
    }
  }

  return 0;
}
