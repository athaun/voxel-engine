vec4 v_color0 : COLOR0 = vec4(1.0, 0.0, 0.0, 1.0);
vec3 a_position : POSITION;
vec4 a_color0 : COLOR0;
vec2 v_texcoord0 : TEXCOORD0; // Texture coordinates
vec2 a_texcoord0 : TEXCOORD0;
vec3 a_normal : NORMAL; // Add this line for the input normal from vertex data
vec3 v_normal : NORMAL; // Add this line for the output normal to the fragment shader

