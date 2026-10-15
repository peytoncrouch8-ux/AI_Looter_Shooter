// Shared signed-distance helpers for the portrait busts (render.js prepends this to each scene).
// Units: 1 = 10 cm. +y up, +z toward the camera, +x the viewer's right.

#define PI 3.14159265

float hash13(vec3 p) { p = fract(p * 0.1031); p += dot(p, p.zyx + 31.32); return fract((p.x + p.y) * p.z); }
float hash12(vec2 p) { vec3 q = fract(vec3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return fract((q.x + q.y) * q.z); }

float vnoise(vec3 p) {
	vec3 i = floor(p), f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash13(i), hash13(i + vec3(1, 0, 0)), f.x), mix(hash13(i + vec3(0, 1, 0)), hash13(i + vec3(1, 1, 0)), f.x), f.y),
		mix(mix(hash13(i + vec3(0, 0, 1)), hash13(i + vec3(1, 0, 1)), f.x), mix(hash13(i + vec3(0, 1, 1)), hash13(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}
float fbm(vec3 p) { float a = 0.5, s = 0.0; for (int i = 0; i < 4; i++) { s += a * vnoise(p); p = p * 2.03 + 7.1; a *= 0.5; } return s; }

mat3 rotX(float a) { float c = cos(a), s = sin(a); return mat3(1, 0, 0, 0, c, s, 0, -s, c); }
mat3 rotY(float a) { float c = cos(a), s = sin(a); return mat3(c, 0, -s, 0, 1, 0, s, 0, c); }
mat3 rotZ(float a) { float c = cos(a), s = sin(a); return mat3(c, s, 0, -s, c, 0, 0, 0, 1); }

// Polynomial smooth min/max: k is the blend width.
float smin(float a, float b, float k) { float h = max(k - abs(a - b), 0.0) / k; return min(a, b) - h * h * k * 0.25; }
float smax(float a, float b, float k) { return -smin(-a, -b, k); }

float sdSphere(vec3 p, float r) { return length(p) - r; }
// Ellipsoid bound good enough for marching (IQ's improved form).
float sdEllipsoid(vec3 p, vec3 r) { float k0 = length(p / r); float k1 = length(p / (r * r)); return k0 * (k0 - 1.0) / max(k1, 1e-6); }
float sdCapsule(vec3 p, vec3 a, vec3 b, float r) { vec3 pa = p - a, ba = b - a; float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0); return length(pa - ba * h) - r; }
// Capsule whose radius runs from ra at a to rb at b.
float sdCone2(vec3 p, vec3 a, vec3 b, float ra, float rb) { vec3 pa = p - a, ba = b - a; float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0); return length(pa - ba * h) - mix(ra, rb, h); }
float sdBox(vec3 p, vec3 b) { vec3 q = abs(p) - b; return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0); }
float sdRoundBox(vec3 p, vec3 b, float r) { return sdBox(p, b - r) - r; }
float sdTorus(vec3 p, vec2 t) { vec2 q = vec2(length(p.xz) - t.x, p.y); return length(q) - t.y; }
float sdCylY(vec3 p, float r, float h) { vec2 d = abs(vec2(length(p.xz), p.y)) - vec2(r, h); return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)); }
// Rounded intersection of two distances (a fillet of radius r on the edge).
float rinter(float a, float b, float r) { vec2 u = max(vec2(a + r, b + r), 0.0); return min(max(a, b), -r) + length(u); }

// Material ids shared by every scene (the shading in main.glsl switches on them).
#define M_SKIN 1.0
#define M_EYE 2.0
#define M_FELT 3.0
#define M_LEATHER 4.0
#define M_BANDANA 5.0
#define M_COAT 6.0
#define M_SHIRT 7.0
#define M_BRASS 8.0
#define M_HAIR 9.0
#define M_FEATHER 10.0
#define M_SILVER 11.0
#define M_LINEN 12.0
#define M_BLACKCLOTH 13.0

vec2 opU(vec2 a, vec2 b) { return a.x < b.x ? a : b; }

// What a scene's getMat() returns for a surface point.
struct Mat {
	vec3 albedo;
	float rough;     // 0 mirror .. 1 chalk
	float spec;      // specular strength
	float metal;     // tints the specular by the albedo
	vec3 sss;        // colour added at the light's terminator (skin, thin cloth)
	float wrap;      // wrap lighting: how far light reaches round the form
	float sheen;     // velvet edge light (felt, wool)
	vec3 emissive;
	float glow;      // 1 where the bloom pass should pick this pixel up
};
Mat mat0() { Mat m; m.albedo = vec3(0.5); m.rough = 0.7; m.spec = 0.1; m.metal = 0.0; m.sss = vec3(0); m.wrap = 0.0; m.sheen = 0.0; m.emissive = vec3(0); m.glow = 0.0; return m; }
