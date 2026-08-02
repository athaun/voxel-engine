#pragma once

namespace Environment {
    void init();
    // dt is seconds since the last frame (Window::delta_time).
    void update(float dt);
    void shutdown();
}
