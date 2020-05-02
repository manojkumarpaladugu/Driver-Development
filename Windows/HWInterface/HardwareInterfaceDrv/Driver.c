/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "Driver.h"
#include "Driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, HardwareInterfaceDrvEvtDriverUnload)
#pragma alloc_text (PAGE, HardwareInterfaceDrvEvtIoDeviceControl)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    NTSTATUS                status          = STATUS_SUCCESS;
    WDFDRIVER               driver          = NULL;
    PWDFDEVICE_INIT         deviceInit      = NULL;
    WDF_DRIVER_CONFIG       config;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDEVICE               controlDevice;
    WDF_IO_QUEUE_CONFIG     IOQueueConfig;
    WDFQUEUE                queue;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // The WDF_DRIVER_CONFIG_INIT function initializes a driver's WDF_DRIVER_CONFIG structure.
    //
    WDF_DRIVER_CONFIG_INIT(&config,
                           WDF_NO_EVENT_CALLBACK
                           );

    config.Size = sizeof(WDF_DRIVER_CONFIG);
    config.DriverInitFlags = WdfDriverInitNonPnpDriver;
    config.EvtDriverUnload = HardwareInterfaceDrvEvtDriverUnload;
    config.DriverPoolTag = DRIVER_POOL_TAG;

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             &driver
                             );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    //
    // In order to create a control device, we first need to allocate a
    // WDFDEVICE_INIT structure and set all properties.
    //
    deviceInit = WdfControlDeviceInitAllocate(driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R
    );
    if (deviceInit == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC!: WdfControlDeviceInitAllocate failed %!STATUS!\n", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    DECLARE_CONST_UNICODE_STRING(NTDeviceName, NT_DEVICE_NAME);
    //
    // Set exclusive to TRUE so that no more than one app can talk to the
    // control device at any time.
    //
    WdfDeviceInitSetExclusive(deviceInit, TRUE);
    WdfDeviceInitSetIoType(deviceInit, WdfDeviceIoBuffered);
    status = WdfDeviceInitAssignName(deviceInit, &NTDeviceName);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC!: WdfDeviceInitAssignName failed %!STATUS!\n", status);
        WPP_CLEANUP(DriverObject);
        if (deviceInit != NULL) {
            WdfDeviceInitFree(deviceInit);
        }
        return status;
    }

    //
    // Specify the size of device context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CONTROL_DEVICE_EXTENSION);

    status = WdfDeviceCreate(&deviceInit, &attributes, &controlDevice);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC!: WdfDeviceCreate failed %!STATUS!\n", status);
        WPP_CLEANUP(DriverObject);
        if (deviceInit != NULL) {
            WdfDeviceInitFree(deviceInit);
        }
        return status;
    }

    DECLARE_CONST_UNICODE_STRING(symbolicLinkName, SYMBOLIC_LINK_NAME);

    //
    // Create a symbolic link for the control object so that usermode can open the device.
    //
    status = WdfDeviceCreateSymbolicLink(controlDevice, &symbolicLinkName);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC!: WdfDeviceCreateSymbolicLink failed %!STATUS!\n", status);
        WPP_CLEANUP(DriverObject);
        if (deviceInit != NULL) {
            WdfDeviceInitFree(deviceInit);
        }
        return status;
    }

    //
    // Configure a default queue
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&IOQueueConfig,
        WdfIoQueueDispatchSequential);

    IOQueueConfig.EvtIoDeviceControl = HardwareInterfaceDrvEvtIoDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    __analysis_assume(IOQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(controlDevice,
        &IOQueueConfig,
        &attributes,
        &queue // pointer to default queue
    );
    __analysis_assume(IOQueueConfig.EvtIoStop == 0);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "%!FUNC!: WdfIoQueueCreate failed %!STATUS!\n", status);
        WPP_CLEANUP(DriverObject);
        if (deviceInit != NULL) {
            WdfDeviceInitFree(deviceInit);
        }
        return status;
    }

    //
    // Control devices must notify WDF when they are done initializing.
    // I/O is rejected until this call is made.
    //
    WdfControlFinishInitializing(controlDevice);

    if (deviceInit != NULL) {
        WdfDeviceInitFree(deviceInit);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");
    return status;
}

void HardwareInterfaceDrvEvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
)
{
    NTSTATUS    status  = STATUS_SUCCESS;
    NTSTATUS	Status  = STATUS_SUCCESS;
    PCHAR       InBuf   = NULL, OutBuf = NULL; // pointer to Input and output buffer
    size_t		BufSize = 0;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC!: Entry\n");

    UNREFERENCED_PARAMETER(Queue);

    PAGED_CODE();

    if (!OutputBufferLength || !InputBufferLength)
    {
        status = STATUS_INVALID_PARAMETER;
        WdfRequestComplete(Request, status);
        return;
    }

    switch (IoControlCode)
    {
        case IOCTL_PLATFORM_PCI_STD_CFG_READ:
        {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Called  IOCTL_PLATFORM_PCI_STD_CFG_READ 0x%x\n", IoControlCode);

            if (InputBufferLength < sizeof(PCI_PCIeCfgData))
            {
                Status = STATUS_INVALID_PARAMETER;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Input buffer too small\n");
                break;
            }

            if (OutputBufferLength < sizeof(PCI_PCIeCfgData))
            {
                Status = STATUS_INVALID_PARAMETER;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Output buffer too small\n");
                break;
            }

            Status = WdfRequestRetrieveInputBuffer(Request, 0, &InBuf, &BufSize);
            if (!NT_SUCCESS(Status)) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveInputBuffer failed with status 0x%x\n", Status);
                break;
            }

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "BufSize: %d, InputBufferLength: %d\n", (int)BufSize, (int)InputBufferLength);
            ASSERT(BufSize == InputBufferLength);

            Status = WdfRequestRetrieveOutputBuffer(Request, 0, &OutBuf, &BufSize);
            if (!NT_SUCCESS(Status)) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveOutputBuffer failed with status 0x%x\n", Status);
                break;
            }

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "BufSize: %d, OutputBufferLength: %d\n", (int)BufSize, (int)OutputBufferLength);
            ASSERT(BufSize == OutputBufferLength);

            PPCI_PCIeCfgData PCIDataIn = (PPCI_PCIeCfgData)InBuf;
            PPCI_PCIeCfgData PCIDataOut = (PPCI_PCIeCfgData)OutBuf;

            if (PCIDataIn->m_Offset + PCIDataIn->OutputData.m_Size > PCI_CFG_SIZE) {
                Status = STATUS_INVALID_PARAMETER;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Requested offset 0x%x is out of range, PCI/PCIe standard configuration size is %d bytes only.", PCIDataIn->m_Offset, PCI_CFG_SIZE);
                break;
            }

            PCI_SLOT_NUMBER slot;
            RtlSecureZeroMemory(&slot, sizeof(slot));
            slot.u.bits.DeviceNumber = PCIDataIn->m_Device;
            slot.u.bits.FunctionNumber = PCIDataIn->m_Function;

            //
            // Get the PCI register data
            //
            UINT32 totalReturned = 0;
            for (UINT32 i = 0; i < PCIDataIn->OutputData.m_Size; i++) {
                ULONG bytesReturned = HalGetBusDataByOffset(PCIConfiguration,
                                                            PCIDataIn->m_Bus,
                                                            slot.u.AsULONG,
                                                            &PCIDataOut->OutputData.DataPointer[i],
                                                            PCIDataIn->m_Offset + i,
                                                            1);

                if (bytesReturned != 1) {
                    TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Failed to read standard PCI config space for Bus: 0x%x, Device: 0x%x, Function: 0x%x, Offset: 0x%x",
                        PCIDataIn->m_Bus, slot.u.bits.DeviceNumber, slot.u.bits.FunctionNumber, PCIDataIn->m_Offset);
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
                totalReturned++;
            }
            PCIDataOut->OutputData.m_Size = totalReturned;
            
            //
            // Assign the length of the data copied to IoStatus.Information
            // of the request and complete the request.
            //
            WdfRequestSetInformation(Request, sizeof(PCI_PCIeCfgData));

            break;
        }

        case IOCTL_PLATFORM_PCIe_MMIO_READ:
        {
            if (InputBufferLength < sizeof(PCIeMMIOData))
            {
                Status = STATUS_INVALID_PARAMETER;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Input buffer too small\n");
                break;
            }

            if (OutputBufferLength < sizeof(PCIeMMIOData))
            {
                Status = STATUS_INVALID_PARAMETER;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Output buffer too small\n");
                break;
            }

            Status = WdfRequestRetrieveInputBuffer(Request, 0, &InBuf, &BufSize);
            if (!NT_SUCCESS(Status)) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveInputBuffer failed with status 0x%x\n", Status);
                break;
            }

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "BufSize: %d, InputBufferLength: %d\n", (int)BufSize, (int)InputBufferLength);
            ASSERT(BufSize == InputBufferLength);

            Status = WdfRequestRetrieveOutputBuffer(Request, 0, &OutBuf, &BufSize);
            if (!NT_SUCCESS(Status)) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveOutputBuffer failed with status 0x%x\n", Status);
                break;
            }

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "BufSize: %d, OutputBufferLength: %d\n", (int)BufSize, (int)OutputBufferLength);
            ASSERT(BufSize == OutputBufferLength);

            PPCIeMMIOData PCIeMMIODataIn = (PPCIeMMIOData)InBuf;
            PPCIeMMIOData PCIeMMIODataOut = (PPCIeMMIOData)OutBuf;

            if (PCIeMMIODataIn->m_Offset + PCIeMMIODataIn->OutputData.m_Size > PCIe_CFG_SIZE) {
                Status = STATUS_INVALID_PARAMETER;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Requested offset 0x%x, data length %d exceeds PCIe range %d bytes.", PCIeMMIODataIn->m_Offset, PCIeMMIODataIn->OutputData.m_Size, PCIe_CFG_SIZE);
                break;
            }

            PHYSICAL_ADDRESS phyAddr;
            phyAddr.QuadPart = PCIeMMIODataIn->m_BaseAddressRegister;

            SIZE_T barSize = PCIe_CFG_SIZE;
            //
            // Map the physical address to virtual space
            //
            PUINT8 pMMIO = (MmMapIoSpace(phyAddr, barSize, MmNonCached));
            if (pMMIO == NULL) {
                Status = STATUS_NO_MEMORY;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Unable to map BAR\n");
                break;
            }

            //
            // Check if the device is in D3 by seeing if first DWORd contains F's
            //
            UINT32 d3Check = *((PUINT32)pMMIO);
            if (d3Check == 0xFFFFFFFF) {
                Status = STATUS_POWER_STATE_INVALID;
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "MMIO access requested in D3 state.\n");
                break;
            }

            //
            // Read data from MMIO region
            //
            PUINT8 pRegData = (PUINT8)(pMMIO + PCIeMMIODataOut->m_Offset);
            for (UINT32 i = 0; i < PCIeMMIODataOut->OutputData.m_Size; i++)
            {
                *PCIeMMIODataOut->OutputData.DataPointer++ = *pRegData++;
            }

            //
            // Assign the length of the data copied to IoStatus.Information
            // of the request and complete the request.
            //
            WdfRequestSetInformation(Request, sizeof(PCIeMMIOData));

            break;
        }

        default:
        {
            //
            // The specified I/O control code is unrecognized by this driver.
            //
            status = STATUS_INVALID_DEVICE_REQUEST;
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "Unrecognized IOCTL code %x\n", IoControlCode);
            break;
        }
    }

    WdfRequestComplete(Request, status);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC!: Exit, status %!STATUS!\n", status);
}

void HardwareInterfaceDrvEvtDriverUnload(
    WDFDRIVER Driver
)
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC!: Entry\n");

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC!: Exit\n");
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)Driver));
    return;
}
