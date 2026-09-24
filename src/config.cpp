#include "config.hpp"
#include "ui.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

GFtpConfig GFtpConfig::getDefaultPreset() {
    GFtpConfig cfg;
    cfg.host = "ftpupload.net";
    cfg.port = 21;
    cfg.user = "mseet_42012618";
    cfg.pass = "hacker321";
    cfg.remote_dir = "/htdocs";
    cfg.passive = true;
    cfg.delete_remote = false;
    cfg.custom_ignores = {".git", ".gftp", "node_modules", "*.tmp", "*.log", "gftp.exe"};
    return cfg;
}

std::string GFtpConfig::getConfigDir() {
    return ".gftp";
}

std::string GFtpConfig::getConfigFilePath() {
    return ".gftp/config";
}

std::string GFtpConfig::getStateFilePath() {
    return ".gftp/state";
}

std::string GFtpConfig::getHistoryLogPath() {
    return ".gftp/history.log";
}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool GFtpConfig::loadFromFile(const std::string& path) {
    std::string filePath = path.empty() ? getConfigFilePath() : path;
    if (!fs::exists(filePath)) {
        return false;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string line;
    custom_ignores.clear();

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = trim(line.substr(0, eqPos));
            std::string val = trim(line.substr(eqPos + 1));

            if (key == "host") host = val;
            else if (key == "port") port = std::stoi(val);
            else if (key == "user") user = val;
            else if (key == "pass") pass = val;
            else if (key == "remote_dir") remote_dir = val;
            else if (key == "passive") passive = (val == "true" || val == "1");
            else if (key == "delete_remote") delete_remote = (val == "true" || val == "1");
            else if (key == "ignore") custom_ignores.push_back(val);
        }
    }
    file.close();
    return true;
}

bool GFtpConfig::saveToFile(const std::string& path) const {
    std::string filePath = path.empty() ? getConfigFilePath() : path;
    fs::create_directories(fs::path(filePath).parent_path());

    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << "# gftp configuration file\n";
    file << "host=" << host << "\n";
    file << "port=" << port << "\n";
    file << "user=" << user << "\n";
    file << "pass=" << pass << "\n";
    file << "remote_dir=" << remote_dir << "\n";
    file << "passive=" << (passive ? "true" : "false") << "\n";
    file << "delete_remote=" << (delete_remote ? "true" : "false") << "\n";
    file << "\n# Ignore patterns\n";
    for (const auto& ig : custom_ignores) {
        file << "ignore=" << ig << "\n";
    }

    file.close();
    return true;
}

bool GFtpConfig::isValid() const {
    return !host.empty() && !user.empty() && port > 0;
}

void GFtpConfig::display() const {
    UI::printHeader("gftp Current Configuration");
    std::cout << "  Host:         " << (host.empty() ? "(not set)" : host) << "\n";
    std::cout << "  Port:         " << port << "\n";
    std::cout << "  Username:     " << (user.empty() ? "(not set)" : user) << "\n";
    std::cout << "  Password:     " << (pass.empty() ? "(not set)" : "********") << "\n";
    std::cout << "  Remote Dir:   " << remote_dir << "\n";
    std::cout << "  Passive Mode: " << (passive ? "Enabled" : "Disabled") << "\n";
    std::cout << "  Auto-Delete:  " << (delete_remote ? "Enabled" : "Disabled (Safe)") << "\n";
    std::cout << "  Ignores:      ";
    if (custom_ignores.empty()) {
        std::cout << "(none)\n";
    } else {
        for (size_t i = 0; i < custom_ignores.size(); ++i) {
            std::cout << custom_ignores[i] << (i + 1 < custom_ignores.size() ? ", " : "\n");
        }
    }
}
