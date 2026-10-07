#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "Chest.generated.h"

class ULootTable;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FRandomStream;

/** Which chest it is: its models, how it opens and what it gives (Docs/Areas/RansomsRest.md, "Loot and chests"). */
UENUM(BlueprintType)
enum class EChestKind : uint8
{
	/** Ruth Calder's Ranger cache, the olive Supply Crate: one gun at Luck 0.5 (3.7% legendary). */
	SupplyCrate,
	/** The gang's Strongbox: two guns at Luck 1.0 (15.5% legendary); its vault wheel spins before the lid lifts. */
	Strongbox,
};

/** Where a chest is in its opening. */
UENUM(BlueprintType)
enum class EChestState : uint8
{
	Closed,
	/** The Strongbox's wheel turning, before the lid moves. */
	Unlocking,
	/** The lid swinging up; the loot comes out partway (AChest::LootShare). */
	Opening,
	/** Open and empty, for good. */
	Open,
};

/**
 * One kind of chest as the design (Docs/Areas/RansomsRest.md) and its models (Art/Models/Loot/Chests.py) have it. The
 * socket points stand in while a model isn't imported (a test level, a level built before the import), in the body's
 * frame: Blender's (x, y, z) is Unreal's (-y, -x, z), its front +X.
 */
struct AI_LOOTER_SHOOTER_API FChestKindInfo
{
	/** Its models in /Game/Art/Loot: the body, the lid on the body's SOCKET_Lid, the wheel on SOCKET_Wheel (null: none). */
	const TCHAR* BodyPath = nullptr;
	const TCHAR* LidPath = nullptr;
	const TCHAR* WheelPath = nullptr;

	/** How far the lid opens about its hinge (degrees: Chests.py's OPEN_ANGLE). */
	float OpenAngle = 90.f;

	/** Whole turns the wheel makes before the lid lifts (0: it has no wheel). */
	float WheelTurns = 0.f;

	/** What it gives, once: guns from the default loot table at this luck, and ammo pickups of a chest's fixed 36 rounds. */
	int32 Guns = 1;
	float Luck = 0.f;
	int32 AmmoPickups = 0;

	/** SOCKET_Lid (the hinge, the back top edge), SOCKET_Loot (the middle of the floor inside), SOCKET_Wheel (cm). */
	FVector Hinge = FVector::ZeroVector;
	FVector LootPoint = FVector::ZeroVector;
	FVector WheelHub = FVector::ZeroVector;

	/** The body's height up to the lid (cm). */
	float Height = 0.f;

	static FChestKindInfo Get(EChestKind Kind);
};

/**
 * A loot chest the player opens once with a tap of Interact (Docs/Areas/RansomsRest.md, step 26: Ruth Calder's three
 * Ranger caches, the Supply Crates her note on the Rim Rangers' board names, and the gang's Strongbox in the sheriff's
 * office). Its body, its lid on the body's SOCKET_Lid and, for the Strongbox, its vault wheel on SOCKET_Wheel come from its
 * kind (FChestKindInfo; OnConstruction sets them, so the level keeps them), unless the build script gave it a model of its
 * own (a variant).
 *
 * Opened, the Strongbox's wheel spins first (WheelSeconds), then the lid swings up about its hinge to its kind's angle over
 * LidSeconds, easing in and out. As the lid passes LootShare of its swing the loot pops out of SOCKET_Loot and lands in
 * front of it, as any loot does (tossed, its beam and label with it): its kind's guns, rolled from the default loot table
 * (ULootLibrary::GetDefaultLootTable) at its luck and at the area's level for the player (the player's level kept inside
 * the area's band, as creature drops and mission rewards take it), and its ammo pickups. It gives once only: open, it's
 * used up and never focused again.
 *
 * Opened stays open: the session keeps the chests that have given their loot with the map's world, by ChestId
 * (FSavedMapWorld::OpenedChests), and puts them back open and empty (RestoreOpened); the loot left lying comes back as
 * every gun on the ground does. A save taken mid-swing, before the loot is out, keeps it closed, so nothing is lost.
 *
 * It's solid (BlockAllDynamic, world dynamic: loot and ground traces pass through it) and tagged Chest and Obstacle. It
 * opens from the start unless OpenWhen says otherwise, and ticks only while it moves.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AChest : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag every chest carries (missions, the console). */
	static const FName ChestTag;

	/** The models' sockets (Chests.py): the lid's hinge, where loot comes out, the wheel's hub. */
	static const FName LidSocket;
	static const FName LootSocket;
	static const FName WheelSocket;

	AChest();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	/** Once opened it's used up for good. */
	virtual bool IsUsedUp() const override { return State != EChestState::Closed; }

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Opens it as a tap of Interact does: the wheel, the lid, the loot partway. bForce opens it whatever OpenWhen says (the
	 * console). Missions hear of it from the player's interaction component, not from here. False when it isn't closed, or
	 * can't be opened now.
	 */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	bool Open(AActor* ByWhom, bool bForce = false);

	/** Moves the opening on by DeltaSeconds (the tick does while it moves; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/** Open and empty as the session keeps it (USessionSubsystem::RestoreWorld): the lid up at once, no loot, no save. */
	void RestoreOpened();

	/** Shut and full again (the console): the lid down, its loot to give once more. What it gave stays where it lies. */
	void CloseAgain();

	/** It's closed and OpenWhen holds (empty: any time). */
	UFUNCTION(BlueprintPure, Category = "Chest")
	bool CanOpen() const;

	EChestState GetState() const { return State; }

	/** Its loot is out (or the session says so): the save keeps it open from here on. */
	bool HasGivenLoot() const { return bLootGiven; }

	/** How far the lid stands open (degrees about its hinge) and the wheel has turned (degrees about its front axis). */
	float GetLidAngle() const { return LidAngle; }
	float GetWheelAngle() const { return WheelAngle; }

	/** What the session keeps it by: ChestId, else its name in the level. */
	FName GetSaveKey() const;

	FChestKindInfo GetKindInfo() const { return FChestKindInfo::Get(Kind); }

	/**
	 * What it gives as a loot table: the default table's guns and ammo classes, every roll its kind's guns at its kind's
	 * luck and its ammo pickups at a chest's fixed 36 rounds. A new transient table each call.
	 */
	ULootTable* MakeLootTable() const;

	/** The level its guns come at: the player's, kept inside the area's band (1 without a player). */
	int32 GetLootLevel() const;

	/** The loot it dropped, while it's still in the world (picked up, it's gone from here). */
	TArray<AActor*> GetDroppedLoot() const;

	/** The words for a kind's prompt: "Open the crate", "Open the strongbox". */
	static FText DefaultPrompt(EChestKind ForKind);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The chest, its pivot on the ground at the middle of its footprint, its front +X. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	/** The lid, its origin on the hinge (the body's SOCKET_Lid); it pitches up about the hinge to open. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lid;

	/** The Strongbox's vault wheel on SOCKET_Wheel, spinning about its front axis; hidden on a kind without one. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Wheel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	EChestKind Kind = EChestKind::SupplyCrate;

	/** Its stable id ("RangerCache_Windmill"; the build script sets it), which the session keeps it open by. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	FName ChestId;

	/** It opens only while this holds. Empty (the caches, the Strongbox): any time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	FStoryCondition OpenWhen;

	/** The prompt's words; empty: its kind's (DefaultPrompt). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	FText Prompt;

	/** How far from the player's eyes it can be opened (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 0.f;

	/** How long the lid takes to swing open, and the wheel to spin before it (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Opening", meta = (ClampMin = "0.05", Units = "s"))
	float LidSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Opening", meta = (ClampMin = "0.05", Units = "s"))
	float WheelSeconds = 0.7f;

	/**
	 * The share of the lid's swing at which the loot comes out: by then the lid stands near upright, so nothing flies
	 * through it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Opening", meta = (ClampMin = "0", ClampMax = "1"))
	float LootShare = 0.6f;

	/**
	 * How the loot is thrown out (cm/s): this fast out of its front, fanned this far apart (degrees, from one end to the
	 * other), and up. About a metre in front of it, so it lands inside a small office too.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Loot", meta = (ClampMin = "0"))
	float TossForward = 110.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Loot", meta = (ClampMin = "0", ClampMax = "180"))
	float TossFan = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Loot", meta = (ClampMin = "0"))
	float TossUp = 430.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its kind's models (keeping one the build script set), the lid and wheel on their sockets, and its pose. */
	void ApplyKind();

	void SetLidAngle(float Degrees);
	void SetWheelAngle(float Degrees);

	/** Rolls and throws its loot out of SOCKET_Loot, once; asks the session to save soon (it's progress). */
	void DropLoot();

	/** Where the loot comes out: SOCKET_Loot, or its kind's point without the model. */
	FVector GetLootOrigin() const;

	/** The Index-th of Count throws: fanned across its front, a little random. */
	FVector TossVelocity(int32 Index, int32 Count, FRandomStream& Random) const;

	EChestState State = EChestState::Closed;
	float Clock = 0.f;
	float LidAngle = 0.f;
	float WheelAngle = 0.f;
	bool bLootGiven = false;

	TArray<TWeakObjectPtr<AActor>> DroppedLoot;
};
