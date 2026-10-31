# Usage (Blender's Python, -I): python -I Art/Sounds/preview_boss.py C:/Dev/AI_Looter_Shooter Keeper Foreman Gravemother Sentinel
# Writes Saved/MusicPreview/<Boss>_Full.wav: the boss theme once alone, then once more with its Rage layer on top,
# at the levels the game plays them (cues.json volume_db), so the user hears what the last phase adds.
import sys, json, os
root = sys.argv[1]
sys.path.insert(0, os.path.join(root, 'Art', 'Sounds'))
import numpy as np
from lib import wav
cues = json.load(open(os.path.join(root, 'Art', 'Sounds', 'cues.json'), encoding='utf-8'))
out = os.path.join(root, 'Saved', 'MusicPreview')
for boss in sys.argv[2:]:
    base_cue, rage_cue = f'Music.Boss.{boss}', f'Music.Boss.{boss}.Rage'
    if base_cue not in cues or rage_cue not in cues:
        print(boss, 'missing in cues.json'); continue
    b, _ = wav.read(os.path.join(root, 'Art', 'Sounds', 'Out', cues[base_cue]['files'][0]))
    r, _ = wav.read(os.path.join(root, 'Art', 'Sounds', 'Out', cues[rage_cue]['files'][0]))
    gb = 10 ** (cues[base_cue]['volume_db'] / 20); gr = 10 ** (cues[rage_cue]['volume_db'] / 20)
    n = min(b.shape[-1], r.shape[-1])
    full = np.concatenate([b[..., :n] * gb, (b[..., :n] * gb + r[..., :n] * gr)], axis=-1)
    full = full / max(1e-9, np.max(np.abs(full))) * 0.89
    wav.write(os.path.join(out, f'{boss}_Full.wav'), full)
    print(boss, 'ok', full.shape)
