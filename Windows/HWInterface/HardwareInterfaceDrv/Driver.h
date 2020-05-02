/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include "Public.h"
#include "Trace.h"

EXTERN_C_START

#define DRIVER_POOL_TAG 'DIWH'

#define NT_DEVICE_NAME L"\\Device\\HWInterface"
#define SYMBOLIC_LINK_NAME L"\\DosDevices\\HWInterface"

typedef struct _CONTROL_DEVICE_EXTENSION {

    HANDLE   FileHandle; // Store your control data here

} CONTROL_DEVICE_EXTENSION, * PCONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_EXTENSION, ControlGetData)

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD HardwareInterfaceDrvEvtDriverUnload;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL HardwareInterfaceDrvEvtIoDeviceControl;

EXTERN_C_END
