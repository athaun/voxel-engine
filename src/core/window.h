#pragma once

namespace Window {
    static int width, height;
    void init();
    void shutdown();
    void begin_update();
    void end_update();
    bool should_close();
}