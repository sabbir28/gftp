# gftp: Git-Aware Differential FTP Sync Engine

`gftp` is a native Windows C++ command-line utility designed for fast, differential file synchronization over FTP. By analyzing Git commit state and local file manifests, `gftp` identifies modified, added, and deleted files, transferring only delta changes over FTP.

**Official Portal**: https://sabbir28.github.io/gftp/

---

## Overview

Traditional FTP tools re-examine or re-upload entire directory trees, introducing significant overhead during web application deployments. `gftp` solves this by tracking the synchronized remote commit SHA in a `.gftp_state` manifest on the remote server. On subsequent runs, `gftp` computes differential file sets using `git diff` or local timestamp hashes, reducing deployment duration by up to 95%.

### Core Features

- **Git-Aware Differential Engine**: Calculates precise file delta sets using Git commit tracking against `.gftp_state`.
- **Direct Remote Repository Management**: Clone and push remote Git repositories directly to FTP servers via `--git <url>` and `--ftp <dir>`.
- **Native WinINet Integration**: Built directly on `wininet.dll` and `shlwapi.dll` with zero external runtime dependencies.
- **Remote Directory Sanitization**: Supports recursive remote directory deletion via `gftp clean-remote`.
- **In-Place Terminal Telemetry**: Single-line real-time display showing file indices, cumulative payload bytes, transfer speed, and ETA.
- **Ignore Filter Engine**: Native parsing of `.gitignore` and `.gftpignore` pattern rules.
- **Pre-Configured Setup Preset**: One-step environment setup via `gftp init --preset`.

---

## Documentation

Comprehensive documentation is available on the GitHub Pages documentation portal:

- **User Guide & CLI Manual**: https://sabbir28.github.io/gftp/docs.html
- **C++ Architecture Specification**: https://sabbir28.github.io/gftp/architecture.html
- **Installation & Compilation Guide**: https://sabbir28.github.io/gftp/install.html
- **Releases & CI/CD Pipeline**: https://sabbir28.github.io/gftp/releases.html

---

## Command Line Interface

### Usage Syntax

```cmd
gftp <command> [options]
```

### Commands

| Command | Options | Description |
|---|---|---|
| `init` | `--preset`, `--host`, `--user`, `--pass`, `--remote-dir` | Initialize local `.gftp` configuration |
| `test` | None | Test FTP server authentication and directory permissions |
| `status` | None | Display local workspace commit SHA vs remote synced commit |
| `push` | `--git <url>`, `--ftp <dir>`, `--clean`, `--dry-run`, `--force` | Perform differential FTP upload |
| `sync` | Same as `push` | Alias for `push` |
| `clean-remote` | `--force`, `-y` | Recursively delete all files in remote target path |
| `log` | None | Display local sync execution history |
| `version` | None | Display binary version and compiler build metadata |

### Quick Start Examples

#### Initialize Workspace
```cmd
gftp init --preset
```

#### Test Server Connectivity
```cmd
gftp test
```

#### Preview Differential Upload (Dry Run)
```cmd
gftp push --dry-run
```

#### Execute Differential Sync
```cmd
gftp push
```

#### Direct Remote Git Repository Clone and Sync
```cmd
gftp --git https://github.com/sabbir28/MiniLMS --ftp /htdocs
```

#### Wipe Remote Directory
```cmd
gftp clean-remote --force
```

---

## Build Prerequisites and Compilation

### Prerequisites

- Operating System: Windows 10 or later (x64)
- Compiler: GCC / MinGW-w64 (C++17 compliant)
- Build System: GNU Make or CMake 3.15+

### Building with MinGW Make

```cmd
mingw32-make
```

### Building with CMake

```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## License

This project is licensed under the MIT License. See `LICENSE` for details.
