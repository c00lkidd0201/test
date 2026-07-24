#pragma once

#include <windows.h>

// Экспорт функции
#define EXPORT __declspec(dllexport)

// Основная функция DLL
extern "C" EXPORT BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

// Функция для инициализации
EXPORT void Initialize();

// Функция для очистки
EXPORT void Cleanup();

// Глобальные переменные
extern HMODULE g_hModule;
