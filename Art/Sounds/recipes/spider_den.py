"""The Sink's spider dens (2026-10-31, Docs/Archive/Work/2026-10-31-SinkDens/CONTRACT.md): the floor's spiders crawl out of
burrows in the pit walls when the player reaches the floor (ASpiderCreature::BeginDenEmerge plays it at the mouth as the
spider starts out; the cue's name is in Audio/LooterSoundCues.h). On the brown spider's own materials (creatures.py:
legs skittering, the stridulating rasp, spiracle hisses) and spider_ambush.py's grit:

- Creature.Spider.Den: something coming up the burrow: claws scrabbling on rock inside a hollow (close, muffled, a
  little boxy, the throat's low room tone under it), louder and brighter as it nears the mouth; its front claws catching
  the lip outside; grit and a few pebbles trickling out and pattering down the wall; a low breathy hiss as it comes out.
  Up to six play within about 2.5 s (the dens stagger their spiders 0.45 s apart), so the takes differ in length and
  rhythm and keep the 2-5 kHz band down: the hollow muffles the inside, the hiss sits low.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, modal as M, granular as G, reverb as R, voice as V
from recipes import cue
from recipes import kit
from recipes import creatures as C


def _hollow(x, r, size):
    """Inside the burrow: a short, close room with the boxy resonances of a rock throat some 2 m deep, and muffled
    (the wall between it and the ear)."""
    ir = R.room(child(r, 'room'), t60=0.3, size=0.45, damp=0.3)
    wet = F.convolve(F.filt(x, F.hp(120.0), extend=False), ir)
    wet = F.filt(wet, F.peak(240.0 * size, 5.0, 3.0), F.peak(530.0 * size, 4.0, 3.5), F.peak(900.0 * size, 2.0, 3.0),
                 extend=False)
    return layers((x, 0.0, -5.0), kit.set_level(wet, x, 0.0, 0.03))


def _claws(dur, r, steps, gap):
    """Claws scrabbling on rock: bursts of quick ticks (a gait cycle each, eight feet, uneven), a scrape of claw
    dragged in some, the body's dull knock against the rock now and then."""
    out = np.zeros(ns(dur) + ns(0.1))
    t = 0.0
    i = 0
    while t < dur and i < steps:
        burst = C._skitter(child(r, 'burst', i), 4 + int(r.integers(0, 4)), 0.0, 0.022)
        out = layers(out, (burst, t, r.uniform(-5.0, 0.0)))
        if r.random() < 0.55:
            drag = kit.grit_burst(r.uniform(0.05, 0.11), child(r, 'drag', i), 2600.0, 700.0, 3600.0, 0.05)
            out = layers(out, (drag, t + r.uniform(0.0, 0.04), -9.0))
        if r.random() < 0.3:
            knock = kit.thunk(r.uniform(260.0, 380.0), child(r, 'knock', i), 0.04, 0.0006, 5, 0.1)
            out = layers(out, (knock, t + r.uniform(0.0, 0.05), -8.0))
        t += gap * r.uniform(0.7, 1.3)
        i += 1
    return normalize(out[:ns(dur) + ns(0.1)], 0.0)


def _pebble(r, bounces, gap, lo=900.0, hi=3400.0):
    """A pebble knocked out of the mouth, bouncing down the wall: a stone tick per strike, each a little fainter and
    darker as it falls away, the gaps lengthening as it gains speed."""
    out = np.zeros(1)
    t = 0.0
    for k in range(bounces):
        m = M.parts(lo * r.uniform(0.9, 1.1), hi, child(r, 'stone', k), 4, 0.016, 0.007)
        tick = normalize(M.strike(m, M.hammer(0.00007), 0.04))
        tick = F.filt(tick, F.lp(3200.0 - 450.0 * k), extend=False)
        out = layers(out, (tick, t, -3.5 * k + r.uniform(-2.0, 0.0)))
        t += gap * (1.0 + 0.25 * k) * r.uniform(0.8, 1.2)
    return out


@cue('Creature.Spider.Den', variations=4, att='Creature', jitter=0.06, conc=6, level=-12.0)
def spider_den(v, r):
    # How long it's inside, and how big the throat sounds: each take its own, so six in a row don't line up.
    inside = (1.05, 1.3, 1.2, 1.45)[v] * jitter(r, 1.0, 0.05)
    size = (1.0, 0.92, 1.08, 0.96)[v]
    out_at = inside
    total = inside + 0.75

    # Claws coming up the throat, louder and brighter as it nears the mouth, through the hollow.
    claws = _claws(inside + 0.05, child(r, 'claws'), 40, 0.15 + 0.02 * v)
    n = claws.size
    # Far down the throat at first (faint), then up to the mouth.
    approach = E.bp([(0.0, 0.06), (0.5 * inside, 0.3), (0.85 * inside, 0.8), (inside, 1.0), (n / SR, 1.0)], n=n,
                    curve='cos')
    cutoff = np.interp(times(n), [0.0, inside], [1300.0, 3000.0])
    claws = F.sweep(claws * approach, 'lp', cutoff, 0.7)
    tone = N.brown(ns(inside + 0.2), child(r, 'tone'))
    tone = F.filt(tone, F.lpn(380.0, 2), F.hp(60.0), extend=False)
    fade = E.bp([(0.0, 0.0), (0.12, 1.0), (inside, 0.8), (inside + 0.2, 0.0)], n=tone.size, curve='cos')
    tone = normalize(tone) * fade
    within = _hollow(layers((normalize(claws), 0.0, 0.0), (tone, 0.0, -27.0)), child(r, 'hollow'), size)

    # Out at the mouth: its front claws catching the lip (dry and close), a short scrape of its body over the sill.
    grip = C._skitter(child(r, 'grip'), 3 + int(r.integers(0, 2)), 0.0, 0.05)
    sill = _shape(normalize(N.band(ns(0.35), child(r, 'sill'), 500.0, 2200.0)), 0.08, 0.22)

    # Grit trickling out and pattering down the wall, and a few pebbles bouncing down it.
    grit = kit.grit_burst(0.9 + 0.1 * v, child(r, 'grit'), 1400.0, 600.0, 3600.0, 0.55, 0.06)
    grit = F.filt(grit, F.lp(3000.0), extend=False)
    stones = np.zeros(1)
    for i in range(2 + v % 2):
        p = _pebble(child(r, 'pebble', i), 2 + int(r.integers(0, 3)), r.uniform(0.06, 0.09))
        stones = layers(stones, (p, out_at - 0.15 + r.uniform(0.0, 0.25), r.uniform(-4.0, 0.0)))

    # The low breathy hiss as it comes out, and its rasp worked once, low and slow, under it.
    hn = ns(0.75)
    hiss = V.hiss(hn, child(r, 'hiss'), (650.0, 1250.0, 2100.0), (2.0, 2.2, 2.5), (1.0, 0.8, 0.35), 260.0)
    hiss = normalize(hiss * kit.turbulence(hn, child(r, 'breath'), 90.0, 0.35)) * E.swell(0.75, 0.3, 0.45)[:hn]
    rasp = C.rasp(0.4, child(r, 'rasp'), 20.0 * jitter(r, 1.0, 0.1), 28.0, (450.0, 900.0, 1500.0), 3.0)
    rasp = _shape(normalize(rasp), 0.1, 0.25)

    x = layers((within, 0.0, 0.0), (grip, out_at - 0.06, -4.0), (sill, out_at - 0.02, -15.0),
               (grit, out_at - 0.3, -11.0), (normalize(stones), 0.0, -13.0),
               (hiss, out_at - 0.05, -7.0), (rasp, out_at + 0.05, -15.0))
    x = x[:max(ns(total), stones.size + 4800)]          # never cut a pebble short
    x = F.filt(x, F.peak(3300.0, -5.0, 0.7), F.highshelf(5500.0, -3.0), F.hp(80.0), extend=False)
    return C._outside(x * E.end_fade(np.ones(x.size), 0.08), r, -24.0)


def _shape(x, attack, release):
    """x through an attack-release envelope its own length."""
    e = E.ar(x.size / SR, attack, release)
    if e.size < x.size:
        e = np.pad(e, (0, x.size - e.size))
    return x * e[:x.size]
