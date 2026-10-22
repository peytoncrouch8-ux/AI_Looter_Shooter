# LOOT-CODE holding tree (Phase 12.3, phase 1)

Base commit when copied: `Originals/BASE_COMMIT.txt` (c0a1f3c). `Originals/` holds the untouched copies of every existing
file edited here: in phase 2 diff each against today's Source copy (`git diff --no-index Originals/X Source/.../X`) and
merge if it moved, then copy `Source/` over the real tree.

## New files (Source/AI_Looter_Shooter/...)
- Elements/LooterElement.h, ElementTuning.h/.cpp (Looter.Element.*), ElementRules.h/.cpp (table, chances, words,
  colours, cues, damage types, WeaknessLine), ElementDamageType.h (UElementDamageType + Ember/SoulLight; Grave-salt uses
  UGraveSaltDamageType), ElementStatus.h/.cpp (pure state), ElementStatusComponent.h/.cpp + ElementStatusComponentLooks.cpp
- Weapons/PartEffect.h, PartEffectTuning.h/.cpp (Looter.PartEffect.*), PartEffectRules.h/.cpp, PartEffectComponent.h/.cpp,
  PartEffectSubsystem.h/.cpp, WeaponFXElements.cpp
- Loot/LootDropMoments.h/.cpp (Looter.Loot.Drop.*), Loot/LootFanfareShows.cpp
- Tests/ElementTests.cpp, PartEffectTests.cpp, DropMomentTests.cpp

## Edited copies
Combat/BulletSubsystem.h/.cpp (FBulletShot fields, FBulletStrike, OnBulletStruck, FireSoon/FireHeld, pass-through),
Weapons/WeaponParts.h/.cpp (option Effect/Element, EffectOf/ElementOf), WeaponTrigger.h/.cpp (FTriggerPlan, PlanFor),
WeaponFX.h (element/loot declarations), Loot/LootFanfareSubsystem.h/.cpp, UI/World/WeaponLabelWidget.h/.cpp (trait line,
SetPop), UI/Style/WeaponText.h (ElementString/EffectString/TraitLine/TraitColor), UI/HUD/PlayerHUDWidgetPickupCard.cpp
(trait rows), UI/Style/LooterUIStyle.h/.cpp (Color::ElementEmber/SoulLight/GraveSalt).
In place: the seven Art/Models/Weapons/*.parts.csv (Effect, Element, Model columns + rows), Tools/Unreal/setup_gun_parts.py.

## Hooks in files LOOT-CODE doesn't own (Main or GUNS-CODE)
1. WeaponBaseFiring.cpp: `const WeaponTrigger::FTriggerPlan Plan = WeaponTrigger::PlanFor(Instance);`
   - StartFireAt: Interval x Plan.PullIntervalScale; Decide(Plan.FireMode, ...); Burst -> BurstShotsRemaining = Plan.BurstCount;
     FireNow with Plan.ChargeSeconds > 0: `UPartEffectComponent::For(*this)->BeginCharge();` and
     `SetTimer(FireTimer, HandleFiring, Plan.ChargeSeconds / GetOwnerTimeScale())` instead of HandleFiring().
   - StopFire: a charging gun (`UPartEffectComponent::Find(this)` IsCharging and FireTimer active) clears the timer and
     calls HandleFiring() (fires at the charge reached); full-auto test reads Plan.FireMode.
   - HandleFiring: switch on Plan.FireMode; a burst's next round at interval x Plan.BurstIntervalScale.
   - Reload / holster / drop: `if (auto* P = UPartEffectComponent::Find(this)) P->CancelCharge();`
   Without these, Burst and Overcharge parts do nothing (their CSV rows still roll, names and cards show).
2. WeaponBase.cpp RefreshLootBeam: Height/Glow/Radius from `LootDropMoments::For(Instance.Rarity)` (BeamHeight, BeamGlow,
   BeamRadius) in place of the Heights table (taller beams each tier).
3. Rocket (GUNS-CODE): per creature in the blast, `UElementStatusComponent::TryApply(*Victim, Element, Chance, Damage, false,
   FMath::FRand(), Where, Gun, Instigator)` with Element/Chance from `UPartEffectComponent::For(*Gun)` (Powderkeg rockets:
   Incendiary = Ember, Saintfire, Saltburst).
4. Optional, Main's call: CreatureHitReactionComponent::KindOf ignores `UElementDamageType`/`UGraveSaltDamageType` (a burn's
   kill today gets its gun's kill knock); bestiary page shows `ElementRules::WeaknessLine(ActorClass)`; GraveSaltBurst sets the
   salted status (`UElementStatusComponent::FindOrAdd(...)->Apply(GraveSalt, Damage, Affinity, ...)`) and scales by
   `ElementRules::AffinityScale`; damage numbers in `ElementRules::UiColor(ElementOf(DamageType))`; UWeaponModelComponent tints
   GunAccentGlow toward `ElementRules::FxColor(WeaponParts::ElementOf(Instance))` (element parts reuse models).

## Phase 2 run order
Merge tree -> `Tools\launch.ps1 -Build` -> `py Tools/Unreal/setup_gun_parts.py` (after create_new_guns.py; logs GUNPARTS done)
-> `create_rank_assets.py` -> tests `Looter.Elements`, `Looter.PartEffects`, `Looter.Loot.DropMoments`, plus
`Looter.Weapons.Parts`, `Looter.Weapons.Trigger`, `Looter.Feedback.LootFanfare` (touched) -> PIE: `Looter.GiveWeapon` with
element/effect keys (Magazine=Ember30, Barrel=Caromer...) vs spiders/Unpaid; a Legendary drop (FeedbackDevCommands).

## CODEMAP lines (add in phase 2)
- `Elements/LooterElement.h`: `ELooterElement` (None, Ember, SoulLight, GraveSalt).
- `Elements/ElementRules`: `ElementRules` (weakness table by `EElementBody`, `Affinity`, chances, words, colours, cues, damage types, `WeaknessLine`). `Elements/ElementTuning`: `FElementTuning`, `Looter.Element.*`.
- `Elements/ElementStatus`: `FElementStatus`, burns/smites/salt as numbers. `Elements/ElementStatusComponent`: `UElementStatusComponent` (runtime on a creature: `TryApply`, damage pieces, smite leap, salt slow, sounds; `...Looks.cpp`).
- `Elements/ElementDamageType.h`: `UElementDamageType`, `UEmberElementDamageType`, `USoulLightElementDamageType`.
- `Weapons/PartEffect.h`: `EPartEffect`. `Weapons/PartEffectRules`, `PartEffectTuning` (`Looter.PartEffect.*`), `PartEffectComponent` (per gun: effect/element cache, Overcharge charge), `PartEffectSubsystem` (bullet hooks: element, Ricochet, Piercing, Explosive, Twin, Overcharge damage). `Weapons/WeaponFXElements.cpp`: element hits/auras/bursts/arcs, splash pop, ground ring, loot motes.
- `Loot/LootDropMoments`: `FDropMoment` per rarity, `Looter.Loot.Drop.*`. `Loot/LootFanfareShows.cpp`: spin, ring, column, name pop.
- Tests: `ElementTests.cpp` (`Looter.Elements.Weakness/.Chance/.Burn/.Smite/.Salt/.Words/.InPlay`), `PartEffectTests.cpp` (`Looter.PartEffects.Trigger/.Rules/.Parts/.Bounce`), `DropMomentTests.cpp` (`Looter.Loot.DropMoments.Rules/.NamePop/.Legendary`).
