#include "core.h"

namespace Core {

    void init() {
        Window::init();
    }

    void shutdown() {
        bgfx::shutdown();
        Window::shutdown();
    }

    void run(void (*user_function)()) {
        while (!Window::should_close()) {
            Window::begin_update();

            user_function();
            
            Window::end_update();
        }
    }
}