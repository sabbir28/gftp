# gftp - Git-Aware Fast FTP Differential Sync Tool for Windows

`gftp` is a high-performance C++ command-line tool built specifically for Windows. It solves the classic issue of slow FTP deployments by leveraging **Git commit differential tracking** and **native WinINet APIs**. Instead of re-uploading thousands of files every time, `gftp` analyzes your Git history (or local folder timestamps) and uploads **only modified or newly added files**, drastically accelerating deployment speed.

---

## Key Features

- ⚡ **Git-Aware Differential Sync**: Calculates `git diff` against the last deployed commit (`.gftp_state` on FTP server) to sync only changed files.
- 🚀 **Native Windows WinINet Subsystem**: Zero external DLL dependencies (built directly on `wininet.dll` and `shlwapi.dll`).
- 📊 **Real-time Console UI**: ANSI VT colored table preview, progress bars with transfer speeds (KB/s, MB/s), ETA, and transfer logs.
- 📁 **Smart Ignore Engine**: Automatically honors `.gitignore` and `.gftpignore` rules to skip `node_modules`, `.git`, temporary files, and binaries.
- 🛡️ **Dry-Run Mode**: Inspect pending file additions, modifications, and deletions with `gftp push --dry-run` before executing real FTP transfers.
- 🔄 **Non-Git Fallback**: Works seamless with normal folders by using local timestamp tracking.

---

## Pre-Configured FTP Target

`gftp` comes with built-in setup presets for fast initialization:
- **Server**: `ftpupload.net`
- **Port**: `21`
- **Username**: `mseet_42012618`
- **Password**: `hacker321`
- **Remote Path**: `/htdocs`

---

## Quick Start Guide

### 1. Initialize Configuration
Run `gftp init --preset` to initialize credentials:
```cmd
gftp init --preset
```
Alternatively, configure custom FTP details:
```cmd
gftp init --host ftpupload.net --user mseet_42012618 --pass hacker321 --port 21 --remote-dir /htdocs
```

### 2. Test Connection
Verify authentication and server directory access:
```cmd
gftp test
```

### 3. Check Pending Changes
View files ready to be uploaded or deleted:
```cmd
gftp status
```

### 4. Preview Sync (Dry Run)
Simulate the upload process without modifying remote files:
```cmd
gftp push --dry-run
```

### 5. Execute Sync / Push
Perform differential synchronization:
```cmd
gftp push
```

---

## CLI Reference

| Command | Arguments | Description |
|---|---|---|
| `gftp init` | `--preset` or `--host <H> --user <U> ...` | Initialize `.gftp/config` in workspace |
| `gftp config` | `show` or `set <key> <val>` | View or update configuration parameters |
| `gftp test` | None | Test FTP server handshake & directory permissions |
| `gftp status` | None | Compare local git commit state with remote synced commit SHA |
| `gftp push` | `--dry-run`, `--all` | Upload modified/new files and update remote state SHA |
| `gftp sync` | Same as push | Alias for `gftp push` |
| `gftp log` | None | View local deployment history log |

---

## Build Instructions (MinGW / GCC)

Requirements: MinGW-w64 (`g++`) with C++17 support.

```cmd
mingw32-make
```

Or using CMake:
```cmd
mkdir build
cd build
cmake ..
cmake --build .
```
