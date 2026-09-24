#pragma once

#include <string>
#include <vector>

class IgnoreParser {
public:
    IgnoreParser();

    void addPattern(const std::string& pattern);
    void loadGitignore(const std::string& filePath = ".gitignore");
    void loadGftpignore(const std::string& filePath = ".gftpignore");

    bool isIgnored(const std::string& relativePath) const;

private:
    std::vector<std::string> patterns;
    static bool matchPattern(const char* pattern, const char* str);
};
