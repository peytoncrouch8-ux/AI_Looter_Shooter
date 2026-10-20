"""Gauge's looks (Art/Models/Characters/Gauge.py), on looter_looks' node writer: Company Iron the day it left the works
(Art/Portraits/Gauge.py OPTIONS["C"]): black enamel under a clear gloss, polished brass, machined iron, the crest's red
enamel and the pressure gauge's ivory dial. Clean, but not smooth: orange peel in the enamel, the polishing rag's fine
swirls and scratches catching the light, brass brushed and darker where the polish dried in the crevices, every edge
rubbed brighter. Patterns in object space (metres), none finer than ~300/m (the bake's ~1300 px/m)."""
import math

import bpy
import numpy as np

import looter_looks as lk
from looter_looks import grime, ridged, wear


def _swirls(g, scale=45.0, width=0.012):
    """Fine polishing scratches two ways, in patches: 0..1."""
    sc = g.math('MAXIMUM', ridged(g, scale, stretch=(1.0, 0.08, 1.0), width=width, rot=(0.0, 0.0, 0.5)),
                ridged(g, scale, stretch=(0.08, 1.0, 1.0), width=width, rot=(0.3, 0.0, -0.4)))
    return g.mul(sc, g.ramp(g.noise(6.0, 3.0, 0.5), 0.42, 0.7))


def enamel(name, color=(0.016, 0.016, 0.019), rough=(0.10, 0.19)):
    """Stove enamel: deep and glossy, a faint mottle, orange peel, polishing swirls paler and rougher, edges rubbed
    glossier."""
    mat, g = lk.new_material(name, 'enamel')
    base = g.mix(g.ramp(g.noise(4.0, 3.0, 0.5), 0.3, 0.7), color, tuple(min(1.0, c * 1.4 + 0.004) for c in color))
    sc = _swirls(g)
    base = g.mix(g.mul(sc, 0.4), base, tuple(min(1.0, c * 2.4 + 0.03) for c in color))
    edge = wear(g, 0.5, 0.54)
    base = g.mix(g.mul(edge, 0.3), base, tuple(min(1.0, c * 1.8 + 0.015) for c in color))
    r = g.mixf(g.ramp(g.noise(15.0, 3.0, 0.5), 0.3, 0.75), rough[0], rough[1])
    r = g.mixf(g.mul(sc, 0.8), r, rough[1] + 0.16)
    r = g.mixf(edge, r, rough[0] * 0.8)
    n = g.bump(g.noise(250.0, 2.0, 0.5), 0.025, 0.0002)                 # orange peel
    n = g.bump(g.mul(sc, -1.0), 0.07, 0.0002, n)
    n = g.bump(g.noise(28.0, 3.0, 0.5), 0.03, 0.0008, n)                # the sheet's own slight waviness
    g.finish(base, r, 0.0, n)
    return mat


def brass(name, dark=(0.74, 0.55, 0.25), light=(0.88, 0.70, 0.36)):
    """Polished brass, new: brushed streaks, a little of the polish dried dark in the crevices, edges rubbed bright,
    fine scratches, a few small dents. (Mottled from 0.64, it read tarnished.)"""
    mat, g = lk.new_material(name, 'brass')
    nz = g.noise(18.0, 3.0, 0.5)
    base = g.mix(g.ramp(nz, 0.3, 0.7), dark, light)
    brush = g.noise(1.0, 2.0, 0.5, vec=g.mapping((260.0, 260.0, 7.0)))
    base = g.mix(g.mul(g.ramp(brush, 0.35, 0.7), 0.3), base, tuple(min(1.0, c * 1.1) for c in light))
    base = g.mix(grime(g, 0.35), base, (0.36, 0.25, 0.10))
    shine = wear(g, 0.65)
    base = g.mix(shine, base, (0.97, 0.86, 0.58))
    sc = _swirls(g, 55.0, 0.014)
    base = g.mix(g.mul(sc, 0.35), base, (0.98, 0.88, 0.62))
    r = g.mixf(g.ramp(brush, 0.3, 0.8), 0.15, 0.27)
    r = g.mixf(shine, r, 0.11)
    r = g.mixf(grime(g, 0.8), r, 0.45)
    r = g.mixf(g.mul(sc, 0.6), r, 0.32)
    n = g.bump(brush, 0.03, 0.0002)
    n = g.bump(g.mul(sc, -1.0), 0.08, 0.0002, n)
    n = g.bump(g.ramp(g.noise(22.0, 3.0, 0.5), 0.62, 0.75), 0.06, 0.0008, n)   # small dents
    g.finish(base, r, 1.0, n)
    return mat


def iron(name, dark=(0.25, 0.255, 0.27), light=(0.34, 0.345, 0.36)):
    """Machined iron (the works), new: turned and polished grey, faint turning lines, fine scratches, oil a little dark
    in the crevices, edges bright. (looter_looks.metal's cast blotches read rusted and sooty on the stacks.)"""
    mat, g = lk.new_material(name, 'iron')
    base = g.mix(g.ramp(g.noise(12.0, 3.0, 0.5), 0.3, 0.7), dark, light)
    turn = g.noise(1.0, 2.0, 0.5, vec=g.mapping((5.0, 5.0, 300.0)))       # turning lines round the pipes and rods
    base = g.mix(g.mul(g.ramp(turn, 0.4, 0.7), 0.25), base, tuple(c * 1.15 for c in light))
    base = g.mix(grime(g, 0.45), base, (0.10, 0.095, 0.09))
    shine = wear(g, 0.6)
    base = g.mix(shine, base, (0.55, 0.56, 0.58))
    sc = _swirls(g, 50.0, 0.014)
    base = g.mix(g.mul(sc, 0.3), base, (0.6, 0.6, 0.62))
    r = g.mixf(g.ramp(turn, 0.3, 0.8), 0.26, 0.38)
    r = g.mixf(shine, r, 0.2)
    r = g.mixf(g.mul(sc, 0.6), r, 0.45)
    n = g.bump(turn, 0.03, 0.0002)
    n = g.bump(g.mul(sc, -1.0), 0.08, 0.0002, n)
    g.finish(base, r, 0.95, n)
    return mat


def dark_iron(name):
    """Blacked iron: the lamp's socket, the firebox's frame, the flues."""
    return lk.metal(name, (0.035, 0.035, 0.04), (0.10, 0.10, 0.11), rough=(0.4, 0.62), tarnish=(0.02, 0.018, 0.016),
                    pattern=60.0)


DIAL_SIZE = 512


def dial_image(name, needle_deg=-20.0):
    """The pressure gauge's face (the portrait's): ivory enamel darkening to the rim, the ticks, a red arc at the top of
    the scale, the needle (a degrees from the top, clockwise positive) and its brass hub."""
    img = bpy.data.images.get(name)
    if img is not None:
        return img
    S_ = DIAL_SIZE
    v, u = (np.mgrid[0:S_, 0:S_] + 0.5) / S_ * 2 - 1           # rows bottom up: v up, u right
    r, a = np.hypot(u, v), np.degrees(np.arctan2(u, v))
    px = np.ones((S_, S_, 4))
    px[..., :3] = np.array([0.88, 0.82, 0.68]) * (1 - 0.22 * r[..., None] ** 3)
    px[..., :3] *= (1.0 - 0.03 * np.cos(u * 23 + v * 17)[..., None])     # a little unevenness in the enamel
    on = np.abs(a) <= 135
    big = (np.abs(((a + 135) % 27) - 13.5) > 12.4) & (r > 0.64)
    small = (np.abs(((a + 135) % 6.75) - 3.375) > 3.0) & (r > 0.75)
    ticks = on & (big | small) & (r < 0.86)
    ring_ = (r > 0.88) & (r < 0.915)
    px[ticks | ring_] = (0.07, 0.06, 0.05, 1)
    px[on & (a > 75) & (ticks | ((r > 0.60) & (r < 0.69)))] = (0.70, 0.07, 0.04, 1)
    na = math.radians(needle_deg)
    d = np.array([math.sin(na), math.cos(na)])
    along = u * d[0] + v * d[1]
    across = np.abs(-u * d[1] + v * d[0])
    needle = (along > -0.2) & (along < 0.84) & (across < 0.045 - 0.035 * np.clip(along, 0, 1))
    px[needle] = (0.62, 0.05, 0.03, 1)
    px[r < 0.085] = (0.55, 0.40, 0.17, 1)
    px[r < 0.035] = (0.30, 0.21, 0.08, 1)
    img = bpy.data.images.new(name, S_, S_, alpha=False)
    img.colorspace_settings.name = 'sRGB'
    img.pixels.foreach_set(px.astype(np.float32).ravel())
    return img


def dial(name, centre, radius):
    """The dial's look: the face image laid on the dial's disc (centre (x, z) world, radius), facing -Y; glossy, as
    under its glass."""
    mat, g = lk.new_material(name, 'dial')
    cx, cz = centre
    x, _y, z = g.xyz()
    v = g.node('ShaderNodeCombineXYZ')
    g.link(x, v.inputs[0])
    g.link(z, v.inputs[1])
    m = g.node('ShaderNodeMapping')
    m.inputs['Scale'].default_value = (0.5 / radius, 0.5 / radius, 1.0)
    m.inputs['Location'].default_value = (0.5 - cx * 0.5 / radius, 0.5 - cz * 0.5 / radius, 0.0)
    g.link(v.outputs[0], m.inputs['Vector'])
    tx = g.node('ShaderNodeTexImage', extension='EXTEND')
    tx.image = dial_image(name + '_Face')
    g.link(m.outputs[0], tx.inputs['Vector'])
    n = g.bump(g.noise(40.0, 2.0, 0.5), 0.01, 0.0002)
    g.finish(tx.outputs['Color'], 0.07, 0.0, n)
    return mat


def make(key, name, notes):
    """The look for a key: <Region><Kind> (HeadEnamel, BodyBrass, ...), or a glow (Lamp, Fire)."""
    if key == 'Lamp':
        return lk.glow(name, (1.0, 0.886, 0.659), 6.0)            # #FFE2A8, the portrait's warm white
    if key == 'Fire':
        return lk.glow(name, (1.0, 0.55, 0.18), 5.0)
    kind = next(key[len(r):] for r in ('Head', 'Chest', 'Belly', 'Body', 'Arms', 'Legs') if key.startswith(r))
    if kind == 'Enamel':
        return enamel(name)
    if kind == 'Black':
        return enamel(name, (0.008, 0.008, 0.009), (0.2, 0.3))
    if kind == 'Red':
        return enamel(name, (0.56, 0.045, 0.032), (0.11, 0.2))
    if kind == 'Brass':
        return brass(name)
    if kind == 'Iron':
        return iron(name)
    if kind == 'Dark':
        return dark_iron(name)
    if kind == 'Dial':
        x, z, r = notes['dial']
        return dial(name, (x, z), r + 0.002)
    raise KeyError(key)
