#include "window.h"
#include "mouse.h"

namespace Mouse {

    void get_mouse_position(float& x, float& y) {
        double mouseX, mouseY;
        glfwGetCursorPos(Window::get_window(), &mouseX, &mouseY);  // Use the getter function

        x = static_cast<float>(mouseX);
        y = static_cast<float>(mouseY);
    }
}