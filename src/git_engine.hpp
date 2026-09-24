#pragma once

#include <string>
#include <vector>
#include <map>

struct FileChange {
    enum Action { ADDED, MODIFIED, DELETED, UNCHANGED };
    std::string relativePath;
    Action action;
    size_t size = 0;
};

class GitEngine {
public:
    static bool isGitRepo();
    static std::string getCurrentCommitSHA();
    static std::string getCurrentBranchName();

    // Returns differential files between old SHA and HEAD (or local uncommitted changes if oldSHA == "HEAD")
    static std::vector<FileChange> getDiffSinceCommit(const std::string& oldSHA);

    // Clone or pull remote git repository URL into target workspace
    static bool cloneOrFetchRepo(const std::string& repoUrl, const std::string& targetDir = ".");

    // Fallback scan for non-git folders
    static std::vector<FileChange> scanNonGitDiff(const std::string& manifestPath);
    static void updateNonGitManifest(const std::string& manifestPath);

private:
    static std::string executeGitCommand(const std::string& args);
};
