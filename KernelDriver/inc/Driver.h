#pragma once

// Для KMDF драйвера используем WDF заголовочные файлы
// Если WDK не установлен, используем стандартные заголовочные файлы
#ifdef _KERNEL_MODE
#include <ntddk.h>
#include <wdf.h>
#else
// Заглушки для сборки без WDK
#include <windows.h>
typedef LONG NTSTATUS;
#define STATUS_SUCCESS 0
#define STATUS_INVALID_DEVICE_REQUEST 0xC0000010
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004
#define STATUS_ACCESS_VIOLATION 0xC0000005
#define STATUS_NOT_FOUND 0xC0000225
#define FILE_DEVICE_UNKNOWN 0x00000022
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#endif

// {GUID для драйвера}
DEFINE_GUID(GUID_DEVINTERFACE_KernelDriver, 
    0x12345678L, 0x9ABC, 0xDEF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0);

#define DEVICE_NAME L"\\Device\\KernelDriver"
#define SYMBOLIC_LINK_NAME L"\\DosDevices\\KernelDriver"

// IO Control Codes
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

// Контекст устройства
typedef struct _DEVICE_CONTEXT {
    WDFDEVICE Device;
    WDFQUEUE IoQueue;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

// Прототипы функций
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit);
VOID EvtDriverContextCleanup(WDFDRIVER Driver);
VOID EvtDeviceContextCleanup(PWDFDEVICE_INIT DeviceInit);
NTSTATUS EvtIoDefault(WDFQUEUE Queue, WDFREQUEST Request);
NTSTATUS EvtIoReadMemory(WDFQUEUE Queue, WDFREQUEST Request);
NTSTATUS EvtIoWriteMemory(WDFQUEUE Queue, WDFREQUEST Request);
NTSTATUS EvtIoGetProcessId(WDFQUEUE Queue, WDFREQUEST Request);
NTSTATUS EvtIoGetModuleBase(WDFQUEUE Queue, WDFREQUEST Request);

// Вспомогательные функции
PVOID GetProcessById(ULONG ProcessId);
PVOID GetModuleBaseByName(PUNICODE_STRING ModuleName, PVOID Process);
NTSTATUS ReadProcessMemory(PVOID Process, ULONG64 Address, PVOID Buffer, ULONG Size);
NTSTATUS WriteProcessMemory(PVOID Process, ULONG64 Address, PVOID Buffer, ULONG Size);
