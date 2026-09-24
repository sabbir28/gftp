#include "ui.hpp"
#include "config.hpp"
#include "ignore_parser.hpp"
#include "git_engine.hpp"
#include "ftp_client.hpp"
#include "sync_engine.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

void printUsage() {
    UI::printBanner();
    std::cout << UI::BOLD << "USAGE:" << UI::RESET << "\n";
    std::cout << "  gftp <command> [options]\n\n";

    std::cout << UI::BOLD << "COMMANDS:" << UI::RESET << "\n";
    std::cout << "  " << UI::CYAN << "init" << UI::RESET << "       Initialize gftp configuration in current folder\n";
    std::cout << "              Options: --preset (Use default server preset: ftpupload.net)\n";
    std::cout << "                       --host <H> --user <U> --pass <P> --port <P> --remote-dir <R>\n";
    std::cout << "  " << UI::CYAN << "config" << UI::RESET << "     View or update settings\n";
    std::cout << "              Usage:   gftp config [show | set <key> <value>]\n";
    std::cout << "  " << UI::CYAN << "test" << UI::RESET << "       Test FTP server connection & authentication\n";
    std::cout << "  " << UI::CYAN << "status" << UI::RESET << "     Show pending modified/added/deleted files ready to push\n";
    std::cout << "  " << UI::CYAN << "push" << UI::RESET << "       Execute differential FTP synchronization\n";
    std::cout << "              Options: --dry-run (Preview without uploading)\n";
    std::cout << "                       --all / --force (Upload all tracked files regardless of SHA)\n";
    std::cout << "  " << UI::CYAN << "sync" << UI::RESET << "       Alias for 'push'\n";
    std::cout << "  " << UI::CYAN << "log" << UI::RESET << "        View sync history\n";
    std::cout << "  " << UI::CYAN << "version" << UI::RESET << "    Display version information\n";
    std::cout << "  " << UI::CYAN << "help" << UI::RESET << "       Show this help message\n\n";

    std::cout << UI::BOLD << "EXAMPLES:" << UI::RESET << "\n";
    std::cout << "  gftp init --preset\n";
    std::cout << "  gftp test\n";
    std::cout << "  gftp status\n";
    std::cout << "  gftp push --dry-run\n";
    std::cout << "  gftp push\n";
}

int main(int argc, char* argv[]) {
    UI::initConsole();

    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string cmd = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    if (cmd == "version" || cmd == "-v" || cmd == "--version") {
        std::cout << UI::CYAN << "gftp version 1.0.0 (Windows Native C++ WinINet Engine)" << UI::RESET << "\n";
        return 0;
    }

    if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        printUsage();
        return 0;
    }

    // Command: init
    if (cmd == "init") {
        UI::printBanner();
        GFtpConfig cfg;

        bool usePreset = false;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--preset") {
                usePreset = true;
            } else if (args[i] == "--host" && i + 1 < args.size()) cfg.host = args[++i];
            else if (args[i] == "--user" && i + 1 < args.size()) cfg.user = args[++i];
            else if (args[i] == "--pass" && i + 1 < args.size()) cfg.pass = args[++i];
            else if (args[i] == "--port" && i + 1 < args.size()) cfg.port = std::stoi(args[++i]);
            else if (args[i] == "--remote-dir" && i + 1 < args.size()) cfg.remote_dir = args[++i];
        }

        if (usePreset || (!usePreset && cfg.host.empty() && cfg.user.empty())) {
            cfg = GFtpConfig::getDefaultPreset();
            UI::printInfo("Loaded target preset configuration (ftpupload.net / mseet_42012618).");
        }

        if (cfg.saveToFile()) {
            UI::printSuccess("Initialized .gftp configuration file at: " + GFtpConfig::getConfigFilePath());
            cfg.display();
        } else {
            UI::printError("Failed to save configuration file.");
            return 1;
        }

        // Create default .gftpignore
        if (!fs::exists(".gftpignore")) {
            std::ofstream gi(".gftpignore");
            gi << "# gftp ignore rules\n.git/\n.gftp/\nnode_modules/\n*.tmp\n*.log\ngftp.exe\n";
            gi.close();
            UI::printSuccess("Created default .gftpignore template.");
        }

        return 0;
    }

    // Load config for all other commands
    GFtpConfig cfg;
    if (!cfg.loadFromFile()) {
        // If config doesn't exist yet, offer default preset
        cfg = GFtpConfig::getDefaultPreset();
    }

    // Command: config
    if (cmd == "config") {
        if (args.empty() || args[0] == "show") {
            cfg.display();
            return 0;
        }

        if (args[0] == "set" && args.size() >= 3) {
            std::string key = args[1];
            std::string val = args[2];

            if (key == "host") cfg.host = val;
            else if (key == "port") cfg.port = std::stoi(val);
            else if (key == "user") cfg.user = val;
            else if (key == "pass") cfg.pass = val;
            else if (key == "remote_dir") cfg.remote_dir = val;
            else if (key == "passive") cfg.passive = (val == "true" || val == "1");
            else if (key == "delete_remote") cfg.delete_remote = (val == "true" || val == "1");
            else {
                UI::printError("Unknown config key: " + key);
                return 1;
            }

            cfg.saveToFile();
            UI::printSuccess("Updated config key '" + key + "' = '" + val + "'");
            return 0;
        }

        UI::printError("Usage: gftp config [show | set <key> <value>]");
        return 1;
    }

    // Command: test
    if (cmd == "test") {
        if (!cfg.isValid()) {
            UI::printError("Invalid or incomplete configuration. Run 'gftp init' first.");
            return 1;
        }
        return SyncEngine::testConnection(cfg) ? 0 : 1;
    }

    // Prepare ignore parser
    IgnoreParser ignoreParser;
    for (const auto& pat : cfg.custom_ignores) {
        ignoreParser.addPattern(pat);
    }
    ignoreParser.loadGitignore(".gitignore");
    ignoreParser.loadGftpignore(".gftpignore");

    // Command: status
    if (cmd == "status") {
        UI::printHeader("gftp Sync Status");

        std::string currentSHA = GitEngine::getCurrentCommitSHA();
        if (!currentSHA.empty()) {
            UI::printInfo("Local Commit HEAD: " + currentSHA.substr(0, 8) + " (" + GitEngine::getCurrentBranchName() + ")");
        } else {
            UI::printInfo("Local directory (Non-Git mode)");
        }

        SyncPlan plan = SyncEngine::calculateSyncPlan(cfg, ignoreParser, false);
        if (!plan.remoteCommitSHA.empty()) {
            UI::printInfo("Last Remote Synced Commit: " + plan.remoteCommitSHA.substr(0, 8));
        } else {
            UI::printInfo("Remote state SHA: (Initial / full sync)");
        }

        std::vector<UI::FileItemUI> tableItems;
        for (const auto& item : plan.itemsToUpload) {
            tableItems.push_back({ (item.action == FileChange::ADDED ? "ADDED" : "MODIFIED"), item.relativePath, item.size });
        }
        for (const auto& item : plan.itemsToDelete) {
            tableItems.push_back({ "DELETED", item.relativePath, 0 });
        }

        UI::printFileTable(tableItems);
        UI::printInfo("Total pending payload: " + UI::formatBytes(plan.totalBytesToUpload) + " (" + std::to_string(plan.itemsToUpload.size()) + " files to upload, " + std::to_string(plan.itemsToDelete.size()) + " to delete)");
        return 0;
    }

    // Command: push / sync
    if (cmd == "push" || cmd == "sync") {
        bool dryRun = false;
        bool forceAll = false;

        for (const auto& arg : args) {
            if (arg == "--dry-run") dryRun = true;
            if (arg == "--all" || arg == "--force") forceAll = true;
        }

        if (!cfg.isValid()) {
            UI::printError("Invalid configuration. Run 'gftp init' to set up credentials.");
            return 1;
        }

        SyncPlan plan = SyncEngine::calculateSyncPlan(cfg, ignoreParser, forceAll);
        return SyncEngine::executePush(cfg, plan, dryRun) ? 0 : 1;
    }

    // Command: log
    if (cmd == "log") {
        SyncEngine::printHistoryLog();
        return 0;
    }

    UI::printError("Unknown command: '" + cmd + "'. Run 'gftp help' for instructions.");
    return 1;
}
