#pragma once

struct BinaryVersion {
    int Major;
    int Minor;
    int Release;
    int SubRelease;

    bool operator<(BinaryVersion const& rhs) const;
};

bool ParseVersion(char const* versionString, BinaryVersion& outResult);