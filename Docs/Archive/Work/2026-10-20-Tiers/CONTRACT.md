# Enemy tiers: the build contract (2026-10-20)

The design the user approved, with every number: `Saved/Tiers/DESIGN.md` (read it whole first; its "user's calls"
section is binding). Rules: `CLAUDE.md`. Today's rank system: `Source/AI_Looter_Shooter/Creatures/CreatureRank.h`,
`CreatureRankSettings.*`, `CreatureBaseRank.cpp`, `Session/SessionSubsystemPromotions.cpp`, `Loot/*`,
`Tools/Unreal/create_rank_assets.py`, `Docs/Story.md` "Enemy ranks and legendary drops", the tests `Tests/LootRank*`.

## Shared names (exact; every agent uses these)

- `ECreatureRank` gains two values APPENDED after `Boss` so saved rank numbers keep their meaning: `Hallowed` (5),
  `Ethereal` (6). Code names Basic/Rare/Epic stay; players see Regular (no word), Restless, Gravebound, Hallowed,
  Ethereal. `LooterRanks::Tiers[] = { Basic, Rare, Epic, Hallowed, Ethereal }` (the five tiers lowest first),
  `int32 LooterRanks::TierIndex(ECreatureRank)` (0..4; INDEX_NONE for Legendary and Boss), `bool LooterRanks::IsTier(
  ECreatureRank)`, `bool LooterRanks::IsAbove(ECreatureRank A, ECreatureRank B)` (order: Basic < Rare < Epic < Hallowed <
  Ethereal < Legendary < Boss). No code may compare ranks by their enum numbers any more: CORE audits every comparison.
- `FCreatureRankInfo` gains: `float Defense` (share of damage removed, 0..0.9), `int32 TraitCount`, `FLinearColor
  GlowColor` (the creature's tier glow; usually = Color), `float SilverMultiplier`, `float SpawnChanceFirst` (Ransom's
  Rest), `float SpawnChanceLate` (the late areas), `int32 MaxAlivePerArea` (0: no cap). `UCreatureRankSettings` gains
  `Hallowed` and `Ethereal` entries; their defaults are DESIGN.md's tables.
- Damage taken: `Damage x (1 - Defense x (bCritical ? 0.5 : 1))` (the user's "crits pierce half"), applied once, where a
  creature takes damage (bullets, melee, grenades, ember powers alike). `ACreatureBase::GetDefense()` returns it.
- Colours: `LooterUI::Color::TierRegular` (white), `TierRestless` (the Rare rarity blue), `TierGravebound` (the Epic
  rarity purple), `TierHallowed` (#F5C842, saint-gold yellow), `TierEthereal` (#FF3B3B). Tier-only colours (the user's
  call): a tag's colour no longer has to match a gun rarity.
- Loot tables: `DA_LootTable_Rare` (60%, 1 gun, Luck 0.5, ammo 2-3; as today), `DA_LootTable_Epic` (90%, 1, Luck 0.8,
  ammo 2-4), new `DA_LootTable_Hallowed` (100%, 1-2, Luck 1.1, ammo 3-5), `DA_LootTable_Ethereal` (100%, 2, Luck 1.3,
  ammo 4-6), by `create_rank_assets.py`; silver by `SilverMultiplier`.
- Traits: `ECreatureTrait { Fleet, Mending, Warded, GraveBurst, Rallying, Vengeful, Phasing }`
  (`Creatures/CreatureTrait.h`), `UCreatureTraitComponent` (`Creatures/CreatureTraitComponent.h`): `void SetTraits(
  TConstArrayView<ECreatureTrait>)`, `bool HasTrait(ECreatureTrait) const`, `TConstArrayView<ECreatureTrait> GetTraits()
  const`, delegate `OnTraitsChanged`; `static TArray<ECreatureTrait> RollTraits(int32 Count, bool bEthereal, const
  ACreatureBase& For, int32 Seed)` (Phasing always one of an Ethereal's; only traits that fit the creature's kind).
  CORE owns choosing WHEN (on a rank change, with `ACreatureBase::GetTierSeed()`, a stable seed from the creature's spawn
  id and its promotion roll, so a load rebuilds the same traits with no new save data) and adds the component; TRAITS
  owns the header and everything the traits do.
- Sound cues (SOUND makes them; constants in `Audio/LooterSoundCues.h`, a `Tier` block, added by CORE):
  `Creature.Tier.Notice.Hallowed`, `Creature.Tier.Notice.Ethereal`, `Creature.Tier.Death.Hallowed`,
  `Creature.Tier.Death.Ethereal`, `Loot.Tier.Pop.Hallowed`, `Loot.Tier.Pop.Ethereal`, `Creature.Trait.WardBreak`,
  `Creature.Trait.Swell`, `Creature.Trait.Burst`, `Creature.Trait.Phase`, `Creature.Trait.Enrage`,
  `Creature.Trait.Mend`, `Creature.Trait.Rally`.
- Dev: `Looter.SpawnCreature` and `Looter.Perf.Horde` take `Hallowed` / `Ethereal`; `Looter.Tier.Odds [kills]`
  simulates spawns and loot.

## Who does what

- **CORE (owns the Unreal Editor and the game module's builds):** the enum and helpers, the settings and defaults,
  defense in the damage path, health/damage/XP/level/size by tier, promotions (odds per tier rising by the area's place
  in the campaign from SpawnChanceFirst to SpawnChanceLate, the per-area caps, no reroll within 20 minutes, saves and
  their upgrade), traits' rolling hook and component creation, loot tables and silver, the name tag's prefix and colour,
  the cue constants, the powers' rank rules (Dust Devil's flings by rank: add Hallowed 60%, Ethereal 50%), the
  bestiary's defeats per tier if cheap, dev commands, tests (`Looter.Loot.RankOdds` for five tiers at 200,000 kills:
  legendary rate rising with tier and within 10% of DESIGN.md, Common the most likely rarity at every tier; spawn odds
  and caps; defense with and without crits; a save round trip keeping ranks and traits), Story.md's ranks table and the
  Pipeline. Builds and tests freely.
- **TRAITS (C++ in a holding tree `Saved/Tiers/Traits/Source/AI_Looter_Shooter/...`, no build until Main says):**
  `CreatureTrait.h`, `UCreatureTraitComponent` and each trait's behaviour and feedback (Fleet's speed, Mending's heal
  and shimmer, Warded's ward and its break, Grave-burst's swell and blast, Rallying's aura, Vengeful's enrage, Phasing's
  blink), which kinds each fits (a slime can't phase? a turret-like spitter isn't fleet?), using the cue names above.
  Merges and builds after CORE when Main says.
- **LOOK (C++ in a holding tree `Saved/Tiers/Look/Source/AI_Looter_Shooter/...`, no build until Main says):** the tier
  glow per creature kind (spider eyes, slime core, the Unpaid's coal, the dead miner's lamp, others), Ethereal's faint
  red ghost-rim (an overlay like the objective highlight's figure look), the name tag's look per tier (thicker frame on
  Hallowed/Ethereal, a small trait line under Gravebound and up), the notice sting for Hallowed/Ethereal, their bigger
  death burst and soul-light pillar, the loot pop with its sound, the XP popup in the tier colour. Merges after TRAITS.
- **SOUND (script-made sounds only; Art/Sounds):** the 13 cues above, in the family of the creatures' and bosses' sounds.
- **Main:** this contract, reviews, merges order (CORE, then TRAITS, then LOOK), imports, PIE checks, perf (a horde of
  Ethereals on Medium inside 8.3 ms), commits, docs.

Agents never commit or change git state. One class per file under ~500 lines. Each agent's report under 40 lines.
