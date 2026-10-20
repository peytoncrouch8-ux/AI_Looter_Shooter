#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HeroDefinition.generated.h"

class UEmberDefinition;
class UIKRetargeter;
class USkeletalMesh;
class UTexture2D;

/**
 * A playable hero as data (`/Game/Data/Embers/DA_Hero_<Id>`, written by Tools/Unreal/create_ember_assets.py): the
 * words the hero select shows (name, role, story), their colour, their painted portrait, their body (the skinned mesh
 * worn over the mannequin and the retargeter that copies the mannequin's pose onto it) and the three ember powers they
 * own from the start (Docs/Story.md, "The heroes": one equipped at a time).
 *
 * Before a new game the player picks one of five (UHeroSelectWidget, Borderlands' way); the session keeps the pick
 * (ULooterSessionSave::HeroId) and the character takes its body (UBodyFigureComponent) and powers (UEmberComponent).
 * A hero is playable once their mesh and retargeter are in the game and every power has its code (IsPlayable); until
 * then the roster shows them "In the works". Ids never change once a session has saved one, like part keys.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UHeroDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** "Ellis", "Odessa", "Crane", "Gauge", "Pike". */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FName Id;

	/** "Ellis Ransom", "Odessa Lark", "Hollis Crane", "Gauge", "Wendell Pike". */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FText DisplayName;

	/**
	 * What a caption calls them: "Ellis", "Odessa", "Crane", "Gauge", "Pike", the speaker of every line the player
	 * character says (Story/HeroLines' {Hero}). Empty: the Id (GetShortName), the same five words today.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FText ShortName;

	/** "the Revenant", "the Cardsharp", "the Unpaid", "the Iron Hand", "the Surveyor". */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FText Role;

	/** Who they were and why they hunt the gang, in two or three sentences (Ellis is "they" in all text). */
	UPROPERTY(EditDefaultsOnly, Category = "Hero", meta = (MultiLine = true))
	FText Story;

	/** The hero's own colour (the roster card's strip, the glow behind the portrait): data, like an ember's. */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	FLinearColor Color = FLinearColor::White;

	/**
	 * The painted bust the user picked (`/Game/UI/Portraits/T_Portrait_<Id>`, a UI texture with its frame painted in).
	 * These three are set by create_ember_assets.py once their asset exists (a soft pointer to a package not in the game
	 * fails the editor's asset validation); while one is empty the naming rule finds it (GetPortraitPath and the rest),
	 * so a hero lights up as soon as Main lands their model.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Hero|Looks")
	TSoftObjectPtr<UTexture2D> Portrait;

	/** The body worn over the mannequin (`/Game/Art/Characters/SK_<Id>`). */
	UPROPERTY(EditDefaultsOnly, Category = "Hero|Looks")
	TSoftObjectPtr<USkeletalMesh> FigureMesh;

	/** The mannequin's pose onto that body (`/Game/Player/Retarget/RTG_MannyTo<Id>`). */
	UPROPERTY(EditDefaultsOnly, Category = "Hero|Looks")
	TSoftObjectPtr<UIKRetargeter> Retargeter;

	/**
	 * Their arms on the gun in first person (`/Game/Art/Characters/SK_FP_Arms_<Id>`; Ellis's is SK_FP_Arms). Every hero's
	 * are made on SK_FP_Arms' skeleton bone for bone (Tools/Blender/fp_arms_heroes.py), so the hands code binds any of
	 * them with the same grips. Set by create_ember_assets.py once imported; while empty the naming rule finds them, and
	 * a hero whose arms aren't in the game wears Ellis's (UFirstPersonHandsComponent).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Hero|Looks")
	TSoftObjectPtr<USkeletalMesh> FirstPersonArms;

	/**
	 * Those arms' size against the guns (UFirstPersonHandsComponent::ArmsScale, Ellis's 0.88): a slight hero's smaller
	 * (Odessa 0.84, Pike 0.86). 0 keeps the component's. Every grip spot stays where it is at any size.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Hero|Looks", meta = (ClampMin = "0", ClampMax = "1.3"))
	float FirstPersonArmsScale = 0.f;

	/**
	 * Where this body holds a gun in third person against where the mannequin holds it (cm on the full-size body, in the
	 * aim's frame: X along the aim, Y to the right, Z up; zero for Ellis, built to the mannequin's size). The guns hang
	 * from the hidden mannequin's hand, posed for its shoulders and eyes, and every hero's hands are pinned to its hands:
	 * tall Crane held them at his chest, Pike had the sight above his eyes. The stance layer moves both hands by this
	 * (LooterStancePoseDetail::ApplyHoldOffset), so the gun sits at the hero's own shoulder and eye line. Set by
	 * create_ember_assets.py (THIRD_PERSON_HOLD), measured in play (Docs/Pipeline.md "The hero pick").
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Hero|Looks")
	FVector ThirdPersonHold = FVector::ZeroVector;

	/** The hero's three, in the order the powers page lists them; the first is equipped in a new game. */
	UPROPERTY(EditDefaultsOnly, Category = "Hero")
	TArray<TObjectPtr<UEmberDefinition>> Powers;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("Hero"), GetFName()); }

	// --- Playable or in the works (HeroDefinition.cpp) ---

	/** The mesh and the retargeter load and the hero owns powers, every one with its code (a power class). */
	bool IsPlayable() const;

	/** Playable, or the developers' Looter.Hero.Unlock 1 lets a hero in the works be picked (Ellis's body, no powers). */
	bool CanBePicked() const;

	/** Where each is: the field's path, or the naming rule's while the field is empty. */
	FSoftObjectPath GetPortraitPath() const;
	FSoftObjectPath GetFigureMeshPath() const;
	FSoftObjectPath GetRetargeterPath() const;
	/** FirstPersonArms' path, or the naming rule's (Ellis: SK_FP_Arms; the rest SK_FP_Arms_<Id>). */
	FSoftObjectPath GetFirstPersonArmsPath() const;

	/** The body and its retargeter, loaded when they're in the game; null while they aren't (no load warning then). */
	USkeletalMesh* LoadFigureMesh() const;
	UIKRetargeter* LoadRetargeter() const;
	/** The first-person arms, or null while they aren't in the game (no load warning then). */
	USkeletalMesh* LoadFirstPersonArms() const;
	/** The portrait texture, or null while it isn't imported. */
	UTexture2D* LoadPortrait() const;

	/** Every power has its code (a non-abstract power class), and there is at least one. */
	bool HasAllPowerCode() const;

	/** ShortName, or the Id while it's empty. */
	FText GetShortName() const;

	// --- The roster ---

	/** Ellis: the hero of a save from before the pick, and of a play with no session unless Looter.Hero says otherwise. */
	static FName DefaultId();

	/** The five as the roster shows them, left to right: Ellis, Odessa, Crane, Gauge, Pike. */
	static const TArray<FName>& RosterIds();

	/** "/Game/Data/Embers/DA_Hero_<Id>.DA_Hero_<Id>". */
	static FString AssetPath(FName Id);

	/** The hero's definition, or null when there is none (no asset by that id: run create_ember_assets.py). */
	static UHeroDefinition* Load(FName Id);

	/** Looter.Hero.Unlock is on (never in a shipping build): heroes in the works can be picked too. */
	static bool AreLockedHeroesUnlocked();

	/**
	 * Points Portrait, FigureMesh and Retargeter at these object paths (an empty one leaves the field empty). For
	 * create_ember_assets.py: Python sets a soft pointer only from a loaded object.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hero|Editor")
	void SetLookPaths(const FString& PortraitPath, const FString& FigureMeshPath, const FString& RetargeterPath);

	/** Points FirstPersonArms at this object path (empty: the naming rule) and sets their size. For create_ember_assets.py. */
	UFUNCTION(BlueprintCallable, Category = "Hero|Editor")
	void SetFirstPersonArms(const FString& ArmsPath, float ArmsScale);
};
