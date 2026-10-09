#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "BossTypes.generated.h"

class ACreatureBase;

/**
 * A boss fight as data (UBossComponent::Phases): phases by the share of health left, each starting a few events. The
 * events cover what every boss does (adds, spells it can't be hurt through, volleys of pellets); anything only one boss
 * does (Abel's grief, the bell, the lanterns) is a Custom event its own code answers (UBossComponent::OnCustomEvent).
 * How the fight is shown (FBossShow), its weak spot's stagger (FBossStagger) and its loot (FBossLootShowerSettings) are data too.
 */

UENUM(BlueprintType)
enum class EBossEventKind : uint8
{
	/** A wave of adds rises around the boss (Wave). */
	AddWave,
	/** The boss can't be hurt for a while, or until its adds are dead (Untargetable). */
	Untargetable,
	/** The boss fires a volley of slow pellets at the player (Volley). */
	Volley,
	/** Tells the boss's own code that something scripted happens now (Name: "Bell", "Grieve"). */
	Custom
};

/** A wave of adds: creatures spawned in play around the boss, set on the player, gone for good once killed. */
USTRUCT(BlueprintType)
struct FBossAddWave
{
	GENERATED_BODY()

	/** What rises; none: the boss's own kind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TSubclassOf<ACreatureBase> CreatureClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	ECreatureRank Rank = ECreatureRank::Basic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "1", ClampMax = "10"))
	int32 Count = 2;

	/** Their size (BodyScale): 0.45 makes spiderlings. 0 keeps the class's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0", ClampMax = "5"))
	float BodyScale = 0.f;

	/** Their own level, before their rank's. 0: what the area gives a creature spawned in play (its band). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0"))
	int32 Level = 0;

	/** How far from the boss they rise (cm), spread round it on the same level of ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "100", Units = "cm"))
	float Radius = 600.f;

	/**
	 * They rise round the fight's spot (the middle of its arena, on its ground) instead of round the boss where it is now:
	 * Abel's adds rise through the deck while he hangs in the fog over the canyon.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	bool bAroundSpot = false;

	/**
	 * The wave only tops the boss's adds up to this many alive ("two rise every 25 s, at most 4"). 0: the boss's own cap
	 * (UBossComponent::MaxAliveAdds), which is never more than 10.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0", ClampMax = "10"))
	int32 MaxAlive = 0;
};

/** A spell the boss can't be hurt through: its bar greys, hits do nothing. */
USTRUCT(BlueprintType)
struct FBossUntargetable
{
	GENERATED_BODY()

	/** The longest it lasts (seconds). 0: no time limit; it ends with its adds, or when the boss's own code ends it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Untargetable", meta = (ClampMin = "0", Units = "s"))
	float Seconds = 0.f;

	/** It ends once every add is dead and the phase has no wave left to send. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Untargetable")
	bool bUntilAddsDie = true;

	/** The boss stops fighting while it lasts and goes back to its spot (ACreatureBase::SetPassive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Untargetable")
	bool bWithdraw = true;

	/** What the bar says under it while it lasts ("Kill her brood"); empty: the bar only greys. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Untargetable")
	FText Hint;
};

/** A volley of slow, glowing pellets in a cone at the player (UEnemyProjectileSubsystem), after a short tell. */
USTRUCT(BlueprintType)
struct FBossVolley
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley", meta = (ClampMin = "1", ClampMax = "24"))
	int32 Pellets = 5;

	/** The cone's full width (degrees): one pellet down the middle, the rest round its edge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley", meta = (ClampMin = "0", ClampMax = "90", Units = "deg"))
	float SpreadDegrees = 14.f;

	/** cm/s: slow enough to see coming and step out of. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley", meta = (ClampMin = "100"))
	float Speed = 1000.f;

	/** Each pellet's damage as a share of the boss's attack (its bite), so it grows with its level and rank. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley", meta = (ClampMin = "0"))
	float DamageShare = 0.35f;

	/** A pellet's radius (cm), which is also how close it must pass to hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley", meta = (ClampMin = "2", Units = "cm"))
	float Radius = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley", meta = (ClampMin = "100", Units = "cm"))
	float Range = 4000.f;

	/** How long a glow grows at the muzzle before the pellets leave (seconds): the tell to dodge on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley", meta = (ClampMin = "0", Units = "s"))
	float WindupSeconds = 0.6f;

	/** Where the pellets leave from, in the boss's own frame at size 1 (forward, right, up from its middle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley")
	FVector Muzzle = FVector(80.0, 0.0, 20.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volley")
	FLinearColor Color = FLinearColor(0.55f, 1.f, 0.2f);
};

/** One thing that happens in a phase, some seconds after it starts, once or over and over while the phase lasts. */
USTRUCT(BlueprintType)
struct FBossPhaseEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	EBossEventKind Kind = EBossEventKind::AddWave;

	/** Seconds after its phase starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0", Units = "s"))
	float Delay = 0.f;

	/** Again every this many seconds while its phase lasts. 0: once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0", Units = "s"))
	float RepeatEvery = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "Kind == EBossEventKind::AddWave", EditConditionHides))
	FBossAddWave Wave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "Kind == EBossEventKind::Untargetable", EditConditionHides))
	FBossUntargetable Untargetable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "Kind == EBossEventKind::Volley", EditConditionHides))
	FBossVolley Volley;

	/** The scripted moment a Custom event names, for the boss's own code. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (EditCondition = "Kind == EBossEventKind::Custom", EditConditionHides))
	FName Name;

	// Events made in code (the test boss, a boss class's defaults).
	static FBossPhaseEvent MakeWave(const FBossAddWave& InWave, float InDelay = 0.f, float InRepeatEvery = 0.f)
	{
		FBossPhaseEvent Event;
		Event.Kind = EBossEventKind::AddWave;
		Event.Wave = InWave;
		Event.Delay = InDelay;
		Event.RepeatEvery = InRepeatEvery;
		return Event;
	}

	static FBossPhaseEvent MakeUntargetable(const FBossUntargetable& InSpell, float InDelay = 0.f)
	{
		FBossPhaseEvent Event;
		Event.Kind = EBossEventKind::Untargetable;
		Event.Untargetable = InSpell;
		Event.Delay = InDelay;
		return Event;
	}

	static FBossPhaseEvent MakeVolley(const FBossVolley& InVolley, float InDelay, float InRepeatEvery)
	{
		FBossPhaseEvent Event;
		Event.Kind = EBossEventKind::Volley;
		Event.Volley = InVolley;
		Event.Delay = InDelay;
		Event.RepeatEvery = InRepeatEvery;
		return Event;
	}

	static FBossPhaseEvent MakeCustom(FName InName, float InDelay = 0.f, float InRepeatEvery = 0.f)
	{
		FBossPhaseEvent Event;
		Event.Kind = EBossEventKind::Custom;
		Event.Name = InName;
		Event.Delay = InDelay;
		Event.RepeatEvery = InRepeatEvery;
		return Event;
	}
};

/** One phase of a boss fight: it starts when the boss's health falls to its share, and its events play out from then. */
USTRUCT(BlueprintType)
struct FBossPhase
{
	GENERATED_BODY()

	/** Shown under the boss bar while the phase lasts ("You brought them here"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	FText Name;

	/**
	 * The share of health left at which it starts: 1 for the first phase (the fight's start), 0.6 for one at 60%. The bar
	 * shows a tick at each later phase's share.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase", meta = (ClampMin = "0", ClampMax = "1"))
	float HealthShare = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase")
	TArray<FBossPhaseEvent> Events;
};

/**
 * How a boss's fight is shown (UBossComponent, BossComponentShow.cpp): the bar sweeping in with its title, a sting and the
 * boss's own cry at its start, at each later phase, at a stagger and at its death; the camera's shakes; and the slow beat as
 * it dies. A boss whose own code plays a moment (the Gravemother roars as she rears) leaves that cue empty and its shake 0.
 */
USTRUCT(BlueprintType)
struct FBossShow
{
	GENERATED_BODY()

	/** Spelled out under the bar as it sweeps in, before the first phase's name ("Brood of the Sink"). Empty: none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show")
	FText Title;

	/** The boss's own cries, played at its body (the bar's stings are every boss's). None: only the sting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show")
	FName IntroCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show")
	FName PhaseCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show")
	FName StaggerCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show")
	FName DeathCue;

	/** How hard the camera shakes (0-1, BossCameraShake) as the fight starts, at a later phase, at a stagger, at its death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show", meta = (ClampMin = "0", ClampMax = "1"))
	float IntroShake = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show", meta = (ClampMin = "0", ClampMax = "1"))
	float PhaseShake = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show", meta = (ClampMin = "0", ClampMax = "1"))
	float StaggerShake = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show", meta = (ClampMin = "0", ClampMax = "1"))
	float DeathShake = 1.f;

	/**
	 * At its death the world slows to this (1: not at all) for DeathSlowSeconds of real time, the last third at half the
	 * way back, then runs on: the killing blow lands, the body falls, the bar empties.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show", meta = (ClampMin = "0.05", ClampMax = "1"))
	float DeathSlowMo = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show", meta = (ClampMin = "0", ClampMax = "3", Units = "s"))
	float DeathSlowSeconds = 0.7f;

	/** The bar's last line as it empties ("DEFEATED" when empty), and its call as the boss staggers ("STAGGERED" when empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show")
	FText DefeatedLine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Show")
	FText StaggerCallout;
};

/**
 * A weak spot that breaks the boss for a moment (UBossComponent): critical hits build it up, and a build-up of CritShare
 * of its most health, landed close enough together (the build-up drains away over DrainSeconds), staggers it. The boss's
 * own code does the reeling (the Gravemother sinks on her legs; Abel drops to a knee, his coal open).
 */
USTRUCT(BlueprintType)
struct FBossStagger
{
	GENERATED_BODY()

	/** Critical damage that staggers it, as a share of its most health. 0: it never staggers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stagger", meta = (ClampMin = "0", ClampMax = "1"))
	float CritShare = 0.f;

	/** A full build-up drains to nothing over this long (s): the crits must come close together. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stagger", meta = (ClampMin = "0.5", Units = "s"))
	float DrainSeconds = 4.f;

	/** How long it reels (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stagger", meta = (ClampMin = "0.2", Units = "s"))
	float Seconds = 2.5f;

	/** After a stagger, nothing builds up for this long (s): it can't be kept down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stagger", meta = (ClampMin = "0", Units = "s"))
	float Cooldown = 9.f;
};

/**
 * A dead boss's loot thrown out Borderlands-style (ABossLootShower) in place of its loot drop component's toss: its table
 * rolled as its kill rolls it, plus BonusAmmo chest-full boxes, popped out one piece at a time in high arcs all round, the
 * ammo first and the guns last, rarest last of all.
 */
USTRUCT(BlueprintType)
struct FBossLootShowerSettings
{
	GENERATED_BODY()

	/** Off: its loot drops as any creature's does. Turned off after play began (the tests), nothing drops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bEnabled = true;

	/** Ammo boxes on top of its table's, each a chest's full box (LooterLoot::ChestAmmoAmount), leaning to the kill gun's class. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0", ClampMax = "10"))
	int32 BonusAmmo = 2;

	/** After the death, before the first piece flies (s): the body falls first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0", Units = "s"))
	float Delay = 1.f;

	/** Between two ammo boxes (s); a gun waits GunPause on top, so each one is seen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0.01", Units = "s"))
	float Interval = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0", Units = "s"))
	float GunPause = 0.18f;

	/** Pieces land this near and this far from where they burst out (cm, on level ground). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0", Units = "cm"))
	float MinReach = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0", Units = "cm"))
	float MaxReach = 430.f;

	/** How fast they're thrown up (cm/s): high arcs, so the shower is seen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "100", Units = "cm/s"))
	float UpSpeed = 780.f;

	/** It bursts from the arena's middle (the boss's spot) rather than the body: Abel may die at the deck's open end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bFromSpot = false;

	/** It waits for a scene playing (or about to, inside Delay) to end: Abel's loot comes after he sits with Ellis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bAfterScene = false;
};
