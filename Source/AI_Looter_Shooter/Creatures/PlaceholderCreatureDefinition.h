#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlaceholderCreatureDefinition.generated.h"

/** How a placeholder fights (Docs/Story.md, "Creature roster"): the roster's roles, each on today's fight director. */
UENUM(BlueprintType)
enum class EPlaceholderRole : uint8
{
	/** Comes in and strikes, waiting its turn on the ring like a spider. */
	Melee,
	/** Keeps its distance in its band and shoots pellets (UEnemyProjectileSubsystem) on a ranged turn; bites when caught. */
	Ranged,
	/** Small, quick and weak: a melee kind that comes in numbers (rats, knockers). */
	Swarm,
	/** Slow and heavy: a long wind-up, a blow that throws the player (golems, giants). */
	Big,
	/** Hovers over the ground and dives on its strike (bats, gulls, drakes). */
	Flyer,
};

/** Whose cries a placeholder borrows until it has its own sounds; its hits always sound of wet clay (the slime's). */
UENUM(BlueprintType)
enum class EPlaceholderVoice : uint8
{
	/** Animals: the spider's alert, attack, hurt and death. */
	Beast,
	/** The dead and the cursed: the Unpaid's. */
	Dead,
	/** Things with no voice (golems, insects): the slime's. */
	Clay,
};

/**
 * One kind of placeholder creature (the user's ask, 2026-10-21: "a placeholder blob of clay with the name of the enemy"):
 * its names, role, clay body's size and colour, numbers and, for a ranged kind, its shots. One asset per kind in
 * /Game/Data/Placeholders (DA_Placeholder_<Name>), with a data-only BP_Placeholder_<Name> child of APlaceholderCreature
 * naming it, so each kind is its own class (its own bestiary page, kill counts and encounter groups). Both, and the page,
 * come from Tools/Unreal/create_placeholder_creatures.py; its numbers are written into the BP's defaults there
 * (APlaceholderCreature::BakeDefinition), which is what play, spawners and the bestiary read.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UPlaceholderCreatureDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Its name in the world: "Coffin Rat". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder")
	FText DisplayName;

	/** The plain kind of thing it stands for, so it reads at a glance what will go there: "rat", "goblin" (the user's call). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder")
	FText PlainType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder")
	EPlaceholderRole Role = EPlaceholderRole::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder")
	EPlaceholderVoice Voice = EPlaceholderVoice::Beast;

	// --- The clay body (cm, at size 1: its BodyScale and rank grow it on top) ---

	/** Front to back, side to side, foot to crown: the clay blob is stretched to these. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Body", meta = (ClampMin = "10", Units = "cm"))
	float Length = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Body", meta = (ClampMin = "10", Units = "cm"))
	float Width = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Body", meta = (ClampMin = "10", Units = "cm"))
	float Height = 80.f;

	/** Its head: a clay ball (the critical spot) this wide, its middle at this share of the height and of the half-length forward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Body", meta = (ClampMin = "5", Units = "cm"))
	float HeadSize = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Body", meta = (ClampMin = "0", ClampMax = "1.5"))
	float HeadHeight = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Body", meta = (ClampMin = "-1", ClampMax = "1.2"))
	float HeadForward = 0.f;

	/** The clay's colour (linear): a tint per kind, never a final look. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Body")
	FLinearColor ClayColor = FLinearColor(0.42f, 0.24f, 0.14f);

	/** A flyer's body rides this high over its feet (cm), diving to the ground on its strike. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Body", meta = (ClampMin = "0", Units = "cm"))
	float HoverHeight = 0.f;

	// --- Numbers at level 1 (ACreatureBase's: its level and rank grow them as any creature's) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "1"))
	float MaxHealth = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0"))
	float AttackDamage = 8.f;

	/** Its kill's experience at level 1, through the kill rules like any creature's (the user's call, 2026-10-22). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0"))
	int32 XPReward = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0", Units = "cm/s"))
	float WalkSpeed = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0", Units = "cm/s"))
	float ChaseSpeed = 420.f;

	/** Where its strike starts (cm, capsule middle to target middle, flat). A ranged kind's comes from CloseInTo instead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "50", Units = "cm"))
	float AttackRange = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0.05", Units = "s"))
	float AttackWindup = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0.05", Units = "s"))
	float AttackRecovery = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0", Units = "s"))
	float AttackCooldown = 1.2f;

	/** How hard its blow lands and throws (HitWithAttack's strength): a big kind's 1.6. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0.1", ClampMax = "3"))
	float StrikeStrength = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0", Units = "cm"))
	float AggroRadius = 2600.f;

	/** Its kind's pack call when one is hurt (0: none; packs of hounds and rats call far). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers", meta = (ClampMin = "0", Units = "cm"))
	float PackAlertRadius = 1500.f;

	/** Kinds that answer each other's calls (the gravepickers' diggers and lookouts). None: only its own kind. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Numbers")
	FName PackTag;

	// --- A ranged kind's shots (Role Ranged) ---

	/** It shoots at a target nearer than this, comes in to CloseInTo, and backs away inside BackOffWithin (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "200", Units = "cm"))
	float ShotRange = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "200", Units = "cm"))
	float CloseInTo = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "100", Units = "cm"))
	float BackOffWithin = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "200", Units = "cm/s"))
	float ShotSpeed = 2600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "2", Units = "cm"))
	float ShotRadius = 9.f;

	/** Pellets a shot fires (a cone ShotSpread wide), each this share of its AttackDamage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "1", ClampMax = "9"))
	int32 ShotCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "0", ClampMax = "40"))
	float ShotSpread = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "0"))
	float ShotDamageShare = 1.f;

	/** Its aim leads a moving target by this share of the flight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots", meta = (ClampMin = "0", ClampMax = "1"))
	float LeadShare = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder|Shots")
	FLinearColor ShotColor = FLinearColor(1.f, 0.55f, 0.2f);

	/** "Coffin Rat (rat)": its name, then the plain type in brackets, on its tag and its bestiary page (the user's call). */
	FText GetTagName() const;

	/** Its numbers make sense (each reason it doesn't goes into OutProblems): for the tests and the script's log. */
	bool IsSound(TArray<FString>& OutProblems) const;
};
