#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteriorVisibility.generated.h"

class UBoxComponent;

/** Where the player's view is, for an interior. */
UENUM(BlueprintType)
enum class EInteriorView : uint8
{
	/** Outside: the interior is hidden (nothing of it can be seen through the rock). */
	Outside,
	/** At the mouth: both show, so the way in looks in and the way out looks out. */
	Portal,
	/** Inside: the heavy outside is hidden (nothing of it can be seen through the rock). */
	Inside,
};

/**
 * Keeps an interior and the world outside it from drawing each other (Docs/Areas/RansomsRest_Expansion.md, section 6
 * "Performance": the Providence's rooms are hidden while the player is outside, and the outside while inside, at the
 * portal; Tilly's cellar may use one too). Two boxes say where: Interior (the rooms; more boxes in AdditionalInside for a
 * dog-legged mine) and Portal (the mouth, where both show). Tags say what: actors tagged InsideTag are the interior's,
 * actors tagged OutsideTag the outside's heavy pieces (the Bench's pines, the Diggings, the far valley's big scenery).
 *
 * It looks at the player's view (the camera, so a scene's view counts) a few times a second (a timer, no tick), with a
 * margin so walking along a box's face doesn't flicker. It hides only what it found shown and shows only what it hid, so a
 * piece the story hides stays hidden. Creatures spawned inside aren't tagged: rock already hides them from outside.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AInteriorVisibility : public AActor
{
	GENERATED_BODY()

public:
	AInteriorVisibility();

	/** Where Point is: the portal first (both show), then the interior (grown by Margin if the view is already Inside, so
	 *  stepping back to the face doesn't flip it), else outside. */
	EInteriorView Classify(const FVector& Point) const;

	/** Shows and hides as View asks: the interior hidden outside, the outside hidden inside, both shown at the portal. */
	void ApplyView(EInteriorView NewView);

	/** Classifies ViewPoint and applies it (the poll; tests). */
	void UpdateFor(const FVector& ViewPoint);

	EInteriorView GetView() const { return View; }

	/** Finds the tagged actors again (a rebuilt or streamed piece). The poll does it whenever the view changes. */
	void GatherActors();

	/** How many actors it is hiding now. */
	int32 CountHidden() const;

	/** The rooms (its extent is the box's half size). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Interior;

	/** The mouth: both show while the view is in it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Portal;

	/** More rooms (boxes in the actor's own space) for an interior one box can't hold. */
	UPROPERTY(EditAnywhere, Category = "Interior")
	TArray<FBox> AdditionalInside;

	/** The interior's actors (hidden outside), and the outside's heavy ones (hidden inside). */
	UPROPERTY(EditAnywhere, Category = "Interior")
	FName InsideTag = TEXT("Interior_Providence");

	UPROPERTY(EditAnywhere, Category = "Interior")
	FName OutsideTag = TEXT("HeavyOutside_Providence");

	UPROPERTY(EditAnywhere, Category = "Interior")
	bool bHideInsideWhenOutside = true;

	UPROPERTY(EditAnywhere, Category = "Interior")
	bool bHideOutsideWhenInside = true;

	/** How far past a box's face the view keeps its place (cm). */
	UPROPERTY(EditAnywhere, Category = "Interior", meta = (ClampMin = "0"))
	float Margin = 150.f;

	/** How often it looks at the view (s). */
	UPROPERTY(EditAnywhere, Category = "Interior", meta = (ClampMin = "0.05"))
	float PollSeconds = 0.2f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The player's view now (their camera), or unset when there's no player. */
	TOptional<FVector> FindViewPoint() const;

	void Poll();

	/** Point is in Box, grown by Grow (cm; negative shrinks). */
	static bool IsIn(const UBoxComponent* Box, const FVector& Point, float Grow);

	/** Point is in one of AdditionalInside, grown by Grow. */
	bool IsInAdditional(const FVector& Point, float Grow) const;

	EInteriorView View = EInteriorView::Portal;
	bool bApplied = false;
	TArray<TWeakObjectPtr<AActor>> InsideActors;
	TArray<TWeakObjectPtr<AActor>> OutsideActors;
	/** What it hid itself, to show again (and nothing else). */
	TArray<TWeakObjectPtr<AActor>> HiddenByMe;
	FTimerHandle PollTimer;
};
