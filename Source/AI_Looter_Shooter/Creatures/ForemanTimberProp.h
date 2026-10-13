#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForemanTimberProp.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * One of the six timber props holding up the Foreman's stope (Docs/Areas/RansomsRest_Expansion.md, "The Providence": the
 * stope's props and the roof): solid cover from floor to roof, until the Foreman's roof strike brings rock down on it and it
 * splinters, its post gone and a stump left, so the cover changes through the fight. He cracks the one he strikes first
 * (a shudder, dust off it). Mended whole again whenever his fight starts over or he's called back, and on every arrival
 * (nothing of it is saved).
 *
 * Its model is SM_RR_MineProp (a post with its cap piece; its pivot at its foot, standing along +Z) and SM_RR_MinePropStump,
 * found once the mountain kit has them; until then a plain cylinder of Height stands in (and a short one for the stump).
 * Blocks the player, creatures and shots like a chest (BlockAllDynamic, world dynamic). The build script places them in
 * the stope; the Foreman finds those within his arena (AForemanCreature::GetProps). Tagged TimberProp.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AForemanTimberProp : public AActor
{
	GENERATED_BODY()

public:
	/** The tag the Foreman and the console find them by. */
	static const FName PropTag;

	static const TCHAR* const ModelPath;
	static const TCHAR* const StumpPath;

	AForemanTimberProp();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** It still stands (whole, or cracked). */
	bool IsStanding() const { return !bSplintered; }

	/** Struck by the Foreman's pick handle: it shudders and sheds dust (it splinters when the rock lands). */
	void Crack();

	/** Rock lands on it: its post splinters away from From, a stump left; it gives no cover now. False if it was down already. */
	bool Splinter(const FVector& From);

	/** Whole again (the fight starting over, the Foreman called back). */
	void Mend();

	/** Its foot on the floor (world): where the roof's rock is aimed when it's the one struck or the player's cover. */
	FVector GetFoot() const { return GetActorLocation(); }

	/** Every prop within Radius of Center (flat), standing only if asked, nearest first. */
	static TArray<AForemanTimberProp*> FindNear(const UWorld* World, const FVector& Center, float Radius, bool bStandingOnly);

	/** Moves its shudder on (the tick does while it shudders; the tests call it). */
	void Advance(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Post;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Stump;

	/** The stand-in's height and width (cm) while it has no model: floor to the stope's roof. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber Prop", meta = (ClampMin = "50", Units = "cm"))
	float Height = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timber Prop", meta = (ClampMin = "5", Units = "cm"))
	float Width = 30.f;

private:
	/** Its models, or the stand-ins sized by Height and Width. */
	void ApplyModels();
	void SplinterEffects(const FVector& From);

	bool bSplintered = false;
	bool bUsesStandIn = false;
	float ShudderClock = -1.f;
	FTransform PostRest = FTransform::Identity;
};
