# Soul flask + slower regen (2026-10-30, Main's contract)

The user's playtest note 2: "HP regen should be slower and you should have to be out of combat completely before it
starts to regen. Add a way to heal during combat." Their answers:
- regen fills a bar in about 30 s, and only once you're out of combat;
- a boss's helpers (adds) drop a soul-mote half the time (was the plain 12%);
- regular kills keep today's mote chances;
- AND a soul flask on a key that works in boss fights.

Rule 7 (CLAUDE.md): every action feels rewarding, with high-quality visual and sound effects. Rule 1: no copyrighted
content (nothing modelled on any game's healing flask, no potion sounds lifted from anything).

## The design (numbers are the contract; tune only in the rules header)

### The flask
- A small stoppered glass flask of soul-light, in the soul-motes' pale green (LooterUI::Color::Heal).
- Carried from the start of every game, with 3 charges (MaxCharges).
- Key H: binding id "Flask", rebindable in Settings > Controls, category "Combat".
- Gamepad: a free button the GAMEPLAY agent picks and names in its README. Check KeyBindingSubsystem.cpp and every
  Gamepad_ use first.

### Drinking
- A press starts a 1.1 s drink (DrinkSeconds).
- The cork comes out at once (FlaskFX::Uncork).
- The heal starts at 0.35 s (SipAt, FlaskFX::Drink). It gives 35% of the most health (HealShare), spread over 0.8 s
  (HealSeconds), so it reads as a gulp, not a switch.
- Once started, it finishes: a hit doesn't cancel it.
- The player keeps moving at 0.75 speed (MoveScale).
- During the drink the player can't fire, aim, reload, throw, strike or use an ember power. The gun dips out of the way
  exactly as the grenade throw dips it (AWeaponBase's melee swing state and pose, reused).
- The next press works 0.4 s after a drink ends (RepeatSeconds).

### When a press is refused (no charge used)
- No charges: FlaskFX::Denied(false), a dry clink; the HUD's pips flash red and shake.
- Full health: FlaskFX::Denied(true), a soft no; the HUD says "Health full" once.
- Also refused when dead, in a scene, behind a menu, mid-throw, mid-strike, mid-mantle or vault, or already drinking.
- Swimming: allowed if the hands allow it. GAMEPLAY decides and says which.

### Refilling with soul
- SoulPerCharge is 100.
- Each kill the player makes gives KillSoul(Rank, bBoss, bBossFight):

  | Kill | Soul |
  |---|---|
  | Basic | 15 |
  | Restless | 30 |
  | Gravebound | 45 |
  | Hallowed, Ethereal | 60 |
  | Legendary (Soulfed) | 100 |
  | A boss (has a UBossComponent) | 300 (all three charges) |
  | Any kill during a boss fight (FightState::IsBossFightOn) | at least 25 |

- Overflow carries into the next charge. With all 3 charges full, soul isn't kept (SoulShare 0).
- A completed charge plays FlaskFX::ChargeFilled, and the HUD's pip lights with a pop.

### Soul-motes
- A soul-mote touched at full health no longer goes to waste. If the flask isn't full, the mote drifts in (same magnet)
  and pours 34 soul (MoteSoul) into the flask instead of healing.
- At full health with a full flask, the mote ignores the player, as now.

### Death and saving
- Respawning refills all three charges (FlaskFX::RefilledAll, quiet).
- A new game starts full.
- The flask is saved with the session (FFlaskSave: Charges, Soul). A save from before the flask existed reads as full
  (bSaved false means full).

### Regen and boss helpers (Main)
- Regen (wounds-that-close) waits for both conditions:
  - no creature has fought the player for 4 s (CalmSeconds); "fighting" is ACreatureBase::IsFightingWith, the music's
    rule;
  - the old 7 s have passed since the last hurt.
- Then it heals 0.035 of the most health a second, about 30 s for a full bar.
- A running boss fight counts as combat the whole time.
- Boss helpers: any creature except the boss itself, killed while a boss fight is on (FightState::IsBossFightOn), drops a
  mote 50% of the time (BossFightMoteChance), or at its rank's chance if that's higher.

## Shared API (exact; compile against these)

### Combat/FightState.h (Main writes it first; read-only for agents)
```cpp
namespace FightState
{
	/** Some living creature is fighting Player now (ACreatureBase::IsFightingWith), or a boss fight is on. */
	AI_LOOTER_SHOOTER_API bool IsInCombat(const UWorld* World, const APawn* Player);
	/** A boss's fight runs (a UBossComponent fighting, or a Legendary creature after Player: the Gravemother). */
	AI_LOOTER_SHOOTER_API bool IsBossFightOn(const UWorld* World, const APawn* Player);
}
```

### Player/PlayerFlaskRules.h (GAMEPLAY)
```cpp
struct AI_LOOTER_SHOOTER_API FFlaskRules
{
	static constexpr int32 MaxCharges = 3;
	static constexpr float SoulPerCharge = 100.f;
	static constexpr float DrinkSeconds = 1.1f;
	static constexpr float SipAt = 0.35f;
	static constexpr float HealSeconds = 0.8f;
	static constexpr float HealShare = 0.35f;
	static constexpr float RepeatSeconds = 0.4f;
	static constexpr float MoveScale = 0.75f;
	static constexpr float MoteSoul = 34.f;
	static constexpr float BossFightKillSoul = 25.f;
	/** Soul one kill gives (see the design). */
	static float KillSoul(ECreatureRank Rank, bool bBoss, bool bBossFight);
	/** Share of the drink's heal given by Seconds into the drink (0 before SipAt, 1 from SipAt + HealSeconds; smooth). */
	static float HealShareAt(float Seconds);
};

enum class EFlaskChange : uint8 { Drunk, Filled, Soul, Refilled, Restored, DeniedEmpty, DeniedFull };
enum class EFlaskBlock : uint8 { None, Empty, FullHealth, Busy, Dead, Scene, Menu, Moving /* mantle, vault */ };
```

### Player/PlayerFlaskComponent.h (GAMEPLAY)
```cpp
/** Count now, change (+1 filled, -1 drunk, 0 denied/soul), why. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnFlaskChanged, int32 /*Charges*/, int32 /*Delta*/, EFlaskChange /*Why*/);

UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerFlaskComponent : public UActorComponent
{
public:
	static UPlayerFlaskComponent* Find(const AActor* Pawn);
	bool TryDrink();
	EFlaskBlock GetBlock() const;
	bool IsDrinking() const;
	/** Seconds into the drink (0 when not drinking), and that over DrinkSeconds. */
	float GetDrinkTime() const;
	float GetDrinkAlpha() const;
	/** The heal is flowing now (between SipAt and SipAt + HealSeconds). */
	bool IsHealing() const;
	int32 GetCharges() const;
	static int32 GetMaxCharges() { return FFlaskRules::MaxCharges; }
	/** 0-1 toward the next charge (0 when full). */
	float GetSoulShare() const;
	void AddSoul(float Soul);
	void RefillAll(EFlaskChange Why = EFlaskChange::Refilled);
	void SaveFlask(FFlaskSave& OutSave) const;
	void RestoreFlask(const FFlaskSave& Save);
	FOnFlaskChanged OnFlaskChanged;
};
```

### Player/PlayerFlaskSave.h (GAMEPLAY)
USTRUCT FFlaskSave { int32 Charges = 3; float Soul = 0; bool bSaved = false; }

### Player/FlaskFX.h (FX), with Player/PlayerFlaskCues.h (FX: namespace FlaskCue, the cue names)
```cpp
namespace FlaskFX
{
	/** The press: the cork's pop and squeak; the flask's light brightens (sound at the hands, 2D for the player). */
	AI_LOOTER_SHOOTER_API void Uncork(APawn& Drinker);
	/** SipAt: the gulp and the soul's rush: green light streaming from Mouth (the flask's SOCKET_Mouth in the world)
	 *  into the view / the chest, and a warm swell (FlaskCue::Drink, FlaskCue::Heal). */
	AI_LOOTER_SHOOTER_API void Drink(APawn& Drinker, const FVector& Mouth);
	/** A charge filled by soul: a clear little chime (FlaskCue::ChargeFilled). */
	AI_LOOTER_SHOOTER_API void ChargeFilled(APawn& Drinker);
	/** Refused: empty (a dry glass clink, FlaskCue::Empty) or full health (a soft muted tap, FlaskCue::Full). */
	AI_LOOTER_SHOOTER_API void Denied(APawn& Drinker, bool bFullHealth);
	/** Respawn's top-up: a soft three-note fill (FlaskCue::Refilled). */
	AI_LOOTER_SHOOTER_API void RefilledAll(APawn& Drinker);
}
```
The cue names (FX writes the recipes; Main renders and imports them):
- Player.Flask.Uncork
- Player.Flask.Drink
- Player.Flask.Heal
- Player.Flask.ChargeFilled
- Player.Flask.Empty
- Player.Flask.Full
- Player.Flask.Refilled

### The model (FX): SM_SoulFlask, /Game/Art/Items/SM_SoulFlask, from Art/Models/Items/SoulFlask.py
- Size: about 16 cm tall, held in the off hand at first-person scale. Check it against SaltGrenade.py's notes about size
  in the hand.
- Pivot: the middle of its foot. SOCKET_Mouth sits at the neck's opening (+Z out).
- Look: a worn, slightly crooked glass flask in a leather and tin harness, with a cork.
- The liquid is an emissive soul-green: an emissive surface, no translucency needed. Nanite off (it's small).
- Materials go through existing texture sets and instances (CLAUDE.md "Assets").
- The liquid's glow brightness is a scalar the code can drive ("Glow"). With 0 charges the flask reads dim or empty.
  GAMEPLAY sets "Fill" (0-1) if the material offers it; FX says in its README which parameters exist.

## Who owns what (two agents never own one file)

### MAIN
- Combat/FightState.*, Combat/RecoverySettings.*, Combat/WoundsClose.*
- Combat/PlayerVitalsSubsystem.* (calls RefillAll on respawn)
- Creatures/FightTuning.*, Loot/LootDropComponent.*, and the existing recovery tests
- CODEMAP, docs, builds, imports, git
- Any hook an agent writes in its README for a file it doesn't own

### GAMEPLAY (Opus)
- Player/PlayerFlaskRules.*, Player/PlayerFlaskComponent.* (plus a split .cpp if it runs long), Player/PlayerFlaskSave.h
- Player/FirstPersonHandsFlask.cpp (new): the off hand brings the flask up to the mouth and tips it, mirroring
  FirstPersonHandsThrow.cpp; plus the edits to Player/FirstPersonHands*.{h,cpp} that this needs.
- The third-person arm too, if the anim instance has a throw-like hook; otherwise say so.
- Core/LooterCharacter.{h,cpp}: adds the component.
- Settings/KeyBindingSubsystem.*: the key.
- Session/SessionSubsystemWorld.cpp and the session save struct: FFlaskSave beside Throwables.
- Loot/SoulMotePickup.*: the full-health mote feeds the flask.
- The player's move-speed scale while drinking, wherever the throw and melee set theirs.
- Dev/FlaskDevCommands.cpp: Looter.Flask.Fill [n], Looter.Flask.Drink, Looter.Flask.Status.
- Tests/PlayerFlaskTests.cpp (Looter.Flask.*).
- Kills: bind UPlayerProgressionSubsystem::OnKillXP (no edits to Main's files).

### HUD (Opus)
- Files:
  - UI/HUD/HudFlaskWidget.{h,cpp} (new) and UI/HUD/PlayerHUDWidget.{h,cpp}.
  - UI/HUD/HudScreenEdgeWidget.*: a soft heal-green glow at the screen's edges while IsHealing.
  - UI/HUD/HudControlHintWidget.*, if a first-time hint fits: "H  Soul flask" the first time health drops under 40% in
    a fight.
  - Art/Icons/InkedIcons.py and UI/Style/InkedIconData.inl: a SoulFlask inked icon. If you can't regenerate the .inl,
    say in the README how Main does it.
  - Tests for the widget's rules, if any (Looter.UI.Flask).
- Placement: near the player frame, bottom-left (healing belongs with health). Never over the health bar's numbers, the
  XP bar or the minimap.
- Look: floating outlined type and metal-edged shapes, no backing panel (UI rules in CLAUDE.md). A key tab like the
  grenade counter's.
- Pips: 3 of them. A lit pip is a full charge; the next one fills as a liquid level with GetSoulShare.
- Flashes:
  - drunk: a pip pours out;
  - filled: a pop and a white flash;
  - denied, empty: red and a shake;
  - denied, full: "Health full".
- Colours only from LooterUI::Color (Heal for the soul-light). Every painted background calls MarkBackground.

### FX (Opus)
- Player/FlaskFX.{h,cpp} and Player/PlayerFlaskCues.h.
- A new flask recipe file in Art/Sounds/recipes. Add its cues.json entries only through the recipe's own mechanism, and
  render only your own cues: Tools\sounds.ps1 Player.Flask.*
- Art/Models/Items/SoulFlask.py and its preview. Run Blender only through Tools\artrun.ps1.
- Any new M_FX_ use goes through existing masters; say what Main must build.
- Use the existing code-drawn FX helpers (Weapons/WeaponFX*, the soul-mote's look) rather than new systems.

## Rules for agents
- Holding tree: put the C++ you change in `Saved/FlaskCode/<AGENT>/Source/AI_Looter_Shooter/...`: copies of the files
  you change, plus your new files, with the originals' `git hash-object` in HASHES.txt.
- Edit the Python, art and sound files you own in place.
- Never build, open Unreal or touch git.
- Write a README in your tree: files, what it does, CODEMAP lines, hooks Main adds, run order, tests, risks.
- Read CLAUDE.md, Docs/Pipeline.md and the pipeline notes that bear on your part first: Game.md (HUD, gun card),
  Gunplay.md (grenades, throw), Assets.md (models), Workflow.md (sound recipes).
- One class per file, under ~500 lines a file. Comments say why, in plain words. Tabs, UE naming, LogLooter.
