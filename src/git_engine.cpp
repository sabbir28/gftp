#include "git_engine.hpp"
#include "ui.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <filesystem>
#include <algorithm>
#include <memory>

namespace fs = std::filesystem;

std::string GitEngine::executeGitCommand(const std::string& args) {
    std::string command = "git " + args + " 2>nul";
    std::array<char, 512> buffer;
    std::string result;
    
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    _pclose(pipe);

    // Trim trailing newlines
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

bool GitEngine::isGitRepo() {
    std::string output = executeGitCommand("rev-parse --is-inside-work-tree");
    return (output == "true");
}

std::string GitEngine::getCurrentCommitSHA() {
    return executeGitCommand("rev-parse HEAD");
}

std::string GitEngine::getCurrentBranchName() {
    return executeGitCommand("rev-parse --abbrev-ref HEAD");
}

std::vector<FileChange> GitEngine::getDiffSinceCommit(const std::string& oldSHA) {
    std::vector<FileChange> changes;

    if (!isGitRepo()) {
        UI::printWarning("Not inside a Git repository. Falling back to timestamp diff scan.");
        return scanNonGitDiff(".gftp/local_manifest.dat");
    }

    std::string gitCmd;
    if (oldSHA.empty()) {
        // Initial push or no previous commit tracked: push all tracked files + untracked modifications
        gitCmd = "ls-files --stage --others --exclude-standard";
    } else {
        // Differential push between old SHA and local working state
        gitCmd = "diff --name-status " + oldSHA;
    }

    std::string output = executeGitCommand(gitCmd);
    if (output.empty() && !oldSHA.empty()) {
        // Also check for untracked / working tree changes
        gitCmd = "status --porcelain";
        output = executeGitCommand(gitCmd);
    }

    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        std::replace(line.begin(), line.end(), '\\', '/');

        FileChange fc;
        std::string filePath;

        if (oldSHA.empty()) {
            // Output from ls-files or status
            if (line.size() > 3 && (line[1] == ' ' || line[2] == ' ')) {
                char code = line[0];
                if (code == ' ' || code == '?') code = line[1];
                filePath = line.substr(3);
                fc.action = (code == 'D') ? FileChange::DELETED : FileChange::ADDED;
            } else {
                // ls-files format
                size_t tabPos = line.find('\t');
                if (tabPos != std::string::npos) {
                    filePath = line.substr(tabPos + 1);
                } else {
                    filePath = line;
                }
                fc.action = FileChange::ADDED;
            }
        } else {
            // git diff format: "M\tfile.txt" or "A\tfile.txt" or "D\tfile.txt"
            char statusChar = line[0];
            size_t spaceOrTab = line.find_first_of(" \t");
            if (spaceOrTab != std::string::npos) {
                filePath = line.substr(spaceOrTab + 1);
                // Handle rename R100 old\tnew
                size_t secondTab = filePath.find('\t');
                if (secondTab != std::string::npos) {
                    filePath = filePath.substr(secondTab + 1);
                }
            }

            if (statusChar == 'A') fc.action = FileChange::ADDED;
            else if (statusChar == 'M') fc.action = FileChange::MODIFIED;
            else if (statusChar == 'D') fc.action = FileChange::DELETED;
            else fc.action = FileChange::MODIFIED;
        }

        // Clean quotes from git output if any
        if (!filePath.empty() && filePath.front() == '"' && filePath.back() == '"') {
            filePath = filePath.substr(1, filePath.length() - 2);
        }

        fc.relativePath = filePath;
        if (fc.action != FileChange::DELETED && fs::exists(filePath)) {
            fc.size = fs::file_size(filePath);
        } else {
            fc.size = 0;
        }

        if (!filePath.empty()) {
            changes.push_back(fc);
        }
    }

    return changes;
}

std::vector<FileChange> GitEngine::scanNonGitDiff(const std::string& manifestPath) {
    std::vector<FileChange> changes;
    std::map<std::string, uint64_t> oldManifest;

    if (fs::exists(manifestPath)) {
        std::ifstream file(manifestPath);
        std::string line;
        while (std::getline(file, line)) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string path = line.substr(0, eq);
                uint64_t mtime = std::stoull(line.substr(eq + 1));
                oldManifest[path] = mtime;
            }
        }
    }

    for (const auto& entry : fs::recursive_directory_iterator(".")) {
        if (!entry.is_regular_file()) continue;
        std::string relPath = fs::relative(entry.path(), ".").string();
        std::replace(relPath.begin(), relPath.end(), '\\', '/');

        if (relPath.rfind(".git", 0) == 0 || relPath.rfind(".gftp", 0) == 0) continue;

        auto ftime = fs::last_write_time(entry.path());
        uint64_t mtime = std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();

        FileChange fc;
        fc.relativePath = relPath;
        fc.size = entry.file_size();

        if (oldManifest.find(relPath) == oldManifest.end()) {
            fc.action = FileChange::ADDED;
            changes.push_back(fc);
        } else if (oldManifest[relPath] != mtime) {
            fc.action = FileChange::MODIFIED;
            changes.push_back(fc);
        }
    }

    return changes;
}

void GitEngine::updateNonGitManifest(const std::string& manifestPath) {
    fs::create_directories(fs::path(manifestPath).parent_path());
    std::ofstream file(manifestPath);
    for (const auto& entry : fs::recursive_directory_iterator(".")) {
        if (!entry.is_regular_file()) continue;
        std::string relPath = fs::relative(entry.path(), ".").string();
        std::replace(relPath.begin(), relPath.end(), '\\', '/');
        if (relPath.rfind(".git", 0) == 0 || relPath.rfind(".gftp", 0) == 0) continue;

        auto ftime = fs::last_write_time(entry.path());
        uint64_t mtime = std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
        file << relPath << "=" << mtime << "\n";
    }
}
