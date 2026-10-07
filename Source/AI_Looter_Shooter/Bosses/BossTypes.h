#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "BossTypes.generated.h"

class ACreatureBase;

/**
 * A boss fight as data (UBossComponent::Phases): phases by the share of health left, each starting a few events. The
 * events cover what every boss does (adds, spells it can't be hurt through, volleys of pellets); anything only one boss
 * does (Abel's grief, the bell, the lanterns) is a Custom event its own code answers (UBossComponent::OnCustomEvent).
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
