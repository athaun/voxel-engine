#pragma once

#include "window.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

namespace Core {
    void init(void (*user_init)());
    void run(void (*user_update)());
    void shutdown(void (*user_shutdown)());
}