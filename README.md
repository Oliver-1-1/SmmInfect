## **About**

The project aims to bring the capabilities of SMM x86-64(System Management Mode) to usermode through a backdoor. The backdoor is triggered through a syscall that invokes an SMI on most systems.
This way, you can easily control the number of times the backdoor is triggered. The project consists of 2 modules. 
One SMM driver that has a registered SMI root handler and a normal usermode application that invokes the SMI's and dictates what information to be transceived.

## **Requirements**

x86-64 processor.

Windows 10/11. Linux is currently unsupported.

Bios that supports [UEFI SMM variables](https://github.com/tianocore/tianocore.github.io/wiki/UEFI-Variable-Runtime-Cache)

Ability to flash bios.


## **Compiling and Installation**

**Compiling**

To compile the SMM driver ```SmmInfect.efi``` you will need the [edk2](https://github.com/tianocore/edk2) project and if you are on Windows you will also need [Visual studio build tools](https://stackoverflow.com/questions/40504552/how-to-install-visual-c-build-tools).
If you are unable to setup the edk2 project on Windows here is a good [tutorial](https://www.youtube.com/watch?v=jrY4oqgHV0o).

  * Place the github project inside edk2 project ```edk2/SmmInfect```

  * Edit ACTIVE_PLATFORM | TARGET | TARGET_ARCH inside ```edk2/Conf/target.txt``` to SmmInfect/SmmInfect.dsc | RELEASE | X64

  * Open ```Developer command promopt for VS 2022/2019``` and enter ```edksetup.bat``` then ```build```

To compile the usermode program just open the visual studio solution (SmiUm folder) and build it as Release x64.

If you are unable to compile there are pre-compiled binaries.

**Installation**

Download and install [UEFITool 0.28.0](https://github.com/LongSoft/UEFITool/releases/tag/0.28.0).
Open UefiTool and go to File → Open image file. Choose the bios file you would like to patch.
Now choose an appropriate SMM module to patch out. Replace this SMM module with SmmInfect.efi.
![path: ](https://i.imgur.com/pb6r0Mu.png "patch: ")

Now save this bios file and flash your bios with it.
Then open up the usermode program to read the first 15 bytes of explorer.exe. 

To find an appropriate SMM module to patch you would either have to reverse it to see if it's not necessary for operating the computer. Or insert your one.
I am using ASUS Prime b650-plus with AMD ryzen 7 7800X3D and patching out this guid: {CE1FD2EA-F80C-4517-8A75-9F7794A1401E}

## **Credits**
Ekknod, for being a good mentor. Check out his [projects](https://github.com/ekknod)
