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

/**
 * Which chest it is: its models, how it opens and what it gives (Docs/Areas/RansomsRest.md, "Loot and chests"; the
 * lootable world, Docs/Polish/BorderlandsComparison.md item 10). New kinds go at the end: levels keep the number.
 */
UENUM(BlueprintType)
enum class EChestKind : uint8
{
	/** Ruth Calder's Ranger cache, the olive Supply Crate: one gun at Luck 0.5 (3.7% legendary). */
	SupplyCrate,
	/** The gang's Strongbox: two guns at Luck 1.0 (15.5% legendary); its vault wheel spins before the lid lifts. */
	Strongbox,
	/** An old pine coffin (the undertaker's yard, the coffin shed), pried open with a hold: ammo, a gun now and then. */
	Coffin,
	/** An old grave dug up with a hold (boot hill, the churchyard): the mound sinks, the coffin comes up, its lid goes. */
	Grave,
	/** A farm's or a house's mailbox: its tin door dropped open with a tap; a pickup of ammo, rarely a gun. */
	Mailbox,
	/** A traveller's or a miner's footlocker: its lid opened with a tap; ammo and a fair chance of a gun. */
	Footlocker,
};

/** Where a chest is in its opening. */
UENUM(BlueprintType)
enum class EChestState : uint8
{
	Closed,
	/** What comes before the lid: the Strongbox's wheel turning, a coffin's lid pried at, a grave dug. */
	Unlocking,
	/** The lid swinging up or shoved off; the loot comes out partway (AChest::LootShare). */
	Opening,
	/** Open and empty, for good. */
	Open,
};

/** How a lid comes off. */
enum class EChestLidMotion : uint8
{
	/** Swung about its hinge (the body's SOCKET_Lid) to OpenAngle: up for a chest's lid, down for a mailbox's door. */
	Hinge,
	/** Shoved off and aside: lifted, carried to SlideOffset and turned to SlideTurn (a coffin's nailed lid). */
	Slide,
};

/** What a chest does before its lid moves. */
enum class EChestPreMotion : uint8
{
	None,
	/** The Strongbox's vault wheel spins (WheelTurns, over the chest's WheelSeconds). */
	Wheel,
	/** A coffin's lid is pried at: it jumps on its nails, twice, over PreSeconds. */
	Pry,
	/** A grave is dug over PreSeconds: its mound sinks into the ground and the body (the open grave) heaves up out of it,
	 *  dirt flying off the spade. */
	Dig,
};

/**
 * One kind of chest as the design (Docs/Areas/RansomsRest.md) and its models (Art/Models/Loot/Chests.py,
 * Art/Models/Props/Lootables.py) have it. The socket points stand in while a model isn't imported (a test level, a level
 * built before the import), in the body's frame: Blender's (x, y, z) is Unreal's (-y, -x, z), its front +X.
 */
struct AI_LOOTER_SHOOTER_API FChestKindInfo
{
	/** Its models: the body, the lid on the body's SOCKET_Lid, the wheel on SOCKET_Wheel; a grave's mound and spade (null: none). */
	const TCHAR* BodyPath = nullptr;
	const TCHAR* LidPath = nullptr;
	const TCHAR* WheelPath = nullptr;
	const TCHAR* CoverPath = nullptr;
	const TCHAR* ShovelPath = nullptr;

	/** How the lid comes off, and how far a hinged lid opens about its hinge (degrees: Chests.py's OPEN_ANGLE; negative swings down). */
	EChestLidMotion LidMotion = EChestLidMotion::Hinge;
	float OpenAngle = 90.f;

	/** A shoved lid: where it ends up from where it sat (cm, the body's frame), turned how, lifted how high on the way. */
	FVector SlideOffset = FVector::ZeroVector;
	FRotator SlideTurn = FRotator::ZeroRotator;
	float SlideLift = 0.f;

	/** How long the lid takes (seconds; 0: the chest's own LidSeconds). */
	float LidSeconds = 0.f;

	/** What comes before the lid, and how long it takes (Pry, Dig; the wheel takes the chest's WheelSeconds). */
	EChestPreMotion PreMotion = EChestPreMotion::None;
	float PreSeconds = 0.f;

	/** Whole turns the wheel makes before the lid lifts (0: it has no wheel). */
	float WheelTurns = 0.f;

	/** Held to open (seconds of the Interact key; 0: a tap). */
	float HoldSeconds = 0.f;

	/**
	 * What it gives, once: its guns from the default loot table at this luck (each opening has GunChance of giving them
	 * at all), ammo pickups of a chest's fixed 36 rounds, and a soul-mote at MoteChance (a grave's dead, giving back).
	 */
	int32 Guns = 1;
	float GunChance = 1.f;
	float Luck = 0.f;
	int32 AmmoPickups = 0;
	float MoteChance = 0.f;

	/** SOCKET_Lid (the hinge, or where a shoved lid sits), SOCKET_Loot (where loot comes out), SOCKET_Wheel (cm). */
	FVector Hinge = FVector::ZeroVector;
	FVector LootPoint = FVector::ZeroVector;
	FVector WheelHub = FVector::ZeroVector;

	/** The body's height up to the lid (cm). */
	float Height = 0.f;

	/** Where the prompt points (the actor's frame), when the body's own SOCKET_Interact won't do (a grave's body starts underground). */
	TOptional<FVector> InteractPoint;

	/** A grave: its body (the open grave) starts this far under the ground; its mound sinks this far as it's dug (cm). */
	float BodySink = 0.f;
	float CoverSink = 0.f;

	/** A grave's spade: standing by the mound before (the tell that it can be dug), in the spoil heap after (the actor's frame). */
	FTransform ShovelBefore = FTransform::Identity;
	FTransform ShovelAfter = FTransform::Identity;

	/** Its body and lid are solid (a grave's heaps and its lid aren't: nothing to stand on or snag). */
	bool bSolid = true;

	/** The sounds: as what comes before the lid starts, and as the lid starts to move. */
	const TCHAR* PreCue = nullptr;
	const TCHAR* OpenCue = nullptr;

	/** Shown on the map as a chest (the Supply Crates and Strongboxes: the treasure; not every mailbox and grave). */
	bool bOnMap = false;

	static FChestKindInfo Get(EChestKind Kind);

	/** Every kind. */
	static TConstArrayView<EChestKind> All();
};

/** A chest's own loot numbers in place of its kind's (the build script sets them: a strongbox behind the saloon gives less). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FChestLootOverride
{
	GENERATED_BODY()

	/** Use these numbers rather than the kind's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	bool bOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ClampMin = "0"))
	int32 Guns = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ClampMin = "0", ClampMax = "1"))
	float GunChance = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ClampMin = "0"))
	float Luck = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ClampMin = "0"))
	int32 AmmoPickups = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ClampMin = "0", ClampMax = "1"))
	float MoteChance = 0.f;
};

/**
 * A loot chest the player opens once with Interact: Ruth Calder's three Ranger caches and the gang's Strongbox in the
 * sheriff's office (Docs/Areas/RansomsRest.md, step 26), and the lootable world's coffins, graves, mailboxes and
 * footlockers (Docs/Polish/BorderlandsComparison.md, item 10; Tools/Unreal/build_area_loot.py places them). Its body, its
 * lid on the body's SOCKET_Lid, the Strongbox's vault wheel on SOCKET_Wheel, and a grave's mound and spade come from its
 * kind (FChestKindInfo; OnConstruction sets them, so the level keeps them), unless the build script gave it a model of its
 * own (a variant).
 *
 * Opened (a tap, or a hold for a coffin or a grave), what comes first comes first: the Strongbox's wheel spins, a coffin's
 * lid jumps on its nails, a grave's mound sinks while the open grave heaves up out of the ground and the spade throws
 * dirt. Then the lid swings about its hinge to its kind's angle, or is shoved off and aside, over its seconds, easing in
 * and out. As the lid passes LootShare the loot pops out of SOCKET_Loot and lands in front of it, as any loot does
 * (tossed, its beam and label with it): its kind's guns (if its chance gives any), rolled from the default loot table
 * (ULootLibrary::GetDefaultLootTable) at its luck and at the area's level for the player (the player's level kept inside
 * the area's band, as creature drops and mission rewards take it); never a gun in a practice area the player has left
 * (UAreaRulesSubsystem::DropsGunsAt); its ammo pickups; and sometimes a soul-mote. It gives once only: open, it's used up
 * and never focused again.
 *
 * Opened stays open: the session keeps the chests that have given their loot with the map's world, by ChestId
 * (FSavedMapWorld::OpenedChests), and puts them back open and empty (RestoreOpened); the loot left lying comes back as
 * every gun on the ground does. A save taken mid-swing, before the loot is out, keeps it closed, so nothing is lost.
 *
 * Solid (BlockAllDynamic, world dynamic: loot and ground traces pass through it) unless its kind says otherwise (a grave),
 * and tagged Chest and Obstacle. It opens from the start unless OpenWhen says otherwise, and ticks only while it moves.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AChest : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag every chest carries (missions, the console). */
	static const FName ChestTag;

	/** The models' sockets (Chests.py, Lootables.py): the lid's hinge, where loot comes out, the wheel's hub. */
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
	 * Opens it as Interact does: what comes first, the lid, the loot partway. bForce opens it whatever OpenWhen says (the
	 * console). Missions hear of it from the player's interaction component, not from here. False when it isn't closed, or
	 * can't be opened now.
	 */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	bool Open(AActor* ByWhom, bool bForce = false);

	/** Moves the opening on by DeltaSeconds (the tick does while it moves; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/** Open and empty as the session keeps it (USessionSubsystem::RestoreWorld): the lid off at once, no loot, no save. */
	void RestoreOpened();

	/** Shut and full again (the console): the lid down, its loot to give once more. What it gave stays where it lies. */
	void CloseAgain();

	/** It's closed and OpenWhen holds (empty: any time). */
	UFUNCTION(BlueprintPure, Category = "Chest")
	bool CanOpen() const;

	EChestState GetState() const { return State; }

	/** Its loot is out (or the session says so): the save keeps it open from here on. */
	bool HasGivenLoot() const { return bLootGiven; }

	/**
	 * How far the lid stands open (degrees about its hinge; a shoved lid's share of its way times its kind's OpenAngle) and
	 * the wheel has turned (degrees about its front axis); how far the lid has come (0 shut to 1 open), and the step before
	 * it (0 to 1: the wheel, the pry, the dig).
	 */
	float GetLidAngle() const { return LidAngle; }
	float GetWheelAngle() const { return WheelAngle; }
	float GetLidShare() const { return LidShare; }
	float GetPreShare() const { return PreShare; }

	/** What the session keeps it by: ChestId, else its name in the level. */
	FName GetSaveKey() const;

	FChestKindInfo GetKindInfo() const { return FChestKindInfo::Get(Kind); }

	/** Held to open, and for how long (seconds); 0: a tap. */
	float GetHoldSeconds() const { return GetKindInfo().HoldSeconds; }

	/** Its loot numbers: its kind's, or its own override's (Guns, GunChance, Luck, AmmoPickups and MoteChance of the kind's info). */
	FChestKindInfo GetLootInfo() const;

	/**
	 * What it gives as a loot table: the default table's guns and ammo classes, every roll its guns at its luck (at its gun
	 * chance; none where the area drops no guns) and its ammo pickups at a chest's fixed 36 rounds. A new transient table each call.
	 */
	ULootTable* MakeLootTable() const;

	/** The level its guns come at: the player's, kept inside the area's band (1 without a player). */
	int32 GetLootLevel() const;

	/** The loot it dropped, while it's still in the world (picked up, it's gone from here). */
	TArray<AActor*> GetDroppedLoot() const;

	/** Whether the map shows it as a chest (its kind's bOnMap: the caches and strongboxes, not every grave and mailbox). */
	bool ShowsOnMap() const { return GetKindInfo().bOnMap; }

	/** The words for a kind's prompt: "Open the crate", "Open the strongbox", "Pry the coffin open", "Dig up the grave"... */
	static FText DefaultPrompt(EChestKind ForKind);

	/** A kind's name on the map or a list: "Supply crate", "Strongbox", "Coffin", "Grave", "Mailbox", "Footlocker". */
	static FText KindName(EChestKind ForKind);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The chest, its pivot on the ground at the middle of its footprint, its front +X. A grave's: the open grave, sunk until dug. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	/** The lid, its origin on the hinge (the body's SOCKET_Lid); it pitches about the hinge, or is shoved off. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lid;

	/** The Strongbox's vault wheel on SOCKET_Wheel, spinning about its front axis; hidden on a kind without one. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Wheel;

	/** A grave's mound, sinking away as it's dug; hidden on a kind without one. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Cover;

	/** A grave's spade, standing by it (the sign it can be dug) and digging; hidden on a kind without one. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Shovel;

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

	/** Its own loot numbers, when set (bOverride), in place of its kind's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	FChestLootOverride LootOverride;

	/** How far from the player's eyes it can be opened (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 0.f;

	/** How long the lid takes to swing open (unless its kind says), and the wheel to spin before it (seconds). */
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

	/** How long the step before the lid takes (the wheel's WheelSeconds, a pry's or a dig's PreSeconds), and the lid. */
	float GetPreSeconds() const;
	float GetLidSecondsFor() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its kind's models (keeping one the build script set), the lid and wheel on their sockets, collision, and its pose. */
	void ApplyKind();

	void SetLidAngle(float Degrees);
	void SetWheelAngle(float Degrees);

	/** The lid Share (0 to 1) of its way off: about its hinge, or shoved aside (ChestOpening.cpp). */
	void SetLidShare(float Share);

	/** What comes before the lid, Share (0 to 1) of the way through: the wheel, the pry's jumps, the dig (ChestOpening.cpp). */
	void SetPreShare(float Share);

	/** Everything shut and still, or open for good: the poses of the lid, the wheel, a grave's body, mound and spade. */
	void PoseClosed();
	void PoseOpen();

	/** The dig's moments: dirt thrown off the spade (ChestOpening.cpp). */
	void ThrowDirt(float Strength) const;

	/** Rolls and throws its loot out of SOCKET_Loot, once; asks the session to save soon (it's progress). */
	void DropLoot();

	/** Where the loot comes out: SOCKET_Loot, or its kind's point without the model. */
	FVector GetLootOrigin() const;

	/** The Index-th of Count throws: fanned across its front, a little random. */
	FVector TossVelocity(int32 Index, int32 Count, FRandomStream& Random) const;

	EChestState State = EChestState::Closed;
	float Clock = 0.f;
	float LidAngle = 0.f;
	float LidShare = 0.f;
	float WheelAngle = 0.f;
	float PreShare = 0.f;
	/** The dig's dirt thrown so far (each of its moments once). */
	int32 DirtThrown = 0;
	bool bLootGiven = false;

	/** Where the lid sits on the body when shut (relative to its socket, or its kind's point without the model). */
	FVector LidRest = FVector::ZeroVector;

	TArray<TWeakObjectPtr<AActor>> DroppedLoot;
};
