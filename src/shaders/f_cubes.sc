$input v_texcoord0, v_normal

#include <bgfx_shader.sh>
#include "shaderlib.sh"

SAMPLER2D(s_texColor,  0);  // Texture sampler
uniform vec3 u_ambientColor; // Ambient light color
uniform vec3 u_lightDirection; // Direction of the light (sun)

void main() {
    // Sample the texture using texture coordinates
    vec4 texColor = texture2D(s_texColor, v_texcoord0);

    // Calculate ambient lighting
    vec3 ambient = u_ambientColor * texColor.rgb;

    // Calculate diffuse lighting
    // Normalize the light direction and calculate dot product
    float diff = max(dot(v_normal, normalize(u_lightDirection)), 0);
    vec3 diffuse = diff * texColor.rgb; // Scale by texture color

    // Combine ambient and diffuse lighting
    vec3 finalColor = ambient + diffuse;

    // Output the final color with ambient and diffuse lighting
    gl_FragColor = vec4(finalColor, texColor.a);
}
