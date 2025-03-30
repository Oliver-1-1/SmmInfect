#include "rendezvous.h"
#include <Uefi.h>
#include <Protocol/SmmBase2.h>
#include <PiSmm.h>
#include <Protocol/SmmCpu.h>

BOOLEAN hooked = FALSE;

// The stub pattern, should be same for a lot of amd processors. See edk2 source
unsigned char stubpattern[] = {
  0x48, 0xB8, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
  0xFF, 0xD0, 0x48, 0x89, 0xD9, 0x48, 0xB8, 0x69, 0x69, 0x69,
  0x69, 0x69, 0x69, 0x69, 0x69, 0xFF, 0xD0, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x0F, 0xAE, 0x0C, 0x24,
  0x48, 0x81,0xC4, 0x00, 0x02, 0x00, 0x00, 0x48, 0xB8
};

EFI_STATUS HookRendezvous(EFI_SMM_SYSTEM_TABLE2* smst, UINT64 handler)
{
  if (!hooked)
  {

    UINT64 base = ((UINT64)smst->CpuSaveState[0]) - 0xFE00;

    BOOLEAN found = FALSE;
    UINT64 temp = base;

    for (UINT64 i = 0x0; i < 0x100000; i++)
    {
      for (UINT64 j = 0; j < sizeof(stubpattern); j++)
      {
        if (pattern[j] == 0x69) // wildcard
        {
          found = TRUE;
          continue;
        }
        UINT8 byte = ReadPhysical8(temp + j);

        if (byte != pattern[j])
        {
          found = FALSE;
          break;
        }
        else
        {
          found = TRUE;
        }

      }
      if (found == TRUE)
      {

        UINT8* rendezvous = (UINT8*)*(UINT64*)(temp + 2);

        rendezvous[0] = 0xFF;
        rendezvous[1] = 0x25;
        rendezvous[2] = 0x00;
        rendezvous[3] = 0x00;
        rendezvous[4] = 0x00;
        rendezvous[5] = 0x00;

        *(UINT64*)(rendezvous + 6) = (UINT64)handler;

        hooked = TRUE;
        return EFI_SUCCESS;
      }


      temp++;
    }
  }

  return EFI_NOT_FOUND;
}
