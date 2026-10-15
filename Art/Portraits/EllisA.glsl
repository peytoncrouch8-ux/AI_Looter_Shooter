// Ellis, option A: "The Revenant". The look the HUD portrait set: cattleman hat pulled low, the red
// bandana up over nose and mouth, eyes burning cyan in the brim's shadow, a dust-brown duster with a
// shoulder cape and the collar turned up, a bandolier of brass across the chest.
// Warm dusk key from the left, a cold cyan rim from behind on the right.
// PAINT {"bgTop":"#0d1622","bgBottom":"#05070b","bgGlow":"#2a3f57","glowAt":[0.72,0.28],"sparks":"#ff9f1c","dust":"#7fb6d6","glow":"#5ad8ff","label":"A · THE REVENANT"}

const vec3 CAM_POS = vec3(0.0, -0.35, 15.0);
const vec3 CAM_TARGET = vec3(0.0, -0.45, 0.0);
const float CAM_FOV = 20.5;
const vec3 KEY_DIR = normalize(vec3(-0.90, 0.40, 0.18));
const vec3 KEY_COL = vec3(3.3, 2.05, 1.10);
const float KEY_SOFT = 10.0;
const vec3 RIM_DIR = normalize(vec3(0.90, 0.20, -0.40));
const vec3 RIM_COL = vec3(0.6, 1.6, 2.3);
const vec3 SKY_COL = vec3(0.06, 0.09, 0.19);
const vec3 GROUND_COL = vec3(0.09, 0.06, 0.045);
const float LIGHT_POOL = 0.72;
const vec3 POOL_AT = vec3(-0.3, 0.2, 0.5);
const vec3 GLOW_COL = vec3(0.18, 0.80, 1.15);

mat3 HM;           // head space -> world
const vec3 HEAD_POS = vec3(0.0, 0.0, 0.0);
mat3 BM;           // body space -> world
const float BODY_SCALE = 0.84;
const vec3 BODY_POS = vec3(0.08, -1.40, -0.32);

#include "EllisHead.glsl"
#include "EllisWear.glsl"

void initScene() {
	HM = rotY(-0.42) * rotX(0.10) * rotZ(0.05);
	BM = rotY(-0.22);
	EYE_UP = 0.016; EYE_LO = -0.036; EYE_ANGRY = 0.20;
	HAT_TIP = 0.16;
	HAIR_LONG = 0.3;
}

vec2 map(vec3 p) {
	vec2 res = vec2(1e9, 0.0);
	vec3 h = transpose(HM) * (p - HEAD_POS);
	float hb = length(h - vec3(0.0, 0.4, 0.1)) - 2.7;
	if (hb < 0.4) {
		float face = sdHeadBase(h);
		float skin = smin(face, sdLids(h), 0.06);   // the hair covers the ears
		res = opU(res, vec2(skin, M_SKIN));
		res = opU(res, vec2(sdEyes(h), M_EYE));
		res = opU(res, vec2(sdBandanaUp(h, sdFaceHull(h)), M_BANDANA));
		res = opU(res, vec2(sdHair(h), M_HAIR));
		res = opU(res, sdHat(h));
	} else res.x = hb;
	vec3 b = transpose(BM) * (p - BODY_POS) / BODY_SCALE;
	float bb = length(b - vec3(0.0, -1.6, 0.0)) - 3.6;
	if (bb < 0.4) { vec2 r = sdWear(b); r.x *= BODY_SCALE; res = opU(res, r); }
	else res.x = min(res.x, bb * BODY_SCALE);
	return res;
}

Mat getMat(float id, vec3 p, vec3 n) {
	vec3 h = transpose(HM) * (p - HEAD_POS);
	vec3 b = transpose(BM) * (p - BODY_POS) / BODY_SCALE;
	if (id == M_SKIN) return skinMat(h, vec3(0.74, 0.73, 0.76));
	if (id == M_EYE) return eyeMat(h, transpose(HM) * (CAM_POS - HEAD_POS), GLOW_COL);
	if (id == M_FELT) return feltMat(h, transpose(HM) * n);
	if (id == M_LEATHER) return leatherMat(p);
	if (id == M_BANDANA) return bandanaMat(h);
	if (id == M_COAT) return coatMat(b, n);
	if (id == M_BRASS) return brassMat(p);
	if (id == M_SILVER) return leadMat();
	if (id == M_HAIR) return hairMat(h);
	if (id == M_FEATHER) return featherMat(h);
	if (id == M_BLACKCLOTH) return blackClothMat(b);
	return mat0();
}

// The eyes' own light falls on the lids, the sockets and the brim's underside.
vec3 extraLight(vec3 p, vec3 n, vec3 rd, Mat m) {
	vec3 acc = vec3(0.0);
	for (int s = -1; s <= 1; s += 2) {
		vec3 e = HM * (eyeCentre(float(s)) + vec3(0.0, 0.0, 0.10)) + HEAD_POS;
		vec3 l = e - p;
		float d2 = dot(l, l);
		acc += m.albedo * GLOW_COL * max(dot(n, normalize(l)) * 0.7 + 0.3, 0.0) * 0.6 / (1.0 + d2 * 25.0);
	}
	return acc;
}
