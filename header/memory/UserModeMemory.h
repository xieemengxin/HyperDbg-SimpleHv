/**
 * @file UserModeMemory.h
 * @brief User-mode memory allocation for R3 EPTHook trampolines
 * @details Provides functions to allocate and free executable memory in user space
 */

#pragma once

/**
 * @brief Allocate executable memory in target process's user space for R3 trampoline
 *
 * @param ProcessId Target process ID
 * @param ProcessCr3 Target process CR3 (for validation)
 * @param Size Size of memory to allocate
 * @return User-mode address of allocated memory, or NULL on failure
 */
PVOID
AllocateUserModeTrampoline(HANDLE ProcessId, CR3_TYPE ProcessCr3, SIZE_T Size);

/**
 * @brief Free user-mode trampoline memory
 *
 * @param TrampolineAddress Address of trampoline to free
 * @param ProcessId Process ID where memory was allocated
 * @param Size Size of memory to free
 */
VOID
FreeUserModeTrampoline(PVOID TrampolineAddress, HANDLE ProcessId, SIZE_T Size);

/**
 * @brief Check if an address is in kernel space
 *
 * @param Address Address to check
 * @return TRUE if kernel address, FALSE if user address
 */
BOOLEAN
IsKernelAddress(PVOID Address);