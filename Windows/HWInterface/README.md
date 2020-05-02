Introduction: This is a HWInterface driver, also called as software driver, it is not associated with any device. In simple, it is a software runs in kernel mode and provides services to the user application.

In this, user application enumerates PCI/PCIe devices and sends request to read each PCI/PCIe device configuration size of 256 Bytes/4K Bytes from driver.

Instructions:
  1. Open HWInterface.sln and build the solution.
  2. Run HardwareInterfaceDrv.sys service using osrloader.exe (Browse driver, Register Service, Start Service).
  3. Run HardwareInterfaceApp.exe.
  4. Stop HardwareInterfaceDrv.sys service using osrloader.exe (Stop Service, Unregister Service).

Output: Dump of 256 Bytes/4K Bytes PCI/PCIe devices configuration space.
