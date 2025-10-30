/**
 * @file DeviceIoctl.h
 * @brief SimpleHv IOCTL Definitions
 * @details Defines IOCTL codes and structures for user-kernel communication
 */

#pragma once

#include <ntddk.h>

//////////////////////////////////////////////////
//            IOCTL Code Definitions            //
//////////////////////////////////////////////////

// Device type (custom device)
#define FILE_DEVICE_SIMPLEHV    0x8000

// IOCTL Codes
#define IOCTL_SIMPLEHV_PING \
    CTL_CODE(FILE_DEVICE_SIMPLEHV, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SIMPLEHV_INSTALL_TEST_HOOKS \
    CTL_CODE(FILE_DEVICE_SIMPLEHV, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SIMPLEHV_UNHOOK_ALL \
    CTL_CODE(FILE_DEVICE_SIMPLEHV, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SIMPLEHV_INSTALL_R3_HOOK \
    CTL_CODE(FILE_DEVICE_SIMPLEHV, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

//////////////////////////////////////////////////
//            Data Structures                   //
//////////////////////////////////////////////////

/**
 * @brief Ping response structure
 */
typedef struct _SIMPLEHV_PING_RESPONSE {
    UINT64 Signature;       // Should be HYPERCALL_SIGN
    UINT32 NumProcessors;   // Number of logical processors
    BOOLEAN IsRunning;      // Is hypervisor running
} SIMPLEHV_PING_RESPONSE, *PSIMPLEHV_PING_RESPONSE;

/**
 * @brief Install test hooks response
 */
typedef struct _SIMPLEHV_INSTALL_HOOKS_RESPONSE {
    NTSTATUS Status;        // Installation status
    UINT32 HooksInstalled;  // Number of hooks installed
} SIMPLEHV_INSTALL_HOOKS_RESPONSE, *PSIMPLEHV_INSTALL_HOOKS_RESPONSE;

/**
 * @brief R3 EPTHook request structure
 */
typedef struct _SIMPLEHV_R3_HOOK_REQUEST {
    PVOID TargetAddress;    // R3 target function address (e.g., MessageBoxW)
    PVOID HookFunction;     // R3 hook function address
    UINT32 ProcessId;       // Target process ID
} SIMPLEHV_R3_HOOK_REQUEST, *PSIMPLEHV_R3_HOOK_REQUEST;

/**
 * @brief R3 EPTHook response structure
 */
typedef struct _SIMPLEHV_R3_HOOK_RESPONSE {
    NTSTATUS Status;        // Installation status
    PVOID Trampoline;       // Returned trampoline address for calling original function
} SIMPLEHV_R3_HOOK_RESPONSE, *PSIMPLEHV_R3_HOOK_RESPONSE;

//////////////////////////////////////////////////
//            Function Declarations             //
//////////////////////////////////////////////////

/**
 * @brief Handle IRP_MJ_CREATE
 */
NTSTATUS DeviceIoctlCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/**
 * @brief Handle IRP_MJ_CLOSE
 */
NTSTATUS DeviceIoctlClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/**
 * @brief Handle IRP_MJ_DEVICE_CONTROL
 */
NTSTATUS DeviceIoctlDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);
