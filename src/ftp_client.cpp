#include "ftp_client.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

FtpClient::FtpClient() {}

FtpClient::~FtpClient() {
    disconnect();
}

void FtpClient::setLastError(const std::string& err) {
    DWORD winErr = ::GetLastError();
    if (winErr != 0) {
        char errBuf[256];
        DWORD len = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, winErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            errBuf, sizeof(errBuf), NULL
        );
        if (len > 0) {
            lastErrorStr = err + " (Code " + std::to_string(winErr) + ": " + std::string(errBuf) + ")";
            return;
        }
    }
    lastErrorStr = err;
}

std::string FtpClient::getLastErrorStr() const {
    return lastErrorStr;
}

bool FtpClient::connect(const std::string& host, int port, const std::string& user, const std::string& pass, bool passiveMode) {
    disconnect();

    hInternet = InternetOpenA("gftp/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        setLastError("Failed to initialize Windows WinINet Internet session");
        return false;
    }

    DWORD flags = passiveMode ? INTERNET_FLAG_PASSIVE : 0;
    hConnect = InternetConnectA(
        hInternet,
        host.c_str(),
        static_cast<INTERNET_PORT>(port),
        user.c_str(),
        pass.c_str(),
        INTERNET_SERVICE_FTP,
        flags,
        0
    );

    if (!hConnect) {
        setLastError("FTP Connection failed to host " + host + ":" + std::to_string(port));
        disconnect();
        return false;
    }

    return true;
}

void FtpClient::disconnect() {
    if (hConnect) {
        InternetCloseHandle(hConnect);
        hConnect = NULL;
    }
    if (hInternet) {
        InternetCloseHandle(hInternet);
        hInternet = NULL;
    }
}

bool FtpClient::isConnected() const {
    return hConnect != NULL;
}

bool FtpClient::changeDirectory(const std::string& remoteDir) {
    if (!hConnect) return false;
    if (remoteDir.empty() || remoteDir == ".") return true;
    return FtpSetCurrentDirectoryA(hConnect, remoteDir.c_str()) == TRUE;
}

bool FtpClient::createDirectory(const std::string& remoteDir) {
    if (!hConnect) return false;
    return FtpCreateDirectoryA(hConnect, remoteDir.c_str()) == TRUE;
}

bool FtpClient::ensureDirectoryExists(const std::string& remotePath) {
    if (!hConnect) return false;

    // Normalize slashes
    std::string path = remotePath;
    std::replace(path.begin(), path.end(), '\\', '/');

    // Remove file name part to leave parent dir
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) return true; // File in current dir

    std::string dirPath = path.substr(0, lastSlash);
    if (dirPath.empty() || dirPath == "/") return true;

    // Split dir into tokens
    std::stringstream ss(dirPath);
    std::string token;
    std::string current;

    if (dirPath[0] == '/') current = "/";

    while (std::getline(ss, token, '/')) {
        if (token.empty()) continue;
        if (!current.empty() && current.back() != '/') current += "/";
        current += token;

        // Try creating directory (ignore error if it already exists)
        FtpCreateDirectoryA(hConnect, current.c_str());
    }

    return true;
}

bool FtpClient::uploadFile(const std::string& localPath, const std::string& remotePath, ProgressCallback progressCb) {
    if (!hConnect) {
        setLastError("Not connected to FTP server");
        return false;
    }

    if (!fs::exists(localPath)) {
        setLastError("Local file does not exist: " + localPath);
        return false;
    }

    // Ensure target remote subdirectories exist
    ensureDirectoryExists(remotePath);

    std::ifstream inFile(localPath, std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) {
        setLastError("Unable to open local file: " + localPath);
        return false;
    }

    size_t fileSize = static_cast<size_t>(inFile.tellg());
    inFile.seekg(0, std::ios::beg);

    // Open remote file for writing using FtpOpenFileA for streaming chunk progress
    HINTERNET hFtpFile = FtpOpenFileA(
        hConnect,
        remotePath.c_str(),
        GENERIC_WRITE,
        FTP_TRANSFER_TYPE_BINARY,
        0
    );

    if (!hFtpFile) {
        // Fallback to FtpPutFileA if FtpOpenFileA fails
        if (FtpPutFileA(hConnect, localPath.c_str(), remotePath.c_str(), FTP_TRANSFER_TYPE_BINARY, 0) == TRUE) {
            if (progressCb) progressCb(fileSize, fileSize, 0.0);
            return true;
        }
        setLastError("Failed to create remote file on FTP: " + remotePath);
        return false;
    }

    const size_t chunkSize = 32768; // 32 KB chunks
    std::vector<char> buffer(chunkSize);
    size_t bytesUploaded = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    while (inFile.good() && bytesUploaded < fileSize) {
        inFile.read(buffer.data(), chunkSize);
        std::streamsize bytesRead = inFile.gcount();
        if (bytesRead <= 0) break;

        DWORD bytesWritten = 0;
        BOOL writeResult = InternetWriteFile(hFtpFile, buffer.data(), static_cast<DWORD>(bytesRead), &bytesWritten);
        if (!writeResult || bytesWritten == 0) {
            InternetCloseHandle(hFtpFile);
            setLastError("Error streaming bytes to FTP server for: " + remotePath);
            return false;
        }

        bytesUploaded += bytesWritten;

        if (progressCb) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(currentTime - startTime).count();
            double speedBps = (elapsed > 0) ? (bytesUploaded / elapsed) : 0.0;
            progressCb(bytesUploaded, fileSize, speedBps);
        }
    }

    InternetCloseHandle(hFtpFile);
    return (bytesUploaded >= fileSize || fileSize == 0);
}

bool FtpClient::deleteFile(const std::string& remotePath) {
    if (!hConnect) return false;
    return FtpDeleteFileA(hConnect, remotePath.c_str()) == TRUE;
}

bool FtpClient::removeDirectory(const std::string& remoteDir) {
    if (!hConnect) return false;
    return FtpRemoveDirectoryA(hConnect, remoteDir.c_str()) == TRUE;
}

bool FtpClient::cleanRemoteDirectory(const std::string& remoteDir) {
    if (!hConnect) return false;

    WIN32_FIND_DATAA findData;
    std::string searchPath = remoteDir.empty() ? "*.*" : (remoteDir + "/*.*");

    HINTERNET hFind = FtpFindFirstFileA(hConnect, searchPath.c_str(), &findData, INTERNET_FLAG_RELOAD, 0);
    if (!hFind) {
        return true;
    }

    std::vector<std::string> subFiles;
    std::vector<std::string> subDirs;

    do {
        std::string name = findData.cFileName;
        if (name == "." || name == "..") continue;

        std::string fullPath = remoteDir.empty() ? name : (remoteDir + "/" + name);

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            subDirs.push_back(fullPath);
        } else {
            subFiles.push_back(fullPath);
        }
    } while (InternetFindNextFileA(hFind, &findData));

    InternetCloseHandle(hFind);

    for (const auto& file : subFiles) {
        deleteFile(file);
    }

    for (const auto& dir : subDirs) {
        cleanRemoteDirectory(dir);
        removeDirectory(dir);
    }

    return true;
}

std::string FtpClient::getRemoteState(const std::string& remoteStateFilename) {
    if (!hConnect) return "";

    std::string tempLocalPath = ".gftp/temp_state";
    fs::create_directories(".gftp");

    BOOL res = FtpGetFileA(hConnect, remoteStateFilename.c_str(), tempLocalPath.c_str(), FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_ASCII, 0);
    if (!res) return "";

    std::ifstream stateFile(tempLocalPath);
    std::string sha;
    if (stateFile >> sha) {
        stateFile.close();
        fs::remove(tempLocalPath);
        return sha;
    }
    return "";
}

bool FtpClient::setRemoteState(const std::string& commitSHA, const std::string& remoteStateFilename) {
    if (!hConnect) return false;

    std::string tempLocalPath = ".gftp/temp_state";
    fs::create_directories(".gftp");

    std::ofstream stateFile(tempLocalPath);
    if (!stateFile.is_open()) return false;
    stateFile << commitSHA << "\n";
    stateFile.close();

    bool uploaded = uploadFile(tempLocalPath, remoteStateFilename, nullptr);
    fs::remove(tempLocalPath);
    return uploaded;
}
