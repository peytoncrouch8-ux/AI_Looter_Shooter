# HITS: enemy hit reactions you can feel (plan, 2026-10-28)

The user's pick (CONTRACT.md): "enemy hit reactions". Main's default: stylized, no blood (ichor goo, gel, embers, ash,
soul-light). Difficulty goes up in FIGHTS.md: nothing here may make fights easier on balance (stagger guards below).
Built on what exists (Gunplay.md "Gunfeel: hit reactions", "VFX pass"; Enemies.md); every new number a cvar; each layer
has an off switch so a morning undo is one console line.

## What exists (read from code, 2026-10-28)

- Order of a hit: `UHealthComponent::HandlePointDamage` (stores `PendingHitLocation`; gets `BoneName` and drops it) ->
  `HandleAnyDamage` -> `OnDamaged` -> `ACreatureBase::HandleDamaged` (CreatureBaseHurt.cpp) ->
  `UCreatureHitReactionComponent::TakeHit` (FHitReaction rules) -> `LastHit` (`FCreatureHit`, fresh) -> kind's `OnHurt`.
  `UCreatureReactComponent` flashes M_FX_HitFlash (0.09 s, 0.65 / crit 1.0, colour (1, .94, .86) / crit (1, .78, .48), linger
  0.4 s). `BulletSubsystem::ResolveHit` draws spill (`FWeaponFX::SpawnCreatureHit`, crit pops `SpawnBloodCrit` by
  `ECreatureBlood`), plays impact/creature cues; `CreatureVoiceComponent` hurt cries <= every 0.9 s, thuds 0.06 s.
- **The bone is lost**: FCreatureHit has no zone. Crit bones only (`CriticalSpotBones`: spider/Gravemother `head`, Spitter
  `head`+sac, slime core, Unpaid coal via `IsCoalShotBone` chest/coal/spine + angle test, miner lamp, bell). Spider leg
  bones `femur_<sfx>`/`tibia_<sfx>` have hulls and report hits, unused.
- Flinch (`FCreatureHit::FlinchStrength`): `Flinch.Scale` 1 x clamp(0.6 + 3 x burst share, 0.6, 2.5), crit x1.4. Rifle 12
  on a 300-HP spider = 0.72. Results today, rifle body hit:
  - Spider (SpiderCreatureReact.cpp `OnHurt`): body kick 9 cm x 0.72 x 0.85 = **5.5 cm**, tilt 7/10 deg x0.72 = 5-7 deg,
    abdomen 6 deg; spring omega 40, hold 0.06 s, fade 10 (SpiderCreature.cpp). Whole body only, any part hit.
  - Unpaid (UnpaidCreature.cpp `OnHurt` -> UnpaidCreatureArms.cpp `ReactToHit`): FlinchKick 0.6 x 1.54 x 0.72 = 0.67, peak
    ~0.65 of it, pelvis 8 deg/unit = **~3.5 deg**, pelvis shift 6 cm/unit; both arms flung 220 x s deg/s; shroud jerk 90 x s;
    coal heat 0.25 x s (crit 0.8). Spine, chest, neck, head untouched by hits (UnpaidCreatureRig.cpp 328-367).
  - Slime (SlimeCreatureReact.cpp): squash to 1 - 0.18 x s (floor 0.55) **only on the ground, not mid-hop or telegraph**;
    lean 10 deg x s (cap 30) spring 18/0.3; core kick 130 (crit 220) x s.
- Stagger: heavy = crit or burst >= 0.12 in 0.08 s; Basic 0.4 s (Restless 0.325, Gravebound 0.25, Hallowed/Ethereal
  shorter); cooldown 0.7; rifle pressure 5 hits / 1 s; shove 420 cm/s x rank share. **No stagger pose**: the brain pauses
  (`TickBrain`), the body only slides. Shotgun power: burst >= 0.35 within 6 m, size <= 1.2 -> `UCreatureKnockComponent::Knock`
  Shove (rocked back 24 deg, 520 across / 40 up, hold 0.25).
- Hit-stop (creature's own `CustomTimeDilation` 0.02): crit 0.05, heavy 0.035, kill 0.075, crit kill 0.1, revolver/sniper
  crit 0.09; cooldown 0.25. Hit slow 0.55 for 0.12 s (Unpaid with kit: once per 0.5 s).
- Kills: corpse slide by gun (shotgun 650 + lift 35, rifle/SMG/LMG 180, else 260, crit 380, launcher own), 0.35 s; kind
  death burst (`EDeathBurst` Shell/Gel/SoulLight); `CreatureKillLook::Play` layers by `VfxRules::KillWeight` (overkill only
  +0.3 max weight) and `KillCarry`; splats (`FDeathSplats`, max 12). Spider death: one script (throe rear 25 deg/0.2 s, drop,
  bounce, legs curl UNDER: the user's 10-14 rule), whatever killed it. Unpaid: crumple + ash. Slime: splat puddle.
  Bosses: none of it (`IsBoss`), flinch x `Flinch.BossScale` 1.
- Why it reads weak (diagnosis): flinches are 3-6 cm / 3-7 deg at 10-30 m; every part of a body reacts the same; a stagger
  has no pose; deaths don't change with the shot (crit, blast, overkill); nothing shows damage building up over a fight.

## Design: five layers on every hit

1. **Weight class** (Lane A, `FCreatureHit::Weight`): Body (burst < 0.12), Heavy (>= 0.12 or crit), Big (single hit >= 0.25,
   or Sniper/Launcher, or shotgun burst >= 0.35 within 6 m), Kill, Overkill (killing hit with overkill >= 0.5 of max health,
   or a Big kill). Overkill = damage - health before the hit, over max health (`UHealthComponent::GetHealthBeforeLastHit`).
2. **Zone + side** (Lane A): bone -> `ECreatureHitZone` per kind; side Front/Left/Right/Back from the creature's facing vs
   -Direction (Front < 60 deg, Back > 120). Lateral offset of the wound from the body's axis (cm) gives a twist.
3. **Additive pose** (Lanes B-D, procedural on each kind's own posed bones; no anim assets, no physics): zone kick, twist,
   whiplash; stagger pose from `GetStaggerAlpha()`; all on critically damped springs stepped at 120 Hz like today's.
4. **Wound stages** (looks only, by health share: 0 >= 50%, 1 < 50%, 2 < 25%): damage reads over the fight.
5. **Kill styles** by weight and zone: Body / Crit / Blast (shotgun, launcher, Big) / Overkill.

### Global numbers (Lane A; `Looter.Gunfeel.*` existing, `Looter.Hits.*` new, defaults)

| What | Today | New |
|---|---|---|
| Flinch floor `Flinch.Floor` (new) / gain / max | 0.6 / 3 / 2.5 | **0.85** / 3 / **3.0** |
| Far boost `Hits.FarBoost` (shooter distance) | none | x1 within 10 m -> **x1.5** at 25 m+ (reads on screen) |
| Rifle 12 on 300 HP spider at 10 / 25 m | 0.72 | 0.97 / 1.45 |
| HitStop heavy / kill / crit kill / overkill (new) / Big (new) | .035/.075/.1/-/- | **.045/.085/.11/.13/.06** |
| Poise (new, stagger by rate): `Hits.PoiseBreak` / `.PoiseDrain` | rifle pressure only | **0.25 / 0.30 per s** |
| Poise rank scale (Basic, Restless, Gravebound, Hallowed, Ethereal) | - | 1, 1.15, 1.35, 1.6, 1.9 |
| Big hit shove (not a kill, size <= 1.2, non-boss) `Hits.BigShove` | none | 300 cm/s + 14 deg rock (Knock Shove, hold 0.15) |
| Overkill corpse knock / lift x | - | x1.5 / 60 cm (shotgun, launcher) |

- Poise: `+= share` per hit, drains 0.30/s; at >= break x rank scale: a stagger (Job `Poise`), poise -> 0. So a Basic
  staggers at ~30%+ of its health per second sustained, or 25% fast. Shares the 0.7 s cooldown with heavy/pressure/power;
  plus `Hits.StaggerGap` 1.6 s minimum between staggers for Gravebound+ (no stun-lock on elites). Test: 10 s of SMG fire on
  a Basic spider <= 5 staggers; on Gravebound <= 3.
- Hit slow unchanged (FIGHTS owns it; the Unpaid lunge commit depends on it).

### Stagger pose curve (`FStaggerCurve::Alpha(t, Seconds)`): rise 0.08 s (ease-out), hold to 55%, ease back to 0.

## Per creature

Zones (Lane A `CreatureHitZones::ZoneOf`): Spider/Spitter/Gravemother: `head` Head, `body` Body, `abdomen` Abdomen, sac Weak,
`femur_*`/`tibia_*` Leg (leg index from suffix via the rig's `Legs`), fangs Head. Unpaid family: Rig.Head/Neck/hat Head,
Chest/Spine/Coal Body (crit = Core when `IsCoalShotBone` + crit), arm bones Arm (side), shroud/strip bones Shroud, bell/lamp
Weak. Slime: core Core, body Body (top/side/bottom by hit height vs middle). No bone (power, status, test level) = Body.

### Spiders (Brown Spider, Spitter; Gravemother as a boss: flinch/zone looks only)

| Hit | Look / move | Sound |
|---|---|---|
| Body | base kick 9 -> **12 cm**, tilt 7/10 -> **9/13 deg**; new yaw twist 8 deg x s x (offset/40 cm), cap 20 | today's Spider.Hit |
| Leg | that leg buckles: knee -12 cm x s, foot +8 cm, envelope in .04/out .14 s; body rolls 5 deg x s toward it (cap 12), dips 4 cm; chitin chips 3 at the hull | `Creature.Spider.LegCrack` (dry snap, -6 dB) |
| Head crit | nose snaps up 14 deg x s (cap 30), fangs splay 35 deg in .08 s back in .3; today's crit pop | Crit.Shell (exists) |
| Abdomen | abdomen kick 10 deg x s (cap 26), wobble 7 Hz .4 s; goo dribble 2 bits | Spider.Hit low-pitched .9 |
| From behind | abdomen kick x1.5, body lurches forward 6 cm instead of back | - |
| Big | Heavy + Big shove; legs on the far side skid (feet slid 15 cm along the shot) | Creature.Hit.Heavy (exists) |
| Stagger | body drops 10 cm, nose up 12 deg, legs splay 15% out, fangs 25 deg; brain held (today) | `Creature.Spider.Stagger` (hiss-shriek, 1 per stagger) |
| Wound 1 / 2 | limp on the most-hit leg (else front-left): its step lift x0.5, step time x1.3, body roll 3 deg bias; stage 2 + goo drip 1 bit / 0.6 s from the most-hit hull, breath x1.6 | breath rasp in Spider.Idle volume +3 dB |
| Kill (body) | today's throe/drop/curl | today's |
| Kill (head crit) | "lights out": no throe (Apex .2 -> .05 s, rear 0), head snaps back 25 deg, drops at .12 s, curl 0.6x time | Crit.Shell + Spider.Death clipped short |
| Kill (blast) | today's thrown slide; legs flail x1.5 in the air, curl under on landing; abdomen burst x1.3 | + Creature.Hit.Overkill |
| Overkill | blast kill + death burst x1.4 at the wound, 4 big chitin plates, goo splat 40 cm downrange | Creature.Hit.Overkill |

Legs always curl UNDER the body at rest (the user's rule; `Looter.Creatures.Anim.SpiderDeathCurl` must pass).
Spitter: sac hit (non-crit too) wobbles the sac (scale 1.15 pulse .2 s) and drips venom (glob FX, 2 bits). Gravemother:
leg buckle only on legs her `GetPoseOverride` doesn't hold; no stagger pose, no limp, no kill styles (her death is her show).

### The Unpaid (plain, Restless..Ethereal; dead miner, Bell-Ringer, Abel, Foreman inherit; bosses: no hat, no stagger, no kill styles)

| Hit | Look / move | Sound |
|---|---|---|
| Body | pelvis 8 -> **11 deg/unit**; chest 0.35x and head 0.5x counter-lag (whiplash: head lags 40 ms then snaps); spine yaw twist 10 deg x s x (offset/25 cm), cap 22; shroud as today | today's Unpaid.Hit |
| Head | head pitched back 18 deg x s (cap 35), neck 0.5x, jaw drops 12 deg; hat knocked off when s >= 1.2 or killed by it | `Creature.Unpaid.HatOff` (felt thump + flap) |
| Arm | that arm flung 2.0x, the other 0.4x (today 1x each); hand fingers splay 1 (Pose.Splay pulse .15 s) | - |
| Shroud | that chain jerk x2, body flinch x0.5 (a hit "through cloth") | soft cloth flap (in HatOff recipe set) |
| Coal crit | today's flare 0.8 + coal bone scale pulse +0.4, 6 ember sparks spat out the back along the shot | Crit.Coal (exists) |
| From behind | lurches forward (pelvis pitch forward), head whips toward the shooter 25 deg yaw over .15 s (pose only, not the actor's facing) | - |
| Big | Heavy + Big shove; shroud strips snap straight along the shot | Creature.Hit.Heavy |
| Stagger | folds at the waist 18 deg away from the hit, arms thrown out (Spread .6), coal gutters to heat 0.1 for the hold then flares 0.6 on recovery, sinks 15 cm | `Creature.Unpaid.Stagger` (choked gasp) |
| Wound 1 / 2 | coal flickers (heat noise +-0.15 at 7 Hz); stage 2 + ember smoke puff / 0.4 s from the coal, bob halved | `Creature.Unpaid.Gutter` one-shot every ~3 s (no loop) |
| Kill (body) | today's crumple + ash | today's |
| Kill (coal crit) | "snuffed": coal scale 1.5 -> 0 in .12 s, body stiffens upright .1 s, crumple 2x speed, soul-light burst upward x1.2 | Crit.Coal + `Creature.Unpaid.Snuff` |
| Kill (blast) | "torn": pelvis thrown back 35 deg + 40 cm along the shot over .2 s, strips whip forward, ember tear: 10-14 streaks 1.5-3 m along the shot, ash cone; ash splat 80 cm downrange | + `Creature.Unpaid.Tear` |
| Overkill | "scattered": body fades out in .3 s through the phase fade (`PhaseAmount`, M_Ghost: no new material), 10-16 soul tatters (ember-edged wisps) fly along the shot and spiral up 2 m; hat spins off | Tear + Creature.Hit.Overkill |

Hat (`Creatures/UnpaidHat.h/.cpp`, `FUnpaidHatFlight`, plain struct): detach `GetHat()`, velocity 350-500 along the shot + 250
up, spin 540 deg/s, gravity 980, ground trace each frame (`StaticGeometryParams`), bounce once (x0.3), lies 6 s, sinks 1 s,
hidden; reattached at respawn/`OnRespawned`; at most 4 flying (else just hidden with a dust puff). Not on Abel/Foreman (bosses).
Dead miner: head hit knocks his lamp swinging (exists `LampStruck` on lamp hits only) and no hat (has none); Bell-Ringer: bell
reactions kept.

### Meadow slime (and nothing else: placeholders keep theirs)

| Hit | Look / move | Sound |
|---|---|---|
| Body | squash also **mid-hop and in the telegraph** (telegraph: 0.5x so the leap still reads); top hit squash x1.3, side hit lean x1.3; lean 10 -> 12 deg | today's Slime.Hit |
| Mid-hop hit | hop deflected: velocity += shot dir x (150 + 400 x burst share) cm/s (cap 450), never past a ledge (hop's own ground rules) | - |
| Core crit | core shoved to the far wall of the gel (core kick 220 -> 320), gel ripple: squash oscillates 9 Hz .35 s; today's crit pop | Crit.Gel |
| Big | shotgun/Big: flattened to 0.6 + mid-hop deflection or ground shove | Creature.Hit.Heavy |
| Stagger | flattens to 0.62 and quivers 14 Hz +-0.04 for the hold, core sinks 10 cm | `Creature.Slime.Stagger` (wet wobble) |
| Wound 1 / 2 | body bone scale lerp(0.88, 1, health share) (hulls follow: fine at -12%); stage 2 core jiggle x1.3, gel drip 1 bit / 0.8 s | - |
| Kill (body) | today's splat | today's |
| Kill (core crit) | "popped": core shoots 150 cm out the back (core-colour bit), gel deflates to 0.15 over .25 s into the puddle | `Creature.Slime.Pop` |
| Kill (blast) | splattered along the shot: puddle + a second splat 60-90 cm downrange, blobs x1.5 | + Creature.Hit.Overkill |
| Overkill | over-inflates to 1.25 on the killing frame (held by the 130 ms hit-stop: an anticipation frame), then bursts: gel x1.6 | Pop + Overkill |

### Hit flash by kind (Lane D, `CreatureReactComponent.cpp`, MID `Color`, no material change)
Spider warm bone (1, .9, .7); slime pale gel (.85, 1, .8); Unpaid ember (1, .62, .32); crit keeps (1, .78, .48). Strength
unchanged. `Looter.Creatures.React.HitFlash` updated for the tints.

## Lanes (disjoint files; Main writes the contract first: the API block below, cue names, cvar names)

**Contract API (Lane A owns, others code against it):**
- `Combat/HitReaction.h`: `enum class ECreatureHitZone : uint8 { Body, Head, Abdomen, Leg, Arm, Core, Shroud, Weak };`
  `enum class EHitWeight : uint8 { Body, Heavy, Big, Kill, Overkill };` `enum class EHitSide : uint8 { Front, Left, Right, Back };`
  `FCreatureHit` += `FName Bone; ECreatureHitZone Zone = Body; EHitWeight Weight = Body; EHitSide Side = Front;`
  `float Overkill = 0.f; float LateralOffset = 0.f;` (cm, + = creature's right). `FHitReaction::EJob` += `Poise`.
  `float FHitReaction::GetStaggerAlpha(double Now) const;` `struct FStaggerCurve { static float Alpha(float Time, float Seconds); };`
- `Combat/HitFeelTuning.h` (new): `FHitFeelTuning` (`Live()`; fields = defaults: FarBoost, FarNear 1000, FarFar 2500,
  PoiseBreak, PoiseDrain, StaggerGap, BigShare .25, BigShove, OverkillShare .5, HitStopBig, HitStopOverkill, OverkillKnock
  1.5, OverkillLift 60, and switches `Zones`, `Wounds`, `KillStyles`, `Hat`, `StaggerPose` (1 = on), `SpiderScale`,
  `UnpaidScale`, `SlimeScale` (1)). Cvars `Looter.Hits.<Field>`.
- `Combat/HitPose.h` (new, plain): `struct FHitSpring { FVector Kick, Offset, Velocity; float Hold = 0.f; void Add(const FVector&
  K, float HoldSeconds); void Step(float Dt, float Omega = 40.f, float Fade = 10.f); bool IsSettled() const; };`
  `float HitEnvelope(float T, float In, float Out);` `int32 WoundStage(float HealthShare);` (0/1/2 at 0.5/0.25).
- `Creatures/CreatureHitZones.h` (new): `namespace CreatureHitZones { ECreatureHitZone ZoneOf(const ACreatureBase&, FName Bone,
  bool bCritical); int32 SpiderLegOf(FName Bone); /* -1 none; same order as ASpiderCreature's Legs */ }`.
- `UHealthComponent`: `FName GetLastHitBone() const; float GetHealthBeforeLastHit() const;` (valid during OnDamaged).
- `ACreatureBase`: `float GetStaggerAlpha() const;` (0..1, 0 for bosses) one-liner in CreatureBase.h; `FCreatureHit
  UCreatureHitReactionComponent::TakeHit(..., FName Bone = NAME_None)`.
- Cues (`Creatures/CreatureHitCues.h`, new, namespace `CreatureHitCue`): Spider.LegCrack, Spider.Stagger, Unpaid.HatOff,
  Unpaid.Stagger, Unpaid.Gutter, Unpaid.Snuff, Unpaid.Tear, Slime.Stagger, Slime.Pop, Hit.Overkill (all `Creature.` prefixed).
- FX (Lane D adds to FWeaponFX, one block in `Weapons/WeaponFX.h`): `SpawnChitinChips(Point, Normal, Size, Count)`,
  `SpawnGooDrip(Point, Tint, Size)`, `SpawnEmberTear(From, Way, Length, Tint, Count)`, `SpawnSoulTatters(Center, Way, Tint,
  Size, Count)`, `SpawnGelPop(Center, Way, Tint, Size)`, `SpawnEmberPuff(Point, Tint, Size)`; all through existing sets,
  `DetailAt`, `KillCrowdDetail`, `MaxParticles`.

**Lane A - core rules (Opus).** Files: `Combat/HitReaction.h/.cpp`, `Combat/GunfeelReactTuning.h/.cpp` (Flinch.Floor, Max),
`Combat/HitFeelTuning.h/.cpp`, `Combat/HitPose.h/.cpp`, `Combat/HealthComponent.h/.cpp` (bone + health before),
`Creatures/CreatureHitZones.h/.cpp`, `Creatures/CreatureHitReactionComponent.h/.cpp` (weight, zone, side, offset, far boost,
poise, Big shove, overkill knock/lift, `GetStaggerAlpha`), `Creatures/CreatureBase.h` (getter only),
`Creatures/CreatureBaseHurt.cpp` (pass bone), `Weapons/VfxRules.cpp` KillWeight overkill term (+0.3 -> +0.6 cap 2.4),
`Dev/HitFeelDevCommands.cpp` (new): `Looter.Hits.Test <zone> [damage] [crit] [kind]` (nearest creature, as the player's shot
from the view, a bone of that zone), `Looter.Hits.Show 1` (debug text over creatures: last zone, weight, side, poise, stage).
Tests `Tests/HitFeelTests.cpp` (new): `Looter.Gunfeel.Hits.Weight`, `.Zones` (bone names per kind), `.Side`, `.Poise`
(break, drain, rank scale, gap; SMG 10 s caps), `.StaggerCurve`, `.FarBoost`, `.Overkill`; update `Tests/GunfeelReactTests.cpp`
(`Looter.Gunfeel.React.Flinch` floor 0.85 numbers: scratch 0.85, rifle 0.97, revolver 1.37, third 1.85, cap 3.0; `.Stagger`).
PIE: `Looter.SpawnCreature Spider Basic 1`, `slomo 0.3`, `Looter.Hits.Show 1`, rifle then revolver then shotgun; log
`LogLooter Verbose` shows weight/zone per hit; `Looter.Hits.Test Leg 30` names a leg bone.

**Lane B - spiders (Opus).** Files: `Creatures/SpiderCreatureHurt.cpp` (new: zone reactions, leg buckle, stagger pose, wound
limp/drip, kill-style setup), `SpiderCreatureReact.cpp` (OnHurt hands to it; OnDied picks style), `SpiderCreature.h`
(fields: FHitSpring twist, per-leg buckle timers, leg damage tally `float LegHurt[8]`, wound stage), `SpiderCreature.cpp`
(apply in AnimateBody: twist yaw, roll, dip, stagger), `SpiderCreatureLegs.cpp` (buckle + limp in the leg pose),
`SpiderCreatureDeath.cpp` (lights-out and blast variants), `SpitterSpider.cpp` (sac wobble/drip). Tests
`Tests/SpiderHitFeelTests.cpp` (new): `Looter.Creatures.Hits.Spider.LegBuckle` (hit leg's knee drops >= 8 cm within 0.1 s,
others < 3 cm, settled < 1 cm by 0.5 s), `.StaggerPose` (body >= 7 cm lower mid-stagger), `.LightsOut` (body on the ground
< 0.3 s vs >= 0.45 s body kill), `.Limp`; `Tests/CreatureAnimTests.cpp` (this lane only): `SpiderHitJolt` bounds 3-15 cm,
step < 5.5; `SpiderDeathCurl` must pass unchanged. PIE: town (`0 -1400 90`), 3 spiders, legs then head, a shotgun kill,
a sniper overkill (`Looter.GiveWeapon`), `slomo 0.25`; one `grab.ps1` window grab of a stagger.

**Lane C - the Unpaid (Opus).** Files: `Creatures/UnpaidCreatureHurt.cpp` (new: zones, whiplash, twist, from-behind turn,
stagger fold, wound gutter, kill styles), `Creatures/UnpaidHat.h/.cpp` (new), `UnpaidCreature.cpp` (OnHurt/OnDied/OnRespawned
routing only), `UnpaidCreatureArms.cpp` (`ReactToHit` per-arm share), `UnpaidCreatureRig.cpp` (spine/chest/neck/head hit
channels, stagger fold), `UnpaidCreatureMoods.cpp` (snuffed/torn/scattered death goals), `UnpaidBodyParts.h` (new
`FHurtChannels` struct: keeps `UnpaidCreature.h` (544 lines) from growing: one member added there). Tests
`Tests/UnpaidHitFeelTests.cpp` (new): `Looter.Creatures.Hits.Unpaid.ArmFling` (hit arm >= 2x the other), `.Whiplash` (head
peaks after pelvis), `.Hat` (flies on head s >= 1.2 / kill, lands, hidden after 7 s, back at respawn, cap 4),
`.Snuffed`, `.Scattered` (phase fade to 1 within 0.35 s); must still pass `Looter.Creatures.Anim.UnpaidArms`,
`UnpaidBank`, `Looter.Creatures.Unpaid.Kit.*`. PIE: chapel yard pack (`Looter.SpawnCreature Unpaid Basic 4 chase`), revolver
heads (hats), shotgun torn kills, rifle stagger; Abel and a dead miner one hit each (no hat on Abel, nothing broken).

**Lane D - slime, flash, FX, sounds (Sonnet).** Files: `SlimeCreatureReact.cpp`, `SlimeCreature.h`, `SlimeCreatureBody.cpp`
(air deflection, wound shrink, pop/over-inflate), `Creatures/CreatureReactComponent.cpp` + `CreatureReactRules.h` (flash
tints), `Creatures/CreatureKillLook.cpp` (style routing: crit/blast/overkill layers, downrange splats; 1 Big-hit ground
splat 30-45 cm behind, hold 4 s, at most 1 per 0.5 s), `Weapons/WeaponFXHitFeel.cpp` (new: the six spawns) + its block in
`Weapons/WeaponFX.h`, `Creatures/CreatureHitCues.h` (new), `Art/Sounds/recipes/hit_feel.py` (new; 10 cues, 4-6 takes each,
script-made). Tests `Tests/HitFeelFxTests.cpp` (new): `Looter.Creatures.Hits.Slime.AirKnock`, `.Pop`, `.Shrink`;
`Looter.Vfx.Hits.Budget` (each spawn within MaxParticles share, thinned by `DetailAt` at 60 m, overkill <= 40 pieces);
update `Looter.Creatures.React.HitFlash`. Main renders `Tools\sounds.ps1 Creature.*` then `build_sound_bank.py`. PIE: west
meadow slimes: mid-hop rifle, core crit kill, shotgun splatter, sniper overkill.

**Order:** Main writes the contract (above) into `CONTRACT.md` "HITS" before launch; A-D in parallel; Main builds once,
runs `Tools\runtests.ps1 -Filter Looter.Gunfeel`, `-Filter Looter.Creatures.Hits`, `-Filter Looter.Creatures.Anim`,
`-Filter Looter.Creatures.React`, `-Filter Looter.Vfx.Hits`, `-Filter Looter.Creatures.Unpaid.Kit`; then the Demo agent's
play-check (one short PIE per lane, above). CODEMAP lines for every new file; Gunplay.md "Gunfeel: hit reactions" gets the
new layers (one place), Handoff "Decided unattended" lists every default here.

**Overlap with GUNS/FIGHTS (Main merges):** `CreatureBase.h` (FIGHTS likely; we add one getter), `UnpaidCreature.cpp/.h`
(FIGHTS kit: we touch only OnHurt/OnDied/OnRespawned + one member), `Weapons/WeaponFX.h` (GUNS muzzles/tracers: we add one
block), `Weapons/VfxRules.cpp` (KillWeight only), `GunfeelReactTuning` (GUNS should not need it; if it does, Lane A owns it).
`BulletSubsystem` untouched (the bone comes through `HandlePointDamage`).

## Perf (120 fps 1080p Medium, RX 580; margin ~0.2 ms; render-thread bound by draws)

- No new draws: poses ride existing skinned meshes (game-thread bone math: ~10 springs per creature, stepped only while
  unsettled; budget <= 0.05 ms for 10 fighting); the hat is the existing component moving (no new mesh); FX all instanced
  FWeaponFX quads/bits under `MaxParticles` 1000, `DetailAt` (x0.35 at 60 m), `KillCrowdDetail`, blast budget; no lights,
  no new materials, no Niagara, no physics/ragdoll (Nanite fallback unaffected).
- Splats: shared pool (`Splat.Max` 12); downrange/Big-hit splats take the oldest; Big-hit splat <= 2 per second world-wide.
- Wound drips/puffs only within 25 m of a player, <= 6 creatures dripping at once.
- Measure (editor closed, ArtPause on): `Tools\perf.ps1 -Label "HITS" -Exec "Looter.Quality Medium"` with
  `Looter.Perf.Horde Spider 8 Basic 0 -1400 90` and `... Unpaid 8 Basic 0 -1400 90` against 7.6 / 7.7 ms (p95 8.7 / 9.4).
  Knobs if over: `Looter.Hits.Wounds 0`, `Looter.Vfx.Kill.Scale 0.8`, `Looter.Hits.KillStyles 0`.

## Risks

- Bigger flinches push arms/hulls into bodies: `UnpaidArms` arm guard, `SpiderDeathCurl`, `SpiderBiteRaise` must pass; the
  twist adds yaw the guard never saw (cap 22 deg; reduce first if it fails).
- Easier fights: more staggers fight FIGHTS' "clearly harder". Guards: poise rank scale, 1.6 s gap on elites, stagger
  seconds unchanged, `GiveBackForHit` unchanged. Watch stagger count per fight in the play-check; `Looter.Hits.PoiseBreak 0.35`
  if packs read stun-locked.
- Hit-stop overkill 130 ms on a corpse while its knock waits: fine; on the slime the over-inflate relies on it (cvar 0 = no
  anticipation frame, still bursts).
- Shotgun pellets: each pellet hits a zone; only the burst's strongest zone kicks (2 zone kicks per 0.08 s burst at most).
- Test levels deal damage through `OnTakeAnyDamage` (no bone): tests pass the bone to `TakeHit` directly.
- Gravemother's pose override vs leg buckle (held front legs skipped); Abel/Foreman through the Unpaid path: `IsBoss` guards
  hat, stagger pose, kill styles.
- Phase fade for "scattered" shares `PhaseAmount` with the phase-step: a dead Unpaid never steps (kit reset in OnDied).
- Hat flying through walls: trace each frame, stops at the hit, else hides.

## Questions for the user (Main answers unattended overnight with the first option)

1. Spider leg shots: **(a) looks only: the leg buckles, it limps when hurt (recommended)** / (b) also slows it 20% for 3 s
   per 25% of its health taken in the legs (a tactic) / (c) (b) plus an overkill knocks a leg clean off (stylized chitin).
2. Unpaid hats: **(a) knocked off by hard head hits and every head kill, lie 6 s (recommended)** / (b) only on kills / (c) never.
3. How strong overall: **(a) as planned: big and readable at 25 m (recommended)** / (b) bigger still, more staggers (fights
   get easier) / (c) subtler: half the new increase.
