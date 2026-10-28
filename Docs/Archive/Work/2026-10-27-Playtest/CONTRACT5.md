# Round 5 contract (Main, 2026-10-27): ideas from a video the user shared

The user shared a video ("I Made a Full Boss Fight in Unreal Engine With AI": C++ + Python-built UE 5.8 game, Opus for code,
AI images for textures, Tripo models, Kimodo + MediaPipe pose-guided animation, Niagara particles built by script, boss
attack rules and hitbox debug view, hit-stop/flinch/sparks/shake, swing smear). The user's picks (2026-10-27): swing smear +
hit feel, boss attack rules, try Niagara effects, look into AI animation (research only, report before installing anything).
Suno: the user would use it "if it's free for voices and sounds"; Main's understanding: Suno's free plan is non-commercial
only, so not usable in a Steam game; RESEARCH confirms the current terms. No Suno until the user decides.

Read first: `CLAUDE.md`, `CONTRACT.md` "Rules for every agent", the pipeline topic files for your area. Edit Source/ directly
this round (each job owns disjoint files below; Main builds). Every number a cvar; tests; 8.3 ms budget; original content only.

## HANDS (same agent): swing smear + melee hit feel
Owns: its melee files (PlayerMelee*, PlayerMeleeTool*, FirstPersonHands* melee, Weapons/WeaponFXMelee*), melee recipes.
A short smear/motion trail behind the tool's head through the strike arc (code-drawn ribbon on FWeaponFX's quads or a
mesh ribbon: cheap, tinted per tool, fades in ~0.12 s), a hit-stop on a melee hit (check the existing AttackerHitStop; make
it read: ~50-70 ms), the struck creature's flinch (the existing hit reactions), a camera kick on hit, sparks per material
(done in round 4: check). Only after the invisible-tool fix lands.

## BOSSRULES (Opus, the round-2 BOSSES agent): boss attack rules + hurt-zone debug view
Owns: `Creatures/Gravemother*`, `Creatures/Foreman*` (+ `DeadMinerCreature*` only if the Foreman's attacks live there),
`Bosses/Abel*`, `Bosses/BossComponent*`, a new shared `Bosses/BossAttackRules*` and `Dev/BossHurtZoneDebug*`, boss tests.
A shared attack chooser for the three bosses: no long idle gaps (a cap), not too fast (a min gap), never the same attack 3
times in a row, an attack only when its reach and facing allow (in front for frontal attacks), each attack's hurt zones
active only in its window (wind-up -> active -> recovery). A debug view (`Looter.Boss.HurtZones 1`) drawing the boss's active
hurt zones (red when live, grey in wind-up) so fairness can be tuned. Keep each boss's identity and the round-2 rules
(1.6x health, stone gate, rim shrug, intros).

## NIAGARA (Opus): one effect rebuilt in Niagara, by script
Owns: new `Tools/Unreal/build_niagara_fx.py`, new `/Game/Art/Effects/Niagara/` assets it makes, new `Weapons/NiagaraFX*`
(spawn helpers), the Cinder grenade's fire bed and burst visuals (`Combat/GraveSaltCinderBed*`, the Cinder part of
`Combat/GraveSaltBurstEffects*` or `GraveSaltGrenadeKinds.cpp`) behind a cvar `Looter.Fx.Niagara 1|0` so both looks can be
compared. Niagara is a built-in engine plugin: check it's enabled in the .uproject (if not, say so: Main enables it). Build the
systems from Python (NiagaraSystem/emitters via the editor's Python API; if the API can't author modules, say what can be
done and the smallest manual step). Fire that licks up, embers rising, smoke rolling, heat-lit ground. Report the GPU/CPU
cost vs the code-drawn version (estimate; Main measures).

## RESEARCH (Sonnet): AI animation + Suno terms
Read-only web research (WebFetch; the built-in browser is failing). (1) NVIDIA Kimodo (or the current name of NVIDIA's free
text/pose-to-motion model) and Google MediaPipe pose detection: what they are, licences (can output go in a commercial game?),
install size and requirements on Windows with an RX 580 GPU (Kimodo may need NVIDIA CUDA: say so plainly), how outputs get to
an Unreal skeleton (BVH/FBX retarget), and a step-by-step of the video's method (AI-drawn key poses -> MediaPipe -> skeleton ->
in-between). (2) Suno: free plan vs paid: commercial use rights, ownership, whether it does sound effects/voices or only music.
Cite sources. Write `Docs/Work/2026-10-27-Playtest/RESEARCH_ANIMATION.md`. Install nothing.
