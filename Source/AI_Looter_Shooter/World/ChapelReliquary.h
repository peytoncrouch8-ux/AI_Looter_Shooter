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
 * Saint Ada's Reliquary in the chapel's apse, whole (the Wardens story since 2026-11-01, Main 4 "A Coal Under Ash";
 * Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 2: "The Reliquary is whole, not smashed; dim since Wednesday"): a
 * granite chest banded in gold, her ember in the lid's recess, guttering, a coal under ash. Its model (the user's Tripo
 * Reliquary, TRIPO_MODELS.md M45-1, set by build_area_chapel.py) stands on the chapel's SOCKET_Reliquary, its front (+X)
 * toward the nave; without it a stand-in of the engine's plain shapes (a granite chest, its lid shut, a brass band, a dark
 * recess on top).
 *
 * Her ember always shows: a small orange glow with a little light of its own, flickering low and uneven (GutterAt), dimmer
 * and faster while a hose drinks her (SetDrinking: Main 4's window hose), bright and steady while LitWhen holds (Main 7:
 * her keeper home, "burns like a sunrise"). Looked at (a tap of Interact, "Look at the Reliquary") while LookWhen holds
 * (Main 4's fifth step on), it plays the two-second Grave Sight flash: the cyan overlay on the screen (UGraveSightSubsystem)
 * and, seen through it, her ember flaring once through the grey, lifting a little off the recess, then sinking back to its
 * gutter. When the flash ends the missions hear SightEvent (Main 4's step waits for it). It can't be looked at again while
 * its flash plays.
 *
 * Solid either way: the player bumps it, and the Interact key's line finds it (its model's UCX boxes). The Interact prompt
 * anchors on its model's SOCKET_Interact when it has one. It ticks only while the player is near (the gutter), or its flash
 * plays.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AChapelReliquary : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag missions find it by (Main 4's "Look at Saint Ada's Reliquary": its arrow). */
	static const FName ReliquaryTag;

	/** What the end of its flash sends the missions: Main 4's fifth step waits for it. */
	static const FName SightEvent;

	AChapelReliquary();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	/** The Reliquary as it stands, anywhere on it. */
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Looks at it as a tap of Interact does: the flash and her ember's flare, and SightEvent once it's over. False when it
	 * can't be looked at now; bForce looks whatever LookWhen says (the console), though never over a flash still playing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Reliquary")
	bool Look(AActor* ByWhom, bool bForce = false);

	/** It can be looked at now: LookWhen holds, and its last flash is over. */
	UFUNCTION(BlueprintPure, Category = "Reliquary")
	bool CanLook() const;

	/** A hose drinks her (AChapelWindowHose): her ember gutters lower and faster. */
	void SetDrinking(bool bInDrinking) { bDrinking = bInDrinking; }
	bool IsDrinking() const { return bDrinking; }

	/** Her keeper's home (LitWhen holds, or a scene lit her: SetLitNow): her ember burns bright and steady. */
	bool IsLit() const;

	/**
	 * Lights her now, whatever the story says until it next agrees (SaintAdaWakes, as Abel's light pours into her, before
	 * the mission's step has moved on); false gives the word back to LitWhen. (Main 7, lane MAIN67.)
	 */
	void SetLitNow(bool bInLit);

	/**
	 * Her blaze over her lit ember (0 none, 1 the waking's white-gold blaze, three times her lit size, bright enough to light
	 * the apse): the scene's to ride up and down; she ticks while it's on.
	 */
	void SetBlaze(float Share);
	float GetBlaze() const { return Blaze; }

	/** Its flash is playing. */
	bool IsFlashing() const { return Flash.IsPlaying(); }

	/** Its flash, playing or the last one. */
	const FGraveSightFlash& GetFlash() const { return Flash; }

	/** Flashes played to their end here. */
	int32 GetLooks() const { return Looks; }

	/** Moves its flash, the flare and the gutter on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/** Where the ember lies: its model's SOCKET_Ember, else just over the top of the model or the stand-in's lid. */
	FVector GetEmberStart() const;

	/** Where the ember is now, and how brightly it burns: its gutter, its flare on top while the flash plays, or lit. */
	FVector GetEmberLocation() const { return EmberAt; }
	float GetEmberGlow() const { return EmberGlow; }

	/** The gutter's strength Time seconds in (0.45-1: an ember's low, uneven breathing); faster and lower while drinking. */
	static float GutterAt(float Time, bool bDrinking);

	/** Its model is in (otherwise the stand-in shows). */
	bool HasModel() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The whole Reliquary's model, set by the build script; none: the stand-in shows. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** The prompt's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary")
	FText Prompt;

	/** It can be looked at only while this holds (Main 4 from its fifth step on). Empty: any time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary")
	FStoryCondition LookWhen;

	/** Her ember burns bright and steady while this holds (Main 7's relighting on; the Main 7 lane sets it). Empty: never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary")
	FStoryCondition LitWhen;

	/** How far from the player's eyes it can be looked at (cm): a look reaches farther than a hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 350.f;

	/** The flash's length (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary", meta = (ClampMin = "0.1", Units = "s"))
	float FlashSeconds = FGraveSightFlash::DefaultSeconds;

	/** How high her ember lifts off the recess at the flare's height (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reliquary", meta = (ClampMin = "0", Units = "cm"))
	float EmberRise = 24.f;

	/** Its model's sockets: where the ember lies, and where the prompt anchors. */
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

	/** The ember where it is now: its place, facing the camera, its glow and light. */
	void PlaceEmber();

	/** The flash has ended: the missions hear. */
	void FinishLook();

	/** The player within this of it (cm). */
	bool IsPlayerNear(double Within) const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> StandIn;

	/** The ember: one camera-facing quad of the game's glow (as the cold open draws it). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> EmberQuad;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> EmberLight;

	FGraveSightFlash Flash;
	FVector EmberAt = FVector::ZeroVector;
	float EmberGlow = 0.f;
	float GutterClock = 0.f;
	bool bDrinking = false;
	int32 Looks = 0;
	bool bLitByScene = false;
	float Blaze = 0.f;
};
