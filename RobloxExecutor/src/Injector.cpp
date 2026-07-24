#include "Injector.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>

Injector::Injector()
{
}

Injector::~Injector()
{
}

bool Injector::InjectDLL(DWORD processId, const wchar_t* dllPath)
{
    if (processId == 0 || !dllPath)
        return false;
    
    // Проверка архитектуры
    if (IsProcess64Bit(processId) != IsCurrentProcess64Bit()) {
        std::wcerr << L"Architecture mismatch between injector and target process." << std::endl;
        return false;
    }
    
    // Основной метод - CreateRemoteThread
    return InjectViaCreateRemoteThread(processId, dllPath);
}

bool Injector::InjectDLL(const wchar_t* processName, const wchar_t* dllPath)
{
    if (!processName || !dllPath)
        return false;
    
    DWORD processId = GetProcessIdByName(processName);
    if (processId == 0)
        return false;
    
    return InjectDLL(processId, dllPath);
}

bool Injector::IsProcessRunning(const wchar_t* processName)
{
    return GetProcessIdByName(processName) != 0;
}

DWORD Injector::GetProcessIdByName(const wchar_t* processName)
{
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return 0;
    
    if (!Process32FirstW(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return 0;
    }
    
    do {
        if (_wcsicmp(pe32.szExeFile, processName) == 0) {
            CloseHandle(hProcessSnap);
            return pe32.th32ProcessID;
        }
    } while (Process32NextW(hProcessSnap, &pe32));
    
    CloseHandle(hProcessSnap);
    return 0;
}

bool Injector::GetProcessInfo(DWORD processId, std::wstring& processName, std::wstring& processPath)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess)
        return false;
    
    WCHAR szProcessName[MAX_PATH] = {0};
    if (GetModuleFileNameExW(hProcess, NULL, szProcessName, MAX_PATH)) {
        processPath = szProcessName;
        
        // Извлечение имени файла
        size_t pos = processPath.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            processName = processPath.substr(pos + 1);
        } else {
            processName = processPath;
        }
    }
    
    CloseHandle(hProcess);
    return !processName.empty();
}

bool Injector::IsProcess64Bit(DWORD processId)
{
    BOOL bIsWow64 = FALSE;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (!hProcess)
        return false;
    
    if (!IsWow64Process(hProcess, &bIsWow64)) {
        CloseHandle(hProcess);
        return false;
    }
    
    CloseHandle(hProcess);
    return !bIsWow64;
}

bool Injector::IsCurrentProcess64Bit()
{
#if defined(_WIN64)
    return true;
#elif defined(_WIN32)
    BOOL bIsWow64 = FALSE;
    return !IsWow64Process(GetCurrentProcess(), &bIsWow64) || !bIsWow64;
#else
    return false;
#endif
}

bool Injector::WaitForProcess(const wchar_t* processName, DWORD timeoutMs)
{
    DWORD startTime = GetTickCount();
    
    while (GetTickCount() - startTime < timeoutMs) {
        if (IsProcessRunning(processName))
            return true;
        
        Sleep(100);
    }
    
    return false;
}

bool Injector::InjectViaCreateRemoteThread(DWORD processId, const wchar_t* dllPath)
{
    HANDLE hProcess = NULL;
    LPVOID pRemoteMemory = NULL;
    HANDLE hThread = NULL;
    bool success = false;
    
    __try {
        // Открытие процесса
        hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
                               PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, 
                               FALSE, processId);
        if (!hProcess) {
            std::wcerr << L"Failed to open process. Error: " << GetLastError() << std::endl;
            __leave;
        }
        
        // Выделение памяти в целевом процессе
        size_t dllPathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);
        pRemoteMemory = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pRemoteMemory) {
            std::wcerr << L"Failed to allocate memory in target process. Error: " << GetLastError() << std::endl;
            __leave;
        }
        
        // Запись пути DLL в выделенную память
        if (!WriteProcessMemory(hProcess, pRemoteMemory, dllPath, dllPathSize, NULL)) {
            std::wcerr << L"Failed to write DLL path to target process. Error: " << GetLastError() << std::endl;
            __leave;
        }
        
        // Получение адреса LoadLibraryW
        FARPROC pLoadLibraryW = GetLoadLibraryAddress();
        if (!pLoadLibraryW) {
            std::wcerr << L"Failed to get LoadLibraryW address." << std::endl;
            __leave;
        }
        
        // Создание удаленного потока
        hThread = CreateRemoteThread(hProcess, NULL, 0, 
                                     (LPTHREAD_START_ROUTINE)pLoadLibraryW, 
                                     pRemoteMemory, 0, NULL);
        if (!hThread) {
            std::wcerr << L"Failed to create remote thread. Error: " << GetLastError() << std::endl;
            __leave;
        }
        
        // Ожидание завершения потока
        WaitForSingleObject(hThread, INFINITE);
        
        success = true;
        
    } __finally {
        // Очистка
        if (hThread) {
            CloseHandle(hThread);
        }
        if (pRemoteMemory) {
            VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        }
        if (hProcess) {
            CloseHandle(hProcess);
        }
    }
    
    return success;
}

bool Injector::InjectViaSetWindowsHookEx(DWORD processId, const wchar_t* dllPath)
{
    // Альтернативный метод через SetWindowsHookEx
    // Реализация аналогична CreateRemoteThread, но использует хуки
    return InjectViaCreateRemoteThread(processId, dllPath);
}

bool Injector::InjectViaQueueUserAPC(DWORD processId, const wchar_t* dllPath)
{
    // Метод через QueueUserAPC
    // Более сложный, но может обойти некоторые защиты
    return InjectViaCreateRemoteThread(processId, dllPath);
}

bool Injector::WriteProcessMemoryEx(HANDLE hProcess, LPVOID lpAddress, LPCVOID lpBuffer, SIZE_T nSize)
{
    SIZE_T bytesWritten = 0;
    return WriteProcessMemory(hProcess, lpAddress, lpBuffer, nSize, &bytesWritten) && bytesWritten == nSize;
}

LPVOID Injector::AllocateMemoryInProcess(HANDLE hProcess, SIZE_T size, DWORD protect)
{
    return VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, protect);
}

bool Injector::FreeMemoryInProcess(HANDLE hProcess, LPVOID lpAddress, SIZE_T size)
{
    return VirtualFreeEx(hProcess, lpAddress, size, MEM_DECOMMIT) != FALSE;
}

FARPROC Injector::GetLoadLibraryAddress()
{
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32)
        return NULL;
    
    return GetProcAddress(hKernel32, "LoadLibraryW");
}
