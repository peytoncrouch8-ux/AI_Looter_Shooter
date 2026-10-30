#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"
#include "Subsystems/WorldSubsystem.h"
#include "StatusCalloutSubsystem.generated.h"

class AActor;

/**
 * A status's word over a creature the first time it is set on it (Elements matter, 2026-10-29): "BURNING", "SMITTEN",
 * "SALTED", "JOLTED", "ROTTING" (ElementRules::StatusWord) in the element's colour with its glyph, popping in over the
 * creature with UI.StatusCallout, so the player learns what their gun just did. It hears every status set
 * (UElementStatusComponent::OnStatusSet) and shows only a creature's first of each element (bFirstOnThisCreature), only
 * with a local player to see it, and never over a creature whose damage numbers are hidden (a boss between phases).
 * Two at once on one creature stack up the screen (CalloutSlot).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UStatusCalloutSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Seconds a callout counts as just shown on its creature (a second one then sits a slot higher). */
	static constexpr float StackSeconds = 0.6f;

	/** Where a creature's callout shows: this share of its half height over its middle. */
	static constexpr float CalloutHeight = 0.85f;

	/** The callout's slot for one more on a creature that showed Recent callouts within StackSeconds. */
	static int32 CalloutSlot(int32 Recent) { return FMath::Max(Recent, 0); }

private:
	void HandleStatusSet(AActor* Target, ELooterElement Element, bool bFirstOnThisCreature);

	FDelegateHandle StatusSetHandle;

	/** The creatures that showed a callout lately, and when (so two at once stack). */
	struct FRecent
	{
		TWeakObjectPtr<const AActor> Creature;
		double When = 0.0;
	};
	TArray<FRecent> Recent;
};
