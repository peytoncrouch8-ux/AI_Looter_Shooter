#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryLine.h"
#include "WantedPoster.generated.h"

class UBoxComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** What a paper nailed up on Ransom's Rest is. */
UENUM(BlueprintType)
enum class EWantedPosterVariant : uint8
{
	/** Ellis's wanted poster: holding Interact tears it down, leaving its corners under the nails. */
	Wanted,
	/** Ranger Calder's note on the Rim Rangers' board: never torn; a tap reads it, its words as captions. */
	CalderNote,
};

/** Lines said as the level's TornCount-th wanted poster comes down (Hob's remarks), after whatever is being said. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWantedPosterRemark
{
	GENERATED_BODY()

	/** Said when this many of the level's wanted posters are down, this one included. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Remark", meta = (ClampMin = "1"))
	int32 TornCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Remark")
	TArray<FStoryLine> Lines;
};

/**
 * Where the pictures are in the posters' atlas, T_Posters_BC (the art session's, Tools/Blender/looter_posters.py): each
 * a UV rectangle (U0, V0, U1, V1; V = 0 at the top), the "Cell" the decal material shows. The atlas is drawn at 1024 px
 * per metre, so a cell's size on a wall is its UV size times Centimeters. If the art's layout ever changes, set it in
 * Config/DefaultGame.ini under [/Script/AI_Looter_Shooter.WantedPoster] (Atlas=(...)): no code change.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FWantedPosterAtlas
{
	GENERATED_BODY()

	/** The whole atlas's side as shown on a wall (cm). */
	UPROPERTY(EditAnywhere, Category = "Atlas", meta = (ClampMin = "1", Units = "cm"))
	float Centimeters = 100.f;

	/** Ellis's wanted poster: 0.5 x 0.703 m, the sheet inside it 0.45 x 0.68 m, nailed 15 mm in from its corners. */
	UPROPERTY(EditAnywhere, Category = "Atlas")
	FVector4 Poster = FVector4(0.0, 0.0, 0.5, 0.703125);

	/** What a tear leaves, the corners under the nails: the poster's size and spot, so it swaps in place. */
	UPROPERTY(EditAnywhere, Category = "Atlas")
	FVector4 Remnant = FVector4(0.5, 0.0, 1.0, 0.703125);

	/** Calder's note, on one nail: 0.25 x 0.297 m. */
	UPROPERTY(EditAnywhere, Category = "Atlas")
	FVector4 Note = FVector4(0.0, 0.703125, 0.25, 1.0);

	/** The scrap that falls: a torn piece of about 0.19 x 0.11 m in a 0.25 x 0.1875 m cell. */
	UPROPERTY(EditAnywhere, Category = "Atlas")
	FVector4 Scrap = FVector4(0.25, 0.703125, 0.5, 0.890625);

	/** The same piece from behind, mirrored, for the scrap card's back faces. */
	UPROPERTY(EditAnywhere, Category = "Atlas")
	FVector4 ScrapBack = FVector4(0.5, 0.703125, 0.75, 0.890625);

	/** A cell's size on a wall (cm): its width and its height. */
	FVector2D SizeOf(const FVector4& Cell) const;
};

/**
 * A paper nailed up on Ransom's Rest (Side 1, "Wanted: Already Dead", Docs/Areas/RansomsRest.md): Ellis's wanted poster,
 * or Ranger Calder's note on the Rim Rangers' board. It's a decal of MI_PosterDecal (M_PosterDecal, made by
 * Tools/Unreal/build_decal_materials.py) showing its cell of the posters' atlas, and a thin box the Interact key's line
 * finds: the box blocks that line only (no player, bullet or camera), and the minimap's bake, which traces world-static
 * things, never sees it.
 *
 * Place it with its front (+X) out of a flat surface and its origin on that surface (a plank wall, a door, a board:
 * projected paper stretches over round logs). The decal reaches 2 cm into the surface, so the back of a thin board never
 * shows the paper, and 8 cm out of it, over clapboard laps and stone.
 *
 * A wanted poster comes down with a short hold of Interact (TearHoldSeconds): the decal swaps in place to the corners
 * left under its nails, a scrap of the paper tumbles down and fades, and Hob remarks on some of them (TearRemarks). It
 * stays down: the session keeps the torn ones by name with the map's world (FSavedMapWorld::TornPosters), and Side 1's
 * lasting objective (UMissionLastingInteractObjective) counts the torn posters it finds, so a reload loses nothing.
 * Calder's note is never torn; a tap reads it (NoteLines as captions).
 *
 * Tagged for missions by its variant (WantedTag or NoteTag), and Obstacle like other props.
 */
UCLASS(Config = Game)
class AI_LOOTER_SHOOTER_API AWantedPoster : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tags missions find them by: every wanted poster carries the first, Calder's note the second. */
	static const FName WantedTag;
	static const FName NoteTag;

	AWantedPoster();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual bool IsUsedUp() const override { return bTorn; }

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Tearing it down (WantedPosterTear.cpp) ---

	/**
	 * Tears it down as a held Interact does: the remnant in its place, the falling scrap, Hob's remark for this many down,
	 * and a save soon (it's progress). False when it can't be (Calder's note, or down already). Missions hear of it from the
	 * player's interaction component, not from here.
	 */
	UFUNCTION(BlueprintCallable, Category = "Poster")
	bool Tear(AActor* ByWhom);

	/** Down as a session keeps it (USessionSubsystem::RestoreWorld): the remnant at once, with no scrap, remark or save. */
	void RestoreTorn();

	/** How many of the level's wanted posters are down. */
	static int32 CountTorn(const UWorld* World);

	/** The scrap while it falls, else null. */
	UStaticMeshComponent* GetScrap() const { return Scrap; }

	/** How solid the falling scrap is now: 1 as it comes off, 0 gone (and 0 when none falls). */
	float GetScrapOpacity() const;

	/** Moves the falling scrap on by DeltaSeconds: the tick does while it falls; tests call it. */
	void Advance(float DeltaSeconds);

	// --- Reading Calder's note ---

	/** Reads Calder's note as a tap does: its lines as captions, cutting off whatever was being said. False when it can't be read now. */
	UFUNCTION(BlueprintCallable, Category = "Poster")
	bool Read(AActor* Reader);

	// --- What it is now ---

	UFUNCTION(BlueprintPure, Category = "Poster")
	bool IsTorn() const { return bTorn; }

	/** A wanted poster still up. */
	UFUNCTION(BlueprintPure, Category = "Poster")
	bool CanTear() const;

	/** Calder's note, its last reading over. */
	UFUNCTION(BlueprintPure, Category = "Poster")
	bool CanRead() const;

	/** Its lines are on screen or still to come. */
	bool IsBeingRead() const;

	EWantedPosterVariant GetVariant() const { return Variant; }

	/** Makes it a wanted poster or Calder's note (a script, a test): its tag, look and box follow. */
	void SetVariant(EWantedPosterVariant NewVariant);

	/** The atlas cell it shows whole: the poster, or the note. Its box and its place on the wall follow this one. */
	FVector4 GetWholeCell() const;

	/** The atlas cell it shows now: the poster or its remnant, or the note. */
	FVector4 GetShownCell() const;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDecalComponent> Decal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Hitbox;

	// --- Settings ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster")
	EWantedPosterVariant Variant = EWantedPosterVariant::Wanted;

	/** Its size against the atlas's (1: as drawn, 0.5 x 0.703 m for the poster). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster", meta = (ClampMin = "0.25", ClampMax = "2"))
	float Scale = 1.f;

	/** How long Interact is held to tear a poster down: short, but deliberate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster", meta = (ClampMin = "0.1", Units = "s"))
	float TearHoldSeconds = 0.6f;

	/** The prompt's words: "Tear down the poster", "Read the note". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster")
	FText TearPrompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster")
	FText ReadPrompt;

	/** How far from the player's eyes it can be used (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 0.f;

	/** What reading Calder's note says: her words as the art writes them on it, then Hob's. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster|Lines")
	TArray<FStoryLine> NoteLines;

	/** Hob's remarks as the level's posters come down (the first, the third, the sixth). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster|Lines")
	TArray<FWantedPosterRemark> TearRemarks;

	/** How long the torn-off scrap tumbles and fades before it's gone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster|Scrap", meta = (ClampMin = "0.1", Units = "s"))
	float ScrapSeconds = 1.5f;

	/** The decal's material: MI_PosterDecal. Unset: that instance, once build_decal_materials.py has made it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster|Look")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	/** The falling scrap's: MI_PosterScrap, which takes its cells and fade from the card's custom primitive data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster|Look")
	TObjectPtr<UMaterialInterface> ScrapMaterial;

	/** The scrap's card: the engine's plane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poster|Look")
	TObjectPtr<UStaticMesh> ScrapMesh;

	/** The atlas's cells, from the art's layout. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Poster|Look")
	FWantedPosterAtlas Atlas;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Tags it for missions by its variant, and as an obstacle. */
	void RefreshTags();

	/** Puts the decal (its cell, size and spot) and the box as they should be now. */
	void RefreshLook();

	/** The decal's own instance of DecalMaterial (or MI_PosterDecal), made once and kept. Null without the material. */
	UMaterialInstanceDynamic* FindOrMakeDecalMaterial();

	/** The scrap's card and material: the settings, else the engine plane and MI_PosterScrap once they exist. */
	UStaticMesh* GetScrapMesh() const;
	UMaterialInterface* GetScrapMaterial() const;

	// WantedPosterTear.cpp
	void SayRemark(int32 TornNow);
	void DropScrap();
	void PoseScrap();
	void EndScrap();

	/** The scrap tearing off, while it falls (made in code: no mesh asset of its own). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Scrap;

	bool bTorn = false;

	/** The falling scrap: how long it has fallen, where it came off, and how it was thrown (drift, flutter, tumble). */
	float ScrapAge = 0.f;
	FVector ScrapStart = FVector::ZeroVector;
	float ScrapDrift = 0.f;
	float ScrapFlutterPhase = 0.f;
	FVector ScrapSpinAxis = FVector::RightVector;
	float ScrapSpinRate = 0.f;

	/** The captions' conversation of the last reading. */
	int32 Conversation = 0;
};
