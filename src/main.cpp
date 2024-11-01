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
#include <bgfx/bgfx.h>
#include <bx/timer.h>
#include "terrain/chunk.h"
#include "terrain/chunk_manager.h"


// For Calculating DeltaTime
using namespace std::chrono;
steady_clock::time_point lastTime = steady_clock::now();

bgfx::UniformHandle u_lightDirection;


// Camera position variables
float cameraPosX = 0.0f;
float cameraPosY = 0.0f;
float cameraPosZ = -5.0f;
float normalSpeed = 0.2f;
float fastSpeed = 2.5f; // For when user holds Shift
float movementSpeed = normalSpeed; // Start with normal speed

float cameraYaw = 0.0f;   // Rotation around the Y axis (horizontal)
float cameraPitch = 0.0f; // Rotation around the X axis (vertical)
float mouseSensitivity = 0.004f; // Change this if you want to modify the sensitivity.

bool enterKeyPressedLastFrame = false; // Track the previous state of the ENTER key
bool isFlying = true; // Start in flying mode by default
bool isOnGround = false;
bool canJump = false;
float verticalVelocity = 0.0f; // The player's current vertical velocity
const float gravity = -15.0f; // Gravity acceleration (you can tweak this value)
const float jumpStrength = 10.0f; // How high the player can jump

// Light direction variables
float angle = 0.0f; // Initialize angle for orbiting
float orbitSpeed = 0.005f; // Adjust this to control the speed of the orbit
float orbitRadius = 100.0f; // Radius of the orbit above and below the voxel plane

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

int spacing = 1.0f;
int grid_size = 100;


int main(int argc, char** argv) {

    Core::init();

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x443355FF, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, Window::width, Window::height);

    // Initialize the chunk manager
    ChunkManager::init();
    bx::Vec3 lightDirection(1.0f, 0.0f, 0.0f);
    u_lightDirection = bgfx::createUniform("u_lightDirection", bgfx::UniformType::Vec4);


    // In Window.cpp, mouse position is initialized and defined to be at the center of the screen.
    // Thus, the last known beginning mouse position, or the first mouse position, will be in the
    // same position.
    int lastMouseX = Window::width / 2;
    int lastMouseY = Window::height / 2;

    // Determine how high we want the max to be to move our camera vertically
    // Recommended to be less than 90 or the camera will invert/flip
    const float maxPitch = 89.0f * (3.14159f / 180.0f);  // Convert degrees to radians

    bool topDownView = true;
    while (!Window::should_close()) {
        Window::begin_update();

        {           
            bgfx::setDebug(BGFX_DEBUG_TEXT);
            bgfx::dbgTextClear(BGFX_DEBUG_TEXT);

            // Display the FPS
            bgfx::dbgTextPrintf(0, 0, 0x0f, "FPS: %.2f", NULL);
            bgfx::dbgTextPrintf(0, 2, 0x0f, "Player Position: (%.2f, %.2f, %.2f)", cameraPosX, cameraPosY, cameraPosZ);
            bgfx::dbgTextPrintf(0, 4, 0x0f, "Player Rotation: (%.2f, %.2f)", cameraYaw, cameraPitch);
            

            // Calculate delta time
            steady_clock::time_point currentTime = steady_clock::now();
            duration<float> deltaTime = duration_cast<duration<float>>(currentTime - lastTime);
            lastTime = currentTime;

            // Handle input for top-down view
            if (Keyboard::is_key_pressed(Keyboard::U)) {
                topDownView = !topDownView; // Toggle top-down view
            }

            if (topDownView) {
                // Set top-down view camera position and orientation
                cameraPosX = (grid_size * spacing) / 2.0f;
                cameraPosY = 100.0f; // Set height for the top-down view
                cameraPosZ = (grid_size * spacing) / 2.0f;
                cameraYaw = 0.0f;
                cameraPitch = -90.0f * (3.14159f / 180.0f); // Look straight down
                topDownView = false;
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
                    float terrainHeight = 100; //getTerrainHeight(gridX, gridZ, octaves, persistence, lacunarity);

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
                bx::mtxProj(proj, 80.0f, float(Window::width) / float(Window::height), 0.1f, 2000.0f, bgfx::getCaps()->homogeneousDepth);
                bgfx::setViewTransform(0, view, proj);

                // Update light direction to create an orbiting effect
                angle += orbitSpeed; // Increment angle
                float lightX = 0.0f; // Calculate X position
                float lightY = orbitRadius * cos(angle); // Calculate Y position
                float lightZ = orbitRadius * sin(angle); // Calculate Z position
            

                bx::Vec3 lightDirection(-lightX, -lightY, -lightZ); // Create the new light direction vector
                bgfx::setUniform(u_lightDirection, &lightDirection);  // Set the uniform value for lightDirection vector
            }
        }

        // Check if the G key is pressed
        if (Keyboard::is_key_pressed(GLFW_KEY_E)) {
            // Get the player's current position
            int chunkX = static_cast<int>(cameraPosX) / CHUNK_WIDTH;
            int chunkZ = static_cast<int>(cameraPosZ) / CHUNK_DEPTH;

            ChunkManager::build_chunk(chunkX, 0, chunkZ);
        }

        //ChunkManager::chunk_circle(cameraPosX / CHUNK_WIDTH, cameraPosZ / CHUNK_DEPTH, 2);
        ChunkManager::update();
        ChunkManager::render();

        Window::end_update();
    }
    Core::shutdown();
    return 0;
}