/**
 * @file R3EptHookTest.cpp
 * @brief R3 EPTHook Test Program - MessageBoxW Hook via EPT
 * @details
 *     This test demonstrates how to use SimpleHv's EPTHook to hook user-mode functions.
 *     The hook function is defined in user mode and EPTHook is installed via driver IOCTL.
 *     Uses double-pointer mechanism to safely receive trampoline address.
 */

#include <Windows.h>
#include <iostream>
#include <string>
#include "SimpleHvClient.h"

// ========================================
// Type Definitions
// ========================================

typedef int (WINAPI* fnMessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
typedef int (WINAPI* fnMessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

// ========================================
// Global Variables - Trampolines
// ========================================

// These will store the trampoline addresses returned by the driver
fnMessageBoxW g_OriginalMessageBoxW = nullptr;
fnMessageBoxA g_OriginalMessageBoxA = nullptr;

// ========================================
// R3 Hook Functions (Execute in User Mode)
// ========================================

/**
 * @brief Hook function for MessageBoxW
 * @details This function runs in user mode (R3) after EPTHook jumps here
 */
int WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    // Log the interception
    std::wcout << L"[R3 Hook] MessageBoxW intercepted!" << std::endl;
    std::wcout << L"  Original Text: " << (lpText ? lpText : L"(null)") << std::endl;
    std::wcout << L"  Original Caption: " << (lpCaption ? lpCaption : L"(null)") << std::endl;

    // Check if trampoline is ready (should always be ready due to double-pointer mechanism)
    if (g_OriginalMessageBoxW == nullptr) {
        std::cerr << "[R3 Hook] ERROR: Trampoline is NULL! This shouldn't happen." << std::endl;
        return IDCANCEL;
    }

    // Call original function with modified text
    return g_OriginalMessageBoxW(hWnd, L"Hooked by EPT", lpCaption, uType);
}

/**
 * @brief Hook function for MessageBoxA
 * @details This function runs in user mode (R3) after EPTHook jumps here
 */
int WINAPI HookedMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    // Log the interception
    std::cout << "[R3 Hook] MessageBoxA intercepted!" << std::endl;
    std::cout << "  Original Text: " << (lpText ? lpText : "(null)") << std::endl;
    std::cout << "  Original Caption: " << (lpCaption ? lpCaption : "(null)") << std::endl;

    // Check if trampoline is ready
    if (g_OriginalMessageBoxA == nullptr) {
        std::cerr << "[R3 Hook] ERROR: Trampoline is NULL! This shouldn't happen." << std::endl;
        return IDCANCEL;
    }

    // Call original function with modified text
    return g_OriginalMessageBoxA(hWnd, "Hooked by EPT", lpCaption, uType);
}

// ========================================
// EPTHook Installation
// ========================================

/**
 * @brief Install EPTHook for MessageBoxW
 */
bool InstallMessageBoxWHook(SimpleHv::Client& client)
{
    std::cout << "\n[*] Installing EPTHook for MessageBoxW..." << std::endl;

    // Get MessageBoxW address
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) {
        std::cerr << "[-] Failed to get user32.dll handle!" << std::endl;
        return false;
    }

    FARPROC pMessageBoxW = GetProcAddress(hUser32, "MessageBoxW");
    if (!pMessageBoxW) {
        std::cerr << "[-] Failed to get MessageBoxW address!" << std::endl;
        return false;
    }

    std::cout << "[+] MessageBoxW address: 0x" << std::hex << pMessageBoxW << std::dec << std::endl;
    std::cout << "[+] Hook function address: 0x" << std::hex << (void*)HookedMessageBoxW << std::dec << std::endl;
    std::cout << "[+] Current Process ID: " << GetCurrentProcessId() << std::endl;

    // Prepare IOCTL request
    SIMPLEHV_R3_HOOK_REQUEST request = {0};
    request.TargetAddress = (PVOID)pMessageBoxW;
    request.HookFunction = (PVOID)HookedMessageBoxW;
    request.ProcessId = GetCurrentProcessId();

    SIMPLEHV_R3_HOOK_RESPONSE response = {0};

    // Call driver to install EPTHook
    if (!client.InstallR3Hook(&request, &response)) {
        std::cerr << "[-] Failed to send IOCTL! Error: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "[+] IOCTL completed successfully!" << std::endl;
    std::cout << "    Status: 0x" << std::hex << response.Status << std::dec << std::endl;
    std::cout << "    Trampoline: 0x" << std::hex << response.Trampoline << std::dec << std::endl;

    if (response.Status != 0) {
        std::cerr << "[-] Driver failed to install hook! Status: 0x"
                  << std::hex << response.Status << std::dec << std::endl;
        return false;
    }

    // Save trampoline for use in hook function
    g_OriginalMessageBoxW = (fnMessageBoxW)response.Trampoline;

    std::cout << "[+] EPTHook installed successfully!" << std::endl;
    return true;
}

/**
 * @brief Install EPTHook for MessageBoxA
 */
bool InstallMessageBoxAHook(SimpleHv::Client& client)
{
    std::cout << "\n[*] Installing EPTHook for MessageBoxA..." << std::endl;

    // Get MessageBoxA address
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) {
        std::cerr << "[-] Failed to get user32.dll handle!" << std::endl;
        return false;
    }

    FARPROC pMessageBoxA = GetProcAddress(hUser32, "MessageBoxA");
    if (!pMessageBoxA) {
        std::cerr << "[-] Failed to get MessageBoxA address!" << std::endl;
        return false;
    }

    std::cout << "[+] MessageBoxA address: 0x" << std::hex << pMessageBoxA << std::dec << std::endl;
    std::cout << "[+] Hook function address: 0x" << std::hex << (void*)HookedMessageBoxA << std::dec << std::endl;
    std::cout << "[+] Current Process ID: " << GetCurrentProcessId() << std::endl;

    // Prepare IOCTL request
    SIMPLEHV_R3_HOOK_REQUEST request = {0};
    request.TargetAddress = (PVOID)pMessageBoxA;
    request.HookFunction = (PVOID)HookedMessageBoxA;
    request.ProcessId = GetCurrentProcessId();

    SIMPLEHV_R3_HOOK_RESPONSE response = {0};

    // Call driver to install EPTHook
    if (!client.InstallR3Hook(&request, &response)) {
        std::cerr << "[-] Failed to send IOCTL! Error: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "[+] IOCTL completed successfully!" << std::endl;
    std::cout << "    Status: 0x" << std::hex << response.Status << std::dec << std::endl;
    std::cout << "    Trampoline: 0x" << std::hex << response.Trampoline << std::dec << std::endl;

    if (response.Status != 0) {
        std::cerr << "[-] Driver failed to install hook! Status: 0x"
                  << std::hex << response.Status << std::dec << std::endl;
        return false;
    }

    // Save trampoline for use in hook function
    g_OriginalMessageBoxA = (fnMessageBoxA)response.Trampoline;

    std::cout << "[+] EPTHook installed successfully!" << std::endl;
    return true;
}

// ========================================
// Test Functions
// ========================================

void TestMessageBoxHooks()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "  MessageBox EPTHook Test              " << std::endl;
    std::cout << "========================================" << std::endl;

    // Test 1: Before hooks
    std::cout << "\n[Test 1] Testing MessageBoxW before hook..." << std::endl;
    MessageBoxW(NULL, L"This is the original text", L"Before EPTHook", MB_OK | MB_ICONINFORMATION);

    std::cout << "\n[Test 2] Testing MessageBoxA before hook..." << std::endl;
    MessageBoxA(NULL, "This is the original text", "Before EPTHook", MB_OK | MB_ICONINFORMATION);

    // Test 2: After hooks
    std::cout << "\n[Test 3] Testing MessageBoxW with EPTHook (should show 'Hooked by EPT')..." << std::endl;
    MessageBoxW(NULL, L"This text will be replaced", L"After EPTHook - W", MB_OK | MB_ICONWARNING);

    std::cout << "\n[Test 4] Testing MessageBoxA with EPTHook (should show 'Hooked by EPT')..." << std::endl;
    MessageBoxA(NULL, "This text will be replaced", "After EPTHook - A", MB_OK | MB_ICONWARNING);

    // Test 3: Multiple calls
    std::cout << "\n[Test 5] Testing multiple MessageBox calls..." << std::endl;
    MessageBoxW(NULL, L"Test 1", L"Multiple Test 1", MB_OK);
    MessageBoxA(NULL, "Test 2", "Multiple Test 2", MB_OK);
    MessageBoxW(NULL, L"Test 3", L"Multiple Test 3", MB_OK);

    // Test 4: Different message types
    std::cout << "\n[Test 6] Testing different message box types..." << std::endl;
    MessageBoxW(NULL, L"Yes/No test", L"Question", MB_YESNO | MB_ICONQUESTION);
    MessageBoxA(NULL, "OK/Cancel test", "Confirmation", MB_OKCANCEL | MB_ICONEXCLAMATION);
}

// ========================================
// Main Entry Point
// ========================================

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "   R3 EPTHook Test Program             " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "[*] This test demonstrates EPTHook for user-mode functions" << std::endl;
    std::cout << "[*] Hook functions run in R3, EPT setup is done by driver" << std::endl;
    std::cout << std::endl;

    // Step 1: Connect to driver
    SimpleHv::Client client;

    std::cout << "[*] Connecting to SimpleHv driver..." << std::endl;
    std::cout << "    Device: \\\\.\\SimpleHv" << std::endl;

    if (!client.Open()) {
        std::cerr << "[-] Failed to open SimpleHv device!" << std::endl;
        std::cerr << "    Error code: " << GetLastError() << std::endl;
        std::cerr << std::endl;
        std::cerr << "[*] Make sure to:" << std::endl;
        std::cerr << "    1. Load the SimpleHv driver (sc start SimpleHv)" << std::endl;
        std::cerr << "    2. Run this program as Administrator" << std::endl;
        std::cerr << "    3. Enable virtualization in BIOS" << std::endl;
        std::cerr << std::endl;
        system("pause");
        return -1;
    }

    std::cout << "[+] Connected to SimpleHv driver!" << std::endl;

    // Step 2: Ping hypervisor
    std::cout << "\n[*] Pinging hypervisor..." << std::endl;
    SIMPLEHV_PING_RESPONSE pingResponse = {0};
    if (client.Ping(&pingResponse)) {
        std::cout << "[+] Hypervisor is running!" << std::endl;
        std::cout << "    Signature: 0x" << std::hex << pingResponse.Signature << std::dec << std::endl;
        std::cout << "    CPUs: " << pingResponse.NumProcessors << std::endl;
    } else {
        std::cerr << "[-] Failed to ping hypervisor!" << std::endl;
        client.Close();
        system("pause");
        return -1;
    }

    // Step 3: Install EPTHooks
    if (!InstallMessageBoxWHook(client)) {
        std::cerr << "[-] Failed to install MessageBoxW hook!" << std::endl;
        client.Close();
        system("pause");
        return -1;
    }

    if (!InstallMessageBoxAHook(client)) {
        std::cerr << "[-] Failed to install MessageBoxA hook!" << std::endl;
        client.Close();
        system("pause");
        return -1;
    }

    // Step 4: Run tests
    TestMessageBoxHooks();

    // Step 5: Clean up
    std::cout << "\n[*] Test completed!" << std::endl;
    std::cout << "[*] Press any key to unhook and exit..." << std::endl;
    system("pause");

    // Unhook all
    std::cout << "\n[*] Removing all hooks..." << std::endl;
    if (client.UnhookAll()) {
        std::cout << "[+] All hooks removed successfully!" << std::endl;
    } else {
        std::cerr << "[-] Failed to remove hooks!" << std::endl;
    }

    // Close connection
    client.Close();
    std::cout << "[*] Connection closed." << std::endl;
    std::cout << std::endl;

    return 0;
}