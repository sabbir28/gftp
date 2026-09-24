#pragma once

#include <string>
#include <vector>

struct GFtpConfig {
    std::string host = "";
    int port = 21;
    std::string user = "";
    std::string pass = "";
    std::string remote_dir = "/htdocs";
    bool passive = true;
    bool delete_remote = false;
    std::vector<std::string> custom_ignores;

    // Default target credentials pre-configured helper
    static GFtpConfig getDefaultPreset();

    // Directory paths
    static std::string getConfigDir();
    static std::string getConfigFilePath();
    static std::string getStateFilePath();
    static std::string getHistoryLogPath();

    // Persistence
    bool loadFromFile(const std::string& path = "");
    bool saveToFile(const std::string& path = "") const;

    // Inspection
    void display() const;
    bool isValid() const;
};
