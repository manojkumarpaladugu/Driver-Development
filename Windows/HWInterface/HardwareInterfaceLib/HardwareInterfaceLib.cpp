#include "HardwareInterfaceLib.h"

CHardwareInterfaceLib::CHardwareInterfaceLib()
{
    m_HardwareInterfaceDrv = NULL;
    m_PCIeExBar = 0;
}

CHardwareInterfaceLib::~CHardwareInterfaceLib()
{

}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CHardwareInterfaceLib::CHardwareInterfaceLibInitialise

  Summary:  Opens handle to Hardware Interface driver.

  Args:     None

  Modifies: None

  Returns:  UserStatus
              Returns error code.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
UserStatus CHardwareInterfaceLib::CHardwareInterfaceLibInitialise()
{
    UserStatus userStatus = Success;
    PCI_PCIeCfgData pciStdData;
    m_StatusMessage.str("");

    m_HardwareInterfaceDrv = CreateFileA(HW_INTERFACE_DRIVER,
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL
                                         );
    if (m_HardwareInterfaceDrv == INVALID_HANDLE_VALUE)
    {
        m_StatusMessage << "Unable to open handle to " << HW_INTERFACE_DRIVER;
        userStatus = InvalidHandle;
        goto Exit;
    }

    pciStdData.m_Bus = 0;
    pciStdData.m_Device = 0;
    pciStdData.m_Function = 0;
    pciStdData.m_Offset = 0x60;
    pciStdData.OutputData.m_Size = sizeof(m_PCIeExBar);
    pciStdData.OutputData.DataPointer = (PUINT8)&m_PCIeExBar;

    userStatus = PCIStdCfgRead(&pciStdData);
    if (userStatus != Success) {
        m_StatusMessage << "PCIStdCfgRead failed, status: 0x" << std::hex << userStatus;
    }

    m_PCIeExBar &= 0x0000000ffc000000i64;

Exit:
    return userStatus;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CHardwareInterfaceLib::PCIeCfgRead

  Summary:  Reads value of the specified register from configuration space of a PCI/PCIe device till 256 bytes.

  Args:     PPCI_PCIeCfgData pPCIStdCfgData
              Contains Bus, Device, Function and Offset values to read from PCI/PCIe device.

  Modifies: [OutputData].

  Returns:  UserStatus
              Returns error code.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
UserStatus CHardwareInterfaceLib::PCIStdCfgRead(PPCI_PCIeCfgData pPCIStdCfgData)
{
    UserStatus userStatus = Success;
    DWORD BytesReturned = 0;
    bool successPCIRead;
    m_StatusMessage.str("");

    if (pPCIStdCfgData->m_Offset + pPCIStdCfgData->OutputData.m_Size > PCI_CFG_SIZE) {
        m_StatusMessage << "Requested offset 0x" << std::hex << pPCIStdCfgData->m_Offset << ", data length: 0x" << std::hex << pPCIStdCfgData->OutputData.m_Size 
            << " is out of range. PCI/PCIe standard configuration size is 0x" << std::hex << PCI_CFG_SIZE << " bytes only";
        userStatus = IndexOutOfRange;
        goto Exit;
    }

    successPCIRead = DeviceIoControl(m_HardwareInterfaceDrv,
                                     IOCTL_PLATFORM_PCI_STD_CFG_READ,
                                     (LPVOID)pPCIStdCfgData, sizeof(*pPCIStdCfgData),
                                     (LPVOID)pPCIStdCfgData, sizeof(*pPCIStdCfgData),
                                     &BytesReturned,
                                     NULL);
    if (successPCIRead == false) {
        userStatus = Failure;
        m_StatusMessage << "Could not read PCI standard config space for Bus: 0x" << std::hex << +(pPCIStdCfgData->m_Bus) << ", Device: 0x" 
            << std::hex << +(pPCIStdCfgData->m_Device) << ", Function: 0x" << std::hex << +(pPCIStdCfgData->m_Function) << ", Offset: 0x" 
            << std::hex << +(pPCIStdCfgData->m_Offset);
    }

Exit:
    return userStatus;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CHardwareInterfaceLib::PCIeExCfgRead

  Summary:  Reads value of the specified register from extended configuration space of a PCIe device till 4 KB.

  Args:     PPCI_PCIeCfgData pPCIeExCfgData
              Contains Bus, Device, Function and Offset values to read from PCIe device.

  Modifies: [OutputData].

  Returns:  UserStatus
              Returns error code.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
UserStatus CHardwareInterfaceLib::PCIeExCfgRead(PPCI_PCIeCfgData pPCIeExCfgData)
{
    UserStatus userStatus = Success;
    m_StatusMessage.str("");

    if (pPCIeExCfgData->m_Offset + pPCIeExCfgData->OutputData.m_Size > PCIe_CFG_SIZE) {
        m_StatusMessage << "Requested offset: 0x" << std::hex << pPCIeExCfgData->m_Offset << ", data length: " << std::hex << pPCIeExCfgData->OutputData.m_Size 
            << " is out of range. PCIe extended configuration size is 0x" << std::hex << PCIe_CFG_SIZE << " bytes only";
        userStatus = IndexOutOfRange;
        goto Exit;
    }

    PCIeMMIOData pcieMMIOData;
    pcieMMIOData.m_BaseAddressRegister = m_PCIeExBar + ((UINT64)pPCIeExCfgData->m_Bus << 20) + ((UINT64)pPCIeExCfgData->m_Device << 15) + ((UINT64)pPCIeExCfgData->m_Function << 12);
    pcieMMIOData.m_Offset = pPCIeExCfgData->m_Offset;
    pcieMMIOData.OutputData.m_Size = pPCIeExCfgData->OutputData.m_Size;
    pcieMMIOData.OutputData.DataPointer = pPCIeExCfgData->OutputData.DataPointer;
    userStatus = PCIeMMIORead(&pcieMMIOData);
    if (userStatus == Success) {
        m_StatusMessage << "Could not read PCIe extended config space for Bus: 0x" << std::hex << +(pPCIeExCfgData->m_Bus) << ", Device: 0x" << std::hex 
            << +(pPCIeExCfgData->m_Device) + ", Function: 0x" << std::hex << +(pPCIeExCfgData->m_Function) << ", Offset: 0x" << std::hex << +(pPCIeExCfgData->m_Offset);
    }

Exit:
    return userStatus;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CHardwareInterfaceLib::PCIeMMIORead

  Summary:  Reads value from the MMIO region address of a PCIe device.

  Args:     PPCI_PCIeMMIOData pPCIeMMIOData
              Reads value of the MMIO region address of a PCIe device..

  Modifies: [OutputData].

  Returns:  UserStatus
              Returns error code.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
UserStatus CHardwareInterfaceLib::PCIeMMIORead(PPCIeMMIOData pPCIeMMIOData)
{
    UserStatus userStatus = Success;
    DWORD BytesReturned = 0;
    bool successPCIeMMIORead;
    m_StatusMessage.str("");

    if (pPCIeMMIOData->m_Offset + pPCIeMMIOData->OutputData.m_Size > PCIe_CFG_SIZE) {
        m_StatusMessage << "Requested offset: 0x" << std::hex << pPCIeMMIOData->m_Offset << ", data length: 0x" << std::hex << pPCIeMMIOData->OutputData.m_Size 
            << " is out of range. PCIe extended configuration size is 0x" << std::hex << PCIe_CFG_SIZE << " bytes only";
        userStatus = IndexOutOfRange;
        goto Exit;
    }

    successPCIeMMIORead = DeviceIoControl(m_HardwareInterfaceDrv,
                                          IOCTL_PLATFORM_PCIe_MMIO_READ,
                                          (LPVOID)pPCIeMMIOData, sizeof(*pPCIeMMIOData),
                                          (LPVOID)pPCIeMMIOData, sizeof(*pPCIeMMIOData),
                                          &BytesReturned,
                                          NULL);
    if (successPCIeMMIORead == false) {
        userStatus = Failure;
        m_StatusMessage << "Could not read PCIe MMIO region at base address: 0x" << std::hex << pPCIeMMIOData->m_BaseAddressRegister << ", offset: 0x" << std::hex << pPCIeMMIOData->m_Offset;
    }

Exit:
    return userStatus;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CHardwareInterfaceLib::CHardwareInterfaceLibUninitialise

  Summary:  Closes handle to Hardware Interface driver.

  Args:     None

  Modifies: None

  Returns:  UserStatus
              Returns error code.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
UserStatus CHardwareInterfaceLib::CHardwareInterfaceLibUninitialise()
{
    UserStatus userStatus = Success;
    m_StatusMessage.str("");

    if (m_HardwareInterfaceDrv) {
        CloseHandle(m_HardwareInterfaceDrv);
    }

    return userStatus;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CHardwareInterfaceLib::GetStatusMessage

  Summary:  Returns the error status message.

  Args:     None

  Modifies: None

  Returns:  Error message
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
std::string CHardwareInterfaceLib::GetStatusMessage()
{
    return m_StatusMessage.str();
}
