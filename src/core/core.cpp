#include "core.h"

namespace Core {
    void init(void (*user_init)()) {
        Window::init();
        user_init();
    }

    void shutdown(void (*user_shutdown)()) {
        user_shutdown();
        bgfx::shutdown();
        Window::shutdown();
    }

    void run(void (*user_update)()) {
        while (!Window::should_close()) {
            Window::begin_update();
            user_update();
            Window::end_update();
        }
    }
}