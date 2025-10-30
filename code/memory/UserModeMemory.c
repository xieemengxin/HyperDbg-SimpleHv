/**
 * @file UserModeMemory.c
 * @brief User-mode memory allocation implementation for R3 EPTHook trampolines
 * @details Allocates executable memory in target process's user space
 */

#include "pch.h"

/**
 * @brief Check if an address is in kernel space
 */
BOOLEAN
IsKernelAddress(PVOID Address)
{
    UINT64 addr = (UINT64)Address;
    // Kernel addresses start at 0xFFFF800000000000 on x64 Windows
    return (addr >= 0xFFFF800000000000);
}

/**
 * @brief Allocate executable memory in target process's user space
 * @details Uses ZwAllocateVirtualMemory to allocate memory in the target process
 *          with PAGE_EXECUTE_READWRITE permissions for trampoline code
 */
PVOID
AllocateUserModeTrampoline(HANDLE ProcessId, CR3_TYPE ProcessCr3, SIZE_T Size)
{
    NTSTATUS    status;
    PVOID       baseAddress = NULL;
    SIZE_T      regionSize = Size;
    PEPROCESS   process = NULL;
    KAPC_STATE  apcState;
    HANDLE      processHandle = NULL;

    UNREFERENCED_PARAMETER(ProcessCr3);

    SimpleHvLog("[UserModeMemory] Allocating R3 trampoline for PID %d, Size: %d", ProcessId, (UINT32)Size);

    //
    // Get the process object from PID
    //
    status = PsLookupProcessByProcessId(ProcessId, &process);
    if (!NT_SUCCESS(status))
    {
        SimpleHvLogError("[UserModeMemory] Failed to get process object for PID %d: 0x%x", ProcessId, status);
        return NULL;
    }

    //
    // Attach to the target process context
    //
    KeStackAttachProcess(process, &apcState);

    //
    // Get a handle to the current process (while attached)
    //
    processHandle = ZwCurrentProcess();

    //
    // Allocate executable memory in user space
    //
    status = ZwAllocateVirtualMemory(
        processHandle,                  // Process handle
        &baseAddress,                   // Base address (NULL = system chooses)
        0,                              // ZeroBits
        &regionSize,                    // Size
        MEM_COMMIT | MEM_RESERVE,      // Allocation type
        PAGE_EXECUTE_READWRITE          // Protection (executable + read/write)
    );

    //
    // Detach from the process
    //
    KeUnstackDetachProcess(&apcState);

    //
    // Dereference the process object
    //
    ObDereferenceObject(process);

    if (!NT_SUCCESS(status))
    {
        SimpleHvLogError("[UserModeMemory] Failed to allocate virtual memory: 0x%x", status);
        return NULL;
    }

    SimpleHvLog("[UserModeMemory] Successfully allocated R3 trampoline at 0x%llx", baseAddress);

    //
    // Verify it's a user-mode address
    //
    if (IsKernelAddress(baseAddress))
    {
        SimpleHvLogError("[UserModeMemory] ERROR: Allocated address is in kernel space! 0x%llx", baseAddress);
        // This should never happen with ZwAllocateVirtualMemory
        FreeUserModeTrampoline(baseAddress, ProcessId, regionSize);
        return NULL;
    }

    return baseAddress;
}

/**
 * @brief Free user-mode trampoline memory
 * @details Frees memory allocated by AllocateUserModeTrampoline
 */
VOID
FreeUserModeTrampoline(PVOID TrampolineAddress, HANDLE ProcessId, SIZE_T Size)
{
    NTSTATUS    status;
    PEPROCESS   process = NULL;
    KAPC_STATE  apcState;
    PVOID       baseAddress = TrampolineAddress;
    SIZE_T      regionSize = 0;  // 0 means free entire region
    HANDLE      processHandle = NULL;

    UNREFERENCED_PARAMETER(Size);

    if (TrampolineAddress == NULL)
    {
        return;
    }

    SimpleHvLog("[UserModeMemory] Freeing R3 trampoline at 0x%llx for PID %d", TrampolineAddress, ProcessId);

    //
    // Get the process object
    //
    status = PsLookupProcessByProcessId(ProcessId, &process);
    if (!NT_SUCCESS(status))
    {
        SimpleHvLogError("[UserModeMemory] Failed to get process object for PID %d: 0x%x", ProcessId, status);
        return;
    }

    //
    // Attach to the target process
    //
    KeStackAttachProcess(process, &apcState);

    //
    // Get a handle to the current process (while attached)
    //
    processHandle = ZwCurrentProcess();

    //
    // Free the virtual memory
    //
    status = ZwFreeVirtualMemory(
        processHandle,      // Process handle
        &baseAddress,       // Base address
        &regionSize,        // Size (0 = free entire region)
        MEM_RELEASE         // Free type
    );

    //
    // Detach from the process
    //
    KeUnstackDetachProcess(&apcState);

    //
    // Dereference the process object
    //
    ObDereferenceObject(process);

    if (!NT_SUCCESS(status))
    {
        SimpleHvLogError("[UserModeMemory] Failed to free virtual memory at 0x%llx: 0x%x", TrampolineAddress, status);
    }
    else
    {
        SimpleHvLog("[UserModeMemory] Successfully freed R3 trampoline");
    }
}