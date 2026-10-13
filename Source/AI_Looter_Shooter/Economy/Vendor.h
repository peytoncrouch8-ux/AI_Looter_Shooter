#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "Vendor.generated.h"

class USceneComponent;

/**
 * A vendor (Docs/Polish/BorderlandsComparison.md, item 13; Docs/Progression.md, "Economy"): a tap of Interact at its
 * spot opens the vendor's screen (ALooterHUD::OpenVendor, UVendorWidget), where the player buys ammo by the box, grave
 * salt by the jar and the gun of the day, and sells guns from the backpack (VendorTrade).
 *
 * It has no model of its own: it stands at a shop's door, whose building is the dressing's. Placed by
 * Tools/Unreal/build_area_vendors.py from the built level:
 *  - Ransom's Rest: Pruitt's General Store on Main Street, the Pruitts trading through the hatch in their door, open once
 *    Main Street is safe (after Main 3, Cold Welcome: OpenWhen);
 *  - Skyreach: the practice store front on Crossroads Town's square, an honesty box on the counter, always open.
 *
 * Until its story allows (OpenWhen) it can't be used at all. Tagged Vendor (missions and the map can find it); no
 * collision, never ticks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AVendor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag every vendor carries. */
	static const FName VendorTag;

	AVendor();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	/**
	 * Opens the vendor's screen for Player (their pawn or controller), as a tap of Interact does. False when it's closed by
	 * the story, or without a local player's HUD to show it (or with the pause menu up).
	 */
	UFUNCTION(BlueprintCallable, Category = "Vendor")
	bool Use(AActor* Player);

	/** Its story lets it trade now (OpenWhen, read from the campaign record; always without a condition). */
	bool IsOpen() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The screen's title: "Pruitt's General Store". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor")
	FText Title;

	/** A line under the title, in the shop's own voice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor")
	FText Greeting;

	/** The Interact prompt's words: "Trade at Pruitt's". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor")
	FText Prompt;

	/** When it trades (Pruitt's: after Main 3); empty: always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor")
	FStoryCondition OpenWhen;

	/** How far from the player's eyes it can be used (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
