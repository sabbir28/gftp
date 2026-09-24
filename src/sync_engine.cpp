#include "sync_engine.hpp"
#include "ui.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

bool SyncEngine::testConnection(const GFtpConfig& cfg) {
    UI::printHeader("Testing FTP Server Connection");
    UI::printInfo("Connecting to " + cfg.host + ":" + std::to_string(cfg.port) + " as user '" + cfg.user + "'...");

    FtpClient ftp;
    auto startTime = std::chrono::high_resolution_clock::now();

    if (!ftp.connect(cfg.host, cfg.port, cfg.user, cfg.pass, cfg.passive)) {
        UI::printError("FTP Connection test FAILED!");
        UI::printError(ftp.getLastErrorStr());
        return false;
    }

    auto connTime = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(connTime - startTime).count();

    UI::printSuccess("Connected successfully! (Handshake & Authentication time: " + UI::formatDuration(elapsed) + ")");

    if (!cfg.remote_dir.empty()) {
        UI::printInfo("Checking remote directory: " + cfg.remote_dir);
        if (ftp.changeDirectory(cfg.remote_dir)) {
            UI::printSuccess("Remote directory '" + cfg.remote_dir + "' exists and is accessible.");
        } else {
            UI::printWarning("Remote directory '" + cfg.remote_dir + "' does not exist or access denied. Will attempt creation during push.");
        }
    }

    std::string remoteStateSHA = ftp.getRemoteState();
    if (!remoteStateSHA.empty()) {
        UI::printInfo("Found existing remote commit state tracking SHA: " + remoteStateSHA.substr(0, 8));
    } else {
        UI::printInfo("No previous remote state tracking file (.gftp_state) found. First push will sync complete codebase.");
    }

    ftp.disconnect();
    UI::printSuccess("FTP Server test passed successfully!");
    return true;
}

SyncPlan SyncEngine::calculateSyncPlan(const GFtpConfig& cfg, const IgnoreParser& ignoreParser, bool forceAll) {
    SyncPlan plan;

    plan.localCommitSHA = GitEngine::getCurrentCommitSHA();

    // Check remote SHA from local cache or FTP server
    std::string cachedRemoteSHA = "";
    std::string stateFile = GFtpConfig::getStateFilePath();
    if (fs::exists(stateFile)) {
        std::ifstream f(stateFile);
        f >> cachedRemoteSHA;
    }

    if (forceAll) {
        plan.remoteCommitSHA = "";
    } else {
        plan.remoteCommitSHA = cachedRemoteSHA;
    }

    std::vector<FileChange> rawChanges = GitEngine::getDiffSinceCommit(plan.remoteCommitSHA);

    for (const auto& fc : rawChanges) {
        // Filter out ignored files
        if (ignoreParser.isIgnored(fc.relativePath)) {
            continue;
        }

        if (fc.action == FileChange::DELETED) {
            if (cfg.delete_remote) {
                plan.itemsToDelete.push_back(fc);
            }
        } else {
            plan.itemsToUpload.push_back(fc);
            plan.totalBytesToUpload += fc.size;
        }
    }

    return plan;
}

bool SyncEngine::executePush(const GFtpConfig& cfg, const SyncPlan& plan, bool dryRun) {
    if (plan.itemsToUpload.empty() && plan.itemsToDelete.empty()) {
        UI::printSuccess("Already up to date! 0 files need to be synchronized.");
        return true;
    }

    if (dryRun) {
        UI::printHeader("DRY RUN MODE - Preview of Pending FTP Sync");
        std::vector<UI::FileItemUI> tableItems;
        for (const auto& item : plan.itemsToUpload) {
            tableItems.push_back({ (item.action == FileChange::ADDED ? "ADDED" : "MODIFIED"), item.relativePath, item.size });
        }
        for (const auto& item : plan.itemsToDelete) {
            tableItems.push_back({ "DELETED", item.relativePath, 0 });
        }
        UI::printFileTable(tableItems);
        UI::printInfo("Total payload: " + UI::formatBytes(plan.totalBytesToUpload) + " across " + std::to_string(plan.itemsToUpload.size()) + " file(s).");
        UI::printSuccess("Dry run preview complete. No files were uploaded or deleted.");
        return true;
    }

    UI::printHeader("Starting Differential FTP Synchronization");
    UI::printInfo("Target: " + cfg.user + "@" + cfg.host + ":" + std::to_string(cfg.port) + cfg.remote_dir);
    UI::printInfo("Files to upload: " + std::to_string(plan.itemsToUpload.size()) + " (" + UI::formatBytes(plan.totalBytesToUpload) + ")");
    if (cfg.delete_remote) {
        UI::printInfo("Files to delete: " + std::to_string(plan.itemsToDelete.size()));
    }

    FtpClient ftp;
    if (!ftp.connect(cfg.host, cfg.port, cfg.user, cfg.pass, cfg.passive)) {
        UI::printError("Failed to connect to FTP server!");
        UI::printError(ftp.getLastErrorStr());
        return false;
    }

    if (!cfg.remote_dir.empty() && cfg.remote_dir != "/") {
        ftp.ensureDirectoryExists(cfg.remote_dir + "/test.tmp");
        ftp.changeDirectory(cfg.remote_dir);
    }

    auto pushStart = std::chrono::high_resolution_clock::now();
    size_t uploadedFilesCount = 0;
    size_t uploadedBytesTotal = 0;
    bool allSuccess = true;

    // Upload Files
    for (size_t i = 0; i < plan.itemsToUpload.size(); ++i) {
        const auto& item = plan.itemsToUpload[i];
        std::string targetRemote = cfg.remote_dir;
        if (!targetRemote.empty() && targetRemote.back() != '/') targetRemote += "/";
        targetRemote += item.relativePath;

        bool uploadOk = ftp.uploadFile(
            item.relativePath,
            targetRemote,
            [&](size_t transferred, size_t total, double speed) {
                UI::renderBatchProgressBar(
                    item.relativePath,
                    i + 1,
                    plan.itemsToUpload.size(),
                    transferred,
                    total,
                    uploadedBytesTotal,
                    plan.totalBytesToUpload,
                    speed
                );
            }
        );

        if (uploadOk) {
            uploadedFilesCount++;
            uploadedBytesTotal += item.size;
        } else {
            UI::finishProgressBar(false);
            UI::printError("Failed uploading " + item.relativePath + ": " + ftp.getLastErrorStr());
            allSuccess = false;
        }
    }
    UI::finishProgressBar(true);

    // Delete Files
    if (cfg.delete_remote) {
        for (const auto& item : plan.itemsToDelete) {
            std::string targetRemote = cfg.remote_dir;
            if (!targetRemote.empty() && targetRemote.back() != '/') targetRemote += "/";
            targetRemote += item.relativePath;

            UI::printInfo("Deleting remote file: " + targetRemote);
            ftp.deleteFile(targetRemote);
        }
    }

    // Update state tracking
    if (allSuccess) {
        if (!plan.localCommitSHA.empty()) {
            ftp.setRemoteState(plan.localCommitSHA);
            
            // Save local cache state
            fs::create_directories(".gftp");
            std::ofstream sf(GFtpConfig::getStateFilePath());
            sf << plan.localCommitSHA << "\n";
            sf.close();
        } else {
            GitEngine::updateNonGitManifest(".gftp/local_manifest.dat");
        }
    }

    auto pushEnd = std::chrono::high_resolution_clock::now();
    double totalDuration = std::chrono::duration<double>(pushEnd - pushStart).count();
    double avgSpeed = (totalDuration > 0) ? (uploadedBytesTotal / totalDuration) : 0.0;

    ftp.disconnect();

    if (allSuccess) {
        UI::printHeader("Synchronization Complete!");
        UI::printSuccess("Successfully synced " + std::to_string(uploadedFilesCount) + " file(s) (" + UI::formatBytes(uploadedBytesTotal) + ") in " + UI::formatDuration(totalDuration) + "!");
        UI::printInfo("Average transfer speed: " + UI::formatBytes(static_cast<size_t>(avgSpeed)) + "/s");

        // Append to history log
        fs::create_directories(".gftp");
        std::ofstream logFile(GFtpConfig::getHistoryLogPath(), std::ios::app);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            char timeBuf[100];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
            logFile << "[" << timeBuf << "] PUSH " << uploadedFilesCount << " files (" 
                    << UI::formatBytes(uploadedBytesTotal) << ") SHA: " << (plan.localCommitSHA.empty() ? "N/A" : plan.localCommitSHA.substr(0, 8))
                    << " Duration: " << UI::formatDuration(totalDuration) << "\n";
        }
    } else {
        UI::printError("Push completed with errors. Check logs above.");
    }

    return allSuccess;
}

void SyncEngine::printHistoryLog() {
    UI::printHeader("gftp Sync History Log");
    std::string path = GFtpConfig::getHistoryLogPath();
    if (!fs::exists(path)) {
        UI::printInfo("No sync history recorded yet.");
        return;
    }

    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        std::cout << "  " << line << "\n";
    }
}
