#pragma once

#include <string>

namespace lsql {

enum class BuildType {
    Debug,
    Release,
    RelWithDebInfo,
};

enum class Toggle {
    ON,
    OFF
};

enum class Platform {
    Unknown,
    Linux,
    MacOS,
};

struct ProjectVersion {
    int major;
    int minor;
    int patch;
};

std::string_view getBuildTimestamp();

std::string_view getGitCommitSHA1();

ProjectVersion getProjectVersion();

BuildType getBuildType();

Platform getCurrentPlatform();

Toggle isBuiltWithASAN();

Toggle isBuiltWithUBSAN();

Toggle isBuiltWithTSAN();

std::string formatBuildInfo();

}  // namespace lsql

#if !defined(NDEBUG)

#define LSQL_BUILD_DEBUG

#endif
