#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/GraveSightFlash.h"
#include "Story/StoryCondition.h"
#include "ChapelReliquary.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Saint Ada's Reliquary in the chapel's apse, smashed, dark and empty (Docs/Areas/RansomsRest.md, Main 4 "Hallowed
 * Ground"; Docs/Story.md: "The Reliquary of Ransom's Rest lies smashed and dark in the chapel"): the backlog Reliquary's
 * smashed variant (Art/Models/Props/Reliquary.py; SM_Reliquary_Smashed or SM_Reliquary, its name still to be confirmed,
 * and the build script sets it), its foot on the chapel's SOCKET_Reliquary, its front (+X) toward the nave.
 *
 * Looked at (a tap of Interact, "Look at the Reliquary") while LookWhen holds (Main 4's fourth step on), it plays the
 * two-second Grave Sight flash: the cyan overlay on the screen (UGraveSightSubsystem) and, seen through it, Saint Ada's ember
 * lifting off the lid's empty gem setting (its model's SOCKET_Ember) and fading as it goes, what the gang took: the
 * cold open's ember look, an orange glow facing the camera with a little light of its own. When the flash ends the
 * missions hear SightEvent (Main 4's step waits for it, so its next step comes after the vision). It can't be looked at
 * again while its flash plays.
 *
 * Without its model (not imported yet) it's a stand-in of the engine's plain shapes, a granite chest with its lid knocked
 * askew and a brass band round it, and the ember rises from just over its lid. Solid either way: the player bumps it, and
 * the Interact key's line finds it (its model's UCX boxes). The Interact prompt anchors on its model's SOCKET_Interact
 * when it has one. It ticks only while its flash plays.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AChapelReliquary : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag missions find it by (Main 4's "Look at the Reliquary": its arrow). */
	static const FName ReliquaryTag;

	/** What the end of its flash sends the missions: Main 4's fourth step waits for it. */
	static const FName SightEvent;

	AChapelReliquary();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Looks at it as a tap of Interact does: the flash and the ember, and SightEvent once it's over. False when it can't be
	 * looked at now; bForce looks whatever LookWhen says (the console), though never over a flash still playing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Reliquary")
	bool Look(AActor* ByWhom, bool bForce = false);

	/** It can be looked at now: LookWhen holds, and its last flash is over. */
	UFUNCTION(BlueprintPure, Category = "Reliquary")
	bool CanLook() const;

	/** Its flash is playing. */
	bool IsFlashing() const { return Flash.IsPlaying(); }

	/** Its flash, playing or the last one. */
	const FGraveSightFlash& GetFlash() const { return Flash; }

	/** Flashes played to their end here. */
	int32 GetLooks() const { return Looks; }

	/** Moves its flash and the ember on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/** Where the ember lifts from: its model's SOCKET_Ember, else just over the top of the model or the stand-in's lid. */
	FVector GetEmberStart() const;

	/** Where the ember is now, and how brightly it burns (0: not seen). */
	FVector GetEmberLocation() const { return EmberAt; }
	float GetEmberGlow() const { return Flash.GetEmberGlow(); }

	/** Its model is in (otherwise the stand-in shows). */
	bool HasModel() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The smashed Reliquary's model, set by the build script; none: the stand-in shows. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** The prompt's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary")
	FText Prompt;

	/** It can be looked at only while this holds (Main 4 from its fourth step on). Empty: any time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary")
	FStoryCondition LookWhen;

	/** How far from the player's eyes it can be looked at (cm): a look reaches farther than a hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 350.f;

	/** The flash's length (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary", meta = (ClampMin = "0.1", Units = "s"))
	float FlashSeconds = FGraveSightFlash::DefaultSeconds;

	/** How high the ember lifts over the flash (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary", meta = (ClampMin = "0", Units = "cm"))
	float EmberRise = 140.f;

	/** Its model's sockets: where the ember lifts from, and where the prompt anchors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary")
	FName EmberSocket = TEXT("Ember");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary")
	FName InteractSocket = TEXT("Interact");

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The stand-in shows (and blocks) without the model, and is hidden (and lets everything through) with it. */
	void RefreshStandIn();

	/** The ember where the flash has it: rising, facing the camera, its glow and light. */
	void PlaceEmber();

	/** The ember and its light shown or put away. */
	void ShowEmber(bool bShown);

	/** The flash has ended: the ember goes, and the missions hear. */
	void FinishLook();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> StandIn;

	/** The ember: one camera-facing quad of the game's glow (as the cold open draws it). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> EmberGlow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> EmberLight;

	FGraveSightFlash Flash;
	FVector EmberAt = FVector::ZeroVector;
	int32 Looks = 0;
};
