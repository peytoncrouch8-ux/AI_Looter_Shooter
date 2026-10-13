"""Mission givers (UMissionOfferWidget, AMissionArtifact): the offer card laid out, a mission taken or set back down, a new
offer's distant chime, and a found letter, tag or journal picked up.

The card is a paper posting on the giver's pine table, so the givers speak in paper and wood, with the interface's brass
chimes for the moments that should feel good (feedback.py's mission pings are their kin). Paper is physical: a sheet
rubbed over boards is a soft hiss of friction with fibres catching in it; a crease letting go is a cluster of crisp little
clicks; a sheet laid flat pats down on the air pushed out from under it; a rubber stamp is a wooden block's knock, a push
of weight, the rubber's slap and a tacky peel as it lifts. Heard again and again across a campaign, so the paper's crackle
stays soft in the ear's sharpest band (2-5 kHz) and its very top.

- UI.Mission.Offer (2D): the posting drawn across the table to you, settling flat, a soft low chime (an open fifth) under it.
- UI.Mission.Accept (2D, rewarding): the clerk's stamp pressed onto it, then two bright pings rising a fourth or a fifth.
- UI.Mission.Decline (2D, quiet): the paper slid back and set down: a short rub and a soft pat on the boards.
- UI.Mission.NewOffer (2D, light): a short chime heard from a little way off as a giver's ! appears.
- World.Artifact.Read (3D, near): something found picked up and read: a letter unfolded (1), a tin tag lifted out of the
  dirt and its paper turned (2), a journal lifted and opened (3); each ends on paper opening, as the card does.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, fade
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes.ui import _ui
from recipes.feedback import _pings


# --- Paper and the stamp (lostloot.py and missions.py use these too) --------------------------------------------------

# The cards' stamps slam on over 0.12 s (UMissionOfferWidget / ULostLootWidget::StampSlamSeconds): a stamp's thump lands then.
STAMP_LANDS = 0.11


def swish(r, dur=STAMP_LANDS):
    """The hand bringing a stamp down: a quick breath of air rising as it comes, already moving at the first sample, so the
    cue speaks at once and the stamp lands on the slam."""
    n = ns(dur)
    air = F.sweep(N.pink(n, child(r, 'swish')), 'bp', O.expsweep(400.0, 1500.0, dur, 1.0, n=n), 0.8)
    env = E.bp([(0.0, 0.3), (dur * 0.85, 1.0), (dur, 0.3)], n=n, curve='cos')
    return normalize(fade(air * env, 0.0015, 0.004), 0.0)


def crackle(dur, r, rate, lo=1000.0, hi=7000.0, cluster=10.0, q=1.4):
    """Paper buckling: crisp little clicks in clusters (each crease letting go), rung through a few broad bands."""
    n = ns(dur)
    clusters = np.clip(N.smooth_random(n, child(r, 'cluster'), cluster) * 1.4 + 0.2, 0.0, 1.0) ** 1.5
    g = G.grit(dur, child(r, 'grit'), rate * clusters, lo, hi, q, 5)
    return normalize(g * E.end_fade(np.ones(n), 0.004), 0.0)


def rub(dur, r, lo=300.0, hi=4000.0, grain=900.0, attack=0.3, release=0.5, rough=30.0):
    """Paper rubbing over boards: a soft hiss of friction with a fine grain of fibres catching, its level wavering with
    the hand's pressure."""
    n = ns(dur)
    hiss = N.pink(n, child(r, 'hiss'), lo=lo, hi=hi)
    fibres = G.grit(dur, child(r, 'fibres'), grain, lo * 2.0, hi, 1.0, 3)
    am = 0.6 + 0.4 * np.abs(N.smooth_random(n, child(r, 'press'), rough))
    y = normalize(hiss) + 0.6 * normalize(fibres)
    return normalize(y * am * E.ar(dur, dur * attack, dur * release, 2.0), 0.0)


def flap(r, dur=0.09, f0=650.0, f1=150.0, pat_db=-9.0):
    """A sheet landing flat: the air pushed out from under it (a soft, dark puff) and the paper's pat on the boards."""
    air = kit.noise_thump(dur, child(r, 'air'), f0, f1, dur * 0.5, 0.9, dur * 0.5, 0.002)
    pat = kit.burst(0.014, child(r, 'pat'), 0.005, lo=500.0, hi=3500.0, attack=0.0008)
    return normalize(layers((air, 0.0, 0.0), (pat, 0.0, pat_db)), 0.0)


def unfold(r, folds=2, gap=0.11):
    """A folded sheet opened: each fold letting go in a burst of crackle, the sheet swinging open on a breath of air, and
    a last soft flap as it straightens."""
    out = np.zeros(1)
    t = 0.0
    for i in range(folds):
        d = 0.06 + 0.04 * r.random()
        c = crackle(d, child(r, 'fold', i), 900.0, 900.0, 6500.0, 18.0)
        c = c * E.ar(d, 0.004, d * 0.7, 1.5)
        swing = kit.whoosh(0.12, child(r, 'swing', i), 300.0, 900.0, 0.8, 0.4, -3.0)
        out = layers(out, (normalize(c), t, -1.5 * i), (swing, t + 0.02, -12.0))
        t += gap * jitter(r, 1, 0.2)
    settle = flap(child(r, 'settle'), 0.07, 900.0, 250.0, -12.0)
    return layers(out, (settle, t, -10.0)), t


def stamp(r, f0=280.0, peel_db=-22.0):
    """A rubber stamp pressed onto paper on a pine table: the stamp's wooden block knocking, the push of the hand's
    weight, the rubber's slap on the paper, the table answering under it, and a tacky peel as it lifts."""
    block = kit.thunk(f0 * jitter(r, 1, 0.05), child(r, 'block'), 0.07, 0.0015, 6, 0.08)
    weight = kit.noise_thump(0.12, child(r, 'weight'), 900.0, 120.0, 0.05, 1.0, 0.04)
    m = ns(0.03)
    slap = normalize(N.band(m, child(r, 'slap'), 250.0, 2500.0) * E.perc(0.03, 0.0005, 0.012))
    table = kit.thunk(150.0 * jitter(r, 1, 0.05), child(r, 'table'), 0.12, 0.002, 6, 0.08)
    k = ns(0.06)
    peel = N.band(k, child(r, 'peel'), 700.0, 3500.0) * (0.3 + 0.7 * np.abs(N.smooth_random(k, child(r, 'tack'), 180.0)))
    peel = normalize(peel * E.ar(0.06, 0.02, 0.03, 1.5))
    return layers((block, 0.0, 0.0), (weight, 0.0, -3.0), (slap, 0.0, -8.0), (table, 0.002, -6.0),
                  (peel, 0.13 + 0.02 * r.random(), peel_db))


def calm(x, hp=80.0):
    """Paper's crackle eased in the ear's sharpest band and the very top: these come again and again."""
    return F.filt(x, F.peak(3300.0, -5.0, 0.8), F.highshelf(7500.0, -4.0), F.hp(hp), extend=False)


def _note_body(name, ring, db=-12.0):
    """A soft sine on a chime's note, so it sings round rather than clinks."""
    f0 = kit.note(name)
    return normalize(O.sine(f0, n=ns(ring)) * E.perc(ring, 0.004, ring * 0.7)), db


# --- The offer card ---------------------------------------------------------------------------------------------------

@cue('UI.Mission.Offer', variations=3, space='2D', cls='Interface', jitter=0.02, conc=2, level=-14.0)
def mission_offer(v, r):
    # The posting drawn across the pine table to you: paper rubbing over the boards, a crease ticking as it comes, the
    # sheet settling flat; under it a soft low chime, an open fifth, struck gently: someone has work for you.
    d = 0.24 + 0.04 * r.random()
    touch = kit.burst(0.012, child(r, 'touch'), 0.005, lo=400.0, hi=3000.0, attack=0.0008)
    drawn = rub(d, child(r, 'drawn'), 280.0, 3800.0, 700.0, 0.25, 0.55)
    tick = crackle(0.05, child(r, 'crease'), 500.0, 1000.0, 6000.0, 20.0)
    settle = flap(child(r, 'settle'), 0.08, 700.0, 160.0)
    lo, hi = [('D4', 'A4'), ('C4', 'G4'), ('E4', 'B4')][v]
    c1 = kit.chime(kit.note(lo), child(r, 'c1'), 0.8, 0.35, 0.0006, 0.5, -40.0)
    c2 = kit.chime(kit.note(hi), child(r, 'c2'), 0.7, 0.35, 0.0006, 0.5, -40.0)
    body, body_db = _note_body(lo, 0.7)
    chime = layers((c1, 0.0, 0.0), (c2, 0.012, -3.0), (body, 0.0, body_db))
    # The fingertips taking the sheet speak first, so the cue starts at once.
    x = layers((touch, 0.0, -12.0), (drawn, 0.0, -2.0), (tick, d * 0.4, -16.0), (settle, d - 0.03, -4.0),
               (normalize(chime), 0.03, -9.0))
    return _ui(calm(x), r, 0.3, -22.0, 0.25)


@cue('UI.Mission.Accept', variations=3, space='2D', cls='Interface', jitter=0.0, conc=1, level=-9.0)
def mission_accept(v, r):
    # Taken: the clerk's rubber stamp swung down (a quick breath of air, so the cue speaks at once) and landing with a
    # solid, satisfying thump as the card's ACCEPTED slams on (0.12 s in), and two bright pings rise out of it (a fourth or
    # a fifth), the second left ringing over a soft steel string an octave down.
    land = STAMP_LANDS
    s = layers((swish(r), 0.0, -14.0), (stamp(child(r, 'stamp'), 270.0 * jitter(r, 1, 0.04)), land, 0.0))
    s = _ui(calm(s, 60.0), child(r, 'room'), 0.25, -18.0, 0.3)
    a, b = [('A5', 'D6'), ('G5', 'D6'), ('B5', 'E6')][v]
    lo = ['D5', 'D5', 'E5'][v]
    pings = _pings(child(r, 'pings'), [(a, 0.0, -2.0, -0.2, 0.3), (b, 0.085, 0.0, 0.2, 0.55)],
                   [(lo, 0.085, -14.0, 0.0, 0.7)], -16.0, 0.6)
    return layers((s, 0.0, 0.0), (pings, land + 0.05, -4.0))


@cue('UI.Mission.Decline', variations=2, space='2D', cls='Interface', jitter=0.03, conc=2, level=-18.0)
def mission_decline(v, r):
    # Not now: the posting slid back across the boards and let go, a soft pat as it lies flat again. Nothing sour: the
    # work stays on the table.
    d = 0.13 + 0.03 * r.random()
    back = rub(d, child(r, 'back'), 260.0, 3200.0, 600.0, 0.2, 0.6)
    down = flap(child(r, 'down'), 0.1, 550.0, 130.0, -10.0)
    knock = kit.thunk(210.0 * jitter(r, 1, 0.06), child(r, 'knock'), 0.05, 0.003, 5, 0.06)
    x = layers((back, 0.0, -4.0), (down, d - 0.02, 0.0), (knock, d - 0.015, -12.0))
    return _ui(calm(x), r, 0.25, -24.0, 0.2)


@cue('UI.Mission.NewOffer', variations=3, space='2D', cls='Interface', jitter=0.0, conc=1, level=-17.0)
def mission_new_offer(v, r):
    # A giver nearby has work (the ! appears): a short brass chime from a little way off, a grace note and its answer a
    # fourth up, soft-struck and dark with distance, in an open-air wash.
    a, b = [('A5', 'D6'), ('G5', 'C6'), ('B5', 'E6')][v]
    c1 = kit.chime(kit.note(a), child(r, 'c1'), 0.5, 0.45, 0.0004, 0.6, -40.0)
    c2 = kit.chime(kit.note(b), child(r, 'c2'), 0.9, 0.45, 0.0004, 0.6, -40.0)
    body, body_db = _note_body(b, 0.8, -12.0)
    x = layers((c1, 0.0, -4.0), (c2, 0.075, 0.0), (body, 0.075, body_db))
    # Distance: the top rolled off and the sharpest band eased, more of the wash than of the bell.
    x = F.filt(normalize(x), F.lp(3600.0, 0.7), F.peak(3000.0, -4.0, 0.8), extend=False)
    dry = S.widen(x, child(r, 'wide'), 0.35)
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=1.6, lp=3000.0, predelay=0.03, density=0.8)
    wet = F.convolve(F.filt(x, F.hp(300.0), extend=False), ir)
    return layers(dry, kit.set_level(wet, dry, -9.0, 0.05))


# --- A found thing ----------------------------------------------------------------------------------------------------

def _outside(x, r, rel_db=-24.0):
    """A short outdoor answer round a 3D handling sound."""
    ir = R.outdoor(child(r, 'ir'), dur=0.6, slaps=((0.05, -5.0), (0.13, -10.0)), tail_t60=0.4, tail_db=-12.0,
                   tail_start=0.02, tail_peak=0.07)
    wet = F.convolve(F.filt(x, F.hpn(250.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.02))


@cue('World.Artifact.Read', variations=3, att='Near', cls='Effects', jitter=0.04, conc=2, level=-14.0)
def artifact_read(v, r):
    # Something found picked up and read. A letter: pulled free (a short rub), unfolded crease by crease. A miner's tag:
    # lifted out of the dirt (grit trickling off, the thin tin ticking against its wire), turned over, its paper label
    # opened. A journal: the leather cover lifted (its creak, a soft thump), opened, a few pages riffled to the writing.
    sleeve = kit.cloth(0.18, child(r, 'sleeve'), 300.0, 2400.0, 120.0, 0.2, 0.6)
    if v == 0:
        lift = rub(0.1, child(r, 'free'), 400.0, 4000.0, 900.0, 0.2, 0.5)
        opened, t = unfold(child(r, 'unfold'), 2, 0.12)
        x = layers((sleeve, 0.0, -14.0), (lift, 0.0, -6.0), (opened, 0.12, 0.0))
    elif v == 1:
        grit = kit.grit_burst(0.2, child(r, 'grit'), 2500.0, 700.0, 3500.0, 0.09)
        tin = normalize(M.strike(M.plate(980.0 * jitter(r, 1, 0.06), child(r, 'tin'), 12, 0.07, 0.6, tilt=0.8),
                                 M.hammer(0.0001), 0.12))
        tin = F.filt(tin, F.peak(3200.0, -6.0, 0.8), F.highshelf(6000.0, -4.0), extend=False)
        wire = kit.jingle(0.2, child(r, 'wire'), 3, 1500.0, 4500.0, 0.03, 0.1, 0.0)
        turn = normalize(M.strike(M.plate(1050.0 * jitter(r, 1, 0.06), child(r, 'turn'), 12, 0.05, 0.6, tilt=0.8),
                                  M.hammer(0.00015), 0.1))
        turn = F.filt(turn, F.peak(3200.0, -6.0, 0.8), F.highshelf(6000.0, -4.0), extend=False)
        opened, t = unfold(child(r, 'label'), 1, 0.1)
        x = layers((sleeve, 0.0, -14.0), (grit, 0.0, -12.0), (tin, 0.02, -6.0), (wire, 0.03, -14.0), (turn, 0.2, -12.0),
                   (opened, 0.26, -2.0))
    else:
        creak = kit.creak(0.16, child(r, 'leather'), 40.0, 70.0, (260.0, 620.0, 1300.0), 7.0, 0.3)
        thump = kit.noise_thump(0.09, child(r, 'cover'), 600.0, 140.0, 0.04, 0.9, 0.04)
        spine = kit.creak(0.12, child(r, 'spine'), 60.0, 30.0, (420.0, 980.0, 2100.0), 8.0, 0.2)
        riffle = np.zeros(1)
        t = 0.0
        for i in range(3 + int(r.integers(0, 2))):
            page = crackle(0.035, child(r, 'page', i), 700.0, 1100.0, 6000.0, 30.0) * E.ar(0.035, 0.003, 0.025, 1.5)
            air = kit.whoosh(0.06, child(r, 'leaf', i), 400.0, 1200.0, 0.8, 0.5, -3.0)
            riffle = layers(riffle, (normalize(page), t, -2.0 * i), (air, t, -6.0 - 2.0 * i))
            t += r.uniform(0.04, 0.06)
        settle = flap(child(r, 'settle'), 0.07, 800.0, 200.0, -12.0)
        x = layers((sleeve, 0.0, -14.0), (creak, 0.0, -10.0), (thump, 0.02, -6.0), (spine, 0.14, -12.0),
                   (normalize(riffle), 0.22, -2.0), (settle, 0.24 + t, -6.0))
    return _outside(calm(x, 100.0), r, -24.0)
