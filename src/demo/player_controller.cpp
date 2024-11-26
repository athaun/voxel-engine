#include "player_controller.h"
#include <math.h>

namespace Player {
    const float gravity = -15.0f;
    const float jump_velocity = 10.0f;

    const float default_speed = 0.02f;
    const float fast_speed = 0.10f;

    const float mouse_sensitivity = 0.004f;
    const float max_camera_pitch = 89.0f * (3.14159f / 180.0f);

    const int spacing = 1.0f;
    const int grid_size = 100;
    
    bx::Vec3 forward(0.0f, 0.0f, 1.0f);
    bx::Vec3 right(1.0f, 0.0f, 0.0f);
    
    bx::Vec3 camera_pos(0.0f, 0.0f, -5.0f);
    bx::Vec3 velocity(0.0f, 0.0f, 0.0f); 

    float move_speed = default_speed;

    float camera_yaw = 0.0f;   // Rotation around the Y axis (horizontal)
    float camera_pitch = 0.0f; // Rotation around the X axis (vertical)
    
    float mouse_x, mouse_y;
    int prev_mouse_x, prev_mouse_y;
    
    bool enter_key_pressed = false;
    bool is_flying = true;
    bool is_grounded = false;
    bool can_jump = false;
    bool top_down_view = true;

    Movement get_movement_direction() {
        Movement movement;
        // WASD stuff, with some extras :D
        if (Keyboard::is_key_pressed(Keyboard::W)) movement.forward += 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::S)) movement.forward -= 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::A)) movement.right -= 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::D)) movement.right += 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::SPACE)) movement.up += 1.0f;
        if (Keyboard::is_key_pressed(Keyboard::CTRL)) movement.up -= 1.0f;

        return movement;
    }

    void init() {
        prev_mouse_x = Window::width / 2;
        prev_mouse_y = Window::height / 2;
    }

    void move() {
        // Handle input for top-down view
        if (Keyboard::is_key_pressed(Keyboard::U)) {
            top_down_view = !top_down_view; // Toggle top-down view
        }

        if (top_down_view) {
            // Set top-down view camera position and orientation
            camera_pos.x = (grid_size * spacing) / 2.0f;
            camera_pos.y = 100.0f; // Set height for the top-down view
            camera_pos.z = (grid_size * spacing) / 2.0f;
            camera_yaw = 0.0f;
            camera_pitch = -90.0f * (3.14159f / 180.0f); // Look straight down
            top_down_view = false;
        } else { 
            // Apply movement to the camera
            // Change the new forward position accordingly so forward is relative to where the camera is facing with the mouse
            Movement movement = get_movement_direction();

                    // Check for the ENTER key press event
            bool enterKeyPressedThisFrame = Keyboard::is_key_pressed(Keyboard::ENTER);
            if (enterKeyPressedThisFrame && !enter_key_pressed) {
                is_flying = !is_flying; // Toggle flying state
            }
            enter_key_pressed = enterKeyPressedThisFrame; // Update the last state


            // Handle camera movement
            forward = {sinf(camera_yaw) * cosf(camera_pitch), sinf(camera_pitch), cosf(camera_yaw) * cosf(camera_pitch)};
            right = {cosf(camera_yaw), 0.0f, -sinf(camera_yaw)};  // Right direction (perpendicular to forward)

            bx::Vec3 norm_forward = normalize(forward);
            bx::Vec3 norm_right = normalize(right);

            // Few more extra keyboard inputs
            // Adjust speed for sprinting
            if (Keyboard::is_key_pressed(Keyboard::LEFT_SHIFT)) {
                move_speed = fast_speed;
            } else {
                move_speed = default_speed;
            }

            // Handle jumping input
            if (Keyboard::is_key_pressed(Keyboard::SPACE) && is_grounded && can_jump) {
                velocity.y = jump_velocity;  // Apply jump force
                can_jump = false;  // Prevent further jumping until the player lands
            }

            // Update your existing movement logic
            if (is_flying) {
                // Flying mode: Use existing movement logic
                camera_pos.x += (norm_forward.x * movement.forward + norm_right.x * movement.right) * move_speed;
                camera_pos.y += (norm_forward.y * movement.forward + movement.up) * move_speed;
                camera_pos.z += (norm_forward.z * movement.forward + norm_right.z * movement.right) * move_speed;
            } else {
                move_speed = 0.05f;
                // Walking mode: Apply horizontal movement
                camera_pos.x += (norm_forward.x * movement.forward + norm_right.x * movement.right) * move_speed;
                camera_pos.z += (norm_forward.z * movement.forward + norm_right.z * movement.right) * move_speed;

                // Apply gravity
                velocity.y += gravity * Window::delta_time; // Apply gravity based on time
                camera_pos.y += velocity.y * Window::delta_time; // Update vertical position

                // Get the terrain height directly below the player
                // int gridX = static_cast<int>(camera_pos.x / spacing);
                // int gridZ = static_cast<int>(camera_pos.z / spacing);
                float terrain_height = 100; //getterrain_height(gridX, gridZ, octaves, persistence, lacunarity);

                // Check if player is below terrain height
                if (camera_pos.y < terrain_height + 5.0f) {
                    camera_pos.y = terrain_height + 5.0f; // Snap the player's position to the terrain height
                    velocity.y = 0.0f; // Reset vertical velocity to stop falling
                    is_grounded = true;
                    can_jump = true;  // Allow jumping again when the player is on the ground
                }
            }

    
            Mouse::get_mouse_position(mouse_x, mouse_y);

            // Calculate the delta from the last known mouse position
            int delta_x = static_cast<int>(mouse_x - prev_mouse_x);
            int delta_y = static_cast<int>(mouse_y - prev_mouse_y);

            // Update camera rotation based on mouse movement
            camera_yaw += delta_x * mouse_sensitivity;
            camera_pitch -= delta_y * mouse_sensitivity;

            // Clamp the pitch to avoid flipping
            // This will stop the camera moving vertically at 89 degrees.
            if (camera_pitch > max_camera_pitch) {
                camera_pitch = max_camera_pitch;
            } else if (camera_pitch < -max_camera_pitch) {
                camera_pitch = -max_camera_pitch;
            }

            // Redefine last mouse position to be current before frame ends
            prev_mouse_x = mouse_x;
            prev_mouse_y = mouse_y;
        
            // Define camera position and target vectors
            bx::Vec3 eye = {camera_pos.x, camera_pos.y, camera_pos.z};
            bx::Vec3 at = {camera_pos.x + sinf(camera_yaw) * cosf(camera_pitch),
                        camera_pos.y + sinf(camera_pitch),
                        camera_pos.z + cosf(camera_yaw) * cosf(camera_pitch)};

            // Up vector (typically, this is (0, 1, 0) for a standard upright camera)
            bx::Vec3 up = {0.0f, 1.0f, 0.0f};

            float view[16];
            bx::mtxLookAt(view, eye, at, up);

            // Set up projection matrix
            float proj[16];
            bx::mtxProj(proj, 80.0f, float(Window::width) / float(Window::height), 0.1f, 2000.0f, bgfx::getCaps()->homogeneousDepth);
            bgfx::setViewTransform(0, view, proj);
        }
    }
}