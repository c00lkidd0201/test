#include "Main.h"
#include <windows.h>

// Глобальные переменные
HMODULE g_hModule = NULL;

// Простая функция логирования без C++ runtime
void Log(const char* format, ...) {
    // В упрощенной версии просто возвращаемся
    // В реальном проекте можно использовать OutputDebugStringA
    return;
}

// Функция для инициализации хуков
void Initialize() {
    Log("Initialize() called");
    InitializeHooks();
}

// Функция для очистки
void Cleanup() {
    Log("Cleanup() called");
    CleanupHooks();
}

// Основная функция DLL
extern "C" EXPORT BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hModule = hModule;
        Log("DLL_PROCESS_ATTACH");
        Initialize();
        break;
    }
    case DLL_PROCESS_DETACH: {
        Log("DLL_PROCESS_DETACH");
        Cleanup();
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
