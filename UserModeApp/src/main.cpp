#include "DriverInterface.h"
#include <iostream>
#include <iomanip>
#include <string>

int main()
{
    std::wcout << L"=== Kernel Driver Interface Test ===" << std::endl;
    
    DriverInterface driver;
    
    // Подключение к драйверу
    std::wcout << L"Connecting to driver..." << std::endl;
    if (!driver.Connect()) {
        std::wcerr << L"Failed to connect to driver!" << std::endl;
        std::wcerr << L"Make sure the driver is installed and Test Mode is enabled." << std::endl;
        
        // Проверка Test Mode
        BOOL testMode = FALSE;
        NTSTATUS status = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)11, &testMode, sizeof(testMode), NULL);
        if (testMode) {
            std::wcout << L"Test Mode is enabled." << std::endl;
        } else {
            std::wcerr << L"Test Mode is NOT enabled!" << std::endl;
            std::wcerr << L"Run: bcdedit /set testsigning on" << std::endl;
        }
        
        return 1;
    }
    
    std::wcout << L"Connected to driver successfully!" << std::endl;
    
    // Получение Process ID Roblox
    std::wcout << L"\nSearching for Roblox process..." << std::endl;
    ULONG robloxPid = driver.GetProcessId(L"RobloxPlayerBeta.exe");
    
    if (robloxPid == 0) {
        std::wcout << L"Roblox process not found. Trying alternative names..." << std::endl;
        robloxPid = driver.GetProcessId(L"Roblox.exe");
    }
    
    if (robloxPid == 0) {
        std::wcout << L"Roblox process not running." << std::endl;
    } else {
        std::wcout << L"Roblox Process ID: " << robloxPid << std::endl;
        
        // Получение базового адреса модуля
        std::wcout << L"\nGetting module base addresses..." << std::endl;
        ULONG64 robloxBase = driver.GetModuleBaseInProcess(robloxPid, L"RobloxPlayerBeta.exe");
        if (robloxBase == 0) {
            robloxBase = driver.GetModuleBaseInProcess(robloxPid, L"Roblox.exe");
        }
        
        if (robloxBase != 0) {
            std::wcout << L"Roblox base address: 0x" << std::hex << std::setw(16) << std::setfill(L'0') << robloxBase << std::endl;
        }
        
        // Пример чтения памяти
        std::wcout << L"\nTesting memory read..." << std::endl;
        if (robloxBase != 0) {
            UCHAR buffer[8] = {0};
            if (driver.ReadMemory(robloxPid, robloxBase, buffer, sizeof(buffer))) {
                std::wcout << L"Read memory at 0x" << std::hex << robloxBase << L": ";
                for (int i = 0; i < sizeof(buffer); i++) {
                    std::wcout << std::hex << std::setw(2) << std::setfill(L'0') << (int)buffer[i] << L" ";
                }
                std::wcout << std::endl;
            } else {
                std::wcerr << L"Failed to read memory." << std::endl;
            }
        }
    }
    
    // Тестирование с текущим процессом
    std::wcout << L"\nTesting with current process..." << std::endl;
    ULONG currentPid = GetCurrentProcessId();
    std::wcout << L"Current Process ID: " << currentPid << std::endl;
    
    // Чтение памяти из текущего процесса
    UCHAR testBuffer[16] = {0};
    ULONG64 testAddress = (ULONG64)&testBuffer;
    
    if (driver.ReadMemory(currentPid, testAddress, testBuffer, sizeof(testBuffer))) {
        std::wcout << L"Read memory from current process: ";
        for (int i = 0; i < sizeof(testBuffer); i++) {
            std::wcout << std::hex << std::setw(2) << std::setfill(L'0') << (int)testBuffer[i] << L" ";
        }
        std::wcout << std::endl;
    }
    
    // Запись памяти в текущий процесс
    UCHAR writeData[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};
    if (driver.WriteMemory(currentPid, testAddress, writeData, sizeof(writeData))) {
        std::wcout << L"Write memory to current process successful." << std::endl;
    }
    
    // Отключение
    driver.Disconnect();
    std::wcout << L"\nDisconnected from driver." << std::endl;
    
    return 0;
}
