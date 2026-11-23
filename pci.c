#include "radhwmgr.h"

BOOLEAN RadValidatePciAccess(
    UCHAR Bus,
    UCHAR Slot,
    UCHAR Function,
    UCHAR Offset,
    ULONG Length,
    BOOLEAN IsWrite
)
{
    UNREFERENCED_PARAMETER(Bus);
    UNREFERENCED_PARAMETER(Slot);
    UNREFERENCED_PARAMETER(Function);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(IsWrite);
    return TRUE;
}

NTSTATUS RadReadPci(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PRADHWMGR_PCI_REQ pReq = (PRADHWMGR_PCI_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG slotDevFunc;
    ULONG bytesProcessed;

    if (pReq == NULL || inLen < sizeof(RADHWMGR_PCI_REQ) || outLen < sizeof(RADHWMGR_PCI_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!RadValidatePciAccess(pReq->Bus, pReq->Slot, pReq->Function, pReq->Offset, sizeof(ULONG), FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    slotDevFunc = (pReq->Slot & 0x1F) | ((pReq->Function & 0x7) << 5);
    pReq->Data = 0;

    bytesProcessed = HalGetBusDataByOffset(
        PCIConfiguration,
        pReq->Bus,
        slotDevFunc,
        &pReq->Data,
        pReq->Offset,
        sizeof(ULONG)
    );

    if (bytesProcessed == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    Irp->IoStatus.Information = sizeof(RADHWMGR_PCI_REQ);
    return STATUS_SUCCESS;
}

NTSTATUS RadWritePci(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PRADHWMGR_PCI_REQ pReq = (PRADHWMGR_PCI_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG slotDevFunc;
    ULONG bytesProcessed;

    if (pReq == NULL || inLen < sizeof(RADHWMGR_PCI_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!RadValidatePciAccess(pReq->Bus, pReq->Slot, pReq->Function, pReq->Offset, sizeof(ULONG), TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    slotDevFunc = (pReq->Slot & 0x1F) | ((pReq->Function & 0x7) << 5);

    bytesProcessed = HalSetBusDataByOffset(
        PCIConfiguration,
        pReq->Bus,
        slotDevFunc,
        &pReq->Data,
        pReq->Offset,
        sizeof(ULONG)
    );

    if (bytesProcessed == 0) {
        return STATUS_UNSUCCESSFUL;
    }

    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
}
