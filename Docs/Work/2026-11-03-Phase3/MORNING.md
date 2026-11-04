# Morning note (2026-11-04): what landed overnight and what to feel for, one change at a time

Rule: play each in PIE with the editor focused (`Tools\pie.ps1 -Map RansomsRest`, or the packaged build); every change has a switch
to A/B the old feel. Nobody has PLAYED any of this; the tests and sims passed where the table says so. Lane notes beside this file.

| # | Change | Commit | Tests | Feel for (PIE) | Switch (old feel) |
|---|---|---|---|---|---|
| 1 | Kill speed + crits | 6e64f232 (+ fixes) | pass | Common rifle on a Regular spider: dead in about nine rounds; shotgun two blasts; revolver three; a head shot on an Unpaid pops the crit number. | Project Settings > Game > Creature Kinds (health 230/160 restores the old) |
| 2 | Hit confirmation | see below | see below | Ranchhand point-blank at a spider: one big number per blast, a crit pellet turns it the white-hot crit; every hit's X stays ~0.35 s; a headshot's X is bigger, white rimmed red; a kill's the biggest, solid red. | `Looter.UI.DamageNumbers.MergeSeconds 0`, `Looter.UI.HitMarker.Seconds 0.18` |
| 3 | Recoil that climbs | | | Hold the Common rifle at a wall from the hip: the reticle climbs most of a hand's width over the magazine, straight up then drifting right; pull down to hold it. Let go: back in ~0.3 s; a 3-round tap settles before the next. | `Looter.Gunfeel.Recoil.WhileFiring 0.35`, `Looter.Gunfeel.Kick.Lift 0` |
| 4 | Fights press | | | After Main 4, from ~25 m fire one shot near Camp SheepFold: the whole camp comes, not only the ones that saw you; among them up to five swing at once, a lunge about 11 (was 14). Quarry Track: shoot one spider, every spider within 15 m comes, a bite about 11 (was 20). Six of any kind kill a standing player in 10-15 s (sims), not 3-5. | `Looter.Fight.HearGunfire 0`, `Looter.Fight.MeleeTokens 3`, `Looter.Fight.DamageScale 1.8`, `Looter.Fight.RingRadius 480` |
| 5 | Loot that differs | | | `Looter.GiveWeapon Rifle Rare 5` three times: at least one card has an effect or element row. `Looter.GiveWeapon Rifle Legendary 5`: a signature line. `Looter.Feedback.Drop Common`: a short beam and a glint; `Looter.Feedback.Drop Rare`: the chime at one loudness whichever way you face. | `Looter.Loot.Parts.TierSlot 0`, `Looter.Legendary.Rolled 0`, `Looter.Loot.Drop.Beam.Common 0` |
| 6 | The camera answers the body | | | Sprint and jump off the porch: the view dips on landing (harder from higher), the arms and gun drop a few cm; sprinting, the view nods and rolls a touch at each stride in step with the gun; strafing leans it. | `Looter.Camera.Motion 0` |
| 7 | The gamepad | | | On the pad a full right-stick push turns 360 in about 1.6 s at `t.MaxFPS 60` and `120` alike; a small push aims finely. Sweep across a spider 10 m away: the crosshair slows on it and eases onto it if the stick drifts; nothing behind a wall. Settings > Controls: Invert Y, Aim assist. If the stick feels numb at the start of a push: `Looter.Look.PadDeadZone 0.05`. | `Looter.AimAssist 0` |
| 8 | The gun lights the hands and ground | | | Fire the shotgun at dusk by the farmhouse: hands, gun and porch light up warm for a frame. Shoot a stone wall or a tin roof from 3+ m: a flash on it; dirt and grass none. Hold the LMG into a wall: it pulses, never a white screen. If the hands blow out: `Looter.Vfx.Muzzle.Light1P 0.4`. | `Looter.Vfx.Muzzle.Light 24`, `Looter.Vfx.Impact.Light 0` |
| 9 | Fire cadence carried | | | Nothing to see; a 30-round burst takes 2.77 s, not 3.0, at 60 and 120 fps alike (count on a stopwatch if curious). | none |
| 10 | No per-frame world scans | | | Nothing visible: minimap dots, loot labels, the gun card, E/hold pickup as before; a dropped ammo box is on the map within a tenth of a second. | `Looter.ActorIndex.Interval` |
| 11 | The Unpaid, one gate | | | (filled when the lane lands) | |
| 12 | Reloads with snap | | | (filled when the lane lands) | |

Art (read-only audit, nothing changed): `Docs/Work/2026-11-03-ArtAudit/AUDIT.md` and the sheet `Saved/ArtPreviews/ArtAudit/compare.png`.
