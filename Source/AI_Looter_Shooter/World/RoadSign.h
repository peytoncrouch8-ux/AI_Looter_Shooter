#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoadSign.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * One face of a signpost's board as ARoadSign lays a name on it, in the actor's space: where the name's middle goes, the
 * way the face looks out (its yaw, degrees) and the room along the board for the name (cm).
 */
struct FRoadSignFace
{
	FVector Middle = FVector::ZeroVector;
	float FacingYaw = 0.f;
	float Room = 0.f;
};

/**
 * A signpost at a road's fork with the places it leads to painted on its two arrow boards (the user's note 16,
 * 2026-10-27: roads should pull the player). The post is VillageProps.py's Signpost (set by
 * Tools/Unreal/build_area_dressing.py, which stands one where the layout's roadside.signs say, its upper board pointing
 * down the branch); the names are text on both faces of each board, cream paint, drawn only near (TextDrawDistance).
 *
 * The boards' frame comes from the model (VillageProps.py signpost(): each arrow board 16 cm tall, its tail at the post,
 * its front face 9 cm out from the post's middle and its back 6 cm, the upper one 1.98 m up pointing 78 degrees right of
 * the actor's front, the lower 1.72 m up pointing 112 degrees left of it, each drooping its own small tilt toward its
 * tip): the text is laid on those faces in OnConstruction, so it moves with the actor.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ARoadSign : public AActor
{
	GENERATED_BODY()

public:
	ARoadSign();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** The signpost's mesh (SM_Signpost). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Post;

	/** The place the upper (teal) board points to, e.g. "THE SINK". Empty: that board stays blank. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	FText UpperName;

	/** The place the lower (red) board points to, the way back, e.g. "MAIN STREET". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	FText LowerName;

	/** The letters' height (cm): a board is 16 cm tall. A long name is squeezed to fit its board, never taller. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign", meta = (ClampMin = "2", ClampMax = "12"))
	float LetterHeight = 6.5f;

	/** How far (cm) the names are drawn: past about 30 m a 6.5 cm letter is a few pixels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign", meta = (ClampMin = "0"))
	float TextDrawDistance = 3500.f;

	/** The paint the names are in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
	FColor PaintColor = FColor(236, 226, 198);

	/** A board's face (Board 0 the upper, 1 the lower; bFront its front, else its back) as the name is laid on it. */
	static FRoadSignFace Face(int32 Board, bool bFront);

	/** The way a board's arrow points, in the actor's frame (yaw, degrees): build_area_dressing.py turns the post by it. */
	static float BoardYaw(int32 Board);

private:
	/** Board 0 front, 0 back, 1 front, 1 back. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UTextRenderComponent>> Names;

	void Lay(UTextRenderComponent* Text, int32 Board, bool bFront, const FText& Name) const;
};
