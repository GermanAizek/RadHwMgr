#include "radhwmgr.h"

BOOLEAN RadValidatePhysicalAddress(
    ULONG_PTR Address,
    SIZE_T Length
)
{
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(Length);
    return TRUE;
}

NTSTATUS RadReadPhysicalMemory(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PRADHWMGR_MEM_REQ pReq = (PRADHWMGR_MEM_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
    PVOID pMapped = NULL;

    if (pReq == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pReq->PhysicalAddress;
    length = pReq->Length;

    if (length == 0 || outLen < length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!RadValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, pMapped, length);
    MmUnmapIoSpace(pMapped, length);

    Irp->IoStatus.Information = length;
    return STATUS_SUCCESS;
}

NTSTATUS RadWritePhysicalMemory(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PRADHWMGR_MEM_REQ pReq = (PRADHWMGR_MEM_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
    PVOID pMapped = NULL;

    if (pReq == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pReq->PhysicalAddress;
    length = pReq->Length;

    if (length == 0 || inLen < sizeof(ULONG) * 2 + length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!RadValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(pMapped, pReq->Buffer, length);
    MmUnmapIoSpace(pMapped, length);

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}
