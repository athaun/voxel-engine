vec3 a_position  : POSITION;
vec3 a_color0    : COLOR0;    // RGB color
vec3 a_normal    : NORMAL;
float a_color1   : COLOR1;    // AO
float a_texcoord0 : TEXCOORD0; // Material id

vec3 v_color0    : COLOR0;    // RGB color
vec3 v_normal    : NORMAL;
float v_ao       : COLOR1;    // AO
float v_texcoord0 : TEXCOORD0 = 0.0; // Material id
vec3 v_worldPos  : TEXCOORD1 = vec3(0.0, 0.0, 0.0); // World-space position (vertex positions are already world-space)