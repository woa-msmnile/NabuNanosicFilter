/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

EXTERN_C_START

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    ULONG PrivateDeviceData;  // just a placeholder
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

typedef struct _FILTER_EXTENSION
{
    WDFDEVICE WdfDevice;
    // More context data here

}FILTER_EXTENSION, * PFILTER_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILTER_EXTENSION,
    FilterGetData)

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
NanosicFilterCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

VOID
OnRequestCompletionRoutine(
    IN WDFREQUEST  Request,
    IN WDFIOTARGET  Target,
    IN PWDF_REQUEST_COMPLETION_PARAMS  Params,
    IN WDFCONTEXT  Context
);

VOID
OnInternalDeviceControl(
    IN WDFQUEUE      Queue,
    IN WDFREQUEST    Request,
    IN size_t        OutputBufferLength,
    IN size_t        InputBufferLength,
    IN ULONG         IoControlCode
);

EXTERN_C_END
