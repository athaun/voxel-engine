$input v_texcoord0

#include <bgfx_shader.sh>
#include "shaderlib.sh"

SAMPLER2D(s_texColor,  0);  // Declare the texture sampler

void main() {
    // Sample the texture using texture coordinates
    vec4 texColor = texture2D(s_texColor, v_texcoord0);
    gl_FragColor = texColor;  // Output the sampled texture color
}