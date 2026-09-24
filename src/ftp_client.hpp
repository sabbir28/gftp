#pragma once

#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include <wininet.h>

typedef std::function<void(size_t bytesTransferred, size_t totalBytes, double speedBps)> ProgressCallback;

class FtpClient {
public:
    FtpClient();
    ~FtpClient();

    bool connect(const std::string& host, int port, const std::string& user, const std::string& pass, bool passiveMode = true);
    void disconnect();
    bool isConnected() const;

    bool changeDirectory(const std::string& remoteDir);
    bool createDirectory(const std::string& remoteDir);
    bool ensureDirectoryExists(const std::string& remotePath);

    bool uploadFile(const std::string& localPath, const std::string& remotePath, ProgressCallback progressCb = nullptr);
    bool deleteFile(const std::string& remotePath);

    // Remote commit state tracking (.gftp_state on server)
    std::string getRemoteState(const std::string& remoteStateFilename = ".gftp_state");
    bool setRemoteState(const std::string& commitSHA, const std::string& remoteStateFilename = ".gftp_state");

    std::string getLastErrorStr() const;

private:
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    std::string lastErrorStr;

    void setLastError(const std::string& err);
};
