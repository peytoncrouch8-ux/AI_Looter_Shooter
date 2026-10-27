# Pipeline: Enemy tiers, the Gravemother fight, fight AI and layout, placeholder creatures

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## Enemy tiers (built 2026-10-20)

The user-approved design, its numbers and the user's calls: Docs/Story.md, "Enemy ranks and legendary drops" (the one
place; the drafts in `Saved/Tiers/` are history). Built by four agents on one contract: the core, then the traits and the
look merged together in one build (staged with `git merge-file` against the core's working copy while it built: it saved
a whole build cycle).
- **The enum**: `ECreatureRank` keeps Basic 0, Rare 1, Epic 2, Legendary 3, Boss 4 and appends Hallowed 5, Ethereal 6 (levels,
  encounter groups, egg sacs and saves hold the numbers). Never compare ranks with `<`/`>` or index arrays by their numbers:
  `LooterRanks::Order`, `IsAbove`, `IsAtLeast`, `TierIndex`, `IsTier`, `TierBelow` (`Creatures/CreatureRank.h`). A rule per
  rank is a `switch` naming each (audit 2026-10-20: stagger, shove, melee knock, Dust Devil's fling, motes, grenades, the
  Bell-Ringer's toll, the Spitter's globs, the Unpaid's lunge, voice pitch, barks, elite music, the patrol's leader).
- **Numbers live in data**: `UCreatureRankSettings` (Project Settings > Game > Creature Ranks; per rank `FCreatureRankInfo`:
  word, colour, glow, size, health, damage, XP, levels, `Defense`, `TraitCount`, loot table, `SilverMultiplier`,
  `SpawnChanceFirst`/`Late`, `MaxAlivePerArea`, pack call). Tables by `Tools/Unreal/create_rank_assets.py` (re-run after a
  change to `DesignLootOdds`). Colours `LooterUI::Color::Tier*`.
- **Defense**: `UHealthComponent::SetDefense` (set in `ACreatureBase::ApplyStats`), applied once in `HandleAnyDamage` after
  the damage-taken scales: `x (1 - Defense x (crit ? 0.5 : 1))`; numbers shown and listeners get what's left.
- **Spawn odds**: the area's place in the campaign = `UCreatureRankSettings::CampaignRise(MinLevel)` (FirstAreaLevel 1 → 0,
  LateAreaLevel 50 → 1); `UAreaDefinition::PickPromotion` rolls the rarest tier first (`PickTier`). The area's own
  Rare/EpicPromotionChance only switch promotions on. Caps: `UAreaRulesSubsystem::KeepUnderCaps` counts the living (a
  creature not begun yet by its StartingRank) and steps a capped roll down; on arrival promotions and on spawner waves
  (rolled groups only, the wave's own rolls counted), never on Fixed ranks, commands or egg sacs.
- **Traits**: `UCreatureTraitComponent` ("Traits") on every creature; `ApplyRank` → `ApplyTraits` (before `OnRankChanged`)
  sets `RollTraits(TraitCount, Ethereal, *this, GetTierSeed())`; the seed is CRC32 of the actor's name (stable across loads
  and PIE; a runtime spawn uses its UniqueID) hashed with the promotion roll's bits (0 when placed or spawned with its
  rank). Nothing saved: a session keeps no creatures, only `PromotionsRolledAt` (no reroll within 20 min), so no save
  version change (still 3).
- **Silver**: Regular, Restless and Gravebound pay exactly what they did before the tiers (an Unpaid on average 2.75 / 10.2 /
  24, a beast 0.6 / 3.5 / 11.2); `SilverMultiplier` means "times a Gravebound kill" (1 below it): Hallowed 1.6, Ethereal 2.5
  (Unpaid 38.5 / 60). The first build took it as "times a Regular kill" through `LooterEconomy::ScaledKillOdds` and cut
  today's silver to a third: a test now pins today's amounts and that silver rises every tier.
- **Traits** (`Creatures/CreatureTrait*`, every number in `UCreatureTraitSettings`, Project Settings > Game > Creature
  Traits): Fleet speed x1.25, wind-ups x0.85 with a glint, wind streaks; Mending after 3 s unhit 3%/s with motes and the
  Mend loop, any hit stops it; Warded a ward of 30% health taking hits first, crits count x2 on it, the breaking hit spills
  over, a turning cyan cage, WardHit pings, WardBreak with shards and a 0.8 s daze, no regrowth, a pale-cyan bar over the
  tag's health; Grave-burst a 1 s swell (body grows, a 3 m ring pulsing faster) then 1.5x its attack (x0.6 at the edge) to
  players in sight, knockback and a jolt; Rallying creatures within 8 m take x0.85 and move x1.1 (once however many),
  ring, tethers, a pulse every 6 s; Vengeful below 30% enrages for good (x1.25 damage and speed, flare, embers); Phasing
  on 8% of its health taken within 1 s blinks 4-6 m aside of the shooter (every 4 s at most, to ground it can stand on and
  they can see; a hard hit mid-attack blinks once the attack ends, up to 1.5 s: melee creatures are nearly always mid-
  attack). Fit by kind: the Spitter and Bell-Ringer never Fleet or Grave-burst, the Bell-Ringer never Vengeful; every kind
  can phase. `SetTraits({})` undoes everything. `Looter.Traits.Give <trait...|none>`, `Looter.Traits.Show`.
- **The look** (`Creatures/CreatureTierLook*`, `CreatureTierFXSubsystem`): a "tier shell" (a leader-posed copy of the body
  in `M_CreatureTier` from `build_tier_material.py`, one translucent draw a creature; the overlay slot belongs to the hit
  flash and the bodies' own slots share M_World / FoliagePalette) glowing on the eye patch, the coal, the lamp, round the
  slime's heart (the imported SK_Slime keeps no vertex colour, and the gel hides the core: the shell lights the gel within
  50 cm of it), Ethereal's red ghost-rim; the tag's frame and clamps on Hallowed/Ethereal, the word popping on notice, a dim
  trait line from Gravebound up; notice stings over the rank sting; death shows (pillar 9 m/1.7 s Hallowed, 13 m/2.1 s
  Ethereal), loot pops 0.15-0.18 s after, the XP popup tinted.
- **Bestiary by tier** (2026-10-20): `FPlayerProgressData::DefeatedByTier` (kind's class path -> `FTierDefeats`, rank -> count,
  rank saved by name) beside `Defeated` (the total, unchanged: every defeat of the kind, whatever its rank). Written by
  `UPlayerProgressionSubsystem::RecordDefeat` (every path that counts a defeat: `AwardKill`, the mound's spiderling) from
  `ACreatureBase::GetRank()` at the death (a promotion lasts until it; the respawn resets it later), tiers only
  (`LooterRanks::IsTier`): Legendary monsters and bosses count in the total alone and keep their own pages; a non-creature
  (dummy) too. Read by `GetDefeatedByTier(class)` (same classes as `GetDefeated`, `CountsForPage`). **Old saves read as tiers
  unknown** (user-visible effect: none shown): a session from before has totals and no split, no version bump (the field is
  new within session version 3; `ULooterProgressSave::ToProgress` likewise), the split counts only defeats since; no tier is
  guessed because Restless and Gravebound existed before today and weren't told apart, so "all Regular" would be wrong. The
  line under "Defeated" (`BestiaryTiers::Pieces/Words`, drawn by `BestiaryWidgetDetails.cpp`): the tiers met above Regular,
  lowest first, small caps in the tag's colour (`CreatureTierLook::TagColor`), dim " · " between, right-aligned under the
  count, a small diamond before Hallowed and Ethereal (`IsTopTier`); Regular is left out (the count above is mostly Regular),
  no line when none met. Checked in play (`Looter.SpawnCreature Spider <tier>`, `Looter.KillNearest`, `Looter.Bestiary.Defeats`
  logs the split; inventory > 3). Tests `Looter.Bestiary.TierCounts` (counting, rank at death, Legendary and dummy
  total-only), `.TierSave` (round trip, an older save and the pre-session save, counting on from them), `.TierLine`
  (words, order, colours, marks). A bare `ULocalPlayer` (`NewObject<ULocalPlayer>(GEngine)`) with `NewObject<UPlayerProgressionSubsystem>`
  inside it is enough to test the progression with no game (no game instance: nothing saves).
- **Seen in play** (2026-10-20, town, `Looter.SpawnCreature DeadMiner Hallowed`, `Looter.Traits.Give Fleet` on a spider): the
  dead miner's flame (glow strength 18) burns out to white on screen whatever its tint, so a Hallowed lamp read white from 4
  to 28 m; Restless and up now add a halo of the tier's own colour round it (`LampHaloSize` 3.2x the flame,
  `LampHaloShare` 0.16 of its strength; DeadMinerCreatureLamp.cpp; the light it throws was already the tier's). Fleet's
  wind streaks started a third of the body back and a running spider's abdomen hid them (a hairline under it): now from
  0.6 of the body's radius, 0.2 s of its speed long, 4.5 cm wide, strength 5 (CreatureTraitComponentPace.cpp).
- **Lessons**: Phasing never blinks in a safe zone (Delia's salt line covers the farm, orchard and family plot: test in
  town); its wall and sight checks trace 150 cm up (fences and gravestones blocked every spot at foot height); a perf horde
  at the game's start sits in that safe zone and barely fights: give `Looter.Perf.Horde` a spot in town (`0 -1400 90`).
  The gun Legendary colour (1.0, 0.6, 0.0) is linear: on screen #FFCB00; the named Legendaries' tags are #FF9900 sRGB now,
  so they read deeper orange than a legendary gun's beam (Abel's coal reads the Boss colour and turned orange too).
- **Dev**: `Looter.SpawnCreature <kind> <tier> [count] [chase]` logs the last one's health, defense, bite, XP and traits;
  `Looter.Tier.Hit [damage] [crit]` hits the nearest as the player's shot and logs what defense let through;
  `Looter.Tier.Odds [kills]`; `Looter.Perf.Horde Spider 8 Ethereal`. A test level's creatures take damage only through
  `OnTakeAnyDamage.Broadcast` under `FEditorScriptExecutionGuard` (no game mode: `ApplyDamage` does nothing there).

## The Gravemother's Barrow fight (built 2026-10-20)

- The design: `Docs/Areas/RansomsRest.md` (Enemies by rank, The Gravemother); contract and plan `Saved/Gravemother/CONTRACT.md`,
  `PLAN.md`; sheets `Saved/Concepts/Gravemother/Barrow_*.png`. Watch it: `Looter.SpawnCreature Gravemother`, `God`,
  `Looter.Gravemother <Fight|Clod|Furrow|Quake|Unearth|Stun|Break|Stone N|Health N|Pace N|Hold [0|1]>` (Hold: she stands
  still but still makes her moves, for a look), `slomo 0.3`. Placing the player near her in the Sink (a bowl) needs a
  ground trace kept level with her feet (`Saved/Gravemother/gm_place.py`). PIE hitches seen during her fight (0.3-0.7 s)
  were the PCG subsystem (90k raycasts in one frame) and Mass (`stat dumphitches`), not hers.
- Her own look (pale hide, earth, dark headstone; 2026-10-22) and the painted ghost (every M_Ghost user): `Docs/Pipeline/
  CharacterModels.md`, "Materials pass".
- Bug fixed (2026-10-20): one long frame (0.4 s, the frame clamp; console typing, a hitch) on a plough's first frame read
  as "blocked" (she stood still through the heave) and she slammed down where she stood. The first frame now only sets
  her going; plough time counts from the next. Verbose `LogLooter` lines trace it: "ploughs", "plough ends <there|out
  of time|blocked>", "furrow is over".
- Every art piece loads by path (`Creatures/GravemotherAssets`) and keeps a stand-in for when it's missing: engine
  cube/sphere in `M_StylizedSurface` colours, `FWeaponFX` dust/sparks/clods, dark `AGroundCrack` cracks for missing decals.
  ART's assets are in (2026-10-20): `Looter.ImportModels <Intermediate/ArtExport_Gravemother>` (24 models), then
  `build_grave_fight_materials.py` (masters M_GraveDecal, M_FX_GraveCard, M_FX_GraveGlow; 11 MIs in
  /Game/Art/Effects/GraveFight; MI_GravemotherBare). Her body is found in the constructor: restart the editor after an
  import. Still code-drawn (ART made none): stun stars, the clod's trail, mud on spiderlings.
- Contracts with the art: card materials read 5 per-instance floats (0-2 the whole linear colour, the MI tint is white;
  3 opacity/strength; 4 sheet cell); the engine Plane's U runs along its +X, V along +Y: the spark's head (U = 1) leads,
  the jet's foot (V = 1) is down (`GravemotherFXDraw.cpp`). Decals: scalar `Opacity`, `Reveal` (the crack), vector `Tiles`
  (U, V repeats; U, V offsets; each furrow shifts its crack and strip in U), U along the strip (the decal's Z:
  `GravemotherAssets::AddGroundDecal`). The soft rear: the abdomen body's only round shape (USP_AbdomenRear; one body per
  bone keeps no names; read with `FCreatureBodyHulls`), else the hull's far end (SK_Spider). The stone on SK_Gravemother:
  stone_attach.json's component-space place (-78.179, 0, 89.017), P 26 / Y -4 / R 5, made relative to the abdomen's rest
  pose at runtime (`StonePlacement`); shards are modelled round the stone's pivot (spawned at its transform). At the
  crack-off her `GravemotherAbdomen` slot wears MI_GravemotherBare (cleared again when the stone is reset). SM_GraveBerm
  (2.6 m) every 2 m (`BermEvery` 200), its steeper -Y face turned to the line on both sides.
- Bug fixed: build_grave_fight_materials.py fed `Tiles` to its Custom node from the vector's RGB pin ("swizzle 'zw' out
  of bounds": M_GraveDecal failed to compile, decals drew the default material); it takes the RGBA pin now. The crack's
  end cap looks for its ground only 60 cm over the last tile's (it once lay on a rock 1.8 m up and reached nothing).
- The decals checked by eye (2026-10-20, POLISH-CODE; the Sink and the west field at (-57, 5) m; `Saved/PolishCode/
  gm_*.py` and `stand.py`, not committed, probe and stage them): (1) Reveal was never driven: the crack popped in a tile
  (2.8 m) at a time. Each tile's Reveal now follows the front (`SetHeave`, soft over `CrackRevealSoft` 0.12 of a tile,
  kept just behind the front). A ground decal's U runs along its Yaw (the engine's decal UV is the box's local Z, Y; Z is
  the line's way here), so 0 -> 1 crawls from her toward the player: seen so. (2) Both strips were cut square at her end
  (the ploughed strip's start stood out as a hard-edged rectangle once she'd left it): M_GraveDecal has `FadeIn`/
  `FadeOut` (shares of U; `SetDecalEnds`): the crack's first tile fades in over 0.3, the strip's first over 0.35 and its
  last out over 0.25. (3) A new furrow's ground trace stopped on an older furrow's berm (a tile 70 cm up whose decal never
  reached the ground: a gap): `OnGround` ignores every furrow and the rubble, and berms, the crest and the rubble pile
  take no decals. (4) The end cap joins: the strip's crack meets U = 0 or 1 within 0.02 V of the cap's entry (1.5 cm of a
  7 cm crack) and the cap overlaps the last tile 14 cm; caught the moment it shows (`Saved/PolishCode/gm_catch.py`: waits
  for a decal, freezes time, stands the player beside it), the crack runs on into the cap's taper unbroken
  (`gm_cap_caught_zoom.png`). The scar, the mound cracks and the ploughed strip between the berms looked right. (5) The
  clod's landing shadow hardly showed through the west field's grass (the picture's alpha is at most 0.5, the decal
  darkens only the terrain between the blades; `gm_shadow_fp.png`): M_GraveDecal's `AlphaScale` (1.8 on
  MI_Decal_LandingShadow, `DECAL_SCALARS` in the script) and the clod's opacity 0.35 -> 1 (was 0.3 -> 0.85) make it a
  plain dark disc with a rim from the throw on (`gm_shadow_after.png`). To make her throw: the player 19-20 m off (her
  furrow's reach is 16 m at her size, the clod's 7-22 m), `Looter.Gravemother Clod`.
- Shared hooks it added: `ASpiderCreature::GetPoseOverride` (a subclass's body and front-leg pose; none by default),
  `UBossComponent::BeginStagger(Seconds, Callout)`, `UBulletSubsystem::OwnLookTag` ("Impact.Own": the stone, clod and mounds
  draw and sound their own hits), `UHudBossBarWidget::SetArmour` (the stone bar), `LooterUI::Color::Granite*`.
- Separate actors so shots on them never touch her health: `AGravemotherStone` (attached to the abdomen bone, Weapon channel
  only, Breakable tag: no kill marker), `AGravemotherClod` (1 HP while held), `AGravemotherMound` (1 HP while it bulges; a pop
  credits the spiderling's kill XP by hand: no creature exists). Berms and rubble are world-static, tagged Obstacle (spawn
  probes keep off), block pawns (rubble only once no character stands in it) but never the camera.
- Test levels never tick: tests advance mounds, the furrow, rubble, the clod and the stone's shards themselves
  (`Tests/GravemotherTestWorld.h`); the stone's shards clamp a step to 0.1 s (advance in steps).

### Playtest round 2 (BOSSES, 2026-10-27, notes 21 and 34): harder Gravemother and Abel, the stone gate, rim shrug

The user: both bosses "way too easy", she could be shot from the Sink's rim without aggro, "break the tombstone on her back
before you can damage her", rim shots do nothing until the player comes down. Code: `Creatures/GravemotherTuning` (cvars
`Looter.Gravemother.*`), `GravemotherCreatureShrug.cpp`, `GravemotherStoneMark.cpp`, tests `Tests/GravemotherShrugTests.cpp`
(`Looter.Creatures.Gravemother.Shrug.Stone`, `.Rim`, `.Tuning`).
- Health x1.6 (`HealthScale`, class default 300 x 1.6) and hits x1.25 (`DamageScale`: bite, Furrow, Gravequake, Clod; class
  default 12 x 1.25, the Ledger reads the class); cvars move the next spawn (`ApplyTuning`, before her rank). Level 6: about
  8,060 health (was 5,040); her stone (20% of hers) about 1,610. Stagger crit share 0.07 -> 0.07/1.6 (same crit damage).
- Stone gate: while the headstone stands nothing hurts her (`ShrugFor` -> `Stone`; `.StoneGuards` 1). The 33% auto-break stays
  as a fallback (reachable only with the cvar off). An early break still staggers her: now always the fight's first big window.
  Her total work is about 1.9x the old (stone 0.32 + body 1.6): if it plays long, lower `.HealthScale` or the stone's
  `StoneSettings.HealthShare` (0.2).
- Rim shrug: a shooter off her hunting ground (the lair's 36 m / 3 m rise + `.RimSlack` 300 cm so berm jumps and the ramp's
  foot count; the rim is 12 m up) does her AND her stone no harm (`OffGround`; `.RimShrug` 1). She still turns to the shooter
  (`AlertTo`: off her ground she goes to the edge and watches).
- Shrug feedback: no damage, number, flinch or brood call. Her own look (pale glint, 1-3 glancing sparks, grave-mud puff, a
  crumb) and sound (Impact.Dirt thud + Impact.Ricochet whine, throttled). A shrugged shot draws no wound: she listens to
  `UBulletSubsystem::OnBulletLanding` and sets/clears the own-look tag on the part struck per shot. Her bar calls out "Break the
  headstone" / "Out of reach" (3 s apart at most, never over a stagger). After `.ShrugHintAfter` (3) shrugs of a kind, once per
  Gravemother, Hob says "That headstone's rooted to her back..." / "Waste of lead from up here...".
- Rim shots, second pass (play-check 2026-10-27): the look grows with the shooter's distance (`ShrugLookScale`: 1x within 15 m up to
  3x, more sparks from far off); from the rim the ricochet whine plays 2D (volume 0.55) so the shooter hears it, and she rears and
  screeches up at them (`AnswerRimShot`: `SpiderAlert` cue, 4 s apart at most).
- Clod headroom: before lifting a clod in her den she traces for a roof over her (the clod's full size above her middle); no room:
  no clod (she bites or ploughs). A lob blocked within 0.25 s of leaving her grip (`ClodMinClearSeconds`) falls from her grip
  harmlessly (it burst on the den's roof).
- The stone is the target: M_ObjectiveHighlight overlay in `LooterUI::Color::Accent`, 2.5 Hz pulse, `.StoneMarkSeconds` 1.6 s
  after each stone shrug and `.StoneMarkAtFight` 3 s as the fight starts; Nanite forced off on the slab only while it glows.
  Crack stages, shatter, shards, stagger unchanged.
- Abel (`Bosses/AbelRules`, `AbelKeeper`): health x1.6 (40 Unpaid x1.6: about 15,150 at his level 7, 16,800 at 9; `FightHealthScale`,
  `Looter.Abel.HealthScale`), hits x1.25 (swing and buckshot pellets via AttackDamage; `.DamageScale`). Stagger crit share
  0.05 -> 0.05/1.6. Every health-gated stretch is 1.6x longer at the same DPS: phase 1's adds (2 per 25 s, at most 4 alive)
  kept; wind phase's walk-off/pull cycles and barrages kept; the bell phase's back half on the deck (35%) was a bare duel and is
  now a trickle like phase 1 (2 every 25 s, at most 4 alive) from `BellRiseFrom` 49.5 s (after the second wave; none rise while
  the bell's waves stand).
- Not checked (no build when written): the overlay's strength on the slab, the shrug sounds' mix, the ricochet rate on fast
  guns. The HUD hit marker still flashes on a shrugged hit (PlayerHUDWidget): a "no damage" marker would help.

### Boss intros (SCENES, playtest round 4, 2026-10-27; the user's pick: cutscenes as the boss fights start, short; rules and commands: `Game.md` round 4)

Short in-engine entrances for the Gravemother, Cyrus Teague (the Foreman) and Abel, all C++ (`Scenes/BossIntro*`), played as each boss's
first fight of a session starts. Camera, words, name card, music; the boss then does its own entrance on camera and the fight goes on.
- Hook: each boss's `HandleFightStarted` ends with `BossIntro::PlayIfDue(*this)` (`GravemotherCreatureMoves.cpp`, `ForemanCreatureEvents.cpp`,
  `AbelKeeperFight.cpp`). It plays only in a game world, scenes on, when `Looter.BossIntro` is 1, not a rematch (`UBossComponent::
  IsRematch`) and `BossIntro.Seen.<Kind>` isn't set (set when it plays; a fight restarted after the player's death, the Gravemother's
  lair return, a stand-down: no intro again).
- Because the fight has already started, the scene (`USceneSubsystem`, so skippable) holds the player (no control), makes them
  untouchable (`FBossIntroGuard`) and freezes the boss at once (`FBossIntroFreeze`: `CustomTimeDilation` 0.001, never 0, a tick of zero
  is divided by in places; it carries the brain, moves, pose and the `UBossComponent` fight clock) and, at the scene's start, every
  living `ACreatureBase` within 60 m of the boss or of the player (a first phase's adds, a second boss, a pack nearby; the play-check
  added the player's radius). The boss bar is hidden and swept in again at the hand back. At `LiveAt` the boss alone runs again and
  does its entrance on camera; the view eases back to the player's eyes over 0.8 s facing the boss, the letterbox goes, the rest wake.
  Skipped: the same at once (the boss's entrance still plays, live).
- Camera: shots in a frame from the boss's feet toward the player's eyes (`FBossIntroFrame`), eased (smootherstep); each frame the eye
  is pulled in front of any world-static or dynamic thing between it and its mark (`BoomClip`: a 20 cm ball sweep, margin 40 cm, never
  nearer than 1.5 m) and kept 60 cm over the floor traced under the shot's mark (`MinEyeOverFloor`), so the den's roof, the stope's walls,
  the fog wall and Abel's deck never block it. Cuts dip to black 0.25 s each side. The scene camera's FOV and exposure are saved and put
  back (it is shared with every later scene). Name card and letterbox: `UIntroWidget`; the card's epithet is the boss bar's own title
  (`FBossShow::Title`), so they agree. Words: `DA_Lines_BossIntro<Kind>` (`create_story_lines.py`) when it has every line, else the
  code's `ScriptLines`; no family words, the same for every hero. Music: each boss's 2D bed from 0 (`Music.BossIntro.<Kind>`), faded at
  `LiveAt` under `Music.BossIntro.Reveal`; the bar's own `Boss.Intro` sting still plays at t=0.
- **The Gravemother (10.2 s)**: "Den" 0-4.4 from over the player's shoulder on her among her graves; 0.6 Hob: "Every spider in the Rest
  answers to her."; 2.6 she dives (dirt, `Boss.Gravemother.Burrow`, a jolt, hidden); 3.0-6.2 five bursts of dirt in a line toward the
  burst spot (faint jolts); cut at 4.4 to "Floor", low and wide 10-12 m off the burst spot; 4.8 Hob: "Crack that headstone off her back
  first, sunshine."; 6.6 LIVE: she is put at the burst spot facing the player (`BurstAlong`: 45% of the way to them, at least 10 m from
  them, within 16 m of her home, on ground within 3 m of her level with room over it; else where she dived), climbs out (`DevPlayEntrance`,
  0.8 s) with a big dirt burst, `Boss.Gravemother.Emerge` and a hard jolt, then roars; 6.9-9.9 card THE GRAVEMOTHER; her headstone's target glow is lit
  again at the end (its fight-start glow ran out under the scene).
- **Cyrus Teague (9.8 s)**: "Drift" 0-5.0 from the player's eyes pushing along the stope on his lamp at the fall, exposure -1.25 -> -0.5 EV
  (-4 was black); 1.0 Teague: "Dig. Keep digging. It was the roof." (sets up his last word); cut at 5.0 to "Lamp", close from his
  side, -0.5 -> 0; 5.6 LIVE: his entrance (turns from the fall, pick handle up 1.1 s, slam, his own "Nobody leaves till the drift's
  cleared!"); 6.2-9.2 card CYRUS TEAGUE. He doesn't literally walk out of the dark (no scripted walk exists): the exposure brings the stope
  out on his lamp.
- **Abel (10.4 s)**: "Boards" 0-5.2 from over the player's shoulder at the Keeper's Gate down the deck to him among the lantern posts, the
  sun behind him; his fight's own first line ("You brought them here, El. Right to our door.", `AbelRules::PhaseLine(0)`) is said once as
  the fight starts (the first pass cleared and repeated it: said twice) and Hob's follows at 3.8 s (`FightLineSeconds` 3.4): "A keeper
  doesn't lie still while his saint is dark."; cut at 5.2 to "Lantern", eye 130-150 cm up (a low eye went under the deck), looking up at
  him; 5.6 LIVE: he raises his ghost lantern (`EAbelMove::Intro`); 5.8-9.0 card ABEL RANSOM.
- Play-check fixes (DEMO, 2026-10-27): no repeated Abel line; the player untouchable through the scene and 1.5 s after (Abel's buckshot
  landed 0.5 s after the hand back); hostiles within 60 m of the player held too; Abel's camera kept above the deck; the Foreman's shot
  readable; card epithets from the bar.
- Not checked in play: freezing assumes every creature system ticks on the actor's dilated time (anything on world time keeps running:
  the Gravemother's headstone glow, Abel's intro charge glow `ShowCharge`, timers, a volley's pellets in flight); the Gravemother is
  teleported to the burst spot, checked for ground and headroom but not for props or rubble; frames come from where boss and player
  stand, so a fight started from an odd spot (the Sink's rim, 12 m up) frames from up there; the beds start 2D under the boss music the
  fight's start may already play (the mix of the two isn't heard); captions use the HUD caption widget 118 px over the bottom and the
  letterbox bar is 10% of the height (108 px at 1080p): check at 1080p and ultra-wide. `ACreatureBase::IsSceneHoldingPlayer` keeps
  every non-boss creature from noticing the player during the scene, so nothing new wanders in.

## Fight: AI (FIGHT-AI, written 2026-10-21, unbuilt and untested in play)

The user's calls (Saved/Fight/CONTRACT.md): packs surround and take turns, ranged kinds use their range, Borderlands-normal
difficulty. Code: `Creatures/FightDirectorSubsystem.h/.cpp` + `FightDirectorPlaces.cpp`, `FightRules`, `FightTuning`,
`CreatureBaseFight.cpp` (the chase's ask; the old Chase block of `TickBrain` moved there). Tests `Tests/FightTests.cpp`,
`Looter.Fight.*`. Cvars `Looter.Fight.*`, live in PIE, defaults in brackets; `Director` [1], 0 = the old free-for-all.
- Who: every creature but a boss (`KeepsBossRules`) or a Legendary monster (`FightRules::KeepsOwnFight`) and a passive one.
  Bosses' adds (spiderlings, Abel's risen) do take turns.
- Turns (`FFightTokens`, per target): `MeleeTokens` [3], `RangedTokens` [2]. A melee creature asks once its cooldown is done
  and it's within `RingApproach` [1.6] x its ring radius; a ranged or support one when it could attack this moment. The
  longest waiter gets the next turn; one silent `TurnKeep` [0.5 s] leaves the line. Given back at the attack's end with
  `TokenRest` [0.6 s] (`EndTurn` from `TickAttack`), on leaving Chase/Attack, on death (`ForgetCreature`, also `EndPlay`),
  out of reach (1.5 approach distances, not mid-attack), and taken back after `TokenMaxSeconds` [3] with the rest. Ranged
  turns begin `RangedGap` [0.35 s] apart.
- Hits: staggered or knocked (`IsHeldByHit`), its turn goes back but it stays first in line for `TurnKeep`: a short stagger
  costs nothing, a long knock lets the next one in. Hit in its wind-up, its attack goes on after only if it gets its turn back,
  else it's called off (`OnAttackCalledOff`: the Spitter's glow out, the slime's squash let go) with a 0.4 s cooldown.
- The ring (melee, `RingMinPack` [3]+ hunting one target): `RingSlots` [8] at `RingRadius` [480] or `RingMargin` [80] past the
  creature's reach (slime 530); refreshed every `RingRefresh` [0.4 s], lazily at the first ask after (one walk of the
  encounters' creature list, 2 traces a slot: a line from the target, ground within 1.6 m of its feet). Slots cost their
  angle from the creature's bearing (180 degrees = 1) + `BehindBias` [0.35] x frontness in the player's view - `RingKeep`
  [0.15] for its own; greedy, extras double up. Turns `RingDrift` [25 deg/s]; `FeintChance` [0.15] a look to step in to
  `FeintDepth` [0.55] of its radius over `FeintSeconds` [0.5]. Waiting ones sidle round (`OrbitWay`, never across the
  player) facing the player (`bOrientRotationToMovement` off while waiting), speed 2.5 x the gap. The flank fan still brings
  them in to the ring. An Unpaid waiting on the ring never phase-steps (`IsWaitingTurn`, UnpaidCreaturePhase.cpp).
- Ranged (`GetFightRole` Ranged: the Spitter; Support: the Bell-Ringer, no ring slot, its own keep-back at 11 m): a spot at
  `RangedOffAngle` [50] or more round from the melee pack's mean bearing (another spitter within 40 degrees costs 1), its
  distance kept in the band (`GetFightBand`: back-off to CloseInTo, 15% in from the near edge, 10% from the far), ground
  within 3.2 m of the target's feet and a clear line from 1.2 m over it; 4 tries. After a shot `RangedRelocate` [0.6] picks
  one 30+ degrees round; its lob blocked 1.5 s at its spot asks for another. It walks there round the target at
  `RangedMoveShare` [0.7] of its chase; backs off inside `RangedBackOff` [900] (BackOffWithin 900 too, was 1000).
- Difficulty: `ChaseScale` [1.3] on chase and track speed (`GetChaseSpeedNow`; the `ChaseSpeed` field is unchanged, so the
  traits' and rally's x/÷ stay exact; the slime hops at its own rhythm, unchanged); spider 413 -> 537 (walk 459, sprint 711).
  `DamageScale` [1.5] on `HitWithAttack` and the Spitter's glob, splash and puddle (`GetFightDamageScale`). Regen
  `RegenDelay` [7 s] and `RegenRate` [0.07] set into `UPlayerVitalsSubsystem::Recovery` each frame (`FRecoverySettings`
  defaults 7 and 0.07 too). Not scaled: the Grave-burst trait's blast (CreatureTraitComponentBurst.cpp) and the bestiary's
  shown damage.
- Perf (2026-10-21): 10 spiders fighting in town 7.5-7.7 ms (Docs/Performance.md). CSV stats `Exclusive/GameThread/CreatureTick`,
  `CreatureBrain`, `CreatureFight`, `CreatureSteer`, `CreatureFace`, `CreatureSpacing`, `CreatureTag`, `FightRefresh` show a
  creature's own share; the director is under 0.05 ms, facing while circling 0.3 ms. Measure with `Saved\ArtPause.flag` set
  and nothing else running (a busy PC read 9.1-9.7 ms), and in town: the spawn horde sits in a safe zone.
- Didn't check (needs play): every feel number; ring slots in the mine and town alleys; the sidle with the spider's gait.

### Fight: the Unpaid's kit (ENEMIES, 2026-10-27, notes 23 and 39)

The user: "Unpaid have terrible ai, need better attacks and targeting sequences"; "mobs are not frightening enough". Code:
`UnpaidCreatureMoves.cpp` (kit: lead, rake, flank, tells, pace), `UnpaidCreatureRing.cpp` (cinder and rally scream on the ring),
`UnpaidFightRules` (`UnpaidFightRules`, `FUnpaidKitState`, `EUnpaidMove`, `EUnpaidAction`), `UnpaidFightTuning`
(`FUnpaidFightTuning`; 42 cvars `Looter.Unpaid.*`, defaults = field values; `UnpaidKitCue` names), tests `Tests/UnpaidFightTests.cpp`
(`Looter.Creatures.Unpaid.Kit.Tuning`, `.Rules`, `.Moves`, `.Director`). Only plain Unpaid and BP children have the kit
(`AUnpaidCreature::HasFightKit`); `Looter.Unpaid.Kit 0` = the old behaviour.
- Lunge (play-check fix 2026-10-27, DEMO: at the first numbers 11 of 13 lunges hit a walking strafe) leads the target a little
  (`LeadShare` 0.25 of their run, at most `LeadMaxSeconds` 0.2 ahead), starts from 5.2 m (was 3.8), flies up to 4.7 m (was 3.2),
  no homing (`LungeHoming` 0: the aim is fixed at the wind-up's end), lands on a target within `LungeHitRadius` 85 cm of its line
  (was 130), hurts `LungeDamageShare` 0.85. A 400 cm/s strafe dodges a Regular's lunge from its farthest start (test).
- Rake (second attack) within 2.6 m, 70%: arm drawn back 0.5 s (the tell), two swipes 0.32 s apart (left/right), 2.4 m reach, 130 deg
  arc, 0.5x damage each (`RakeDamageShare`), drifting in at 240 cm/s. New pose channel `FPoseChannels::Rake[2]` (Rig follows it, Arms applies it).
- Coal flares through every wind-up (`TellFlare` 0.6): the read for all attacks.
- Flank: readied while waiting on the ring (rolled once a wait, so its next turn opens with it; the first version rolled at the
  turn and never fired): in the player's view (`FlankFront` 0.6, within about 78 deg; `FlankMinRange` 300 cm, was 450; to 11 m),
  `FlankChance` 0.6: phase-fades and returns 3.3 m
  away at their side/behind (105, 145, 180, 75 deg off their view, its own side first; same ground/room/sight/safe-zone checks as
  the phase-step, `CanStandAt`); a whisper (`Creature.Unpaid.Flank`) plays at the arrival spot first. Cooldown 7 s.
- Ring pressure: waiting on the ring 1.2 s+, 4.5-22 m, clear line: a cinder: 0.7 s tell (glow swelling at its chest via
  `UEnemyProjectileSubsystem::ShowCharge` + `Creature.Unpaid.Cinder`), then a led pellet (1500 cm/s, 16 cm, `CinderDamageShare` 0.4, coal
  colour). One cinder per player per `CinderGap` 2 s (director), one per Unpaid per ~6 s. A stagger or knock cancels the tell.
- Rally: 3+ Unpaid on one player, one waiting 2.5 s+ screams (`Creature.Unpaid.Rally`, 0.8 s, stands still, a 14 m ring of its
  coal's light via `AShriekRing`, no slow); then a 4 s frenzy: that pack may hold 4 melee turns at the player (usual 3) and chases
  x1.2 (`FrenzyChase`), coals glowing. One rally per player per 16 s.
- Howl: the first Unpaid to notice a player howls (`Creature.Unpaid.Howl`, 'Gun' attenuation, about 60 m); once per player per 12 s.
- Closing: chasing from over 9 m (not waiting on the ring): x1.2 chase.
- Unchanged: Abel, dead miners, the Bell-Ringer (native subclasses), boss adds (found once a life via
  `UBossComponent::GetAliveAdds`: Abel's risen keep the old shriek+lunge so his pacing holds). The Gravebound+ slowing shriek stays.
- Director (`FightDirectorSubsystem`): `OpenFrenzy`/`GetFrenzyLeft`, `TryStartRally`, `TryHowl`, `TryTakeCinder`, `CountPack` (pack
  tags counted at each look), `SetChaseBoost`/`GetChaseBoost` (forgotten in `ForgetCreature`). `ACreatureBase::GetChaseSpeedNow` and
  `TickChaseFight` multiply by the creature's boost (1 for all but the Unpaid). Flank and cinder need the director
  (`Looter.Fight.Director 0` turns them off).
- Sounds: recipes group `unpaid_kit` (`Art/Sounds/recipes/unpaid_kit.py`): `Creature.Unpaid.Rake`, `.Cinder`, `.Flank`, `.Rally`,
  `.Howl` (render with `Tools\sounds.ps1`, then `build_sound_bank.py`; the cue names live in `UnpaidKitCue`, not yet in
  `Audio/LooterSoundCues.h`: move them there when next in that file).
- Cost: per Unpaid a frame a few cvar reads, the director's maps, one LOS trace per ring look that passes the other checks (every
  0.3-0.5 s), a flank's spot search once a turn (7 spots x 3 queries at most); cinders are pellets (no actors); the rally ring is
  one `AShriekRing` actor per 16 s per player. Perf check to run: `Tools\perf.ps1 -Label "Unpaid kit" -Exec "Looter.Quality Medium"`
  with `Looter.Perf.Horde Unpaid 8 Basic 0 -1400 90` against 10 spiders' 7.5-7.7 ms.
- Not checked in play: every number (frenzy 4 turns x DamageScale 1.8, cinder 0.4x, rake 2 x 0.5x are the dangerous ones, all
  cvars; the lunge, flank and cinder numbers were retuned after DEMO's play-check, 2026-10-27). `Looter.Creatures.Anim.UnpaidArms` places the player 2.5 m off, so the rake may start there; other tests that tick an
  Unpaid 3.8-5.2 m from a player may see an attack sooner (lunge starts 5.2 m). `UnpaidCreature.h` 558, `.cpp` 514 and
  `UnpaidCreatureArms.cpp` 515 lines: over the ~500 rule, split next time.

## Fight: Layout (FIGHT-LAYOUT, written 2026-10-21, unbuilt and untested in play)

The user's calls (Saved/Fight/CONTRACT.md): 6-10 enemies a normal fight, camps and patrols 5-8, the chapel yard and the
mine at the top; a few hand-made set pieces an area, announced and rewarded, once a session; not plain waves, not new mixed
packs. Counts and the set pieces: `Docs/Areas/RansomsRest.md` "Fights and set pieces"; the pacing: `Docs/Progression.md`.
- Caps (`UEncounterSettings`): 20 creatures within 80 m of the player (was 16), the Unpaid family 16 (was 12; Bell-Ringers
  and dead miners count). A set piece's placed creatures skip the caps (one or two).
- Where the counts live: the story's fights in their scripts (`build_area_story.py` NEST_BASIC, GATE_BASIC and GATE_SPOTS,
  `build_area_chapel.py` YARD_WAVE/YARD_SECOND/ROAMING_COUNT, `build_area_whitlock.py` HANDS_BASIC, `build_area_sink.py`
  FLOOR_COUNT, `build_area_missionprops_valley.py` DEPOT_BASIC/CELLAR_BASIC); the rest in `layout.json`
  gameplay.encounters (camps, patrols, ambushes, fights). More creatures than spots: the spawner rings its spots (a
  spacing out, then two), so spots weren't added where the tests check them.
- Set pieces: `ASetPieceSpawner` (an `AAmbushSpawner`; `Creatures/SetPieceSpawner*.cpp`, rules `SetPieceRules`), data in a
  layout entry's `setPiece` block (`Tools/Unreal/build_area_setpieces.py`'s docstring lists every key). An ambush with one
  is placed by `build_area_camps.py`; a fight of spawner `SetPieceSpawner` by `build_area_fights_mountain.place_one`, from
  `build_area_camps.py` when it says `"pass": "valley"` (Main Street, folder Gameplay), else in the mountain pass.
  - Springs on walking onto its ground, or with `springRadius` near its spot; not while a `waitFor` spawner is Engaged.
  - Pushes: each wave a beat (`FSetPieceBeat`: its spots, a caption line, a 2D sting, a world cue, a shake through
    `BossCameraShake::Kick`, dust puffs sifting from the roof: one up-trace each, once, none without a roof); the next push
    comes once `pushWhenLeft` or fewer are left (alive and owed), `pushAfter` s at the latest. `aroundPlayer`: its pushes
    round where the player stood as it sprang (`GetSpawnCenter`), and its shower bursts there.
  - `placed`: creatures at spots of their own (`FSetPiecePlaced`, world spots found by the build: a balcony's deck traced
    down off its facade, `high` the highest terrain within so far inside its ground); taken away with the rest when the
    player goes far, owed back; given up (removed) if they outlive the fight with the player gone.
  - `hush`: speaker points (by tag) shut (`bEnabled` off) from the first push until it's done or off; only the ones it shut
    are opened again.
  - Done (`SetPieceRules::IsDone`: Cleared after a wave, placed ones down): the cleared beat, a loot shower
    (`ABossLootShower::Throw` with the spawner's own `ULootDropComponent`: a transient table, the default table's guns,
    `RewardGuns` [2] at `RewardLuck` [1], `RewardAmmo` [2] chest-full, `RewardBonusAmmo` [1]; no silver), then the story flag
    (`UMissionRunner::SetFlag`; saved in the campaign record) last. `DoneFlag` is added to `ActiveWhen.WithoutFlags` as play
    begins, so a later load finds it off.
  - Cvars `Looter.Fight.SetPiece.*` [defaults]: `ShakeScale` [1], `PushWhenLeft` [-1: each set piece's own], `DustScale` [1],
    `RewardGuns` [2], `RewardLuck` [1], `RewardAmmo` [2], `RewardBonusAmmo` [1]. Tests `Looter.SetPiece.Rules`, `.Play`.
  - Hooks added to `AEncounterSpawner` for it: `OnStateChanged(From, To)` (from `SetState`) and `OnWaveStarted(Wave)`
    (from `StartWave`, before the wave is queued).
- Sounds: the beats use existing cues (Boss.Phase, Creature.RankSting, Creature.Unpaid.Shriek, Creature.Spider.Drop,
  Boss.Gravemother.Quake, Boss.Foreman.Slam, Boss.Loot.Burst); own SetPiece.* stings would be a sound task.
- Didn't check (needs a build and play): the Gilt Spur's deck trace (else the Bell-Ringer stands in the street), the
  Webwood Spitters' high spots, Main Street's street-edge spots (the probe rings them if a boardwalk is in the way), the
  gallery's spots and the 13 m spring on both ways out, perf with 10 fighting.

## Placeholder creatures (ROSTER, built 2026-10-22)

The user's roster ask (Docs/Story.md "Creature roster"): a named clay blob wherever the story wants a kind not modelled.
- A kind = `/Game/Data/Placeholders/DA_Placeholder_<Name>` (`UPlaceholderCreatureDefinition`) + a data-only
  `BP_Placeholder_<Name>` naming it (its own class: bestiary page, kill counts, encounter groups) + `DA_Bestiary_<Name>`,
  all from `Tools/Unreal/create_placeholder_creatures.py` (no args: PLACED kinds only, so the bestiary lists no unmet
  future silhouettes; `all`: every roster kind). The BP's defaults hold the numbers (`APlaceholderCreature::BakeDefinition`,
  called on its CDO): rerun the script after editing a kind; `Looter.Creatures.Placeholder.Pages` catches drift.
- Tag and page: "Name (plain type)" (the user, 2026-10-22). Pays like any creature (the user, 2026-10-22): `XPReward`
  through the kill rules, the default/rank loot tables, ranks, levels from the zone, traits; nothing special-cased.
- Body: SK_Slime with M_StylizedSurface on every slot (MID: `Color` the kind's clay, `Variation` 0.25). **The stretch rides on
  the `body` bone's pose, never the mesh component's scale**: Unreal composes an attached part's (and a physics body's)
  scale per axis in its parent's axes (FTransform has no shear), so a stretched component put the head 40 cm off and would
  stretch the hit hulls along the bone's axes. Head ball (the crit, Weapon + Visibility) and two lumps on the bone, their
  place and scale taken back through the bone's rest turn and stretch. Pose (breath, bob, tell, strike, hover/dive, slump,
  sink, rise-in) is `PlaceholderRules::PoseAt` on that one bone. A test level never initialises actors: shape and look are
  applied in BeginPlay when PostInitializeComponents was skipped.
- Feedback: hits are `ECreatureBlood::Clay` (clods, dust and a splat in the clay's colour, WeaponFXCreatures/Kills), death
  its own Gel burst plus `CreatureKillLook::Play`; cries borrowed by `EPlaceholderVoice` (Beast spider, Dead Unpaid, Clay
  slime), hits the slime's. Elements: body Other (neutral).
- Ranged: band BackOffWithin..CloseInTo (AttackRange = CloseInTo / 0.75), shoots within ShotRange with a clear world-static
  line, `ShowCharge` glow as the tell, pellets via `UEnemyProjectileSubsystem`, bites within 2.3 m; keep-away as the
  Spitter's without the strafe.
- Placing: `Tools/Unreal/build_area_placeholders.py` (hooked after the secrets in build_area.py's gameplay): 'like' entries
  copy another script's spawner (ground, roof band, level, spots, condition: the cellar's and depot's rats), 'center'
  entries are camps. Keep camps 20 m in from the Rim: hounds placed 15 m from it followed the player to the lip and fell
  (PIE 2026-10-22). A spawner sprung with the player standing on its spots finds nowhere to stand (by the player): normal.
- Check: `Looter.SpawnPlaceholder <Name> [rank] [count] [chase]`; `Looter.Encounter.Wave Placeholder_<Id> force`.
- Replacing one: model it, make its real class, point the page's ActorClass at it, swap the class in
  `build_area_placeholders.py`, delete its DA/BP.

## Bounties (BOUNTY, built 2026-10-22)

The user's pick (2026-10-22): a bounty board so Ransom's Rest has replayable hard fights before new regions. Design and
numbers: `Docs/Work/2026-10-22-Next/BOUNTY.md` (until archived). Code `Bounties/`, screen `UI/World/BountyBoardWidget*`,
placing `Tools/Unreal/build_area_bounties.py` (gameplay pass after the placeholders; runs alone; `Gameplay/Bounties`).
- **Board** `ABountyBoard` (`BountyBoard_RansomsRest`, id `RansomsRest`): SM_NoticeBoard_Town 2.8 m at the Depot (the user's
  note 12, 2026-10-27, "by the train station"): (-130, 10250) yaw -35, facing the Depot roost and the yard road (chosen from the
  layout only: look at it in the editor; the script warns if it is hidden). It stood beside the Rangers' board at (660, 1400)
  before. Open after Main 3; Dangerous after
  Main 5, Deadly after Main 6 (`OpenWhen`, `DangerousWhen`, `DeadlyWhen`). Sheets are code stand-ins (engine plane, wax
  seal cylinder in M_StylizedSurface, tier colour); a taken or won posting's sheet is gone. Own `AGiverMarkActor`: ? for
  unread postings, ! for a won one.
- **Subsystem** `UBountyBoardSubsystem` (0.5 s timer; tests call `Refresh`): record in `FCampaignRecord::Bounties` by
  board id (the runner's campaign, else the session's); clock = session time played (`SetTestClock` in tests). Posts on a
  fresh board, rotates every `RotateMinutes` [120], refills a paid slot after `RefillMinutes` [10], fills an empty slot
  when the story opens an arena (runner `OnChanged`). Draw `BountyRules::Draw` seeded by board id + serial, saved whole.
- **Fight** `ABountyFight` (an `ASetPieceSpawner`, spawned at the arena while a bounty is taken, every level start; never
  saved). Mark = the group of the tier's rank (posse is Fixed Basic/Restless only), named via `DisplayName` (tag reads
  "Gravebound Old Man Corrigan", traits line under it), traits set exactly to the posting's. Won on the mark's death
  (saved). No chest (the user's note 40, 2026-10-27; `ABountyChest` and `Looter.Bounty.ChestDrop` deleted): the loot comes when
  the bounty is turned in at the board. Paid at the board: XP + silver, the mission banner (a transient side-mission definition
  broadcast on the runner's `OnMissionCompleted`; title "Bounty: <mark>"), and `BountyPayout::HandOver` throws the tier's loot
  (the old strongbox's rolls: 2/2/3 guns-or-wards at luck 1, 2 ammo x36, a Strongbox's grenade chance, silver) out of the
  board's face toward the player, fanned; the best gun on the banner ("RARE GUN"). The mark and the reward table keep their
  legendary odds (`KeepingLegendaryOdds`, `bKeepsLegendaryOdds`: Gunplay.md, early cap). Old saves: `OwedChests` kept; handed
  over at the board (Interact or Collect) with Hob's line, the board shows ! while owed, never twice (`HandOverOwed`).
  Tracker = a `UMissionSubsystem` entry (steps Hunt, Put down, then turn-in); the minimap arrow follows it.
- **Harder** (note 33): +25% health on every bounty creature (posse, support, mark), `Looter.Bounty.HealthScale` [1.25] (not +1
  level: levels stay the zone's).
- **Arenas** (note 22: all bounties were Unpaid at one grove): new Ransom's Point (Spider, after Main 3, levels 1-2) and the
  quarry's mouth (Spider/Spitter, after Main 3, before Dead Lift, levels 5-6). Before Main 5 the draw has Scatter, Ransom's
  Point, Quarry Mouth, Whitlock Yard (Side 2), Webwood (Main 4 + flag). Old saves keep their Scatter-only postings until the
  rotation (`Looter.Bounty.Post` shows the new mix). QuarryMouth is next to the QuarryTrack patrol, which may join its fight.
  Not seen in play yet.
- **Arena waits** (`WaitForSpawners`): only the story's own fights on its ground. A camp, patrol or roaming group is
  Engaged whenever the player is near (its creatures stand about), so waiting on one held the Scatter's bounty until the
  orchard camp was cleared (PIE 2026-10-22): never wait on those.
- Cvars `Looter.Bounty.*` (BOUNTY.md lists them). Dev: `Looter.Bounty.List`, `.Post`, `.Take <1-3>`, `.Abandon`, `.Win`,
  `.Collect`, `.Make <slot> <Kind> <Tier> <Arena> [twist...]`, `.Forget`, `.Board` (by the board: the screen closes past
  4.5 m), `.Clock [min]`, `.Arenas [off]`. Tests `Looter.Bounty.Rules`, `.Rewards`, `.Pacing`, `.View`, `.Play`, `.Save`.
- **Checked in PIE** (2026-10-22, past Main 3 by `Looter.Mission.Complete`): the board screen (one Scatter posting, NEW,
  Take), tracker and minimap arrow, the Scatter fight (Unpaid rising, the Gravebound mark named with Grave-burst ·
  Fleet), won, strongbox dropped at its spot, paid +242 XP +41 silver, MISSION COMPLETE banner. Grabs
  `Saved/Screenshots/Tools/bounty_*.png`. Not checked in play: the Webwood, quarry, yard and meadow arenas, Dangerous and
  Deadly tiers, opening the strongbox by hand (the play test opens one).
- Lessons: a `UUserWidget` member named `Slot` hides `UWidget::Slot`, an error here (C4458 is an error under
  `BuildSettingsVersion.Latest`); `CreateWidget` runs `NativeOnInitialized` before `OpenFor` sets its fields, so bind
  delegates in `OpenFor`; copy a card's words before an action that remakes the cards.
