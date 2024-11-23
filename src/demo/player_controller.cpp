#pragma once
#include "player_controller.h"
#include "../render/camera.h"
#include "../core/keyboard.h"
#include <math.h>
namespace Player {
    const float gravity = -15.0f;
    const float jump_velocity = 10.0f;

    const float default_speed = 0.4f;
    const float fast_speed = 1.2f;

    const float mouse_sensitivity = 0.004f;
    const float max_camera_pitch = 89.0f * (3.14159f / 180.0f);

    const int spacing = 1.0f;
    const int grid_size = 100;
    
    bx::Vec3 velocity(0.0f, 0.0f, 0.0f); 
    float move_speed = default_speed;
    
    float mouse_x, mouse_y;
    int prev_mouse_x, prev_mouse_y;
    
    bool is_flying = true;
    bool is_grounded = false;
    bool can_jump = false;
    bool top_down_view = true;

    Movement get_movement_direction() {
        Movement movement;
        if (Keyboard::is_key_pressed(Keyboard::W)) movement.forward += 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::S)) movement.forward -= 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::A)) movement.right += 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::D)) movement.right -= 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::SPACE)) movement.up += 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::CTRL)) movement.up -= 1.0f;
        return movement;
    }

    void init() {
        prev_mouse_x = Window::width / 2;
        prev_mouse_y = Window::height / 2;
        Core::camera.set_position(0.0f, 0.0f, -5.0f);
        Core::camera.set_rotation(0.0f, 0.0f); // Ensure initial rotation is set
    }

    void move() {
        Movement movement = get_movement_direction();

        // Adjust movement speed based on shift key
        move_speed = Keyboard::is_key_pressed(Keyboard::LEFT_SHIFT) ? fast_speed : default_speed;

        // Get current camera position and directions
        bx::Vec3 pos = Core::camera.get_position();
        bx::Vec3 fwd = Core::camera.get_forward();
        bx::Vec3 right = Core::camera.get_right();

        if (is_flying) {
            Core::camera.move(
                (fwd.x * movement.forward + right.x * movement.right) * move_speed,
                (fwd.y * movement.forward + movement.up) * move_speed,
                (fwd.z * movement.forward + right.z * movement.right) * move_speed
            );
        } else {

        }

        // Handle mouse look
        Mouse::get_mouse_position(mouse_x, mouse_y);
        int delta_x = static_cast<int>(mouse_x - prev_mouse_x);
        int delta_y = static_cast<int>(mouse_y - prev_mouse_y);

        // Update yaw and pitch based on mouse movement
        float new_yaw = Core::camera.get_yaw() + delta_x * mouse_sensitivity;
        float new_pitch = Core::camera.get_pitch() - delta_y * mouse_sensitivity;
        new_pitch = bx::clamp(new_pitch, -max_camera_pitch, max_camera_pitch);

        Core::camera.set_rotation(new_yaw, new_pitch);

        prev_mouse_x = mouse_x;
        prev_mouse_y = mouse_y;

        // Update and apply the camera matrices
        Core::camera.update_matrices(
            float(Window::width),
            float(Window::height),
            bgfx::getCaps()->homogeneousDepth
        );
        Core::camera.set_view_transform(0);  // Ensure the correct view transformation is applied
    }
}