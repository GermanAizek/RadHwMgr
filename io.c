#include "radhwmgr.h"

BOOLEAN RadValidateIOPortAccess(
    USHORT Port,
    USHORT Width,
    BOOLEAN IsWrite
)
{
    UNREFERENCED_PARAMETER(Port);
    UNREFERENCED_PARAMETER(Width);
    UNREFERENCED_PARAMETER(IsWrite);
    return TRUE;
}

NTSTATUS RadGetIo(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PRADHWMGR_IO_REQ pReq = (PRADHWMGR_IO_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    USHORT port;

    if (pReq == NULL || inLen < sizeof(RADHWMGR_IO_REQ) || outLen < sizeof(RADHWMGR_IO_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    port = pReq->Port;

    if (!RadValidateIOPortAccess(port, pReq->Width, FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    switch (pReq->Width) {
    case 0: // 1 byte
        pReq->Data = (ULONG)READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)port);
        break;
    case 1: // 2 bytes
        pReq->Data = (ULONG)READ_PORT_USHORT((PUSHORT)(ULONG_PTR)port);
        break;
    case 2: // 4 bytes
        pReq->Data = READ_PORT_ULONG((PULONG)(ULONG_PTR)port);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Information = sizeof(RADHWMGR_IO_REQ);
    return STATUS_SUCCESS;
}

NTSTATUS RadSetIo(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PRADHWMGR_IO_REQ pReq = (PRADHWMGR_IO_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    USHORT port;

    if (pReq == NULL || inLen < sizeof(RADHWMGR_IO_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    port = pReq->Port;

    if (!RadValidateIOPortAccess(port, pReq->Width, TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    switch (pReq->Width) {
    case 0: // 1 byte
        WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)port, (UCHAR)(pReq->Data & 0xFF));
        break;
    case 1: // 2 bytes
        WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)port, (USHORT)(pReq->Data & 0xFFFF));
        break;
    case 2: // 4 bytes
        WRITE_PORT_ULONG((PULONG)(ULONG_PTR)port, pReq->Data);
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}
