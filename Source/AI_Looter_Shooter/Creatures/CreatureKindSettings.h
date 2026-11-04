#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CreatureKindSettings.generated.h"

/** One creature kind's starting numbers: what it is at level 1 as a Regular, before its level and rank grow it. */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FCreatureKindStats
{
	GENERATED_BODY()

	/** The creature's class by name (SpiderCreature, UnpaidCreature, DredgerRifleman): that exact class, never a subclass (Abel keeps his own). */
	UPROPERTY(Config, EditAnywhere, Category = "Kind")
	FName ClassName;

	/** Its health at level 1 as a Regular; 0 keeps the class's own number. */
	UPROPERTY(Config, EditAnywhere, Category = "Kind", meta = (ClampMin = "0"))
	float Health = 0.f;

	/** Its attack's damage at level 1 as a Regular, before the fight's DamageScale; 0 keeps the class's own number. */
	UPROPERTY(Config, EditAnywhere, Category = "Kind", meta = (ClampMin = "0"))
	float AttackDamage = 0.f;
};

/**
 * The creature kinds' starting health and damage, tunable without code: Project Settings > Game > Creature Kinds, saved to
 * Config/DefaultGame.ini under [/Script/AI_Looter_Shooter.CreatureKindSettings]. A kind with no entry keeps its class's own
 * numbers (its constructor's). Read as a creature captures its base stats (ACreatureBase::CaptureBaseStats) and by the
 * Warden's Log (UBestiaryEntry::ReadStats), so the page and the fight agree. Kill speed (2026-11-03): the numbers here sit
 * just under the starter guns' breakpoints (a Regular dies to two shotgun blasts, three revolver shots, about nine rifle rounds).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Creature Kinds"))
class AI_LOOTER_SHOOTER_API UCreatureKindSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCreatureKindSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Kinds", meta = (TitleProperty = "ClassName"))
	TArray<FCreatureKindStats> Kinds;

	/** The project's settings. */
	static const UCreatureKindSettings& Get();

	/** The entry for this exact class, if any. */
	const FCreatureKindStats* Find(const UClass* Class) const;

	/** The health a creature of this exact class starts with at level 1 as a Regular: its entry's, else ClassValue (the class's own). */
	static float HealthFor(const UClass* Class, float ClassValue);
	/** Its attack's damage, likewise. */
	static float AttackDamageFor(const UClass* Class, float ClassValue);

	/** The same, read off the class itself (its default object's health component and AttackDamage). */
	static float BaseHealthOf(const UClass* Class);
	static float BaseAttackDamageOf(const UClass* Class);
};
