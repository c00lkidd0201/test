#pragma once

#include <windows.h>
#include <winioctl.h>

// IO Control Codes (должны совпадать с драйвером)
#define IOCTL_READ_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_PROCESS_ID CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_MODULE_BASE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Структура для чтения памяти
typedef struct _READ_MEMORY_REQUEST {
    ULONG64 ProcessId;
    ULONG64 Address;
    ULONG Size;
    UCHAR Buffer[1]; // Переменный размер
} READ_MEMORY_REQUEST, *PREAD_MEMORY_REQUEST;

// Структура для записи памяти
typedef struct _WRITE_MEMORY_REQUEST {
    ULONG64 ProcessId;
    ULONG64 Address;
    ULONG Size;
    UCHAR Buffer[1]; // Переменный размер
} WRITE_MEMORY_REQUEST, *PWRITE_MEMORY_REQUEST;

// Структура для получения Process ID
typedef struct _GET_PROCESS_ID_REQUEST {
    WCHAR ProcessName[256];
    ULONG ProcessId;
} GET_PROCESS_ID_REQUEST, *PGET_PROCESS_ID_REQUEST;

// Структура для получения базового адреса модуля
typedef struct _GET_MODULE_BASE_REQUEST {
    WCHAR ModuleName[256];
    ULONG ProcessId;
    ULONG64 ModuleBase;
} GET_MODULE_BASE_REQUEST, *PGET_MODULE_BASE_REQUEST;

// Класс для работы с драйвером
class DriverInterface {
public:
    DriverInterface();
    ~DriverInterface();
    
    bool Connect();
    void Disconnect();
    bool IsConnected() const;
    
    // Чтение памяти
    bool ReadMemory(ULONG64 processId, ULONG64 address, LPVOID buffer, ULONG size);
    
    // Запись памяти
    bool WriteMemory(ULONG64 processId, ULONG64 address, LPCVOID buffer, ULONG size);
    
    // Получение Process ID по имени
    ULONG GetProcessId(const wchar_t* processName);
    
    // Получение базового адреса модуля
    ULONG64 GetModuleBase(ULONG processId, const wchar_t* moduleName);
    
    // Получение базового адреса модуля в процессе
    ULONG64 GetModuleBaseInProcess(ULONG processId, const wchar_t* moduleName);

private:
    HANDLE m_hDevice;
    
    // Вспомогательные функции
    bool SendIOCTL(DWORD ioControlCode, LPVOID inputBuffer, ULONG inputSize, LPVOID outputBuffer, ULONG outputSize, LPDWORD bytesReturned);
    ULONG FindProcessIdByName(const wchar_t* processName);
};
