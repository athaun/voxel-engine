#pragma once

#include "window.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "render/camera.h"

namespace Core {
    extern Camera camera;
    
    void init(void (*user_init)());
    void run(void (*user_update)());
    void shutdown(void (*user_shutdown)());
}