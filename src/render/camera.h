#pragma once

#include "bx/math.h"
#include <bgfx/bgfx.h>

class Camera {
private:
    bx::Vec3 position;
    bx::Vec3 target;
    bx::Vec3 up;

    float fov;
    float near;
    float far;
    
    float view[16];
    float proj[16];
    float view_proj[16];

    bx::Vec3 forward;
    bx::Vec3 right;
    
    float yaw;
    float pitch;

public:
    Camera();

    void set_position(float x, float y, float z);

    void set_target(float x, float y, float z);

    void set_up(float x, float y, float z);

    void move(float dx, float dy, float dz);

    void set_rotation(float yaw_value, float pitch_value);

    void update_matrices(float width, float height, bool homogeneous_depth);

    void set_view_transform(uint8_t id);

    bx::Vec3 get_position();
    bx::Vec3 get_forward();
    bx::Vec3 get_right();

    float get_yaw();
    float get_pitch();

    float* get_view();
    float* get_projection();
};