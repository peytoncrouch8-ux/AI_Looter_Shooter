# Elements matter (2026-10-29 night)

Design and the user's verbatim answers: Docs/Archive/Work/2026-10-29-NewElements/ELEMENT_DESIGN.md (read it). Code names
stay: Ember = Cinder, SoulLight = Hallow, GraveSalt = Wardbreak, Thunder, Rot (player words via ElementRules::Word).

## The user's rulings (2026-10-29 night)
- **Bullets scale too, like Borderlands**: an element gun's hit damage AND its status scale by the target's affinity to
  the gun's element: weak x2, resist x0.33, immune: the hit x0.1 and no status at all. (Tuning: `Weak` 2, `Resist` 0.33,
  new `ImmuneHit` 0.1.) A plain gun's hits never scale.
- **Typed wards**: a ward's element; damage into it: its matching element x2, Wardbreak x1.5 on any other ward, anything else
  x1 (the larger, never multiplied together); then the crit's x2 as today; the old salted-ward x1.5 is REMOVED. No status
  sets through a standing ward. A break always staggers (ignoring the stagger cooldown) and keeps the daze. The ward bar sits
  above the health bar in the ward's colour (UiColor). Roll: Thunder 0.5, Wardbreak 0.2, Cinder / Hallow / Rot 0.1 each; a
  Touched creature's ward is its own element.
- **Spirit adds on top**: Ethereal rank (with Phasing) = a spirit: immune to Cinder, Rot, Thunder, weak to Wardbreak and
  Hallow, on top of its kind's body (an Ethereal spider keeps chitin's other weaknesses; immune wins). Warded no longer
  counts as a spirit.
- **Touched in Ransom's Rest**: rare (about 6% of Basic, Restless, Gravebound; at most 3 alive), named "<Word>-touched"
  ("LV 3 Restless Cinder-touched Brown Spider": Word = Cinder, Hallow, Wardbreak, Thunder, Rot); never in the spider fights,
  set pieces or bounties (`bMayBeTouched` false there).

## Bodies (EElementBody becomes a UENUM in Elements/ElementBody.h; lane A)
`enum class EElementBody : uint8 { Flesh, Chitin, Gel, Bone, Coal, Spirit, Stone, Iron }` (Flesh was Other, Bone was
Undead, Coal was Unpaid). Table (weak / resist / immune):
Flesh: Thunder, Rot, Ember / - / -. Chitin: Ember, Rot / GraveSalt / -. Gel: Ember, Thunder / SoulLight, Rot / -.
Bone: SoulLight / Thunder / -. Coal: SoulLight (+ GraveSalt on the coal crit) / Ember, Thunder / -.
Spirit: GraveSalt, SoulLight / - / Ember, Rot, Thunder. Stone: GraveSalt, Thunder / Ember / Rot. Iron: Thunder, Rot / Ember / GraveSalt.
Order: immune wins, then weak, then resist. Story immunities (bosses): Gravemother Rot, Foreman Ember, Abel SoulLight.

## Shared names (code against these exactly; lane A lands them)
- `Elements/ElementBody.h`: the UENUM above.
- `ACreatureBase` (CreatureBase.h): `UPROPERTY(EditDefaultsOnly, Category = "Elements") EElementBody ElementBody = EElementBody::Flesh;`
  `EElementBody GetElementBody() const`; `ELooterElement GetTouchedElement() const` (None = not touched);
  `UPROPERTY(EditAnywhere, Category = "Elements") bool bMayBeTouched = true;`; `void UpdateStatusBrain(float DeltaSeconds);`
  (declared by A, called by A from TickBrain, DEFINED by lane R in Creatures/CreatureBaseStatus.cpp).
- `EElementAffinity` gains `Immune`. `FElementTarget` gains `ELooterElement Touched`, `uint8 ImmuneMask`, `uint8 WeakMask`.
- `ElementRules`: `float HitScale(EElementAffinity, const FElementTuning&)` (the bullet's multiplier), `uint8 MaskOf(ELooterElement)`,
  `ELooterElement StoryImmunityOf(const UClass*)`, `FString StatusWord(ELooterElement)` ("BURNING", "SMITTEN", "SALTED",
  "JOLTED", "ROTTING").
- `Elements/ElementStatusIcons.h`: `struct FCreatureStatusIcon { ELooterElement Element; float TimeShare; /* 0..1 left */ int32 Stacks; };`
  `UElementStatusComponent::GetStatusIcons(TArray<FCreatureStatusIcon>& Out) const`.
- `UElementStatusComponent`: `DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnElementStatusSet, AActor* /*Target*/, ELooterElement, bool /*bFirstOnThisCreature*/);`
  `static FOnElementStatusSet OnStatusSet;` (broadcast in Apply). Status queries kept: IsBurning, IsSmiting, IsSalted,
  IsJolting, IsRotting, GetStatus().
- `UCreatureTraitComponent`: `ELooterElement GetWardElement() const` (None without a ward).
- `UCreatureHitReactionComponent::WardBreak(const FVector& Direction)`.
- `Creatures/CreatureTouched.h`: `namespace CreatureTouched { ELooterElement ElementFor(const UClass* Kind); ELooterElement CounterOf(ELooterElement);
  FString Word(ELooterElement) /* "Cinder-touched" */; }` (kinds: spider Ember, slime Thunder, dead miner Rot, Unpaid GraveSalt,
  Bell-Ringer SoulLight, Sentinel none).
- `Creatures/CreatureStatusPose.h` (lane R): `struct FCreatureStatusPose { float Burn = 0, Jolt = 0, Rot = 0, Salt = 0, Smite = 0; float ClockScale = 1; };`
  `namespace CreatureStatusPose { FCreatureStatusPose Of(const AActor* Creature); }`.
- Health bar (lane B): `UCreatureHealthBarWidget::SetWard(bool bShown, float Fill, const FLinearColor& Color)`,
  `SetStatuses(TConstArrayView<FCreatureStatusIcon> Icons)`; lane C's DressTag calls both every tag update.
- Loot (lane C): `WeaponParts::Pick(..., ELooterElement Lean = ELooterElement::None)` x3 weight on that element's parts;
  `ULootDropComponent` leans to its owner's GetTouchedElement().
- Sounds (lane E adds to Audio/LooterSoundCues.h, only E edits it): `Creature.Sentinel.Wake/Step/WindUp/Slam/Chip/Crumble`,
  `Element.<Ember|SoulLight|GraveSalt|Thunder|Rot>.WardBreak`, `UI.StatusCallout`.
- Stone Sentinel: `AStoneSentinelCreature : ACreatureBase` (lane D; Creatures/StoneSentinelCreature*.cpp/.h,
  Creatures/StoneSentinelRules.*), body Stone, meshes `/Game/Creatures/StoneSentinel/SK_StoneSentinel` and
  `SM_StoneSentinelPlinth` (lane E's model, bone names: root, pelvis, spine_01, spine_02, head, upperarm_l, lowerarm_l,
  hand_l, upperarm_r, lowerarm_r, hand_r, thigh_l, calf_l, foot_l, thigh_r, calf_r, foot_r), sockets SOCKET_Head (crit),
  SOCKET_FistL, SOCKET_FistR. Bestiary page `DA_Bestiary_StoneSentinel` (E's script, Main runs it).

## Lanes (disjoint files)
- **A rules (Opus)**: Elements/ (ElementBody.h, ElementRules, ElementTuning, ElementStatus, ElementStatusComponent,
  ElementStatusIcons.h, ElementCallout incl. "NO EFFECT" and "IMMUNE TO" in WeaknessLine), the bullet's hit scaling (find
  where an element gun's hit deals damage: Combat/BulletSubsystemImpacts.cpp or the creature damage path), CreatureBase.h /
  CreatureBaseRank.cpp (Touched roll, name word via CreatureTouched, ElementBody), CreatureTouched.*, the kinds'
  constructors (one line each: ElementBody), CreatureTrait* (typed ward), HitReaction / CreatureHitReactionComponent
  (WardBreak), PartEffectSubsystem (remove the salted x1.5), ShieldRules (nova of 5 elements), AreaDefinition
  (TouchedChance, MaxTouchedAlive), Dev commands for spawning touched, and the tests (Elements.*, CreatureTrait*,
  DamageNumbers element facts, shield nova, new Elements.Bodies/.Immune/.Ward, Creatures.Touched).
- **B UI (Opus)**: UI/World/CreatureHealthBarWidget* (ward bar above the health bar in the ward colour, status icons with a
  draining ring timer under it using GunCardIcons::ElementBrush glyphs), HudHitMarker / PlayerHUDWidgetHitMarker (ward marks
  tinted by ward element), DamageNumber actor/widget (first-time status callout via OnStatusSet, a DoT stream: one rolling
  number per creature per element), UI/Bestiary detail lines (IMMUNE TO, the body word); test UI.StatusIcons.
- **C Touched look + loot (Sonnet)**: Creatures/CreatureTierLook*, CreatureTierLookComponent* (DressTag: name with the
  Touched word, SetWard/SetStatuses calls), CreatureTierFXSubsystem* (shell worn at Basic for Touched, glow FxColor(Touched)),
  Loot/LootDropComponent, Loot/LootLibrary, Weapons/WeaponParts.cpp (the lean).
- **D Sentinel code (Opus)**: Creatures/StoneSentinelCreature*, StoneSentinelRules*, its CreatureTraitRules kind hook only via
  A's names, Dev/CreatureDevCommands.cpp (spawn it), test Creatures.Sentinel.
- **E Sentinel art + sounds (Opus)**: Art/Models/Creatures/StoneSentinel.py (+ plinth; original hooded mourner statue ~2.4 m,
  existing stone texture set, moss, cracks, the armature above; Art/README rules), Tools/Unreal/build_area_sentinels.py
  (2 on Boot Hill, 2 in the churchyard's west rows, outside Saint Ada's Yard; after Main 4), the bestiary page script,
  Art/Sounds/recipes/sentinel.py + the WardBreak variants + UI.StatusCallout in the right recipe file, LooterSoundCues.h names.
  Render sounds with Tools\sounds.ps1 <cues>; export the model with Tools\artrun.ps1 -Export ... -NoImport style only
  (Main imports).
- **R reactions (Opus)**: Creatures/CreatureStatusPose.*, Creatures/CreatureBaseStatus.cpp (UpdateStatusBrain: a burning
  Basic/Restless may panic-run 1.5 s, Looter.Element.BurnPanic; never a boss, Soulfed or a spider), each kind's pose blend
  (SpiderCreatureHurt, SlimeCreatureBody, UnpaidCreatureMoods, DeadMinerCreaturePose, BellRingerCreaturePose,
  PlaceholderRules/PlaceholderCreatureMoves): Burn flail, Jolt lock-up twitch, Rot hunch and cough, Salt stiff (slower clock),
  Smite flinch; test Creatures.StatusPose.
- **F firing + world looks (Opus)**: Weapons/WeaponBaseMuzzle.cpp, VfxRulesMuzzle.cpp (flash tinted toward the element
  with its sparks), the tracer trail per element (Weapons/WeaponFXShots.cpp / the tracer code), the held gun's idle element
  motes (WeaponModelElements.cpp, first person only), element bullet marks (Weapons/BulletMarks* +
  Art/Textures/BulletMarks/make_bullet_marks.py new rows; Main imports the texture), bigger element deaths
  (ElementDeathLook.cpp, WeaponFXElementBursts.cpp: ash, shock pop, melt, crystallised, burst of light).

## Rules for every lane
Never build, commit, open/close the editor, run tests or the game, or edit CODEMAP.md / Docs. Only your own files; read
anything. Code against the shared names exactly (lane A lands them; small mismatches Main fixes at build). Keep every public
name others or tests use (grep Tests/). The spiders' fight balance stays untouched (the user's rule). Files under ~500
lines. Comments say why, plain words. Tabs. Include what you use (no unity build). Colours only from LooterUI::Color in UI.
Report: files changed/added (for CODEMAP), new cvars, tests changed/added, anything unsure.
