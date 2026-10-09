#pragma once

#include "CoreMinimal.h"

/**
 * The controls the contextual hints teach (Docs/Polish/TutorialRework.md, "Contextual hints"), in the order they're
 * weighed when several are due at once: the most pressing first.
 */
enum class EControlHint : uint8
{
	/** Move and look: only for a player who hasn't moved for a while after control came. */
	Move,
	/** Reload: the magazine low, with rounds to reload from. */
	Reload,
	/** Melee: a creature close in front, and the player hasn't struck yet. */
	Melee,
	/** Grenade: a crowd (two or more creatures) ahead, a grenade in hand, and none thrown yet. */
	Grenade,
	/** Aim down sights: something to shoot far off under the crosshair. */
	Aim,
	/** Jump (and climb, where the mantle is): a knee-to-chest ledge just ahead. */
	Jump,
	/** Sprint: walking a while without it. */
	Sprint,
	/** Slide: sprinting a while without it. */
	Slide,
	/** Swap guns: a second gun in the slots. */
	Swap,
	/** The inventory: a gun picked up past the first. */
	Inventory,
	/** The gunsmith's bench: at one with two guns of a kind. */
	Bench,
	Count
};

/** What the player is doing, as UControlHintSubsystem sees it at each look (ControlHintSubsystemSenses.cpp gathers it). */
struct AI_LOOTER_SHOOTER_API FControlHintInput
{
	/** The player can play: a living pawn in their hands, no menu or scene holding them, the game running. Hints wait otherwise. */
	bool bInControl = false;
	/** Hints are wanted (Settings > Interface > Control hints). */
	bool bEnabled = true;
	/** How far (cm) the player walked or ran on the ground since the last look; looking around doesn't count. */
	float Moved = 0.f;
	/** On the ground and moving, not sprinting. */
	bool bWalking = false;
	bool bSprinting = false;
	bool bSliding = false;
	/** Left the ground going up (a jump), or came back down onto ground a good step higher (a climb, a mantle). */
	bool bJumped = false;
	/** A ledge of knee to chest height just ahead, the player heading for it. */
	bool bLedgeAhead = false;
	bool bGunInHand = false;
	/** The crosshair is on something that can be hurt, farther than FControlHintRules::FarTargetDistance. */
	bool bFarTarget = false;
	bool bAiming = false;
	/** The gun in hand is low (a quarter of its magazine or less, or empty) with rounds to reload from. */
	bool bMagazineLow = false;
	bool bReloading = false;
	/** A live creature is close in front, in the strike's cone and in sight, and this body hasn't struck yet (UPlayerMeleeComponent::FindTargetInReach). */
	bool bCloseTarget = false;
	/** A melee strike started since the last look. */
	bool bMeleed = false;
	/** Two or more live creatures ahead in sight, with a grenade to throw and none thrown yet (UPlayerThrowComponent::CountTargetsAhead). */
	bool bCrowdAhead = false;
	/** A grenade throw started since the last look. */
	bool bThrew = false;
	/** Guns in the equip slots. */
	int32 GunsEquipped = 0;
	/** A gun came into the player's hands since the last look, past their first. */
	bool bPickedUpGun = false;
	/** The gun in hand changed to another slot's since the last look (not by a pickup). */
	bool bSwapped = false;
	bool bInventoryOpen = false;
	/** Close to a gunsmith's bench, carrying two guns of one kind. */
	bool bAtBenchWithPair = false;
	bool bBenchOpen = false;
};

/** One hint's memory: how often it has shown and whether the player has shown they know its control. */
struct AI_LOOTER_SHOOTER_API FControlHintMemory
{
	int32 Shows = 0;
	bool bLearned = false;
};

/** How the last hint on screen went away. */
enum class EControlHintEnd : uint8
{
	None,
	/** Its control was used: the widget flashes it before it goes. */
	Done,
	/** It showed its time (or its moment passed) unheeded: it fades. */
	TimedOut,
	/** Hints were turned off, or another was forced over it. */
	Hidden,
};

/**
 * The contextual hints' rules, apart from the world so tests can drive them: one hint at a time, never blocking, each only
 * while its control hasn't been used yet and at most a few times; gone the moment its control is used. The world's side
 * (what the player is doing, the profile's memory, the HUD) is UControlHintSubsystem's.
 *
 *  - A control used at any time is learned for good, so a hint never teaches what the player already does (someone who
 *    sprints from the start never sees the sprint hint), and learning goes on with hints turned off or under a menu (the
 *    inventory hint's control is opening a menu).
 *  - Triggers, in control only: not moved for MoveIdleSeconds since control came (move); WalkSeconds of walking since the
 *    last sprint (sprint); SlideSprintSeconds of sprinting (slide); a ledge ahead (jump); a far target with a gun in hand
 *    and not aiming (aim); a low magazine with a reserve, not reloading (reload); a creature close in front, before the
 *    first strike (melee); a crowd of two or more ahead with a grenade in hand, before the first throw (grenade); two guns
 *    in the slots (swap); a gun picked up past the first (inventory, due until learned); at a bench with two guns of a kind
 *    (bench).
 *  - A hint shows up to ShowSeconds; one about a moment (a ledge, a far target, a low magazine, a close creature, a crowd, the bench) goes LingerSeconds
 *    after the moment passes. Unheeded, it may come again after RetrySeconds, until it has shown MaxShows times.
 *  - GapSeconds of quiet between two hints. With hints off, the one on show goes and none come.
 */
class AI_LOOTER_SHOOTER_API FControlHintRules
{
public:
	// --- When hints come (the design's numbers) ---

	static constexpr float MoveIdleSeconds = 6.f;
	static constexpr float WalkSeconds = 8.f;
	static constexpr float SlideSprintSeconds = 4.f;
	/** A target farther than this (cm) under the crosshair is worth raising the sights for. */
	static constexpr float FarTargetDistance = 2500.f;
	/** The magazine counts as low at or under this share of it. */
	static constexpr float LowMagazineShare = 0.25f;
	/** Walked this far (cm, all told), the player knows how to move. */
	static constexpr float LearnMoveDistance = 150.f;

	// --- How they show ---

	/** The longest a hint stays up when its control isn't used. */
	static constexpr float ShowSeconds = 8.f;
	/** A hint about a moment goes this long after the moment passes. */
	static constexpr float LingerSeconds = 2.5f;
	/** Quiet between two hints. */
	static constexpr float GapSeconds = 1.5f;
	/** An unheeded hint waits this long before it may come again. */
	static constexpr float RetrySeconds = 45.f;

	/** How many times it may show before it gives up: once for the jump and the slide (the design's "once"), three for the rest. */
	static int32 MaxShows(EControlHint Hint);

	/** The key it teaches, by its binding id (UKeyBindingSubsystem: "Sprint"; Move for the four movement keys). */
	static FName Action(EControlHint Hint);

	/** What the key does, after the keycap. bToggle: the player switched its key to press-to-toggle (sprint). */
	static FText Words(EControlHint Hint, bool bToggle = false);

	/** Its name in the saved memory ("Sprint"): never rename one, or a profile forgets what it learned. */
	static FName Id(EControlHint Hint);

	/** The hint with that name, or Count. */
	static EControlHint FromId(FName InId);

	/** It's about a moment that passes (a ledge, a far target, a low magazine, a close creature, a crowd, the bench) rather than a habit. */
	static bool IsMomentary(EControlHint Hint);

	/** One look's worth: what the player did is learned, the shown hint's time runs, and the next one may come. */
	void Update(const FControlHintInput& In, float DeltaSeconds);

	/** The hint on screen, or Count for none. */
	EControlHint GetShown() const { return Shown; }
	bool IsShowing() const { return Shown != EControlHint::Count; }

	/** How long the hint on screen has shown (in control). */
	float GetShownSeconds() const { return ShownFor; }

	/** The last hint that went away and how. */
	EControlHint GetLastEnded() const { return LastEnded; }
	EControlHintEnd GetLastEnd() const { return LastEnd; }

	/** Grows by one each time a hint goes away, so a widget can tell a new ending from the one before. */
	int32 GetEndCount() const { return EndCount; }

	const FControlHintMemory& GetMemory(EControlHint Hint) const;
	void SetMemory(EControlHint Hint, const FControlHintMemory& InMemory);

	/** It won't show again: learned, or shown its most. */
	bool IsRetired(EControlHint Hint) const;

	/** Its control was used: learned for good, and gone from the screen if it was on show. */
	void Learn(EControlHint Hint);

	/** Shows it now, whatever its triggers and memory say (Looter.Hints show, the HUD photos). */
	void ForceShow(EControlHint Hint);

	/** Forgets everything learned and shown (Looter.Hints reset), and the hint on show goes. */
	void ResetMemory();

	/** The memory changed (a show, a lesson) since this was last asked: the subsystem saves it. */
	bool ConsumeChanged();

private:
	static int32 IndexOf(EControlHint Hint) { return static_cast<int32>(Hint); }

	/** Every control the player used this look is learned. */
	void LearnFrom(const FControlHintInput& In);

	/** The triggers' clocks and the inventory's pending pickup, in control. */
	void Notice(const FControlHintInput& In, float DeltaSeconds);

	/** Its trigger holds now. */
	bool Wants(EControlHint Hint, const FControlHintInput& In) const;

	void Show(EControlHint Hint);
	void End(EControlHintEnd How);

	FControlHintMemory Memories[static_cast<int32>(EControlHint::Count)];
	/** Seconds before an unheeded hint may come again. */
	float Cooldowns[static_cast<int32>(EControlHint::Count)] = {};

	EControlHint Shown = EControlHint::Count;
	float ShownFor = 0.f;
	/** Seconds the shown momentary hint's moment has been gone. */
	float MomentGone = 0.f;
	/** Seconds since the last hint went (the gap starts open). */
	float SinceEnd = GapSeconds;

	EControlHint LastEnded = EControlHint::Count;
	EControlHintEnd LastEnd = EControlHintEnd::None;
	int32 EndCount = 0;

	// The triggers' clocks
	float IdleFor = 0.f;
	float MovedTotal = 0.f;
	float WalkFor = 0.f;
	float SprintFor = 0.f;
	/** A gun was picked up past the first, and the inventory isn't learned yet. */
	bool bInventoryDue = false;

	bool bChanged = false;
};
