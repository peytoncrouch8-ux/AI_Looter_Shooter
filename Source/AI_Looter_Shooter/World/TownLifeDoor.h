#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TownLifeDoor.generated.h"

class USceneComponent;
class USpeakerPointComponent;

/**
 * A townsfolk's door on Ransom's Rest (Pruitt's store, the north and south cottages): nobody comes out and nobody can be
 * talked to, but as the player passes somebody inside mutters a line, shown as a caption, and the house is heard through
 * it now and then (its latch, a voice). It names its household (TownLifeRules): its speaker point mutters that household's
 * lines for the point in the story (TownLifeRules::MutterTopicsFor) unless it's given lines of its own, and it joins the
 * town's life (UTownLifeSubsystem) as one of the household's sound sources. Placed by Tools/Unreal/build_area_townlife.py
 * just outside the door, about head high. No mesh, no collision, no tick.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ATownLifeDoor : public AActor
{
	GENERATED_BODY()

public:
	ATownLifeDoor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Who mutters here and what (its Mutters; the Interact key is off: bTalkable). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpeakerPointComponent> SpeakerPoint;

	/** The household behind the door (Pruitt, CottageNorth, CottageSouth...; TownLifeRules::Households). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Life")
	FName Household;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
