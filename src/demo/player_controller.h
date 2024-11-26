#pragma once

#include <bx/timer.h>
#include "bx/bx.h"
#include <bx/math.h>
#include <bgfx/bgfx.h>
#include "../core/keyboard.h"
#include "../core/mouse.h"
#include "../core/core.h"

namespace Player {
    struct Movement {
        float forward = 0.0f;
        float right = 0.0f;
        float up = 0.0f;
    };

    void init();
    void move();
}