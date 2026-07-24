@echo off
chcp 65001 >nul
echo ========================================
echo   Enabling Test Mode for Driver Signing
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

echo Checking current Test Mode status...
bcdedit | find "testsigning"
if %errorLevel% equ 0 (
    echo Test Mode is already enabled.
    goto :CHECK_DRIVER
)

echo Enabling Test Mode...
bcdedit /set testsigning on
if %errorLevel% neq 0 (
    echo Error: Failed to enable Test Mode
    pause
    exit /b 1
)

echo Test Mode enabled successfully!
echo.

:CHECK_DRIVER
echo Checking if driver is installed...
sc query KernelDriver >nul 2>&1
if %errorLevel% equ 0 (
    echo Driver service exists.
    goto :START_SERVICE
)

echo Driver not installed. Please install the driver first.
echo.

:START_SERVICE
echo Starting driver service...
sc start KernelDriver
if %errorLevel% neq 0 (
    echo Warning: Failed to start driver service
    echo This might be normal if the driver is not properly installed
)

echo.
echo ========================================
echo   Test Mode Setup Complete
echo ========================================
echo.
echo You may need to reboot your computer for changes to take effect.
echo After reboot, the driver should load automatically.
echo.
pause
