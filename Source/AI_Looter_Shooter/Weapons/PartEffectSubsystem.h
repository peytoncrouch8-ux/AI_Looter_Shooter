#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PartEffectSubsystem.generated.h"

class AActor;
class UBulletSubsystem;
struct FBulletLanding;
struct FBulletShot;
struct FBulletStrike;
struct FHitResult;

/**
 * The guns' parts in the bullets (Phase 12.3): listens to UBulletSubsystem's three hooks and plays each gun's part effect
 * and element (UPartEffectComponent knows the gun's).
 *  - Firing: stamps the shot with its gun's element (its tracer tinted toward the element's colour), effect and pierce;
 *    an Overcharge round takes its charge's damage; a Twin shot fires its second bullet beside the first.
 *  - Landing: an Explosive round keeps Direct of itself for what it hit; a salted creature's standing ward takes a bullet
 *    GraveSaltWardBonus times (salted wards break faster).
 *  - Struck: the element's look on every hit and its roll for the status (UElementStatusComponent::TryApply); a Piercing
 *    round flies on through a creature; a Ricochet round bounces once to the nearest creature it can see within its
 *    radius; an Explosive round splashes the rest of itself round where it landed (never through walls, never on the
 *    shooter).
 * Every number: FPartEffectTuning (Looter.PartEffect.*) and FElementTuning (Looter.Element.*).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPartEffectSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Bounces, splashes and statuses set since play began (the tests and Looter.PartEffect.Show). */
	int32 NumRicochets() const { return Ricochets; }
	int32 NumSplashes() const { return Splashes; }
	int32 NumStatusesSet() const { return StatusesSet; }

	/**
	 * Of the creatures near From (within Radius, alive, not Except), the nearest From can see (one trace each, nearest first,
	 * at most a few): where a Ricochet round bounces. Null for none.
	 */
	AActor* FindBounceTarget(const FVector& From, float Radius, const AActor* Except) const;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void NoteFiring(FBulletShot& Shot);
	void NoteLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing);
	void NoteStruck(const FBulletShot& Shot, const FHitResult& Hit, FBulletStrike& Strike);

	void Bounce(const FBulletShot& Shot, const FHitResult& Hit, float Damage);
	void Splash(const FBulletShot& Shot, const FHitResult& Hit, float Damage);

	TWeakObjectPtr<UBulletSubsystem> Bullets;
	FDelegateHandle FiringHandle;
	FDelegateHandle LandingHandle;
	FDelegateHandle StruckHandle;

	int32 Ricochets = 0;
	int32 Splashes = 0;
	int32 StatusesSet = 0;
	/** The last splash's pop sound (an SMG's stream of them sounds as a few). */
	double LastPopSound = -1000.0;
	double LastBounceSound = -1000.0;
};
