@echo off
chcp 65001 >nul
echo ========================================
echo   Installing Kernel Mode Driver
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

:: Проверка Test Mode
bcdedit | find "testsigning on" >nul
if %errorLevel% neq 0 (
    echo Warning: Test Mode is not enabled!
    echo Enabling Test Mode...
    bcdedit /set testsigning on
    if %errorLevel% neq 0 (
        echo Error: Failed to enable Test Mode
        pause
        exit /b 1
    )
    echo Test Mode enabled. You may need to reboot.
    echo.
)

:: Поиск драйвера
echo Searching for driver files...

:: Проверка текущей директории
if exist "KernelDriver.sys" (
    set DRIVER_PATH=%cd%\KernelDriver.sys
    set INF_PATH=%cd%\KernelDriver.inf
    goto :INSTALL_DRIVER
)

:: Проверка в поддиректории
if exist "..\KernelDriver\Release\KernelDriver.sys" (
    set DRIVER_PATH=..\KernelDriver\Release\KernelDriver.sys
    set INF_PATH=..\KernelDriver\Release\KernelDriver.inf
    goto :INSTALL_DRIVER
)

if exist "..\KernelDriver\Debug\KernelDriver.sys" (
    set DRIVER_PATH=..\KernelDriver\Debug\KernelDriver.sys
    set INF_PATH=..\KernelDriver\Debug\KernelDriver.inf
    goto :INSTALL_DRIVER
)

echo Error: KernelDriver.sys not found!
echo Please build the driver first or place it in the current directory.
pause
exit /b 1

:INSTALL_DRIVER
echo Found driver: %DRIVER_PATH%
echo Found INF file: %INF_PATH%
echo.

:: Установка драйвера
echo Installing driver...
pnputil /add-driver %INF_PATH% /install
if %errorLevel% neq 0 (
    echo Error: Failed to install driver using pnputil
    echo Trying alternative method...
    
    :: Альтернативный метод через devcon
    where devcon >nul 2>&1
    if %errorLevel% equ 0 (
        devcon install %INF_PATH%
        if %errorLevel% neq 0 (
            echo Error: Failed to install with devcon
            goto :MANUAL_INSTALL
        )
    ) else (
        goto :MANUAL_INSTALL
    )
)

echo Driver installed successfully!
echo.

:: Проверка установки
sc query KernelDriver >nul 2>&1
if %errorLevel% equ 0 (
    echo Driver service exists.
    echo Starting service...
    sc start KernelDriver
    if %errorLevel% equ 0 (
        echo Driver started successfully!
    ) else (
        echo Warning: Failed to start driver service
    )
) else (
    echo Warning: Driver service not found
)

goto :COMPLETE

:MANUAL_INSTALL
echo.
echo Manual installation required:
echo 1. Open Device Manager (devmgmt.msc)
echo 2. Right-click on your computer name and select "Add legacy hardware"
echo 3. Select "Install the hardware that I manually select from a list"
echo 4. Select "System devices" and click Next
echo 5. Click "Have Disk" and browse to: %INF_PATH%
echo 6. Select "Kernel Mode Driver" and click Next
echo 7. Complete the installation

:COMPLETE
echo.
echo ========================================
echo   Driver Installation Complete
echo ========================================
echo.
echo Note: You may need to reboot for the driver to load properly.
echo After reboot, run enable_test_mode.bat to ensure Test Mode is active.
echo.
pause
