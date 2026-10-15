// The march and the painterly lighting shared by every bust. render.js builds the shader as
// lib.glsl + <scene>.glsl + this file. A scene defines map(), getMat(), extraLight() and the
// camera and light constants (CAM_*, KEY_*, RIM_*, SKY_COL, GROUND_COL).

uniform vec2 uRes;
uniform int uPass;   // 0 colour + coverage, 1 glow mask + depth + id, 2 normal
out vec4 outColor;

vec3 calcNormal(vec3 p) {
	const vec2 e = vec2(0.0015, -0.0015);
	return normalize(e.xyy * map(p + e.xyy).x + e.yyx * map(p + e.yyx).x + e.yxy * map(p + e.yxy).x + e.xxx * map(p + e.xxx).x);
}

// Penumbra shadow (IQ): k sets how soft the shadow's edge is.
float softShadow(vec3 ro, vec3 rd, float k) {
	float res = 1.0, t = 0.03;
	for (int i = 0; i < 64; i++) {
		float h = map(ro + rd * t).x;
		res = min(res, k * h / t);
		t += clamp(h * 0.8, 0.02, 0.35);
		if (res < 0.002 || t > 12.0) break;
	}
	return clamp(res, 0.0, 1.0);
}

float calcAO(vec3 p, vec3 n) {
	float occ = 0.0, sca = 1.0;
	for (int i = 1; i <= 5; i++) {
		float h = 0.04 + 0.12 * float(i);
		occ += (h - map(p + n * h).x) * sca;
		sca *= 0.75;
	}
	return clamp(1.0 - 1.6 * occ, 0.0, 1.0);
}

vec3 shade(vec3 p, vec3 n, vec3 rd, Mat m) {
	// Scenes pick colours as sRGB; light them in linear.
	m.albedo = pow(m.albedo, vec3(2.2));
	vec3 v = -rd;
	float ao = calcAO(p, n);
	float ndl = dot(n, KEY_DIR);
	float sh = ndl > -0.3 ? softShadow(p + n * 0.012, KEY_DIR, KEY_SOFT) : 0.0;
	// Wrapped diffuse with a firmed-up terminator: reads as a painter's light/shadow split.
	float dif = clamp((ndl + m.wrap) / (1.0 + m.wrap), 0.0, 1.0);
	dif = mix(dif, smoothstep(0.05, 0.55, dif), 0.45) * sh;
	float pool = 1.0 - LIGHT_POOL * smoothstep(1.2, 4.2, length(p - POOL_AT));
	dif *= pool;
	vec3 col = m.albedo * KEY_COL * dif;
	// Warm (skin) or tinted bleed where light turns to shadow.
	float term = smoothstep(-0.25, 0.2, ndl) * (1.0 - smoothstep(0.2, 0.65, ndl));
	col += m.albedo * m.sss * KEY_COL * term * mix(0.35, 1.0, sh) * pool;
	// Key specular, Blinn-Phong from the roughness.
	vec3 h = normalize(KEY_DIR + v);
	float shin = mix(180.0, 6.0, m.rough);
	float spe = pow(max(dot(n, h), 0.0), shin) * (shin + 8.0) / 60.0 * m.spec * sh;
	col += KEY_COL * spe * mix(vec3(1.0), m.albedo * 1.6, m.metal);
	// Rim (back) light: the cool edge that pulls the bust off the background.
	float fres = pow(1.0 - clamp(dot(n, v), 0.0, 1.0), 3.0);
	float rimL = smoothstep(-0.1, 0.6, dot(n, RIM_DIR));
	col += RIM_COL * mix(m.albedo, vec3(0.6), 0.35 + 0.4 * m.metal) * rimL * fres * (0.7 + m.sheen) * ao;
	// Sheen for felt and wool.
	col += m.sheen * KEY_COL * m.albedo * fres * clamp(ndl + 0.5, 0.0, 1.0) * 0.6;
	// Sky/ground ambient.
	col += m.albedo * mix(GROUND_COL, SKY_COL, n.y * 0.5 + 0.5) * ao;
	col += extraLight(p, n, rd, m) * mix(0.5, 1.0, ao);
	col += m.emissive;
	return col;
}

vec3 tonemap(vec3 x) {
	// ACES fit (Narkowicz), then a touch of contrast.
	x *= 0.9;
	x = clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
	return pow(x, vec3(1.0 / 2.2));
}

void main() {
	initScene();
	vec2 uv = (gl_FragCoord.xy * 2.0 - uRes) / uRes.y;
	vec3 ro = CAM_POS;
	vec3 fw = normalize(CAM_TARGET - CAM_POS);
	vec3 rt = normalize(cross(fw, vec3(0, 1, 0)));
	vec3 up = cross(rt, fw);
	float fl = 1.0 / tan(radians(CAM_FOV) * 0.5);
	vec3 rd = normalize(uv.x * rt + uv.y * up + fl * fw);

	float t = 0.0, id = -1.0;
	for (int i = 0; i < 260; i++) {
		vec2 h = map(ro + rd * t);
		if (h.x < 0.0008 * t) { id = h.y; break; }
		t += h.x * 0.85;
		if (t > 40.0) break;
	}
	if (id < 0.0) { outColor = vec4(0.0); return; }
	vec3 p = ro + rd * t;
	vec3 n = calcNormal(p);
	Mat m = getMat(id, p, n);
	if (uPass == 0) {
		outColor = vec4(tonemap(shade(p, n, rd, m)), 1.0);
	} else if (uPass == 1) {
		outColor = vec4(m.glow, t / 40.0, id / 16.0, 1.0);
	} else {
		outColor = vec4(n * 0.5 + 0.5, 1.0);
	}
}
