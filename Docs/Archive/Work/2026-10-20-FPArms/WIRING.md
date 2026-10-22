# Wiring the heroes' first-person arms (for Main)

Four meshes, `SK_FP_Arms_Odessa`, `SK_FP_Arms_Crane`, `SK_FP_Arms_Gauge`, `SK_FP_Arms_Pike` (`/Game/Art/Characters/`),
from `Art/Models/Characters/FPArms<Hero>.py` (thin entries) on `Tools/Blender/fp_arms_heroes.py` with
`fp_arms_base.py` (FPArms.py's definitions read without running its build), `fp_arms_hands.py`, `fp_arms_cloth.py`,
`fp_arms_paint.py`, `fp_arms_gauge.py`; look sheets `Tools/Blender/fp_arms_heroes_sheet.py <Hero>` into
`Saved/ArtPreviews/Characters/FPArms_<Hero>/` (sheet.jpg). Ellis keeps `SK_FP_Arms`; FPArms.py and every shared helper
are untouched.

| Hero | Triangles | Bones | Slots (master) | Texture sets (2048) |
|---|---|---|---|---|
| Odessa | 11,676 | 46 | FPArmsOdessa_Glove (M_Character), FPArmsOdessa_Coat (M_CharacterCloth) | FPArmsOdessaGlove, FPArmsOdessaCoat |
| Crane | 10,724 | 46 | FPArmsCrane_Skin (M_CharacterSkin), FPArmsCrane_Coat (M_CharacterCloth) | FPArmsCraneSkin, FPArmsCraneCoat |
| Gauge | 17,708 | 46 | FPArmsGauge_Iron (M_Character, RoughnessBreakup 0.06) | FPArmsGauge |
| Pike | 10,412 | 46 | FPArmsPike_Skin (M_CharacterSkin), FPArmsPike_Coat (M_CharacterCloth) | FPArmsPikeSkin, FPArmsPikeCoat |

(SK_FP_Arms is ~8k. Gauge's riveted casings cost more; they're one skinned draw, no LODs, as SK_FP_Arms.)

## What the code can rely on (no per-hero rig data)

- Every hero's skeleton is SK_FP_Arms's bone for bone: the same 46 bones and parents, root at the eye facing +X, the
  same bind pose. `Intermediate/FPArms/FPArms_<Hero>_rig.json` has FPArms_rig.json's keys and, checked, identical values
  (only `note` differs).
- So `FFirstPersonHandsRig::Bind` gives the same frames, grip points (`HoldAlongKnuckles`, `HoldTowardPinky`,
  `GripOffPalm` stay right), curl axes and rest curls for all five: `FirstPersonHandsGrips`' tables, the reload and throw
  motions and `FHandFingers::Of` work unchanged.
- Hands were kept to the bones: every palm's underside is within 2 mm of Ellis's (the 3.6 cm grip offset holds);
  finger girth 0.8 (Crane) to 1.0 (Pike) of Ellis's gloved fingers, so the game's fixed curls leave at most a hair's gap
  round a grip and never close through it. Gauge's blocks are 2.8 x 2.5 cm (a little over a glove's 2.2-2.4): his
  fingers may sink 2-3 mm into a thin grip at the fixed curls; tune only if it shows.
- Gauge: thumb on `thumb_*`, his two fingers on `index_*` and `middle_*` (their blocks moved 6 and 7 mm apart along
  their curl axes, so the curls still bend them; his third-person bones are spread instead, which SK_FP_Arms' skeleton
  can't be); `ring_*` and `pinky_*` exist and carry nothing. The trigger finger is `index_*` as on the others.

## The one per-hero number: the arms' size

`UFirstPersonHandsComponent::ArmsScale` (0.88, the user's for Ellis). Suggested, and what the look sheets draw: Ellis
0.88, Odessa 0.84 (slight), Crane 0.88, Gauge 0.88 (his casings already built near Ellis's sleeve size), Pike 0.86
(short, stocky). Grip spots stay exact at any scale (the component scales the targets into the mesh).

## The C++ change

1. `Embers/HeroDefinition.h` (data, beside `FigureMesh`):
   ```cpp
   /** The hero's first-person arms (`/Game/Art/Characters/SK_FP_Arms_<Id>`; Ellis's is SK_FP_Arms). */
   UPROPERTY(EditDefaultsOnly, Category = "Hero|Looks")
   TSoftObjectPtr<USkeletalMesh> FirstPersonArms;

   /** Their size against the guns (UFirstPersonHandsComponent::ArmsScale); 0 keeps the component's (0.88). */
   UPROPERTY(EditDefaultsOnly, Category = "Hero|Looks", meta = (ClampMin = "0", ClampMax = "1.3"))
   float FirstPersonArmsScale = 0.f;

   FSoftObjectPath GetFirstPersonArmsPath() const;
   /** The arms, loaded when they're in the game; null while they aren't (no load warning then). */
   USkeletalMesh* LoadFirstPersonArms() const;
   ```
   `HeroDefinition.cpp`, beside `GetFigureMeshPath` (Ellis's mesh has no suffix):
   ```cpp
   FSoftObjectPath UHeroDefinition::GetFirstPersonArmsPath() const
   {
       if (!FirstPersonArms.IsNull())
       {
           return FirstPersonArms.ToSoftObjectPath();
       }
       return Id == DefaultId() ? FSoftObjectPath(TEXT("/Game/Art/Characters/SK_FP_Arms.SK_FP_Arms"))
           : NamedPath(TEXT("/Game/Art/Characters"), TEXT("SK_FP_Arms_"), Id);
   }

   USkeletalMesh* UHeroDefinition::LoadFirstPersonArms() const
   {
       return LoadIfThere<USkeletalMesh>(GetFirstPersonArmsPath());
   }
   ```
   `create_ember_assets.py`: `first_person_arms_scale` per hero (Odessa 0.84, Pike 0.86, the rest 0.88 or 0); the path
   can stay empty (the naming rule finds the mesh), as for the portraits.
2. `Player/FirstPersonHandsComponent.{h,cpp}`:
   - `LoadArms()`, before `LoadIfMade(ArmsMesh)` (includes `Embers/HeroDefinition.h`, `Session/SessionSubsystem.h`):
     ```cpp
     USkeletalMesh* Arms = nullptr;
     HeroScale = 0.f;
     if (const UHeroDefinition* Hero = USessionSubsystem::FindPlayedHero(this))
     {
         Arms = Hero->LoadFirstPersonArms();          // null until that hero's arms are imported: Ellis's below
         HeroScale = Arms ? Hero->FirstPersonArmsScale : 0.f;
     }
     if (!Arms)
     {
         Arms = LoadIfMade(ArmsMesh);
     }
     bStandIn = Arms == nullptr;
     ...
     ```
     A hero without arms yet wears Ellis's (as `UBodyFigureComponent::PickBody` does for bodies).
   - `float HeroScale = 0.f;` member; `GetWantedScale()`: `Tuning > 0 ? Tuning : (HeroScale > 0 ? HeroScale : ArmsScale)`.
   - `void WearHero()` (public): `if (LoadArms()) { MarkRefreshTransformDirty(); }` (rebinds the rig, sets the mesh,
     re-places it on the camera). `Dev/HeroDevCommands.cpp`'s `WearHeroNow` calls it on every
     `TObjectIterator<UFirstPersonHandsComponent>` in the game world, as it does `UBodyFigureComponent::WearHero`.
     A session's hero is known before the pawn's BeginPlay, so a normal start needs nothing more.
3. Test `Looter.Player.Hands.HeroArms` (FirstPersonHandsTests.cpp or HeroTests.cpp): for each roster id whose arms
   load, `Rig.Bind(Arms, false)` succeeds, faces +X, its right grip point and palm match SK_FP_Arms's (within 0.1 cm and
   0.01), and `Looter.Player.Hands.Arms`' hip-hold pose puts both hands on their spots.
4. CODEMAP: the `HeroDefinition` and `FirstPersonHandsComponent` lines (the arms per hero). Pipeline, under
   "First-person hands": the heroes' arms share SK_FP_Arms' skeleton, per-hero ArmsScale, built by fp_arms_heroes.py
   (each hero's own looks baked from their model's materials, then fitted to first person: bumps scaled, colour
   softened), checked with fp_arms_heroes_sheet.py.

## Import

Each hero is its own export family (one FBX, one manifest, its own sets); all four export tests pass:

```powershell
powershell -NoProfile -File Tools\artrun.ps1 -Export FPArmsOdessa -Family FPArmsOdessa
powershell -NoProfile -File Tools\artrun.ps1 -Export FPArmsCrane -Family FPArmsCrane
powershell -NoProfile -File Tools\artrun.ps1 -Export FPArmsGauge -Family FPArmsGauge
powershell -NoProfile -File Tools\artrun.ps1 -Export FPArmsPike -Family FPArmsPike
# editor open, per hero:
powershell -NoProfile -File Tools\console.ps1 "Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_FPArmsOdessa" -Until "Looter\.ImportModels: imported"
```

Each import makes `SK_FP_Arms_<Hero>` (+ its `_Skeleton`; the physics asset is unused, the component turns collision
off) and `MI_<slot>` per slot above, Nanite off. An export bakes again only when a script, a look module or the hero's
model script changed (`Art/Textures/FPArms<Hero><Set>/bake.digest` in the hero's first set; `-ScriptArgs rebake` forces
it; the bake is deterministic, ~3 min).
