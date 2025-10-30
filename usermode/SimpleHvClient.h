/**
 * @file SimpleHvClient.h
 * @brief SimpleHv User Mode Client - IOCTL Interface
 * @details Provides C++ wrapper for communicating with SimpleHv driver via IOCTL
 */

#pragma once

#include <Windows.h>
#include <stdio.h>
#include <stdint.h>

//////////////////////////////////////////////////
//            IOCTL Code Definitions            //
//////////////////////////////////////////////////

#define FILE_DEVICE_SIMPLEHV    0x8000

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

typedef struct _SIMPLEHV_PING_RESPONSE {
    UINT64 Signature;
    UINT32 NumProcessors;
    BOOLEAN IsRunning;
} SIMPLEHV_PING_RESPONSE, *PSIMPLEHV_PING_RESPONSE;

typedef struct _SIMPLEHV_INSTALL_HOOKS_RESPONSE {
    UINT32 Status;          // NTSTATUS
    UINT32 HooksInstalled;
} SIMPLEHV_INSTALL_HOOKS_RESPONSE, *PSIMPLEHV_INSTALL_HOOKS_RESPONSE;

typedef struct _SIMPLEHV_R3_HOOK_REQUEST {
    PVOID TargetAddress;    // R3 target function address (e.g., MessageBoxW)
    PVOID HookFunction;     // R3 hook function address
    UINT32 ProcessId;       // Target process ID
} SIMPLEHV_R3_HOOK_REQUEST, *PSIMPLEHV_R3_HOOK_REQUEST;

typedef struct _SIMPLEHV_R3_HOOK_RESPONSE {
    UINT32 Status;          // NTSTATUS
    PVOID Trampoline;       // Returned trampoline address
} SIMPLEHV_R3_HOOK_RESPONSE, *PSIMPLEHV_R3_HOOK_RESPONSE;

//////////////////////////////////////////////////
//            SimpleHv Client Class             //
//////////////////////////////////////////////////

namespace SimpleHv {

class Client {
private:
    HANDLE hDevice;

public:
    Client() : hDevice(INVALID_HANDLE_VALUE) {}

    ~Client() {
        Close();
    }

    /**
     * @brief Open connection to SimpleHv driver
     */
    bool Open() {
        if (hDevice != INVALID_HANDLE_VALUE) {
            return true; // Already opened
        }

        hDevice = CreateFileW(
            L"\\\\.\\SimpleHv",
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        return (hDevice != INVALID_HANDLE_VALUE);
    }

    /**
     * @brief Close connection
     */
    void Close() {
        if (hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(hDevice);
            hDevice = INVALID_HANDLE_VALUE;
        }
    }

    /**
     * @brief Check if connected
     */
    bool IsConnected() const {
        return (hDevice != INVALID_HANDLE_VALUE);
    }

    /**
     * @brief Ping hypervisor
     */
    bool Ping(SIMPLEHV_PING_RESPONSE* response) {
        if (!IsConnected()) return false;

        DWORD bytesReturned;
        return DeviceIoControl(
            hDevice,
            IOCTL_SIMPLEHV_PING,
            NULL,
            0,
            response,
            sizeof(SIMPLEHV_PING_RESPONSE),
            &bytesReturned,
            NULL
        ) != FALSE;
    }

    /**
     * @brief Install test hooks
     */
    bool InstallTestHooks(SIMPLEHV_INSTALL_HOOKS_RESPONSE* response) {
        if (!IsConnected()) return false;

        DWORD bytesReturned;
        return DeviceIoControl(
            hDevice,
            IOCTL_SIMPLEHV_INSTALL_TEST_HOOKS,
            NULL,
            0,
            response,
            sizeof(SIMPLEHV_INSTALL_HOOKS_RESPONSE),
            &bytesReturned,
            NULL
        ) != FALSE;
    }

    /**
     * @brief Unhook all
     */
    bool UnhookAll() {
        if (!IsConnected()) return false;

        DWORD bytesReturned;
        return DeviceIoControl(
            hDevice,
            IOCTL_SIMPLEHV_UNHOOK_ALL,
            NULL,
            0,
            NULL,
            0,
            &bytesReturned,
            NULL
        ) != FALSE;
    }

    /**
     * @brief Install R3 EPTHook
     */
    bool InstallR3Hook(SIMPLEHV_R3_HOOK_REQUEST* request, SIMPLEHV_R3_HOOK_RESPONSE* response) {
        if (!IsConnected()) return false;

        DWORD bytesReturned;
        return DeviceIoControl(
            hDevice,
            IOCTL_SIMPLEHV_INSTALL_R3_HOOK,
            request,
            sizeof(SIMPLEHV_R3_HOOK_REQUEST),
            response,
            sizeof(SIMPLEHV_R3_HOOK_RESPONSE),
            &bytesReturned,
            NULL
        ) != FALSE;
    }
};

} // namespace SimpleHv
