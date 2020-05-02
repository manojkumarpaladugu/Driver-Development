/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an symbolic link so that apps can find the device and talk to it.
//
#define HW_INTERFACE_DRIVER "\\\\.\\HWInterface"

#define PCI_CFG_SIZE  0x100
#define PCIe_CFG_SIZE 0x1000

#define IOCTL_PLATFORM_PCI_PCIe 0x8081

#define IOCTL_PLATFORM_PCI_STD_CFG_READ\
        CTL_CODE(IOCTL_PLATFORM_PCI_PCIe, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PLATFORM_PCIe_MMIO_READ\
        CTL_CODE(IOCTL_PLATFORM_PCI_PCIe, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    PUINT8 DataPointer;
    UINT32 m_Size;
}DataElement;

typedef struct
{
    UINT8 m_Bus;
    UINT8 m_Device;
    UINT8 m_Function;
    UINT32 m_Offset;
    DataElement OutputData;
}PCI_PCIeCfgData, *PPCI_PCIeCfgData;

typedef struct
{
    UINT64 m_BaseAddressRegister;
    UINT32 m_Offset;
    DataElement OutputData;
}PCIeMMIOData, *PPCIeMMIOData;
#pragma pack(pop)
