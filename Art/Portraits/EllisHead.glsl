// Ellis's head, hat, hair and eyes in head space, shared by the three portrait options.
// Head space: origin at the eye line's centre, +z out of the face, +y up, +x the subject's left.
// The scene sets the knobs below in initScene() before anything is marched.

float EYE_UP = 0.030;      // upper lid line at the eye's centre (smaller = narrower glare)
float EYE_LO = -0.050;     // lower lid line
float EYE_ANGRY = 0.16;    // how far the upper lid drops toward the nose
float HAT_ON = 1.0;        // 0 leaves the hat off
float HAT_TIP = 0.14;      // forward tilt of the hat (radians)
vec3 HAT_POS = vec3(0.0, 0.50, -0.08);
float HAIR_LONG = 0.0;     // 0 short crop under the hat, 1 longer locks to the jaw

// The skull and face, without eyes or lids.
float sdHeadBase(vec3 p) {
	vec3 q = vec3(abs(p.x), p.y, p.z);
	float d = sdEllipsoid(p - vec3(0.0, 0.30, -0.10), vec3(0.74, 0.88, 0.97));
	d = smin(d, sdEllipsoid(p - vec3(0.0, -0.26, 0.18), vec3(0.60, 0.84, 0.70)), 0.25);
	// Jaw from under the ear to the chin, then the chin itself.
	d = smin(d, sdCone2(q, vec3(0.50, -0.40, -0.24), vec3(0.19, -0.98, 0.42), 0.16, 0.12), 0.22);
	d = smin(d, sdEllipsoid(p - vec3(0.0, -1.00, 0.47), vec3(0.21, 0.16, 0.18)), 0.16);
	// Cheekbones, set high; a hollow under them (Ellis is dead and lean).
	d = smin(d, sdEllipsoid(q - vec3(0.43, -0.10, 0.50), vec3(0.22, 0.13, 0.19)), 0.14);
	d = smax(d, -sdEllipsoid(q - vec3(0.52, -0.52, 0.48), vec3(0.11, 0.17, 0.15)), 0.16);
	// Brow ridge and the temples' dip.
	d = smin(d, sdCapsule(q, vec3(0.07, 0.21, 0.79), vec3(0.47, 0.22, 0.60), 0.08), 0.12);
	d = smax(d, -sdEllipsoid(q - vec3(0.80, 0.16, 0.30), vec3(0.12, 0.18, 0.20)), 0.12);
	// Eye sockets.
	d = smax(d, -sdEllipsoid(q - vec3(0.29, 0.02, 0.76), vec3(0.18, 0.11, 0.14)), 0.12);
	// Nose: bridge, tip, wings, nostrils.
	float nose = sdCone2(p, vec3(0.0, 0.10, 0.81), vec3(0.0, -0.40, 1.00), 0.062, 0.08);
	nose = smin(nose, sdSphere(p - vec3(0.0, -0.40, 0.985), 0.09), 0.06);
	nose = smin(nose, sdEllipsoid(q - vec3(0.105, -0.45, 0.885), vec3(0.075, 0.06, 0.07)), 0.05);
	d = smin(d, nose, 0.07);
	d = smax(d, -sdEllipsoid(q - vec3(0.065, -0.505, 0.93), vec3(0.032, 0.018, 0.04)), 0.02);
	// Lips and the line between them; the philtrum's groove.
	float ul = sdEllipsoid(p - vec3(0.0, -0.635, 0.865), vec3(0.21, 0.055, 0.075));
	float ll = sdEllipsoid(p - vec3(0.0, -0.755, 0.835), vec3(0.17, 0.06, 0.075));
	d = smin(d, smin(ul, ll, 0.03), 0.07);
	d = smax(d, -sdBox(p - vec3(0.0, -0.695, 0.93), vec3(0.19, 0.005, 0.1)), 0.015);
	d = smax(d, -sdEllipsoid(p - vec3(0.0, -0.56, 0.93), vec3(0.025, 0.06, 0.02)), 0.02);
	// Neck.
	d = smin(d, sdCone2(p, vec3(0.0, -0.55, -0.28), vec3(0.0, -2.5, -0.42), 0.44, 0.52), 0.26);
	return d;
}

// The face's smooth hull (no hollows, nose or lips): what cloth worn over the face drapes on.
float sdFaceHull(vec3 p) {
	vec3 q = vec3(abs(p.x), p.y, p.z);
	float d = sdEllipsoid(p - vec3(0.0, 0.30, -0.10), vec3(0.74, 0.88, 0.97));
	d = smin(d, sdEllipsoid(p - vec3(0.0, -0.26, 0.20), vec3(0.62, 0.84, 0.72)), 0.25);
	d = smin(d, sdCone2(q, vec3(0.50, -0.40, -0.24), vec3(0.19, -0.98, 0.42), 0.17, 0.13), 0.3);
	d = smin(d, sdEllipsoid(p - vec3(0.0, -1.00, 0.47), vec3(0.22, 0.17, 0.19)), 0.2);
	d = smin(d, sdEllipsoid(q - vec3(0.43, -0.10, 0.50), vec3(0.23, 0.14, 0.20)), 0.2);
	d = smin(d, sdCone2(p, vec3(0.0, -0.55, -0.28), vec3(0.0, -2.5, -0.42), 0.44, 0.52), 0.26);
	return d;
}

float sdEars(vec3 p) {
	vec3 q = vec3(abs(p.x), p.y, p.z);
	vec3 e = rotY(-0.30) * (q - vec3(0.73, -0.08, -0.10));
	float ear = sdEllipsoid(e, vec3(0.07, 0.26, 0.16));
	return smax(ear, -sdEllipsoid(e - vec3(0.055, 0.0, 0.02), vec3(0.035, 0.17, 0.10)), 0.04);
}

vec3 eyeCentre(float side) { return vec3(0.29 * side, 0.02, 0.655); }

float sdEyes(vec3 p) {
	vec3 q = vec3(abs(p.x), p.y, p.z);
	return sdSphere(q - vec3(0.29, 0.02, 0.655), 0.122);
}

// Lids: a shell round each eyeball with the slit cut out, sloped into a glare.
float sdLids(vec3 p) {
	vec3 q = vec3(abs(p.x), p.y, p.z);
	vec3 e = q - vec3(0.29, 0.02, 0.655);
	float shell = sdSphere(e, 0.142);
	float up = EYE_UP + EYE_ANGRY * e.x;
	float lo = EYE_LO + 0.05 * e.x - 0.25 * e.x * e.x;
	float slit = max(e.y - up, lo - e.y);
	slit = max(slit, 0.02 - e.z);
	return smax(shell, -slit, 0.012);
}

// Hat space: origin at the band's bottom centre, +y up the crown.
vec3 toHat(vec3 h) { return rotX(-HAT_TIP) * (h - HAT_POS); }

float sdCrown(vec3 h) {
	float ty = clamp(h.y / 1.08, 0.0, 1.0);
	vec2 r = vec2(0.82, 1.00) * (1.0 - 0.13 * ty * ty);
	// teardrop plan: narrower at the front
	r.x *= 1.0 - 0.10 * clamp(h.z, 0.0, 1.0) * ty;
	float ell = (length(h.xz / r) - 1.0) * min(r.x, r.y);
	float c = rinter(ell, max(-h.y - 0.1, h.y - 1.08 + 0.10 * abs(h.x)), 0.24);
	// Cattleman crease: a long dent down the top and two pinches at the front.
	c = smax(c, -sdEllipsoid(h - vec3(0.0, 1.10, -0.06), vec3(0.18, 0.28, 0.80)), 0.20);
	c = smax(c, -sdEllipsoid(vec3(abs(h.x), h.y, h.z) - vec3(0.76, 0.86, 0.50), vec3(0.10, 0.26, 0.24)), 0.18);
	return c;
}

float brimHeight(vec2 xz) {
	float ax = abs(xz.x);
	return 0.42 * pow(clamp((ax - 0.70) / 1.0, 0.0, 1.0), 1.6)
		- 0.10 * pow(clamp((xz.y - 0.85) / 0.85, 0.0, 1.0), 2.0)
		- 0.05 * pow(clamp((-xz.y - 0.85) / 0.85, 0.0, 1.0), 2.0);
}

float sdBrim(vec3 h) {
	float y = brimHeight(h.xz);
	float sheet = abs(h.y - y) / 1.25 - 0.032;
	float radial = (length(h.xz / vec2(1.74, 1.70)) - 1.0) * 1.7;
	return rinter(sheet, radial, 0.028);
}

// Returns (distance, material) for the hat: felt, band, feather.
vec2 sdHat(vec3 hh) {
	vec3 h = toHat(hh);
	float b = length(h - vec3(0.0, 0.5, 0.0)) - 2.2;
	if (b > 0.3) return vec2(b, M_FELT);
	float crown = sdCrown(h);
	float felt = min(crown, sdBrim(h));
	vec2 r = vec2(felt, M_FELT);
	float band = max(crown - 0.022, abs(h.y - 0.14) - 0.115);
	r = opU(r, vec2(band, M_LEATHER));
	// A crow's feather tucked in the band on the left: Hob's, a gift or a mark.
	vec3 f = h - vec3(0.80, 0.42, -0.30);
	f = rotZ(0.55) * rotY(0.55) * f;
	float fl = f.y;
	float w = 0.10 * sin(clamp((fl + 0.55) / 1.1, 0.0, 1.0) * PI) + 0.004;
	float vane = max(max(abs(f.x) - 0.008, abs(f.z) - w), abs(fl) - 0.55);
	vane += 0.006 * sin(fl * 90.0 + f.z * 20.0);
	r = opU(r, vec2(vane * 0.8, M_FEATHER));
	return r;
}

// Hair: a shaggy crop under the hat, over the ears, down to the collar (HAIR_LONG lengthens it),
// with a couple of locks out from under the brim.
float sdHair(vec3 p) {
	vec3 q = vec3(abs(p.x), p.y, p.z);
	float cran = sdEllipsoid(p - vec3(0.0, 0.30, -0.12), vec3(0.75, 0.88, 0.97));
	// Strands: noise stretched down the fall of the hair, and broader clumps.
	vec3 sp = vec3(p.x * 16.0, p.y * 2.2 + p.z * 1.5, p.z * 16.0);
	float ha = atan(p.x, -p.z);
	float clump = 0.05 * abs(sin(ha * 7.0 + fbm(p * 2.0) * 3.0));
	float strands = 0.035 * vnoise(sp) + 0.06 * fbm(p * vec3(5.0, 2.0, 5.0)) - clump;
	float cap = cran - 0.10 - strands;
	// The hair hangs: extend the cap downward behind and over the ears.
	float hang = sdEllipsoid(p - vec3(0.0, -0.20 - 0.25 * HAIR_LONG, -0.22), vec3(0.86, 0.62 + 0.2 * HAIR_LONG, 0.88)) - strands;
	cap = smin(cap, hang, 0.25);
	// Bottom edge: high at the temples (sideburns), low at the nape; ragged.
	float zf = clamp((p.z + 0.15) / 0.75, 0.0, 1.0);
	float bottom = mix(-0.62 - 0.35 * HAIR_LONG, 0.05, zf * zf) + 0.16 * vnoise(vec3(p.x * 11.0, 0.0, p.z * 11.0)) + 0.12 * abs(sin(ha * 7.0 + 1.3));
	cap = smax(cap, (bottom - p.y) * 0.8, 0.06);
	// Clear of the face: nothing in front of the cheeks and forehead.
	cap = smax(cap, (p.z - 0.42 + 0.25 * max(0.4 - q.x, 0.0)) * 0.8, 0.08);
	// A couple of locks fall out from under the brim over the left brow.
	float lock = sdCone2(p, vec3(0.32, 0.55, 0.62), vec3(0.46, 0.08, 0.86), 0.06, 0.012);
	lock = smin(lock, sdCone2(p, vec3(0.22, 0.56, 0.68), vec3(0.27, 0.22, 0.88), 0.045, 0.01), 0.03);
	return min(cap, lock + 0.01 * vnoise(p * 30.0));
}
