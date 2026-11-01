#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "CompanyBanner.generated.h"

class AController;
class UCapsuleComponent;
class UDamageType;
class UMissionRunner;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * The Company's banner across Main Street (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md, Main 3: "UNDER NEW MANAGEMENT
 * across Main Street falls into the street"): a 12 m canvas strung between two facades on two ropes, its words on both
 * faces. Each rope is a target: a shot cuts it (its own hit, a little thicker than the rope, so a near miss counts), the
 * banner's freed end drops and sags to the street; cut both and it falls flat into the dirt, words up, with a flap and
 * a puff of dust. The missions hear DownEvent as it lands (Main 3's optional "Cut down the banner"), and the story keeps
 * DownFlag, so it lies in the street from then on (until HiddenWhen: the Company's gone).
 *
 * It's up while ShownWhen holds (the Company's town: after Main 1). It ticks only while it falls. Stand-in look: an
 * engine cube in the game's canvas with the engine's text on both faces, ropes of thin cylinders; ModelPath (the Tripo
 * banner, TRIPO_MODELS.md "Mains 2-3", blank) replaces the cloth once it's imported, the words still drawn on it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ACompanyBanner : public AActor
{
	GENERATED_BODY()

public:
	ACompanyBanner();

	/** The Tripo banner (one path): its cloth, in place of the stand-in's cube (it comes blank; the words are drawn on it). */
	static const TCHAR* const ModelPath;

	/** Cuts rope 0 (its left, -Y) or 1 (its right), as a shot does: By, who cut it. False if already cut or not up. */
	bool CutRope(int32 Rope, AController* By);

	/** How many of its two ropes are cut, and whether it has fallen (both cut, or it lay in the street from the start). */
	int32 NumCut() const { return (bCut[0] ? 1 : 0) + (bCut[1] ? 1 : 0); }
	bool IsDown() const { return bDown; }

	/** Moves its fall on by DeltaSeconds (the tick does while it falls; tests call it). */
	void Advance(float DeltaSeconds);

	/** Reads its story again: up, down in the street, or gone. */
	void RefreshStory();

	/** Where its two rope ends are tied on the facades, relative to it (left: -Y, right: +Y). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banner", meta = (MakeEditWidget = true))
	FVector AnchorLeft = FVector(0.0, -850.0, 40.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banner", meta = (MakeEditWidget = true))
	FVector AnchorRight = FVector(0.0, 850.0, 40.0);

	/** How far below it the street is (cm): where it falls to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banner", meta = (ClampMin = "0", Units = "cm"))
	float StreetBelow = 500.f;

	/** Its words, on both faces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banner", meta = (MultiLine = true))
	FText Text;

	/** What the missions hear as it lands in the street, and the story flag that keeps it down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banner")
	FName DownEvent = FName(TEXT("Banner.Down"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banner")
	FName DownFlag;

	/** It's strung up (or lies cut down) while this holds; gone while any of HiddenWhen does. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banner")
	FStoryCondition ShownWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Banner")
	TArray<FStoryCondition> HiddenWhen;

	/** The cloth's size (cm: length, height). */
	static constexpr float ClothLength = 1200.f;
	static constexpr float ClothHeight = 140.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The cloth's frame (its sag and its fall move it); the cloth and its words hang on it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ClothFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Cloth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> WordsFront;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> WordsBack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RopeLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RopeRight;

	/** What shots on each rope hit (only the Weapon channel). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> RopeLeftHit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> RopeRightHit;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** The cloth, its words and its ropes where its cuts and its fall have them (Fall 0-1 through the drop to the street). */
	void Pose();

	/** Where the cloth's top corner on Side (-1 left, +1 right) is now, relative to it. */
	FVector CornerNow(float Side) const;

	void HandleRunnerChanged();

	UFUNCTION()
	void HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType,
		AActor* DamageCauser);

	bool bCut[2] = { false, false };
	/** The rope cut first (its end drops first, the cloth swinging from the other): 0, 1, or INDEX_NONE. */
	int32 FirstCut = INDEX_NONE;
	bool bDown = false;
	bool bTold = false;
	/** The sag of one end cut (0-1 toward hanging) and the fall to the street once both are (0-1). */
	float Sag = 0.f;
	float Fall = 0.f;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
};
