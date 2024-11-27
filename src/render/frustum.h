#pragma once

#include "bx/math.h"

// Frustum planes in ax + by + cz + d = 0 form
struct Frustum {
    struct Plane {
        float a, b, c, d;
        
        void normalize() {
            float mag = std::sqrt(a * a + b * b + c * c);
            a /= mag;
            b /= mag;
            c /= mag;
            d /= mag;
        }
    };
    
    Plane planes[6]; // Left, Right, Top, Bottom, Near, Far
    
    void update(const float* view, const float* proj) {
        float viewProj[16];
        bx::mtxMul(viewProj, view, proj);
        
        // Left plane
        planes[0].a = viewProj[3] + viewProj[0];
        planes[0].b = viewProj[7] + viewProj[4];
        planes[0].c = viewProj[11] + viewProj[8];
        planes[0].d = viewProj[15] + viewProj[12];
        
        // Right plane
        planes[1].a = viewProj[3] - viewProj[0];
        planes[1].b = viewProj[7] - viewProj[4];
        planes[1].c = viewProj[11] - viewProj[8];
        planes[1].d = viewProj[15] - viewProj[12];
        
        // Top plane
        planes[2].a = viewProj[3] - viewProj[1];
        planes[2].b = viewProj[7] - viewProj[5];
        planes[2].c = viewProj[11] - viewProj[9];
        planes[2].d = viewProj[15] - viewProj[13];
        
        // Bottom plane
        planes[3].a = viewProj[3] + viewProj[1];
        planes[3].b = viewProj[7] + viewProj[5];
        planes[3].c = viewProj[11] + viewProj[9];
        planes[3].d = viewProj[15] + viewProj[13];
        
        // Near plane
        planes[4].a = viewProj[3] + viewProj[2];
        planes[4].b = viewProj[7] + viewProj[6];
        planes[4].c = viewProj[11] + viewProj[10];
        planes[4].d = viewProj[15] + viewProj[14];
        
        // Far plane
        planes[5].a = viewProj[3] - viewProj[2];
        planes[5].b = viewProj[7] - viewProj[6];
        planes[5].c = viewProj[11] - viewProj[10];
        planes[5].d = viewProj[15] - viewProj[14];
        
        // Normalize all planes
        for (auto& plane : planes) {
            plane.normalize();
        }
    }
    
    bool is_box_visible(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) const {
        // Test AABB against all frustum planes
        for (const auto& plane : planes) {
            // Find the point of the box furthest in the direction of the plane normal
            float px = (plane.a > 0.0f) ? maxX : minX;
            float py = (plane.b > 0.0f) ? maxY : minY;
            float pz = (plane.c > 0.0f) ? maxZ : minZ;
            
            // If this point is behind the plane, the box is outside the frustum
            if (plane.a * px + plane.b * py + plane.c * pz + plane.d < 0.0f) {
                return false;
            }
        }
        return true;
    }
};