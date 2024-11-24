$input v_color0, v_normal, v_ao

#include <bgfx_shader.sh>

uniform vec3 u_ambientColor;
uniform vec3 u_lightDirection;

void main() {
    // Calculate ambient lighting
    vec3 ambient = u_ambientColor * v_color0 * v_ao;

    // Calculate diffuse lighting
    float diff = max(dot(v_normal, normalize(u_lightDirection)), 0.0);
    vec3 diffuse = diff * v_color0;

    // Combine ambient and diffuse lighting
    vec3 finalColor = ambient + diffuse;
    
    gl_FragColor = vec4(finalColor, 1.0);
}