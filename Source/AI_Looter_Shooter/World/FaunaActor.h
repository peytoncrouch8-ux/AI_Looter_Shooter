#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/FaunaTypes.h"
#include "FaunaActor.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

/**
 * Ambient life in a level (Docs/Polish/BorderlandsComparison.md, item 5: motion in the corner of the eye): the base of
 * the flocks, swarms, tumbleweeds, dust devils and washing (World/Fauna*). None of it ticks on its own: UFaunaSubsystem
 * updates each one as often as its distance from the view and whether it's on screen allow (FaunaRules::UpdateInterval),
 * hides it past CullDistance, outside its lighting state (ShownIn) or while Looter.Fauna is 0, and hands it the frame's
 * threats and noises (FFaunaContext). Everything it draws is passable: no collision, no navigation, never an Obstacle,
 * no shadows unless a subclass asks. Placed by Tools/Unreal/build_area_fauna.py with its data (perches, zones, lanes);
 * its components are made as play begins.
 */
UCLASS(Abstract)
class AI_LOOTER_SHOOTER_API AFaunaActor : public AActor
{
	GENERATED_BODY()

public:
	AFaunaActor();

	/** Moves it on by Tick.DeltaSeconds: the subsystem calls this at the rate it's allowed (tests call it directly). */
	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context);

	/** The middle of what it draws (world) and how far that reaches from there (cm). */
	virtual FVector GetFaunaCenter() const;
	virtual float GetFaunaRadius() const;

	/** How near what it draws comes to Location (cm; 0 inside its reach). */
	float DistanceFrom(const FVector& Location) const;

	/** Whether something of it is on the move (birds in the air, a roll): it updates more often then. */
	virtual bool IsBusy() const { return false; }

	/** Shows or hides everything it draws (and stops its sounds while hidden). */
	void SetFaunaShown(bool bShown);
	bool IsFaunaShown() const { return bFaunaShown; }

	/** Whether it's out in the lighting state Lighting: ShownIn None is out in all of them. */
	bool IsInItsLight(FName Lighting) const;

	/** Past this from the view (cm) it hides and only keeps time (0: never). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna", meta = (ClampMin = "0", Units = "cm"))
	float CullDistance = 8000.f;

	/** The lighting state it's out in (Day, Dusk); None: every state, and a level without states. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	FName ShownIn;

	/** Its seed: every run plays out the same from the same start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fauna")
	int32 Seed = 1;

	/** The subsystem's bookkeeping: when it's next due, when it last updated (world seconds), on screen then. */
	double NextUpdateTime = 0.0;
	double LastUpdateTime = -1.0;
	bool bOnScreenLast = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Called as it's shown or hidden: a subclass stops its loops here. */
	virtual void OnFaunaShownChanged(bool bShown) {}

	/**
	 * A new instanced component drawing Mesh (in play; nothing is saved): movable, passable, no navigation, no decals or
	 * distance fields, a shadow only with bCastShadows, each instance culled on its own at CullDistance.
	 */
	UInstancedStaticMeshComponent* MakeInstances(FName Name, UStaticMesh* Mesh, bool bCastShadows = false);

	/** The seed as the rules take it. */
	uint32 SeedBits() const { return static_cast<uint32>(Seed); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

private:
	bool bFaunaShown = true;
};
