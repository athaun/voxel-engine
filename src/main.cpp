#include <bx/math.h>

#include <iostream>
#include <chrono>

#include "bx/bx.h"
#include "core/core.h"
#include "core/log.h"
#include "render/batch.h"
#include "render/geometry.h"
#include "core/keyboard.h"
#include "core/mouse.h"
#include "noise/OpenSimplexNoise.h"

using namespace std::chrono;
steady_clock::time_point lastTime = steady_clock::now();

// Create an instance of the noise generator
OpenSimplexNoise::Noise noiseGen;

// Camera position variables
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;
float cameraPosZ = -5.0f;
float normalSpeed = 1.0f;
float fastSpeed = 2.5f; // For when user holds Shift
float movementSpeed = normalSpeed; // Start with normal speed

float cameraYaw = 0.0f;   // Rotation around the Y axis (horizontal)
float cameraPitch = 0.0f; // Rotation around the X axis (vertical)
float mouseSensitivity = 0.004f; // Change this if you want to modify the sensitivity.

// Define noise parameters
int octaves = 7;   // Number of noise layers
double persistence = 0.5;  // How much each octave contributes to the overall noise
double lacunarity = 2.0;   // How much the frequency increases per octave

bool enterKeyPressedLastFrame = false; // Track the previous state of the ENTER key
bool isFlying = true; // Start in flying mode by default
bool isOnGround = false;
bool canJump = false;
float verticalVelocity = 0.0f; // The player's current vertical velocity
const float gravity = -15.0f; // Gravity acceleration (you can tweak this value)
const float jumpStrength = 10.0f; // How high the player can jump

////////////////////////
// Define a struct for movement direction
struct Movement {
    float forward = 0.0f;
    float right = 0.0f;
    float up = 0.0f;
};

// Function to handle input and return a movement direction
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
////////////////////////

// Define movement vectors
bx::Vec3 forward(0.0f, 0.0f, 1.0f);
bx::Vec3 right(1.0f, 0.0f, 0.0f);

float getTerrainHeight(int x, int z, int octaves, double persistence, double lacunarity) {
    double noiseValue = 0.0;
    double amplitude = 1.0;
    double frequency = 0.5;
    double maxValue = 0.0;

    for (int i = 0; i < octaves; ++i) {
        noiseValue += noiseGen.eval(x * 0.01 * frequency, z * 0.01* frequency) * amplitude;
        maxValue += amplitude;

        amplitude *= persistence;
        frequency *= lacunarity;
    }

    noiseValue /= maxValue; // Normalize the noise value
    return noiseValue * 200.0f; // Scale the height appropriately
}

int main(int argc, char** argv) {

    Core::init();

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, Window::width, Window::height);


    int grid_size = 500;
    float spacing = 2.0f;

    int numCubes = grid_size * grid_size;
    int verticesPerCube = 8;  // Assuming each cube has 8 vertices
    int indicesPerCube = 36;  // Assuming each cube has 36 indices (12 triangles * 3 indices each)
    Render::Batch batch(numCubes * verticesPerCube, numCubes * indicesPerCube, "cubes");

    Render::Mesh cube = Render::cube(1.0f);
    batch.push_mesh(cube);

    for (int x = 0; x < grid_size; ++x) {
        for (int z = 0; z < grid_size; ++z) {
            Render::Mesh c(cube);
            float noiseValue = getTerrainHeight(x, z, octaves, persistence, lacunarity);
            float y = noiseValue;
            batch.push_mesh(Render::transform_mesh(c, x * spacing, y, z * spacing));
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
            // Calculate delta time
            steady_clock::time_point currentTime = steady_clock::now();
            duration<float> deltaTime = duration_cast<duration<float>>(currentTime - lastTime);
            lastTime = currentTime;

            bool topDownView = false;
            // Handle input for top-down view
            if (Keyboard::is_key_pressed(Keyboard::U)) {
                topDownView = !topDownView; // Toggle top-down view
            }

            if (topDownView) {
                // Set top-down view camera position and orientation
                cameraPosX = (grid_size * spacing) / 2.0f;
                cameraPosY = 500.0f; // Set height for the top-down view
                cameraPosZ = (grid_size * spacing) / 2.0f;
                cameraYaw = 0.0f;
                cameraPitch = -90.0f * (3.14159f / 180.0f); // Look straight down
            } else { 

                // Apply movement to the camera
                // Change the new forward position accordingly so forward is relative to where the camera is facing with the mouse
                Movement movement = get_movement_direction();

                        // Check for the ENTER key press event
                bool enterKeyPressedThisFrame = Keyboard::is_key_pressed(Keyboard::ENTER);
                if (enterKeyPressedThisFrame && !enterKeyPressedLastFrame) {
                    isFlying = !isFlying; // Toggle flying state
                }
                enterKeyPressedLastFrame = enterKeyPressedThisFrame; // Update the last state


                // Handle camera movement
                bx::Vec3 forward = {sinf(cameraYaw) * cosf(cameraPitch), sinf(cameraPitch),
                                    cosf(cameraYaw) * cosf(cameraPitch)};

                bx::Vec3 right = {cosf(cameraYaw), 0.0f,
                                -sinf(cameraYaw)};  // Right direction (perpendicular to forward)

                // Normalize directions manually
                // (Since bx doesn't have a vec3normalize.....)
                bx::Vec3 normForward = normalize(forward);
                bx::Vec3 normRight = normalize(right);

                // Few more extra keyboard inputs
                // Adjust speed for sprinting
                if (Keyboard::is_key_pressed(Keyboard::LEFT_SHIFT)) {
                    movementSpeed = fastSpeed;
                } else {
                    movementSpeed = normalSpeed;
                }

                // Handle escape
                if (Keyboard::is_key_pressed(Keyboard::ESCAPE)) {
                    break;
                }
                // Handle jumping input
                if (Keyboard::is_key_pressed(Keyboard::SPACE) && isOnGround && canJump) {
                    verticalVelocity = jumpStrength;  // Apply jump force
                    canJump = false;  // Prevent further jumping until the player lands
                }

                // Update your existing movement logic
                if (isFlying) {
                    // Flying mode: Use existing movement logic
                    cameraPosX += (normForward.x * movement.forward + normRight.x * movement.right) * movementSpeed;
                    cameraPosY += (normForward.y * movement.forward + movement.up) * movementSpeed;
                    cameraPosZ += (normForward.z * movement.forward + normRight.z * movement.right) * movementSpeed;
                } else {
                    movementSpeed = 0.05f;
                    // Walking mode: Apply horizontal movement
                    cameraPosX += (normForward.x * movement.forward + normRight.x * movement.right) * movementSpeed;
                    cameraPosZ += (normForward.z * movement.forward + normRight.z * movement.right) * movementSpeed;

                    // Apply gravity using deltaTime
                    verticalVelocity += gravity * deltaTime.count(); // Apply gravity based on time
                    cameraPosY += verticalVelocity * deltaTime.count(); // Update vertical position

                    // Get the terrain height directly below the player
                    int gridX = static_cast<int>(cameraPosX / spacing);
                    int gridZ = static_cast<int>(cameraPosZ / spacing);
                    float terrainHeight = getTerrainHeight(gridX, gridZ, octaves, persistence, lacunarity);

                    // Check if player is below terrain height
                    if (cameraPosY < terrainHeight + 5.0f) {
                        cameraPosY = terrainHeight + 5.0f; // Snap the player's position to the terrain height
                        verticalVelocity = 0.0f; // Reset vertical velocity to stop falling
                        isOnGround = true;
                        canJump = true;  // Allow jumping again when the player is on the ground
                    }
                }

                // Handle mouse position
                float mouseX, mouseY;
                Mouse::get_mouse_position(mouseX, mouseY);

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
                bx::mtxProj(proj, 80.0f, float(Window::width) / float(Window::height), 0.1f, 1000.0f, bgfx::getCaps()->homogeneousDepth);
                bgfx::setViewTransform(0, view, proj);
            }
        }

        batch.submit();

        Window::end_update();
    }
    Core::shutdown();
    return 0;
}