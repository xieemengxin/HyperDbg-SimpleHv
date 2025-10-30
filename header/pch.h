/**
 * @file pch.h
 * @author Your Name
 * @brief Precompiled Header for SimpleHv
 * @details Simple Hypervisor for learning Intel VT-x (based on HyperDbg architecture)
 * @version 0.1
 * @date 2025-10-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

 // ����Evade����
#    define DISABLE_HYPERDBG_HYPEREVADE FALSE


//
// ========================================
// Windows Kernel Headers
// ========================================
//
#include <ntifs.h>
#include <wdf.h>
#include <intrin.h>
#include <minwindef.h>


//
// Definition of Intel primitives (External header)
//
#include "ia32-doc/out/ia32.h"

//
// HyperDbg SDK headers
//
#include "SDK/HyperDbgSdk.h"

//
// HyperDbg Kernel-mode headers
//
#include "config/Configuration.h"
#include "macros/MetaMacros.h"

//
// Platform independent headers
//
#include "platform/kernel/header/Mem.h"

//
// VMM Callbacks
//
#include "SDK/modules/VMM.h"

//
// The core's state
//
#include "common/State.h"

//
// VMX and EPT Types
//
#include "vmm/vmx/Vmx.h"
#include "vmm/vmx/VmxRegions.h"
#include "vmm/ept/Ept.h"
#include "SDK/imports/kernel/HyperDbgVmmImports.h"

//
// Hyper-V TLFS
//
#include "hyper-v/HypervTlfs.h"

//
// VMX and Capabilities
//
#include "vmm/vmx/VmxBroadcast.h"
#include "memory/MemoryMapper.h"
#include "interface/Dispatch.h"
#include "common/Dpc.h"
#include "common/Msr.h"
#include "memory/PoolManager.h"
#include "common/Trace.h"
#include "assembly/InlineAsm.h"
#include "vmm/ept/Vpid.h"
#include "memory/Conversion.h"
#include "memory/Layout.h"
#include "memory/SwitchLayout.h"
#include "memory/AddressCheck.h"
#include "memory/Segmentation.h"
#include "common/Bitwise.h"
#include "common/Common.h"
#include "vmm/vmx/Events.h"
#include "devices/Apic.h"
#include "devices/Pci.h"
#include "processor/Smm.h"
#include "processor/Idt.h"
#include "vmm/vmx/Mtf.h"
#include "vmm/vmx/Counters.h"
#include "vmm/vmx/IdtEmulation.h"
#include "vmm/ept/Invept.h"
#include "vmm/vmx/Vmcall.h"
#include "interface/DirectVmcall.h"
#include "vmm/vmx/Hv.h"
#include "vmm/vmx/MsrHandlers.h"
#include "vmm/vmx/ProtectedHv.h"
#include "vmm/vmx/IoHandler.h"
#include "vmm/vmx/VmxMechanisms.h"
#include "hooks/Hooks.h"
#include "hooks/ModeBasedExecHook.h"
#include "hooks/SyscallCallback.h"
#include "interface/Callback.h"
#include "features/DirtyLogging.h"
#include "features/CompatibilityChecks.h"
#include "mmio/MmioShadowing.h"

#include "interface/HyperCall.h"

//
// Disassembler Header
//
#include "Zydis/Zydis.h"
#include "disassembler/Disassembler.h"

//
// Broadcast headers
//
#include "broadcast/Broadcast.h"
#include "broadcast/DpcRoutines.h"

//
// Headers for supporting the reversing machine (TRM)
//
#include "hooks/ExecTrap.h"

//
// Headers for user-mode memory management
//
#include "memory/UserModeMemory.h"

//
// Headers for exporting functions to remove the driver
//
//#include "common/UnloadDll.h"

//
// Optimization algorithms
//
#include "components/optimizations/header/AvlTree.h"
#include "components/optimizations/header/BinarySearch.h"
#include "components/optimizations/header/InsertionSort.h"

//
// Spinlocks
//
#include "components/spinlock/header/Spinlock.h"

//
// Global Variables should be the last header to include
//
#include "globals/GlobalVariableManagement.h"
#include "globals/GlobalVariables.h"

//
// HyperLog Module
//
#include "SDK/modules/HyperLog.h"
//#include "SDK/imports/kernel/HyperDbgHyperLogIntrinsics.h"
#include "components/interface/HyperLogCallback.h"

//
// Transparent-mode (hyperevade) headers
//
#include "SDK/modules/HyperEvade.h"
#include "SDK/imports/kernel/HyperDbgHyperEvade.h"


//
// Transparency and footprints headers
//
#include "transparency/Transparency.h"
#include "transparency/VmxFootprints.h"
#include "transparency/SyscallFootprints.h"

// Debugger
#include "globals/globals.h"
#include "SDK/headers/Events.h"

//
// ========================================
// Debug Macros
// ========================================
//
#if DBG
#define SimpleHvLog(Format, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, \
                "[SimpleHv] " Format "\n", ##__VA_ARGS__)
#define SimpleHvLogError(Format, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, \
                "[SimpleHv] ERROR: " Format "\n", ##__VA_ARGS__)
#define SimpleHvLogWarning(Format, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, \
                "[SimpleHv] WARNING: " Format "\n", ##__VA_ARGS__)
#else
#define SimpleHvLog(Format, ...)
#define SimpleHvLogError(Format, ...)
#define SimpleHvLogWarning(Format, ...)
#endif


// Loader and test
#include "loader/Loader.h"
#include "test.h"
#include "interface/TriggerEvents.h"

// IOCTL interface
#include "interface/DeviceIoctl.h"