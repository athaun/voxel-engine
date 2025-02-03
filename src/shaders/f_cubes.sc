$input v_color0, v_normal, v_ao

#include <bgfx_shader.sh>

uniform vec3 u_ambientColor;

void main() {
    // Calculate ambient lighting
    vec3 ambient = u_ambientColor * v_color0 * v_ao;

    vec3 finalColor = ambient;
    
    gl_FragColor = vec4(finalColor, 1.0);
}