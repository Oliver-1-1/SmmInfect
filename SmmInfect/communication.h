#pragma once 
#include <Uefi.h>
#include <PiSmm.h>
#include <Protocol/SmmCpu.h>
#define SMM_PROTOCOL_MAGIC 'i'
#define MAGIC_OFFSET 0
#define PROCESS_OFFSET 1
#define MODULE_OFFSET 31
#define MEMORY_OFFSET 91
#define READ_SIZE_OFFSET 99
#define READ_BUFFER_OFFSET 107
#define SMI_COUNT_OFFSET 137


#pragma pack(1) // Ensure that alignment is 1-byte
typedef struct _SmmCommunicationProtocol
{
    UINT8 magic; // offset 0
    UINT8 process_name[30]; // offset 1
    UINT16 module_name[30]; // offset 31
    UINT64 offset; // offset 91
    UINT64 read_size; // offset 99
    UINT8 read_buffer[30]; // offset 107
    UINT64 smi_count; // offset 137
} SmmCommunicationProtocol, * PSmmCommunicationProtocol; // size 145
#pragma pack() // reset

UINT64 GetCommunicationProcess();
EFI_STATUS PerformCommunication();
