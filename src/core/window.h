#pragma once
#include <GLFW/glfw3.h>
namespace Window {
    extern int width;
    extern int height;
    void init();
    void shutdown();
    void begin_update();
    void end_update();
    bool should_close();
    bool is_key_pressed(int key);
    void get_mouse_position(float& x, float& y);
    // Define key constants
    namespace Key {
        const int W = GLFW_KEY_W;
        const int A = GLFW_KEY_A;
        const int S = GLFW_KEY_S;
        const int D = GLFW_KEY_D;
        const int ESCAPE = GLFW_KEY_ESCAPE;
        const int SPACE = GLFW_KEY_SPACE;
        const int CTRL = GLFW_KEY_LEFT_CONTROL;
    }
}