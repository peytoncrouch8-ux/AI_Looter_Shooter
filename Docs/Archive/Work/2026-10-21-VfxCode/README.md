# VFX holding tree (12.5 VFX pass, 2026-10-21 night; plan `Saved/Vfx/PLAN.md`)

Unbuilt, unrun, uncommitted. Base: HEAD 8ac1be7 (`Originals/HASHES.txt`: git blob hash of each copied file, all clean at
copy time; `git hash-object Source/AI_Looter_Shooter/<file>` still equal = copy straight over, else
`git merge-file Source/.../<file> Originals/<file> Saved/VfxCode/Source/AI_Looter_Shooter/<file>`). `Originals/` holds the
untouched copies.

## New files (Source/AI_Looter_Shooter/...)
- Weapons/VfxTypes.h (EVfxShape, FVfxKill, FVfxExplosion, FVfxMuzzle, FVfxMuzzleLook, FVfxTierLayers, FVfxExplosionLook, FVfxHitStream)
- Weapons/VfxTuning.h/.cpp (FVfxTuning, the `Looter.Vfx.*` cvars, Live())
- Weapons/VfxRules.h/.cpp (detail, stream, weights, tiers, muzzles, blasts, curves)
- Weapons/WeaponFXShapes.cpp (SpawnShockRing, SpawnFireball, SpawnStarburst, DetailAt, StreamDetailFor, NumShapes)
- Weapons/WeaponFXKills.cpp (SpawnKillLayers, SpawnExplosion)
- Weapons/DeathSplats.h/.cpp (FDeathSplats, EVfxSplat, FVfxSplatLook), Weapons/VfxSubsystem.h/.cpp (UVfxSubsystem: the splats, ticks while one shows)
- Creatures/CreatureKillLook.h/.cpp (MakeKill, Play, LaySplat, SplatFor)
- Embers/Powers/EmberVfx.h/.cpp (Activation, BodyHit, Strike, Blast, Column, BlowScale)
- Tests/VfxTests.cpp (`Looter.Vfx.Rules.Detail/.Stream/.Kills/.Tiers/.Muzzles/.Shapes/.Splats`, `Looter.Vfx.Effects`)
- ../Tools/Unreal/build_vfx_materials.py -> copy to `Tools/Unreal/` (M_FX_Shape, M_FX_Puff, M_FX_SplatDecal in /Game/Art/Materials/Masters)

## Edited copies
Weapons/WeaponFX.cpp (Shapes component, M_FX_Puff smoke when present, pool size cvar, Target/scene-flash pops),
WeaponFXDraw.cpp (camera kept, Ring/Fireball/Star drawing + stand-ins, puff seed/erosion floats, water rings padded, spray stretch),
WeaponFXCreatures.cpp, WeaponFXImpacts.cpp, WeaponFXWorld.cpp, WeaponFXShots.cpp (ricochet glint, SpawnMuzzleBurst),
Creatures/SpiderCreatureReact.cpp (OnDied -> kill layers; body lands -> ichor splat), SlimeCreatureReact.cpp (OnDied -> layers + gel splat),
UnpaidCreatureMoods.cpp (first dead tick -> layers + ash splat), Embers/Powers/EmberPowerBase.cpp (Hurt -> BodyHit for every blow),
EmberDustDevil, EmberSundown, EmberBorrowedTime, EmberLuckyStreak, EmberDoubleOrNothing, EmberFiftyTwoPickup, EmberTollChain,
EmberGraveWard, EmberLastRites, EmberSlagBomb, EmberIronHide, EmberRailSpikeLook, EmberTripwire, EmberLongShot, EmberSpyglass (.cpp: a few lines each).

## WeaponFX.h (not edited: LOOT-CODE owns it) - apply `Proposals/WeaponFX.h.diff` after LOOT-CODE's version
Full proposed file `Proposals/Weapons/WeaponFX.h`. Additions only, apart from LOOT-CODE's block (theirs sits after SpawnCreatureHit; mine after
SpawnDeathBurst and in the private part):
1. `#include "Weapons/VfxTypes.h"`.
2. Public: `SpawnShockRing(Center, Axis, StartRadius, EndRadius, Width, Color, Intensity, Life, bool bFacing = false)`,
   `SpawnFireball(Location, Velocity, StartSize, EndSize, Color, Intensity, Life)`, `SpawnStarburst(Location, Size, Color, Intensity, Life, int32 Spikes = 8)`,
   `SpawnKillLayers(const FVfxKill&)`, `SpawnExplosion(const FVfxExplosion&)`, `SpawnMuzzleBurst(const FVfxMuzzle&)`, `float DetailAt(const FVector&) const`,
   `int32 NumShapes() const`.
3. Private: `EParticle` appends `Ring, Fireball, Star`; `FParticle` gains `float Width = 0.f; bool bFacing = false;`; members
   `TWeakObjectPtr<UInstancedStaticMeshComponent> Shapes; FVector LastCamera; bool bHasCamera = false; FVfxHitStream HitStream;
   int32 MaxParticleCount = 1000; float StreamDetailFor(const FVector&);`; SpawnDeathBurst's comment (35-45 pieces).

## No edits (proposals)
`Proposals/GUNS_CODE_muzzle_rocket_tracers.md`: WeaponBaseFiring.cpp per-kind flash quads + SpawnMuzzleBurst call (needs an
`ShotsInBurst` counter), WeaponRocketBlastEffects.cpp -> SpawnExplosion + scorch splat, BulletSubsystem.cpp tracers per kind.
CreatureHitReactionComponent.cpp and CreatureTierFXSubsystem untouched (the kill layers come from the React files).

## Order
1. After GUNS-CODE and LOOT-CODE merge: apply the header diff on their WeaponFX.h, copy this tree over Source (merge-file where a hash moved).
   LOOT-CODE's WeaponFXElements.cpp needs nothing from here (its new particles still use AddParticle/AddGlow).
2. `Tools\launch.ps1 -Build`. Risky spots to read in the log: lambda captures in WeaponFXCreatures/Impacts/Kills (shadowing is an
   error here), `TAutoConsoleVariable<bool>`, `UDecalComponent::DecalColor` (BulletMarks sets it the same way).
3. `Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_vfx_materials.py"` (VFXMATS lines; Custom HLSL written
   blind: fix by its compile errors). Then reopen the level (FWeaponFX loads the materials as its effects start).
4. Tests (new/changed only): `Looter.Vfx`, `Looter.Weapons.FX`, `Looter.Gunfeel.React.Spurts`, `Looter.Embers.Power`, `Looter.Creatures.React`
   (whichever group the React tests are under).
5. PIE by eye (single grabs): rifle/SMG/LMG into dirt and stone at 10 and 40 m, a spider and an Unpaid killed by rifle, shotgun,
   sniper crit; `Looter.SpawnCreature Spider Epic`; Slag Bomb, Last Rites, Rail Spike; water rings still plain. Then
   `Tools\perf.ps1 -Label "vfx pass" -Exec "Looter.Quality Medium"` and the horde in town; knobs in PLAN.md if over 8.3 ms.

## CODEMAP lines (add at the merge)
- `Weapons/VfxTypes.h`: `EVfxShape`, `FVfxKill`, `FVfxExplosion`, `FVfxMuzzle`, `FVfxMuzzleLook`, `FVfxTierLayers`, `FVfxExplosionLook`, `FVfxHitStream` (the VFX pass's plain types).
- `Weapons/VfxTuning`: `FVfxTuning`, the `Looter.Vfx.*` cvars (`Live()`; field values = defaults). `Weapons/VfxRules`: `VfxRules` (detail by distance and stream, hit/kill weights, kill carry, tier layers, muzzle looks by kind, blast looks, ring/fireball curves).
- `Weapons/WeaponFXShapes.cpp`: `FWeaponFX` shock rings, fireballs, starbursts (M_FX_Shape or stand-ins), `DetailAt`. `Weapons/WeaponFXKills.cpp`: `SpawnKillLayers` (a kill's gore, mist, skirt, rings, tier), `SpawnExplosion`.
- `Weapons/DeathSplats`: `FDeathSplats`, `EVfxSplat`, ground splats (M_FX_SplatDecal pool). `Weapons/VfxSubsystem`: `UVfxSubsystem` (the splats).
- `Creatures/CreatureKillLook`: `CreatureKillLook` (a kill's layers and splat, called by each kind's death in its React file).
- `Embers/Powers/EmberVfx`: `EmberVfx` (the powers' activation ring, body hit, strike, blast, column).
- `Tools/Unreal/build_vfx_materials.py`: M_FX_Shape, M_FX_Puff, M_FX_SplatDecal.
- Amend `Weapons/WeaponFX` line: `WeaponFXShots.cpp` adds `SpawnMuzzleBurst`; `WeaponFXDraw.cpp` draws the shapes.

## Pipeline section (paste as "## VFX pass (12.5, built ...)" once checked)
Code-drawn, no particle assets, no lights. Masters `M_FX_Shape` (additive: ring / fireball / starburst by custom data 4; 7 floats),
`M_FX_Puff` (translucent noise puffs eroding with age; 6 floats; seed < 0 = plain disc), `M_FX_SplatDecal` (DBuffer splat; Spread,
Seed, Wet; DecalColor = colour + opacity) from `build_vfx_materials.py`; until they exist FWeaponFX draws stand-ins and no splats.
Detail x1 to 15 m -> x0.35 at 60 m; a stream into one spot (0.3 s, 150 cm) thins by 1/sqrt(n-1) to x0.25; a hit's pop never
thinned. Hits: splat puff + starburst pop + 2-3 gobs; heavy rounds a ring; crits gold star + ring. Impacts: soft ground geyser +
skirt + big clods; hard pops (metal ping ring). Kills: base bursts (shared with powers and globs) + kill layers by gun
(`KillWeight`, `KillCarry`) + tier layers (`TierLayers`) + a splat (ichor pools on landing; gel; ash), bosses none. Powers:
`EmberVfx` at each beat; `Hurt` shows every blow. Muzzles/tracers/rocket: GUNS-CODE's files (proposal). Cvars `Looter.Vfx.*`.
