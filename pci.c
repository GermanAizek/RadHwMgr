#include "radhwmgr.h"

//
// Validation of PCI configuration access to prevent arbitrary hardware disruption
//
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

    // Max device is 31, function is 7
    if (Slot > 31 || Function > 7) {
        return FALSE;
    }

    if (Length == 0 || Length > 4) {
        return FALSE;
    }

    if ((ULONG)Offset + Length > 0x100) {
        return FALSE;
    }

    if (IsWrite) {
        ULONG start = Offset;
        ULONG end = Offset + Length;

        // Block writing to Vendor ID, Device ID (0x00 - 0x03)
        if (start < 0x04) {
            return FALSE;
        }

        // Block writing to Command & Status Register (0x04 - 0x07)
        if (start < 0x08 && end > 0x04) {
            return FALSE;
        }

        // Block writing to Class Code & Revision (0x08 - 0x0B)
        if (start < 0x0C && end > 0x08) {
            return FALSE;
        }

        // Block writing to Base Address Registers BAR0-BAR5 (0x10 - 0x27)
        if (start < 0x28 && end > 0x10) {
            return FALSE;
        }
    }

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

    // FIX: Validate PCI parameters and restrict dangerous access
    if (!RadValidatePciAccess(pReq->Bus, pReq->Slot, pReq->Function, pReq->Offset, sizeof(ULONG), FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    slotDevFunc = (pReq->Slot & 0x1F) | ((pReq->Function & 0x7) << 5);
    pReq->Data = 0;

    __try {
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
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
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

    // FIX: Validate PCI parameters and restrict dangerous register writes (BAR, Command)
    if (!RadValidatePciAccess(pReq->Bus, pReq->Slot, pReq->Function, pReq->Offset, sizeof(ULONG), TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    slotDevFunc = (pReq->Slot & 0x1F) | ((pReq->Function & 0x7) << 5);

    __try {
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
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}
