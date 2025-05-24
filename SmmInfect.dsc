[Defines]
    PLATFORM_NAME = SmmInfect
    PLATFORM_GUID = 07886e23-b344-468f-8a36-b91478fffacf
    PLATFORM_VERSION = 1.000
    DSC_SPECIFICATION = 0x0001001B
    OUTPUT_DIRECTORY = Build/SmmInfect
    SUPPORTED_ARCHITECTURES = X64
    BUILD_TARGETS = RELEASE
    SKUID_IDENTIFIER = DEFAULT

[LibraryClasses.common]
    !include MdePkg/MdeLibs.dsc.inc

    BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
    BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
    DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLibOptionalDevicePathProtocol.inf
    MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
    UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
    UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
    UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
    DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
    SmmPeriodicSmiLib|MdePkg/Library/SmmPeriodicSmiLib/SmmPeriodicSmiLib.inf
    TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
    HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
    MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
    SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
    SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
    SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf

[Components]
    SmmInfect-main/SmmInfect/SmmInfect.inf
