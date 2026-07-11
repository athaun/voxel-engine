#pragma once

// Installs process-wide handlers for unhandled SEH exceptions (access
// violations, etc.), abort(), and std::terminate that print a symbolized
// stack trace to stderr before the process dies. Windows-only; a no-op
// elsewhere. Call once, as early as possible in main().
namespace CrashHandler {
    void install();
}
