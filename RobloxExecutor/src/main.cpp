#include "Injector.h"
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

int main()
{
    std::wcout << L"=== Roblox Executor ===" << std::endl;
    
    Injector injector;
    
    // Проверка архитектуры
    std::wcout << L"Current process is " << (injector.IsCurrentProcess64Bit() ? L"64-bit" : L"32-bit") << std::endl;
    
    // Поиск процесса Roblox
    std::wcout << L"\nSearching for Roblox process..." << std::endl;
    
    const wchar_t* robloxProcesses[] = {
        L"RobloxPlayerBeta.exe",
        L"Roblox.exe",
        L"RobloxStudioBeta.exe"
    };
    
    DWORD robloxPid = 0;
    std::wstring robloxProcessName;
    
    for (const wchar_t* processName : robloxProcesses) {
        robloxPid = injector.GetProcessIdByName(processName);
        if (robloxPid != 0) {
            robloxProcessName = processName;
            break;
        }
    }
    
    if (robloxPid == 0) {
        std::wcerr << L"Roblox process not found!" << std::endl;
        std::wcout << L"Waiting for Roblox to start..." << std::endl;
        
        if (!injector.WaitForProcess(L"RobloxPlayerBeta.exe", 60000)) {
            std::wcerr << L"Roblox did not start within 60 seconds." << std::endl;
            return 1;
        }
        
        robloxPid = injector.GetProcessIdByName(L"RobloxPlayerBeta.exe");
        robloxProcessName = L"RobloxPlayerBeta.exe";
    }
    
    std::wcout << L"Found Roblox process: " << robloxProcessName << L" (PID: " << robloxPid << L")" << std::endl;
    
    // Проверка архитектуры процесса
    bool is64Bit = injector.IsProcess64Bit(robloxPid);
    std::wcout << L"Roblox process is " << (is64Bit ? L"64-bit" : L"32-bit") << std::endl;
    
    // Проверка совместимости архитектуры
    if (is64Bit != injector.IsCurrentProcess64Bit()) {
        std::wcerr << L"Architecture mismatch! Injector and target process must have the same architecture." << std::endl;
        return 1;
    }
    
    // Поиск DLL для инъекции
    std::wcout << L"\nSearching for Roblox DLL..." << std::endl;
    
    // Поиск в текущей директории
    fs::path dllPath = fs::current_path() / L"RobloxDLL.dll";
    if (!fs::exists(dllPath)) {
        // Поиск в поддиректории
        dllPath = fs::current_path() / L".." / L"RobloxDLL" / L"Release" / L"RobloxDLL.dll";
    }
    
    if (!fs::exists(dllPath)) {
        std::wcerr << L"RobloxDLL.dll not found!" << std::endl;
        std::wcout << L"Expected path: " << dllPath.wstring() << std::endl;
        return 1;
    }
    
    std::wcout << L"Found DLL: " << dllPath.wstring() << std::endl;
    
    // Инъекция DLL
    std::wcout << L"\nInjecting DLL into Roblox process..." << std::endl;
    
    if (injector.InjectDLL(robloxPid, dllPath.c_str())) {
        std::wcout << L"DLL injected successfully!" << std::endl;
    } else {
        std::wcerr << L"Failed to inject DLL!" << std::endl;
        return 1;
    }
    
    std::wcout << L"\nRoblox Executor completed successfully!" << std::endl;
    std::wcout << L"The DLL should now be loaded in the Roblox process." << std::endl;
    
    return 0;
}
