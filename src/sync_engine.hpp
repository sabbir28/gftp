#pragma once

#include "config.hpp"
#include "ftp_client.hpp"
#include "git_engine.hpp"
#include "ignore_parser.hpp"
#include "ui.hpp"
#include <string>
#include <vector>

struct SyncPlan {
    std::string localCommitSHA;
    std::string remoteCommitSHA;
    std::vector<FileChange> itemsToUpload;
    std::vector<FileChange> itemsToDelete;
    size_t totalBytesToUpload = 0;
};

class SyncEngine {
public:
    static bool testConnection(const GFtpConfig& cfg);
    
    static SyncPlan calculateSyncPlan(const GFtpConfig& cfg, const IgnoreParser& ignoreParser, bool forceAll = false);
    
    static bool executePush(const GFtpConfig& cfg, const SyncPlan& plan, bool dryRun = false);
    
    static void printHistoryLog();
};
