#include "Main.h"
#include "Hooks.h"
#include <iostream>
#include <fstream>

// Глобальные переменные
HMODULE g_hModule = NULL;

// Файл лога
std::ofstream g_LogFile;

void Log(const char* format, ...)
{
    if (!g_LogFile.is_open())
        return;
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, format, args);
    va_end(args);
    
    g_LogFile << buffer << std::endl;
    g_LogFile.flush();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hModule = hModule;
        
        // Открытие файла лога
        char logPath[MAX_PATH];
        GetTempPathA(MAX_PATH, logPath);
        strcat_s(logPath, "RobloxDLL.log");
        g_LogFile.open(logPath, std::ios::app);
        
        Log("DLL_PROCESS_ATTACH - DLL loaded into process");
        
        // Инициализация хуков
        if (InitializeHooks()) {
            Log("Hooks initialized successfully");
        } else {
            Log("Failed to initialize hooks");
        }
        
        break;
    }
    case DLL_PROCESS_DETACH: {
        Log("DLL_PROCESS_DETACH - DLL unloading from process");
        
        // Очистка хуков
        CleanupHooks();
        
        // Закрытие файла лога
        if (g_LogFile.is_open()) {
            g_LogFile.close();
        }
        
        break;
    }
    case DLL_THREAD_ATTACH:
        Log("DLL_THREAD_ATTACH");
        break;
    case DLL_THREAD_DETACH:
        Log("DLL_THREAD_DETACH");
        break;
    }
    
    return TRUE;
}

EXPORT void Initialize()
{
    Log("Initialize() called");
    InitializeHooks();
}

EXPORT void Cleanup()
{
    Log("Cleanup() called");
    CleanupHooks();
}
