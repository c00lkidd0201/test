#pragma once

#include <windows.h>
#include <string>
#include <vector>

class Injector {
public:
    Injector();
    ~Injector();
    
    // Методы инъекции
    bool InjectDLL(DWORD processId, const wchar_t* dllPath);
    bool InjectDLL(const wchar_t* processName, const wchar_t* dllPath);
    
    // Проверка процесса
    bool IsProcessRunning(const wchar_t* processName);
    DWORD GetProcessIdByName(const wchar_t* processName);
    
    // Получение информации о процессе
    bool GetProcessInfo(DWORD processId, std::wstring& processName, std::wstring& processPath);
    
    // Проверка архитектуры
    bool IsProcess64Bit(DWORD processId);
    bool IsCurrentProcess64Bit();
    
    // Вспомогательные функции
    bool WaitForProcess(const wchar_t* processName, DWORD timeoutMs = 30000);
    
private:
    // Методы инъекции
    bool InjectViaCreateRemoteThread(DWORD processId, const wchar_t* dllPath);
    bool InjectViaSetWindowsHookEx(DWORD processId, const wchar_t* dllPath);
    bool InjectViaQueueUserAPC(DWORD processId, const wchar_t* dllPath);
    
    // Вспомогательные функции
    bool WriteProcessMemoryEx(HANDLE hProcess, LPVOID lpAddress, LPCVOID lpBuffer, SIZE_T nSize);
    LPVOID AllocateMemoryInProcess(HANDLE hProcess, SIZE_T size, DWORD protect = PAGE_EXECUTE_READWRITE);
    bool FreeMemoryInProcess(HANDLE hProcess, LPVOID lpAddress, SIZE_T size);
    
    FARPROC GetLoadLibraryAddress();
};
