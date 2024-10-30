/*
UINT8 patch[] = { 0x88, 0x90, 0xEB, 0xFE }; // 0xC3

VOID EFIAPI NotifySetVirtualAddressMap(EFI_EVENT Event, VOID* Context)
{
    for(;;){}
}

typedef struct _IMAGE_SECTION_HEADER {
    UINT8  Name[8];
    union {
        UINT32 PhysicalAddress;
        UINT32 VirtualSize;
    } Misc;
    UINT32 VirtualAddress;
    UINT32 SizeOfRawData;
    UINT32 PointerToRawData;
    UINT32 PointerToRelocations;
    UINT32 PointerToLinenumbers;
    UINT16  NumberOfRelocations;
    UINT16  NumberOfLinenumbers;
    UINT32 Characteristics;
} IMAGE_SECTION_HEADER, * PIMAGE_SECTION_HEADER;

BOOLEAN IsRuntimeImage(UINT64 base)
{
    if (*(UINT16*)(base) != 23117)
    {
        return FALSE;
    }

    UINT32 v4 = *(UINT32*)(base + 60);
    if (*(UINT32*)(v4 + base) != 17744)
    {
        return FALSE;
    }

    if (*(UINT32*)(base + v4 + 0x5C) == 0xC)
    {
        return TRUE;
    }
    return FALSE;
}

UINT64 FindTextPadding(UINT64 MinimumPadding)
{
    EFI_STATUS Status;
    UINTN HandleCount = 0;
    EFI_HANDLE* HandleBuffer = NULL;

    Status = gBS->LocateHandleBuffer(AllHandles, NULL, NULL, &HandleCount, &HandleBuffer);

    if (EFI_ERROR(Status)) 
    {
        Print(L"locating handles error: %r\n", Status);
        return 0;
    }

    for (UINTN i = 0; i < HandleCount; i++) 
    {
        EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;

        Status = gBS->HandleProtocol(HandleBuffer[i], &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);

        if (!EFI_ERROR(Status) && LoadedImage != NULL) 
        {

            if (IsRuntimeImage(LoadedImage->ImageBase))
            {
                UINT32 v4 = *(UINT32*)((UINT64)LoadedImage->ImageBase + 60);
                UINT16 v1 = *(UINT16*)(v4 + (UINT64)LoadedImage->ImageBase + 6);

                PIMAGE_SECTION_HEADER sec = (v4 + (UINT64)LoadedImage->ImageBase + 4) + 20 + *(UINT16*)(v4 + (UINT64)LoadedImage->ImageBase + 0x14);

                for (int j = 0; j < v1; j++, sec++)
                {
                    if (!AsciiStrCmp(sec->Name, ".text"))
                    {
                        Print(L".text\n");
                        UINT64 End = (UINT64)LoadedImage->ImageBase + sec->VirtualAddress + sec->SizeOfRawData;

                        // Walk down memory to search for padding :)
                        UINT64 Count = 0;
                        while (*(UINT8*)(End) == 0)
                        {
                            Count++;
                            End++;
                        }

                        if (Count > MinimumPadding)
                        {
                            Print(L"Found padding!\n");
                            return End;
                        }

                    }
                }

            }

        }
    }
    if (HandleBuffer != NULL) 
    {
        FreePool(HandleBuffer);
    }
    return 0;
}

EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable)
{
    Print(L"Handle is %lx  System Table is at %p\n", ImageHandle, SystemTable);

    UINT64 padding = FindTextPadding(500);

    padding += 0x10;

    Print(L"Padding Address %x: \n", padding);

    UINT64 cr0 = AsmReadCr0();
    AsmWriteCr0(cr0 & ~0x10000ull);

    CopyMem(padding, patch, sizeof(patch));

    AsmWriteCr0(cr0);

    EFI_EVENT event;
    
    gBS->CreateEvent(EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE, TPL_NOTIFY, padding, NULL, &event);

    return EFI_SUCCESS;
}
*/