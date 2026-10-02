#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "InteractionSubsystem.generated.h"

class AActor;

/**
 * The level's interactables (actors implementing IInteractable), so the players' interaction components look among a
 * few dozen of them instead of every actor in the level. Each one joins as its play begins and leaves as it ends
 * (AInteractableProp does both). Loot isn't here: the weapon manager offers it (IInteractionSource).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UInteractionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UInteractionSubsystem* Get(const UObject* WorldContextObject);

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** An interactable joins (as its play begins); joining twice changes nothing. */
	void Register(AActor* Interactable);

	/** It leaves (as its play ends). */
	void Unregister(AActor* Interactable);

	/** Every interactable that joined; ones destroyed since may be among them (null). */
	const TArray<TWeakObjectPtr<AActor>>& GetInteractables() const { return Interactables; }

private:
	TArray<TWeakObjectPtr<AActor>> Interactables;
};
