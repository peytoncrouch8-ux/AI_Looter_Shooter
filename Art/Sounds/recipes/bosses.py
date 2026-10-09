"""Bosses: the show every boss shares, and the two bosses of Ransom's Rest.

- The show: its bar sweeping in (2D: a big drum struck once, a low steel string, a sheet of metal shimmering up into
  an anvil's ring), a phase's turn (2D: a boom, then a dark chord and metal swelling up in reverse), the stagger at
  the boss (something brittle crunching, a body's weight, a sheet of steel wobbling), its death (2D: a boom and a
  knell under the slow-motion beat, the whole tail sinking as if the tape slowed) and its loot bursting out at the
  boss (a pop of air, brass coins and chimes, each piece's own small pop).
- The Gravemother: a giant Meadow Wolf spider, all chitin and no voice. The spiders' vocabulary scaled up: a ridged
  file scraped so fast it screeches, scraped slowly into a deep rasp, mandibles clicking, air hissing out of big
  spiracles, her shell and the ground cracking, venom bubbling in her abdomen. Slower, lower and heavier than the
  brown spiders, rung through a bigger body, with the Sink's cliffs answering.
- Abel Ransom: the keeper walking as one of the Unpaid. His voice is the ghosts' (creatures.py's choir of breathy
  formant voices, their cold ring and their haunt), in a man's lower register; his lantern is a flame (combustion's
  low roar, torn by flicker) and its wire bail creaking; the Gravewind howls off the point.

Stingers sit in a wide open-air wash; the bosses' own cues are mono and placed at the boss. The big hits go through a
soft clip and a limiter, like the guns, so they're dense enough to stand beside the rifle without a raised volume.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono, length, reverse, smoothstep
from lib import noise as N, env as E, osc as O, filters as F, dist as D, dynamics as Y, modal as M, reverb as R
from lib import stereo as S, granular as G, voice as V
from recipes import cue
from recipes import kit
from recipes import creatures as C


# --- Shared pieces --------------------------------------------------------------------------------------------------

def _drum(f0, dur, r, t60=0.8, bend=0.15, contact=0.002, air_db=-6.0, slap_db=-16.0):
    """A big drum struck with a felt beater: the head's circular modes (their pitch sagging a little as the head
    relaxes), softened by the beater's few milliseconds on the skin, the air the head shoves (a thud of dark noise,
    not a tone) and a slap of the skin itself."""
    n = ns(dur)
    head = O.membrane(f0, dur, child(r, 'head'), t60, bend, 0.04)
    head = F.convolve(head, M.hammer(contact))[:n]
    head = F.filt(head, F.lp(f0 * 10.0, 0.7), extend=False)
    air = kit.noise_thump(min(dur, 0.3), child(r, 'air'), f0 * 14.0, f0 * 1.3, 0.14, 1.0, 0.06)
    k = ns(0.03)
    slap = N.band(k, child(r, 'slap'), 300.0, 2600.0) * E.perc(0.03, 0.0004, 0.012)
    return normalize(layers((normalize(head), 0.0, 0.0), (air, 0.0, air_db), (normalize(slap), 0.0, slap_db)), 0.0)


def _pressure(r, T, dur, lp, jag=0.15):
    """A blast's push of air (Friedlander's form: a near-instant rise, a positive phase T long, a shallower suction
    after), roughened and kept to its lows: weight without a pitch."""
    n = ns(dur)
    t = times(n)
    p = (1.0 - t / T) * np.exp(-t / T)
    p = p * (1.0 + jag * N.smooth_random(n, child(r, 'jag'), 2.0 / T))
    k = max(2, ns(0.0002))
    p[:k] *= np.linspace(0.0, 1.0, k)
    return normalize(F.filt(E.end_fade(p), F.lpn(lp, 4), extend=False), 0.0)


def _rumble(dur, r, lp=300.0, t60=1.5, peak=0.12, wander=4.0):
    """The valley answering a big hit: dark noise rising over peak seconds, its level wandering like thunder's rather
    than falling smoothly like a reverb's."""
    n = ns(dur)
    t = times(n)
    x = F.filt(N.pink(n, child(r, 'rum')), F.lpn(lp, 4), F.hp(35.0, 0.7), extend=False)
    w = (0.35 + 0.65 * np.abs(N.smooth_random(n, child(r, 'wander'), wander))) ** 1.3
    rise = np.clip(t / max(peak, 1e-3), 0.0, 1.0) ** 2
    env = rise * np.exp(-6.9 * np.clip(t - peak, 0.0, None) / t60)
    return normalize(E.end_fade(x * w * env, 0.02), 0.0)


def _stage(x, r, kind='open', t60=2.0, wet_db=-14.0, width=0.3, lp=3500.0, hp=200.0):
    """A 2D sting's space: the dry sound centred with some width, in a wide, dark open-air wash (or a plate)."""
    dry = S.widen(x, child(r, 'wide'), width) if x.ndim == 1 else x
    if kind == 'plate':
        ir = R.stereo_ir(R.plate, child(r, 'ir1'), child(r, 'ir2'), t60=t60)
    else:
        ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=t60, lp=lp)
    wet = F.convolve(F.filt(mono(x), F.hp(hp), extend=False), ir)
    return layers(dry, kit.set_level(wet, dry, wet_db, 0.05))


def _sink(x, r, rel_db=-15.0, t60=1.0):
    """Where the Gravemother lives: the Sink's ring of cliffs throwing her back twice, then a rolling tail."""
    ir = R.outdoor(child(r, 'sink'), dur=max(1.0, t60 * 1.4), slaps=((0.11, -4.0), (0.26, -8.0)), slap_lp=2600.0,
                   tail_t60=t60, tail_db=-9.0, tail_start=0.03, tail_peak=0.14, tail_lp=1800.0)
    wet = F.convolve(F.filt(x, F.hpn(150.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


def _outside(x, r, rel_db=-20.0):
    ir = R.outdoor(child(r, 'ir'), dur=0.7, slaps=((0.07, -4.0), (0.17, -9.0)), tail_t60=0.5, tail_db=-11.0,
                   tail_start=0.02, tail_peak=0.08)
    wet = F.convolve(F.filt(x, F.hpn(220.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


def _master(x, clip_db=4.0, push_db=2.0, lookahead=0.002, release=0.06):
    """The big hits' last stage, as for the guns: a soft clip rounds the front off, a limiter catches the rest, so the
    sound is dense enough to sit beside the rifle without its volume raised."""
    x = D.drive(normalize(x), clip_db, 'tanh')
    return Y.limit(normalize(x, 0.0) * db2a(push_db), -1.0, lookahead, release)


def _slowing(x, start, glide, end_speed):
    """x played as if the tape slows down after start seconds, easing to end_speed over glide: everything after it
    stretches and sinks in pitch (the slow-motion beat, heard)."""
    n = length(x)
    est = int(ns(start) + (n - ns(start)) / end_speed) + ns(0.05)
    t = np.arange(est) / SR
    u = smoothstep((t - start) / max(glide, 1e-3))
    speed = 1.0 + (end_speed - 1.0) * u
    pos = np.concatenate([[0.0], np.cumsum(speed[:-1])])
    pos = pos[pos < n - 1]
    if x.ndim == 1:
        y = np.interp(pos, np.arange(n), x)
    else:
        y = np.vstack([np.interp(pos, np.arange(n), c) for c in x])
    return E.end_fade(np.ones(length(y)), 0.02) * y


def _stridulate(dur, r, rates, body, q=6.0, wobble=0.12, rough=0.25, strokes=0.0, stroke_depth=0.7, soft=2500.0,
                grit_db=-22.0):
    """A ridged file drawn across a pick, the way spiders make their sounds: a train of slips at a rate following
    rates [(t, Hz)], rung through the shell's resonances (body, Hz). Fast enough and it buzzes at the slip rate (a
    screech); slow, it rasps. rough scatters each slip's timing and strength (chitin, not a throat), and strokes > 0
    breaks it into strokes of the file, back and forth that many times a second."""
    n = ns(dur)
    rate = O.glide(rates, n=n)
    rate = rate * (1.0 + wobble * N.smooth_random(n, child(r, 'wob'), 9.0)) \
        * (1.0 + rough * 0.5 * N.smooth_random(n, child(r, 'rough'), 70.0))
    ph = O.phase(rate, n)
    edges = np.nonzero(np.diff(np.floor(ph)) > 0)[0] + 1
    pulses = np.zeros(n)
    pulses[edges] = r.uniform(1.0 - rough * 1.5, 1.0, edges.size).clip(0.15, 1.0)
    pulses = F.filt(pulses, F.lp1(soft), extend=False)
    y = np.zeros(n)
    for i, f in enumerate(body):
        fc = f * jitter(r, 1.0, 0.04)
        y += F.filt(pulses, F.bp(fc, q), F.bp(fc, q), extend=False) / (1.0 + 0.4 * i)
    y = normalize(y)
    grit = N.white(n, child(r, 'grit')) * np.abs(N.smooth_random(n, child(r, 'ga'), 60.0))
    y = y + float(db2a(grit_db)) * normalize(F.filt(grit, F.hp(body[0], 0.7), F.lp(body[-1] * 1.5, 0.7), extend=False))
    if strokes > 0:
        sph = O.phase(strokes * (1.0 + 0.15 * N.smooth_random(n, child(r, 'sk'), 3.0)), n)
        st = 0.5 - 0.5 * np.cos(2.0 * np.pi * sph)
        y = y * ((1.0 - stroke_depth) + stroke_depth * st ** 1.5)
    return normalize(y, 0.0)


def _flame(dur, r, fc_pts, flicker=24.0, depth=0.7, q=1.2, color=-3.0):
    """A flame: combustion's low, broadband roar, torn by flicker (the flame's surface lapping and tearing), through a
    loose resonance that follows fc_pts [(t, Hz)]. The caller shapes its level."""
    n = ns(dur)
    x = N.shaped(n, child(r, 'roar'), color) * kit.turbulence(n, child(r, 'lick'), flicker * 6.0, 0.6)
    fl = (1.0 - depth) + depth * np.abs(N.smooth_random(n, child(r, 'flick'), flicker)) ** 0.8
    y = F.sweep(x, 'bp', O.glide(fc_pts, n=n), q)
    return normalize(y * fl, 0.0)


def _crackle(dur, r, rate, tau, lo=1500.0, hi=6000.0, lp=5000.0):
    """Embers crackling: sparse small snaps, thinning out over tau seconds."""
    n = ns(dur)
    g = G.grit(dur, child(r, 'crackle'), rate * np.exp(-times(n) / tau), lo, hi, 3.0, 3)
    return normalize(F.filt(g, F.lp(lp, 0.7), extend=False), 0.0)


def _snaps(dur, r, rate, lo=500.0, hi=3000.0, t60=0.02, start=0.0):
    """Rock or shell splitting: separate brittle snaps at random times (rate per second: a number or a function)."""
    def grain(rr, t, i):
        m = M.parts(lo, hi, rr, 5, t60, t60 * 0.5, 0.4)
        return normalize(M.strike(m, M.hammer(0.00008), t60 * 2.0 + 0.01)) * rr.uniform(0.4, 1.0)
    return G.cloud(dur, child(r, 'snaps'), rate, grain, start=start)


def _debris(dur, r, count, start, spread, lo=1400.0, hi=5000.0):
    """Stones and clods landing after a hit: small scattered ticks, fewer and fainter."""
    def grain(rr, i):
        m = M.parts(lo * rr.uniform(0.8, 1.2), hi, rr, 4, 0.014, 0.006)
        return normalize(M.strike(m, M.hammer(0.0001), 0.035))
    return G.rattle(dur, child(r, 'debris'), count, spread, grain, start, 0.85)


def _gutter(dur, r, fc, puff_at):
    """A lantern's flame guttering out: its roar sputtering into gaps as it starves, then a last puff of air and a
    thread of smoke."""
    n = ns(dur)
    t = times(n)
    fire = _flame(dur, child(r, 'fire'), [(0.0, fc), (dur, fc * 0.6)], 18.0, 0.6, 1.4)
    s = N.smooth_random(n, child(r, 'starve'), 26.0)
    thr = np.interp(t, [0.0, puff_at], [-1.0, 0.7])
    gate = F.filt(np.clip((s - thr) * 3.0, 0.0, 1.0), F.lp1(60.0), extend=False)
    fire = fire * gate * np.clip(1.0 - (t - puff_at) / 0.03, 0.0, 1.0)
    k = ns(0.05)
    puff = N.band(k, child(r, 'puff'), 250.0, 1600.0) * E.perc(0.05, 0.004, 0.035)
    m = ns(0.35)
    smoke = F.filt(N.pink(m, child(r, 'smoke')), F.hp(2200.0, 0.7), F.lp(6000.0, 0.7), extend=False)
    smoke = smoke * E.swell(0.35, 0.04, 0.25) * kit.turbulence(m, child(r, 'st'), 40.0, 0.6)
    wick = _crackle(0.35, child(r, 'wick'), 90.0, 0.1, 1800.0, 5000.0)
    return layers((normalize(fire), 0.0, 0.0), (normalize(puff), puff_at, -4.0), (normalize(smoke), puff_at + 0.01, -20.0),
                  (wick, puff_at, -20.0))


def _sheet_wobble(f0, dur, r, rate=6.0, depth=0.05, t60=0.7):
    """A sheet of steel struck and left wobbling: its dense modes bending up and down together as the sheet flexes,
    its level swinging with them."""
    n = ns(dur)
    t = times(n)
    m = M.plate(f0, child(r, 'sheet'), 18, t60, 0.6, tilt=0.8)
    flex = np.sin(2.0 * np.pi * rate * t * (1.0 - 0.3 * t / dur) + 2.0 * np.pi * r.random())
    bend = 1.0 + depth * flex * np.exp(-t / (dur * 0.5))
    out = np.zeros(n)
    for f, d, a in zip(*m):
        if f * (1 + depth) >= 20000.0:
            continue
        out += a * np.exp(-E.LN1000 * t / d) * np.sin(2.0 * np.pi * O.phase(f * bend, n))
    out *= 0.6 + 0.4 * (0.5 + 0.5 * flex)
    k = ns(0.002)
    out[:k] *= np.linspace(0.0, 1.0, k)
    return normalize(E.end_fade(out, 0.01), 0.0)


# --- The show -------------------------------------------------------------------------------------------------------

@cue('Boss.Intro', variations=1, space='2D', cls='Effects', jitter=0.0, conc=1, level=-4.0)
def boss_intro(v, r):
    # The bar sweeps in: a big drum struck once with a low steel string under it, then a sheet of metal shimmering up
    # (bowed into its modes, brighter as it climbs, a struck sheet's ring played backwards with it) as the fill runs
    # up, crowned by an anvil's ring and a softer stroke of the drum.
    drum = _drum(60.0, 1.2, child(r, 'drum'), 0.7, 0.14, 0.0015)
    low = layers(kit.twang(kit.note('E1'), 1.3, child(r, 'l1'), 0.45, 1.0),
                 (kit.twang(kit.note('E2'), 1.3, child(r, 'l2'), 0.45, 0.9), 0.008, -4.0))
    # The crown lands 0.75 s in, on the music's phase sting's hit (MusicRules::PhaseStingHit), which opens the fight.
    rise = 0.7
    n = ns(rise)
    plate = M.plate(310.0, child(r, 'plate'), 40, 1.6, 0.35, tilt=0.35)
    bow = N.pink(n, child(r, 'bow')) * np.linspace(0.0, 1.0, n) ** 2.2
    shimmer = M.strike(plate, bow / np.sqrt(np.sum(bow * bow)), rise)[:n]
    shimmer = F.sweep(shimmer, 'lp', O.expsweep(1500.0, 10000.0, rise, 0.7, n=n), 0.8)
    shimmer = F.filt(shimmer * np.linspace(0.0, 1.0, n) ** 1.5, F.hp(600.0, 0.7), extend=False)
    struck = normalize(M.strike(M.plate(420.0, child(r, 'cym'), 36, 1.2, 0.35, tilt=0.4), M.hammer(0.00008), 1.0))
    backward = F.filt(reverse(struck[:n]), F.hp(500.0, 0.7), extend=False)
    anvil = normalize(M.strike(M.bar(495.0, child(r, 'anvil'), 1.0, 0.5, 6, 0.006, 0.6), M.hammer(0.00008), 1.0))
    anvil = F.filt(anvil, F.peak(3000.0, -4.0, 1.0), extend=False)
    drum2 = _drum(60.0, 0.7, child(r, 'drum2'), 0.45, 0.1, 0.003)
    body = layers((drum, 0.0, 0.0), (normalize(low), 0.004, -10.0))
    crown = layers((anvil, 0.05 + rise, -9.0), (drum2, 0.05 + rise, -13.0))
    shine = layers((normalize(shimmer), 0.05, -5.0), (normalize(backward), 0.05 + rise - length(backward) / SR, -10.0))
    x = layers(S.widen(body, child(r, 'wb'), 0.12), S.widen(shine, child(r, 'ws'), 0.7), S.widen(crown, child(r, 'wc'), 0.3))
    x = F.filt(x, F.lowshelf(70.0, -3.0), F.hp(32.0), extend=False)
    x = _stage(x, r, 'open', 1.5, -14.0, 0.3, 3500.0)
    x = x[..., :ns(1.9)] * E.bp([(0.0, 1.0), (1.2, 1.0), (1.9, 0.0)], n=min(length(x), ns(1.9)), curve='cos')
    return _master(x, 2.0, 0.5)


@cue('Boss.Phase', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-5.0)
def boss_phase(v, r):
    # The fight turns: a boom (a big drum, a shove of air, the valley rumbling), then a dark chord and a sheet of
    # metal swelling up in reverse and blooming out again, menacing (a tritone on low steel strings).
    boom = layers(_drum(50.0, 0.9, child(r, 'drum'), 0.55, 0.16, 0.002),
                  (_pressure(child(r, 'push'), 0.012, 0.12, 180.0), 0.0, -3.0),
                  (_rumble(0.9, child(r, 'rumble'), 260.0, 0.5, 0.08), 0.0, -12.0))
    root = ['E3', 'D3'][v]
    f0 = kit.note(root)
    chord = layers(kit.twang(f0, 1.2, child(r, 'c1'), 0.6, 1.2), (kit.twang(f0 * 2 ** (6 / 12), 1.2, child(r, 'c2'), 0.6, 1.1), 0.0, -2.0),
                   (kit.twang(f0 * 2, 1.2, child(r, 'c3'), 0.6, 1.0), 0.0, -3.0))
    sheet = normalize(M.strike(M.plate(380.0, child(r, 'plate'), 34, 1.1, 0.4, tilt=0.45), M.hammer(0.0001), 1.1))
    bloom = layers((normalize(chord), 0.0, -2.0), (sheet, 0.0, 0.0))
    ir = R.open_air(child(r, 'bir'), t60=1.4, lp=4000.0)
    bloom = normalize(R.apply(F.filt(bloom, F.hp(150.0, 0.7), extend=False), ir, -4.0))
    # The swell crests 0.75 s in, on the music's phase sting's hit (MusicRules::PhaseStingHit), and blooms out from it.
    swell_len = 0.75
    k = ns(swell_len)
    back = reverse(bloom[:k]) * np.linspace(0.0, 1.0, k) ** 0.5
    fwd = bloom[:ns(0.5)] * E.perc(0.5, 0.0, 0.3)
    turn = layers((back, 0.0, 0.0), (fwd, swell_len, -3.0))
    x = layers(S.widen(boom, child(r, 'wb'), 0.12), (S.widen(turn, child(r, 'wt'), 0.6), 0.0, -1.0))
    x = F.filt(x, F.lowshelf(70.0, -3.0), F.hp(32.0), extend=False)
    x = _stage(x, r, 'open', 1.3, -15.0)
    x = x[..., :ns(1.6)] * E.bp([(0.0, 1.0), (1.0, 1.0), (1.6, 0.0)], n=min(length(x), ns(1.6)), curve='cos')
    return _master(x, 2.0, 0.5)


@cue('Boss.Stagger', variations=3, att='Gun', cls='Effects', jitter=0.04, conc=2, level=-6.0)
def boss_stagger(v, r):
    # Its weak spot breaks: something brittle crunching in, a heavy body's weight lurching, and a sheet of steel left
    # wobbling (the daze), with the land answering.
    crunch = layers(kit.grit_burst(0.18, child(r, 'crunch'), 9000.0, 600.0, 5000.0, 0.1, 0.0005),
                    (_snaps(0.12, child(r, 'snaps'), 60.0, 700.0, 3500.0, 0.025), 0.0, -4.0))
    weight = layers(_pressure(child(r, 'push'), 0.007, 0.08, 220.0),
                    (kit.noise_thump(0.3, child(r, 'thud'), 1100.0, 70.0, 0.16, 1.0, 0.06), 0.0, -2.0))
    wobble = _sheet_wobble(240.0 * jitter(r, 1, 0.08), 1.0, child(r, 'wob'), 5.0 + r.random(), 0.06, 0.8)
    knock = kit.thunk(300.0 * jitter(r, 1, 0.08), child(r, 'knock'), 0.06, 0.0004)
    x = layers((weight, 0.0, -2.0), (crunch, 0.0, -4.0), (knock, 0.0, -7.0), (wobble, 0.01, -3.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.lowshelf(70.0, -4.0), F.hp(35.0), extend=False)
    return _master(_outside(x, r, -18.0), 2.5, 1.0)


@cue('Boss.Death', variations=1, space='2D', cls='Effects', jitter=0.0, conc=1, level=-3.0)
def boss_death(v, r):
    # It falls, slowly: a low boom (a huge drum, the air shoved, the valley rumbling long), a deep knell and a crumble
    # of debris, the whole tail slowing like a tape as time drags.
    boom = layers(_drum(44.0, 1.6, child(r, 'drum'), 0.8, 0.18, 0.0025),
                  (_pressure(child(r, 'push'), 0.014, 0.14, 160.0), 0.0, -3.0),
                  (_rumble(1.8, child(r, 'rumble'), 240.0, 1.0, 0.15, 3.0), 0.0, -14.0))
    # A deep knell over it: as the tape slows, its pitch sags, and that's what tells the ear time is dragging.
    knell = normalize(M.strike(M.bell(196.0, child(r, 'bell'), 2.4, 0.6, 0.9), M.hammer(0.0004), 1.8))
    knell = F.filt(knell, F.lp(3200.0, 0.7), extend=False)
    crumble = layers(kit.grit_burst(0.6, child(r, 'grit'), 2500.0, 400.0, 3000.0, 0.4, 0.01),
                     (_debris(1.0, child(r, 'deb'), 8, 0.1, 0.7), 0.0, -6.0))
    x = layers((boom, 0.0, 0.0), (knell, 0.01, -5.0), (crumble, 0.02, -11.0))
    x = F.filt(x, F.lowshelf(60.0, -3.0), F.hp(28.0), extend=False)
    x = _stage(x, r, 'open', 1.6, -12.0, 0.25, 2800.0)
    # The tape slows over the slow-motion beat; the tail is let go by about 2.8 s.
    x = _slowing(x, 0.12, 0.8, 0.6)
    x = x[..., :ns(2.9)] * E.bp([(0.0, 1.0), (1.5, 1.0), (2.9, 0.0)], n=min(length(x), ns(2.9)), curve='cos')
    return _master(x, 2.0, 0.5)


@cue('Boss.Loot.Burst', variations=2, att='Creature', cls='Effects', jitter=0.02, conc=1, level=-7.0)
def boss_loot_burst(v, r):
    # The loot bursts out: a pop of air, brass chimes struck together, coins and rounds spraying, a rush of air going
    # up and a sparkle of small ticks.
    pop = layers(_pressure(child(r, 'push'), 0.004, 0.05, 320.0),
                 (kit.noise_thump(0.18, child(r, 'thud'), 1600.0, 140.0, 0.09, 1.0, 0.05), 0.0, -3.0))
    notes = [['E6', 'B6', 'E7'], ['E6', 'G#6', 'B6']][v]
    bells = np.zeros(1)
    for i, nm in enumerate(notes):
        bells = layers(bells, (kit.chime(kit.note(nm), child(r, 'b', i), 0.9, 0.6, 0.00025), 0.012 * i, -2.0 * i))
    coins = kit.jingle(0.6, child(r, 'coins'), 12, 1800.0, 6500.0, 0.05, 0.35, 0.02)
    up = kit.whoosh(0.5, child(r, 'up'), 500.0, 4500.0, 1.0, 0.25, -2.0, 0.8)
    sparkle = np.zeros(1)
    sr = child(r, 'sparkle')
    for i in range(8):
        sparkle = layers(sparkle, (kit.metal_click(sr, 4500.0, 9500.0, 0.03, 0.00004, 3, -20.0), 0.05 + 0.4 * sr.random() ** 1.3,
                                   sr.uniform(-10.0, -2.0)))
    x = layers((pop, 0.0, 0.0), (normalize(bells), 0.005, -7.0), (coins, 0.01, -13.0), (up, 0.0, -12.0),
               (normalize(sparkle), 0.0, -22.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), extend=False)
    return _outside(x, r, -21.0)


@cue('Boss.Loot.Pop', variations=4, att='Creature', cls='Effects', jitter=0.03, conc=6, level=-14.0)
def boss_loot_pop(v, r):
    # One piece thrown out of the shower (the code plays it higher for rarer ones): a hollow pop, a little whoosh as
    # it flies up and a small brass glint.
    pop = C._hollow(child(r, 'pop'), 470.0 * jitter(r, 1, 0.08), 0.05, 2600.0)
    thud = kit.noise_thump(0.06, child(r, 'thud'), 900.0, 160.0, 0.03, 1.0, 0.02)
    up = kit.whoosh(0.22, child(r, 'up'), 500.0, 2600.0, 1.0, 0.35, -2.0, 0.9)
    glint = kit.brass_tick(child(r, 'glint'), 1750.0 * jitter(r, 1, 0.04), 0.035, 4, 0.0001, -14.0, -10.0, 0.1)
    x = layers((pop, 0.0, 0.0), (thud, 0.0, -6.0), (up, 0.0, -9.0), (glint, 0.012, -13.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.lp(7500.0, 0.7), extend=False)
    return _outside(x, r, -24.0)


# --- The Gravemother ------------------------------------------------------------------------------------------------

@cue('Boss.Gravemother.Roar', variations=2, att='Gun', cls='Effects', jitter=0.04, conc=1, level=-3.0)
def gravemother_roar(v, r):
    # She rears and screams, a spider's way: her file scraped so fast it screeches, scraped slowly under it into a deep
    # pulsing rasp, mandibles chattering, air roaring out of big spiracles, her carapace rumbling. Chitin, not a voice.
    dur = 1.6
    n = ns(dur)
    p = [1.0, 0.92][v]
    screech = _stridulate(dur, child(r, 'screech'), [(0.0, 150 * p), (0.14, 330 * p), (0.55, 370 * p), (1.05, 290 * p), (dur, 130 * p)],
                          (620.0, 1250.0, 2200.0, 3500.0), 5.0, 0.12, 0.35, 5.5 + v, 0.35)
    screech = screech * E.bp([(0.0, 0.0), (0.1, 1.0), (0.9, 0.85), (dur, 0.0)], n=n, curve='cos')
    rasp = _stridulate(dur, child(r, 'rasp'), [(0.0, 40.0), (0.4, 65.0), (dur, 32.0)], (210.0, 460.0, 920.0), 4.0, 0.15, 0.3,
                       11.0 + 2.0 * v, 0.65)
    rasp = rasp * E.bp([(0.0, 0.0), (0.06, 1.0), (1.1, 0.8), (dur, 0.0)], n=n, curve='cos')
    hiss = V.hiss(n, child(r, 'hiss'), (1400.0, 2500.0, 4200.0), (2.5, 2.5, 2.0), (1.0, 0.8, 0.5), 500.0)
    hiss = hiss * E.bp([(0.0, 0.0), (0.2, 1.0), (1.0, 0.6), (dur, 0.0)], n=n, curve='cos')
    chat = C.chitter(dur, child(r, 'chitter'), 14.0, 34.0, 700.0, 2600.0, 0.35, 0.6)
    chat = chat[:n] * E.bp([(0.0, 0.3), (0.3, 1.0), (dur, 0.0)], n=n, curve='cos')
    body = F.filt(N.pink(n, child(r, 'body')), F.lpn(160.0, 4), F.hp(40.0), extend=False)
    body = body * kit.turbulence(n, child(r, 'bt'), 30.0, 0.6) * E.bp([(0.0, 0.0), (0.12, 1.0), (1.2, 0.6), (dur, 0.0)], n=n, curve='cos')
    x = layers((screech, 0.0, 0.0), (rasp, 0.0, -3.0), (normalize(hiss), 0.0, -9.0), (normalize(chat), 0.0, -9.0),
               (normalize(body), 0.0, -9.0))
    x = D.drive(normalize(x), 6.0, 'tanh', bias=0.1)
    x = F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)
    return _master(_sink(x, r, -14.0, 1.2), 3.0, 2.0)


@cue('Boss.Gravemother.Slam', variations=3, att='Gun', cls='Effects', jitter=0.04, conc=2, level=-3.0)
def gravemother_slam(v, r):
    # Her forelegs come down one after the other: chitin knocking on the ground, her weight shoving the air out, the
    # earth cracking under them and clods and stones raining down.
    def leg(rr):
        push = _pressure(child(rr, 'push'), 0.008, 0.1, 200.0)
        thud = kit.noise_thump(0.35, child(rr, 'thud'), 1200.0, 60.0, 0.2, 1.0, 0.07)
        knock = kit.thunk(340.0 * jitter(rr, 1, 0.1), child(rr, 'knock'), 0.05, 0.0003, 6, 0.1)
        return layers((push, 0.0, 0.0), (thud, 0.0, -1.0), (knock, 0.0, -9.0))
    gap = 0.025 + 0.02 * r.random()
    legs = layers(leg(child(r, 'l1')), (leg(child(r, 'l2')), gap, -2.0))
    n = ns(0.5)
    crack = G.grit(0.5, child(r, 'crack'), 9000.0 * np.exp(-times(n) / 0.09), 250.0, 2600.0, 1.6, 5)
    crack = normalize(crack) * E.perc(0.5, 0.001, 0.35)
    snaps = _snaps(0.35, child(r, 'snaps'), lambda t: 50.0 * np.exp(-t / 0.12), 400.0, 2400.0, 0.03)
    dirt = N.band(ns(0.8), child(r, 'dirt'), 400.0, 3500.0) * kit.turbulence(ns(0.8), child(r, 'dt'), 90.0, 0.7)
    dirt = dirt * E.swell(0.8, 0.08, 0.55)
    debris = _debris(1.1, child(r, 'debris'), 9, 0.12, 0.8)
    x = layers((legs, 0.0, 0.0), (normalize(crack), 0.002, -10.0), (normalize(snaps), 0.004, -11.0), (normalize(dirt), 0.02, -18.0),
               (normalize(debris), 0.0, -20.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(30.0), extend=False)
    return _master(_sink(x, r, -13.0, 1.1), 5.0, 3.0)


@cue('Boss.Gravemother.Quake', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-6.0)
def gravemother_quake(v, r):
    # She rears and the ground gives under her: a first crack, then the earth groaning and creaking as the ring of
    # cracks spreads out, more and more of them, over a rumble that builds until she comes down.
    dur = 1.25
    n = ns(dur)
    t = times(n)
    rumble = F.filt(N.pink(n, child(r, 'rumble')), F.lpn(150.0, 4), F.hp(30.0), extend=False)
    rumble = rumble * (0.5 + 0.5 * np.abs(N.smooth_random(n, child(r, 'rw'), 6.0))) * np.clip(t / dur, 0.0, 1.0) ** 1.4
    first = layers(kit.grit_burst(0.12, child(r, 'first'), 9000.0, 500.0, 3500.0, 0.06, 0.0005),
                   (kit.noise_thump(0.2, child(r, 'ft'), 800.0, 70.0, 0.1, 1.0, 0.05), 0.0, -2.0))
    groans = np.zeros(1)
    for i, (at, d) in enumerate([(0.05, 0.6), (0.35, 0.7), (0.6, 0.6)]):
        g = kit.creak(d, child(r, 'groan', i), 14.0 + 6.0 * i, 30.0 + 10.0 * i, (130.0 * (1 + 0.2 * i), 310.0, 720.0), 5.0, 0.3)
        groans = layers(groans, (g, at, -2.0 * (2 - i)))
    snaps = _snaps(dur, child(r, 'snaps'), lambda tt: 4.0 + 30.0 * (tt / dur) ** 1.5, 350.0, 2600.0, 0.025, 0.04)
    grit = G.grit(dur, child(r, 'grit'), 300.0 + 3000.0 * (t / dur) ** 2, 300.0, 2500.0, 1.6, 4)
    x = layers((normalize(rumble), 0.0, -6.0), (first, 0.0, 0.0), (normalize(groans), 0.0, -5.0), (normalize(snaps), 0.0, -4.0),
               (normalize(grit), 0.0, -11.0))
    x = F.filt(x, F.lowshelf(80.0, -3.0), F.hp(30.0), extend=False)
    x = x[:n] * E.bp([(0.0, 1.0), (dur - 0.08, 1.0), (dur, 0.0)], n=n)
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False)
    return _master(_sink(x, r, -15.0, 1.0), 3.0, 1.5)


@cue('Boss.Gravemother.Gurgle', variations=2, att='Creature', cls='Effects', jitter=0.04, conc=1, level=-10.0, swell=True)
def gravemother_gurgle(v, r):
    # Venom welling up in her abdomen: thick liquid glugging, its big bubbles coming faster and climbing as it rises,
    # all muffled through her shell, and a hiss as it reaches her fangs.
    dur = 0.8
    n = ns(dur)
    t = times(n)
    glug = N.pink(n, child(r, 'glug')) * kit.turbulence(n, child(r, 'gt'), 60.0, 0.85)
    fc = O.expsweep(240.0, 520.0, dur, 1.0, n=n)
    glug = F.sweep(glug, 'bp', fc, 3.0) + 0.4 * F.sweep(glug, 'bp', fc * 2.3, 3.5)
    pulse = 0.5 - 0.5 * np.cos(2.0 * np.pi * O.phase(O.expsweep(5.0, 11.0, dur, 1.0, n=n), n))
    glug = normalize(glug) * (0.3 + 0.7 * pulse ** 1.5)
    bub = np.zeros(1)
    br = child(r, 'bub')
    for tt in N.times_poisson(dur, br, lambda u: 8.0 + 40.0 * (u / dur) ** 1.5, 0.0):
        lo, hi = 130.0 + 200.0 * tt / dur, 260.0 + 400.0 * tt / dur
        bub = layers(bub, (kit.bubble(br.uniform(lo, hi), br, 0.12), tt, br.uniform(-8.0, 0.0)))
    m = ns(0.25)
    hiss = V.hiss(m, child(r, 'hiss'), (1800.0, 3000.0, 5200.0)) * E.swell(0.25, 0.2, 0.06)
    x = layers((glug, 0.0, 0.0), (normalize(bub), 0.0, -3.0), (normalize(hiss), dur - 0.22, -20.0))
    x = x * E.bp([(0.0, 0.5), (dur * 0.9, 1.0), (dur + 0.25, 0.6)], n=length(x))
    x = F.filt(x, F.lp(1800.0, 0.7), F.hp(70.0), extend=False)
    return _outside(x, r, -24.0)


@cue('Boss.Gravemother.Spit', variations=3, att='Creature', cls='Effects', jitter=0.05, conc=2, level=-8.0)
def gravemother_spit(v, r):
    # The venom spat: a wet burst out of her fangs, a hiss of acid trailing it, the glob whooshing off, droplets.
    burst = layers(C._wet_slap(0.06, child(r, 'slap'), 250.0, 3000.0, 0.03, 0.0006),
                   (kit.noise_thump(0.1, child(r, 'push'), 1400.0, 150.0, 0.05, 1.0, 0.03), 0.0, -4.0))
    m = ns(0.45)
    hiss = V.hiss(m, child(r, 'hiss'), (2000.0, 3300.0, 6000.0), levels=(0.7, 0.6, 1.0)) * E.perc(0.45, 0.006, 0.35)
    hiss = F.sweep(hiss, 'lp', O.expsweep(9000.0, 2500.0, 0.45, n=m), 0.7)
    wet = kit.squelch(0.16, child(r, 'wet'), 1500.0 * jitter(r, 1, 0.1), 450.0, 2.5)
    fly = kit.whoosh(0.3, child(r, 'fly'), 1600.0, 500.0, 1.0, 0.2, -3.0, 0.8)
    drops = np.zeros(1)
    dr = child(r, 'drops')
    for tt in N.times_poisson(0.35, dr, lambda u: 50.0 * np.exp(-u / 0.12), 0.01):
        drops = layers(drops, (kit.bubble(dr.uniform(1200.0, 3200.0), dr, 0.12), tt, dr.uniform(-10.0, 0.0) - 30.0 * tt))
    x = layers((burst, 0.0, 0.0), (normalize(hiss), 0.0, -5.0), (wet, 0.003, -5.0), (fly, 0.01, -12.0), (normalize(drops), 0.0, -16.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(8000.0, -3.0), extend=False)
    return _outside(x, r, -21.0)


@cue('Boss.Gravemother.CrackBurn', variations=4, att='Creature', cls='Effects', jitter=0.05, conc=2, level=-14.0)
def gravemother_crack_burn(v, r):
    # The burning seam sears whoever stands on it (every half second): a short sizzle, crackling fat-bright on top, a
    # soft flare of flame under it. Kept off the ear's sharpest band, since it repeats.
    dur = 0.4
    n = ns(dur)
    t = times(n)
    sizzle = G.grit(dur, child(r, 'sizzle'), 7000.0 * np.exp(-t / 0.12) + 600.0, 4500.0, 10000.0, 2.0, 4)
    sizzle = normalize(sizzle) * E.perc(dur, 0.002, 0.3)
    hiss = F.filt(N.white(n, child(r, 'hiss')), F.hp(5500.0, 0.7), F.lp(11000.0, 0.7), extend=False)
    hiss = normalize(hiss * kit.turbulence(n, child(r, 'ht'), 120.0, 0.7)) * E.perc(dur, 0.003, 0.25)
    flare = _flame(dur, child(r, 'flare'), [(0.0, 300.0), (0.08, 700.0), (dur, 350.0)], 22.0, 0.6, 1.0)
    flare = flare * E.perc(dur, 0.006, 0.28)
    pop = kit.noise_thump(0.08, child(r, 'pop'), 700.0, 120.0, 0.04, 1.0, 0.03)
    x = layers((sizzle, 0.0, 0.0), (hiss, 0.0, -5.0), (normalize(flare), 0.0, -11.0), (pop, 0.0, -15.0))
    x = F.filt(x, F.peak(3300.0, -6.0, 0.7), F.highshelf(12000.0, -6.0), extend=False)
    return _outside(x, r, -24.0)


@cue('Boss.Gravemother.Death', variations=1, att='Gun', cls='Effects', jitter=0.0, conc=1, level=-3.0)
def gravemother_death(v, r):
    # She gives out: a long shriek of her file collapsing (screeching, then breaking into slower and slower strokes),
    # her great body slumping onto the ground with a crunch of shell, her legs curling in, and her spiracles wheezing
    # weaker and slower until they stop.
    dur = 2.4
    n = ns(dur)
    shriek = _stridulate(dur, child(r, 'shriek'), [(0.0, 300.0), (0.2, 420.0), (0.6, 330.0), (1.3, 140.0), (dur, 40.0)],
                         (680.0, 1350.0, 2400.0, 3700.0), 5.0, 0.15, 0.4)
    strokes = 0.5 - 0.5 * np.cos(2.0 * np.pi * O.phase(O.expsweep(16.0, 3.0, dur, 1.0, n=n), n))
    depth = np.clip((times(n) - 0.5) / 1.2, 0.0, 0.85)
    shriek = shriek * ((1.0 - depth) + depth * strokes ** 1.5) * E.bp([(0.0, 0.0), (0.08, 1.0), (0.6, 0.9), (dur, 0.0)], n=n, curve='cos')
    rasp = _stridulate(1.6, child(r, 'rasp'), [(0.0, 60.0), (1.6, 20.0)], (220.0, 480.0, 950.0), 4.0, 0.2, 0.3, 9.0, 0.6)
    rasp = rasp * E.perc(1.6, 0.03, 1.4)
    chat = C.chitter(1.6, child(r, 'chitter'), 26.0, 3.0, 700.0, 2600.0, 0.35, 0.6)
    chat = chat * np.linspace(1.0, 0.2, chat.size)
    t_fall = 0.85
    fall = layers(_pressure(child(r, 'push'), 0.012, 0.14, 160.0),
                  (kit.noise_thump(0.5, child(r, 'thud'), 900.0, 50.0, 0.3, 1.0, 0.08), 0.0, -1.0),
                  (kit.grit_burst(0.3, child(r, 'crunch'), 7000.0, 500.0, 4500.0, 0.18, 0.001), 0.0, -8.0),
                  (_snaps(0.3, child(r, 'snaps'), lambda t: 40.0 * np.exp(-t / 0.1), 500.0, 3000.0, 0.025), 0.0, -10.0),
                  (_debris(0.9, child(r, 'deb'), 7, 0.1, 0.6), 0.0, -18.0))
    curl = np.zeros(1)
    cr = child(r, 'curl')
    tt = 0.0
    for i in range(6):
        curl = layers(curl, (kit.thunk(cr.uniform(500.0, 900.0), child(cr, 'tap', i), 0.02, 0.0003, 4, 0.1), tt, -2.0 * i))
        tt += cr.uniform(0.09, 0.18)
    wn = ns(1.8)
    wheeze = V.hiss(wn, child(r, 'wheeze'), (1300.0, 2300.0, 3800.0), (2.0, 2.0, 2.0), (1.0, 0.8, 0.5), 500.0)
    breaths = 0.5 - 0.5 * np.cos(2.0 * np.pi * O.phase(O.expsweep(2.6, 1.1, 1.8, 1.0, n=wn), wn))
    wheeze = F.sweep(wheeze * breaths ** 2, 'lp', O.expsweep(5000.0, 1400.0, 1.8, n=wn), 0.8) * E.perc(1.8, 0.1, 1.6)
    x = layers((shriek, 0.0, 0.0), (rasp, 0.0, -6.0), (normalize(chat), 0.0, -10.0), (fall, t_fall, -1.0),
               (normalize(curl), t_fall + 0.4, -16.0), (normalize(wheeze), 1.3, -7.0))
    x = D.drive(normalize(x), 5.0, 'tanh', bias=0.08)
    x = F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)
    return _master(_sink(x, r, -14.0, 1.4), 3.0, 2.0)


# --- Abel, the keeper -----------------------------------------------------------------------------------------------

@cue('Boss.Abel.Intro', variations=1, att='Gun', cls='Effects', jitter=0.0, conc=1, level=-4.0)
def abel_intro(v, r):
    # He raises his lantern: its wire bail creaks and clinks, the flame flares up with a whoosh, and a low moan of
    # several voices at once rises out of him, a man's, cold and far-carrying.
    clink = kit.metal_click(child(r, 'clink'), 1700.0, 5000.0, 0.04, 0.0001, 5, -14.0)
    bail = kit.creak(0.3, child(r, 'bail'), 45.0, 22.0, (950.0, 2100.0, 3300.0), 9.0, 0.3)
    flare = _flame(0.9, child(r, 'flare'), [(0.0, 260.0), (0.25, 1100.0), (0.9, 420.0)], 22.0, 0.6, 1.1)
    flare = flare * E.swell(0.9, 0.25, 0.6, 1.5)
    whump = kit.noise_thump(0.3, child(r, 'whump'), 700.0, 90.0, 0.16, 1.0, 0.08)
    sparks = _crackle(0.8, child(r, 'sparks'), 120.0, 0.3)
    dur = 1.6
    moan = C._choir(child(r, 'moan'), [(0.0, 90.0), (0.4, 110.0), (1.0, 104.0), (dur, 80.0)], dur,
                    [(0.0, 'u'), (0.5, 'o'), (1.1, 'o'), (dur, 'u')], 3, 18.0, 0.05, 0.5, 0.94, (4.0, 0.25), 0.2, -2.0)
    moan = C._cold(moan, child(r, 'cold'), 31.0, 0.18) * E.swell(moan.size / SR, 0.6, 1.3, 1.6)
    x = layers((clink, 0.0, -10.0), (bail, 0.0, -16.0), (normalize(flare), 0.05, -6.0), (whump, 0.08, -10.0),
               (sparks, 0.1, -22.0), (moan, 0.15, 0.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(60.0), extend=False)
    return C._haunt(x, r, -9.0, 2.4, 3000.0)


@cue('Boss.Abel.Wail', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-4.0, swell=True)
def abel_wail(v, r):
    # His grief: a breath drawn in, then a mournful wail of many voices, a man's, rising a minor third and sinking
    # away below where it began, cold and wide.
    f = [196.0, 175.0][v]
    dur = 2.0
    wail = C._choir(child(r, 'wail'), [(0.0, f), (0.3, f * 1.19), (0.9, f * 1.12), (1.5, f * 0.94), (dur, f * 0.75)], dur,
                    [(0.0, 'o'), (0.4, 'a'), (1.0, 'a'), (1.6, 'o'), (dur, 'u')], 3, 22.0, 0.05, 0.55, 0.97, (5.0, 0.35), 0.1)
    wail = C._cold(wail, child(r, 'cold'), 33.0, 0.2) * E.swell(wail.size / SR, 0.35, 1.8, 1.6)
    draw = V.whisper(0.4, [(0.0, 'h'), (0.4, 'o')], child(r, 'draw'), 0.92) * E.swell(0.4, 0.36, 0.08, 2.0)
    air = V.whisper(dur, [(0.0, 'h'), (0.9, 'a'), (dur, 'h')], child(r, 'air'), 1.0) * E.swell(dur, 0.6, 1.4)
    x = layers((normalize(draw), 0.0, -12.0), (wail, 0.22, 0.0), (normalize(air), 0.22, -13.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(110.0), extend=False)
    return C._haunt(x, r, -7.0, 2.6)


@cue('Boss.Abel.LanternsOut', variations=1, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-9.0)
def abel_lanterns_out(v, r):
    # His three lanterns gutter out one after another: each flame sputters as it starves and dies with a puff and a
    # thread of smoke, a cold breath of wind carrying them off.
    out = np.zeros(1)
    for i, (at, fc) in enumerate([(0.0, 520.0), (0.32, 440.0), (0.62, 380.0)]):
        g = _gutter(0.36, child(r, 'gutter', i), fc, 0.27 + 0.02 * i)
        out = layers(out, (normalize(g), at, -1.5 * i))
    gust = kit.whoosh(1.0, child(r, 'gust'), 300.0, 900.0, 0.9, 0.55, -4.0)
    x = layers((normalize(out), 0.0, 0.0), (gust, 0.05, -14.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(80.0), extend=False)
    return C._haunt(x, r, -13.0, 1.6, 3000.0)


@cue('Boss.Abel.FogFlare', variations=2, att='Gun', cls='Effects', jitter=0.05, conc=1, level=-9.0, swell=True)
def abel_fog_flare(v, r):
    # Out in the fog his lantern flares: a soft catch, the flame roaring up and falling back, a few sparks, all of it
    # dark and smeared by the fog around it.
    ign = kit.noise_thump(0.15, child(r, 'ign'), 600.0, 120.0, 0.08, 1.0, 0.04)
    roar = _flame(1.2, child(r, 'roar'), [(0.0, 220.0), (0.35, 900.0 * jitter(r, 1, 0.1)), (1.2, 330.0)], 20.0, 0.55, 1.1)
    roar = roar * E.swell(1.2, 0.33, 0.8, 1.6)
    sparks = _crackle(0.9, child(r, 'sparks'), 90.0, 0.35, 1500.0, 4500.0, 3500.0)
    x = layers((ign, 0.0, -8.0), (normalize(roar), 0.0, 0.0), (sparks, 0.25, -22.0))
    x = F.filt(x, F.lp(2600.0, 0.7), F.hp(70.0), extend=False)
    return C._haunt(x, r, -5.0, 2.2, 2400.0)


@cue('Boss.Abel.WindRise', variations=1, space='2D', cls='Effects', jitter=0.0, conc=1, level=-8.0, swell=True)
def abel_wind_rise(v, r):
    # The Gravewind comes off the point: a gust building for three seconds, its rush brightening and its howl in the
    # gaps climbing, the dead whispering faintly in it, then falling away into the evening's wind.
    dur = 3.2
    n = ns(dur)
    t = times(n)
    env = E.bp([(0.0, 0.0), (0.4, 0.15), (2.5, 1.0), (dur, 0.0)], n=n, curve='cos')

    def side(rr):
        rush = N.pink(n, child(rr, 'rush'))
        rush = F.sweep(rush, 'lp', O.expsweep(380.0, 2600.0, dur * 0.8, 1.2, n=n), 0.7)
        gust = 0.7 + 0.3 * N.smooth_random(n, child(rr, 'gust'), 1.6)
        howl = np.zeros(n)
        for i, (f0, f1) in enumerate([(320.0, 600.0), (510.0, 880.0), (760.0, 1300.0)]):
            wander = 1.0 + 0.07 * N.smooth_random(n, child(rr, 'w', i), 0.9)
            fc = O.expsweep(f0, f1, dur * 0.8, 1.0, n=n) * wander
            src = N.pink(n, child(rr, 'h', i))
            hw = F.sweep(F.sweep(src, 'bp', fc, 14.0, 24), 'bp', fc, 14.0, 24)
            howl += normalize(hw) * (0.5 + 0.5 * np.abs(N.smooth_random(n, child(rr, 'ha', i), 1.2))) / (1.0 + 0.5 * i)
        return normalize(normalize(rush) * gust + 0.9 * normalize(howl))

    wind = np.vstack([side(child(r, 'left')), side(child(r, 'right'))]) * env
    souls = V.whisper(dur, [(0.0, 'h'), (1.0, 'u'), (1.8, 'o'), (2.6, 'u'), (dur, 'h')], child(r, 'souls'), 1.05)
    souls = souls * E.bp([(0.0, 0.0), (1.2, 0.3), (2.4, 1.0), (dur, 0.0)], n=n, curve='cos')
    x = layers(wind, (S.widen(normalize(souls), child(r, 'sw'), 0.5), 0.0, -22.0))
    return F.filt(x, F.hp(60.0), F.peak(3200.0, -3.0, 0.8), extend=False)


@cue('Boss.Abel.Stagger', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-6.0)
def abel_stagger(v, r):
    # Driven to a knee: a sharp ghostly gasp, then a groan choked off in his throat, flickering as if the voice can't
    # hold together.
    # A gasp catches at once (no slow swell): it answers the hit that staggered him.
    gasp = V.whisper(0.24, [(0.0, 'h'), (0.12, 'a'), (0.24, 'uh')], child(r, 'gasp'), 0.95)
    gasp = gasp * E.bp([(0.0, 0.0), (0.004, 0.35), (0.11, 1.0), (0.24, 0.0)], n=gasp.size, curve='cos')
    catch = V.voice(O.glide([(0.0, 160.0), (0.09, 135.0)], n=ns(0.09)), [(0.0, 'uh'), (0.09, 'uh')], child(r, 'catch'),
                    breath=0.6, tilt=-1.0, shift=0.95) * E.perc(0.09, 0.005, 0.07)
    f = [118.0, 108.0][v]
    dur = 0.75
    groan = C._choir(child(r, 'groan'), [(0.0, f), (0.15, f * 0.92), (dur, f * 0.7)], dur, [(0.0, 'uh'), (0.3, 'o'), (dur, 'u')],
                     2, 12.0, 0.015, 0.35, 0.95, (5.0, 0.2), 0.6, 2.0)
    m = groan.size
    chop = np.abs(N.smooth_random(m, child(r, 'chop'), 14.0))
    chop = F.filt((chop > 0.25).astype(float), F.lp1(90.0), extend=False)
    groan = C._cold(groan * (0.4 + 0.6 * chop), child(r, 'cold'), 29.0, 0.2) * E.perc(m / SR, 0.02, 0.6)
    x = layers((normalize(gasp), 0.0, -2.0), (normalize(catch), 0.17, -8.0), (groan, 0.22, 0.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(80.0), extend=False)
    return C._haunt(x, r, -10.0, 1.6, 3000.0)


@cue('Boss.Abel.Death', variations=1, att='Gun', cls='Effects', jitter=0.0, conc=1, level=-5.0)
def abel_death(v, r):
    # At rest: a long last breath out of him, a sigh of voices fading under it, the coal in his chest crackling and
    # dying away, a cold ring of his light going, and the soul's release of air rising off.
    dur = 2.8
    breath = V.whisper(dur, [(0.0, 'a'), (1.0, 'o'), (2.0, 'u'), (dur, 'h')], child(r, 'breath'), 0.95)
    breath = breath * E.bp([(0.0, 0.0), (0.004, 0.5), (0.05, 1.0), (0.8, 0.7), (dur, 0.0)], n=ns(dur), curve='cos')
    sigh = C._choir(child(r, 'sigh'), [(0.0, 122.0), (1.0, 84.0)], 1.0, [(0.0, 'a'), (1.0, 'o')], 2, 14.0, 0.03, 0.75, 0.95,
                    (4.0, 0.2), 0.15)
    sigh = C._cold(sigh, child(r, 'cold'), 31.0, 0.15) * E.perc(sigh.size / SR, 0.03, 0.9)
    coal = _crackle(3.0, child(r, 'coal'), 260.0, 0.9)
    n = ns(2.6)
    ember = F.filt(N.white(n, child(r, 'ember')), F.hp(3000.0), F.lp(8000.0), extend=False) * E.perc(2.6, 0.05, 2.2)
    ring = C._ghost_ring(kit.note('E5'), 1.6, child(r, 'ring'))
    release = kit.whoosh(1.0, child(r, 'release'), 400.0, 4500.0, 1.1, 0.4, -2.0, 0.8)
    x = layers((normalize(breath), 0.0, 0.0), (sigh, 0.0, -4.0), (coal, 0.05, -14.0), (normalize(ember), 0.05, -30.0),
               (ring, 0.3, -22.0), (release, 2.0, -15.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(80.0), extend=False)
    return C._haunt(x, r, -7.0, 3.0)
