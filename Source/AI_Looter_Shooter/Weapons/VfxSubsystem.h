#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/DeathSplats.h"
#include "VfxSubsystem.generated.h"

/**
 * The VFX pass's level-wide pieces (Saved/Vfx/PLAN.md): the ground splats under kills and blasts (FDeathSplats: a pool of
 * decals spreading out, holding and fading; Looter.Vfx.Splat*). Ticks only while a splat shows. Played worlds and the
 * editor preview worlds the automated tests build. The code-drawn particles stay FWeaponFX's (UBulletSubsystem's).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UVfxSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UVfxSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * A splat of Kind on the ground at Ground (its Normal) for a body or blast Size cm across (100 = a spider at size 1),
	 * Tint the creature's own (a slime's gel). False when none was laid (Looter.Vfx.Splats off, no material yet, no room).
	 */
	bool AddSplat(const FVector& Ground, const FVector& Normal, EVfxSplat Kind, const FLinearColor& Tint, float Size);

	/** Spreads the fresh splats and hides the ended ones (the tick; a test level, which doesn't tick, calls it). */
	void Advance(float DeltaSeconds);

	/** Splats showing now (tests). */
	int32 NumSplats() const;

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	FDeathSplats Splats;
	/** A splat was laid and hasn't ended: tick until it has. */
	double LiveUntil = -1.0;
};
