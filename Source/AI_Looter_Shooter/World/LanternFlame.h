#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "LanternFlame.generated.h"

class AKeeperLanternPost;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMissionRunner;
class UStaticMesh;
struct FCampaignRecord;

/** Which way a lantern's flame leans, apart from the world (the tests check it). */
namespace LanternLean
{
	/**
	 * The flame's axis, from its wick to its tip: up, leaned LeanDegrees toward Bearing (degrees from north, +X, round
	 * through east, +Y: 45 is north-east).
	 */
	AI_LOOTER_SHOOTER_API FVector Axis(float BearingDegrees, float LeanDegrees);

	/** The turn taking a flame standing along +Z onto Axis(BearingDegrees, LeanDegrees), its foot toward the bearing. */
	AI_LOOTER_SHOOTER_API FQuat Turn(float BearingDegrees, float LeanDegrees);
}

/**
 * The Keeper's Lantern's flame leaning toward the next saint's light still burning (Docs/Story.md, Campaign gating: "The
 * Keeper's Lantern leans toward the nearest saint's light"; Docs/Areas/RansomsRest.md, Main 7 "The Lantern Leans": "The
 * flame leans north-east, over the ridges", toward Lucky Ned Purcell and the Gilded Lily). The lantern's own glass glows
 * (its LanternGlow slot) and has no flame in it, so this draws one: a few crossed glow cards (M_FX_Glow, the gunfire's
 * additive glow; one instanced draw) standing on its SOCKET_Light, the wick, leaned toward Bearing in the world whichever
 * way the lantern hangs. It shows while ShownWhen holds (after Main 6, when Abel has lit the lantern), and is gone before.
 *
 * Tools/Unreal/build_area_depot.py hangs it in the Keeper's Lantern where Main 6 leaves it lit (on the keeper's post on the
 * burial deck, AKeeperLanternPost, which tilts the lantern itself on its hook toward the same bearing), attached there so
 * it goes where the lantern goes; its turn stays its own (absolute), so the lantern's tilt never turns the flame off its
 * bearing. Hung on the keeper's post, it lights the moment Abel lights the lantern in his scene (the post's
 * OnKeepersLanternLit), and stays lit while the post's lantern is. Hob's first perch in Main 7 is by it. It never ticks: it
 * reads the story as the missions change.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALanternFlame : public AActor
{
	GENERATED_BODY()

public:
	ALanternFlame();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Shown or not, now (Abel's scene lights it as it happens; the story keeps it so afterwards). */
	void SetLit(bool bInLit);
	bool IsLit() const { return bLit; }

	/** ShownWhen holds in Campaign (with the missions running in this level). */
	bool ShouldBeLit(const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr) const;

	/** Reads the story again: lit while ShownWhen holds, or while the keeper's post it hangs on has its lantern lit. */
	void RefreshStory();

	/** Turns the flame toward Bearing again in the world (after the lantern it hangs from has turned). */
	void Lean();

	/** The way the flame points now, from wick to tip (world). */
	FVector GetFlameAxis() const;

	/** The flame's cards, standing along the component's +Z from its origin (the wick). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Flame;

	/** Where it leans (degrees from north, +X, through east, +Y): north-east, toward the Lily. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flame")
	float Bearing = 45.f;

	/** How far off upright it leans (degrees): plainly leaning, as in a draught that isn't there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flame", meta = (ClampMin = "0", ClampMax = "80"))
	float LeanDegrees = 38.f;

	/** The flame's length and width (cm): inside the lantern's globe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flame", meta = (ClampMin = "1"))
	float Length = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flame", meta = (ClampMin = "0.5"))
	float Width = 4.f;

	/** Its color and how brightly it glows (the glow material's strength). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flame")
	FLinearColor Color = FLinearColor(1.f, 0.66f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flame", meta = (ClampMin = "0"))
	float Strength = 24.f;

	/** When it burns: after Main 6, when Abel has lit the lantern. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flame|Story")
	FStoryCondition ShownWhen;

	/** As the level shows it before the story says (out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flame|Story")
	bool bLit = false;

	UPROPERTY(EditAnywhere, Category = "Flame|Art")
	TObjectPtr<UStaticMesh> CardMesh;

	UPROPERTY(EditAnywhere, Category = "Flame|Art")
	TObjectPtr<UMaterialInterface> GlowMaterial;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The cards: tongues crossed about the flame's axis and a round core at the wick. */
	void BuildCards();

	void HandleMissionsChanged();

	/** Abel has lit the lantern it hangs in (his scene): lit with it, before the story says. */
	void HandleLanternLit(AKeeperLanternPost& LitPost);

	/** The keeper's post it hangs on (its attach parent), when it is one. */
	AKeeperLanternPost* FindPost() const;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;

	TWeakObjectPtr<AKeeperLanternPost> BoundPost;
	FDelegateHandle LanternLitHandle;
};
