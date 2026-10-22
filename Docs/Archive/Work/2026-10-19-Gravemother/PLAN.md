# The Gravemother's Barrow fight: BARROW-CODE's plan (phase 1, 2026-10-19)

Read with `Saved/Gravemother/CONTRACT.md` (the user's calls, the asset and cue names). Sources read: her code
(`GravemotherCreature*`, `GravemotherFight`), the spider (`SpiderCreature*`), `UBossComponent` (+Show/Phases/Adds),
`AGroundCrack`, `ASpitterGlob`/`ASpitterSpider` (the lob, the choke-in-tell), `AForemanRockFall`, `UBulletSubsystem`
(+Impacts), `UHealthComponent`, `FWeaponFX`, `UCreatureReactComponent`, `UCreatureKnockComponent` (dizzy sparks),
`FCreatureBodyHulls`, the importer's hit zones (`ModelImporterRig.cpp`), the Gravemother tests, RansomsRest.md (Side 3,
Enemies by rank), Pipeline (who does what, tests, packaging), the four Barrow sheets.

Ground rule kept everywhere: **every asset is loaded by path and may be missing.** Until ART's assets are imported the
fight runs whole on `SK_Spider` with engine-shape stand-ins (cube slab, flattened spheres) in `M_StylizedSurface`
colours and today's `AGroundCrack`/`FWeaponFX` effects. Nothing logs a warning for a missing asset except one Log line
per asset per session (`GravemotherAssets::Load`).

---------------------------------------------------------------------------------------------------------------------

## 1. Files

### New (all `Source/AI_Looter_Shooter/`; class prefix `Gravemother` so CODEMAP stays together; asset names stay ART's)

| File | Class / content | ~lines |
|---|---|---|
| `Creatures/GravemotherTypes.h` | enums `EGravemotherFurrow`, `EGravemotherMove`, `EGravemotherStone`; USTRUCTs `FGravemotherFurrowSettings`, `FGravemotherClodSettings`, `FGravemotherUnearthSettings`, `FGravemotherStoneSettings` (her tunables move here, so her header stays < 500) | 230 |
| `Creatures/GravemotherAssets.h/.cpp` | namespace `GravemotherAssets`: every path (one place), `Load<T>(Path)` (DoesPackageExist + LoadObject, null when missing, one Log line), stand-in meshes/looks, `AddGroundDecal(...)`/`SetDecalOpacity` | 60 + 150 |
| `Creatures/GravemotherCreatureClod.cpp` | her Grave Clod: tell, tear-up, hold, throw, counter, drop, lob clearance | 300 |
| `Creatures/GravemotherCreatureStone.cpp` | her side of the stone: spawn/reset, `IsSoftRear`, `IsCriticalSpot`, break reaction (long stagger), abdomen zones read from the physics asset | 220 |
| `Creatures/GravemotherCreaturePose.cpp` | `GetPoseOverride` per move (dig, lift, throw, heave, plough, unearth slam), stun stars | 220 |
| `Creatures/GravemotherStone.h/.cpp` | `AGravemotherStone`: own `UHealthComponent`, three states + broken, mesh swaps, the hit's look | 130 + 300 |
| `Creatures/GravemotherStoneBreak.cpp` | the crack-off: shards split, slide off, land, lie, sink; reset | 220 |
| `Creatures/GravemotherClod.h/.cpp` | `AGravemotherClod`: held (shootable), thrown (lob flight), dropped | 130 + 300 |
| `Creatures/GravemotherClodLanding.cpp` | landing shadow, burst (damage, shove, FX), rubble spawn, burst-on-her | 220 |
| `Creatures/GravemotherRubble.h/.cpp` | `AGravemotherRubble`: the pile (cover 10 s, settles away), impact scar | 80 + 200 |
| `Creatures/GravemotherFurrow.h/.cpp` | `AGravemotherFurrow`: layout on the ground, heave (crack tiles, front, jets, crest), furrow strip | 140 + 330 |
| `Creatures/GravemotherFurrowBerms.cpp` | berms rising behind her, 20 s, crumble; earth thrown both sides | 250 |
| `Creatures/GravemotherMound.h/.cpp` | `AGravemotherMound`: bulge 1 s (shootable), burst (spiderling), pop (caved), collapse, crater | 110 + 330 |
| `Creatures/GravemotherFX.h/.cpp` | `UGravemotherFX` (UTickableWorldSubsystem): spawn API for ART's cards and chunks, fallbacks to `FWeaponFX` | 150 + 300 |
| `Creatures/GravemotherFXDraw.cpp` | its pools: move, land on a floor plane, fade, camera-facing instance transforms | 300 |
| `Dev/GravemotherDevCommands.cpp` | `Looter.Gravemother <Clod|Furrow|Quake|Unearth|Stun|Break|Stone <0-100>|Pace <0-2>>` on the nearest Gravemother (PIE checks, the user) | 140 |
| `Tests/GravemotherFurrowTests.cpp` | `.Furrow.Line`, `.Furrow.Berms` | 300 |
| `Tests/GravemotherClodTests.cpp` | `.Clod.Rules`, `.Clod.Lob`, `.Clod.Countered` | 350 |
| `Tests/GravemotherStoneTests.cpp` | `.Stone.Health`, `.Stone.Crits`, `.Stone.Break` | 350 |
| `Tests/GravemotherFXTests.cpp` | `.FX.Pools`, `.Assets.Paths`, `.Pose.Grip` | 220 |
| `Tests/GravemotherTestWorld.h` | shared test helpers (SpawnMother, HitOn, floor box, AdvanceMounds) pulled out of the two current test files | 90 |

### Renamed (git mv by Main, then edited)

- `Creatures/GravemotherCreatureCharge.cpp` -> `GravemotherCreatureFurrow.cpp` (lane measure kept as is; telegraph/dash ->
  heave/plough; the crack -> `AGravemotherFurrow`). ~420 lines; if it passes 480, the lane (`IsLaneClear`, `MeasureLane`)
  moves to `GravemotherCreatureLane.cpp`.
- `Creatures/GravemotherCreatureBrood.cpp` -> `GravemotherCreatureUnearthed.cpp` (spiderlings kept: `MakeSpiderlingSpawn`,
  `SpawnSpiderling`, `SpiderlingName` used by Dev commands and CastShots; the call now raises mounds).

### Changed

| File | Change |
|---|---|
| `Creatures/GravemotherCreature.h` | new API (§3), settings as the four structs, `EGravemotherCharge`/`Spit`/burn members gone, overrides `IsCriticalSpot`, `SetActorHiddenInGame`, `SetHitVolumesEnabled`, `GetPoseOverride` |
| `Creatures/GravemotherCreature.cpp` | constructor (SK_Gravemother if made, else SK_Spider + pale hide), BeginPlay (stone, zones), EndPlay (clod, mounds, stone; furrow/rubble finish alone), Tick order, OnHurt (pending unearths, stone at 33%), OnDied, OnRespawned |
| `Creatures/GravemotherCreatureMoves.cpp` | attack choice Roar > Furrow > Quake > Clod > Bite; roar's slam raises pending mounds; spit and burning cracks removed; reel + stun; pace |
| `Creatures/GravemotherFight.h/.cpp` | `FPace` reworked (§4.8); `Spit()` removed; rules for tests: `ClodFlightSeconds`, `HeaveFront`, `StoneStateFor`, `StoneHealthFor`, `BermRiseShare`; phase names kept |
| `Creatures/SpiderCreature.h/.cpp`, `SpiderCreatureLegs.cpp` | the pose hook (§2.1) and `GetPosedFoot`; default no-op, every other spider unchanged |
| `Bosses/BossComponent.h`, `BossComponentShow.cpp` | `BeginStagger(float Seconds, const FText& Callout)` overload (§2.2) |
| `Combat/BulletSubsystem.h/.cpp` | `OwnLookTag` opt-out (§2.3) |
| `Audio/LooterSoundCues.h` | 13 cue constants in the Gravemother block (§8) |
| `Config/DefaultGame.ini` | `+DirectoriesToAlwaysCook=(Path="/Game/Art/Effects")` (the GraveFight MIs are loaded by path) |
| `Tests/GravemotherTests.cpp` | Body (new crit rules, her look), `.Charge` -> moved/rewritten as `.Furrow.Line`, `.Brood` -> `.Unearthed.Mounds` + `.Unearthed.Shot`; PackCalls, Loot kept |
| `Tests/GravemotherFightTests.cpp` | Rules (new pace), Phases (mounds after the roar, stone off at 33%, +25%), Moves (quake kept, clod replaces spit, reel, furrow miss window, no burn), StandDown kept |
| `Tests/GravemotherSideTests.cpp` | LairRoom: `CallBrood()` now raises mounds: check the mounds' spots in the den (or advance them 1 s and check spiderlings) |
| `Tests/PlayerPaceTests.cpp` | `ChargeSpeed` -> `Furrow.Speed` |
| `Tools/Unreal/create_bestiary_pages.py` | her page's description and notes (§11; Main runs it) |
| `CODEMAP.md`, `Docs/Areas/RansomsRest.md` (Side 3, Enemies by rank), `Docs/Pipeline.md` (a Barrow note) | phase 2's end |

Unused after (left, as the contract says): cues `GravemotherGurgle`, `GravemotherSpit`, `GravemotherCrackBurn`;
`AGroundCrack` stays (the quake's ring, and the stand-in for decals not yet imported).

---------------------------------------------------------------------------------------------------------------------

## 2. Shared code outside her files (small, additive)

### 2.1 Spider pose hook (`SpiderCreature.h/.cpp`, `SpiderCreatureLegs.cpp`)

Her moves need poses the spider doesn't have (front legs dug into the floor, lifted together over her head with a clod,
head low down a furrow). Laying them on `BonePose` after the spider poses it would mean re-solving the IK, so the spider
gets one protected hook, a no-op by default.

```cpp
/** A subclass's own pose this frame (ASpiderCreature::GetPoseOverride), laid on the spider's. */
struct FSpiderPoseOverride
{
	/** Added to the body's targets: height and lunge (cm at size 1), pitch (degrees, nose up). */
	float Rise = 0.f;
	float Pitch = 0.f;
	float Lunge = 0.f;
	/** The fangs' spread (degrees), when set. */
	TOptional<float> Fang;
	/** How much of the spider's own attack rear and lunge stays (0: the override's alone). */
	float OwnShare = 1.f;
	/** The front pair held by the override: in the air at FrontAir (body frame at size 1; Y is the right leg's, mirrored
	 *  for the left), or on the ground under FrontGround (actor frame at size 1, X ahead, Y out) sunk DigDepth (cm at
	 *  size 1), blended by GroundShare (1: on the ground); on a spring of FrontOmega (1/s). */
	bool bFrontFeet = false;
	FVector FrontAir = FVector::ZeroVector;
	FVector2D FrontGround = FVector2D::ZeroVector;
	float GroundShare = 0.f;
	float DigDepth = 0.f;
	float FrontOmega = 14.f;
};
protected:
	/** Its own pose for this frame, alive; false: none (every spider but the Gravemother). */
	virtual bool GetPoseOverride(FSpiderPoseOverride& Out) const { return false; }
public:
	/** Where a leg's foot was posed this frame (world): Pair 0-3, Side -1 left / +1 right. For the Gravemother's grip. */
	FVector GetPosedFoot(int32 Pair, float Side) const;
```

- `AnimateBody`: before the state branch, `bPoseOverride = State != Dead && GetPoseOverride(PoseOverride)`; in the Attack
  branch the rear/rise/lunge terms are multiplied by `OwnShare`; after the branch `TargetZ += Rise*Scale`,
  `TargetPitch += Pitch`, `Lunge += Lunge*Scale`, `TargetFang = Fang` if set. Kept as members for `AnimateLegs`.
- `AnimateLegs`: `IsHeld` also holds `Pair == 0` while `bFrontFeet`; that branch computes the target (`FrontFootTarget`
  in `SpiderCreatureLegs.cpp`: air = `Body.TransformPosition(FrontAir with Y*Side)`, ground = `GroundUnder(actor +
  Yaw*(FrontGround with Y*Side)*Scale) - Z*DigDepth*Scale`, lerped) and calls `StepHeldFoot`. `bNeedsReset = true` so the
  gait steps the feet back when the override lets go (as after a bite).
- SpiderCreature.cpp grows ~25 lines (437 -> ~465), Legs ~20, header ~35. Every spider test (`Looter.Creatures.Anim.*`,
  CastShots numbers) must stay as it is: the default returns false and nothing else moves.

### 2.2 Boss stagger with its own length (`BossComponent.h`, `BossComponentShow.cpp`)

```cpp
/** Staggers it now for Seconds, its bar calling Callout (empty: the show's): a boss's own stun (the Gravemother's clod
 *  burst on her, 2 s). Same refusals as BeginStagger(); the stagger's cooldown follows it as usual. */
bool BeginStagger(float Seconds, const FText& Callout = FText::GetEmpty());
```
`BeginStagger()` becomes `return BeginStagger(Stagger.Seconds);`. Abel and the test boss unchanged.

### 2.3 Shots on things that show their own hit (`Combat/BulletSubsystem.h/.cpp`)

The stone must spark and chip with its own cards and `StoneHit`, not a world stone puff + `Impact.Stone` on top (double
sound), nor her ichor (it would be `Flesh` if it were her component). One tag:

```cpp
/** A component tagged so (the Gravemother's stone, clod and mounds) shows and sounds its own hits: a bullet only damages it. */
static constexpr const TCHAR* OwnLookTag = TEXT("Impact.Own");
```
In `ResolveHit`: `const bool bOwnLook = HitComponent && HitComponent->ComponentHasTag(OwnLookTag);` skips `ShowHit`,
`PlayImpactSound`, `LeaveMark` (the Niagara path too). Damage, `NotifyBulletHit`, `OnBulletHit` unchanged (hit marker
stays). ~8 lines. The same pieces carry `Surface.Stone` / `Surface.Dirt` tags for melee and any other surface reader.

### 2.4 Cues and config: §8, and `/Game/Art/Effects` into DirectoriesToAlwaysCook.

---------------------------------------------------------------------------------------------------------------------

## 3. Her class (`AGravemotherCreature`, still `ASpiderCreature`'s subclass on SK_Spider's rig)

### Constructor

- Body: `FindIfMade<USkeletalMesh>(GravemotherAssets::Body)`; found -> `SetSkeletalMeshAsset` (its own materials, no
  pale hide); else SK_Spider (spider's constructor) + `MI_SpiderBody_Pale` as today. Same bones, so `SetupRig` works on
  either; same capsule and -62 offset (her model is Spider.py's at size 1).
- `CriticalSpotBones = { head, abdomen }` kept (the Super's test); `IsCriticalSpot` gates the abdomen (§5.4).
- Boss setup as today. `ChaseSpeed`, `WalkSpeed` as today; `Furrow.Speed = 900 * MoveScale` (was ChargeSpeed).

### Settings (in `GravemotherTypes.h`; distances "x size" are at a spider's size and grow with hers, world cm don't)

```
FGravemotherFurrowSettings  Range 900 x size, MinDistance 300 x size, Overshoot 150 x size, Speed 900*MoveScale x size,
  Tell 2.0 s (user), AimAt 0.4 s, CrawlEnd 1.8 s, Recover 1.0, MissRecover 2.0, Cooldown 8, FirstDelay 2,
  Strength 2 (x bite), HitMargin 40 x size, BurstRadius 170 x size (slam), StripWidth 190 (world), CrackWidth 70 (world),
  CrackTile 280, StripTile 380, BermOffset 165 (world, from the line to a berm's middle), BermEvery 100, BermSeconds 20
  (user), BermRiseSeconds 0.3, BermCrumbleSeconds 1.2, RearLag 260 (world: a berm rises once her abdomen tip is past it)
FGravemotherClodSettings    MinDistance 400 x size (7.2 m), MaxDistance 1200 x size (21.6 m), Tell 1.2 s (user),
  TearAt 0.4, LiftedAt 0.8, Recovery 0.7, Gravity 1800, FlightAt20m 1.5 s (user), FlightMin 0.8, FlightMax 1.7,
  Strength 1.5 (x bite), BurstRadius 200 (world), EdgeShare 0.5, StunSeconds 2 (user), RubbleSeconds 10 (user),
  RubbleSettleSeconds 1, HitRadius 75 (world: the held clod's shootable sphere), Size L
FGravemotherUnearthSettings Radius 450 x size, MaxStep 200, Spacing 200, ClearOfPlayer 300 (the old Brood* values),
  BulgeSeconds 1.0 (user), CrackRaceSeconds 0.3, CraterSeconds 6, PopLieSeconds 4, SinkSeconds 1, LateRaiseSeconds 2.5
FGravemotherStoneSettings   HealthShare 0.2 (Main), CrackedAt 0.66, ShatteredAt 0.33 (Main), RearMargin 4 (cm, bone
  space), RearShare 0.68 (fallback), ShardLieSeconds 12, ShardSinkSeconds 1
```
Roar (0.9 / 0.6) and Quake (range 280, radius 300, windup 1.2, recovery 0.9, strength 1.5) unchanged, as UPROPERTYs.

### Public API (new or renamed; the rest as today)

```cpp
// Furrow (GravemotherCreatureFurrow.cpp)
EGravemotherFurrow GetFurrowState() const;          // None, Heave, Plough, Recover
bool IsFurrowReady() const;  void ReadyFurrow();
bool HasTakenAim() const;
FVector GetFurrowStart() const;  FVector GetFurrowDirection() const;  float GetFurrowLength() const;
float GetHeaveFront() const;                        // cm along the line the split has reached
bool HasFurrowHit() const;
AGravemotherFurrow* GetFurrow() const;              // this furrow's, while hers (it lives on its 20 s alone)
// Grave Clod (GravemotherCreatureClod.cpp)
bool IsClodReady() const;  void ReadyClod();
AGravemotherClod* GetHeldClod() const;  AGravemotherClod* GetThrownClod() const;
bool IsStunned() const;
void CounterClod(AController* By);                  // the held clod calls it when shot
// Unearthed (GravemotherCreatureUnearthed.cpp)
int32 CallBrood();                                  // one call's mounds now (console, tests); how many
int32 GetPendingUnearths() const;
TArray<AGravemotherMound*> GetMounds() const;       // bulging or burst, not yet gone
ASpiderCreature* RaiseFromMound(AGravemotherMound& Mound);   // the mound calls it as it bursts
void NoteMoundPopped(AGravemotherMound& Mound, AController* By);
// (MakeSpiderlingSpawn, SpawnSpiderling, SpiderlingName, GetBroodCallsMade, GetBrood kept)
// Stone (GravemotherCreatureStone.cpp)
AGravemotherStone* GetStone() const;  bool IsStoneOn() const;
bool IsSoftRear(const FVector& WorldPoint) const;
void OnStoneBroken(bool bByHerThird);               // the stone calls it
virtual bool IsCriticalSpot(const FHitResult& Hit) const override;
// Quake, roar, reel, pace, GetMove, GetBoss as today
```

### Tick order

`Super::Tick` (brain: OnAttackStarted / Strike; spider pose through `GetPoseOverride`) -> `Clock += Dt` ->
`TickFurrow` -> `TickClod` (tear-up, hold the clod at her grip this frame from `GetPosedFoot(0,±1)`) -> `TickMoves`
(cooldowns, quake ring, reel sink, stun stars, late unearth). The clod's grip uses this frame's posed feet: no lag.

---------------------------------------------------------------------------------------------------------------------

## 4. The moves, tick by tick (t = seconds from the move's start)

### 4.1 Attack choice (`GravemotherCreatureMoves.cpp`)

`GetAttackStartRange`: reeling 0; `bRoarWanted` anywhere; `WantsToFurrow` -> Range; `WantsToQuake` -> QuakeRange;
`WantsToClod` -> MaxDistance; else bite. `OnAttackStarted` in the same order: BeginRoar / BeginFurrow / BeginQuake /
BeginClod / bite. `CanStartAttack`: not reeling, furrow None, no clod in hand. `Strike`: Roar -> RoarPeak (+ pending
mounds); Quake -> QuakeSlam; Clod -> ThrowClod; Furrow (Heave) -> BeginPlough; else bite. `TracksTargetInWindup`: false
once the furrow's aim is taken; true through the clod's tell (she keeps facing them).

`WantsToFurrow`: as WantsToCharge (cooldown, MinDistance..Range, lane clear, sampled 4/s). `WantsToClod`: fight on,
cooldown done, MinDistance..MaxDistance, and the lob clear (`IsLobClear`: the arc in 6 straight pieces to 85% of the
flight, world-static only, via `ASpitterGlob::MakeWorldQuery`, sampled 4/s, as the Spitter does: no lobbing into the
den's overhang).

### 4.2 Grave Clod (user: tell 1.2 s, ~1.5 s at 20 m, rubble 10 s, shot in her grip -> burst on her, stun 2 s)

| t | What happens |
|---|---|
| 0.00 | `BeginClod`: Move = Clod, `AttackWindup = 1.2`, `AttackRecovery = 0.7`. `ClodDig` started on her root (kept, stopped if cut). Pose: dig (front feet on the ground 150 ahead / 50 out, dug 20 cm, `OwnShare` 0, pitch -6, rise -8). Dirt kicked at each front foot every 0.15 s (Earth puffs + 2 debris). |
| 0.40 | Tear-up: `AGravemotherClod::Spawn` at the ground between the front feet, scale 0.3; earth burst there (12 Earth puffs, 5 debris, 2 S clods); shootable from now. |
| 0.40-0.80 | Lift: front feet GroundShare 1 -> 0 to FrontAir (60, 22, 110) (both over her head, close together); pitch +18, rise +14. Clod held each frame at the feet's midpoint + 0.5 clod size up, scale 0.3 -> 1 by 0.65; grit sifts off it every 0.08 s. |
| 0.80-1.20 | Held overhead, drawn back a little (FrontAir (35, 22, 125) at 1.15). |
| 1.20 | Strike, `ThrowClod`: aim = the target's feet now (no lead: "at where you stand"); `Seconds = ClodFlightSeconds(dist)` = clamp(1.5 * sqrt(dist/2000), 0.8, 1.7) (20 m -> 1.5, 7.2 m -> 0.9); `Velocity = SpitterRules::LobVelocity(ClodSpot, Aim, Seconds, 1800)`; the landing spot is found by stepping that arc with 8 traces (world static + berms/rubble); `Clod->Throw(...)`; `ClodThrow` attached to the clod; feet flung forward (FrontAir (150, 30, 70) for 0.15 s), then let go. Cooldown = Pace.ClodCooldown. |
| 1.20-1.90 | Recovery. |
| flight | `AGravemotherClod::Advance`: gravity step, one world-static line trace + players' capsules (`UEnemyProjectileSubsystem::CanHurt`, `SweepHitsCapsule`), spin; trail: an Earth puff every 0.05 s + grit. Landing shadow decal (`MI_Decal_LandingShadow`) on the found spot, size 100 -> 260 cm and opacity 0.3 -> 0.85 with flight share. |
| landing | Burst: 22 Earth puffs (75-200 cm, outward 0-260 cm over 1.0 s, opacity 0.30-0.38), 8 debris, 3 S clods, `ClodBurst`, shake 0.35 near; every player within 200 cm (flat, to their body's side; level within 150) takes `bite x 1.5 x lerp(1, 0.5, edge share)` as `UCreatureAttackDamageType` (instigator her controller, causer her or the clod), shoved `(Away*450 + Up*180) x 1.5` like `HitWithAttack`. `AGravemotherRubble` spawned on the ground there. Shadow off. Clod gone after 0.2 s. |
| rubble | Pops up 0.15 s, impact scar decal (260 cm), 3 loose S clods; cover for 10 s (blocks Pawn, Weapon, Visibility, world-static queries; ObjectType WorldStatic; actor tag Obstacle); its pawn block turns on only once no character overlaps it; then settles: collision off, sinks its height over 1 s, scar fades; destroyed. |

Counter (the held clod's `UHealthComponent` (1 HP) dies, any time 0.40-1.20): clod `BurstOnMother` (16 Earth puffs above
her back falling with gravity 0.3, 8 debris raining on her head and back, dust ring at her feet), `ClodShot` on her,
`ClodDig` stopped, `Mother->CounterClod(By)`: the throw is cancelled (Move stays Clod but `Strike` throws nothing: flag
`bClodCountered`), `bStunned = true`, `Boss->BeginStagger(2.0, "Stunned")` (refused -> her own reel with a 2 s clock);
the stagger's reel does the rest (§4.6). Concept timing: hit 0.8 -> burst 0.9 -> stunned to ~2.9 -> up. No damage to
her from the burst (the stun with her crits open is the reward); clod cooldown restarts in full.

Drop (her stagger, death, the attack cut short while the clod is in hand): it falls from where it is, lands, a small
burst (`ClodBurst` at 0.6 volume, 8 puffs), no damage, no rubble. Fight reset: destroyed silently.

### 4.3 Furrow (user: 2 s tell, berms each side 20 s, never off a drop or through a wall)

| t | What happens |
|---|---|
| 0.00 | `BeginFurrow`: Move = Furrow, State = Heave, `AttackWindup = 2.0`, `AttackRecovery` = longest dash + grace + Recover (as today). `Heave` started on her root. Pose: head low, front legs in the floor (OwnShare 0, pitch -10, rise -12, front feet on the ground 170 ahead / 45 out, dug 20). Dirt at the front feet. She tracks her target until the aim. |
| 0.40 | `TakeAim` (as today): line toward the target's feet now, length = clamp(dist + Overshoot, Min, Range+Overshoot) measured by `MeasureLane` (walls, drops, rises). `AGravemotherFurrow::Spawn(Feet + Dir*front, Dir, Length)`: lays crack tiles (280 x 70) and furrow tiles (380 x 190) on the ground by traces, berm spots both sides every 100 cm. The Heave sound moves to the furrow's `Front` component (stop+start). |
| 0.40-1.80 | `Furrow->SetHeave(Front)`, Front = Length x (t-0.4)/1.4 (reaches a target at 10 m about 1.6 s, the end at 1.8): each crack tile fades in (0.12 s) as the front reaches it (+ `Reveal` if ART's material has it); at the front, every 0.1 s two dust jets (34x100 and 70x210 cm, life 0.7) and a debris chunk; a crest bump (2 berm instances at 30% height riding the front, sinking 0.3 s behind it); floor dust puffs along the newest 2 m. Pose shudder (pitch +-1.5 at 9 Hz). |
| 1.80-2.00 | Whole line open: one more jet every 2.5 m along it; end cap tile on (`MI_Decal_HeaveCrackEnd`). |
| 2.00 | Strike, `BeginPlough`: State = Plough; `Heave` stopped, `Plough` started on her root; `SpiderAlert` cry kept. Pose: head low (pitch -12, rise -10), front feet ploughing on the ground (FrontGround (185, 40), dug 25). |
| plough | As the dash (full speed down the line, `MaxWalkSpeed` = Speed x size x Pace.SpeedScale). `TryFurrowHit` (run down once, Strength 2, shove to their side) as today. `Furrow->SetPlough(Covered)`: furrow tiles laid behind her head, a berm rises at each spot once `Covered - RearLag` passes it (0.3 s, from 40 cm under to full, per-instance height x0.88-1.12, yaw +-5, each half over the next); earth thrown both sides at her head every 0.06 s (Earth puffs + debris + S clods arcing out 2-4 m). Shake 0.2 for a player within 8 m every 0.3 s. |
| end | Arrived / out of time / blocked (as today) -> `EndPlough`: `Plough` stopped, slam at her front: impact scar decal 340 cm, dirt ring, `GravemotherSlam`, shake 0.4. Recover 1.0 s, missed 2.0 s with the sink (the window). |
| after | `FinishFurrow`: cooldown; the furrow is let go: berms stand 20 s from the plough's end, then crumble (1.2 s: sink under, Earth puffs along each row, `BermCrumble` once per row), crack and furrow decals fade with them; actor gone. A tell cut short before the plough (stagger, reset, death): the furrow cancels (crack tiles fade 0.5 s, no berms). |

Berms: one `UInstancedStaticMeshComponent` per side (SM_GraveBerm), instance added as it starts to rise; collision
ObjectType WorldStatic, blocks Pawn, Weapon, Visibility, WorldStatic queries (lobs, globs), ignores Camera; actor tag
Obstacle (spawn probes keep spiderlings off them). A rising berm under a player pushes them out (character
depenetration; noted as a risk). Her next lane check sees them (ECC_Pawn sweep): she won't plough through her own cover.

### 4.4 Unearthed (user: 66% and 33%, slam, 4 spiderlings from mounds that bulge 1 s; a mound shot while it bulges kills it)

1. `OnHurt` crossing 66% / 33% (each once a life; a hit past both: two): `++BroodCallsMade`, `++PendingUnearths`,
   `bRoarWanted = true`, `PendingSince = Clock`. (The boss's phase change also wants the roar, as today.)
2. Her next attack is the roar (0.9 s rear and scream). `RoarPeak`: the slam as today; if `PendingUnearths > 0`,
   `Unearth(PendingUnearths)`, which picks `BroodSize x calls` spots (today's `CallBrood` spot rules: her level, the
   encounter cap near the player, safe zones, playable area, her hunting ground, 3 m from the player, apart) and raises a
   mound on each, with cracks racing from her feet to each spot over 0.3 s (narrow `MI_Decal_HeaveCrack` tiles, 40 cm).
3. Late raise: still pending 2.5 s after the call (no target to roar at, reeling) -> `Unearth` at once with the slam's
   dust and sound, no pose.
4. `AGravemotherMound` (t from its spawn): 0-1.0 bulge: SM_GraveMound scale 0.15 -> 1 (ease-out, wobble +-4% at 7 Hz),
   `MoundCracks` decal fading in, grit and a puff every 0.12 s, `MoundBulge` started on it; shootable: a box (170 x 160 x
   60 at full, grown with it) on the Weapon channel, tags `Impact.Own`, `Surface.Dirt`, `UHealthComponent` 1 HP, no
   damage numbers.
5. Burst at 1.0: mesh -> SM_GraveMoundBurst, impact scar decal 230 cm, 12 Earth puffs + 5 debris, `MoundBurst`, box off;
   `Mother->RaiseFromMound(*this)`: `SpawnSpiderling` at the spot facing her target, her hunting ground, `AlertTo`,
   `Brood.Add`, `Boss->AdoptAdd`, `PlayEntranceIfSeen()` (the spider's 0.8 s climb out: out by ~1.8 s, as the sheet).
   Crater lies 6 s, sinks 1 s, gone.
6. Pop (its 1 HP gone while bulging): caved in (burst mesh at 0.7, sunk 10 cm), 8 puffs drawn inward and up, `MoundPop`,
   `UI.Kill` 2D for the local player who did it (it is a kill; the HUD shows the red confirm since the mound has health
   and no Breakable tag), `NoteMoundPopped` (log "one less"); no spiderling. Lies 4 s, sinks, gone.
7. Collapse (her death, fight reset, EndPlay): quietly caved, no spiderling.

`CallBrood()` = `Unearth(1)` now (console, tests, LairRoom test).

### 4.5 The Gravequake and the roar: unchanged (`AGroundCrack` ring, `GravemotherQuake`/`Slam`, shakes).

### 4.6 Reel and stun

`HandleStaggered(true)` -> `BeginReel` as today (attack broken off: a held clod drops, a furrow tell cancels, a plough
ends where it is; held still; `Sink()` every frame). If `bStunned` (the clod): stars too: 4 stars of 37 cm wheeling on a
170 cm ring over her head (1.2 turns/s, a short arc trail each), drawn with `FWeaponFX::AddStreak` (two crossed streaks a
star), fading in 0.1 / out 0.2. `HandleStaggered(false)` -> `EndReel`, `bStunned = false`.

### 4.7 Bite: unchanged.

### 4.8 Pace (`GravemotherFight::FPace`)

```
                     FurrowCooldownScale  ClodCooldown  bQuake  QuakeCooldown  SpeedScale
Phase 0 Fresh Meat          1.0               7 s        no        -            1.0
Phase 1 Brood Wakes         0.875             6 s        yes       9 s          1.0
Phase 2 Mother's Fury       0.69              4.5 s      yes       7 s          1.25  (user: +25% in the last third)
```
`ApplyPace`: `ChaseSpeed = Own x SpeedScale` (0 while reeling), plough speed x SpeedScale, cooldowns scaled. Tells are
NOT scaled (the user's timings, all phases). Gone: `ChargeTelegraphScale`, spit pellets/spread, `bBurningCracks` (the
burning seam was the old charge's crack; not in the user's keep list). The speed comes only with the boss's phase 2
(her health at 33%), never with an early stone break (Main's call).

---------------------------------------------------------------------------------------------------------------------

## 5. The headstone

### 5.1 `AGravemotherStone` (separate actor, attached to her mesh's `abdomen` bone)

- Why an actor of its own: bullets hit it, not her (`Hit.GetActor()` is the stone), so her health, boss, OnHurt and
  stagger never see stone hits; it gets its own `UHealthComponent` (damage numbers, the HUD's white hit marker), and
  `LooterBreakables::Tag` (its break isn't a "kill": white marker; no bullet marks). Spawned in her BeginPlay (test levels:
  `DispatchBeginPlay`), destroyed in her EndPlay; `SetActorHiddenInGame` and `SetHitVolumesEnabled` forward to it.
- Placement: `AttachToComponent(GetMesh(), KeepRelative, "abdomen")` with ART's `stone_attach.json` numbers pasted as
  constants (`GravemotherStone::Attach` location/rotation/scale, bone space at size 1; her actor scale carries her size).
  Until the json exists: a generic place worked out from the abdomen's hull (its bound centre, raised 0.6 of its bound
  radius along component up, upright in component space).
- Mesh: SM_GravemotherStone_0/_1/_2 by state; collision QueryOnly, ObjectType WorldDynamic, blocks ONLY the Weapon
  channel (not Pawn: her lane sweeps; not Visibility: her eyes would see her own stone), tags `Impact.Own`,
  `Surface.Stone`. Stand-in: engine cube 90 x 28 x 110 cm, granite #8E8B86.

### 5.2 Health and states

- `MaxHealth = HealthShare (0.2) x her max health` at every `ResetStone` (BeginPlay after her rank/level, OnRespawned:
  ResetToHome / fight reset / rematch / respawn). A stand-down (no healing) keeps it as it is.
- `GravemotherFight::StoneStateFor(share)`: Intact > 0.66 >= Cracked > 0.33 >= Shattered > 0 = Broken.
- On its health's `OnDamaged`: state changes swap the mesh, play `StoneCrack` at the stone, a chip and stone-dust burst,
  her abdomen jolts (she calls `ASpiderCreature::OnHurt(false, stone)`: the flinch only), `AlertTo(By)` on her (a stone
  hit turns her on the shooter like any hit). On `OnDeath`: `Break(false)`.
- The hit's look (`TakeDamage` override, from the `FPointDamageEvent`'s hit): 3-6 spark streaks glancing off along the
  shot's mirror (`MI_FX_StoneSpark`, 20-55 x 1-5 cm), a 55 cm flash (`MI_FX_GraveFlash`, once a frame), 4 stone-dust
  cards of 30 cm (`MI_FX_GraveDust` tinted #D2CDC3), 2-3 `SM_StoneChip_*` flung, `StoneHit` (not twice within 0.05 s and
  250 cm: a shotgun volley is one crack). Then `Super::TakeDamage` (its health).
- Tests deal damage the test-level way (`BossTestWorld::Hurt(Stone, X)`): the state logic hangs off its health events,
  only the look needs `TakeDamage`.

### 5.3 Breaking (`GravemotherStoneBreak.cpp`; concept: 0.0 cracks, 0.3 splits and roots snap, 0.6 slides off, 1.0 floor)

- `Break(bByHerThird)`: slab hidden, collision off; `StoneBreak` at the stone, shake 0.45, a big chip + stone-dust burst;
  five shard components (SM_GravemotherStoneShard_1..5, absolute transforms) start assembled where the slab was: 0-0.3 s
  drift apart 30 cm/s, 0.3 s gravity on, outward 150-250 cm/s off her back with spin about each shard's own bounds centre,
  land on the ground found by one trace each (bounce once at 0.3), lie 12 s, sink 1 s, hidden. No collision. Stand-in:
  5 small cubes.
- Her reaction (`OnStoneBroken`): crits change at once (§5.4); broken early (her share > 0.33, fighting):
  `Boss->BeginStagger()` (her boss's own stagger, 2.5 s: "the long stagger", longer than the clod's 2 s); by her third:
  no stagger (her phase roar and mounds are coming). Her last third is never brought early.
- `OnHurt` at <= 0.33 with the stone still on: `Stone->Break(true)` (the user: it cracks off by itself at her 33%).

### 5.4 Hits on her: zones read, crits

- Shots on the stone hit the stone actor (5.1): never her. Shots on her mesh report a bone (her physics asset):
  `head` crit always; `abdomen`: crit when the stone is off, else only on the soft rear; every other bone base damage.
- `IsSoftRear(WorldPoint)`: point into the abdomen bone's space (`GetSocketTransform("abdomen")`, scale included).
  Zones read once at BeginPlay with `FCreatureBodyHulls::AddFromPhysicsAsset(PA, &RefSkeleton, {"abdomen"})`:
  - SK_Gravemother: the abdomen body's ROUND shapes (sphere / capsule) are the soft rear (ART's `USP_AbdomenRear`;
    names don't survive the import, one body per bone, so the shape kind tells it): rear when
    `FCreatureBodyHulls::Depth(shape, P) >= -RearMargin (4 cm)`.
  - No round shape (today's SK_Spider: one hull): the rear is the hull's far end: projection of P on the bone-origin ->
    hull-centre axis >= RearShare (0.68) of the hull's farthest projection.
- `IsCriticalSpot`: `Hit.BoneName == abdomen && Hit.GetComponent() == GetMesh()` -> `!IsStoneOn() || IsSoftRear(Hit.ImpactPoint)`
  (`Hit.Location` when ImpactPoint is zero); else Super. Crits feed the boss's stagger build-up as today.

---------------------------------------------------------------------------------------------------------------------

## 6. Effects

### 6.1 `UGravemotherFX` (UTickableWorldSubsystem; Game, PIE, EditorPreview so tests count)

```cpp
enum class EGraveDust : uint8 { Earth, Floor, Stone };   // MI_FX_GraveDust (#806B55), MI_FX_FloorDust (#DCC9A4), GraveDust tinted #D2CDC3
enum class EGraveChunk : uint8 { Debris, StoneChip, Clod, Shard };
void Puffs(EGraveDust Kind, const FVector& Center, int32 Count, float Spread, FVector2D StartSize, FVector2D EndSize,
	FVector2D Life, float Opacity, const FVector& Drift, float Outward, float Gravity = -0.05f);
void Jet(const FVector& Base, float Width, float Height, float Life);          // upright, turns only about Z to the camera
void Sparks(const FVector& At, const FVector& Normal, const FVector& ShotDirection, int32 Count);
void Flash(const FVector& At, float Size);
void Chunks(EGraveChunk Kind, const FVector& At, const FVector& Up, int32 Count, float Speed, float Size);
int32 NumCards() const;  int32 NumChunks() const;  void Advance(float DeltaSeconds);   // tests
```
- Cards: one ISM per material on `/Engine/BasicShapes/Plane` (as `FWeaponFX`), camera-facing each frame, per-instance
  custom data (the layout ART's materials must read, §13). Chunks: one ISM per mesh (debris 1-5, chips 1-3, clod S),
  ballistic, land on a floor plane found by one trace per burst, bounce, lie 1.5-3 s, shrink away. No collision, no shadows.
- Caps: 220 cards (Earth 150, Floor 40, Jets 30) + 60 spark/flash, 90 chunks; oldest go first. Nothing spawned farther
  than 60 m from a viewer (`UCreatureReactSubsystem::IsNearAViewer`).
- Fallback per kind when its MI/mesh isn't imported: puffs -> `FWeaponFX::SpawnDustPuff` (colour x the dust light, it's
  unlit), jets -> 3 stacked rising puffs, sparks -> `AddStreak` + `SpawnRicochet`-like streaks, flash -> `SpawnFlash`,
  chunks -> `SpawnDirt` / `SpawnGrit` / stone chips via `SpawnSurfaceImpact(Stone)`.

### 6.2 Decals (`GravemotherAssets::AddGroundDecal`)

`UDecalComponent` on the owning actor, pointed down (pitch -90), a MID of the MI with scalar `Opacity` (0-1) driven by
code (fade in/out), size (depth 120, length, width), sort order: furrow 0, crack 1, scar 2, shadow 3. Used: heave crack
tiles + end cap, furrow tiles, impact scar (clod 260, slam 340, mound 230 cm), landing shadow (100 -> 260), mound cracks.
Strip orientation (U along the line) checked in PIE; roll 90 if it runs across. Missing MI -> the stand-ins of §7.

### 6.3 Per moment

| Moment | Cards | Chunks | Decal | Shake |
|---|---|---|---|---|
| Clod dig | Earth 3 per foot per 0.15 s | 2 debris | - | - |
| Clod tear-up | Earth 12 | 5 debris, 2 S clods | - | - |
| Clod flight | Earth 1 / 0.05 s (ribbon) | grit | shadow | - |
| Clod burst | Earth 22 (75-200 cm, out to 2.6 m, 30-38%) | 8 debris, 3 S clods | scar 260 | 0.35 |
| Clod on her | Earth 16 raining | 8 debris on her back | - | 0.3 |
| Heave front | 2 jets / 0.1 s, Floor 2 | 1 debris / 0.1 s | crack tiles | - |
| Plough sides | Earth 4 / 0.06 s | 2 debris + 1 S clod / 0.06 s | furrow tiles | 0.2 / 0.3 s |
| Plough slam | Earth 14 ring | 6 debris | scar 340 | 0.4 |
| Berm crumble | Earth 2 per berm | 1 debris per berm | (fade) | - |
| Unearth slam | Earth ring (as roar) | - | 4 racing cracks | 0.55 |
| Mound bulge | Earth 1 / 0.12 s | grit | mound cracks | - |
| Mound burst | Earth 12 | 5 debris | scar 230 | - |
| Mound pop | Earth 8 (inward) | 3 debris | - | - |
| Stone hit | Stone 4, sparks 3-6, flash | 2-3 chips | - | - |
| Stone crack (66/33) | Stone 10 | 6 chips | - | 0.2 |
| Stone break | Stone 18 | 10 chips + 5 shards | - | 0.45 |
| Stun | 4 stars (FWeaponFX streaks) | - | - | - |
| Footfalls | `UCreatureReactComponent::KickFootfall` (pale floor dust) as today | | | |

---------------------------------------------------------------------------------------------------------------------

## 7. Assets (all in `GravemotherAssets.h`; categories from the contract: stone pieces Creatures, earth Props)

| Path | Used by | Missing -> |
|---|---|---|
| `/Game/Art/Creatures/SK_Gravemother` (+ its PA_Gravemother) | her body (constructor) | SK_Spider + MI_SpiderBody_Pale |
| `/Game/Art/Creatures/SM_GravemotherStone_0/_1/_2` | stone states | granite cube slab (one look for all three) |
| `/Game/Art/Creatures/SM_GravemotherStoneShard_1.._5` | the crack-off | 5 small granite cubes |
| `/Game/Art/Creatures/SM_StoneChip_1.._3` | stone hits | FWeaponFX stone chips |
| `/Game/Art/Props/SM_GraveClod_S/_M/_L` | L: the thrown clod; S: loose clods (tear-up, burst, plough, rubble) | earth sphere 120 cm / FWeaponFX clods |
| `/Game/Art/Props/SM_GraveRubble` | the pile (its UCX is the cover) | half-sunk earth sphere 160 x 180 x 80 |
| `/Game/Art/Props/SM_GraveBerm` | berm instances, the heave's crest bump | earth cube 230 x 185 x 74 |
| `/Game/Art/Props/SM_GraveMound`, `SM_GraveMoundBurst` | mound bulge, crater | flattened earth sphere; burst: same at 0.4 height |
| `/Game/Art/Props/SM_GraveDebris_1.._5` | thrown chunks | FWeaponFX dirt clods |
| `/Game/Art/Effects/GraveFight/MI_Decal_HeaveCrack`, `_HeaveCrackEnd` | heave, unearth cracks | `AGroundCrack` line in a dark seam (today's look, no glow) |
| `.../MI_Decal_Furrow` | ploughed strip | none (berms and dust carry it) |
| `.../MI_Decal_ImpactScar` | clod 260, slam 340, mound 230 | `AGroundCrack` burst (dark) for the slam; none else |
| `.../MI_Decal_LandingShadow` | clod shadow | dark `AGroundCrack` burst ring at the spot, burst over the flight |
| `.../MI_Decal_MoundCracks` | mound overlay | none |
| `.../MI_FX_GraveDust`, `MI_FX_FloorDust`, `MI_FX_GraveDustJet`, `MI_FX_StoneSpark`, `MI_FX_GraveFlash` | §6.1 cards | §6.1 fallbacks |

Stand-in looks: `M_StylizedSurface` MIDs (as `AGroundCrack`): earth #6E5C49, berm #B3A08A, granite #8E8B86.
Stand-in meshes: `/Engine/BasicShapes/Cube|Sphere` (already always-cooked). Constructor-time finds use `FindIfMade` (body
only); everything else is runtime `GravemotherAssets::Load` (spawned actors, the FX subsystem).

---------------------------------------------------------------------------------------------------------------------

## 8. Cues (`Audio/LooterSoundCues.h`, Gravemother block)

| Constant | Cue | Played |
|---|---|---|
| `GravemotherClodDig` | Boss.Gravemother.ClodDig | `LooterSound::Start` on her root at the clod tell; `Stop(0.1)` when countered, dropped, cut; ends by itself at the throw |
| `GravemotherClodThrow` | .ClodThrow | `PlayAttached` to the thrown clod (the whoosh travels) |
| `GravemotherClodBurst` | .ClodBurst | `PlayAt` the landing; a drop at 0.6 volume |
| `GravemotherClodShot` | .ClodShot | `PlayAttached` her root when the held clod is shot |
| `GravemotherStoneHit` | .StoneHit | `PlayAt` the hit, merged within 0.05 s / 250 cm |
| `GravemotherStoneCrack` | .StoneCrack | `PlayAttached` the stone at Cracked and at Shattered |
| `GravemotherStoneBreak` | .StoneBreak | `PlayAt` the stone at the break |
| `GravemotherHeave` | .Heave | `Start` on her root at the furrow tell, moved to the furrow's Front at the aim; `Stop` at the plough or a cut |
| `GravemotherPlough` | .Plough | `Start` on her root at the plough; `Stop(0.2)` at the slam / cut |
| `GravemotherBermCrumble` | .BermCrumble | `PlayAt` each berm row's middle as it crumbles |
| `GravemotherMoundBulge` | .MoundBulge | `Start` on each mound; `Stop` at burst/pop/collapse |
| `GravemotherMoundBurst` | .MoundBurst | `PlayAt` the mound at its burst |
| `GravemotherMoundPop` | .MoundPop | `PlayAt` the mound when shot (+ `UI.Kill` 2D for the shooter) |

Kept: Roar (roar/unearth), Slam (plough end, roar peak, quake), Quake, Death; the `SpiderAlert` cry at the plough;
`Boss.Stagger` (the boss's own) at every stagger. Unused: Gurgle, Spit, CrackBurn.

---------------------------------------------------------------------------------------------------------------------

## 9. Lifecycle (what each event does to the pieces)

| Event | Held clod | Flying clod / rubble | Furrow (berms) | Mounds | Pending unearths | Stone |
|---|---|---|---|---|---|---|
| Stagger (crits, early break) | drops | carry on | tell: cancel; plough: ends there, berms stay | carry on | kept | - |
| Clod countered | bursts on her | - | - | carry on | kept | - |
| Her death | drops | carry on | as stagger | collapse | cleared | collision off, stays on the corpse, hidden with it |
| Fight reset (player died) | destroyed | carry on | cancel / berms crumble now | collapse | cleared | reset (intact, full) |
| Stand-down | (attack plays out) | carry on | carry on | carry on | kept | kept |
| OnRespawned / rematch | - | - | - | - | cleared | reset |
| EndPlay | destroyed | carry on | carry on | collapse | - | destroyed |

---------------------------------------------------------------------------------------------------------------------

## 10. Tests (`Looter.Creatures.Gravemother.*`, the family her tests already use; every name a leaf)

Kept/updated: `.Body` (Legendary, 1.8x, x12 health, pack; crits: head yes, legs/thorax no, abdomen front not while the
stone is on, soft rear yes, whole abdomen once broken; SK_Gravemother's own look when imported, else pale hide),
`.PackCalls`, `.Loot`, `.Return`, `.LairRoom` (mounds in the den), `Looter.Story.Gravemother.Mission`,
`.Fight.Rules` (phases; pace: quake from 1, SpeedScale 1.25 only in 2, cooldowns shrink; tells 2.0 / 1.2 at every pace;
no spit, no burning), `.Fight.Phases` (roar on turning; at 66% phase 1, a pending unearth, roar -> 4 mounds -> +1 s 4
adds; at 33% phase 2, the stone cracks off by itself with no stagger, ChaseSpeed own x 1.25; death kills adds),
`.Fight.Moves` (quake as today; clod at 18 m after the roar; reel breaks off whatever she's at; a missed plough's 2 s
window; no burning crack), `.Fight.StandDown`.
New:
- `.Furrow.Line`: 10 m, ready -> Heave, windup 2.0; tracks before 0.4, line fixed after; front at 1.6 s >= the target's
  distance, whole at 1.8; no plough before 2.0; plough down the line at Speed x size; run down once; slam; berms rise
  only behind her; cooldown; up close she bites; a wall 6 m out shortens the lane (SpawnBlocks).
- `.Furrow.Berms`: a full plough lays a berm every 100 cm each side (count = length/100 +-1 per side), each at +-165 cm,
  colliding on Pawn and Weapon once risen; advanced 20 s they crumble; gone after; a cancelled tell lays none.
- `.Unearthed.Mounds`: `CallBrood` -> 4 mounds bulging, no spiderlings; +1 s -> 4 spiderlings (0.45x, 60 HP at level 1,
  Basic, named, gone for good, her boss's adds, round her on her level, apart); 66%/33% once a life; a killing blow none.
- `.Unearthed.Shot`: one mound hurt while bulging -> popped, caved, no spiderling; the other 3 come; a burst mound can't
  be hurt (collision off).
- `.Clod.Rules`: `ClodFlightSeconds(2000) == 1.5`, monotone, clamped; a lob from her grip lands within 20 cm of the aim.
- `.Clod.Lob`: in her fight at 18 m, the tell: no clod before 0.4, held 0.4-1.2 between her raised front feet, thrown at
  1.2 (cooldown), lands near the planned time on a floor box at the player's feet; a player there hurt and shoved;
  rubble there blocking Pawn/Weapon; +10 s it settles; +1 s gone; a player 3 m off untouched.
- `.Clod.Countered`: the held clod hurt -> it bursts on her, nothing thrown, she's staggered 2.0 s with stars
  (`IsStunned`), up after; the clod's cooldown full.
- `.Stone.Health`: stone max = 0.2 x hers (level 1 and 8); stone hits leave her health whole; Intact -> Cracked at 66%
  -> Shattered at 33% (mesh index); reset on `ResetToHome`.
- `.Stone.Crits`: soft rear point (bone space -> world) crit with the stone on, abdomen top not; both crit once off; zones
  read from the PA (round shape) or the hull fallback on SK_Spider.
- `.Stone.Break`: broken early in her fight -> staggered (boss stagger seconds), phase unchanged, pace unchanged; shards
  shown, fall, lie, gone after 13 s; at her 33% it cracks off by itself, no stagger.
- `.Pose.Grip`: during the lift her front feet are above her body bone and the clod between them; a plain spider with
  the same state poses as before (the hook's default).
- `.FX.Pools`: spawning past the caps keeps the counts at the caps; everything expires; with no MIs (or forced fallback)
  the FWeaponFX path makes particles instead.
- `.Assets.Paths`: every path in `GravemotherAssets` names an existing package once imported (missing ones: AddInfo,
  not a failure, as the pale-hide check does).
Run: `Tools\runtests.ps1 -Filter Looter.Creatures.Gravemother`, then all, then `Looter.Creatures.Anim` and
`Looter.Bosses` (the hook and the stagger overload touch them).

---------------------------------------------------------------------------------------------------------------------

## 11. Phase 2 order, PIE checks, docs

1. Shared pieces (2.1-2.3), cues, config; build; spider/boss/bullet tests green.
2. Types, assets, FX subsystem, the five actors; her files; tests; build; `-Filter Looter.Creatures.Gravemother`.
3. PIE on Ransom's Rest: `Looter.SpawnCreature Gravemother`, `God`, `slomo 0.3`, `Looter.Gravemother Clod|Furrow|Unearth|Stone 70|Stone 40|Break|Stun|Pace 2`;
   grab and look at: the clod tell, lob, shadow, burst, rubble; the counter and stars; the heave crawl, plough, berms,
   20 s crumble; mounds bulging, a mound shot, bursts; the stone's three states and its crack-off; her +25% run. Also the
   den: the stone vs the overhang, lobs under it. Stand-ins first; again when ART's assets are imported.
4. `perf.ps1 -Exec "Looter.Quality Medium"` with a fight running (a view in front of the den with
   `Looter.SpawnCreature Gravemother` + `Looter.Gravemother Furrow` in its exec) before/after: dust card overdraw.
5. Docs: CODEMAP lines for every file in §1; RansomsRest.md Side 3 + Enemies by rank ("A charge with a long telegraph"
   -> the Barrow fight's three moves, the stone, crits, last third); a Pipeline "Barrow fight" note (the hook, the
   own-look tag, stand-ins, what to rerun when ART's assets land); `create_bestiary_pages.py` her page (draft below).

Bestiary draft (user-facing; Main may reword): description "...now she's mud-grey and broad as a hearse, with a toppled
headstone roots-bound to her back..."; notes: "The headstone on her back turns bullets: break it (it cracks twice
first), or aim for the soft rear under its back edge. Her head is always soft." / "When she tears up a clod, shoot it in
her grip: it bursts on her and she's dazed a moment. Thrown, watch its shadow; the rubble it leaves is cover." / "When
the ground heaves toward you, step off the line: she ploughs it, and the earth she throws up is cover a while." / "At
two thirds and one third she slams the floor and mounds swell: shoot them before they break." / "Past one third the
stone comes off on its own and she runs faster." (+ the den and return lines as today).

---------------------------------------------------------------------------------------------------------------------

## 12. Risks

- **Spider hook regression**: the shared spider pose path; mitigated by a no-op default, `.Pose.Grip`'s plain-spider
  check, the Anim tests and CastShots numbers. SpiderCreature.cpp ends near 465 lines (limit ~500).
- **Translucent dust overdraw** on Medium (22-card bursts of 2 m cards near the player, jets, plough spray): caps in
  §6.1; measure (§11.4); halve counts if the heaviest view passes 8.3 ms.
- **Decal UV orientation / ART's MI parameters**: names and custom-data layout are guesses until ART builds them (§13);
  setting a parameter a MID lacks is harmless, a wrong custom-data layout is not (wrong tint/opacity).
- **Berms rising under a player**: the instance body pushes the character out (depenetration); if it ever traps one in
  PIE, the side's berms near a player wait until clear (per-instance add is already incremental).
- **The stone vs the den's overhang** (it rides above her capsule) and vs her own poses (the lifted front legs pass near
  it? no: front vs abdomen); checked in PIE.
- **Lobbing in the den**: `IsLobClear` keeps her from throwing into the overhang; she bites/furrows instead there.
- **Physics-asset convention**: the rear zone is told by shape kind (round on `abdomen`); if ART makes it a hull, the
  fallback (axis share) still works but less exactly; ask ART (§13).
- **Two test families renamed** (`.Charge` -> `.Furrow.Line`, `.Brood` -> `.Unearthed.Mounds`): runtests reports by
  name; no prefix clashes (checked against the leaf rule).
- **Stone hits record an encounter** for `AGravemotherStone`'s class (UHealthComponent does it for any owner): a harmless
  extra entry in the progress save; mounds/clod the same.
- **Packaging**: `/Game/Art/Effects` must be always-cooked (config line in §1) or the cards fall back in a packaged build.
- Concurrent agents: Source edits wait for Main's "editor free"; CODEMAP/Pipeline/Handoff carry others' uncommitted
  edits: Edit-tool appends only.

---------------------------------------------------------------------------------------------------------------------

## 13. Open questions for Main, and what to relay

To ART (relay; their outputs decide code constants):
1. Card materials' per-instance custom data, read by code: 0-2 tint RGB (linear, multiplies the texture), 3 opacity
   (cards) or glow strength (spark, flash), 4 atlas cell (GraveDust/FloorDust 0-3 of the 2x2; DustJet 0-1). Cards are
   the engine Plane (100 cm, UVs 0-1), camera-facing (jets: about Z only). OK?
2. Decal MIs: a scalar `Opacity` (0-1) code drives; optional `Reveal` (0-1 along U) on HeaveCrack for a crawling front;
   U runs along the strip's length.
3. Soft rear: a sphere or capsule (USP_/UCP_) on the `abdomen` bone, the only round shape there; the rest a hull.
4. Pivots: clods, debris, chips at their centre; rubble, berm (X along its length), mound, burst at the base centre on
   the ground; shards modelled in place with the stone's pivot (so they spawn assembled); `stone_attach.json` with
   location, rotation and scale, in the bone's space at the rig's size 1.
5. Categories as assumed in §7 (stone/shards/chips -> Creatures, earth -> Props).
To SOUND: ClodDig 1.2 s, Heave ~2 s and MoundBulge ~1 s are started and may be cut (they get a 0.1 s fade); Plough is a
  loop-ish bed stopped with 0.2 s; StoneHit can play every 0.05 s; 4 MoundBulge at once.
Decisions for Main / the user (I made a call; say if wrong):
6. "The long stagger" = her boss's own stagger (2.5 s) for an early break; the clod stun is the same mechanism at 2 s
   with "Stunned" on the bar.
7. Burning cracks dropped (were the old charge's; not in the keep list); tells never shortened by phase.
8. The clod's burst on her does no damage; she is only stunned. Popped mounds give no XP (only the red confirm and
   `UI.Kill`); should the spiderling's kill XP be awarded?
9. The boss bar doesn't show the stone's health (its look and damage numbers do). Want a thin stone bar under hers?
10. Spiderlings "caked in grave mud" (the spiderling sheet) has no asset in the contract: they stay the plain spider.
11. Test family kept as `Looter.Creatures.Gravemother.*` (the contract wrote `Looter.Gravemother.*`).
12. `BossComponent`, `BulletSubsystem` and `SpiderCreature` get small additive changes (§2): OK to touch them in phase 2?
