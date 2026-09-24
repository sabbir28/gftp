#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace UI {
    // Enable ANSI colors in Windows Console
    void initConsole();

    // Color definitions
    extern const char* RESET;
    extern const char* BOLD;
    extern const char* RED;
    extern const char* GREEN;
    extern const char* YELLOW;
    extern const char* BLUE;
    extern const char* MAGENTA;
    extern const char* CYAN;
    extern const char* WHITE;
    extern const char* GRAY;

    // Formatting helpers
    std::string formatBytes(size_t bytes);
    std::string formatDuration(double seconds);

    // UI Components
    void printBanner();
    void printSuccess(const std::string& msg);
    void printInfo(const std::string& msg);
    void printWarning(const std::string& msg);
    void printError(const std::string& msg);
    void printHeader(const std::string& title);

    // Progress Bar
    void renderBatchProgressBar(
        const std::string& currentFilename,
        size_t currentFileIndex,
        size_t totalFilesCount,
        size_t fileTransferredBytes,
        size_t fileTotalBytes,
        size_t batchTransferredBytes,
        size_t batchTotalBytes,
        double speedBps
    );
    void finishProgressBar(bool success = true);

    // Summary tables
    struct FileItemUI {
        std::string status; // "ADDED", "MODIFIED", "DELETED", "UNCHANGED"
        std::string path;
        size_t size;
    };
    void printFileTable(const std::vector<FileItemUI>& files);
}
