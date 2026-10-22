# Proposal for GUNS-CODE (and the tracer's owner): muzzles, the rocket's blast, tracers

VFX wrote no line in these files (they're GUNS-CODE's / LOOT-CODE's tonight). Everything below calls code that lives in
the VFX holding tree (`FWeaponFX::SpawnMuzzleBurst`, `SpawnExplosion`, `VfxRules::MuzzleLook`, `UVfxSubsystem::AddSplat`),
so it builds only after `Saved/VfxCode` is merged. Numbers per kind are `VfxRules::MuzzleLook` (one place; tests
`Looter.Vfx.Rules.Muzzles`).

## 1. `Weapons/WeaponBaseFiring.cpp`: a muzzle flash per gun kind

Today every kind draws the same 5 quads (core 11, star 34, three tongues 26 long) for 0.055 s, sized only by
`MuzzleFlashScale`. Change:

1. `SetupMuzzleFlash()`: read `const FVfxMuzzleLook Look = VfxRules::MuzzleLook(Definition->Kind);` and build the quads from
   it instead of the file's constants:
   - core `Look.CoreSize` (shape 1), star `Look.StarSize` (shape 2) - unchanged code, new sizes;
   - `Look.Tongues` tongues (was 3) spread `180 / Look.Tongues` degrees apart, `Look.TongueLength` x `Look.TongueWidth`;
   - `Look.SideJets > 0` (sniper's brake): two more streak quads square to the barrel, `FRotator(0, +-90, 0)`, length
     `Look.SideJets`, width 8, at X = -3 (the brake sits just behind the crown), `FlashTongueColor`, glow 16;
   - `Look.GapSpit > 0` (revolver): two small round quads (shape 1) at X = -16, Y = +-2, size `Look.GapSpit`, glow 12.
   Keep `FlashGlow` per quad as today (UpdateMuzzleFlash fades them all).
2. `PlayMuzzleFlash()`: `FlashTimeLeft = Look.Duration` (store `FlashDurationNow` from Setup; was `FlashDuration`), and
   after `UpdateMuzzleFlash(0.f)`:
   ```cpp
   if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
   {
       FVfxMuzzle Muzzle;
       Muzzle.Location = GetVisibleMuzzleLocation();
       Muzzle.Forward = MuzzleFlash->GetForwardVector();      // the barrel's +X as drawn
       Muzzle.Up = MuzzleFlash->GetUpVector();
       Muzzle.Kind = Instance.Definition ? Instance.Definition->Kind : EWeaponKind::Rifle;
       Muzzle.Scale = Instance.Definition ? Instance.Definition->MuzzleFlashScale * (bUsingModel ? Model->GetModelScale() : 1.f) : 1.f;
       Muzzle.bFirstPerson = IsDrawnFirstPerson();
       Muzzle.InBurst = ShotsInBurst;                        // new: ++ each shot, 0 after a 0.35 s pause (the rifle climb's rule)
       if (UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this)) { Muzzle.DustLight = Reacts->GetDustLight(); }
       Bullets->GetEffects().Initialize(GetWorld());
       Bullets->GetEffects().SpawnMuzzleBurst(Muzzle);
   }
   ```
   (`MuzzleFlash` is turned at random about X each shot; its forward stays the barrel's. In first person
   `GetVisibleMuzzleLocation` is where the flash shows; the burst keeps its smoke 45 cm out and leaves the ring out.)
3. The existing `MuzzleLight` stays as is (no new lights). `Muzzle.Scale1P` (1.4) still scales the quads in first person.

What each kind gets (SpawnMuzzleBurst + the quads): Rifle as today plus a spark and a breath of smoke every other shot;
Shotgun a wide 5-tongue bloom, 9 powder sparks, 3 smoke puffs; Revolver a hard 8-point star (M_FX_Glow's star is 6 points:
fine), cylinder-gap spit both sides; Rustler SMG small 4-point flicker for 0.035 s, sparks/smoke every 3rd round; Longhorn
sniper big star, brake side jets with their smoke rolling sideways, a blast ring of smoke and a faint hot-air ring square to
the barrel (third person only), 4 hanging puffs; Thresher LMG heavy 4-tongue flash, smoke every 2nd round (its barrel heat
wisps are WeaponBaseSmoke's); Powderkeg the motor's gout (12-spike star, 4 fat tongues), a 200 cm blast ring, 5 smoke puffs
(the back-blast stays `WeaponRocketBlast::SpawnBackblast`).

Cost: 3-14 particles a shot (thinned by `Every` on the fast guns: an SMG's 15 rounds/s throw ~7 particles/s more), one more
shape quad for the sniper/launcher. Cvar `Looter.Vfx.Muzzle.Scale` (0 = off).

## 2. `Weapons/WeaponRocketBlastEffects.cpp`: the rocket's burst

Replace `SpawnEffects`' body (flash 4.5, SpawnDeathBurst SoulLight 260 as the fireball, dirt, 12+5 puffs, 16 grit) with:
```cpp
FVfxExplosion Blast;
Blast.Center = Center;
Blast.Up = Up;
Blast.Radius = GunKinds::RocketSplashRadius();   // or the 350 the rocket uses
Blast.Fire = FireTint;
Blast.Ground = Hit && Hit->bBlockingHit ? ImpactSurface::Of(*Hit) : EImpactMaterial::Unknown;
Blast.bOnGround = FVector::DotProduct(Up, FVector::UpVector) > GroundUp && Blast.Ground != EImpactMaterial::Unknown;
if (UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&World)) { Blast.DustLight = Reacts->GetDustLight(); }
Effects->SpawnExplosion(Blast);
if (Blast.bOnGround) { if (UVfxSubsystem* Vfx = UVfxSubsystem::Get(&World)) { Vfx->AddSplat(Center, Up, EVfxSplat::Scorch, FLinearColor::Black, Blast.Radius * 0.6f); } }
```
It gives: a white 14-spike crack, 5 fireballs blooming white-hot -> ember orange -> dull red and eroding (M_FX_Shape), a ring
racing over the ground to 1.6x the reach, a faint shock in the air, a soot skirt and a 3 s column, 22 embers arcing and
skittering, the ground thrown up in its own look (dirt geyser, stone chips...) and a scorch decal that holds 9 s and fades.
About 90 particles + 8 shapes + 1 decal; `Looter.Vfx.Blast.Scale`. `DrawFlame`: raise the motor glow to 34 cm / 14 and add a
`SpawnFireball(Exhaust, -Way * 300, 10, 30, FlameColor, 10, 0.12)` every `SmokeEvery` beside the puff (a flame you can see
from behind the launcher).

## 3. Tracers (`Combat/BulletSubsystem.cpp` `DrawTracer`, LOOT-CODE's hooks file; Main's call who edits)

Per kind, through `Shot.TracerWidth/TracerLength/TracerColor` set where the shot is made (WeaponBaseFiring `FBulletShot`):
SMG width x0.75, length x0.8 (a fast stream of thin needles); LMG every 3rd round a full tracer, the others x0.5 glow
(`TracerColor * 0.5`): the classic belt read; sniper width x1.6, head white-hot x1.4, and its smoke thread x2.5 wide, 2.5x
life (a vapour trail that hangs: the sniper's signature); revolver x1.2 width; shotgun pellets length x0.6. Add a round head
glow on every tracer farther than 15 m from the camera (`Effects.AddGlow(Head, Width * 6, HeadColor, TracerGlow * 0.6f)`) so
a shot reads as a dot of light at range. Cost: one more quad per tracer in flight past 15 m.
