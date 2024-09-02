#pragma once

namespace Window {
    void init();
    void shutdown();
    void begin_update();
    void end_update();
    bool should_close();
}