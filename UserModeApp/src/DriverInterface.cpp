#include "DriverInterface.h"
#include <iostream>
#include <tlhelp32.h>

DriverInterface::DriverInterface() : m_hDevice(INVALID_HANDLE_VALUE)
{
}

DriverInterface::~DriverInterface()
{
    Disconnect();
}

bool DriverInterface::Connect()
{
    if (m_hDevice != INVALID_HANDLE_VALUE)
        return true;
    
    // Открытие устройства
    m_hDevice = CreateFileW(
        L"\\\\.\\KernelDriver",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (m_hDevice == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        std::wcerr << L"Failed to open device: " << error << std::endl;
        return false;
    }
    
    return true;
}

void DriverInterface::Disconnect()
{
    if (m_hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hDevice);
        m_hDevice = INVALID_HANDLE_VALUE;
    }
}

bool DriverInterface::IsConnected() const
{
    return m_hDevice != INVALID_HANDLE_VALUE;
}

bool DriverInterface::SendIOCTL(DWORD ioControlCode, LPVOID inputBuffer, ULONG inputSize, LPVOID outputBuffer, ULONG outputSize, LPDWORD bytesReturned)
{
    if (!IsConnected())
        return false;
    
    DWORD bytesReturnedLocal = 0;
    BOOL result = DeviceIoControl(
        m_hDevice,
        ioControlCode,
        inputBuffer,
        inputSize,
        outputBuffer,
        outputSize,
        &bytesReturnedLocal,
        NULL
    );
    
    if (bytesReturned)
        *bytesReturned = bytesReturnedLocal;
    
    return result != FALSE;
}

bool DriverInterface::ReadMemory(ULONG64 processId, ULONG64 address, LPVOID buffer, ULONG size)
{
    if (!IsConnected() || !buffer || size == 0)
        return false;
    
    // Выделение буфера для запроса
    ULONG requestSize = sizeof(READ_MEMORY_REQUEST) - 1 + size;
    PREAD_MEMORY_REQUEST request = (PREAD_MEMORY_REQUEST)malloc(requestSize);
    if (!request)
        return false;
    
    request->ProcessId = processId;
    request->Address = address;
    request->Size = size;
    
    DWORD bytesReturned = 0;
    bool success = SendIOCTL(IOCTL_READ_MEMORY, request, requestSize, request, requestSize, &bytesReturned);
    
    if (success && bytesReturned >= sizeof(READ_MEMORY_REQUEST) - 1 + size) {
        // Копирование данных
        memcpy(buffer, request->Buffer, size);
    }
    
    free(request);
    return success;
}

bool DriverInterface::WriteMemory(ULONG64 processId, ULONG64 address, LPCVOID buffer, ULONG size)
{
    if (!IsConnected() || !buffer || size == 0)
        return false;
    
    // Выделение буфера для запроса
    ULONG requestSize = sizeof(WRITE_MEMORY_REQUEST) - 1 + size;
    PWRITE_MEMORY_REQUEST request = (PWRITE_MEMORY_REQUEST)malloc(requestSize);
    if (!request)
        return false;
    
    request->ProcessId = processId;
    request->Address = address;
    request->Size = size;
    memcpy(request->Buffer, buffer, size);
    
    DWORD bytesReturned = 0;
    bool success = SendIOCTL(IOCTL_WRITE_MEMORY, request, requestSize, request, requestSize, &bytesReturned);
    
    free(request);
    return success;
}

ULONG DriverInterface::GetProcessId(const wchar_t* processName)
{
    if (!IsConnected() || !processName)
        return 0;
    
    GET_PROCESS_ID_REQUEST request = {0};
    wcscpy_s(request.ProcessName, processName);
    
    DWORD bytesReturned = 0;
    bool success = SendIOCTL(IOCTL_GET_PROCESS_ID, &request, sizeof(request), &request, sizeof(request), &bytesReturned);
    
    if (success)
        return request.ProcessId;
    
    // Fallback: поиск через Toolhelp32
    return FindProcessIdByName(processName);
}

ULONG64 DriverInterface::GetModuleBase(ULONG processId, const wchar_t* moduleName)
{
    if (!IsConnected() || !moduleName)
        return 0;
    
    GET_MODULE_BASE_REQUEST request = {0};
    request.ProcessId = processId;
    wcscpy_s(request.ModuleName, moduleName);
    
    DWORD bytesReturned = 0;
    bool success = SendIOCTL(IOCTL_GET_MODULE_BASE, &request, sizeof(request), &request, sizeof(request), &bytesReturned);
    
    if (success)
        return request.ModuleBase;
    
    return 0;
}

ULONG64 DriverInterface::GetModuleBaseInProcess(ULONG processId, const wchar_t* moduleName)
{
    // Альтернативный метод через EnumProcessModules
    HMODULE hModule = NULL;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess)
        return 0;
    
    HMODULE hModules[1024];
    DWORD cbNeeded;
    
    if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
        DWORD moduleCount = cbNeeded / sizeof(HMODULE);
        
        for (DWORD i = 0; i < moduleCount; i++) {
            WCHAR szModName[MAX_PATH];
            if (GetModuleFileNameExW(hProcess, hModules[i], szModName, MAX_PATH)) {
                std::wstring modulePath(szModName);
                size_t pos = modulePath.find_last_of(L"\\/");
                std::wstring moduleFileName = (pos != std::wstring::npos) ? modulePath.substr(pos + 1) : modulePath;
                
                if (_wcsicmp(moduleFileName.c_str(), moduleName) == 0) {
                    CloseHandle(hProcess);
                    return (ULONG64)hModules[i];
                }
            }
        }
    }
    
    CloseHandle(hProcess);
    return 0;
}

// Вспомогательная функция для поиска Process ID
ULONG DriverInterface::FindProcessIdByName(const wchar_t* processName)
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
