#include "ignore_parser.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <shlwapi.h>

namespace fs = std::filesystem;

IgnoreParser::IgnoreParser() {
    // Default system ignores
    addPattern(".git");
    addPattern(".git/*");
    addPattern(".gftp");
    addPattern(".gftp/*");
    addPattern("gftp.exe");
    addPattern("*.o");
    addPattern("*.obj");
}

void IgnoreParser::addPattern(const std::string& pat) {
    if (pat.empty()) return;
    // Normalize path separators to /
    std::string p = pat;
    std::replace(p.begin(), p.end(), '\\', '/');
    if (!p.empty() && p[0] == '/') p = p.substr(1);
    patterns.push_back(p);
}

void IgnoreParser::loadGitignore(const std::string& filePath) {
    if (!fs::exists(filePath)) return;
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        // Trim
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        size_t last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, (last - first + 1));

        if (line.empty() || line[0] == '#') continue;
        addPattern(line);
    }
}

void IgnoreParser::loadGftpignore(const std::string& filePath) {
    loadGitignore(filePath);
}

// Simple wildcard match for '*' and '?'
bool IgnoreParser::matchPattern(const char* pattern, const char* str) {
    if (*pattern == '\0' && *str == '\0') return true;

    if (*pattern == '*' && *(pattern + 1) != '\0' && *str == '\0') return false;

    if (*pattern == '?' || *pattern == *str) return matchPattern(pattern + 1, str + 1);

    if (*pattern == '*') return matchPattern(pattern + 1, str) || matchPattern(pattern, str + 1);

    return false;
}

bool IgnoreParser::isIgnored(const std::string& relativePath) const {
    std::string normPath = relativePath;
    std::replace(normPath.begin(), normPath.end(), '\\', '/');
    if (!normPath.empty() && normPath[0] == '/') normPath = normPath.substr(1);

    // Extract filename component
    fs::path p(normPath);
    std::string filename = p.filename().string();

    for (const auto& pat : patterns) {
        std::string pattern = pat;
        bool isDirOnly = false;
        if (!pattern.empty() && pattern.back() == '/') {
            isDirOnly = true;
            pattern.pop_back();
        }

        // Match exact or wildcard against path
        if (matchPattern(pattern.c_str(), normPath.c_str())) return true;
        if (matchPattern(pattern.c_str(), filename.c_str())) return true;

        // Path prefix matching (e.g., node_modules matches node_modules/express/index.js)
        if (normPath.rfind(pattern + "/", 0) == 0) return true;
        if (normPath.find("/" + pattern + "/") != std::string::npos) return true;
    }
    return false;
}
