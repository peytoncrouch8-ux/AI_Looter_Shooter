#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BestiaryEntry.generated.h"

class AActor;
class UAnimationAsset;
class UMaterialInterface;
class USkeletalMesh;

/** The bestiary's sections, in the order it lists them. */
UENUM(BlueprintType)
enum class EBestiaryCategory : uint8
{
	/** Wildlife and monsters: spiders and whatever else lives out there. */
	Creature,
	/** Hostile people: bandits, raiders, their bosses. */
	Enemy,
	/** People and things who aren't against you: traders, quest givers, training dummies. */
	NPC,
	/** Allies who fight or travel with you. */
	Friend,
};

/** What the bestiary shows about an entry, read from its actor class's defaults so it never drifts from the game. */
struct FBestiaryStats
{
	int32 Level = 0;
	float Health = 0.f;
	/** Average damage of one attack; 0 when it doesn't fight. */
	float AttackDamage = 0.f;
	int32 XPReward = 0;
	/** It has stats at all (a creature or something with health), as opposed to a story character. */
	bool bHasHealth = false;
	bool bAttacks = false;
};

/**
 * One page of the bestiary (the inventory's second tab): a creature, enemy, NPC or friend. Make one per kind of
 * character in /Game/Data/Bestiary (right-click > Miscellaneous > Data Asset > Bestiary Entry, or duplicate one); the
 * bestiary finds them all by itself. Level, health, attack and experience come from ActorClass, and kills of it are
 * counted, so only the words and the picture are written here.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UBestiaryEntry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	EBestiaryCategory Category = EBestiaryCategory::Creature;

	/** What it is, in a few words: "Arachnid", "Training target". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	FText Kind;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary", meta = (MultiLine = true))
	FText Description;

	/** Where it's found: "Tutorial Island: the forest". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	FText Habitat;

	/** Short field notes, one line each: weak spots, habits, how to fight it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	TArray<FText> Notes;

	/** The actor in the world (a creature class, a Blueprint): its stats are read from it and kills of it are counted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	TSoftClassPtr<AActor> ActorClass;

	/** The model on the bestiary's stand. Empty: the actor class's own skeletal mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	TSoftObjectPtr<USkeletalMesh> PreviewMesh;

	/** Loops on the stand, if set. Without one the model stands in its modeled pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	TSoftObjectPtr<UAnimationAsset> PreviewAnimation;

	/** Turns the model on the stand (degrees) when its front isn't the mesh's +X. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	float PreviewYaw = 0.f;

	/** Order within its section; lower comes first, ties by name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	int32 SortOrder = 0;

	/** Level, health, attack and experience from ActorClass's defaults (loads the class). */
	FBestiaryStats ReadStats() const;

	/** PreviewMesh, or the first skeletal mesh on ActorClass's defaults. */
	USkeletalMesh* LoadPreviewMesh() const;

	/**
	 * The materials the actor wears on this mesh in the world, slot by slot (its Blueprint may override the mesh's own),
	 * so the stand shows it as it looks out there. Empty when the actor doesn't use this mesh.
	 */
	TArray<UMaterialInterface*> GetPreviewMaterials(const USkeletalMesh* Mesh) const;

	/** The actor is of this entry's kind (ActorClass or a child of it). */
	bool Describes(const UClass* ActorType) const;

	/** "Creatures", "Enemies", "NPCs", "Friends". */
	static FText CategoryName(EBestiaryCategory Category);

	/** Every entry in the project (loaded), by section, then SortOrder, then name. */
	static TArray<UBestiaryEntry*> LoadAll();
};
