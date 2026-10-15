"""Procedural looks for characters (Cycles node trees), made to be baked into the game's texture sets by looter_bake.py.

A look is a Blender material whose tree ends in four Reroute nodes the bake reads: OUT_COLOR (base colour), OUT_ROUGH
(roughness), OUT_METAL (metallic) and OUT_NORMAL (a shading normal with the look's bump), wired into a Principled BSDF
so the look also renders as it is. Everything is driven by object-space coordinates in metres (the characters' parts
sit at the origin untransformed, so every part of one material shares one pattern), the surface normal (dust settles
on what faces up), height above the ground (dirt climbs from the boots) and Cycles' Pointiness (grime in the creases,
wear on the edges; no rays, so a bake needs one sample). A look can also read a UV map by name (hair
strands run along their ribbons) and the 'Seams' colour attribute the model writes on its cloth and leather: R is how
close a point is to an open edge (1 at the edge, 0 from 10 cm in), G how close to a sewn seam; construction() draws
hems, topstitching and seams from it.

Nothing is "smooth": every look carries crease networks (ridged noise: thin lines where a noise crosses its middle,
stretched down the body on hanging cloth), its material's grain, and wear and dirt with real contrast. No pattern is
finer than ~300 cycles/m: finer ones alias into moire at the bake's ~1000 px/m.

    mat = leather('Ellis_Boots', (0.17, 0.10, 0.065), grain=900.0, dust=0.6)

Colours are sRGB triples (0..1) as picked; the trees work in linear.
"""
import bpy

OUTS = ('OUT_COLOR', 'OUT_ROUGH', 'OUT_METAL', 'OUT_NORMAL')


def lin(c):
    return tuple(((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in c)


class G:
    """A small node-graph writer: every method returns an output socket."""

    def __init__(self, mat):
        self.nt = mat.node_tree
        self.n = self.nt.nodes
        self.l = self.nt.links
        self._tc = None
        self._geo = None

    def node(self, kind, **props):
        nd = self.n.new(kind)
        for k, v in props.items():
            setattr(nd, k, v)
        return nd

    def link(self, out, inp):
        if isinstance(out, (int, float)):
            inp.default_value = out
        elif isinstance(out, tuple):
            inp.default_value = (*lin(out), 1.0) if len(out) == 3 else out
        else:
            self.l.new(out, inp)

    # --- inputs
    def obj(self):
        if self._tc is None:
            self._tc = self.node('ShaderNodeTexCoord')
        return self._tc.outputs['Object']

    def geo(self):
        if self._geo is None:
            self._geo = self.node('ShaderNodeNewGeometry')
        return self._geo

    def uv(self, name):
        nd = self.node('ShaderNodeUVMap')
        nd.uv_map = name
        return nd.outputs['UV']

    def xyz(self, vec=None):
        sep = self.node('ShaderNodeSeparateXYZ')
        self.link(vec if vec is not None else self.obj(), sep.inputs[0])
        return sep.outputs['X'], sep.outputs['Y'], sep.outputs['Z']

    def up(self):
        """The surface normal's Z (1 facing the sky)."""
        return self.xyz(self.geo().outputs['Normal'])[2]

    # --- patterns
    def noise(self, scale, detail=4.0, rough=0.55, vec=None, dist=0.0, dims='3D'):
        nd = self.node('ShaderNodeTexNoise')
        nd.noise_dimensions = dims
        nd.inputs['Scale'].default_value = scale
        nd.inputs['Detail'].default_value = detail
        nd.inputs['Roughness'].default_value = rough
        nd.inputs['Distortion'].default_value = dist
        self.link(vec if vec is not None else self.obj(), nd.inputs['Vector'])
        return nd.outputs['Fac']

    def noise_col(self, scale, detail=4.0, rough=0.55, vec=None):
        nd = self.node('ShaderNodeTexNoise')
        nd.inputs['Scale'].default_value = scale
        nd.inputs['Detail'].default_value = detail
        nd.inputs['Roughness'].default_value = rough
        self.link(vec if vec is not None else self.obj(), nd.inputs['Vector'])
        return nd.outputs['Color']

    def voronoi(self, scale, vec=None, feature='F1', out='Distance', rand=1.0):
        nd = self.node('ShaderNodeTexVoronoi', feature=feature)
        nd.inputs['Scale'].default_value = scale
        nd.inputs['Randomness'].default_value = rand
        self.link(vec if vec is not None else self.obj(), nd.inputs['Vector'])
        return nd.outputs[out]

    def wave(self, scale, vec=None, kind='BANDS', direction='X', profile='SIN', distortion=0.0, detail=2.0):
        nd = self.node('ShaderNodeTexWave', wave_type=kind, wave_profile=profile)
        if kind == 'BANDS':
            nd.bands_direction = direction
        else:
            nd.rings_direction = direction
        nd.inputs['Scale'].default_value = scale
        nd.inputs['Distortion'].default_value = distortion
        nd.inputs['Detail'].default_value = detail
        self.link(vec if vec is not None else self.obj(), nd.inputs['Vector'])
        return nd.outputs['Fac']

    def ao(self, distance, inside=False, local=False, samples=16):
        nd = self.node('ShaderNodeAmbientOcclusion', inside=inside, only_local=local, samples=samples)
        nd.inputs['Distance'].default_value = distance
        return nd.outputs['AO']

    def mapping(self, scale=(1, 1, 1), vec=None, rot=(0, 0, 0)):
        nd = self.node('ShaderNodeMapping')
        nd.inputs['Scale'].default_value = scale
        nd.inputs['Rotation'].default_value = rot
        self.link(vec if vec is not None else self.obj(), nd.inputs['Vector'])
        return nd.outputs['Vector']

    # --- maths
    def math(self, op, a, b=0.0, clamp=False):
        nd = self.node('ShaderNodeMath', operation=op, use_clamp=clamp)
        self.link(a, nd.inputs[0])
        self.link(b, nd.inputs[1])
        return nd.outputs[0]

    def mul(self, a, b, clamp=False):
        return self.math('MULTIPLY', a, b, clamp)

    def add(self, a, b, clamp=False):
        return self.math('ADD', a, b, clamp)

    def ramp(self, x, lo, hi, out_lo=0.0, out_hi=1.0, smooth=True):
        """x mapped from lo..hi to out_lo..out_hi, clamped (smoothstep if smooth)."""
        nd = self.node('ShaderNodeMapRange', interpolation_type='SMOOTHSTEP' if smooth else 'LINEAR', clamp=True)
        self.link(x, nd.inputs['Value'])
        nd.inputs['From Min'].default_value = lo
        nd.inputs['From Max'].default_value = hi
        nd.inputs['To Min'].default_value = out_lo
        nd.inputs['To Max'].default_value = out_hi
        return nd.outputs['Result']

    def mix(self, fac, a, b, blend='MIX'):
        nd = self.node('ShaderNodeMix', data_type='RGBA', blend_type=blend)
        nd.clamp_result = True
        self.link(fac, nd.inputs[0])
        self.link(a, nd.inputs[6])
        self.link(b, nd.inputs[7])
        return nd.outputs[2]

    def mixf(self, fac, a, b):
        nd = self.node('ShaderNodeMix', data_type='FLOAT')
        self.link(fac, nd.inputs[0])
        self.link(a, nd.inputs[2])
        self.link(b, nd.inputs[3])
        return nd.outputs[0]

    def bump(self, height, strength, dist=0.001, normal=None):
        nd = self.node('ShaderNodeBump')
        nd.inputs['Strength'].default_value = strength
        nd.inputs['Distance'].default_value = dist
        self.link(height, nd.inputs['Height'])
        if normal is not None:
            self.link(normal, nd.inputs['Normal'])
        return nd.outputs['Normal']

    # --- the outputs
    def finish(self, color, rough, metal=0.0, normal=None, emit=None, sheen=0.0, sss=0.0):
        outs = {}
        for name, sock in zip(OUTS, (color, rough, metal, normal)):
            r = self.node('NodeReroute')
            r.name = r.label = name
            if sock is None:
                sock = self.geo().outputs['Normal']
            if isinstance(sock, (int, float)):
                v = self.node('ShaderNodeValue')
                v.outputs[0].default_value = sock
                sock = v.outputs[0]
            elif isinstance(sock, tuple):
                c = self.node('ShaderNodeRGB')
                c.outputs[0].default_value = (*lin(sock), 1.0)
                sock = c.outputs[0]
            self.l.new(sock, r.inputs[0])
            outs[name] = r.outputs[0]
        b = self.node('ShaderNodeBsdfPrincipled')
        self.l.new(outs['OUT_COLOR'], b.inputs['Base Color'])
        self.l.new(outs['OUT_ROUGH'], b.inputs['Roughness'])
        self.l.new(outs['OUT_METAL'], b.inputs['Metallic'])
        self.l.new(outs['OUT_NORMAL'], b.inputs['Normal'])
        b.inputs['Sheen Weight'].default_value = sheen
        if sss:
            b.inputs['Subsurface Weight'].default_value = sss
            b.inputs['Subsurface Radius'].default_value = (1.0, 0.45, 0.35)
            b.inputs['Subsurface Scale'].default_value = 0.006
        if emit is not None:
            b.inputs['Emission Color'].default_value = (*lin(emit[0]), 1.0)
            b.inputs['Emission Strength'].default_value = emit[1]
        out = self.node('ShaderNodeOutputMaterial')
        self.l.new(b.outputs[0], out.inputs['Surface'])
        return b


def new_material(name, look):
    mat = bpy.data.materials.get(name)
    if mat is not None:
        bpy.data.materials.remove(mat)
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.node_tree.nodes.clear()
    mat['Look'] = look
    return mat, G(mat)


# ------------------------------------------------------------------------------------------------- shared touches

def dust(g, color, amount, height=0.6, up_bias=0.5):
    """(factor socket) dust: settles on upward faces, climbs from the ground, broken up."""
    z = g.xyz()[2]
    climb = g.ramp(z, height, 0.0, 0.0, 1.0)
    upf = g.ramp(g.up(), 0.1, 0.9, 0.0, up_bias)
    patchy = g.ramp(g.noise(9.0, 6.0, 0.62), 0.35, 0.7)
    f = g.mul(g.add(climb, upf), patchy)
    return g.mul(f, amount, clamp=True)


def grime(g, amount, distance=0.04):
    """(factor socket) dirt in the creases: where the surface curves in (Cycles' Pointiness below a half), sharpened.
    No rays: the looks are baked from the detailed model onto the game mesh, and occlusion is its own map (the ORM's
    R). distance is kept for the callers; deeper creases simply read darker."""
    cavity = g.ramp(g.geo().outputs['Pointiness'], 0.497, 0.44)
    brk = g.ramp(g.noise(30.0, 4.0, 0.6), 0.25, 0.6)
    return g.mul(g.mul(cavity, brk), amount, clamp=True)


def wear(g, amount, edge=0.56):
    """(factor socket) wear on convex edges and fold crests, from Cycles' pointiness (the mesh's own curvature, so
    it costs no rays), broken up."""
    convex = g.ramp(g.geo().outputs['Pointiness'], 0.505, edge)
    brk = g.noise(60.0, 4.0, 0.6)
    return g.mul(g.mul(convex, g.ramp(brk, 0.35, 0.65)), amount, clamp=True)


def ridged(g, scale, stretch=None, width=0.07, detail=3.0, rough=0.55, dist=0.0, rot=(0.0, 0.0, 0.0)):
    """(socket 0..1) a network of thin crease lines: 1 where a noise crosses its middle, falling to 0 at width.
    stretch (x, y, z) scales the coordinates first: (1, 1, 0.25) makes lines four times longer up and down."""
    v = g.mapping(stretch or (1.0, 1.0, 1.0), rot=rot)
    n = g.noise(scale, detail, rough, vec=v, dist=dist)
    d = g.math('ABSOLUTE', g.math('SUBTRACT', n, 0.5))
    return g.ramp(d, width * 0.5, 0.0)


def dashes(g, period=0.007):
    """(socket 0..1) dashes along any line on the surface: the sum of three sines of x, y and z, which flips sign
    every half period whichever way a line runs."""
    x, y, z = g.xyz()
    k = 6.2832 / period
    s = g.add(g.add(g.math('SINE', g.mul(x, k)), g.math('SINE', g.mul(y, k * 1.13))), g.math('SINE', g.mul(z, k * 0.91)))
    return g.ramp(s, -0.25, 0.35)


def construction(g, rows=(0.007, 0.014), width=0.0024, hem=0.03, seam_rows=(0.006,), period=0.007):
    """Sewing from the 'Seams' attribute: rows of topstitching parallel to every open edge, a turned hem band along it
    (a ridge where the hem ends), and at sewn seams a pressed groove with a row of stitches either side. Returns
    (stitches, height): stitches 0..1 (thread showing), height to add to the bump."""
    a = g.node('ShaderNodeAttribute', attribute_name='Seams')
    sep = g.node('ShaderNodeSeparateColor')
    g.link(a.outputs['Color'], sep.inputs[0])
    edge = g.mul(g.math('SUBTRACT', 1.0, sep.outputs[0]), 0.1)      # metres from the edge (0.1: far, or no attribute)
    seam = g.mul(g.math('SUBTRACT', 1.0, sep.outputs[1]), 0.1)
    line = None
    for d in rows:
        l_ = g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', edge, d)), width * 0.5, 0.0)
        line = l_ if line is None else g.math('MAXIMUM', line, l_)
    for d in seam_rows:
        l_ = g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', seam, d)), width * 0.5, 0.0)
        line = g.math('MAXIMUM', line, l_)
    stitches = g.mul(line, dashes(g, period))
    band = g.ramp(edge, hem + 0.003, hem - 0.002)                    # the turned hem: two layers of cloth
    groove = g.ramp(seam, 0.003, 0.0)
    height = g.add(g.mul(band, 0.6), g.add(g.mul(stitches, 0.35), g.mul(groove, -1.0)))
    return stitches, height


def stains(g, amount, scale=6.0):
    """(factor socket) old water stains: soft rings where water dried, here and there."""
    vor = g.voronoi(scale, out='Distance')
    ring = g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', vor, 0.32)), 0.05, 0.0)
    where = g.ramp(g.noise(2.5, 3.0, 0.5), 0.55, 0.7)
    return g.mul(g.mul(ring, where), amount, clamp=True)


def splatter(g, amount, top=0.5, scale=40.0):
    """(factor socket) mud flecks: blobs thickening toward the ground, up to top metres."""
    z = g.xyz()[2]
    low = g.ramp(z, top, 0.02)
    flecks = g.ramp(g.noise(scale, 6.0, 0.75, dist=0.6), 0.62, 0.70)
    return g.mul(g.mul(flecks, low), amount, clamp=True)


# ----------------------------------------------------------------------------------------------------------- looks

def oilskin(name, dark, light, dust_amt=0.7, hem=0.36, thread=(0.52, 0.42, 0.30)):
    """Waxed cotton duck, old and dusty: a dark warm brown blotched lighter, long streaks where the rain ran, wax worn
    chalky on the fold crests and edges, grime in the folds, water stains, mud flecks up from the hem, dust on the
    shoulders; vertical drape creases and crumpled ones, a fine grain; topstitched hems and edges, sewn seams."""
    mat, g = new_material(name, 'oilskin')
    big = g.noise(2.6, 5.0, 0.62)
    base = g.mix(g.ramp(big, 0.3, 0.75), dark, light)
    streak = g.noise(1.0, 3.0, 0.5, vec=g.mapping((5.0, 5.0, 0.6)))
    base = g.mix(g.mul(g.ramp(streak, 0.45, 0.75), 0.35), base, light)
    # canvas creases: long soft ones down the drape, fewer and broader crumples (a dense fine network read as
    # cracked stone, not cloth)
    drape = ridged(g, 7.0, stretch=(1.0, 1.0, 0.22), width=0.11, dist=0.3)
    crumple = ridged(g, 12.0, stretch=(1.0, 1.0, 0.6), width=0.12, detail=2.0, dist=0.3)
    creased = g.math('MAXIMUM', g.mul(drape, 0.9), g.mul(crumple, 0.35))
    base = g.mix(g.mul(creased, 0.35), base, tuple(c * 0.6 for c in dark))       # dirt lodged in the creases
    chalk = wear(g, 0.4, 0.6)
    base = g.mix(chalk, base, (0.52, 0.45, 0.37))
    base = g.mix(grime(g, 0.7, 0.05), base, (0.035, 0.026, 0.02))
    base = g.mix(stains(g, 0.35), base, tuple(c * 0.6 for c in dark))
    z = g.xyz()[2]
    mud = g.mul(g.ramp(z, hem + 0.22, hem + 0.02), g.ramp(g.noise(14.0, 6.0, 0.7, dist=0.4), 0.48, 0.62), clamp=True)
    base = g.mix(g.mul(mud, 0.8), base, (0.16, 0.13, 0.10))
    base = g.mix(splatter(g, 0.7, top=hem + 0.25), base, (0.24, 0.20, 0.15))
    base = g.mix(dust(g, (0.55, 0.49, 0.42), dust_amt, height=hem + 0.45, up_bias=0.8), base, (0.55, 0.49, 0.42))
    stitches, sewn = construction(g)
    base = g.mix(g.mul(stitches, 0.85), base, thread)
    h = g.add(g.mul(drape, -1.0), g.mul(crumple, -0.4))
    n = g.bump(g.noise(260.0, 3.0, 0.6), 0.10, 0.0004)
    n = g.bump(g.noise(16.0, 6.0, 0.62), 0.30, 0.004, n)
    n = g.bump(h, 0.4, 0.0025, n)
    n = g.bump(sewn, 0.6, 0.0015, n)
    rough = g.mixf(chalk, 0.5, 0.85)
    rough = g.mixf(g.mul(creased, 0.5), rough, 0.75)
    rough = g.mixf(dust(g, (1, 1, 1), dust_amt, hem + 0.45, 0.8), rough, 0.9)
    g.finish(base, rough, 0.0, n, sheen=0.25)
    return mat


def felt(name, dark, light, band_z=None):
    """Fur felt, old: a sweat stain soaked up from the band, dust on the brim's top and in the crown's crease, edges
    rubbed pale, fibres and fuzz, dents and crumples where it was pushed back into shape."""
    mat, g = new_material(name, 'felt')
    base = g.mix(g.ramp(g.noise(7.0, 6.0, 0.6), 0.3, 0.75), dark, light)
    if band_z is not None:
        z = g.xyz()[2]
        tide = g.add(band_z + 0.022, g.mul(g.noise(18.0, 4.0, 0.6), 0.03))
        sweat = g.mul(g.ramp(z, band_z - 0.004, band_z + 0.004), g.ramp(g.math('SUBTRACT', z, tide), 0.004, -0.004))
        base = g.mix(g.mul(sweat, 0.75), base, tuple(c * 0.45 for c in dark))
    base = g.mix(grime(g, 0.55, 0.04), base, (0.05, 0.04, 0.035))
    base = g.mix(dust(g, (0.55, 0.50, 0.44), 0.6, height=0.001, up_bias=0.9), base, (0.55, 0.50, 0.44))
    base = g.mix(wear(g, 0.7), base, (0.42, 0.36, 0.30))
    base = g.mix(stains(g, 0.3, 14.0), base, tuple(c * 0.6 for c in dark))
    fuzz = g.noise(300.0, 4.0, 0.75)
    dents = ridged(g, 14.0, width=0.12, detail=3.0)
    base = g.mix(g.mul(g.ramp(fuzz, 0.4, 0.8), 0.18), base, light)
    n = g.bump(fuzz, 0.12, 0.0004)
    n = g.bump(g.noise(25.0, 5.0, 0.6), 0.25, 0.003, n)
    n = g.bump(g.mul(dents, -1.0), 0.35, 0.002, n)
    g.finish(base, g.mixf(fuzz, 0.85, 0.95), 0.0, n, sheen=0.7)
    return mat


def leather(name, color, grain=850.0, dust_amt=0.4, scuff=(0.6, 0.48, 0.36), dust_height=0.25, rough=(0.42, 0.7),
            thread=None, mud=0.0, boot=None):
    """Oiled leather, used hard: mottled, creases that crack and darken, edges and toes scuffed pale, a pebbled grain
    and pores, stitched edges and seams; dust and (mud > 0) mud from the ground up. boot=(toe_y, welt_z) adds a
    stitched toe cap and the welt's stitching above the sole."""
    mat, g = new_material(name, 'leather')
    mott = g.noise(22.0, 6.0, 0.6)
    darker = tuple(c * 0.55 for c in color)
    base = g.mix(g.ramp(mott, 0.2, 0.9), tuple(c * 0.7 for c in color), color)
    creases = ridged(g, 34.0, width=0.08, detail=4.0, dist=0.4)
    cracks = g.mul(creases, g.ramp(g.voronoi(160.0, feature='DISTANCE_TO_EDGE', out='Distance'), 0.06, 0.0))
    base = g.mix(g.mul(creases, 0.5), base, tuple(c * 0.4 for c in color))
    base = g.mix(g.mul(cracks, 0.4), base, tuple(min(1.0, c * 1.4 + 0.05) for c in color))
    base = g.mix(grime(g, 0.8, 0.025), base, tuple(c * 0.3 for c in color))
    sc = wear(g, 0.9)
    base = g.mix(sc, base, scuff)
    stitches, sewn = construction(g, rows=(0.005,), hem=0.0, period=0.006)
    th = thread or tuple(min(1.0, c * 1.5 + 0.1) for c in color)
    extra = None
    if boot is not None:
        x, y, z = g.xyz()
        toe_y, welt_z = boot
        cap = g.mul(g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', y, toe_y)), 0.0014, 0.0), g.ramp(z, 0.10, 0.08))
        welt = g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', z, welt_z)), 0.0014, 0.0)
        # decorative rows on the shaft, scalloped round it
        wave = g.mul(g.math('SINE', g.mul(g.add(x, y), 34.0)), 0.025)
        row = None
        for zz in (0.30, 0.316, 0.332):
            r_ = g.ramp(g.math('ABSOLUTE', g.math('SUBTRACT', z, g.add(wave, zz))), 0.0012, 0.0)
            row = r_ if row is None else g.math('MAXIMUM', row, r_)
        extra = g.mul(g.math('MAXIMUM', g.math('MAXIMUM', cap, welt), row), dashes(g, 0.006))
        stitches = g.math('MAXIMUM', stitches, extra)
    base = g.mix(g.mul(stitches, 0.85), base, th)
    base = g.mix(dust(g, (0.52, 0.46, 0.39), dust_amt, height=dust_height, up_bias=0.7), base, (0.52, 0.46, 0.39))
    if mud > 0:
        base = g.mix(splatter(g, mud, top=0.16, scale=55.0), base, (0.20, 0.16, 0.12))
    n = g.bump(g.noise(grain * 0.35, 3.0, 0.65), 0.10, 0.0003)
    n = g.bump(g.noise(grain * 0.6, 2.0, 0.5), 0.05, 0.0002, n)
    n = g.bump(g.noise(90.0, 5.0, 0.65, dist=0.3), 0.25, 0.0015, n)
    n = g.bump(g.mul(creases, -1.0), 0.45, 0.0012, n)
    n = g.bump(sewn, 0.5, 0.001, n)
    if extra is not None:
        n = g.bump(g.mul(extra, -1.0), 0.4, 0.0008, n)
    r = g.mixf(sc, rough[0], rough[1])
    r = g.mixf(g.mul(creases, 0.6), r, rough[1])
    g.finish(base, r, 0.0, n)
    return mat


def wool(name, color, dust_amt=0.5):
    """Wool trousers: drape and crumple creases, pilled and worn shiny at the knees, dusty below them."""
    mat, g = new_material(name, 'wool')
    base = g.mix(g.ramp(g.noise(6.0, 5.0, 0.6), 0.3, 0.7), tuple(c * 0.7 for c in color), color)
    drape = ridged(g, 9.0, stretch=(1.0, 1.0, 0.3), width=0.11, dist=0.3)
    crumple = ridged(g, 14.0, stretch=(1.0, 1.0, 0.6), width=0.12, detail=2.0, dist=0.3)
    base = g.mix(g.mul(g.math('MAXIMUM', drape, g.mul(crumple, 0.5)), 0.3), base, tuple(c * 0.6 for c in color))
    base = g.mix(grime(g, 0.6, 0.04), base, tuple(c * 0.35 for c in color))
    base = g.mix(wear(g, 0.6), base, tuple(min(1.0, c * 1.5) for c in color))
    base = g.mix(dust(g, (0.50, 0.45, 0.39), dust_amt, height=0.75, up_bias=0.6), base, (0.50, 0.45, 0.39))
    stitches, sewn = construction(g, rows=(0.006,))
    base = g.mix(g.mul(stitches, 0.6), base, tuple(c * 1.4 for c in color))
    n = g.bump(g.noise(300.0, 3.0, 0.6), 0.10, 0.0003)
    n = g.bump(g.noise(20.0, 5.0, 0.6), 0.25, 0.003, n)
    n = g.bump(g.add(g.mul(drape, -1.0), g.mul(crumple, -0.4)), 0.4, 0.002, n)
    n = g.bump(sewn, 0.5, 0.0012, n)
    g.finish(base, 0.88, 0.0, n, sheen=0.4)
    return mat


def brocade(name, color, sheen_color):
    """The waistcoat: smooth black wool with a faint woven figure that catches the light, a few soft creases across
    the stomach, its edges stitched. (Its creases were a dense network bumped hard and its figure raised: it read as
    crinkled black leather; the user wanted it "as cloth", 2026-10-15.)"""
    mat, g = new_material(name, 'brocade')
    fig = g.voronoi(160.0, feature='SMOOTH_F1', out='Distance')
    pat = g.ramp(fig, 0.15, 0.35)
    base = g.mix(g.mul(pat, 0.12), color, sheen_color)
    creases = ridged(g, 9.0, stretch=(1.0, 1.0, 2.5), width=0.08)
    base = g.mix(grime(g, 0.5, 0.03), base, tuple(c * 0.4 for c in color))
    base = g.mix(wear(g, 0.5), base, tuple(min(1.0, c * 2.2 + 0.05) for c in color))
    stitches, sewn = construction(g, rows=(0.005,), hem=0.0)
    base = g.mix(g.mul(stitches, 0.5), base, sheen_color)
    n = g.bump(pat, 0.04, 0.0003)
    n = g.bump(g.mul(creases, -1.0), 0.12, 0.0008, n)
    n = g.bump(sewn, 0.4, 0.001, n)
    r = g.mixf(pat, 0.65, 0.45)
    g.finish(base, r, 0.0, n, sheen=0.3)
    return mat


def linen(name, color):
    mat, g = new_material(name, 'linen')
    base = g.mix(g.ramp(g.noise(8.0, 5.0, 0.6), 0.3, 0.75), tuple(c * 0.75 for c in color), color)
    creases = ridged(g, 30.0, width=0.08, detail=4.0, dist=0.5)
    base = g.mix(g.mul(creases, 0.35), base, tuple(c * 0.6 for c in color))
    base = g.mix(grime(g, 0.75, 0.03), base, (0.30, 0.27, 0.22))
    base = g.mix(stains(g, 0.4, 10.0), base, (0.48, 0.42, 0.32))
    n = g.bump(g.noise(300.0, 3.0, 0.6), 0.08, 0.0002)
    n = g.bump(g.noise(30.0, 5.0, 0.6), 0.2, 0.002, n)
    n = g.bump(g.mul(creases, -1.0), 0.45, 0.0012, n)
    g.finish(base, 0.85, 0.0, n, sheen=0.2)
    return mat


def bandana(name, red_dark, red_light, figure=(0.88, 0.80, 0.68)):
    """Faded red cotton printed with a pale figure of rings and dots (the portrait's), dusty, creased where it's
    pulled over the nose and knotted, a rolled hem."""
    mat, g = new_material(name, 'bandana')
    vor = g.voronoi(95.0)
    ring = g.node('ShaderNodeValToRGB')
    el = ring.color_ramp.elements
    el[0].position, el[0].color = 0.0, (1, 1, 1, 1)
    el[1].position, el[1].color = 0.12, (0, 0, 0, 1)
    e = el.new(0.30)
    e.color = (0, 0, 0, 1)
    e = el.new(0.34)
    e.color = (1, 1, 1, 1)
    e = el.new(0.39)
    e.color = (0, 0, 0, 1)
    g.link(vor, ring.inputs[0])
    red = g.mix(g.noise(3.0, 4.0, 0.6), red_dark, red_light)
    pat = g.mul(ring.outputs['Color'], 0.6)
    base = g.mix(pat, red, figure)
    creases = ridged(g, 24.0, stretch=(1.0, 1.0, 0.5), width=0.1, dist=0.4)
    base = g.mix(g.mul(creases, 0.4), base, tuple(c * 0.55 for c in red_dark))
    base = g.mix(g.mul(g.noise(9.0, 5.0, 0.6), 0.25), base, (0.55, 0.45, 0.38))
    base = g.mix(grime(g, 0.5, 0.03), base, tuple(c * 0.35 for c in red_dark))
    stitches, sewn = construction(g, rows=(0.004,), hem=0.008, width=0.002, period=0.005)
    n = g.bump(g.noise(300.0, 2.0, 0.5), 0.08, 0.0002)
    n = g.bump(g.noise(40.0, 5.0, 0.6), 0.25, 0.002, n)
    n = g.bump(g.mul(creases, -1.0), 0.5, 0.0015, n)
    n = g.bump(sewn, 0.4, 0.0008, n)
    g.finish(base, 0.85, 0.0, n, sheen=0.35)
    return mat


def metal(name, dark, light, rough=(0.25, 0.55), tarnish=(0.20, 0.14, 0.07), pattern=120.0):
    """Worn metal: tarnish in the recesses, bright on the edges, fine scratches every way, small dents."""
    mat, g = new_material(name, 'metal')
    nz = g.noise(pattern, 6.0, 0.6)
    base = g.mix(nz, dark, light)
    base = g.mix(grime(g, 0.75, 0.01), base, tarnish)
    shine = wear(g, 0.7)
    base = g.mix(shine, base, tuple(min(1.0, c * 1.25) for c in light))
    scratch = g.math('MAXIMUM', ridged(g, 60.0, stretch=(1.0, 0.05, 1.0), width=0.03, rot=(0.0, 0.0, 0.6)),
                     ridged(g, 60.0, stretch=(0.05, 1.0, 1.0), width=0.03, rot=(0.4, 0.0, -0.3)))
    base = g.mix(g.mul(scratch, 0.5), base, tuple(min(1.0, c * 1.35) for c in light))
    r = g.mixf(g.ramp(nz, 0.3, 0.8), rough[0], rough[1])
    r = g.mixf(shine, r, rough[0] * 0.7)
    r = g.mixf(g.mul(scratch, 0.6), r, rough[1])
    n = g.bump(g.noise(pattern * 2.0, 4.0, 0.6), 0.06, 0.0002)
    n = g.bump(g.mul(scratch, -1.0), 0.15, 0.0002, n)
    n = g.bump(g.noise(40.0, 3.0, 0.5), 0.12, 0.001, n)
    g.finish(base, r, 1.0, n)
    return mat


def walnut(name):
    """The revolver's walnut grip: dark figured grain, oiled, worn pale where the hand holds it, checkered."""
    mat, g = new_material(name, 'walnut')
    grain = g.wave(140.0, kind='RINGS', direction='Z', distortion=3.0, detail=4.0)
    base = g.mix(g.ramp(grain, 0.2, 0.8), (0.10, 0.05, 0.025), (0.30, 0.16, 0.08))
    base = g.mix(wear(g, 0.7), base, (0.42, 0.26, 0.14))
    check = g.math('MAXIMUM', ridged(g, 140.0, stretch=(1.0, 1.0, 0.02), width=0.2, rot=(0.0, 0.0, 0.785)),
                   ridged(g, 140.0, stretch=(1.0, 1.0, 0.02), width=0.2, rot=(0.0, 0.0, -0.785)))
    n = g.bump(g.mul(check, -1.0), 0.25, 0.0004)
    g.finish(base, 0.38, 0.0, n)
    return mat


def skin(name, color, zone_attr='zones', scalp_color=(0.06, 0.05, 0.045)):
    """Grave-pale skin: the colour times painted zones (sunken dark eyes, a cold flush on nose and ears, bloodless
    lips: a colour attribute), mottled, blue veins under it, grave dirt in the creases; pores, fine wrinkles, lumps."""
    mat, g = new_material(name, 'skin')
    zones = g.node('ShaderNodeAttribute', attribute_name=zone_attr)
    mott = g.noise_col(38.0, 6.0, 0.6)
    base = g.mix(0.22, color, mott, 'OVERLAY')
    base = g.mix(1.0, base, zones.outputs['Color'], 'MULTIPLY')
    veins = g.ramp(g.noise(55.0, 8.0, 0.75, dist=0.8), 0.48, 0.52)
    base = g.mix(g.mul(veins, 0.14), base, (0.35, 0.42, 0.52))
    wrinkles = ridged(g, 60.0, stretch=(1.0, 1.0, 3.0), width=0.06, detail=3.0)
    base = g.mix(g.mul(wrinkles, 0.2), base, tuple(c * 0.6 for c in color))
    base = g.mix(grime(g, 0.5, 0.02), base, (0.22, 0.16, 0.15))
    base = g.mix(splatter(g, 0.25, top=3.0, scale=90.0), base, (0.30, 0.25, 0.21))
    pores = g.noise(400.0, 2.0, 0.5)
    n = g.bump(pores, 0.12, 0.0005)
    n = g.bump(g.mul(wrinkles, -1.0), 0.25, 0.0008, n)
    n = g.bump(g.noise(70.0, 4.0, 0.6), 0.1, 0.002, n)
    g.finish(base, g.mixf(pores, 0.42, 0.6), 0.0, n, sss=0.25)
    return mat


def scalp(name, color):
    mat, g = new_material(name, 'scalp')
    n = g.bump(g.noise(200.0, 3.0, 0.6), 0.2, 0.0005)
    g.finish(g.mix(g.noise(40.0, 4.0, 0.6), tuple(c * 0.6 for c in color), color), 0.6, 0.0, n)
    return mat


def hair(name, dark, light, uv='Strand'):
    """Dark hair on ribbons: strands run along each ribbon (its 'Strand' UV: u across, v along), separated, lighter
    at the tips, dull with grave dust."""
    mat, g = new_material(name, 'hair')
    sep = g.xyz(g.uv(uv))
    stretched = g.node('ShaderNodeCombineXYZ')
    g.link(g.mul(sep[0], 900.0), stretched.inputs[0])
    g.link(g.mul(sep[1], 12.0), stretched.inputs[1])
    strands = g.noise(1.0, 3.0, 0.6, vec=stretched.outputs[0])
    lines = g.ramp(strands, 0.3, 0.7)
    base = g.mix(lines, dark, light)
    base = g.mix(g.mul(g.ramp(sep[1], -0.06, -0.18), 0.4), base, light)      # v runs down the lock, negative
    base = g.mix(grime(g, 0.6, 0.02), base, tuple(c * 0.4 for c in dark))
    base = g.mix(g.mul(g.noise(12.0, 4.0, 0.6), 0.3), base, (0.30, 0.27, 0.24))
    n = g.bump(strands, 0.6, 0.0006)
    g.finish(base, g.mixf(lines, 0.55, 0.38), 0.0, n, sheen=0.3)
    return mat


def stacked(name, color, layer=0.006):
    """A stacked leather sole and heel: layers showing as fine lines round the edge, scuffed, earth-dark below, the
    welt's stitches along its top."""
    mat, g = new_material(name, 'stacked')
    z = g.xyz()[2]
    lines = g.ramp(g.wave(1.0 / layer, direction='Z', profile='SAW'), 0.85, 1.0)
    base = g.mix(g.mul(lines, 0.5), color, tuple(min(1.0, c * 1.8) for c in color))
    base = g.mix(g.ramp(z, 0.006, 0.0), base, (0.10, 0.085, 0.07))
    base = g.mix(wear(g, 0.7), base, (0.40, 0.33, 0.26))
    base = g.mix(g.mul(g.noise(40.0, 5.0, 0.6), 0.4), base, (0.36, 0.31, 0.26))
    base = g.mix(splatter(g, 0.8, top=0.05, scale=70.0), base, (0.22, 0.18, 0.14))
    n = g.bump(lines, 0.25, 0.0004)
    n = g.bump(g.noise(120.0, 4.0, 0.6), 0.2, 0.0006, n)
    g.finish(base, 0.75, 0.0, n)
    return mat


def glow(name, color, strength):
    """Something burning (the eyes, the lantern): plain colour with emission; the game gives it a Glow material."""
    mat, g = new_material(name, 'glow')
    g.finish(color, 0.3, 0.0, None, emit=(color, strength))
    return mat
