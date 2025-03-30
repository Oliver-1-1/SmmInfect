#include <Uefi.h>
#include <Protocol/SmmBase2.h>
#include <PiSmm.h>
#include <Protocol/SmmCpu.h>


// Search for the unique smm entry stub that every core enteres on smm
EFI_STATUS HookRendezvous(EFI_SMM_SYSTEM_TABLE2* smst, UINT64 handler);

