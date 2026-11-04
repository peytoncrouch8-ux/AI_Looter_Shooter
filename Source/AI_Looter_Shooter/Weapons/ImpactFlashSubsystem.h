#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/VfxLightRules.h"
#include "ImpactFlashSubsystem.generated.h"

class AActor;
class UPointLightComponent;
struct FVfxTuning;

/**
 * The flash a hit throws on the rock (Phase 3, lane MUZZLE; the audit's change 8): a round into stone, metal or wood lights the
 * wall for ~60 ms. Four point lights (FImpactFlashPool's four slots) live on one transient actor made the first time a hit wants
 * one: none is spawned per hit, none is ever shadowed (Looter.Vfx.Impact.Light*; 0 turns them off), and each is hidden while
 * unused. FWeaponFX::SpawnSurfaceImpact calls Flash for the hit; this ticks only while a flash shows. Played worlds only (a test
 * level, which builds an editor preview world, gets no lights; the pool's rules are tested as plain data).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UImpactFlashSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UImpactFlashSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * Lights a hit on Material at Point (just off the surface, Normal its facing): the rules decide (VfxRules::ImpactLightShape): hard
	 * surfaces only, Weight the hit's, Eye where the camera was (nullptr: unknown), never nearer than Looter.Vfx.Impact.LightEye. True when a
	 * light was lit or refreshed for it.
	 */
	static bool Flash(const UObject* WorldContextObject, const FVfxTuning& Tuning, const FVector& Point, const FVector& Normal,
		EImpactMaterial Material, float Weight, const FVector* Eye);

	/** Flash lights lit now (the pool's live slots). */
	int32 NumLit() const { return Pool.NumLive(); }

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** The light that slot Index drives, made when first asked for (null in a world that isn't played). */
	UPointLightComponent* LightAt(int32 Index);

	FImpactFlashPool Pool;
	TWeakObjectPtr<AActor> LightOwner;
	TWeakObjectPtr<UPointLightComponent> Lights[FImpactFlashPool::MaxSlots];
};
