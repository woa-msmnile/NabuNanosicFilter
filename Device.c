/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"
#include "stdio.h"
#include "string.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, NanosicFilterCreateDevice)
#endif

NTSTATUS
NanosicFilterCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    PDEVICE_CONTEXT pDevice = NULL;
    PAGED_CODE();
    //
    // Tell the framework that you are filter driver. Framework
    // takes care of inherting all the device flags & characterstics
    // from the lower device you are attaching to.
    //
    WdfFdoInitSetFilter(DeviceInit);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    status = WdfDeviceCreate(
        &DeviceInit,
        &deviceAttributes,
        &device);
    
    pDevice = DeviceGetContext(device);

    if (!NT_SUCCESS(status))
    {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DEVICE,
            "WdfDeviceCreate failed - 0x%08lX",
            status);

        goto exit;
    }

    //
    // Create a parallel dispatch queue to handle requests from HID Class
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel);

    queueConfig.EvtIoInternalDeviceControl = OnInternalDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE); // pointer to default queue

    if (!NT_SUCCESS(status))
    {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DEVICE,
            "Error creating WDF default queue - 0x%08lX",
            status);

        goto exit;
    }

    TraceEvents(
        TRACE_LEVEL_ERROR,
        TRACE_DEVICE,
        "NanosicFilterCreateDeviceExit: %!STATUS!\n", status);

exit:
    return status;
}


VOID
OnInternalDeviceControl(
    IN WDFQUEUE      Queue,
    IN WDFREQUEST    Request,
    IN size_t        OutputBufferLength,
    IN size_t        InputBufferLength,
    IN ULONG         IoControlCode
)
/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.

Return Value:

   VOID

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    BOOLEAN forwardWithCompletionRoutine = FALSE;
    BOOLEAN requestSent = TRUE;
    WDF_REQUEST_SEND_OPTIONS options;
    WDFMEMORY memory;
    WDFIOTARGET Target;

    //UNREFERENCED_PARAMETER(OutputBufferLength);
    //UNREFERENCED_PARAMETER(InputBufferLength);

    PAGED_CODE();

    device = WdfIoQueueGetDevice(Queue);
    Target = WdfDeviceGetIoTarget(device);

    //
    // Please note that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl. So depending on the ioctl code, we will either
    // use retreive function or escape to WDM to get the UserBuffer.
    //
    TraceEvents(
        TRACE_LEVEL_ERROR,
        TRACE_DEVICE,
        "OnInternalDeviceControl: 0x%llx, 0x%llx, 0x%llx\n",
        IoControlCode, OutputBufferLength, InputBufferLength);
    

    //switch (IoControlCode) {

    //                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
    //    //
    //    // Obtains the report descriptor for the HID device
    //    //
        forwardWithCompletionRoutine = TRUE;
    //    break;

    //default:
    //    break;
    //}

    //
    // Forward the request down. WdfDeviceGetIoTarget returns
    // the default target, which represents the device attached to us below in
    // the stack.
    //
    if (forwardWithCompletionRoutine) {
        //
        // Format the request with the output memory so the completion routine
        // can access the return data in order to cache it into the context area
        //
        status = WdfRequestRetrieveOutputMemory(Request, &memory);

        if (!NT_SUCCESS(status)) {
            TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_DEVICE,
                "WdfRequestRetrieveOutputMemory failed: 0x%x\n",
                status);

            WdfRequestComplete(Request, status);
            return;
        }

        status = WdfIoTargetFormatRequestForInternalIoctl(
            Target,
            Request,
            IoControlCode,
            NULL,
            NULL,
            memory,
            NULL);

        if (!NT_SUCCESS(status)) {
            TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_DEVICE,
                "WdfIoTargetFormatRequestForInternalIoctl failed: 0x%x\n",
                status);

            WdfRequestComplete(Request, status);
            return;
        }
        WdfRequestFormatRequestUsingCurrentType(Request);
        //
        // Set our completion routine with a context area that we will save
        // the output data into
        //
        WdfRequestSetCompletionRoutine(
            Request,
            OnRequestCompletionRoutine,
            WDF_NO_CONTEXT);

        requestSent = WdfRequestSend(
            Request,
            Target,
            WDF_NO_SEND_OPTIONS);
    }
    else
    {
        //
        // We are not interested in post processing the IRP so
        // fire and forget.
        //
        WDF_REQUEST_SEND_OPTIONS_INIT(
            &options,
            WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

        requestSent = WdfRequestSend(Request, Target, &options);
    }

    if (requestSent == FALSE) {
        status = WdfRequestGetStatus(Request);

        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DEVICE,
            "WdfRequestSend failed: 0x%x\n",
            status);

        WdfRequestComplete(Request, status);
    }

    return;
}

static
PVOID
USBPcapURBGetBufferPointer(
    ULONG length,
    PVOID buffer,
    PMDL  bufferMDL
)
{
    ASSERT((length == 0) ||
        ((length != 0) && (buffer != NULL || bufferMDL != NULL)));

    if (length == 0)
    {
        return NULL;
    }
    else if (buffer != NULL)
    {
        return buffer;
    }
    else if (bufferMDL != NULL)
    {
        PVOID address = MmGetSystemAddressForMdlSafe(bufferMDL,
            NormalPagePriority);
        return address;
    }
    else
    {
        return NULL;
    }
}

VOID
OnRequestCompletionRoutine(
    IN WDFREQUEST  Request,
    IN WDFIOTARGET  Target,
    IN PWDF_REQUEST_COMPLETION_PARAMS  Params,
    IN WDFCONTEXT  Context
)
/*++

Routine Description:

    Completion Routine

Arguments:

    Target - Target handle
    Request - Request handle
    Params - request completion params
    Context - Driver supplied context


Return Value:

    VOID

--*/
{
    NTSTATUS    status;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);
    status = Params->IoStatus.Status;
    PUCHAR buffer = NULL;
    const PIRP pIrp = WdfRequestWdmGetIrp(Request);
    const PURB pUrb = (PURB)URB_FROM_IRP(pIrp);

    const struct _URB_CONTROL_TRANSFER* pTransfer = &pUrb->UrbControlTransfer;
    const ULONG bufferLength = pTransfer->TransferBufferLength;
    buffer = (PUCHAR)USBPcapURBGetBufferPointer(
        pTransfer->TransferBufferLength,
        pTransfer->TransferBuffer,
        pTransfer->TransferBufferMDL
    );
    /*
    TraceEvents(
        TRACE_LEVEL_WARNING,
        TRACE_DEVICE,
        "Urb Function: 0x%llx, Length: 0x%llx, Status: 0x%llx",
        pUrb->UrbHeader.Function, pUrb->UrbHeader.Length, pUrb->UrbHeader.Status);
    */
    if (bufferLength == 0x104 && buffer[3] == 0x05) {
        buffer[3] = 0x04;
    }

    TraceEvents(
        TRACE_LEVEL_INFORMATION,
        TRACE_DEVICE,
        "OnRequestCompletionRoutine Finished %!STATUS!",
        status);

    WdfRequestComplete(Request, Params->IoStatus.Status);
}
