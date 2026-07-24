@echo off
chcp 65001 >nul
echo ========================================
echo   Uninstalling Kernel Mode Driver
echo ========================================
echo.

:: Проверка прав администратора
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Error: This script must be run as Administrator!
    echo Please right-click and select "Run as administrator"
    pause
    exit /b 1
)

:: Остановка сервиса
echo Stopping driver service...
sc stop KernelDriver >nul 2>&1

:: Удаление драйвера
echo Removing driver...
sc delete KernelDriver >nul 2>&1

:: Удаление через pnputil
echo Removing driver package...
pnputil /delete-driver oem*.inf /uninstall /force
if %errorLevel% neq 0 (
    echo Warning: pnputil deletion may have failed
)

:: Удаление файлов
echo Cleaning up files...
if exist "KernelDriver.sys" del /f /q KernelDriver.sys
if exist "KernelDriver.inf" del /f /q KernelDriver.inf
if exist "KernelDriver.pdb" del /f /q KernelDriver.pdb

:: Удаление из System32
echo Checking System32 directory...
if exist "%windir%\System32\drivers\KernelDriver.sys" (
    takeown /f "%windir%\System32\drivers\KernelDriver.sys" >nul 2>&1
    icacls "%windir%\System32\drivers\KernelDriver.sys" /grant administrators:F >nul 2>&1
    del /f /q "%windir%\System32\drivers\KernelDriver.sys"
)

if exist "%windir%\System32\KernelDriver.sys" (
    takeown /f "%windir%\System32\KernelDriver.sys" >nul 2>&1
    icacls "%windir%\System32\KernelDriver.sys" /grant administrators:F >nul 2>&1
    del /f /q "%windir%\System32\KernelDriver.sys"
)

echo.
echo ========================================
echo   Driver Uninstallation Complete
echo ========================================
echo.
echo The driver has been uninstalled.
echo You may need to reboot for changes to take full effect.
echo.
pause
