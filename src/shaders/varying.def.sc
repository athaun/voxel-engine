vec3 a_position : POSITION;
vec4 a_color0 : COLOR0;
vec2 a_texcoord0 : TEXCOORD0;
vec3 a_normal : NORMAL;
float a_color1 : COLOR0; // AMBIENT OCCLUSION

vec4 v_color0 : COLOR0 = vec4(1.0, 0.0, 0.0, 1.0);
vec2 v_texcoord0 : TEXCOORD0;
vec3 v_normal : NORMAL;
float v_ambient_occlusion : COLOR0 = vec4(1.0, 0.0, 0.0, 1.0);