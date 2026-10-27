#pragma once

#include "CoreMinimal.h"
#include "Bounties/BountyTypes.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "BountyBoard.generated.h"

class AGiverMarkActor;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The bounty board in a town (Docs/Work/2026-10-22-Next/BOUNTY.md): Ransom's Rest's stands at the depot, where the yard
 * road meets the platform, its face toward the Depot roost and the road in (the user's playtest, 2026-10-27, note 12;
 * Tools/Unreal/build_area_bounties.py). A notice board (SM_NoticeBoard_Bounty, BOUNTIES on
 * its header) with a sheet on its face for each posting up (the art's decals, Tools/Blender/looter_bounty_sheets.py: WANTED BY THE
 * RIM RANGERS, the mark's kind inked in a frame, its name printed on its band; plain paper until they are imported), sealed in its mark's tier colour; a taken or won one's sheet
 * is gone (the player carries it). Over it floats its own mark: ? while a posting is up the player hasn't read here, !
 * while a won bounty waits to be paid (or an older save's strongbox waits to be handed over). A tap of Interact opens the
 * board's screen (UBountyBoardWidget), once OpenWhen holds (after Main 3: the town is safe), and hands over any
 * strongbox an older save still owes (UBountyBoardSubsystem::HandOverOwed).
 *
 * Its postings live in UBountyBoardSubsystem (the session's record, by BoardId); the board only shows them and opens the
 * screen. One board per level (the first that begins is the level's).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABountyBoard : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABountyBoard();

	/** The tag every bounty board carries (the console, the tests). */
	static const FName BoardTag;

	/** Its record's key in the session ("RansomsRest"). Never renamed: the session keeps its postings by it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FName BoardId = TEXT("RansomsRest");

	/** The screen's title and the line under it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FText Subtitle;

	/** The prompt's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FText Prompt;

	/** It posts and opens only while this holds (after Main 3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FStoryCondition OpenWhen;

	/** Dangerous bounties are drawn once this holds (after Main 5), Deadly ones once this does (after Main 6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FStoryCondition DangerousWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FStoryCondition DeadlyWhen;

	/** Where the face's middle is, in its frame (the model's SOCKET_Decal: 8 cm out, 1.36 m up), and its sheets' spacing (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Look")
	FVector FaceOffset = FVector(9.0, 0.0, 136.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Look")
	float SheetSpacing = 44.f;

	/** Where each wax seal sits on its sheet, from the sheet's middle: cm to the reader's right, cm up (the printed ring on the sheet, Tools/Blender/looter_bounty_sheets.py SEAL_AT). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Look")
	FVector2D SealOffset = FVector2D(10.6, -17.3);

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	virtual void OnConstruction(const FTransform& Transform) override;

	/** It can be used now: OpenWhen holds (the subsystem says), and the board is on (Looter.Bounty.Enabled). */
	bool IsOpen() const;

	/** Its look for the record as it stands: a sheet per posting up, sealed in its tier's colour, and its mark (? or !). */
	void ShowRecord(const FBountyBoardRecord& Record, bool bOpen);

	/**
	 * The sheet's rectangle in T_BountySheets_BC for a kind (U0, V0, U1, V1; V = 0 at the top), and a mark's printed name's in
	 * T_BountyNames_BC (kind x 8 + name): M_PosterDecal's "Cell". An unknown kind shows the Unpaid's sheet. The numbers are the
	 * art script's (Tools/Blender/looter_bounty_sheets.py logs them).
	 */
	static FVector4 SheetCell(FName Kind);
	static FVector4 NameCell(FName Kind, int32 NameIndex);

	/** The sheets are decals of the art (their assets made), else the plain paper planes. */
	bool UsesDecals() const { return SheetDecalBase != nullptr; }

	/** Whether a sheet shows now, for the tests. */
	bool IsSheetShown(int32 Slot) const;

	/** The mark shown over it: none, an offer (?) or a turn-in (!). */
	enum class EShownMark : uint8 { None, Offer, TurnIn };
	EShownMark GetShownMark() const { return ShownMark; }

	/** The board with BOUNTIES on its header, and the NOTICES one it stands in with until that is imported. */
	static const TCHAR* const ModelPath;
	static const TCHAR* const FallbackModelPath;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The board, solid (stood against, found by the Interact key's line). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Board;

	/** A sheet per slot and its wax seal. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Sheets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Seals;

	/** The sheet's paper and picture, and the mark's printed name over its band, as decals on the board's face (as AWantedPoster's). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UDecalComponent>> SheetDecals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UDecalComponent>> NameDecals;

private:
	/** The sheets and seals laid out on its face from FaceOffset and SheetSpacing (the build script may move them). */
	void LayOutSheets();
	void ShowMark(EShownMark Mark);

	UPROPERTY(Transient)
	TObjectPtr<AGiverMarkActor> MarkActor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SealMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceBase;

	/** MI_BountySheetDecal and MI_BountyNameDecal (build_bounty_sheet_materials.py), and a dynamic instance of each per slot. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SheetDecalBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> NameDecalBase;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SheetDecalMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> NameDecalMaterials;

	EShownMark ShownMark = EShownMark::None;
};
