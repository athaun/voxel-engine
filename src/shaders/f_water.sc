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
uniform vec4 u_time;  // x = seconds since start

// The greedy mesher can merge an entire lake into a handful of huge quads,
// so there isn't enough vertex density for real displaced waves -- instead
// fake the ripple as a perturbed normal driven by world position + time,
// entirely in the fragment shader.
vec3 rippleNormal(vec2 worldXZ, float t)
{
	float wave1 = sin(worldXZ.x * 0.5 + t * 1.3) + cos(worldXZ.y * 0.6 - t * 0.9);
	float wave2 = sin((worldXZ.x + worldXZ.y) * 0.9 + t * 2.1);
	vec2 slope = vec2(wave1, wave2) * 0.15;
	return normalize(vec3(slope.x, 1.0, slope.y));
}

void main() {
	vec3 rippled = rippleNormal(v_worldPos.xz, u_time.x);
	// Blend the fake ripple into the real face normal so vertical shoreline
	// faces still mostly look vertical instead of tilting toward "up".
	vec3 normal = normalize(mix(v_normal, rippled, max(v_normal.y, 0.0)));

	vec3 waterTint = toLinear(v_color0);

	// Environment colors are hand-authored display-space values, same as
	// the per-material colors -- linearize them here too so they compose
	// correctly with the rest of the lighting math before the single
	// toGamma() on the way out.
	vec3 lightColor = toLinear(u_lightColor.rgb);
	vec3 ambientSky = toLinear(u_ambientSky.rgb);
	vec3 ambientGround = toLinear(u_ambientGround.rgb);
	vec3 fogColor = toLinear(u_fogColor.rgb);

	// Same reasoning as f_cubes.sc: v_ao is a display-space darkening factor,
	// linearize it before using it in the linear lighting math.
	float ao = pow(v_ao, 2.2);
	vec3 ambient = mix(ambientGround, ambientSky, normal.y * 0.5 + 0.5) * waterTint * ao;

	vec3 toLight = normalize(u_lightDirection.xyz);
	float ndotl = dot(normal, toLight);
	float wrapped = ndotl * 0.5 + 0.5;
	wrapped *= wrapped;
	vec3 diffuse = wrapped * lightColor * waterTint;

	vec3 viewDir = normalize(u_cameraPos.xyz - v_worldPos);
	vec3 halfVec = normalize(toLight + viewDir);
	float spec = pow(max(dot(normal, halfVec), 0.0), 48.0);

	// Schlick fresnel: near-grazing angles reflect the sky, straight-on
	// angles show the water tint through. Capped well under 1.0 -- at full
	// strength this made distant/grazing water (most of a lake as normally
	// viewed) blend almost entirely into the fog/sky color on top of the
	// separate distance-fog blend below, so water stopped reading as water.
	float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 5.0);
	fresnel = mix(0.03, 0.45, fresnel);

	vec3 base = ambient + diffuse;
	vec3 color = mix(base, fogColor, fresnel) + spec * lightColor;

	float dist = length(v_worldPos - u_cameraPos.xyz);
	float fogFactor = clamp((dist - u_fogParams.x) / max(u_fogParams.y - u_fogParams.x, 0.001), 0.0, 1.0);
	color = mix(color, fogColor, fogFactor);

	float alpha = mix(0.75, 0.95, fresnel);
	gl_FragColor = vec4(toGamma(color), alpha);
}
