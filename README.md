# Project Glass (Launcher)

![Platform](https://img.shields.io/badge/platform-Windows-0078D6.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B%20%7C%20Win32-00599C.svg)
![Architecture](https://img.shields.io/badge/architecture-Cloud%20Connected-orange.svg)

**Project Glass** is a native, borderless Windows frontend for the [Cofee Analysis Engine](https://github.com/shadowofaroman/Operation-Cofee). It bridges the gap between raw CLI tools and modern user experiences using pure Win32 API and COM.

> "The interface is the product."

## 💎 Features

- **Modern Native UI:** Custom-drawn borderless window with specific GDI+ styling and "Segoe UI" typography.
- **Smart Dependency Management:** Automatically detects if the analysis engine (`cofee.exe`) is missing and fetches it Over-The-Air (OTA) from our [Rust Cloud Vault](https://github.com/shadowofaroman/Operation-Vault).
- **Process Injection:** Spawns child processes via Windows Pipes to capture stdout/stderr in real-time without showing console windows.
- **COM Integration:** Utilizes the modern Windows `IFileDialog` for native folder selection.

## 🏗️ Architecture

1.  **Frontend:** Project Glass (C++ / Win32)
2.  **Engine:** Cofee (C++17 / Multithreaded)
3.  **Infrastructure:** Operation Vault (Rust / Axum / Render Cloud)

## 📦 Usage

1.  Download `Glass.exe` from [Releases](#).
2.  Run it.
3.  **Auto-Repair:** If `cofee.exe` is not found, Glass will request to download it from the cloud automatically.
4.  Select a target folder and click **Launch**.

## 🛠️ Build from Source

- **IDE:** Visual Studio 2022
- **Workload:** Desktop development with C++
- **Dependencies:** `urlmon.lib`, `comctl32.lib` (Linked automatically in source)

## 📝 License

MIT License.