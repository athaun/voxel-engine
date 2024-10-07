$input a_position, a_texcoord0, a_normal
$output v_texcoord0
$output v_normal

#include <bgfx_shader.sh>
#include "shaderlib.sh"

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_texcoord0 = a_texcoord0;
    v_normal = normalize(a_normal); // Pass the normal to the fragment shader
}
