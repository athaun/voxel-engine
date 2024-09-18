$input a_position, a_color0
$output v_color0

#include <bgfx_shader.sh>

void main() {
    // mat4 projViewWorld = mul(mul(u_proj, u_view), u_model[0]);
    // gl_Position = mul(projViewWorld, vec4(a_position, 1.0));
    gl_Position = vec4(a_position, 1.0);
    v_color0 = vec4(1.0, 0.0, 0.0, 1.0);
}