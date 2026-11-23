#include "radhwmgr.h"

BOOLEAN RadValidatePhysicalAddress(
    ULONG_PTR Address,
    SIZE_T Length
)
{
    // Check for overflow
    if (Address + Length < Address) {
        return FALSE;
    }

    // Limit maximum transfer size (64KB)
    if (Length == 0 || Length > 0x10000) {
        return FALSE;
    }

    // Block lower physical memory containing IVT, BDA, and kernel zero-page
    if (Address < 0x000A0000) {
        return FALSE;
    }

    // Allow BIOS / Option ROM / Video RAM area: 0x000A0000 - 0x00100000
    if (Address >= 0x000A0000 && (Address + Length) <= 0x00100000) {
        return TRUE;
    }

    // Allow hardware MMIO address space (PCI BARs / hardware registers)
    if (Address >= 0xC0000000) {
        return TRUE;
    }

    // Block arbitrary system RAM access
    return FALSE;
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
    NTSTATUS status = STATUS_SUCCESS;

    if (pReq == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pReq->PhysicalAddress;
    length = pReq->Length;

    if (length == 0 || outLen < length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // FIX: Restrict arbitrary physical memory read to safe hardware MMIO and BIOS ranges
    if (!RadValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, pMapped, length);
            Irp->IoStatus.Information = length;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(pMapped, length);
    }

    return status;
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
    NTSTATUS status = STATUS_SUCCESS;

    if (pReq == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pReq->PhysicalAddress;
    length = pReq->Length;

    if (length == 0 || inLen < sizeof(ULONG) * 2 + length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // FIX: Restrict arbitrary physical memory write to safe hardware MMIO ranges
    if (!RadValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            RtlCopyMemory(pMapped, pReq->Buffer, length);
            Irp->IoStatus.Information = 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(pMapped, length);
    }

    return status;
}
