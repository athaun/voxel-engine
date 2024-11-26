#pragma once

#include <math.h>
#include "bx/math.h"
#include <bgfx/bgfx.h>
#include <array>
#include "camera.h"

Camera::Camera()
    : position(0.0f, 0.0f, 0.0f),
      target(0.0f, 0.0f, 0.0f),
      up(0.0f, 1.0f, 0.0f),
      fov(80.0f), near(0.1f), far(2000.0f),
      forward(0.0f, 0.0f, 0.0f), right(0.0f, 0.0f, 0.0f),
      yaw(0.0f), pitch(0.0f)
{}

void Camera::set_position(float x, float y, float z) {
    position = {x, y, z};
}

void Camera::set_target(float x, float y, float z) {
    target = {x, y, z};
}

void Camera::set_up(float x, float y, float z) {
    up = {x, y, z};
}

void Camera::move(float dx, float dy, float dz) {
    position.x += dx;
    position.y += dy;
    position.z += dz;

    target.x += dx;
    target.y += dy;
    target.z += dz;
}

void Camera::set_rotation(float yaw_value, float pitch_value) {
    yaw = yaw_value;
    pitch = pitch_value;

    // Update target based on rotation
    target.x = position.x + sinf(yaw) * cosf(pitch);
    target.y = position.y + sinf(pitch);
    target.z = position.z + cosf(yaw) * cosf(pitch);
}

void Camera::update_matrices(float width, float height, bool homogeneous_depth) {
    // Update view matrix
    bx::mtxLookAt(view, position, target, up);

    // Update projection matrix
    bx::mtxProj(proj, fov, width / height, near, far, homogeneous_depth);

    // Update combined view-projection matrix
    bx::mtxMul(view_proj, view, proj);

    // Update cached direction vectors
    forward = bx::sub(target, position);
    forward = bx::normalize(forward);

    right = bx::cross(forward, up);
    right = bx::normalize(right);
}

void Camera::set_view_transform(uint8_t id) {
    bgfx::setViewTransform(id, view, proj);
}

bx::Vec3 Camera::get_position() {
    return position;
}

bx::Vec3 Camera::get_forward() {
    return forward;
}

bx::Vec3 Camera::get_right() {
    return right;
}

float Camera::get_yaw() {
    return yaw;
}

float Camera::get_pitch() {
    return pitch;
}

float* Camera::get_view(){
    return view;
}

float* Camera::get_projection(){
    return proj;
}
