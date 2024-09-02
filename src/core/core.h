#pragma once

#include "window.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

namespace Core {
    void init();
    void run(void (*user_function)());
    void shutdown();
}