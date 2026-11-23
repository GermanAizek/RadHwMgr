#include "radhwmgr.h"

BOOLEAN RadValidateMsrAccess(
    ULONG Msr,
    BOOLEAN IsWrite
)
{
    UNREFERENCED_PARAMETER(Msr);
    UNREFERENCED_PARAMETER(IsWrite);
    return TRUE;
}

NTSTATUS RadGetMsr(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PRADHWMGR_MSR_REQ pReq = (PRADHWMGR_MSR_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG64 msrValue;

    if (pReq == NULL || inLen < sizeof(RADHWMGR_MSR_REQ) || outLen < sizeof(RADHWMGR_MSR_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!RadValidateMsrAccess(pReq->Register, FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    msrValue = __readmsr(pReq->Register);
    pReq->HighPart = (ULONG)(msrValue >> 32);
    pReq->LowPart = (ULONG)(msrValue & 0xFFFFFFFF);

    Irp->IoStatus.Information = sizeof(RADHWMGR_MSR_REQ);
    return STATUS_SUCCESS;
}

NTSTATUS RadSetMsr(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PRADHWMGR_MSR_REQ pReq = (PRADHWMGR_MSR_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG64 msrValue;

    if (pReq == NULL || inLen < sizeof(RADHWMGR_MSR_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!RadValidateMsrAccess(pReq->Register, TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    msrValue = ((ULONG64)pReq->HighPart << 32) | (ULONG64)pReq->LowPart;
    __writemsr(pReq->Register, msrValue);

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}
