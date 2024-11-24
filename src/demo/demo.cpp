#include "demo.h"

#include "../core/core.h"
#include <bx/math.h>
#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include "terrain/chunk.h"
#include "terrain/chunk_manager.h"
#include "../core/keyboard.h"
#include "player_controller.h"

namespace Demo {
    float light_angle = 0.0f;
    float light_orbit_speed = 0.005f;
    float light_orbit_radius = 100.0f;
    
    bx::Vec3 light_direction(0.0f, 0.0f, 0.0f);
    bgfx::UniformHandle u_light_direction;

    void init() {
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, Window::width, Window::height);

        u_light_direction = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4);
        
        Player::init();
        ChunkManager::init();
    }

    void lighting() {
        light_angle += light_orbit_speed;
        
        float lightX = 0.0f;
        float lightY = light_orbit_radius * cos(light_angle);
        float lightZ = light_orbit_radius * sin(light_angle);

        light_direction = {-lightX, -lightY, -lightZ};
        bgfx::setUniform(u_light_direction, &light_direction);
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
    }

    void update() {
        debug_text();
        //lighting();
        chunks();
        input();
        Player::move();
    }

    void shutdown() {
        ChunkManager::cleanup();
    }
}