#include "ui.hpp"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace UI {
    const char* RESET   = "\033[0m";
    const char* BOLD    = "\033[1m";
    const char* RED     = "\033[31m";
    const char* GREEN   = "\033[32m";
    const char* YELLOW  = "\033[33m";
    const char* BLUE    = "\033[34m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN    = "\033[36m";
    const char* WHITE   = "\033[37m";
    const char* GRAY    = "\033[90m";

    void initConsole() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
        // Set UTF-8 output code page
        SetConsoleOutputCP(CP_UTF8);
    }

    std::string formatBytes(size_t bytes) {
        if (bytes == 0) return "0 B";
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int i = 0;
        double size = static_cast<double>(bytes);
        while (size >= 1024.0 && i < 4) {
            size /= 1024.0;
            i++;
        }
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(i == 0 ? 0 : 2) << size << " " << units[i];
        return ss.str();
    }

    std::string formatDuration(double seconds) {
        if (seconds < 0) seconds = 0;
        int totalSec = static_cast<int>(seconds);
        int mins = totalSec / 60;
        int secs = totalSec % 60;
        std::ostringstream ss;
        if (mins > 0) {
            ss << mins << "m " << secs << "s";
        } else {
            ss << secs << "s";
        }
        return ss.str();
    }

    void printBanner() {
        std::cout << CYAN << BOLD
                  << "   __ _ / _| |_ \n"
                  << "  / _` | |_|  _|\n"
                  << " | (_| |  _| |  \n"
                  << "  \\__, |_| |_|   " << WHITE << "v1.0.0 (Git-Aware FTP Sync CLI for Windows)\n"
                  << CYAN << "  |___/         \n" << RESET
                  << GRAY << "===============================================================\n" << RESET;
    }

    void printSuccess(const std::string& msg) {
        std::cout << GREEN << BOLD << "[✓] " << RESET << msg << "\n";
    }

    void printInfo(const std::string& msg) {
        std::cout << CYAN << BOLD << "[i] " << RESET << msg << "\n";
    }

    void printWarning(const std::string& msg) {
        std::cout << YELLOW << BOLD << "[!] " << RESET << msg << "\n";
    }

    void printError(const std::string& msg) {
        std::cerr << RED << BOLD << "[✗] Error: " << RESET << msg << "\n";
    }

    void printHeader(const std::string& title) {
        std::cout << "\n" << BOLD << MAGENTA << "=== " << title << " ===" << RESET << "\n";
    }

    void renderBatchProgressBar(
        const std::string& currentFilename,
        size_t currentFileIndex,
        size_t totalFilesCount,
        size_t fileTransferredBytes,
        size_t fileTotalBytes,
        size_t batchTransferredBytes,
        size_t batchTotalBytes,
        double speedBps
    ) {
        const int barWidth = 20;
        size_t overallTransferred = batchTransferredBytes + fileTransferredBytes;
        double pct = (batchTotalBytes > 0) ? (static_cast<double>(overallTransferred) / batchTotalBytes) : 1.0;
        if (pct > 1.0) pct = 1.0;
        int pos = static_cast<int>(barWidth * pct);

        std::string fn = currentFilename;
        if (fn.length() > 20) {
            fn = "..." + fn.substr(fn.length() - 17);
        }

        std::ostringstream bar;
        // Carriage return and ANSI reset to refresh line in-place
        bar << "\r" << BOLD << CYAN << "[" << currentFileIndex << "/" << totalFilesCount << "] " << RESET;
        bar << WHITE << std::left << std::setw(21) << fn << RESET << " [";
        
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) bar << GREEN << "=";
            else if (i == pos) bar << GREEN << ">";
            else bar << GRAY << " ";
        }
        
        bar << RESET << "] " << std::right << std::setw(3) << static_cast<int>(pct * 100) << "% ";
        bar << "(" << formatBytes(overallTransferred) << "/" << formatBytes(batchTotalBytes) << ") ";
        
        if (speedBps > 0) {
            bar << YELLOW << formatBytes(static_cast<size_t>(speedBps)) << "/s" << RESET;
        }

        // Pad trailing space to overwrite previous longer text
        bar << "   ";

        std::cout << bar.str() << std::flush;
    }

    void finishProgressBar(bool success) {
        if (success) {
            std::cout << "\r" << std::string(80, ' ') << "\r"; // Clear line
        } else {
            std::cout << RED << " [FAILED]" << RESET << "\n";
        }
    }

    void printFileTable(const std::vector<FileItemUI>& files) {
        if (files.empty()) {
            std::cout << GRAY << "  No file modifications detected." << RESET << "\n";
            return;
        }

        std::cout << BOLD << std::left 
                  << std::setw(12) << "ACTION"
                  << std::setw(45) << "FILE PATH"
                  << std::setw(15) << "SIZE"
                  << RESET << "\n";
        std::cout << GRAY << std::string(72, '-') << RESET << "\n";

        for (const auto& item : files) {
            const char* color = WHITE;
            if (item.status == "ADDED") color = GREEN;
            else if (item.status == "MODIFIED") color = YELLOW;
            else if (item.status == "DELETED") color = RED;
            else if (item.status == "UNCHANGED") color = GRAY;

            std::string pathDisp = item.path;
            if (pathDisp.length() > 42) {
                pathDisp = "..." + pathDisp.substr(pathDisp.length() - 39);
            }

            std::cout << color << std::left 
                      << std::setw(12) << item.status
                      << std::setw(45) << pathDisp
                      << std::setw(15) << (item.status == "DELETED" ? "-" : formatBytes(item.size))
                      << RESET << "\n";
        }
        std::cout << GRAY << std::string(72, '-') << RESET << "\n";
    }
}
