#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <cstring>

//
// Linux is does NOT yet have a supported SMM module!
//

#pragma pack(1)
typedef struct _SmmCommunicationProtocol
{
    uint8_t magic = 'i';
    uint8_t process_name[30] = {0};
    uint64_t offset = 0;
    uint64_t read_size = 0;
    uint8_t read_buffer[30] = {0};
    uint64_t smi_count = 2976579765;
} SmmCommunicationProtocol, *PSmmCommunicationProtocol;
#pragma pack()

volatile SmmCommunicationProtocol protocol __attribute__((section(".ZEPTA")));

void trigger_smi()
{
    system("efivar --write --name 8be4df61-93ca-11d2-aa0d-00e098032b8c-Zepta --datafile val.bin --attributes 0x7");
}

int main()
{
    while(true)
    {
        strcpy((char*)protocol.process_name, "X");
        protocol.offset = 0;
        protocol.read_size = 15;
        memset((void*)protocol.read_buffer, 0, sizeof(protocol.read_buffer));

        trigger_smi();

        printf("Smi count: %lu\n", protocol.smi_count);

        for(int i = 0; i < protocol.read_size; i++)
        {
            printf("%02X", protocol.read_buffer[i]);
        }
        usleep(50);
    }

    return 0;
}