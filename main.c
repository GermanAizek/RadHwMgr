#include "radhwmgr.h"

// Device Class GUID for RadHwMgr (LOLDrivers: 7bb4d807-9a66-48ff-9fb7-82780f3b015e)
static const GUID GUID_DEVCLASS_RADHWMGR = {
    0x7BB4D807, 0x9A66, 0x48FF, { 0x9F, 0xB7, 0x82, 0x78, 0x0F, 0x3B, 0x01, 0x5E }
};

PDEVICE_OBJECT g_DeviceObject = NULL;
PDRIVER_OBJECT g_DriverObject = NULL;

NTSTATUS RadCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID RadUnload(
    PDRIVER_OBJECT DriverObject
)
{
    UNICODE_STRING dosDeviceName;

    if (g_DeviceObject != NULL) {
        RtlInitUnicodeString(&dosDeviceName, RADHWMGR_DOS_DEVICE_NAME);
        IoDeleteSymbolicLink(&dosDeviceName);

        // FIX: Unconditionally delete device object to prevent resource leak on unload
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
    }

    UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS RadDispatchDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION irpSp;
    ULONG ioctlCode;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR information = 0;
    PVOID buffer;
    ULONG inLength;
    ULONG outLength;

    UNREFERENCED_PARAMETER(DeviceObject);

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    ioctlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    buffer = Irp->AssociatedIrp.SystemBuffer;
    inLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    UNREFERENCED_PARAMETER(inLength);

    switch (ioctlCode) {
    case IOCTL_RADHWMGR_GET_VERSION:
        if (buffer != NULL && outLength >= sizeof(ULONG)) {
            *(PULONG)buffer = 0x03010016; // Version 3.1.0.22
            information = sizeof(ULONG);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_RADHWMGR_NOP:
        status = STATUS_SUCCESS;
        information = 0;
        break;

    case IOCTL_RADHWMGR_GET_INFO:
        if (buffer != NULL && outLength >= sizeof(ULONG)) {
            *(PULONG)buffer = 0x00000001;
            information = sizeof(ULONG);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    // MSR Access
    case IOCTL_RADHWMGR_GET_MSR:
        status = RadGetMsr(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_RADHWMGR_SET_MSR:
        status = RadSetMsr(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    // Physical Memory Access
    case IOCTL_RADHWMGR_READ_MEM:
        status = RadReadPhysicalMemory(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_RADHWMGR_WRITE_MEM:
        status = RadWritePhysicalMemory(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    // Port I/O Access
    case IOCTL_RADHWMGR_READ_IO:
        status = RadGetIo(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_RADHWMGR_WRITE_IO:
        status = RadSetIo(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    // PCI Configuration Access
    case IOCTL_RADHWMGR_READ_PCI:
        status = RadReadPci(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_RADHWMGR_WRITE_PCI:
        status = RadWritePci(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        information = 0;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING dosDeviceName;
    UNICODE_STRING sddl;
    PRADHWMGR_DEVICE_EXTENSION devExt;

    UNREFERENCED_PARAMETER(RegistryPath);

    g_DriverObject = DriverObject;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = RadCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = RadCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RadDispatchDeviceControl;
    DriverObject->DriverUnload = RadUnload;

    RtlInitUnicodeString(&deviceName, RADHWMGR_DEVICE_NAME);
    RtlInitUnicodeString(&dosDeviceName, RADHWMGR_DOS_DEVICE_NAME);

    // FIX: Secure device creation to mitigate unprivileged access (LOLDrivers: RadHwMgr.sys, 7bb4d807-9a66-48ff-9fb7-82780f3b015e)
    // SDDL: D:P(A;;GA;;;SY)(A;;GA;;;BA)
    RtlInitUnicodeString(&sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");

    status = IoCreateDeviceSecure(
        DriverObject,
        sizeof(RADHWMGR_DEVICE_EXTENSION),
        &deviceName,
        RADHWMGR_DEVICE_TYPE,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &sddl,
        &GUID_DEVCLASS_RADHWMGR,
        &g_DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoCreateSymbolicLink(&dosDeviceName, &deviceName);
    if (!NT_SUCCESS(status)) {
        // FIX: Delete device object on symbolic link creation failure
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        return status;
    }

    devExt = (PRADHWMGR_DEVICE_EXTENSION)g_DeviceObject->DeviceExtension;
    RtlZeroMemory(devExt, sizeof(RADHWMGR_DEVICE_EXTENSION));
    devExt->DeviceObject = g_DeviceObject;
    devExt->DriverObject = DriverObject;
    IoInitializeRemoveLock(&devExt->RemoveLock, TAG_RADHWMGR_POOL, 1, 20);
    devExt->IsStarted = TRUE;
    devExt->LinkCreated = TRUE;

    return STATUS_SUCCESS;
}
