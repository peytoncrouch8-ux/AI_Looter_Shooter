#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ColdOpenSet.generated.h"

class UArrowComponent;
class UMaterialInterface;
class USkeletalMesh;
class USoundBase;
class UStaticMesh;
class UAnimationAsset;

/**
 * Where the cold open plays (Docs/Story.md: Cold open; Docs/Areas/RansomsRest.md: Main 1 and Scope cuts, item 3): its
 * marks and its looks, as data placed in the level, with nothing in Sequencer. The level's first area has one, standing
 * where the lookout on Ransom's Point stands and facing as it faces (Tools/Unreal/build_area_story.py places it), so the
 * marks on the Point follow the lookout wherever it's built: +X toward the sunset over the deck's front rail, +Y toward
 * the deck's stair landing, the deck's boards 6.5 m up. The gang's skiff flies in the world's own frame, from the evening
 * cloud in the west down Gravewind Canyon toward the Mooring Ledge.
 *
 * The cold open finds the set as it plays (UColdOpenSubsystem): a level without one has no cold open. Move a mark here
 * to tune a shot; the defaults are the first pass, for the user to judge.
 */
UCLASS(hidecategories = (Input, Collision, Replication, HLOD, Physics, Networking))
class AI_LOOTER_SHOOTER_API AColdOpenSet : public AActor
{
	GENERATED_BODY()

public:
	AColdOpenSet();

	/** The set in World, or null (only the story's first area has one). */
	static AColdOpenSet* Find(const UWorld* World);

	/** A mark on the Point, in the world. */
	FVector ToWorld(const FVector& Local) const;
	FTransform ToWorld(const FTransform& Local) const;

	// --- The gang's skiff, seven days ago ---

	/**
	 * The skiff's course in the world (cm, where its keel goes): out of the evening cloud over the plains, down past the
	 * canyon's far wall, over the river and up toward the Mooring Notch under Ransom's Point. A smooth curve through them.
	 */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Skiff")
	TArray<FVector> SkiffCourse;

	/** Seconds the skiff takes over its course, the white's reveal and both lines included. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Skiff", meta = (ClampMin = "8"))
	float SkiffSeconds = 26.f;

	/** The gang on the skiff's deck, in the skiff's own frame (feet, facing), the Deacon first: he stands beside Ellis. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Skiff")
	TArray<FTransform> SkiffCrew;

	/** Ellis's eyes over the skiff's Deck socket (cm), where the player stood on the packet skiff's deck. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Skiff")
	float EyeHeight = 162.f;

	/** How far ahead of the course's start the evening cloud's front stands (cm): the skiff glides out of it as the white thins. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Skiff", meta = (ClampMin = "0"))
	float CloudFrontAhead = 2500.f;

	/** The evening cloud's light, warm and dim beside the day's white. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Skiff")
	FLinearColor EveningCloudTint = FLinearColor(0.62f, 0.42f, 0.36f);

	// --- Dusk on Ransom's Point (in the set's frame) ---

	/** Where Ellis's eyes are, facing the Deacon over the ember. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Point")
	FVector EllisView = FVector(110.0, -70.0, 810.0);

	/** Where Ellis steps to at the stair landing's rail when Abel calls, to see him on the bluff path. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Point")
	FVector EllisRail = FVector(175.0, 190.0, 810.0);

	/** Where Ellis's eyes end, shot and fallen on the boards, the sky turned over. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Point")
	FVector EllisFallen = FVector(150.0, 165.0, 678.0);

	/** The gang on the deck (feet, facing): the Deacon first, Lucky Ned second, then the other five. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Point")
	TArray<FTransform> Gang;

	/** Abel's run up the bluff path toward the lookout (feet): where Ellis first sees him, and where Ned's shot finds him. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Point")
	FVector AbelFrom = FVector(-2600.0, 1300.0, -10.0);

	UPROPERTY(EditAnywhere, Category = "Cold Open|Point")
	FVector AbelTo = FVector(-950.0, 650.0, -10.0);

	/**
	 * Where Sexton sits "far along the railing", used when the level's lookout has no Sit socket to seat him on (his
	 * model's pivot is the seat point, its front into the deck).
	 */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Point")
	FTransform SextonSeat;

	// --- Looks: nothing new, all existing models drawn black, or cheap stand-ins ---

	UPROPERTY(EditAnywhere, Category = "Cold Open|Look")
	TSoftObjectPtr<UStaticMesh> SkiffMesh;

	/** The gang and Abel: the UE mannequin, posed by code, drawn flat black. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Look")
	TSoftObjectPtr<USkeletalMesh> FigureMesh;

	/** Abel's run up the bluff path. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Look")
	TSoftObjectPtr<UAnimationAsset> RunAnimation;

	/** Mister Sexton's seated model and his ledger, drawn black; a stand-in of plain shapes until they're imported. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Look")
	TSoftObjectPtr<UStaticMesh> SextonMesh;

	UPROPERTY(EditAnywhere, Category = "Cold Open|Look")
	TSoftObjectPtr<UStaticMesh> SextonLedgerMesh;

	/** The unlit master the silhouettes are drawn with (M_Backdrop: Tint times Brightness, unlit). */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Look")
	TSoftObjectPtr<UMaterialInterface> SilhouetteMaterial;

	/** How black the silhouettes are: a warm near-black, so they read as figures against the sunset, not holes. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Look")
	FLinearColor SilhouetteColor = FLinearColor(0.008f, 0.0065f, 0.006f);

	// --- Sound (none made yet: each plays when set) ---

	/** The chapel bell clanging across the Rest at dusk. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Sound")
	TObjectPtr<USoundBase> BellSound;

	/** Lucky Ned's shot and the Deacon's. */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Sound")
	TObjectPtr<USoundBase> ShotSound;

	/** Abel's call up the bluff path ("El!"). */
	UPROPERTY(EditAnywhere, Category = "Cold Open|Sound")
	TObjectPtr<USoundBase> CallSound;

	/** The gang's figures on the Point: the Deacon, Lucky Ned, and five more. */
	static constexpr int32 DeaconIndex = 0;
	static constexpr int32 NedIndex = 1;
	static constexpr int32 GangSize = 7;

private:
#if WITH_EDITORONLY_DATA
	/** Toward the sunset, as the lookout faces (the editor shows it; the game never does). */
	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
