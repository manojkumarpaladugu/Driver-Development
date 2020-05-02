#include <iostream>
#include <iomanip>
#include <vector>
#include <Windows.h>
#include <Cfgmgr32.h>
#include <regstr.h>
#include "..\HardwareInterfaceLib\HardwareInterfaceLib.h"

#define PCI_STD_CFG_SIZE 256

#pragma pack(1)

typedef struct {
    std::string DeviceName;
    UINT8 Bus;
    UINT8 Device;
    UINT8 Function;
}PCI_PCIeDevice;

void Dump256BytesPCIConfigSpace(CHardwareInterfaceLib& CHWLib, std::vector<PCI_PCIeDevice> PCIDevices);
void Dump4KBytesPCIConfigSpace(CHardwareInterfaceLib& CHWLib, std::vector<PCI_PCIeDevice> PCIeDevices);
UserStatus GetPCIPCIeDevices(std::vector<PCI_PCIeDevice>& PCIPCIeDevices);

int main()
{
    UserStatus userStatus = Success;
    std::vector<PCI_PCIeDevice> PCIPCIeDevices;

    userStatus = GetPCIPCIeDevices(PCIPCIeDevices);
    if (userStatus != Success) {
        std::cout << "GetPCIDevices failed, status: 0x" << std::hex << userStatus << std::endl;
        return 1;
    }

    CHardwareInterfaceLib CHWLib;
    userStatus = CHWLib.CHardwareInterfaceLibInitialise();
    if (userStatus != Success)
    {
        std::cout << "CHardwareInterfaceLibInitialise failed, Error: " << CHWLib.GetStatusMessage() << std::endl;
        return 1;
    }

    //
    // Dump all  256 bytes config space of all PCI devices
    //
    Dump256BytesPCIConfigSpace(CHWLib, PCIPCIeDevices);

    //
    // Dump all 4 KB config space of all PCIe devices
    //
    Dump4KBytesPCIConfigSpace(CHWLib, PCIPCIeDevices);


    userStatus = CHWLib.CHardwareInterfaceLibUninitialise();
    if (userStatus != Success)
    {
        std::cout << "CHardwareInterfaceLibUninitialise failed, Error: " << CHWLib.GetStatusMessage() << std::endl;
        return 1;
    }
}

void Dump256BytesPCIConfigSpace(CHardwareInterfaceLib& CHWLib, std::vector<PCI_PCIeDevice> PCIDevices)
{
    UserStatus userStatus = Success;

    //
    // Dump all  256 bytes config space of all PCI devices
    //
    for (auto Device = PCIDevices.begin(); Device != PCIDevices.end(); ++Device)
    {
        std::cout << "Device: " << Device->DeviceName << ", " << "Bus: 0x" << std::hex << +(Device->Bus) << ", " << "Device: 0x" << std::hex << +(Device->Device) << ", "
            << "Function: 0x" << std::hex << +(Device->Function) << std::endl;

        PCI_PCIeCfgData pciStdData;
        pciStdData.m_Bus = Device->Bus;
        pciStdData.m_Device = Device->Device;
        pciStdData.m_Function = Device->Function;
        pciStdData.m_Offset = 0;
        pciStdData.OutputData.m_Size = PCI_STD_CFG_SIZE;
        pciStdData.OutputData.DataPointer = (PUINT8)calloc(1, pciStdData.OutputData.m_Size);
        if (pciStdData.OutputData.DataPointer == NULL) {
            std::cout << "Memory allocation failed" << std::endl;
            return;
        }

        userStatus = CHWLib.PCIStdCfgRead(&pciStdData);
        if (userStatus != Success) {
            if (pciStdData.OutputData.DataPointer) {
                free(pciStdData.OutputData.DataPointer);
            }
            std::cout << "PCIStdCfgRead failed, Error: " << CHWLib.GetStatusMessage() << std::endl;
            std::cout << std::endl << std::string(100, '*') << std::endl << std::endl;
            continue;
        }

        std::cout << "   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F" << std::endl;
        std::cout << "-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --" << std::endl;
        for (UINT32 RowIndex = 0; RowIndex < pciStdData.OutputData.m_Size; RowIndex += 0x10)
        {
            std::cout << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << RowIndex << "|";
            for (UINT32 ByteIndex = RowIndex; ByteIndex < RowIndex + 0x10; ByteIndex++)
            {
                std::cout << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << +(pciStdData.OutputData.DataPointer[ByteIndex]) << " ";
            }
            std::cout << std::endl;
        }

        if (pciStdData.OutputData.DataPointer) {
            free(pciStdData.OutputData.DataPointer);
        }
        std::cout << std::endl << std::string(100, '*') << std::endl << std::endl;
    }
}

void Dump4KBytesPCIConfigSpace(CHardwareInterfaceLib& CHWLib, std::vector<PCI_PCIeDevice> PCIeDevices)
{
    UserStatus userStatus = Success;

    //
    // Dump all  256 bytes config space of all PCI devices
    //
    for (auto Device = PCIeDevices.begin(); Device != PCIeDevices.end(); ++Device)
    {
        std::cout << "Device: " << Device->DeviceName << ", " << "Bus: 0x" << std::hex << +(Device->Bus) << ", " << "Device: 0x" << std::hex << +(Device->Device) << ", "
            << "Function: 0x" << std::hex << +(Device->Function) << std::endl;

        PCI_PCIeCfgData pciExCfgData;
        pciExCfgData.m_Bus = Device->Bus;
        pciExCfgData.m_Device = Device->Device;
        pciExCfgData.m_Function = Device->Function;
        pciExCfgData.m_Offset = 0;
        pciExCfgData.OutputData.m_Size = PCIe_CFG_SIZE;
        pciExCfgData.OutputData.DataPointer = (PUINT8)calloc(1, pciExCfgData.OutputData.m_Size);
        if (pciExCfgData.OutputData.DataPointer == NULL) {
            std::cout << "Memory allocation failed" << std::endl;
            return;
        }

        userStatus = CHWLib.PCIeExCfgRead(&pciExCfgData);
        if (userStatus != Success) {
            if (pciExCfgData.OutputData.DataPointer) {
                free(pciExCfgData.OutputData.DataPointer);
            }
            std::cout << "PCIeExCfgRead failed, Error: " << CHWLib.GetStatusMessage() << std::endl;
            std::cout << std::endl << std::string(100, '*') << std::endl << std::endl;
            continue;
        }

        std::cout << "    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F" << std::endl;
        std::cout << " -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --" << std::endl;
        for (UINT32 RowIndex = 0; RowIndex < pciExCfgData.OutputData.m_Size; RowIndex += 0x10)
        {
            std::cout << std::setw(3) << std::setfill('0') << std::uppercase << std::hex << RowIndex << "|";
            for (UINT32 ByteIndex = RowIndex; ByteIndex < RowIndex + 0x10; ByteIndex++)
            {
                std::cout << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << +(pciExCfgData.OutputData.DataPointer[ByteIndex]) << " ";
            }
            std::cout << std::endl;
        }

        if (pciExCfgData.OutputData.DataPointer) {
            free(pciExCfgData.OutputData.DataPointer);
        }
        std::cout << std::endl << std::string(100, '*') << std::endl << std::endl;
    }
}

UserStatus GetPCIPCIeDevices(std::vector<PCI_PCIeDevice>& PCIPCIeDevices)
{
    UserStatus userStatus = Success;
    CONFIGRET cr = CR_SUCCESS;
    ULONG DeviceListLength = 0;
    PWSTR DeviceList = NULL;
    PWSTR CurrentDevice;
    DEVINST DevInst;
    ULONG RequiredLength = 0;

    CHardwareInterfaceLib CHWLib;
    userStatus = CHWLib.CHardwareInterfaceLibInitialise();
    if (userStatus != Success)
    {
        userStatus = Failure;
        std::cout << "CHardwareInterfaceLibInitialise failed, Error: " << CHWLib.GetStatusMessage() << std::endl;
        return userStatus;
    }

    cr = CM_Get_Device_ID_List_Size(&DeviceListLength,
        REGSTR_KEY_PCIENUM,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS) {
        userStatus = Failure;
        std::cout << "Failed to get PCI device list size." << std::endl;
        goto Exit;
    }

    DeviceList = (PWSTR)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        DeviceListLength * sizeof(WCHAR));

    if (DeviceList == NULL) {
        userStatus = NullPointer;
        std::cout << "Failed to allocate memory for PCI device list size." << std::endl;
        goto Exit;
    }

    cr = CM_Get_Device_ID_List(REGSTR_KEY_PCIENUM,
        DeviceList,
        DeviceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

    if (cr != CR_SUCCESS) {
        userStatus = Failure;
        std::cout << "Failed to get PCI device list." << std::endl;
        goto Exit;
    }

    //
    // Iterate through all PCI devices
    //
    for (CurrentDevice = DeviceList;
        *CurrentDevice;
        CurrentDevice += wcslen(CurrentDevice) + 1)
    {
        cr = CM_Locate_DevNode(&DevInst,
            CurrentDevice,
            CM_LOCATE_DEVNODE_NORMAL);

        if (cr != CR_SUCCESS)
        {
            if ((cr == CR_NO_SUCH_DEVNODE) || (cr == CR_INVALID_DEVICE_ID))
                continue;
        }

        //
        // Get bus number
        //
        UINT32 BusNumber = 0;
        RequiredLength = sizeof(BusNumber);
        cr = CM_Get_DevNode_Registry_Property(DevInst,
            CM_DRP_BUSNUMBER,
            NULL,
            (PVOID)&BusNumber,
            &RequiredLength,
            0);
        if (cr != CR_SUCCESS) {
            continue;
        }

        //
        // Get address.  upper 16 bits of address is device number.  lower 16 bits is function.
        //
        UINT32 Address;
        UINT8 DeviceNumber = 0;
        UINT8 FunctionNumber = 0;
        RequiredLength = sizeof(Address);
        cr = CM_Get_DevNode_Registry_Property(DevInst,
            CM_DRP_ADDRESS,
            NULL, (PVOID)&Address,
            &RequiredLength,
            0);
        if (cr != CR_SUCCESS) {
            continue;
        }

        DeviceNumber = (Address & 0xFFFF0000) >> 16;
        FunctionNumber = (Address & 0x0000FFFF);

        //
        // Get device friendly name / device description
        //
        std::string DeviceName;
        CHAR Buffer[1024];
        RequiredLength = sizeof(Buffer);
        cr = CM_Get_DevNode_Registry_PropertyA(DevInst,
            CM_DRP_FRIENDLYNAME,
            NULL,
            Buffer,
            &RequiredLength,
            0);
        if (cr != CR_SUCCESS) {
            cr = CM_Get_DevNode_Registry_PropertyA(DevInst,
                CM_DRP_DEVICEDESC,
                NULL,
                Buffer,
                &RequiredLength,
                0);
            if (cr != CR_SUCCESS) {
                DeviceName = "Unknown Device";
            }
            else {
                Buffer[sizeof(Buffer) - 1] = '\0';
                DeviceName = Buffer;
            }
        }
        else {
            Buffer[sizeof(Buffer) - 1] = '\0';
            DeviceName = Buffer;
        }

        UINT32 RegValue = 0;
        PCI_PCIeCfgData pciStdData;
        pciStdData.m_Bus = BusNumber;
        pciStdData.m_Device = DeviceNumber;
        pciStdData.m_Function = FunctionNumber;
        pciStdData.m_Offset = 0;
        pciStdData.OutputData.m_Size = sizeof(RegValue);
        pciStdData.OutputData.DataPointer = (PUINT8)&RegValue;

        userStatus = CHWLib.PCIStdCfgRead(&pciStdData);
        if (userStatus == Success) {
            PCI_PCIeDevice Device;
            Device.DeviceName = DeviceName;
            Device.Bus = BusNumber;
            Device.Device = DeviceNumber;
            Device.Function = FunctionNumber;
            PCIPCIeDevices.push_back(Device);
        }
    }

Exit:
    if (DeviceList != NULL)
    {
        HeapFree(GetProcessHeap(),
            0,
            DeviceList);
    }

    userStatus = CHWLib.CHardwareInterfaceLibUninitialise();
    if (userStatus != Success)
    {
        std::cout << "CHardwareInterfaceLibUninitialise failed, Error: " << CHWLib.GetStatusMessage() << std::endl;
    }

    return userStatus;
}