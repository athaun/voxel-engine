$input a_position, a_texcoord0, a_normal, a_color1
$output v_texcoord0, v_ambient_occlusion
$output v_normal

#include <bgfx_shader.sh>
#include "shaderlib.sh"

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_ambient_occlusion = a_color1;
    v_texcoord0 = a_texcoord0;
    v_normal = (a_normal); // Pass the normal to the fragment shader
}
