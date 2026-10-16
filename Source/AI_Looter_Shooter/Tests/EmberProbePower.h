#pragma once

#include "CoreMinimal.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "EmberProbePower.generated.h"

/**
 * A power that does nothing but count what the component asks of it, for Tests/EmberTests.cpp (Looter.Embers.Component).
 * A UCLASS has to live in a header for the reflection tool, so it can't sit in the test's .cpp; hidden from the editor's
 * class pickers so no definition can name it.
 */
UCLASS(NotBlueprintable, NotBlueprintType, HideDropdown)
class AI_LOOTER_SHOOTER_API UEmberProbePower : public UEmberPower
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const override { return Refusal; }

	virtual void Activate(const FEmberContext& Context) override
	{
		++Activations;
		LastLevelScale = Context.LevelScale;
		LastDefinition = Context.Definition ? Context.Definition->Id : NAME_None;
	}

	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override
	{
		++Ticks;
	}

	virtual void End(const FEmberContext& Context, bool bInterrupted) override
	{
		++Ends;
		bLastInterrupted = bInterrupted;
	}

	/** What CanActivate answers (None: it may fire). */
	EEmberDenied Refusal = EEmberDenied::None;

	int32 Activations = 0;
	int32 Ticks = 0;
	int32 Ends = 0;
	bool bLastInterrupted = false;
	float LastLevelScale = 0.f;
	FName LastDefinition;
};
