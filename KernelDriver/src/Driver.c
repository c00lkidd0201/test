#include "Driver.h"
#include <ntifs.h>

// Глобальные переменные
WDF_DRIVER_GLOBALS DriverGlobals;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG driverConfig;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFDRIVER driver;

    // Инициализация WDF
    WDF_DRIVER_CONFIG_INIT(&driverConfig, EvtDeviceAdd);
    driverConfig.DriverPoolTag = (ULONG)'KMDF';
    driverConfig.EvtDriverUnload = NULL;
    driverConfig.EvtDriverContextCleanup = EvtDriverContextCleanup;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.SynchronizationScope = WdfSynchronizationScopeInval;

    // Создание WDF драйвера
    status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, &driverConfig, &driver);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfDriverCreate failed: 0x%X\n", status);
        return status;
    }

    DbgPrint("KernelDriver: Driver loaded successfully\n");
    return status;
}

NTSTATUS EvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    WDFQUEUE queue;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_FILEOBJECT_CONFIG fileConfig;

    // Выделение контекста устройства
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = EvtDeviceContextCleanup;

    status = WdfDeviceInitAllocateContext(DeviceInit, &attributes);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfDeviceInitAllocateContext failed: 0x%X\n", status);
        return status;
    }

    // Настройка устройства
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetCharacteristics(DeviceInit, FILE_DEVICE_SECURE_OPEN, FALSE);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    // Создание устройства
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfDeviceCreate failed: 0x%X\n", status);
        return status;
    }

    deviceContext = WdfObjectGetContext(device);
    deviceContext->Device = device;

    // Создание символьной ссылки
    status = WdfDeviceCreateSymbolicLink(device, SYMBOLIC_LINK_NAME);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfDeviceCreateSymbolicLink failed: 0x%X\n", status);
        return status;
    }

    // Настройка очереди IOCTL
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDefault = EvtIoDefault;
    queueConfig.EvtIoDeviceControl = EvtIoDefault;

    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfIoQueueCreate failed: 0x%X\n", status);
        return status;
    }

    deviceContext->IoQueue = queue;

    // Регистрация IOCTL обработчиков
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    DbgPrint("KernelDriver: Device created successfully\n");
    return status;
}

VOID EvtDriverContextCleanup(WDFDRIVER Driver)
{
    DbgPrint("KernelDriver: Driver cleanup\n");
}

VOID EvtDeviceContextCleanup(PWDFDEVICE_INIT DeviceInit)
{
    PDEVICE_CONTEXT deviceContext = WdfObjectGetContext(WdfDeviceInitGetDevice(DeviceInit));
    if (deviceContext) {
        DbgPrint("KernelDriver: Device context cleanup\n");
    }
}

NTSTATUS EvtIoDefault(WDFQUEUE Queue, WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ioControlCode;
    size_t inputBufferLength, outputBufferLength;
    
    // Получение IOCTL кода
    status = WdfRequestRetrieveIoControlCode(Request, &ioControlCode);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfRequestRetrieveIoControlCode failed: 0x%X\n", status);
        return status;
    }

    // Обработка IOCTL команд
    switch (ioControlCode) {
    case IOCTL_READ_MEMORY:
        return EvtIoReadMemory(Queue, Request);
    case IOCTL_WRITE_MEMORY:
        return EvtIoWriteMemory(Queue, Request);
    case IOCTL_GET_PROCESS_ID:
        return EvtIoGetProcessId(Queue, Request);
    case IOCTL_GET_MODULE_BASE:
        return EvtIoGetModuleBase(Queue, Request);
    default:
        DbgPrint("KernelDriver: Unknown IOCTL code: 0x%X\n", ioControlCode);
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // Завершение запроса
    WdfRequestComplete(Request, status);
    return status;
}

NTSTATUS EvtIoReadMemory(WDFQUEUE Queue, WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;
    PREAD_MEMORY_REQUEST readRequest = NULL;
    size_t inputBufferLength, outputBufferLength;
    PVOID process = NULL;
    
    // Получение буферов
    status = WdfRequestRetrieveInputBuffer(Request, 0, (PVOID*)&readRequest, &inputBufferLength);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfRequestRetrieveInputBuffer failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    // Проверка размера структуры
    if (inputBufferLength < sizeof(READ_MEMORY_REQUEST) - 1) {
        DbgPrint("KernelDriver: Invalid READ_MEMORY_REQUEST size\n");
        WdfRequestComplete(Request, STATUS_INFO_LENGTH_MISMATCH);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    // Получение процесса
    process = GetProcessById((ULONG)readRequest->ProcessId);
    if (!process) {
        DbgPrint("KernelDriver: Process not found: %llu\n", readRequest->ProcessId);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    // Чтение памяти
    status = ReadProcessMemory(process, readRequest->Address, readRequest->Buffer, readRequest->Size);
    if (!NT_SUCCESS(status)) {
        DbgPrint("KernelDriver: ReadProcessMemory failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    // Установка вывода
    status = WdfRequestRetrieveOutputBuffer(Request, 0, (PVOID*)&readRequest, &outputBufferLength);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfRequestRetrieveOutputBuffer failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    // Установка размера вывода
    WdfRequestSetInformation(Request, readRequest->Size);
    WdfRequestComplete(Request, status);
    
    return status;
}

NTSTATUS EvtIoWriteMemory(WDFQUEUE Queue, WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;
    PWRITE_MEMORY_REQUEST writeRequest = NULL;
    size_t inputBufferLength;
    PVOID process = NULL;

    // Получение буферов
    status = WdfRequestRetrieveInputBuffer(Request, 0, (PVOID*)&writeRequest, &inputBufferLength);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfRequestRetrieveInputBuffer failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    // Проверка размера структуры
    if (inputBufferLength < sizeof(WRITE_MEMORY_REQUEST) - 1) {
        DbgPrint("KernelDriver: Invalid WRITE_MEMORY_REQUEST size\n");
        WdfRequestComplete(Request, STATUS_INFO_LENGTH_MISMATCH);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    // Получение процесса
    process = GetProcessById((ULONG)writeRequest->ProcessId);
    if (!process) {
        DbgPrint("KernelDriver: Process not found: %llu\n", writeRequest->ProcessId);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    // Запись памяти
    status = WriteProcessMemory(process, writeRequest->Address, writeRequest->Buffer, writeRequest->Size);
    if (!NT_SUCCESS(status)) {
        DbgPrint("KernelDriver: WriteProcessMemory failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    WdfRequestSetInformation(Request, writeRequest->Size);
    WdfRequestComplete(Request, status);
    
    return status;
}

NTSTATUS EvtIoGetProcessId(WDFQUEUE Queue, WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;
    PGET_PROCESS_ID_REQUEST request = NULL;
    size_t inputBufferLength, outputBufferLength;

    // Получение буферов
    status = WdfRequestRetrieveInputBuffer(Request, 0, (PVOID*)&request, &inputBufferLength);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfRequestRetrieveInputBuffer failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    // Проверка размера структуры
    if (inputBufferLength < sizeof(GET_PROCESS_ID_REQUEST)) {
        DbgPrint("KernelDriver: Invalid GET_PROCESS_ID_REQUEST size\n");
        WdfRequestComplete(Request, STATUS_INFO_LENGTH_MISMATCH);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    // Поиск процесса по имени
    UNICODE_STRING processName;
    RtlInitUnicodeString(&processName, request->ProcessName);
    
    // Проход по всем процессам
    PEPROCESS currentProcess = NULL;
    ULONG currentPid = 0;
    
    // Это упрощенная версия - в реальном драйвере нужно использовать PsLookupProcessByProcessId
    // и перебирать все процессы через PsGetNextProcess
    
    // Для примера - ищем Roblox
    if (wcsstr(request->ProcessName, L"Roblox") != NULL) {
        // Это заглушка - в реальности нужно правильно искать процесс
        request->ProcessId = 0; // Будет заменено на реальный PID
        
        // Попытка найти процесс
        status = PsLookupProcessByProcessId((HANDLE)4, &currentProcess); // Пример
        if (NT_SUCCESS(status)) {
            request->ProcessId = 4; // Пример
            ObDereferenceObject(currentProcess);
        }
    }

    // Установка вывода
    status = WdfRequestRetrieveOutputBuffer(Request, 0, (PVOID*)&request, &outputBufferLength);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfRequestRetrieveOutputBuffer failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    WdfRequestSetInformation(Request, sizeof(GET_PROCESS_ID_REQUEST));
    WdfRequestComplete(Request, status);
    
    return status;
}

NTSTATUS EvtIoGetModuleBase(WDFQUEUE Queue, WDFREQUEST Request)
{
    NTSTATUS status = STATUS_SUCCESS;
    PGET_MODULE_BASE_REQUEST request = NULL;
    size_t inputBufferLength, outputBufferLength;

    // Получение буферов
    status = WdfRequestRetrieveInputBuffer(Request, 0, (PVOID*)&request, &inputBufferLength);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfRequestRetrieveInputBuffer failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    // Проверка размера структуры
    if (inputBufferLength < sizeof(GET_MODULE_BASE_REQUEST)) {
        DbgPrint("KernelDriver: Invalid GET_MODULE_BASE_REQUEST size\n");
        WdfRequestComplete(Request, STATUS_INFO_LENGTH_MISMATCH);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    // Получение процесса
    PVOID process = GetProcessById(request->ProcessId);
    if (!process) {
        DbgPrint("KernelDriver: Process not found: %lu\n", request->ProcessId);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    // Получение базового адреса модуля
    UNICODE_STRING moduleName;
    RtlInitUnicodeString(&moduleName, request->ModuleName);
    
    PVOID moduleBase = GetModuleBaseByName(&moduleName, process);
    if (moduleBase) {
        request->ModuleBase = (ULONG64)moduleBase;
    } else {
        request->ModuleBase = 0;
        status = STATUS_NOT_FOUND;
    }

    // Установка вывода
    status = WdfRequestRetrieveOutputBuffer(Request, 0, (PVOID*)&request, &outputBufferLength);
    if (!NT_SUCCESS(status)) {
        DbgPrint("WdfRequestRetrieveOutputBuffer failed: 0x%X\n", status);
        WdfRequestComplete(Request, status);
        return status;
    }

    WdfRequestSetInformation(Request, sizeof(GET_MODULE_BASE_REQUEST));
    WdfRequestComplete(Request, status);
    
    return status;
}

// Вспомогательные функции
PVOID GetProcessById(ULONG ProcessId)
{
    PEPROCESS process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)ProcessId, &process);
    if (NT_SUCCESS(status)) {
        return process;
    }
    return NULL;
}

PVOID GetModuleBaseByName(PUNICODE_STRING ModuleName, PVOID Process)
{
    // Это заглушка - в реальности нужно использовать LdrGetDllHandle или аналоги
    // Для KMDF драйвера это сложно, обычно используется обход PEB
    
    PEPROCESS eprocess = (PEPROCESS)Process;
    PPEB peb = NULL;
    
    // Получение PEB
    __try {
        peb = (PPEB)PsGetProcessPeb(eprocess);
        if (!peb) {
            return NULL;
        }
        
        // Обход Ldr
        PLDR_DATA_TABLE_ENTRY ldrEntry = (PLDR_DATA_TABLE_ENTRY)peb->Ldr->InMemoryOrderModuleList.Flink;
        
        while (ldrEntry != &peb->Ldr->InMemoryOrderModuleList) {
            if (ldrEntry->BaseDllName.Buffer && 
                RtlEqualUnicodeString(&ldrEntry->BaseDllName, ModuleName, TRUE)) {
                return ldrEntry->DllBase;
            }
            ldrEntry = (PLDR_DATA_TABLE_ENTRY)ldrEntry->InMemoryOrderLinks.Flink;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
    
    return NULL;
}

NTSTATUS ReadProcessMemory(PVOID Process, ULONG64 Address, PVOID Buffer, ULONG Size)
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS eprocess = (PEPROCESS)Process;
    PVOID targetAddress = (PVOID)Address;
    
    // Проверка адреса
    if (!MmIsAddressValid(targetAddress)) {
        return STATUS_ACCESS_VIOLATION;
    }
    
    // Чтение памяти
    __try {
        // Прикрепиться к процессу
        KAPC_STATE apcState;
        KeStackAttachProcess(eprocess, &apcState);
        
        // Проверка доступа
        if (!MmIsAddressValid(targetAddress)) {
            KeUnstackDetachProcess(&apcState);
            return STATUS_ACCESS_VIOLATION;
        }
        
        // Копирование памяти
        RtlCopyMemory(Buffer, targetAddress, Size);
        
        KeUnstackDetachProcess(&apcState);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        DbgPrint("KernelDriver: Exception in ReadProcessMemory: 0x%X\n", status);
    }
    
    return status;
}

NTSTATUS WriteProcessMemory(PVOID Process, ULONG64 Address, PVOID Buffer, ULONG Size)
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS eprocess = (PEPROCESS)Process;
    PVOID targetAddress = (PVOID)Address;
    
    // Проверка адреса
    if (!MmIsAddressValid(targetAddress)) {
        return STATUS_ACCESS_VIOLATION;
    }
    
    // Запись памяти
    __try {
        // Прикрепиться к процессу
        KAPC_STATE apcState;
        KeStackAttachProcess(eprocess, &apcState);
        
        // Проверка доступа
        if (!MmIsAddressValid(targetAddress)) {
            KeUnstackDetachProcess(&apcState);
            return STATUS_ACCESS_VIOLATION;
        }
        
        // Изменение защиты памяти
        MEMORY_BASIC_INFORMATION mbi;
        status = ZwQueryVirtualMemory(NtCurrentProcess(), targetAddress, MemoryBasicInformation, &mbi, sizeof(mbi), NULL);
        if (!NT_SUCCESS(status)) {
            KeUnstackDetachProcess(&apcState);
            return status;
        }
        
        // Копирование памяти
        RtlCopyMemory(targetAddress, Buffer, Size);
        
        KeUnstackDetachProcess(&apcState);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        DbgPrint("KernelDriver: Exception in WriteProcessMemory: 0x%X\n", status);
    }
    
    return status;
}
