#pragma once

#include "CoreMinimal.h"
#include "Elements/ElementCallout.h"
#include "Subsystems/WorldSubsystem.h"
#include "DamageNumberSubsystem.generated.h"

class AActor;
class ADamageNumberActor;

/** One hit's damage number as UHealthComponent::SpawnDamageNumber hands it over. */
struct FDamageNumberHit
{
	/** What was hit (its numbers merge; a status's pieces on it roll into its stream). */
	AActor* Target = nullptr;
	float Damage = 0.f;
	bool bCritical = false;
	/** Where it landed (the number rises from here). */
	FVector Where = FVector::ZeroVector;
	/** How it shows its element (ElementCallout::LookFor): colour, WEAK / RESIST tag, size. */
	FElementNumberLook Look;
	/** A status's piece being dealt now (UElementStatusComponent::IsDealingNow): it joins the creature's stream. */
	bool bStatusPiece = false;
	/** The number's class (the health component's DamageNumberClass; none: the plain one). */
	UClass* Class = nullptr;
};

/**
 * The damage numbers' merge and pool rules as plain data (UDamageNumberSubsystem plays them; Looter.UI.DamageNumbers.Merge
 * checks them). The audit (2026-11-03): a shotgun piled eight numbers on one chest, one spawned actor per pellet.
 */
namespace DamageNumberMerge
{
	/** The numbers, each a Looter.UI.DamageNumbers.* cvar; the fields' values are the defaults. */
	struct AI_LOOTER_SHOOTER_API FTuning
	{
		/** .MergeSeconds: hits on one target this soon after its number's first are that number's (0: every hit its own). */
		float MergeSeconds = 0.08f;
		/** .MergeGrow / .MergeGrowMax: a merged number is 1 + MergeGrow per hit after the first its size, up to MergeGrowMax. */
		float Grow = 0.04f;
		float GrowMax = 1.3f;
		/** .PoolSize: the numbers made at play start and never more (a burst past it takes over the oldest hit's). */
		int32 PoolSize = 20;

		static FTuning Live();
	};

	/** Whether a hit at Now joins a number whose first hit landed at GroupStart (within MergeSeconds; never with 0, never back in time). */
	AI_LOOTER_SHOOTER_API bool Joins(double GroupStart, double Now, float MergeSeconds);

	/**
	 * Whether a hit with Next's look can join a number of Shown's: the same element, and both or neither a status's own colour
	 * (bShownStatusTint: the number's first hit was a smite's burst, say). A crit joins a plain hit's number (it turns it a crit).
	 */
	AI_LOOTER_SHOOTER_API bool SameLook(ELooterElement ShownElement, bool bShownStatusTint, const FElementNumberLook& Next, bool bNextCritical);

	/** A number's look after Next joined it: a tag any of its hits called out, the element's tint if any had it, the biggest size. */
	AI_LOOTER_SHOOTER_API FElementNumberLook Merged(const FElementNumberLook& Shown, const FElementNumberLook& Next);

	/** A merged number's size for Hits in it (1 for one hit). */
	AI_LOOTER_SHOOTER_API float GrowFor(int32 Hits, const FTuning& Tuning);
}

/**
 * Every floating damage number (change 2 of the 2026-11-03 audit): a pool of ADamageNumberActor made at play start (like
 * UXpPopupSubsystem's popups) and used again, so a hit spawns nothing. The hits on one target inside MergeSeconds are one
 * number summing them (a shotgun blast is one big number; a crit pellet in it makes it a crit; the WEAK / RESIST tag and the
 * element's colour keep), which keeps rising from where it started. A status's pieces roll into the creature's stream as
 * before (DamageNumberActorStream.cpp). Only the player who dealt the damage sees it (UHealthComponent::SpawnDamageNumber).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UDamageNumberSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UDamageNumberSubsystem* Get(const UObject* WorldContextObject);

	/** A hit's number at the world's time now; the number that shows it (merged into, a stream, or a new one), null for none. */
	ADamageNumberActor* ShowHit(const FDamageNumberHit& Hit);

	/** The same at Now (the tests drive the clock). */
	ADamageNumberActor* ShowHitAt(const FDamageNumberHit& Hit, double Now);

	/** Makes numbers up to the pool's size (play start does it, so the first fight makes none). */
	void Prewarm();

	/** Creature's live stream of Element, and how many it has of any element (where a new one stacks). */
	ADamageNumberActor* FindStream(const AActor* Creature, ELooterElement Element) const;
	int32 CountStreams(const AActor* Creature) const;

	/** Numbers made so far (kept), those on show, and how many were ever spawned (none per hit once warm). */
	int32 NumPooled() const;
	int32 NumShowing() const;
	int32 NumMade() const { return Made; }

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** A free number of Class (made if the pool has room), else the oldest hit's number of it. */
	ADamageNumberActor* Take(UClass* Class, int32 PoolSize);
	ADamageNumberActor* Make(UClass* Class);
	/** The number on show Hit joins: its target's, inside the window, of a look it can share. */
	ADamageNumberActor* FindMerge(const FDamageNumberHit& Hit, double Now, float MergeSeconds) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ADamageNumberActor>> Pool;

	int32 Made = 0;
};
