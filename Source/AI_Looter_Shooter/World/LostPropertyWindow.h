#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "LostPropertyWindow.generated.h"

class USceneComponent;
class UTextRenderComponent;

/**
 * The depot's LOST PROPERTY window in the baggage room (the user's call, 2026-10-13; Mr. Sloane, the station agent, keeps
 * it): a tap of Interact there opens Lost Property's screen (ULostLootWidget), where the guns left lying are claimed and
 * shelving is bought (ULostLootSubsystem, LostLootRules). The shelves are the session's, so every window shows the same.
 *
 * It has no model of its own: it stands in the depot's window, whose building is the station's (SM_RR_Depot's
 * SOCKET_LostProperty: on the window's sill, +X out of the building toward whoever stands at it). Until that model is in,
 * Tools/Unreal/build_area_lostloot.py stands it on the old depot's wall beside the station board, with a painted
 * placeholder sign (bShowPlaceholderSign) so it can be found. Its +X faces the player; Interact is aimed at InteractOffset in
 * front of it, on the player's side of the glass. Tagged LostProperty (the map, missions); no collision, never ticks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALostPropertyWindow : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag every Lost Property window carries. */
	static const FName WindowTag;

	ALostPropertyWindow();

	virtual void OnConstruction(const FTransform& Transform) override;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	/** Opens Lost Property's screen for Player (their pawn or controller), as a tap of Interact does. False when it can't. */
	UFUNCTION(BlueprintCallable, Category = "Lost Property")
	bool Use(AActor* Player);

	/** Its story lets it open now (OpenWhen; always without a condition), and it isn't the main menu's backdrop. */
	bool IsOpen() const;

	/** The shutter's sound at the window as the screen opens or closes. */
	void PlayShutter(bool bOpening) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** LOST PROPERTY painted over the window, only while the depot's own model has no sign of its own (bShowPlaceholderSign). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> PlaceholderSign;

	/** The screen's title: "Lost Property". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lost Property")
	FText Title;

	/** The notice on the screen, in the station agent's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lost Property")
	FText Notice;

	/** The Interact prompt's words: "Claim lost property" (the count on the shelves follows it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lost Property")
	FText Prompt;

	/** When the window opens (empty: always, the game's choice: guns are handed in from the start). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lost Property")
	FStoryCondition OpenWhen;

	/** How far from the player's eyes it can be used (cm), measured to InteractOffset: from the counter, a step back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lost Property", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 300.f;

	/** The point looked at to use it, in its own space: a little in front of the sill, on the player's side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lost Property")
	FVector InteractOffset = FVector(30.f, 0.f, 0.f);

	/** Shows the painted placeholder sign (the build script sets it where the depot's model has no window of its own). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lost Property")
	bool bShowPlaceholderSign = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ApplySign();
};
