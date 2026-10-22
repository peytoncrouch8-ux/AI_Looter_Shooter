# New guns contract (Phase 12.3 loot, models first; Main, 2026-10-21 night)

The user's calls: four new kinds (SMG, sniper rifle, LMG, rocket launcher), look = concept B "frontier-tech"
(`Saved/ArtPreviews/Weapons/Concepts/NewGuns_ConceptB.png`: sand polymer shells, gunmetal, brass fittings, ember-orange
heat glow, cyan optics), the SMG = option 7 "Helix" (`SMG_Options2.png`). Look at both images first. Concept A
(`NewGuns_Concept.png`, western walnut) was rejected: not sci-fi enough, SMG too like the LMG. Four distinct silhouettes
are the point. The Bullpup is the closest existing family in look (POLY_SAND shell, accent line).

Read: CLAUDE.md, Art/README.md, Docs/Pipeline.md "Making an asset" item 7 (guns from parts) and its rules, the whole
`Art/Models/Weapons/Bullpup.py` (structure to copy: parts by slot, the same sockets on every body, PARTS layout,
preview) and `Bullpup.parts.csv` (columns), `Drover.py` (a second family), `Tools/Blender/looter_guns.py` (the kit:
materials, `glow`, `Gun`), `Weapons/WeaponParts.h` (stat caps), `Weapons/WeaponModelComponent` (how parts are assembled,
socket names it needs: SOCKET_Muzzle, SOCKET_Aim, SOCKET_Eject, grips, the reload part), `Weapons/WeaponTypes.h`.

## The four families (names final; keys never renamed once exported)
| Family | Kind (C++ comes later) | Ammo | Silhouette and parts | Feel numbers to plan the CSV round |
|---|---|---|---|---|
| **Rustler** SMG | `EWeaponKind::SMG` | SMG | Helix: a long round magazine under the barrel with a spiral window glowing (the rounds left: a `GunAmmoGlow` slot the game drives), wire/skeleton stock, slim sand upper, short shroud; long and low. Slots: Body, Barrel, Muzzle, Magazine (helix lengths = capacity), Sight, Stock. | 40-60 rounds, ~900 rpm, low damage, fast handling |
| **Longhorn** sniper | `EWeaponKind::Sniper` | Sniper | Long and lean: thumbhole sand stock, fluted long barrel, bolt with brass knob, long cyan scope (4-8x by sight part), folded bipod. Slots: Body, Barrel, Muzzle, Magazine (small box), Sight (scopes: Zoom column), Stock. Bolt is the reload/cycle part. | 5-8 rounds, ~45 rpm, huge damage |
| **Thresher** LMG | `EWeaponKind::LMG` | AssaultRifle | Blocky heavy receiver with a sand top, side belt box with brass rounds visible, carry handle, thick barrel with heat rings (`GunHeatGlow` slot the game drives by heat), bipod. Slots: Body, Barrel, Muzzle, Magazine (box sizes), Sight, Stock. | 80-150 rounds, ~650 rpm, slow aim |
| **Powderkeg** launcher | `EWeaponKind::Launcher` | Rocket (new) | Segmented gunmetal tube with sand panels and an ember line, bell muzzle with a brass ring, cyan optic, shoulder pad; the loaded rocket's tip shows and glows (`SOCKET_Rocket`, a separate `PowderkegRocket` model for the projectile and the reload). Slots: Body (tube), Barrel (front bell/extension), Muzzle, Magazine (the rocket kind: frag, cluster...), Sight, Stock (shoulder). | 1-3 rockets, slow reload |

Each family: 6-8 options per slot like the Bullpup (common through legendary, rarer parts bring brass, engraving, more
glow), a `<Gun>.parts.csv` with every option (Name, Word, MinRarity, stat ranges inside the caps; keys unique), sizes
in proportion to the Bullpup (sniper longest, Rustler shortest of the long guns), triangle budget like the Bullpup's
parts. Sockets identical across a family's bodies so every part fits every body. Material slots from the kit (same
names as the Bullpup's where the material is the same, so `M_Gun*` instances are shared); new glow slots
(`GunAmmoGlow`, `GunHeatGlow`) named exactly so. Exports must be repeatable (export twice, compare).

## Agents
- **GUNS-ART-A (Opus): Rustler and Thresher.** Files: `Art/Models/Weapons/Rustler.py`, `Rustler.parts.csv`,
  `Thresher.py`, `Thresher.parts.csv`; previews in `Saved/ArtPreviews/Weapons/<Gun>*.png`.
- **GUNS-ART-B (Opus): Longhorn and Powderkeg.** Files: `Longhorn.py`, `Longhorn.parts.csv`, `Powderkeg.py`,
  `Powderkeg.parts.csv` (+ the rocket model in Powderkeg.py); previews likewise.
- Shared kit changes (`Tools/Blender/looter_guns.py`): only additions (new helpers, new material constants), and only
  GUNS-ART-A edits it; B asks A or writes local helpers in its own scripts.
- Blender beside the running editor goes through `Tools\artrun.ps1` (two at a time; it waits on Saved\ArtPause.flag).
  Export test into `Intermediate/ArtExport_<Gun>` (`Tools\models.ps1 ... -NoImport` as Art/README says). **No Unreal
  import, no editor, no commits**: Main imports and wires them with the C++ (GUNS-CODE, later).
- Look at your previews like a player would (side, three-quarter, first-person view from behind the grip) and fix what
  reads poorly; compare against the concept sheet. Report under 300 words: files, part counts per slot, triangle counts,
  sockets, new material slots, preview paths, what you're unsure of.
