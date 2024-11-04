#include "window.h"
#include "keyboard.h"

namespace Keyboard {

    bool is_key_pressed(int key) {
        // Use the Window::get_window() getter to access the window
        return glfwGetKey(Window::get_window(), key) == GLFW_PRESS;
    }
}