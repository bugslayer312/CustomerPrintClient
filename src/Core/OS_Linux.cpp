#include "OS.h"

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace OS {

/*std::string const& GetConfigDir() {
    static std::string const configDir(getpwuid(getuid())->pw_dir);
    return configDir;
} */

} // namespace OS