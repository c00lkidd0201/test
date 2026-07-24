#include "Hooks.h"
#include "Main.h"
#include <d3d11.h>
#include <dxgi.h>

// Глобальные переменные для оригинальных функций
tD3D11Present oD3D11Present = NULL;
tD3D11ResizeBuffers oD3D11ResizeBuffers = NULL;

// Указатели на vtable
void** g_pD3D11DeviceVTable = NULL;
void** g_pDXGISwapChainVTable = NULL;

// Функции хуков
HRESULT WINAPI hkD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    Log("hkD3D11Present called");
    
    // Вызов оригинальной функции
    if (oD3D11Present)
        return oD3D11Present(pSwapChain, SyncInterval, Flags);
    
    return S_OK;
}

HRESULT WINAPI hkD3D11ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    Log("hkD3D11ResizeBuffers called - Width: %d, Height: %d", Width, Height);
    
    // Вызов оригинальной функции
    if (oD3D11ResizeBuffers)
        return oD3D11ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    
    return S_OK;
}

bool SetupHooks()
{
    Log("Setting up hooks...");
    
    // Получение устройства и swap chain
    // Это упрощенная версия - в реальности нужно правильно получать указатели
    
    // Пример получения через D3D11
    ID3D11Device* pDevice = NULL;
    IDXGISwapChain* pSwapChain = NULL;
    
    // Создание временного устройства для получения vtable
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;
    
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(scd));
    scd.BufferCount = 1;
    scd.BufferDesc.Width = 1;
    scd.BufferDesc.Height = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = GetDesktopWindow();
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;
    scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 1, 
        D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, &featureLevel, NULL);
    
    if (FAILED(hr)) {
        Log("Failed to create D3D11 device: 0x%X", hr);
        return false;
    }
    
    // Получение vtable
    g_pD3D11DeviceVTable = *(void***)pDevice;
    g_pDXGISwapChainVTable = *(void***)pSwapChain;
    
    // Сохранение оригинальных функций
    oD3D11Present = (tD3D11Present)g_pDXGISwapChainVTable[8]; // Present находится на 8 позиции
    oD3D11ResizeBuffers = (tD3D11ResizeBuffers)g_pDXGISwapChainVTable[13]; // ResizeBuffers на 13 позиции
    
    // Установка хуков
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    DetourAttach(&(PVOID&)oD3D11Present, hkD3D11Present);
    DetourAttach(&(PVOID&)oD3D11ResizeBuffers, hkD3D11ResizeBuffers);
    
    LONG result = DetourTransactionCommit();
    
    if (result != NO_ERROR) {
        Log("Failed to commit detour transaction: %d", result);
        DetourTransactionAbort();
        return false;
    }
    
    Log("Hooks set up successfully");
    
    // Очистка временных объектов
    if (pDevice) pDevice->Release();
    if (pSwapChain) pSwapChain->Release();
    
    return true;
}

void RemoveHooks()
{
    Log("Removing hooks...");
    
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    if (oD3D11Present)
        DetourDetach(&(PVOID&)oD3D11Present, hkD3D11Present);
    if (oD3D11ResizeBuffers)
        DetourDetach(&(PVOID&)oD3D11ResizeBuffers, hkD3D11ResizeBuffers);
    
    LONG result = DetourTransactionCommit();
    
    if (result != NO_ERROR) {
        Log("Failed to commit detour removal: %d", result);
        DetourTransactionAbort();
    } else {
        Log("Hooks removed successfully");
    }
}

bool InitializeHooks()
{
    Log("Initializing hooks...");
    
    // Инициализация Detours
    if (DetourIsHelperProcess()) {
        Log("Already in helper process");
        return true;
    }
    
    DetourRestoreAfterWith();
    
    // Установка хуков
    return SetupHooks();
}

void CleanupHooks()
{
    Log("Cleaning up hooks...");
    RemoveHooks();
}
