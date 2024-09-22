#include <bx/math.h>

#include <iostream>

#include "bx/bx.h"
#include "core/core.h"
#include "core/log.h"
#include "render/batch.h"
#include "render/geometry.h"

// Camera position variables
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;
float cameraPosZ = -5.0f;
float movementSpeed = 0.1f;

float cameraYaw = 0.0f;           // Rotation around the Y axis (horizontal)
float cameraPitch = 0.0f;         // Rotation around the X axis (vertical)
float mouseSensitivity = 0.004f;  // Change this if you want to modify the sensitivity.

// Define movement vectors
bx::Vec3 forward(0.0f, 0.0f, 1.0f);
bx::Vec3 right(1.0f, 0.0f, 0.0f);

int main(int argc, char** argv) {

    Core::init();

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, Window::width, Window::height);


    int grid_size = 500;

    int numCubes = grid_size * grid_size;
    int verticesPerCube = 8;  // Assuming each cube has 8 vertices
    int indicesPerCube = 36;  // Assuming each cube has 36 indices (12 triangles * 3 indices each)
    Render::Batch batch(numCubes * verticesPerCube, numCubes * indicesPerCube, "cubes");

    Render::Mesh cube = Render::cube(1.0f);
    batch.push_mesh(cube);

    for (int x = 0; x < grid_size; ++x) {
        for (int z = 0; z < grid_size; ++z) {
            Render::Mesh c(cube);
            batch.push_mesh(Render::transform_mesh(c, x * 3.0f, 0.0f, z * 3.0f));
        }
    }

    // Render::Mesh triangle;
    // triangle.vertices = {
    //     { 0.0f,  2.0f, 0.0f, 0xff00ff00},
    //     {-2.0f, -2.0f, 0.0f, 0xff0000ff},
    //     { 2.0f, -2.0f, 0.0f, 0xffff0000},
    // };
    // triangle.vertexIndices = {
    //     0, 1, 2,
    // };
    // batch.push_mesh(triangle);

    // In Window.cpp, mouse position is initialized and defined to be at the center of the screen.
    // Thus, the last known beginning mouse position, or the first mouse position, will be in the
    // same position.
    int lastMouseX = Window::width / 2;
    int lastMouseY = Window::height / 2;

    // Determine how high we want the max to be to move our camera vertically
    // Recommended to be less than 90 or the camera will invert/flip
    const float maxPitch = 89.0f * (3.14159f / 180.0f);  // Convert degrees to radians

    while (!Window::should_close()) {
        Window::begin_update();

        {
            // Handle camera movement
            bx::Vec3 forward = {sinf(cameraYaw) * cosf(cameraPitch), sinf(cameraPitch),
                                cosf(cameraYaw) * cosf(cameraPitch)};

            bx::Vec3 right = {cosf(cameraYaw), 0.0f,
                            -sinf(cameraYaw)};  // Right direction (perpendicular to forward)

            // Normalize directions manually
            // (Since bx doesn't have a vec3normalize.....)
            bx::Vec3 normForward = normalize(forward);
            bx::Vec3 normRight = normalize(right);

            // Movement speed applied to direction vectors
            // WASD stuff, with extras :D
            // Change the new forward position accordingly so forward is relative to where the
            // camera is facing with the mouse
            if (Window::is_key_pressed(Window::Key::W)) {
                cameraPosX += normForward.x * movementSpeed;
                cameraPosY += normForward.y * movementSpeed;
                cameraPosZ += normForward.z * movementSpeed;
            }
            if (Window::is_key_pressed(Window::Key::S)) {
                cameraPosX -= normForward.x * movementSpeed;
                cameraPosY -= normForward.y * movementSpeed;
                cameraPosZ -= normForward.z * movementSpeed;
            }
            if (Window::is_key_pressed(Window::Key::A)) {
                cameraPosX -= normRight.x * movementSpeed;
                cameraPosZ -= normRight.z * movementSpeed;
            }
            if (Window::is_key_pressed(Window::Key::D)) {
                cameraPosX += normRight.x * movementSpeed;
                cameraPosZ += normRight.z * movementSpeed;
            }
            // Go Up
            if (Window::is_key_pressed(Window::Key::SPACE)) {
                cameraPosY += movementSpeed;
            }
            // Go Down
            if (Window::is_key_pressed(Window::Key::CTRL)) {
                cameraPosY -= movementSpeed;
            }
            // Escape
            if (Window::is_key_pressed(Window::Key::ESCAPE)) {
                break;
            }

            // Handle mouse position
            float mouseX, mouseY;
            Window::get_mouse_position(mouseX, mouseY);

            // Calculate the delta from the last known mouse position
            int deltaX = static_cast<int>(mouseX - lastMouseX);
            int deltaY = static_cast<int>(mouseY - lastMouseY);

            // Update camera rotation based on mouse movement
            cameraYaw += deltaX * mouseSensitivity;
            cameraPitch -= deltaY * mouseSensitivity;

            // Clamp the pitch to avoid flipping
            // This will stop the camera moving vertically at 89 degrees.
            if (cameraPitch > maxPitch) {
                cameraPitch = maxPitch;
            } else if (cameraPitch < -maxPitch) {
                cameraPitch = -maxPitch;
            }

            // Redefine last mouse position to be current before frame ends
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        
            // Define camera position and target vectors
            bx::Vec3 eye = {cameraPosX, cameraPosY, cameraPosZ};
            bx::Vec3 at = {cameraPosX + sinf(cameraYaw) * cosf(cameraPitch),
                        cameraPosY + sinf(cameraPitch),
                        cameraPosZ + cosf(cameraYaw) * cosf(cameraPitch)};

            // Up vector (typically, this is (0, 1, 0) for a standard upright camera)
            bx::Vec3 up = {0.0f, 1.0f, 0.0f};

            float view[16];
            bx::mtxLookAt(view, eye, at, up);

            // Set up projection matrix
            float proj[16];
            bx::mtxProj(proj, 60.0f, float(Window::width) / float(Window::height), 0.1f, 1000.0f, bgfx::getCaps()->homogeneousDepth);
            bgfx::setViewTransform(0, view, proj);
        }

        batch.submit();

        Window::end_update();
    }
    Core::shutdown();
    return 0;
}