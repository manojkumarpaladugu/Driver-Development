#pragma once
#include <ntddk.h>
#include <wdf.h>
#include <Ntstrsafe.h>
#include "public.h"
#include "Trace.h"

#define NT_DEVICE_NAME		        L"\\Device\\NonPnP"
#define SYMBOLIC_LINK_NAME	        L"\\DosDevices\\NonPnP"
#define DRIVER_POOL_TAG		        'NPnP'

typedef struct _CONTROL_DEVICE_EXTENSION {

    HANDLE   FileHandle; // Store your control data here

} CONTROL_DEVICE_EXTENSION, * PCONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_EXTENSION, ControlGetData)

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD EvtWdfDriverUnload;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtWdfIoDeviceControl;