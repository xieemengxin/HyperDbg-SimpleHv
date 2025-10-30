#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <string>
#include <vector>
#include "SimpleHvClient.h"

void PrintMenu() {
    std::cout << "========================================" << std::endl;
    std::cout << "          Test Menu                     " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  1. Ping Hypervisor" << std::endl;
    std::cout << "  2. Install Test Hooks (Kernel)" << std::endl;
    std::cout << "  3. Unhook All" << std::endl;
    std::cout << "  4. Test R3 EPTHook (MessageBox)" << std::endl;
    std::cout << "  0. Exit" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Select option: ";
}

void RunPingTest(SimpleHv::Client& client) {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "         Ping Test                      " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "[*] Sending PING IOCTL..." << std::endl;

    SIMPLEHV_PING_RESPONSE response = { 0 };
    if (!client.Ping(&response)) {
        std::cout << "[-] PING failed! Error: " << GetLastError() << std::endl;
        std::cout << "    Make sure SimpleHv driver is loaded" << std::endl;
        return;
    }

    std::cout << "[+] PING successful!" << std::endl;
    std::cout << "    Signature    : 0x" << std::hex << std::uppercase
              << response.Signature << std::dec << std::endl;
    std::cout << "    NumProcessors: " << response.NumProcessors << std::endl;
    std::cout << "    IsRunning    : " << (response.IsRunning ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    if (response.Signature == 0xE79086E5A198) {
        std::cout << "[+] Hypervisor signature matches!" << std::endl;
    } else {
        std::cout << "[-] Unexpected signature!" << std::endl;
    }
    std::cout << std::endl;
}

// Forward declarations for R3 EPTHook test
typedef int (WINAPI* fnMessageBoxW)(HWND, LPCWSTR, LPCWSTR, UINT);
fnMessageBoxW g_OriginalMessageBoxW = nullptr;

// R3 Hook function for MessageBoxW
int WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    std::wcout << L"[R3 Hook] MessageBoxW intercepted! Text: " << (lpText ? lpText : L"(null)") << std::endl;

    if (g_OriginalMessageBoxW) {
        return g_OriginalMessageBoxW(hWnd, L"Hooked by EPT", lpCaption, uType);
    }
    return IDCANCEL;
}

void RunR3EptHookTest(SimpleHv::Client& client) {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  R3 EPTHook Test (MessageBox)         " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Get MessageBoxW address
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) {
        std::cout << "[-] Failed to get user32.dll handle!" << std::endl;
        return;
    }

    FARPROC pMessageBoxW = GetProcAddress(hUser32, "MessageBoxW");
    if (!pMessageBoxW) {
        std::cout << "[-] Failed to get MessageBoxW address!" << std::endl;
        return;
    }

    std::cout << "[*] MessageBoxW address: 0x" << std::hex << pMessageBoxW << std::dec << std::endl;
    std::cout << "[*] Hook function address: 0x" << std::hex << (void*)HookedMessageBoxW << std::dec << std::endl;
    std::cout << "[*] Process ID: " << GetCurrentProcessId() << std::endl;
    std::cout << std::endl;

    // Test before hook
    std::cout << "[Test 1] MessageBox before hook..." << std::endl;
    MessageBoxW(NULL, L"This is original text", L"Before Hook", MB_OK);

    // Install EPTHook
    std::cout << "\n[*] Installing EPTHook..." << std::endl;

    SIMPLEHV_R3_HOOK_REQUEST request = {0};
    request.TargetAddress = (PVOID)pMessageBoxW;
    request.HookFunction = (PVOID)HookedMessageBoxW;
    request.ProcessId = GetCurrentProcessId();

    SIMPLEHV_R3_HOOK_RESPONSE response = {0};

    if (!client.InstallR3Hook(&request, &response)) {
        std::cout << "[-] Failed to send IOCTL! Error: " << GetLastError() << std::endl;
        return;
    }

    std::cout << "[+] IOCTL completed!" << std::endl;
    std::cout << "    Status: 0x" << std::hex << response.Status << std::dec << std::endl;
    std::cout << "    Trampoline: 0x" << std::hex << response.Trampoline << std::dec << std::endl;

    if (response.Status != 0) {
        std::cout << "[-] Failed to install hook! Status: 0x" << std::hex << response.Status << std::dec << std::endl;
        return;
    }

    // Save trampoline
    g_OriginalMessageBoxW = (fnMessageBoxW)response.Trampoline;
    std::cout << "[+] EPTHook installed successfully!" << std::endl;

    // Test after hook
    std::cout << "\n[Test 2] MessageBox after hook (should show 'Hooked by EPT')..." << std::endl;
    MessageBoxW(NULL, L"This text will be replaced", L"After Hook", MB_OK);

    std::cout << "\n[Test 3] Multiple MessageBox calls..." << std::endl;
    MessageBoxW(NULL, L"Test 1", L"Multi Test 1", MB_OK);
    MessageBoxW(NULL, L"Test 2", L"Multi Test 2", MB_OK);

    std::cout << "\n[+] R3 EPTHook test completed!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   SimpleHv Usermode Test Client       " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Create client
    SimpleHv::Client client;

    std::cout << "[*] Connecting to SimpleHv driver..." << std::endl;
    std::cout << "    Device: \\\\.\\SimpleHv" << std::endl;
    std::cout << std::endl;

    if (!client.Open()) {
        std::cout << "[-] Failed to open SimpleHv device!" << std::endl;
        std::cout << "    Error code: " << GetLastError() << std::endl;
        std::cout << std::endl;
        std::cout << "[*] Make sure to:" << std::endl;
        std::cout << "    1. Load the SimpleHv driver (sc start SimpleHv)" << std::endl;
        std::cout << "    2. Run this program as Administrator" << std::endl;
        std::cout << "    3. Enable virtualization in BIOS" << std::endl;
        std::cout << std::endl;
        system("pause");
        return -1;
    }

    std::cout << "[+] Connected to SimpleHv driver!" << std::endl;
    std::cout << std::endl;

    // 主菜单循环
    while (true) {
        PrintMenu();

        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        int choice = std::stoi(input);

        switch (choice) {
            case 1:
                RunPingTest(client);
                std::cout << "Press any key to continue..." << std::endl;
                system("pause");
                break;

            case 2:
                std::cout << std::endl;
                std::cout << "========================================" << std::endl;
                std::cout << "  Install Test Hooks                   " << std::endl;
                std::cout << "========================================" << std::endl;
                std::cout << std::endl;
                std::cout << "[*] Sending IOCTL_SIMPLEHV_INSTALL_TEST_HOOKS..." << std::endl;

                {
                    SIMPLEHV_INSTALL_HOOKS_RESPONSE response = { 0 };
                    if (client.InstallTestHooks(&response)) {
                        std::cout << "[*] IOCTL completed" << std::endl;
                        std::cout << "    Status         : 0x" << std::hex << std::uppercase
                                  << response.Status << std::dec << std::endl;
                        std::cout << "    Hooks Installed: " << response.HooksInstalled << std::endl;

                        if (response.Status == 0) {
                            std::cout << "[+] Test hooks installed successfully!" << std::endl;
                        } else {
                            std::cout << "[-] Failed to install hooks" << std::endl;
                        }
                    } else {
                        std::cout << "[-] IOCTL failed! Error: " << GetLastError() << std::endl;
                    }
                }

                std::cout << std::endl;
                std::cout << "Press any key to continue..." << std::endl;
                system("pause");
                break;

            case 3:
                std::cout << std::endl;
                std::cout << "========================================" << std::endl;
                std::cout << "  Unhook All                           " << std::endl;
                std::cout << "========================================" << std::endl;
                std::cout << std::endl;
                std::cout << "[*] Sending IOCTL_SIMPLEHV_UNHOOK_ALL..." << std::endl;

                if (client.UnhookAll()) {
                    std::cout << "[+] All hooks removed successfully!" << std::endl;
                } else {
                    std::cout << "[-] IOCTL failed! Error: " << GetLastError() << std::endl;
                }

                std::cout << std::endl;
                std::cout << "Press any key to continue..." << std::endl;
                system("pause");
                break;

            case 4:
                RunR3EptHookTest(client);
                std::cout << std::endl;
                std::cout << "Press any key to continue..." << std::endl;
                system("pause");
                break;

            case 0:
                std::cout << std::endl;
                std::cout << "[*] Exiting..." << std::endl;
                client.Close();
                return 0;

            default:
                std::cout << "[-] Invalid option!" << std::endl;
                std::cout << std::endl;
                break;
        }

        std::cout << std::endl;
    }

    return 0;
}
