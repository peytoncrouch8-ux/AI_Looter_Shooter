// Ellis's clothes in body space (origin at the base of the neck, +z out of the chest), and the
// palette every option shares. Included after EllisHead.glsl; the scene sets the knobs in initScene().

float CAPE_ON = 1.0;       // the duster's shoulder cape
float COLLAR_UP = 1.0;     // the duster's collar turned up round the jaw
float BANDOLIER_ON = 1.0;
float VEE = 0.22;          // half-width of the coat's front opening at the collar

float sdTorso(vec3 b) {
	vec3 q = vec3(abs(b.x), b.y, b.z);
	float d = sdEllipsoid(b - vec3(0.0, -1.75, -0.05), vec3(1.80, 1.95, 1.0));
	d = smin(d, sdEllipsoid(q - vec3(1.28, -0.66, -0.10), vec3(0.74, 0.50, 0.68)), 0.55);
	d = smin(d, sdCone2(q, vec3(0.35, 0.05, -0.15), vec3(1.26, -0.42, -0.10), 0.30, 0.40), 0.4);
	d = smin(d, sdCone2(q, vec3(1.62, -0.90, -0.10), vec3(1.80, -3.8, -0.20), 0.56, 0.52), 0.45);
	return d;
}

// The bandolier's 2D frame across the chest: u along the strap, v across it.
vec2 strapUV(vec3 b) {
	vec2 a = vec2(1.20, -0.05), c = vec2(-1.50, -3.6);
	vec2 s = normalize(c - a);
	vec2 rel = b.xy - a;
	return vec2(dot(rel, s), dot(rel, vec2(-s.y, s.x)));
}

// The duster: coat, shoulder cape, turned-up collar; the waistcoat and its buttons in the opening;
// the bandolier over everything.
vec2 sdWear(vec3 b) {
	float torso = sdTorso(b);
	float ang = atan(b.x, b.z);
	float folds = 0.03 * fbm(b * vec3(1.5, 0.6, 1.5)) + 0.02 * sin(b.x * 4.0 + b.y * 1.2 + fbm(b * 1.5) * 3.0);
	float coat = torso - 0.15 + folds;
	float halfW = VEE + max(0.15 - b.y, 0.0) * 0.12;
	float vee = max(abs(b.x) - halfW, 0.30 - b.z);
	coat = smax(coat, -vee, 0.05);
	vec2 r = vec2(coat, M_COAT);
	// Waistcoat in the opening, with a row of buttons.
	float vest = torso - 0.07;
	r = opU(r, vec2(vest, M_BLACKCLOTH));
	vec3 bt = b - vec3(0.0, 0.0, 0.0);
	float by = clamp(floor((bt.y + 0.55) / 0.36 + 0.5), -6.0, 0.0) * 0.36 - 0.55;
	float button = length(vec3(bt.x - 0.05, bt.y - by, vest + 0.0)) - 0.045;
	r = opU(r, vec2(button, M_BRASS));
	float outer = coat;
	if (CAPE_ON > 0.5) {
		// The cape: a shell hung from the collar over the shoulders, fluted lower down, open at the front.
		float flute = 0.045 * sin(ang * 9.0 + fbm(b * 1.2) * 2.5) * smoothstep(-0.1, -1.1, b.y);
		float base = torso - 0.32 + flute + 0.02 * fbm(b * 3.0);
		float hem = -1.30 - 0.18 * (1.0 - abs(cos(ang))) + 0.05 * sin(ang * 9.0 + 1.0);
		float cape = rinter(abs(base) - 0.045, hem - b.y, 0.035);
		cape = smax(cape, -max(abs(b.x) - halfW - 0.06, 0.30 - b.z), 0.05);
		r = opU(r, vec2(cape, M_COAT));
		outer = min(outer, rinter(base - 0.045, hem - b.y, 0.035));
	}
	if (COLLAR_UP > 0.5) {
		// The collar turned up against the wind: a flared shell, open at the front, higher at the back.
		vec3 c = b - vec3(0.0, 0.0, -0.12);
		float ca = atan(c.x, c.z);
		float rad = 0.80 + 0.16 * clamp(c.y, 0.0, 1.0) + 0.03 * fbm(c * 4.0);
		float top = 0.52 + 0.36 * (1.0 - cos(ca)) * 0.5;
		float collar = rinter(abs(length(c.xz) - rad) - 0.06, max(-c.y - 0.5, c.y - top), 0.045);
		collar = smax(collar, -(0.60 - abs(ca) - c.y * 0.18) * 0.8, 0.05);
		r = opU(r, vec2(collar, M_COAT));
	}
	if (BANDOLIER_ON > 0.5) {
		vec2 uv = strapUV(b);
		if (abs(uv.y) < 0.6 && uv.x > -0.6 && uv.x < 5.0) {
			float strap = rinter(outer - 0.07, abs(uv.y) - 0.21, 0.02);
			r = opU(r, vec2(strap, M_LEATHER));
			float cell = 0.17;
			float ui = (floor(uv.x / cell) + 0.5) * cell;
			float du = uv.x - ui;
			if (ui > 0.3 && ui < 3.4) {
				float hh = outer - 0.125;
				float rr = length(vec2(hh, du));
				float vl = uv.y - 0.04;
				r = opU(r, vec2(max(rr - 0.05, abs(vl) - 0.20), M_BRASS));
				float tip = rr - 0.045 * clamp(1.0 - (vl - 0.20) / 0.10, 0.0, 1.0);
				r = opU(r, vec2(max(tip, max(0.19 - vl, vl - 0.30)), M_SILVER));
				r = opU(r, vec2(max(rr - 0.064, abs(uv.y + 0.02) - 0.10), M_LEATHER));
			}
		}
	}
	return r;
}

// The bandana worn up: tented from the nose tip, falling straight to below the chin, then a bib over
// the throat. face is the head without ears.
float sdBandanaUp(vec3 p, float face) {
	float t = smin(face, sdSphere(p - vec3(0.0, -0.42, 0.99), 0.09), 0.45);
	t = smin(t, sdEllipsoid(p - vec3(0.0, -0.80, 0.80), vec3(0.36, 0.42, 0.20)), 0.30);
	t = smin(t, sdEllipsoid(p - vec3(0.0, -1.02, 0.55), vec3(0.36, 0.22, 0.24)), 0.35);
	t = smin(t, sdEllipsoid(p - vec3(0.0, -1.55, 0.28), vec3(0.62, 0.62, 0.34)), 0.45);
	t = smin(t, sdCone2(p, vec3(0.0, -1.25, 0.32), vec3(0.0, -2.30, 0.62), 0.30, 0.04), 0.3);
	float ay = max(-p.y - 0.25, 0.0);
	float folds = 0.022 * sin(p.x / (0.35 + ay * 0.6) * 4.0 + fbm(p * 1.5) * 2.0) * smoothstep(0.0, 0.6, ay);
	folds += 0.016 * sin(p.y * 7.0 + p.x * 2.5 + fbm(p * 2.0) * 2.0) * smoothstep(0.7, 1.3, ay);
	float d = t - 0.055 + folds;
	// The top edge rides over the nose bridge and runs down the cheeks to pass under the ears.
	float top = -0.17 - 0.55 * p.x * p.x;
	return smax(d, (p.y - top) * 0.9, 0.02);
}

// ---- palette (sRGB picks; main.glsl lights them in linear)

Mat skinMat(vec3 h, vec3 tint) {
	Mat m = mat0();
	vec3 q = vec3(abs(h.x), h.y, h.z);
	m.albedo = tint;
	float ring = 1.0 - smoothstep(0.10, 0.26, length((q - vec3(0.29, 0.0, 0.66)) * vec3(1.0, 1.25, 1.0)));
	m.albedo = mix(m.albedo, tint * vec3(0.66, 0.66, 0.72), ring * 0.45);
	// A cold flush where the living would be warm: nose, ears, cheeks.
	float flush = (1.0 - smoothstep(0.0, 0.2, length(h - vec3(0.0, -0.42, 0.98)))) + (1.0 - smoothstep(0.0, 0.25, length(q - vec3(0.42, -0.22, 0.55))));
	m.albedo = mix(m.albedo, tint * vec3(0.95, 0.78, 0.86), clamp(flush, 0.0, 1.0) * 0.5);
	// Brows pulled low and in toward the nose.
	float by = 0.175 + 0.13 * (q.x - 0.08);
	float brow = (1.0 - smoothstep(0.022, 0.04, abs(h.y - by))) * smoothstep(0.05, 0.10, q.x) * (1.0 - smoothstep(0.40, 0.50, q.x));
	brow *= step(0.5, h.z) * (0.75 + 0.25 * vnoise(h * 60.0));
	m.albedo = mix(m.albedo, vec3(0.20, 0.15, 0.12), brow);
	float dirt = smoothstep(0.55, 0.8, fbm(h * 3.0 + 4.0));
	m.albedo = mix(m.albedo, vec3(0.50, 0.42, 0.34), dirt * 0.5);
	m.rough = 0.5; m.spec = 0.3; m.wrap = 0.35;
	m.sss = vec3(0.12, 0.05, 0.06);
	return m;
}

Mat eyeMat(vec3 h, vec3 camHead, vec3 glowCol) {
	Mat m = mat0();
	vec3 ec = eyeCentre(sign(h.x));
	float c = dot(normalize(h - ec), normalize(camHead - ec));
	m.albedo = vec3(0.05, 0.10, 0.12);
	// The whole visible eye burns; a hotter core where it looks out.
	float core = smoothstep(0.86, 0.99, c);
	m.emissive = glowCol * (1.1 + 0.9 * core) + vec3(0.30, 0.65, 0.75) * core * core;
	m.glow = 1.0; m.spec = 1.0; m.rough = 0.1;
	return m;
}

Mat feltMat(vec3 h, vec3 nh) {
	Mat m = mat0();
	float dust = smoothstep(0.1, 0.9, nh.y) * 0.5 + fbm(h * 5.0) * 0.4;
	m.albedo = mix(vec3(0.10, 0.09, 0.085), vec3(0.27, 0.235, 0.20), dust);
	m.rough = 0.95; m.spec = 0.05; m.sheen = 0.55; m.wrap = 0.15;
	return m;
}

Mat leatherMat(vec3 p) {
	Mat m = mat0();
	m.albedo = vec3(0.20, 0.12, 0.08) * (0.8 + 0.4 * fbm(p * 12.0));
	m.rough = 0.45; m.spec = 0.45;
	return m;
}

Mat bandanaMat(vec3 h) {
	// Turkey red, sun-faded, a small printed teardrop and dot pattern, dust in the folds.
	Mat m = mat0();
	vec2 uv = h.xy * 9.0;
	vec2 ci = floor(uv);
	vec2 cell = fract(uv) - 0.5;
	float rot = hash12(ci) * 6.28;
	cell = mat2(cos(rot), sin(rot), -sin(rot), cos(rot)) * cell;
	float drop = length(cell * vec2(1.0, 1.6) + vec2(0.0, 0.08)) - 0.16 + cell.y * 0.25;
	float motif = 1.0 - smoothstep(0.0, 0.03, abs(drop) - 0.025);
	float dots = 1.0 - smoothstep(0.03, 0.05, length(fract(uv * 2.0 + 0.25) - 0.5));
	vec3 red = mix(vec3(0.50, 0.075, 0.07), vec3(0.62, 0.17, 0.13), fbm(h * 4.0));
	m.albedo = mix(red, vec3(0.86, 0.78, 0.66), max(motif * 0.32, dots * 0.15));
	// a pale border along the hem that rides over the nose
	float edge = h.y - (-0.17 - 0.55 * h.x * h.x);
	m.albedo = mix(m.albedo, vec3(0.78, 0.68, 0.58), (1.0 - smoothstep(0.007, 0.013, abs(edge + 0.045))) * 0.35);
	m.albedo = mix(m.albedo, vec3(0.62, 0.48, 0.40), smoothstep(0.55, 0.85, fbm(h * 2.5 + 9.0)) * 0.5);
	m.rough = 0.85; m.spec = 0.08; m.wrap = 0.08; m.sheen = 0.25;
	m.sss = vec3(0.28, 0.03, 0.02);
	return m;
}

Mat coatMat(vec3 b, vec3 n) {
	// Oiled canvas, sun- and dust-bleached on the shoulders.
	Mat m = mat0();
	float dust = fbm(b * 2.0) * 0.6 + smoothstep(0.2, 1.0, n.y) * 0.4;
	m.albedo = mix(vec3(0.19, 0.145, 0.11), vec3(0.36, 0.30, 0.23), dust);
	m.rough = 0.75; m.spec = 0.15; m.sheen = 0.35; m.wrap = 0.15;
	return m;
}

Mat brassMat(vec3 p) {
	Mat m = mat0();
	m.albedo = mix(vec3(0.92, 0.75, 0.45), vec3(0.70, 0.56, 0.36), fbm(p * 20.0) * 0.6);
	m.metal = 1.0; m.spec = 1.4; m.rough = 0.28;
	return m;
}

Mat leadMat() { Mat m = mat0(); m.albedo = vec3(0.80, 0.62, 0.52); m.metal = 1.0; m.spec = 1.0; m.rough = 0.35; return m; }

Mat hairMat(vec3 h) {
	Mat m = mat0();
	m.albedo = vec3(0.16, 0.12, 0.10) * (0.7 + 0.6 * vnoise(h * vec3(40.0, 6.0, 40.0)));
	m.rough = 0.45; m.spec = 0.5; m.sheen = 0.5;
	return m;
}

Mat featherMat(vec3 h) {
	Mat m = mat0();
	float tip = smoothstep(0.85, 1.25, toHat(h).y);
	m.albedo = mix(vec3(0.08, 0.09, 0.11), vec3(0.62, 0.60, 0.58), tip);
	m.rough = 0.3; m.spec = 0.7;
	return m;
}

Mat blackClothMat(vec3 b) {
	Mat m = mat0();
	m.albedo = vec3(0.13, 0.12, 0.13) * (0.85 + 0.3 * fbm(b * 8.0));
	m.rough = 0.8; m.spec = 0.1; m.sheen = 0.3;
	return m;
}
