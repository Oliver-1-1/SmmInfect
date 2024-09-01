#pragma optimize("", off)

#include "memory.h"

UINT64 map_begin = 0x1000;
UINT64 map_end = 0;

UINT8 *ReadPhysical(UINT64 address, UINT8 *buffer, UINT64 length)
{
  if (address == 0)
    return NULL;

  for (UINT64 i = 0; i < length; i++)
  {
    buffer[i] = *(UINT8 *)(address + i);
  }

  return buffer;
}

UINT8 ReadPhysical8(UINT64 address)
{
  return !address ? 0 : *(UINT8 *)address;
}

UINT16 ReadPhysical16(UINT64 address)
{
  return !address ? 0 : *(UINT16 *)address;
}

UINT32 ReadPhysical32(UINT64 address)
{
  return !address ? 0 : *(UINT32 *)address;
}

UINT64 ReadPhysical64(UINT64 address)
{
  return !address ? 0 : *(UINT64 *)address;
}

void *ZMemSet(void *ptr, int value, UINT64 num)
{
  unsigned char *p = (unsigned char *)ptr;
  while (num--)
  {
    *p++ = (unsigned char)value;
  }
  return ptr;
}

// Clamp the virtual read to bounderies of page size.
UINT8 *ReadVirtual(UINT64 address, UINT64 cr3, UINT8 *buffer, UINT64 length)
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
  ReadVirtual(address, cr3, (UINT8 *)&value, sizeof(value));
  return value;
}

UINT32 ReadVirtual32(UINT64 address, UINT64 cr3)
{
  UINT32 value = 0;
  ReadVirtual(address, cr3, (UINT8 *)&value, sizeof(value));
  return value;
}
UINT64 ReadVirtual64(UINT64 address, UINT64 cr3)
{
  UINT64 value = 0;
  ReadVirtual(address, cr3, (UINT8 *)&value, sizeof(value));
  return value;
}

#ifdef Windows
STATIC BOOLEAN CheckLow(UINT64 *pml4, UINT64 *kernel_entry);

EFI_STATUS SetupMemoryMap()
{
  UINT32 descriptor_version;
  UINTN memory_map_size = 0;
  EFI_MEMORY_DESCRIPTOR *memory_map = NULL;
  UINTN map_key;
  UINTN descriptor_size;
  EFI_MEMORY_DESCRIPTOR *last;
  EFI_STATUS status;
  UINT64 end;

  status = gBS->GetMemoryMap(&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
  if (status == EFI_INVALID_PARAMETER || status == EFI_SUCCESS)
  {
    return EFI_ACCESS_DENIED;
  }

  status = gBS->AllocatePool(EfiBootServicesData, memory_map_size, (VOID **)&memory_map);

  if (EFI_ERROR(status))
  {
    return status;
  }

  status = gBS->GetMemoryMap(&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
  if (EFI_ERROR(status))
  {
    gBS->FreePool(memory_map);
  }

  last = (EFI_MEMORY_DESCRIPTOR *)((INT8 *)memory_map + (((UINT32)memory_map_size / (UINT32)descriptor_size) * descriptor_size));
  end = last->PhysicalStart + (last->NumberOfPages * 0x1000);
  gBS->FreePool(memory_map);
  map_end = end;
  return EFI_SUCCESS;
}

BOOLEAN IsAddressValid(UINT64 address)
{
  return !(address < map_begin || address > map_end);
}

EFI_STATUS MemGetKernelCr3(UINT64 *cr3)
{
  if(cr3 == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  UINT64 pml4 = 0;
  UINT64 entry = 0;
  CheckLow(&pml4, &entry);

  *cr3 = pml4;

  return EFI_SUCCESS;
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

// credits rain / hermes.(i belive its h33p's code actually) project for finding ntoskrnl and cr3.
BOOLEAN p_memCpy(UINT64 dest, UINT64 src, UINTN n, BOOLEAN verbose)
{
  if ((IsAddressValid((UINT64)src) == FALSE || IsAddressValid((UINT64)(src + n - 1)) == FALSE))
  {
    return FALSE;
  }

  CHAR8 *csrc = (char *)src;
  CHAR8 *cdest = (char *)dest;

  for (INT32 i = 0; i < n; i++)
    cdest[i] = csrc[i];

  return TRUE;
}

STATIC BOOLEAN CheckLow(UINT64 *pml4, UINT64 *kernel_entry)
{
  UINT64 o = 0;
  while (o < 0x100000)
  {
    o += 0x1000;

    if (IsAddressValid(o) == TRUE)
    {
      if (0x00000001000600E9 != (0xffffffffffff00ff & *(UINT64 *)(void *)(o + 0x000)))
      {
        continue;
      } 
      if (0xfffff80000000000 != (0xfffff80000000003 & *(UINT64 *)(void *)(o + 0x070)))
      {
        continue;
      } 
      if (0xffffff0000000fff & *(UINT64 *)(void *)(o + 0x0a0))
      {
        continue;
      } 

      p_memCpy((UINT64)pml4, (UINT64)o + 0xa0, 8, FALSE);
      p_memCpy((UINT64)kernel_entry, (UINT64)o + 0x70, 8, FALSE);

      return TRUE;
    }
  }
  return FALSE;
}

EFI_STATUS MemGetKernelBase(UINT64 *base)
{
  if (base == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  UINT64 cr3 = 0;
  UINT64 kernel_entry = 0;
  CheckLow(&cr3, &kernel_entry);

  UINT64 physical_first = 0;
  physical_first = TranslateVirtualToPhysical(cr3, kernel_entry & 0xFFFFFFFFFF000000);

  if (IsAddressValid(physical_first) == TRUE && physical_first != 0)
  {
    if (((kernel_entry & 0xFFFFFFFFFF000000) & 0xfffff) == 0 && *(INT16 *)(VOID *)(physical_first) == 0x5a4d)
    {
      INT32 kdbg = 0, pool_code = 0;
      for (INT32 u = 0; u < 0x1000; u++)
      {
        kdbg = kdbg || *(UINT64 *)(VOID *)(physical_first + u) == 0x4742444b54494e49;
        pool_code = pool_code || *(UINT64 *)(VOID *)(physical_first + u) == 0x45444f434c4f4f50;
        if (kdbg & pool_code)
        {
          *base = kernel_entry & 0xFFFFFFFFFF000000;
          return EFI_SUCCESS;
        }
      }
    }
  }

  UINT64 physical_sec = 0;
  physical_sec = TranslateVirtualToPhysical(cr3, (kernel_entry & 0xFFFFFFFFFF000000) + 0x2000000);

  if (IsAddressValid(physical_sec) == TRUE && physical_sec != 0)
  {
    if ((((kernel_entry & 0xFFFFFFFFFF000000) + 0x2000000) & 0xfffff) == 0 && *(INT16 *)(VOID *)(physical_sec) == 0x5a4d)
    {
      INT32 kdbg = 0, pool_code = 0;
      for (INT32 u = 0; u < 0x1000; u++)
      {
        kdbg = kdbg || *(UINT64 *)(VOID *)(physical_sec + u) == 0x4742444b54494e49;
        pool_code = pool_code || *(UINT64 *)(VOID *)(physical_sec + u) == 0x45444f434c4f4f50;
        if (kdbg & pool_code)
        {
          *base = (kernel_entry & 0xFFFFFFFFFF000000) + 0x2000000;
          return EFI_SUCCESS;
        }
      }
    }
  }

  UINT64 i, p, u, mask = 0xfffff;

  while (mask >= 0xfff)
  {
    for (i = (kernel_entry & ~0x1fffff) + 0x10000000; i > kernel_entry - 0x20000000; i -= 0x200000)
    {
      for (p = 0; p < 0x200000; p += 0x1000)
      {

        UINT64 physical_p = 0;
        physical_p = TranslateVirtualToPhysical(cr3, i + p);

        if (IsAddressValid(physical_p) == TRUE && physical_p != 0)
        {
          if (((i + p) & mask) == 0 && *(INT16 *)(VOID *)(physical_p) == 0x5a4d)
          {
            INT32 kdbg = 0, poolCode = 0;
            for (u = 0; u < 0x1000; u++)
            {
              if (IsAddressValid(p + u) == FALSE)
                continue;

              kdbg = kdbg || *(UINT64 *)(VOID *)(physical_p + u) == 0x4742444b54494e49;
              poolCode = poolCode || *(UINT64 *)(VOID *)(physical_p + u) == 0x45444f434c4f4f50;
              if (kdbg & poolCode)
              {
                *base = i + p;
                return EFI_SUCCESS;
              }
            }
          }
        }
      }
    }

    mask = mask >> 4;
  }
  return EFI_NOT_FOUND;
}

#endif
#pragma optimize("", on)
