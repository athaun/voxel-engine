#pragma once

namespace Window {
    extern int width;
    extern int height;
    void init();
    void shutdown();
    void begin_update();
    void end_update();
    bool should_close();
}