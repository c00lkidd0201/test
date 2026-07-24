# Roblox Kernel Mode Driver Project

## 📋 Overview

This project provides a complete solution for creating a Kernel Mode Driver (KMDF) that can interact with Roblox processes. It includes:

- **KernelDriver**: A KMDF driver for Ring 0 memory operations
- **UserModeApp**: A user-mode application to communicate with the driver
- **RobloxExecutor**: A DLL injector for Roblox processes
- **RobloxDLL**: A DLL with hooks for Roblox functionality

## 🛠️ Requirements

### Software Requirements
- **Visual Studio 2022** (with Windows Driver Kit - WDK)
- **Windows 10/11** (x64)
- **Administrator privileges** (for driver installation)

### WDK Installation
1. Download and install **Windows Driver Kit (WDK)** for your Windows version
2. In Visual Studio Installer, select:
   - Desktop development with C++
   - Windows Driver Kit (WDK)
   - C++ KMDF driver support

## 📁 Project Structure

```
RobloxKernelProject/
├── RobloxKernelProject.sln          # Visual Studio Solution
├── README.md                         # This file
│
├── KernelDriver/                     # KMDF Driver
│   ├── inc/
│   │   └── Driver.h                  # Driver headers
│   ├── src/
│   │   └── Driver.c                  # Driver implementation
│   ├── KernelDriver.vcxproj          # VS Project file
│   └── KernelDriver.inf              # Driver INF file
│
├── UserModeApp/                      # User-mode interface
│   ├── inc/
│   │   └── DriverInterface.h         # Interface headers
│   ├── src/
│   │   ├── DriverInterface.cpp       # Interface implementation
│   │   └── main.cpp                  # Main application
│   └── UserModeApp.vcxproj            # VS Project file
│
├── RobloxExecutor/                   # DLL Injector
│   ├── inc/
│   │   └── Injector.h                # Injector headers
│   ├── src/
│   │   ├── Injector.cpp              # Injector implementation
│   │   └── main.cpp                  # Main executor
│   └── RobloxExecutor.vcxproj        # VS Project file
│
├── RobloxDLL/                        # Roblox DLL
│   ├── inc/
│   │   ├── Main.h                    # Main headers
│   │   └── Hooks.h                   # Hook headers
│   ├── src/
│   │   ├── Main.cpp                  # DLL entry point
│   │   └── Hooks.cpp                 # Hook implementation
│   └── RobloxDLL.vcxproj              # VS Project file
│
└── Scripts/                          # Installation scripts
    ├── enable_test_mode.bat          # Enable Test Mode
    ├── install_driver.bat            # Install driver
    └── uninstall_driver.bat          # Uninstall driver
```

## 🚀 Quick Start

### 1. Build the Projects

1. Open **RobloxKernelProject.sln** in Visual Studio 2022
2. Select **Release** configuration and **x64** platform
3. Build all projects (Ctrl+Shift+B)

### 2. Enable Test Mode

Run as Administrator:
```batch
Scripts\enable_test_mode.bat
```

This will enable Test Mode which allows loading unsigned drivers.

### 3. Install the Driver

Run as Administrator:
```batch
Scripts\install_driver.bat
```

This will install the KernelDriver and start the service.

### 4. Test the Driver

Run the UserModeApp to test communication with the driver:
```
UserModeApp\Release\UserModeApp.exe
```

### 5. Run Roblox Executor

1. Start Roblox game
2. Run the executor:
```
RobloxExecutor\Release\RobloxExecutor.exe
```

This will inject the RobloxDLL into the Roblox process.

## 🔧 Detailed Setup

### Building the Kernel Driver

1. **Install WDK**: Ensure Windows Driver Kit is installed
2. **Open Solution**: Open `RobloxKernelProject.sln` in Visual Studio
3. **Build KernelDriver**: Right-click on KernelDriver project → Build
4. **Output**: The driver will be created as `KernelDriver\Release\KernelDriver.sys`

### Driver Installation

#### Method 1: Using pnputil (Recommended)
```batch
pnputil /add-driver KernelDriver.inf /install
sc start KernelDriver
```

#### Method 2: Manual Installation
1. Open Device Manager (`devmgmt.msc`)
2. Right-click on your computer name → "Add legacy hardware"
3. Select "Install the hardware that I manually select from a list"
4. Select "System devices" → Next
5. Click "Have Disk" → Browse to `KernelDriver.inf`
6. Select "Kernel Mode Driver" → Next → Finish

### Enabling Test Mode

```batch
bcdedit /set testsigning on
```

**Note**: You must reboot after enabling Test Mode.

### Verifying Test Mode

```batch
bcdedit | find "testsigning"
```

Should show: `testsigning                   Yes`

## 🎮 Using with Roblox

### 1. Build All Components
- KernelDriver (creates .sys file)
- UserModeApp (creates .exe file)
- RobloxExecutor (creates .exe file)
- RobloxDLL (creates .dll file)

### 2. Enable Test Mode and Install Driver
```batch
Scripts\enable_test_mode.bat
Scripts\install_driver.bat
```

### 3. Start Roblox
- Launch Roblox Player or Studio
- Wait for the game to fully load

### 4. Inject the DLL
```
RobloxExecutor\Release\RobloxExecutor.exe
```

The executor will:
- Find the Roblox process
- Check architecture compatibility
- Inject RobloxDLL.dll into the process

### 5. Verify Injection
- Check the log file: `%TEMP%\RobloxDLL.log`
- The DLL should log hook installations and function calls

## 📊 IOCTL Commands

The driver supports the following IO Control Codes:

| Command | Code | Description |
|---------|------|-------------|
| IOCTL_READ_MEMORY | 0x800 | Read memory from a process |
| IOCTL_WRITE_MEMORY | 0x801 | Write memory to a process |
| IOCTL_GET_PROCESS_ID | 0x802 | Get process ID by name |
| IOCTL_GET_MODULE_BASE | 0x803 | Get module base address |

### Usage Example (C++)

```cpp
#include "DriverInterface.h"

DriverInterface driver;
if (driver.Connect()) {
    // Read memory
    UCHAR buffer[64];
    driver.ReadMemory(processId, address, buffer, sizeof(buffer));
    
    // Write memory
    UCHAR data[4] = {0x90, 0x90, 0x90, 0x90};
    driver.WriteMemory(processId, address, data, sizeof(data));
    
    // Get process ID
    ULONG pid = driver.GetProcessId(L"RobloxPlayerBeta.exe");
    
    // Get module base
    ULONG64 base = driver.GetModuleBase(pid, L"RobloxPlayerBeta.exe");
    
    driver.Disconnect();
}
```

## ⚠️ Important Notes

### Security Considerations
- **Test Mode** allows loading unsigned drivers (security risk)
- **Antivirus** may detect and block kernel drivers
- **Roblox Anti-Cheat** may detect and ban for using kernel drivers
- **Use at your own risk** - this is for educational purposes only

### Architecture Compatibility
- All components must be built for the same architecture (x64)
- The injector and target process must have matching architecture
- Roblox is typically x64, so all components should be x64

### Driver Signing
- For production use, drivers must be **code-signed**
- Test Mode allows unsigned drivers for development
- Consider using **WHQL certification** for distribution

## 🔍 Troubleshooting

### Driver Installation Fails
- **Check Test Mode**: `bcdedit | find "testsigning"`
- **Check Admin Rights**: Run CMD as Administrator
- **Check WDK**: Ensure WDK is properly installed
- **Check INF File**: Verify paths in KernelDriver.inf

### Driver Not Loading
- **Check Service**: `sc query KernelDriver`
- **Check Event Viewer**: Look for driver load errors
- **Check Dependencies**: Ensure all required files are present

### Connection Failed
- **Check Device**: `\\.\KernelDriver` should exist
- **Check Driver**: Ensure driver is running
- **Check Permissions**: User must have access to the device

### Roblox Injection Fails
- **Check Architecture**: Injector and Roblox must be same bitness
- **Check Process Name**: Verify Roblox process name
- **Check DLL Path**: Ensure RobloxDLL.dll is in the correct location
- **Check Anti-Cheat**: Roblox may have anti-injection protection

## 📝 License

This project is for **educational purposes only**. Use at your own risk.

## 🙏 Acknowledgments

- Microsoft WDK Documentation
- Detours Library (Microsoft Research)
- Roblox Developer Community

---

**⚠️ WARNING**: Using kernel drivers to modify game memory may violate terms of service and can result in account bans. This project is for educational and research purposes only.
