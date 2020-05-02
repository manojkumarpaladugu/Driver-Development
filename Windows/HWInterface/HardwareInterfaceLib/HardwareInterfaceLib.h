#pragma once
/*+===================================================================
  File:      HardwareInterfaceLib.h

  Summary:   Provides APIs to read registers from DUT.

  Classes:   CHardwareInterfaceLib.

  Functions: PCIStdCfgRead, PCIeExCfgRead, PCIeMMIORead.

  Origin:    

##

  Copyright and Legal notices.
===================================================================+*/

#include <Windows.h>
#include <sstream>
#include "..\HardwareInterfaceDrv\Public.h"

typedef enum
{
    Success,
    Failure,
    InvalidHandle,
    IndexOutOfRange,
    NullPointer
}UserStatus;

/*C+C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C+++C
  Class:    CHardwareInterfaceLib

  Summary:  Provides APIs to read registers from DUT.

  Methods:  CHardwareInterfaceLib()
              Constructor.
            ~CHardwareInterfaceLib()
              Destructor.
            UserStatus CHardwareInterfaceLibInitialise()
              Opens handle to Hardware Interface driver.
            UserStatus PCIStdCfgRead(PPCI_PCIeCfgData pPCIStdCfgData)
              Reads value of the specified register from configuration space of a PCI/PCIe device till 256 bytes.
            UserStatus PCIeExCfgRead(PPCI_PCIeCfgData pPCIeExCfgData)
              Reads value of the specified register from extended configuration space of a PCIe device till 4 KB.
            UserStatus PCIeMMIORead(PPCIeMMIOData pPCIeMMIOData)
              Reads value from the MMIO region address of a PCIe device.
            UserStatus CHardwareInterfaceLibUninitialise()
              Closes handle to Hardware Interface driver.
            std::string GetStatusMessage()
              Returns the error status message.
C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C---C-C*/
class CHardwareInterfaceLib
{
public:
    CHardwareInterfaceLib();
    ~CHardwareInterfaceLib();
    UserStatus CHardwareInterfaceLibInitialise();
    UserStatus PCIStdCfgRead(PPCI_PCIeCfgData pPCIStdCfgData);
    UserStatus PCIeExCfgRead(PPCI_PCIeCfgData pPCIeExCfgData);
    UserStatus PCIeMMIORead(PPCIeMMIOData pPCIeMMIOData);
    UserStatus CHardwareInterfaceLibUninitialise();
    std::string GetStatusMessage();

private:
    HANDLE m_HardwareInterfaceDrv;
    UINT64 m_PCIeExBar;
    std::stringstream m_StatusMessage;
};