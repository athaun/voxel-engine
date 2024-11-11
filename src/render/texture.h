#pragma once

#include <bgfx/bgfx.h>
#include <bx/bx.h>
#include "log.h"

// Function to load a texture from a file
bgfx::TextureHandle load_texture(const char* filePath, uint32_t flags = 
    BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | 
    BGFX_SAMPLER_MIP_POINT);

// bgfx::TextureHandle solid_color_texture(uint32_t color, int width, int height);

// const bgfx::Memory* load_mem(const char* filePath);