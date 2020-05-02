#include "NonPnPDrv.h"
#include "NonPnPDrv.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, EvtWdfDriverUnload)
#pragma alloc_text(PAGE, EvtWdfIoDeviceControl)
#endif

NTSTATUS
DriverEntry(
	IN OUT PDRIVER_OBJECT	DriverObject,
	IN PUNICODE_STRING		RegistryPath
)
{
	NTSTATUS				Status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG		Config;
	WDFDRIVER				Driver = NULL;
	PWDFDEVICE_INIT			pDeviceInit = NULL;
	WDFDEVICE				ControlDevice;
	WDF_IO_QUEUE_CONFIG		IOQueueConfig;
	WDF_OBJECT_ATTRIBUTES	Attributes;
	WDFQUEUE				Queue;

	DECLARE_CONST_UNICODE_STRING(NTDeviceName, NT_DEVICE_NAME);
	DECLARE_CONST_UNICODE_STRING(SymbolicLinkName, SYMBOLIC_LINK_NAME);

	WPP_INIT_TRACING(DriverObject, RegistryPath);

	TraceEvents(TRACE_LEVEL_VERBOSE, NON_PNP, "%!FUNC!: Entry\n");

	//
	// The WDF_DRIVER_CONFIG_INIT function initializes a driver's WDF_DRIVER_CONFIG structure.
	//
	WDF_DRIVER_CONFIG_INIT(&Config, WDF_NO_EVENT_CALLBACK);

	Config.Size = sizeof(WDF_DRIVER_CONFIG);
	Config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
	Config.EvtDriverUnload = EvtWdfDriverUnload;
	Config.DriverPoolTag = DRIVER_POOL_TAG;

	//
	// The WdfDriverCreate method creates a framework driver object for the calling driver.
	//
	Status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &Config, &Driver);
	if (!NT_SUCCESS(Status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "%!FUNC!: WdfDriverCreate failed with status 0x%x\n", Status);
		WPP_CLEANUP(DriverObject);
		goto Exit;
	}

	//
	// In order to create a control device, we first need to allocate a
	// WDFDEVICE_INIT structure and set all properties.
	//
	pDeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R
	);

	if (pDeviceInit == NULL) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "%!FUNC!: WdfControlDeviceInitAllocate failed with status 0x%x\n", Status);
		WPP_CLEANUP(DriverObject);
		goto Exit;
	}

	//
	// Set exclusive to TRUE so that no more than one app can talk to the
	// control device at any time.
	//
	WdfDeviceInitSetExclusive(pDeviceInit, TRUE);
	WdfDeviceInitSetIoType(pDeviceInit, WdfDeviceIoBuffered);
	Status = WdfDeviceInitAssignName(pDeviceInit, &NTDeviceName);
	if (!NT_SUCCESS(Status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "%!FUNC!: WdfDeviceInitAssignName failed with status 0x%x\n", Status);
		WPP_CLEANUP(DriverObject);
		goto Exit;
	}

	//
	// Specify the size of device context
	//
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, CONTROL_DEVICE_EXTENSION);

	Status = WdfDeviceCreate(&pDeviceInit, &Attributes, &ControlDevice);
	if (!NT_SUCCESS(Status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "%!FUNC!: WdfDeviceCreate failed with status 0x%x\n", Status);
		WPP_CLEANUP(DriverObject);
		goto Exit;
	}

	//
	// Create a symbolic link for the control object so that usermode can open the device.
	//
	Status = WdfDeviceCreateSymbolicLink(ControlDevice, &SymbolicLinkName);
	if (!NT_SUCCESS(Status))
	{
		TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "%!FUNC!: WdfDeviceCreateSymbolicLink failed with status 0x%x\n", Status);
		WPP_CLEANUP(DriverObject);
		goto Exit;
	}

	//
	// Configure a default queue
	//
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&IOQueueConfig,
		WdfIoQueueDispatchSequential);

	IOQueueConfig.EvtIoDeviceControl = EvtWdfIoDeviceControl;

	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);

	__analysis_assume(IOQueueConfig.EvtIoStop != 0);
	Status = WdfIoQueueCreate(ControlDevice,
		&IOQueueConfig,
		&Attributes,
		&Queue // pointer to default queue
	);
	__analysis_assume(IOQueueConfig.EvtIoStop == 0);
	if (!NT_SUCCESS(Status)) {
		TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "%!FUNC!: WdfIoQueueCreate failed with status 0x%x\n", Status);
		WPP_CLEANUP(DriverObject);
		goto Exit;
	}

	//
	// Control devices must notify WDF when they are done initializing.   I/O is
	// rejected until this call is made.
	//
	WdfControlFinishInitializing(ControlDevice);

Exit:
	//
	// If the device is created successfully, framework would clear the
	// DeviceInit value. Otherwise device create must have failed so we
	// should free the memory ourself.
	//
	if (pDeviceInit != NULL) {
		WdfDeviceInitFree(pDeviceInit);
	}

	TraceEvents(TRACE_LEVEL_VERBOSE, NON_PNP, "%!FUNC!: Exiting with status 0x%x", Status);
	return Status;
}

void EvtWdfIoDeviceControl(
	WDFQUEUE Queue,
	WDFREQUEST Request,
	size_t OutputBufferLength,
	size_t InputBufferLength,
	ULONG IoControlCode
)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	PCHAR       InBuf = NULL, OutBuf = NULL; // pointer to Input and output buffer
	size_t		BufSize = 0;
	PCHAR       Data = "Hello %s!, greetings from NonPnP driver.";
	CHAR        TempBuf[500] = { 0 };

	TraceEvents(TRACE_LEVEL_VERBOSE, NON_PNP, "%!FUNC!: Entry\n");

	UNREFERENCED_PARAMETER(Queue);

	PAGED_CODE();

	if (!OutputBufferLength || !InputBufferLength)
	{
		Status = STATUS_INVALID_PARAMETER;
		WdfRequestComplete(Request, Status);
		return;
	}

	switch (IoControlCode)
	{
		case IOCTL_NONPNP_GET_MESSAGE:
		{
			TraceEvents(TRACE_LEVEL_INFORMATION, NON_PNP, "Called IOCTL_NONPNP_GET_MESSAGE\n");

			Status = WdfRequestRetrieveInputBuffer(Request, 0, &InBuf, &BufSize);
			if (!NT_SUCCESS(Status)) {
				Status = STATUS_INSUFFICIENT_RESOURCES;
				TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "WdfRequestRetrieveInputBuffer failed with status 0x%x\n", Status);
				break;
			}

			TraceEvents(TRACE_LEVEL_INFORMATION, NON_PNP, "BufSize: %d, InputBufferLength: %d\n", (int)BufSize, (int)InputBufferLength);
			ASSERT(BufSize == InputBufferLength);

			Status = WdfRequestRetrieveOutputBuffer(Request, 0, &OutBuf, &BufSize);
			if (!NT_SUCCESS(Status)) {
				Status = STATUS_INSUFFICIENT_RESOURCES;
				TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "WdfRequestRetrieveOutputBuffer failed with status 0x%x\n", Status);
				break;
			}

			TraceEvents(TRACE_LEVEL_INFORMATION, NON_PNP, "BufSize: %d, OutputBufferLength: %d\n", (int)BufSize, (int)OutputBufferLength);
			ASSERT(BufSize == OutputBufferLength);

			//
			// Format the string with data received from input buffer
			//
			RtlStringCchPrintfA(TempBuf, sizeof(TempBuf), Data, InBuf);
			size_t cnt = 0;
			RtlStringCchLengthA(TempBuf, sizeof(TempBuf), &cnt);

			//
			// Write data into the buffer
			//
			RtlCopyMemory(OutBuf, TempBuf, OutputBufferLength);

			//
			// Assign the length of the data copied to IoStatus.Information
			// of the request and complete the request.
			//
			WdfRequestSetInformation(Request,
				OutputBufferLength < cnt ? OutputBufferLength : cnt);

			break;
		}

		default:
		{
			//
			// The specified I/O control code is unrecognized by this driver.
			//
			Status = STATUS_INVALID_DEVICE_REQUEST;
			TraceEvents(TRACE_LEVEL_ERROR, NON_PNP, "Unrecognized IOCTL code %x\n", IoControlCode);
			break;
		}
	}

	WdfRequestComplete(Request, Status);

	TraceEvents(TRACE_LEVEL_VERBOSE, NON_PNP, "%!FUNC!: Exiting with status 0x%x", Status);
}

void EvtWdfDriverUnload(
	WDFDRIVER Driver
)
{
	TraceEvents(TRACE_LEVEL_VERBOSE, NON_PNP, "%!FUNC!: Entry\n");
	
	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)Driver));

	TraceEvents(TRACE_LEVEL_VERBOSE, NON_PNP, "%!FUNC!: Exit\n");
	return;
}