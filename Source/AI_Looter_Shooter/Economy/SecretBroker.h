#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SecretBroker.generated.h"

class AActor;

/**
 * Someone who sells the level's secrets (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 12; lanes/SECRETS.md): Mister
 * Sexton at his ferry (AMisterSexton makes one). Its owner asks it whether the Interact key trades now (ShouldTrade: the
 * level's catalog open in the story, the owner shown, and no mission waiting to be turned in or offered at the owner,
 * which the talk keeps first, as Pruitt's hatch does), and has it open the screen (USecretsWidget). What's sold and when is
 * the level's catalog (USecretSubsystem); this only says who sells it. No tick.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API USecretBrokerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USecretBrokerComponent();

	/** The Interact prompt's words while he trades. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secrets")
	FText Prompt;

	/** The level's catalog is open in the story now. */
	bool IsOpen() const;

	/** A mission waits to be turned in at the owner, or is offered there: the story's talk comes first. */
	bool HasStoryBusiness() const;

	/** The key trades now: open, the owner shown, no story business. */
	bool ShouldTrade() const;

	/** Opens the screen for Player (their pawn or controller). False when it can't (shut, no local player). */
	bool OpenFor(AActor* Player);
};
