#include "environment.h"

#include <algorithm>
#include <cmath>

#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "../core/core.h"
#include "../terrain/chunk.h"
#include "../terrain/chunk_manager.h"

// Single owner of every per-frame lighting/fog/time uniform. Previously this
// state was split between demo.cpp (light direction) and duplicated inside
// every chunk's Batch (ambient color, also recreated per-chunk) -- pulling
// it into one place means the uniforms are created once and set once per
// frame instead of once per chunk per frame.
namespace Environment {
    namespace {
        // Orbit speed is radians/second now that Window::delta_time is
        // actually wired up; previously this was a fixed per-frame
        // increment, i.e. implicitly tied to framerate.
        float light_angle = 0.0f;
        constexpr float LIGHT_ORBIT_SPEED = 0.03f;  // ~3.5 minute day/night cycle
        constexpr float LIGHT_ORBIT_RADIUS = 100.0f;
        // Fixed tilt on the orbit's Z axis so the light direction is never
        // exactly axis-aligned with the cube faces (avoids a degenerate dot
        // product at noon/midnight when it would otherwise point straight
        // down the Y axis).
        constexpr float LIGHT_ORBIT_TILT = 0.35f;

        float time_seconds = 0.0f;

        bgfx::UniformHandle u_lightDirection;
        bgfx::UniformHandle u_lightColor;
        bgfx::UniformHandle u_ambientSky;
        bgfx::UniformHandle u_ambientGround;
        bgfx::UniformHandle u_fogColor;
        bgfx::UniformHandle u_fogParams;
        bgfx::UniformHandle u_cameraPos;
        bgfx::UniformHandle u_time;

        struct Color3 { float r, g, b; };

        Color3 lerp(const Color3& a, const Color3& b, float t) {
            t = std::clamp(t, 0.0f, 1.0f);
            return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
        }
    }

    void init() {
        u_lightDirection = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4);
        u_lightColor     = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4);
        u_ambientSky     = bgfx::createUniform("u_ambientSky", bgfx::UniformType::Vec4);
        u_ambientGround  = bgfx::createUniform("u_ambientGround", bgfx::UniformType::Vec4);
        u_fogColor       = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
        u_fogParams      = bgfx::createUniform("u_fogParams", bgfx::UniformType::Vec4);
        u_cameraPos      = bgfx::createUniform("u_cameraPos", bgfx::UniformType::Vec4);
        u_time           = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
    }

    void update(float dt) {
        light_angle += LIGHT_ORBIT_SPEED * dt;
        time_seconds += dt;

        const float lightX = LIGHT_ORBIT_RADIUS * cosf(light_angle);
        const float lightY = LIGHT_ORBIT_RADIUS * sinf(light_angle);  // real elevation: sun rises/sets now
        const float lightZ = LIGHT_ORBIT_RADIUS * LIGHT_ORBIT_TILT;

        // Same sign convention the original demo.cpp lighting() used: the
        // direction fed to the shader points from the sun toward the scene.
        const float lightDir[4] = { -lightX, -lightY, -lightZ, 0.0f };

        // -1 at midnight (sun straight down) .. +1 at noon (sun straight up).
        const float elevation = sinf(light_angle);
        // 0 at night, 1 at full day.
        const float dayFactor = std::clamp(elevation * 0.5f + 0.5f, 0.0f, 1.0f);
        // Peaks right at the horizon (elevation == 0), falls off toward
        // noon/midnight -- used to add a warm sunrise/sunset tint. Raised to
        // the 4th power (instead of squared) so it's a brief transition
        // right at the horizon rather than a wide band that dominates a
        // fifth of the whole day/night cycle -- a wide band here reads as
        // the sky spending most of its time reddish/purple instead of blue.
        float horizon = std::clamp(1.0f - std::fabs(elevation), 0.0f, 1.0f);
        horizon *= horizon;
        horizon *= horizon;

        constexpr Color3 NIGHT_SUN   = { 0.18f, 0.20f, 0.32f };
        constexpr Color3 DAY_SUN     = { 1.00f, 0.97f, 0.90f };
        constexpr Color3 HORIZON_SUN = { 1.00f, 0.55f, 0.28f };

        // A saturated cornflower sky at noon, deepening to indigo at night --
        // avoids the pale washed-out blue a lower-saturation pick gamma-drifts
        // toward once it's round-tripped through the shader's linear pipeline.
        constexpr Color3 NIGHT_SKY    = { 0.035f, 0.045f, 0.13f };
        constexpr Color3 DAY_SKY      = { 0.30f, 0.60f, 0.95f };
        constexpr Color3 NIGHT_GROUND = { 0.03f, 0.03f, 0.05f };
        constexpr Color3 DAY_GROUND   = { 0.35f, 0.28f, 0.22f };

        Color3 sunColor = lerp(NIGHT_SUN, DAY_SUN, dayFactor);
        sunColor = lerp(sunColor, HORIZON_SUN, horizon * 0.5f);

        Color3 skyColor = lerp(NIGHT_SKY, DAY_SKY, dayFactor);
        skyColor = lerp(skyColor, HORIZON_SUN, horizon * 0.3f);

        const Color3 groundColor = lerp(NIGHT_GROUND, DAY_GROUND, dayFactor);

        const float lightColorArr[4]    = { sunColor.r, sunColor.g, sunColor.b, 0.0f };
        const float ambientSkyArr[4]    = { skyColor.r, skyColor.g, skyColor.b, 0.0f };
        const float ambientGroundArr[4] = { groundColor.r, groundColor.g, groundColor.b, 0.0f };
        // Fog matches the sky color at the horizon so distant terrain fades
        // into the sky instead of a mismatched haze.
        const float fogColorArr[4]      = { skyColor.r, skyColor.g, skyColor.b, 1.0f };

        const float render_distance = static_cast<float>(ChunkManager::CHUNK_RADIUS * CHUNK_WIDTH);
        const float fogParamsArr[4] = { render_distance * 0.6f, render_distance * 0.95f, 0.0f, 0.0f };

        const bx::Vec3 camPos = Core::camera.get_position();
        const float cameraPosArr[4] = { camPos.x, camPos.y, camPos.z, 0.0f };

        const float timeArr[4] = { time_seconds, 0.0f, 0.0f, 0.0f };

        bgfx::setUniform(u_lightDirection, lightDir);
        bgfx::setUniform(u_lightColor, lightColorArr);
        bgfx::setUniform(u_ambientSky, ambientSkyArr);
        bgfx::setUniform(u_ambientGround, ambientGroundArr);
        bgfx::setUniform(u_fogColor, fogColorArr);
        bgfx::setUniform(u_fogParams, fogParamsArr);
        bgfx::setUniform(u_cameraPos, cameraPosArr);
        bgfx::setUniform(u_time, timeArr);

        const uint32_t clearColor =
            (static_cast<uint32_t>(std::clamp(skyColor.r, 0.0f, 1.0f) * 255.0f) << 24) |
            (static_cast<uint32_t>(std::clamp(skyColor.g, 0.0f, 1.0f) * 255.0f) << 16) |
            (static_cast<uint32_t>(std::clamp(skyColor.b, 0.0f, 1.0f) * 255.0f) << 8) |
            0xFF;
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
    }

    void shutdown() {
        bgfx::destroy(u_lightDirection);
        bgfx::destroy(u_lightColor);
        bgfx::destroy(u_ambientSky);
        bgfx::destroy(u_ambientGround);
        bgfx::destroy(u_fogColor);
        bgfx::destroy(u_fogParams);
        bgfx::destroy(u_cameraPos);
        bgfx::destroy(u_time);
    }
}
