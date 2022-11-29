#include "BinaryVersion.h"

#include <cstdio>

bool BinaryVersion::operator<(BinaryVersion const& rhs) const {
    return Major < rhs.Major || Major == rhs.Major && (
        Minor < rhs.Minor || Minor == rhs.Minor && (
            Release < rhs.Release || Release == rhs.Release && (
                SubRelease < rhs.SubRelease
            )
        )
    );
}

bool ParseVersion(char const* versionString, BinaryVersion& outResult) {
    return std::sscanf(versionString, "%d.%d.%d.%d", &outResult.Major, &outResult.Minor,
        &outResult.Release, &outResult.SubRelease) == 4;
}