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

// C-функция для инициализации хуков
extern "C" __declspec(dllexport) void Initialize() {
    Log("Initialize() called");
}

// C-функция для очистки
extern "C" __declspec(dllexport) void Cleanup() {
    Log("Cleanup() called");
}

// Основная функция DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hModule = hModule;
        Log("DLL_PROCESS_ATTACH");
        break;
    }
    case DLL_PROCESS_DETACH: {
        Log("DLL_PROCESS_DETACH");
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
