#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "InteractableProp.generated.h"

class AInteractableProp;
class UMaterialInterface;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInteractablePropUsed, AInteractableProp*, Prop, AActor*, User, bool, bHeld);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnInteractablePropUsedNative, AInteractableProp& /*Prop*/, AActor* /*User*/, bool /*bHeld*/);

/** What a use does to the prop's state. */
UENUM(BlueprintType)
enum class EInteractablePropUse : uint8
{
	/** Every use is the same and leaves it as it was (the bell rung, a sign read). */
	Trigger,
	/** Each use turns it on or off (a door opened and shut). */
	Toggle,
	/** A use turns it on, and it can't be used again until something turns it off (a lantern lit; a boss puts it out). */
	TurnOn,
};

/** What being on looks like. */
UENUM(BlueprintType)
enum class EInteractablePropEffect : uint8
{
	/** Nothing to see: the delegate and the mission event are all. */
	None,
	/** The mesh swings about a hinge, open while on (a door, a gate, a shutter). */
	Swing,
	/** A material slot of the mesh glows while on (a lantern's glass). */
	Glow,
};

/**
 * A static mesh the player uses with the Interact key, for tests and greybox levels: a door that opens and shuts, a bell
 * held to ring, a lantern post held to light. Its prompt, tap or hold, cooldown and what a use does are settings; tag
 * the actor (its Tags) for missions to find it ("Bell", "Lantern"). A use runs the effect, logs, and broadcasts OnUsed;
 * the player's interaction component tells the missions. Tagged Obstacle like other props (the minimap, ground cover).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AInteractableProp : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractableProp();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Uses it as the player would (a script, a scene): the change, the effect, the log and OnUsed. False when it can't be
	 * used now. Missions hear of uses through the player's interaction component, not from here.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool Use(AActor* User, bool bHeld);

	/** Turns it on or off without a use (a boss putting the lanterns out, a script opening a door); bInstant skips the swing. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetOn(bool bInOn, bool bInstant = false);

	/** On: a door open, a lantern lit. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsOn() const { return bOn; }

	/** It can be used now: enabled, past its cooldown, and not a lit TurnOn prop. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsUsable() const;

	/** Moves its swing and cooldown on by DeltaSeconds: the tick calls it while there's something to move; tests call it. */
	void Advance(float DeltaSeconds);

	/** The words a use would do now: PromptWhenOn while on (when it has them), else Prompt. */
	const FText& GetCurrentPrompt() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** What using it does, in the prompt's words: "Open the door", "Ring the bell", "Light the lantern". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText Prompt;

	/** The words while it's on, for a toggle: "Close the door". Empty: Prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText PromptWhenOn;

	/** Hold the key HoldSeconds to use it (the bell, a lantern) instead of tapping it (a door). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bHold = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.05", EditCondition = "bHold"))
	float HoldSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractablePropUse UseMode = EInteractablePropUse::Trigger;

	/** Seconds after a use before it can be used again (a door's swing, a bell still ringing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0"))
	float CooldownSeconds = 0.5f;

	/** How far from the player's eyes it can be used (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0"))
	float Reach = 0.f;

	/** It can be used at all (a mission or a boss fight may turn it off for a while). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bEnabled = true;

	/** It starts on: a door open, a lantern lit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bStartOn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Effect")
	EInteractablePropEffect Effect = EInteractablePropEffect::None;

	/** Swing: the hinge in the prop's own space (cm); the mesh turns about the upright line through it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Effect", meta = (EditCondition = "Effect == EInteractablePropEffect::Swing", EditConditionHides))
	FVector HingeOffset = FVector::ZeroVector;

	/** Swing: how far it turns open, in degrees (the sign picks the way). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Effect", meta = (EditCondition = "Effect == EInteractablePropEffect::Swing", EditConditionHides))
	float OpenAngle = 95.f;

	/** Swing: seconds to swing open, or shut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Effect", meta = (ClampMin = "0", EditCondition = "Effect == EInteractablePropEffect::Swing", EditConditionHides))
	float SwingSeconds = 0.4f;

	/** Glow: the mesh's material slot that glows, by name (none: the first). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Effect", meta = (EditCondition = "Effect == EInteractablePropEffect::Glow", EditConditionHides))
	FName GlowSlot;

	/** Glow: the slot's material while on (the lantern lit). Empty: the mesh's own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Effect", meta = (EditCondition = "Effect == EInteractablePropEffect::Glow", EditConditionHides))
	TObjectPtr<UMaterialInterface> GlowMaterial;

	/** Glow: the slot's material while off (dark glass). Empty: the mesh's own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Effect", meta = (EditCondition = "Effect == EInteractablePropEffect::Glow", EditConditionHides))
	TObjectPtr<UMaterialInterface> DarkMaterial;

	/** It was used (after the change): by whom, and whether the key was held. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractablePropUsed OnUsed;

	/** The same, for C++ listeners. */
	FOnInteractablePropUsedNative OnUsedNative;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Remembers the mesh as placed (shut, its glow slot's own material) the first time it's needed. */
	void CapturePlacedLook();

	/** Puts the mesh where its swing has got to. */
	void ApplySwing();

	/** Gives the glow slot its material for on or off. */
	void ApplyGlow();

	int32 GetGlowIndex() const;

	/** Ticks only while a swing or a cooldown is under way. */
	void RefreshTick();

	/** The glow slot's material as placed, to go back to. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PlacedGlowMaterial;

	/** The mesh's place and turn as placed (shut). */
	FTransform PlacedTransform;
	bool bPlacedLookCaptured = false;

	bool bOn = false;
	/** How far open the swing is: 0 shut, 1 open. */
	float SwingAlpha = 0.f;
	float CooldownLeft = 0.f;
};
