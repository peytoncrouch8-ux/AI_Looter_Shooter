"""The sound synthesis library: everything the recipes build sounds from, written with numpy alone. Only the guns' shots
and bullet impacts may use recordings (sample, the user's rule of 2026-10-21). See Art/Sounds/README.md.

    core      sample rate, seeded randomness, levels, layering, fades, trims, varispeed
    noise     white/pink/brown/blue noise, band noise, impulse dust, velvet noise, slow random curves
    env       percussive decays, attack-release, breakpoint curves, swells
    osc       sines and pitch curves, band-limited saw/square/triangle, FM, plucked strings, drum membranes
    filters   biquads and combs applied exactly through the FFT, moving filters, time-varying magnitude filters
    dist      oversampled saturation and distortion, ring modulation, bit reduction
    dynamics  envelope followers, transient shaping, compression, a look-ahead limiter
    modal     struck things as modes: bars, plates, machined parts, wood, shells, bells
    granular  grain clouds, grit, rattles
    voice     formant voices, breath and whispers, hiss
    reverb    convolution with generated IRs: outdoor slap-back, open air, small room, plate
    stereo    panning and a mono-safe widener for 2D sounds
    wav       16-bit WAV in and out
    sample    the recordings (Art/Sounds/packs): any WAV read, resampled, shots found by onset and sliced
    analysis  loudness, spectrum balance and the checks
    png       waveform and spectrogram pictures
"""
from . import core, noise, env, osc, filters, dist, dynamics, modal, granular, voice, reverb, stereo, wav  # noqa: F401
