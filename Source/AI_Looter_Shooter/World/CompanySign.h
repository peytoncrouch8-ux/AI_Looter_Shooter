#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "CompanySign.generated.h"

class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** How a sign's words are put on. */
UENUM(BlueprintType)
enum class ECompanySignLook : uint8
{
	/** Stencilled on a black board in company white, on two posts (the checkpoint's "VANE CONSOLIDATED"). */
	Board,
	/** Chalked straight on a wall, no board (the "ASSESSED" marks on Main Street's doors). */
	Chalk,
};

/**
 * Words Vane Consolidated puts up (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md, Main 3): the checkpoint's board across the
 * town gate ("VANE CONSOLIDATED / ASSESSMENT IN PROGRESS / NO ENTRY. NO EXIT."), and the chalk on Main Street's doors
 * ("ASSESSED / 4 SOULS / V.C.": a house counted, its dead next). Every sign tells the player why the town hides: the
 * Company is counting it.
 *
 * Its words are drawn by a text render on its face (crisp at any distance, no texture to make); a Board is a stencilled
 * black board on two posts, Chalk is the words alone on whatever it's set against. Shown by its story as an AStoryProp is
 * (ShownWhen, HiddenWhen, read again whenever the missions change). Never ticks; only a board's posts collide.
 *
 * Stand-in look: engine shapes and the engine's text (the Tripo checkpoint sign, TRIPO_MODELS.md "Mains 2-3", replaces the
 * board and posts through BoardModelPath; it comes blank, so the words are still drawn on its face).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ACompanySign : public AActor
{
	GENERATED_BODY()

public:
	ACompanySign();

	/** The Tripo checkpoint board (one path): used in place of the stand-in board, posts and text once it's imported. */
	static const TCHAR* const BoardModelPath;

	/** It's there now, as its story stands. */
	bool ShouldShow() const;

	/** Shows or hides it as its story stands (the runner's changes call it; so can tests). */
	void Refresh();

	/** Lays out its board, posts and words from its settings (the editor and play both). */
	void Arrange();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Board;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PostLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PostRight;

	/** Its words, on its front (+X). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> Words;

	/** The words, line by line ("VANE CONSOLIDATED\nASSESSMENT IN PROGRESS"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign", meta = (MultiLine = true))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	ECompanySignLook Look = ECompanySignLook::Board;

	/** The letters' height (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign", meta = (ClampMin = "1", Units = "cm"))
	float LetterSize = 24.f;

	/** A board's size (cm: width, height) and how high its bottom stands (cm over the ground on its posts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	FVector2D BoardSize = FVector2D(260.0, 120.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign", meta = (ClampMin = "0", Units = "cm"))
	float BoardLift = 90.f;

	/** Chalk is slanted a little, by a hurried hand (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	float Slant = 0.f;

	/** It's there while any of these holds (none: always), and gone while any of HiddenWhen does. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	TArray<FStoryCondition> ShownWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	TArray<FStoryCondition> HiddenWhen;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleRunnerChanged();

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
};
