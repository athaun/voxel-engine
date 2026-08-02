$input v_color0, v_normal, v_ao, v_texcoord0, v_worldPos

#include <bgfx_shader.sh>
#include <shaderlib.sh>

uniform vec4 u_lightDirection;
uniform vec4 u_lightColor;
uniform vec4 u_ambientSky;
uniform vec4 u_ambientGround;
uniform vec4 u_fogColor;
uniform vec4 u_fogParams;  // x = fog start distance, y = fog end distance
uniform vec4 u_cameraPos;

// Material ids -- must stay in sync with voxel::Material (src/terrain/voxel.h)
#define MAT_STONE 1
#define MAT_DIRT  2
#define MAT_GRASS 3
#define MAT_SAND  5
#define MAT_SNOW  8

// Integer bit-mixing hash (PCG2D-style) -- avoids both the sin()-based hash
// in shaderlib.sh's random() (bands/artifacts once its input magnitude gets
// large, and world coordinates grow unbounded as the player roams) and a
// float-multiply hash like fract(p*123.34), which looks fine on continuous
// input but collapses to a short repeating ramp on the whole-integer texel
// coordinates this is actually called with here (fract(n*123.34) is just
// fract(0.34*n), an obviously-patterned sequence, not noise).
float hash21(vec2 p)
{
	uvec2 v = uvec2(ivec2(floor(p)));
	v = v * 1664525u + 1013904223u;
	v.x += v.y * 1664525u;
	v.y += v.x * 1664525u;
	v ^= v >> 16u;
	v.x += v.y * 1664525u;
	v.y += v.x * 1664525u;
	return float(v.x) * (1.0 / 4294967296.0);
}

float valueNoise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	float a = hash21(i);
	float b = hash21(i + vec2(1.0, 0.0));
	float c = hash21(i + vec2(0.0, 1.0));
	float d = hash21(i + vec2(1.0, 1.0));
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Picks the two world-space axes that lie in a cube face's plane, using
// whichever normal component is dominant (faces are axis-aligned, so this
// is always unambiguous). Used to fake a 16x16 pixel grid per face without
// an actual texture.
vec2 faceUV(vec3 worldPos, vec3 normal)
{
	vec3 a = abs(normal);
	if (a.x >= a.y && a.x >= a.z) return worldPos.yz;
	if (a.y >= a.z) return worldPos.xz;
	return worldPos.xy;
}

// Breaks up an otherwise flat per-material color with cheap procedural noise
// (no textures): a coarse blotch/hue shift for grass and other terrain
// materials, plus a crisp 16-texel-per-block pixel dither applied to every
// material so blocks read as a subtly pixelated texture instead of a flat
// fill -- not a full Minecraft-style texture, just enough grain to not look
// like a solid color.
vec3 breakUpColor(vec3 baseColor, int materialId, vec3 worldPos, vec3 normal)
{
	float blotch = valueNoise(worldPos.xz * 0.12);

	vec3 color = baseColor;
	if (materialId == MAT_GRASS)
	{
		vec3 altShade = baseColor * vec3(0.72, 0.98, 0.55);
		color = mix(baseColor, altShade, blotch * 0.45);
	}
	else if (materialId == MAT_STONE || materialId == MAT_DIRT || materialId == MAT_SAND || materialId == MAT_SNOW)
	{
		color = baseColor * (1.0 + (blotch - 0.5) * 0.12);
	}

	vec2 texel = floor(faceUV(worldPos, normal) * 16.0);
	float pixel = hash21(texel) - 0.5;
	return color * (1.0 + pixel * 0.10);
}

void main() {
	vec3 baseColor = toLinear(v_color0);
	int materialId = int(v_texcoord0 + 0.5);
	baseColor = breakUpColor(baseColor, materialId, v_worldPos, v_normal);

	// Environment colors (sun/sky/ground/fog) are hand-authored "what looks
	// right on screen" values, same as the per-material colors above --
	// linearize them here too so they compose correctly with the rest of
	// the lighting math before the single toGamma() on the way out.
	vec3 lightColor = toLinear(u_lightColor.rgb);
	vec3 ambientSky = toLinear(u_ambientSky.rgb);
	vec3 ambientGround = toLinear(u_ambientGround.rgb);
	vec3 fogColor = toLinear(u_fogColor.rgb);

	// v_ao is baked as a direct display-space darkening factor, same as the
	// material colors -- linearize it too, or the corner-darkening it's
	// meant to produce gets compressed by the single toGamma() at the end of
	// main() (a 45% linear darkening only reads as ~23% once gamma-encoded),
	// making occlusion look far weaker than it's actually specified to be.
	float ao = pow(v_ao, 2.2);

	// Hemisphere ambient: faces looking toward the sky read as sky-lit,
	// faces looking toward the ground pick up a warm bounce tint, instead of
	// one flat ambient value regardless of orientation.
	vec3 ambient = mix(ambientGround, ambientSky, v_normal.y * 0.5 + 0.5) * baseColor * ao;

	// Half-Lambert instead of a hard max(dot,0) clamp -- softens the
	// terminator so unlit faces fade instead of snapping to black.
	float ndotl = dot(v_normal, normalize(u_lightDirection.xyz));
	float wrapped = ndotl * 0.5 + 0.5;
	wrapped *= wrapped;
	vec3 diffuse = wrapped * lightColor * baseColor;

	vec3 color = ambient + diffuse;

	float dist = length(v_worldPos - u_cameraPos.xyz);
	float fogFactor = clamp((dist - u_fogParams.x) / max(u_fogParams.y - u_fogParams.x, 0.001), 0.0, 1.0);
	color = mix(color, fogColor, fogFactor);

	gl_FragColor = vec4(toGamma(color), 1.0);
}
