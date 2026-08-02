#include "demo.h"

#include "../core/core.h"
#include <bx/math.h>
#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include "terrain/chunk.h"
#include "terrain/chunk_manager.h"
#include "../core/keyboard.h"
#include "../render/environment.h"
#include "player_controller.h"
#include <iostream>

namespace Demo {
    void init() {
        bgfx::setViewRect(0, 0, 0, Window::width, Window::height);

        Environment::init();

        Player::init();
        ChunkManager::init();
    }

    void debug_text() {
        bgfx::setDebug(BGFX_DEBUG_TEXT);
        bgfx::dbgTextClear(BGFX_DEBUG_TEXT);

        bgfx::dbgTextPrintf(0, 0, 0x0f, "FPS: %.2f", Window::fps);
    }

    void chunks() {
        ChunkManager::update();
        ChunkManager::render();
    }

    void input() {
        if (Keyboard::is_key_pressed(Keyboard::ESCAPE)) {
            Window::exit();
        }

        if (Keyboard::is_key_pressed(Keyboard::U)) {
            ChunkManager::destroy_all();
        }
    }

    void update() {
        debug_text();
        Environment::update(Window::delta_time);
        chunks();
        input();
        Player::move();
    }

    void shutdown() {
        ChunkManager::cleanup();
        Environment::shutdown();
    }
}