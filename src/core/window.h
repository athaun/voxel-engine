#pragma once
#include <GLFW/glfw3.h>

namespace Window {
    extern int width, height;
    extern float delta_time, fps;

    void init();
    void shutdown();
    void begin_update();
    void end_update();
    bool should_close();
    void exit();
    // Adding a getter so that Mouse and Keyboard namespaces could use the Window namespace
    GLFWwindow* get_window();
}