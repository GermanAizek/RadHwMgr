# RadHwMgr

RadHwMgr (RadHwMgr.sys / Radiant Hardware Manager) - kernel mode driver used by Radiant Systems, Inc. for hardware management and platform diagnostics on P15xx platforms.

My opensource driver does not use the original source codes and is a reverse-engineered driver using [Ghidra](https://en.wikipedia.org/wiki/Ghidra) and [Radare2](https://en.wikipedia.org/wiki/Radare2). The rights have not been violated, and the new driver cannot be signed due to the EV certificate. I hope that the vulnerabilities can be closed using my security patches.

The main goal of the project is to recreate a secure driver with fixed vulnerabilities and errors made by past developers.

### Functions

Driver RadHwMgr.sys allows platform utility software to interact with system hardware and peripherals directly:

- Model Specific Register (MSR) read and write via rdmsr and wrmsr (IOCTL 0x9C406474, 0x9C406478)
- Direct physical memory mapping, reading, and writing via MmMapIoSpace (IOCTL 0x9C40242C, 0x9C406484)
- Direct x86 hardware I/O port input and output (IOCTL 0x9C406438, 0x9C40243C)
- PCI configuration space inspection and modification (IOCTL 0x9C406440, 0x9C402444)
- Platform version querying, diagnostics, and status monitoring (IOCTL 0x9C402404, 0x9C40240C, 0x9C402414)

Fixed vulnerabilities in opensource driver RadHwMgr.sys:

- [ ] (LOLDrivers: RadHwMgr.sys)[https://www.loldrivers.io/drivers/7bb4d807-9a66-48ff-9fb7-82780f3b015e/]
  - Low-privilege users can access MSRs, map arbitrary physical memory, read/write hardware I/O ports, and execute arbitrary code in kernel mode (BYOVD, MITRE ATT&CK T1068).

Fixed no publish vulnerabilities and bugs which contributors to this project found:

- [ ] Device deletion bypassed on symbolic link deletion failure ([view code](main.c))
- [ ] Arbitrary MSR read and write in commands 0x9C406474, 0x9C406478 ([view code](msr.c))
- [ ] Arbitrary physical memory read and write in commands 0x9C40242C, 0x9C406484 ([view code](mem.c))
- [ ] Unfiltered write to critical x86 I/O ports and controllers in commands 0x9C406438, 0x9C40243C ([view code](io.c))
- [ ] Arbitrary PCI configuration space modification in commands 0x9C406440, 0x9C402444 ([view code](pci.c))
- [ ] Device creation with weak permissions ([view code](main.c))

Authors:

- 2026 - Herman Semenoff <<GermanAizek@yandex.ru>> ([germanaizek.github.io](https://germanaizek.github.io))
- Radiant Systems, Inc. <<support@radiantsystems.com>>
