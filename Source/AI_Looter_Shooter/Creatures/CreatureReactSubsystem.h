#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureReactRules.h"
#include "Subsystems/WorldSubsystem.h"
#include "CreatureReactSubsystem.generated.h"

class UDirectionalLightComponent;

/**
 * What the creatures' reactions share in a level (UCreatureReactComponent): the allowance for the dust their feet kick up
 * and the ash the Unpaid crumble to (FCreatureDustBudget: nobody's dust is thrown past it), where the local players' views
 * are this frame (dust nobody is near enough to see isn't thrown), and the light the dust is in (the puffs are unlit, so
 * they take the sun's hue and the lighting state's tint, as FSlideDust's do, looked up every couple of seconds). No tick:
 * the creatures ask it. Played worlds, and the editor preview worlds the automated tests build.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureReactSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UCreatureReactSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Footfall puffs a second across the level, and at once. */
	static constexpr float FootfallsPerSecond = 40.f;
	static constexpr float FootfallBurst = 12.f;
	/** Ash motes a second across the level, and at once (a few Unpaid crumbling together). */
	static constexpr float AshPerSecond = 60.f;
	static constexpr float AshBurst = 20.f;

	/** How many of Wanted footfall puffs may be thrown now. */
	int32 TakeFootfalls(int32 Wanted);
	/** How many of Wanted ash motes may be thrown now. */
	int32 TakeAsh(int32 Wanted);

	/** Whether a local player's camera (or pawn) is within Within (cm) of Point. */
	bool IsNearAViewer(const FVector& Point, float Within);

	/** The light unlit dust shows in (a multiplier of its own color): the sun's hue and height, the sky, the lighting state. */
	FLinearColor GetDustLight();

private:
	void RefreshViewers();
	void RefreshLight();

	FCreatureDustBudget Footfalls{ FootfallsPerSecond, FootfallBurst };
	FCreatureDustBudget Ash{ AshPerSecond, AshBurst };

	/** The local players' cameras and pawns, as last looked at (once a frame at most). */
	TArray<FVector, TInlineAllocator<4>> Viewers;
	uint64 ViewersFrame = MAX_uint64;

	TWeakObjectPtr<const UDirectionalLightComponent> Sun;
	FLinearColor DustLight = FLinearColor::White;
	double LightLookedAt = -1000.0;
};
