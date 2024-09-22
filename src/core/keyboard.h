#pragma once
// Define key constants
namespace Keyboard {
    const int W = GLFW_KEY_W;
    const int A = GLFW_KEY_A;
    const int S = GLFW_KEY_S;
    const int D = GLFW_KEY_D;
    const int ESCAPE = GLFW_KEY_ESCAPE;
    const int SPACE = GLFW_KEY_SPACE;
    const int CTRL = GLFW_KEY_LEFT_CONTROL;
    const int LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT;
    const int U = GLFW_KEY_U;
    const int ENTER = GLFW_KEY_ENTER;
    bool is_key_pressed(int key);
}