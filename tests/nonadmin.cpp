#include <iostream>
#include <string>
#include <windows.h>

int wmain(int argc, wchar_t **argv) {
    if (argc != 2)
        return 2;
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY,
                          &token))
        return 1;
    SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
    PSID admin = nullptr;
    if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &admin)) {
        CloseHandle(token);
        return 1;
    }
    SID_AND_ATTRIBUTES disabled{admin, 0};
    HANDLE restricted = nullptr;
    BOOL member = TRUE;
    if (!CheckTokenMembership(nullptr, admin, &member)) {
        FreeSid(admin);
        CloseHandle(token);
        return 1;
    }
    BOOL ok = TRUE;
    if (member)
        ok = CreateRestrictedToken(token, DISABLE_MAX_PRIVILEGE, 1, &disabled, 0, nullptr, 0,
                                   nullptr, &restricted);
    FreeSid(admin);
    CloseHandle(token);
    if (!ok) {
        std::cerr << "CreateRestrictedToken failed: " << GetLastError() << '\n';
        return 1;
    }
    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\" --nonadmin-child";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (member)
        ok = CreateProcessAsUserW(restricted, nullptr, command.data(), nullptr, nullptr, FALSE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    else
        ok = CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                            nullptr, nullptr, &startup, &process);
    if (restricted)
        CloseHandle(restricted);
    if (!ok) {
        std::cerr << "CreateProcessAsUser failed: " << GetLastError() << '\n';
        return 1;
    }
    CloseHandle(process.hThread);
    if (WaitForSingleObject(process.hProcess, 60000) != WAIT_OBJECT_0) {
        CloseHandle(process.hProcess);
        return 1;
    }
    DWORD result = 1;
    GetExitCodeProcess(process.hProcess, &result);
    CloseHandle(process.hProcess);
    std::cout << "Restricted-token child exit: " << result << '\n';
    return static_cast<int>(result);
}
