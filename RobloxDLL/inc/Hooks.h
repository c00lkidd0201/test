#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

// Прототипы функций для хуков
typedef HRESULT (WINAPI *tD3D11Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT (WINAPI *tD3D11ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

// Глобальные переменные для хуков
extern tD3D11Present oD3D11Present;
extern tD3D11ResizeBuffers oD3D11ResizeBuffers;

// Функции хуков
HRESULT WINAPI hkD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT WINAPI hkD3D11ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

// Установка и удаление хуков
bool SetupHooks();
void RemoveHooks();

// Инициализация и очистка
bool InitializeHooks();
void CleanupHooks();
