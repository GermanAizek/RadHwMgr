#pragma once

#include <ntddk.h>
#include <wdm.h>
#include <wdmsec.h>
#include <ntstrsafe.h>

#define RADHWMGR_DEVICE_NAME        L"\\Device\\RadHwMgr"
#define RADHWMGR_DOS_DEVICE_NAME    L"\\DosDevices\\RadHwMgrDev"
#define RADHWMGR_DEVICE_TYPE        40000       // 0x9C40

#define TAG_RADHWMGR_POOL           'MdaR'      // 0x5261644D ('RadM')

//
// IOCTL definitions from RadHwMgr.sys
//
// MSR Access
#define IOCTL_RADHWMGR_GET_MSR      0x9C406474  // GetModelSpecificRegister
#define IOCTL_RADHWMGR_SET_MSR      0x9C406478  // SetModelSpecificRegister

// Physical Memory Access
#define IOCTL_RADHWMGR_READ_MEM     0x9C40242C  // ReadSystemMemoryLocation
#define IOCTL_RADHWMGR_WRITE_MEM    0x9C406484  // WriteSystemMemoryLocation

// Port I/O Access
#define IOCTL_RADHWMGR_READ_IO      0x9C406438  // GetIO
#define IOCTL_RADHWMGR_WRITE_IO     0x9C40243C  // SetIO

// PCI Configuration Access
#define IOCTL_RADHWMGR_READ_PCI     0x9C406440  // ReadPciConfigSpace
#define IOCTL_RADHWMGR_WRITE_PCI    0x9C402444  // WritePciConfigSpace

// General / Board Management
#define IOCTL_RADHWMGR_GET_VERSION  0x9C402404
#define IOCTL_RADHWMGR_NOP          0x9C40240C
#define IOCTL_RADHWMGR_GET_INFO     0x9C402414

#pragma pack(push, 1)

typedef struct _RADHWMGR_MSR_REQ {
    ULONG Register;  // 0x00: MSR Register Index
    ULONG HighPart;  // 0x04: High 32 bits (EDX)
    ULONG LowPart;   // 0x08: Low 32 bits (EAX)
} RADHWMGR_MSR_REQ, *PRADHWMGR_MSR_REQ;

typedef struct _RADHWMGR_MEM_REQ {
    ULONG PhysicalAddress; // 0x00: Base physical address
    ULONG Length;          // 0x04: Number of bytes to transfer
    UCHAR Buffer[1];       // 0x08: Data buffer
} RADHWMGR_MEM_REQ, *PRADHWMGR_MEM_REQ;

typedef struct _RADHWMGR_IO_REQ {
    USHORT Port;     // 0x00: Port address
    USHORT Width;    // 0x02: 0 = byte (1), 1 = word (2), 2 = dword (4)
    ULONG Data;      // 0x04: Data to read / write
} RADHWMGR_IO_REQ, *PRADHWMGR_IO_REQ;

typedef struct _RADHWMGR_PCI_REQ {
    UCHAR Bus;       // 0x00: PCI Bus number
    UCHAR Slot;      // 0x01: PCI Device / Slot
    UCHAR Function;  // 0x02: PCI Function number
    UCHAR Offset;    // 0x03: PCI Register offset
    ULONG Data;      // 0x04: Data to read / write
} RADHWMGR_PCI_REQ, *PRADHWMGR_PCI_REQ;

typedef struct _RADHWMGR_DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
    IO_REMOVE_LOCK RemoveLock;
    BOOLEAN IsStarted;
    BOOLEAN LinkCreated;
    UNICODE_STRING DeviceName;
    UNICODE_STRING DosDeviceName;
} RADHWMGR_DEVICE_EXTENSION, *PRADHWMGR_DEVICE_EXTENSION;

#pragma pack(pop)

//
// Global state
//
extern PDEVICE_OBJECT g_DeviceObject;
extern PDRIVER_OBJECT g_DriverObject;

//
// Function prototypes
//

// main.c
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD RadUnload;
DRIVER_DISPATCH RadCreateClose;
DRIVER_DISPATCH RadDispatchDeviceControl;

// msr.c
NTSTATUS RadGetMsr(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS RadSetMsr(PIRP Irp, PIO_STACK_LOCATION irpSp);
BOOLEAN RadValidateMsrAccess(ULONG Msr, BOOLEAN IsWrite);

// mem.c
NTSTATUS RadReadPhysicalMemory(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS RadWritePhysicalMemory(PIRP Irp, PIO_STACK_LOCATION irpSp);
BOOLEAN RadValidatePhysicalAddress(ULONG_PTR Address, SIZE_T Length);

// io.c
NTSTATUS RadGetIo(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS RadSetIo(PIRP Irp, PIO_STACK_LOCATION irpSp);
BOOLEAN RadValidateIOPortAccess(USHORT Port, USHORT Width, BOOLEAN IsWrite);

// pci.c
NTSTATUS RadReadPci(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS RadWritePci(PIRP Irp, PIO_STACK_LOCATION irpSp);
BOOLEAN RadValidatePciAccess(UCHAR Bus, UCHAR Slot, UCHAR Function, UCHAR Offset, ULONG Length, BOOLEAN IsWrite);
