#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "BargeBoarding.generated.h"

class ABargeFurnace;
class ABargeHoseDrum;
class AFurnaceDraftValve;
class ALamplighterShip;
class ALandfallMarker;
class APawn;
class UAudioComponent;
class UPrimitiveComponent;
class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMissionRunner;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Where Vane's barge is in Main 6 (its own state, read from the story and moved on by its moments). */
UENUM(BlueprintType)
enum class EBargeState : uint8
{
	/** Not this visit (not Main 6, or fallen): hidden, no collision, no tick. */
	Away,
	/** Hanging over the chapel knoll, her furnace lit, Abel's light in its cage (steps 0-6). */
	Hanging,
	/** The cage burst (the CageBurst scene's end): her pilot light gone, about to die. */
	Burst,
	/** Step 7: every light out, listing and drifting west, her back half burning. */
	Dying,
	/** BargeFall: the scene has her (ApplyFallAt). */
	Falling,
	/** Down in Gravewind Canyon (Pilot.BargeFell): gone from the sky for good. */
	Fallen,
};

/**
 * Vane's dredge barge brought down over the chapel to hose Saint Ada herself (Main 6, "The Pilot Light": MAINS_2_7.md
 * section 10; lane MAIN67's contract, W3): the one time she's in reach, so the Wardens board her. The Lamplighter (W2's
 * sortie) grapples her at GetBerth and lays a plank to GetBoardingPoint; from there everything of steps 3-7 is hers:
 *
 *  - Her top deck, ~70 m of riveted plate walkable as a kit built in code (deck plates, rails and unseen rail walls, three
 *    stacks, a jib crane, cover crates, the hose drum's gantry with ABargeHoseDrum on it, four horn posts the build script
 *    stands ACompanyLoudspeakers on): the BargeDeck fight's ground. Eight furnace vents along it breathe in a running
 *    pattern: a second's glow through their slots, then a column of furnace fire that burns whatever stands in it.
 *  - The furnace hatch aft (a Reach marker tagged Barge_FurnaceHatch), a stair down to the furnace hall's catwalk.
 *  - The furnace hall (22 m round: the widest that fits the hull; the design's 26 m doesn't), 9 m high, its catwalk ring at
 *    4 m, two stokehold doors (the FurnaceHall pushes come through them), the stoker's office off the catwalk (the manifest
 *    lies on its desk; its radio carries Ned's call once the manifest is taken), and in the middle ABargeFurnace (the soul
 *    cage, the grate ring) and three AFurnaceDraftValves at the wall.
 *  - Her hose, laid each frame as instanced sections from the drum under her keel toward the chapel's roof (HoseTarget),
 *    snaking lower through the fight (BargeRules::HoseReach), and stopped where it is when the drum is jammed.
 *  - The cage's burst (Scenes/CageBurst) sets her dying (step 7): every light out, the deck listing 12 degrees to her berth
 *    side and drifting west on the dusk wind, her back half burning, the third stack toppling across the port lane; the
 *    Lamplighter hangs under the broken gangway (GetJumpBerth) for the jump.
 *  - BargeFall (Scenes/BargeFall) carries her over the Rest raining soul coal and down into Gravewind Canyon (FallTarget):
 *    the fireball, then Pilot.BargeFell, and she's gone (the build script's wreck shows from that flag).
 *
 * She's in the world during Main 6 only (ShownWhen, and never once Pilot.BargeFell is set): hidden, without collision and
 * not ticking otherwise. A session loaded on any step finds her as that step leaves her (valves open on step 5's start,
 * all shut past it; the cage burst from step 7; dying in step 7). A player off both her and the Lamplighter during steps
 * 3-7 is put back aboard (a fall, a death's wake at a lantern post far below). Split by topic: BargeBoarding.cpp (life,
 * story, the getters), BargeBoardingKit.cpp (the walkable kit), BargeBoardingDeck.cpp (the tick: vents, hose, keeping the
 * player aboard, the beats' lines), BargeBoardingDying.cpp (dying, the stack, the fall).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABargeBoarding : public AActor
{
	GENERATED_BODY()

public:
	ABargeBoarding();

	/** The tag the build script gives her (and the dev commands find her by). */
	static const FName BargeTag;
	/** The furnace hatch's Reach marker's tag (Main 6 step 3). */
	static const FName HatchTag;
	/** The deck's and the hall's fights' spawner ids (their spots: GetFightSpots). */
	static const FName DeckFight;
	static const FName HallFight;
	/** The user's Tripo hull (one path; the stand-in is an iron box of its size). */
	static const TCHAR* const HullPath;

	/** The level's barge (the first found), or null. */
	static ABargeBoarding* Find(const UWorld* World);

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// --- The contract's interface (W2's sortie reads these) ---

	/** Shown this visit: during Main 6, before she falls. */
	bool IsPresent() const { return State != EBargeState::Away && State != EBargeState::Fallen; }

	/** Where the Lamplighter's keel lies grappled alongside (world): her +X along the barge, her starboard to it. Callable
	 *  from Python too: build_area_main6_sortie.py aims her course's end at it. */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	FTransform GetBerth() const;

	/** Where she hangs for the jump in step 7 (world): under the broken gangway's end, her deck JumpUnderDeck below it, level
	 *  (the barge's yaw, not her list). Moves with the dying barge. */
	FTransform GetJumpBerth() const;

	/** The two points on her rail the Lamplighter's grapples bite (world), Index 0 fore, 1 aft. */
	FVector GetGrappleAnchor(int32 Index) const;

	/** Where the gangplank from the Lamplighter's starboard rail lands on her deck (world). */
	FVector GetBoardingPoint() const;

	/** Where a player is put aboard (a respawn, a fall, the dev jump): the boarding point in steps 3-4, by the hatch after. */
	FTransform GetPlayerStart() const;

	/** The cage's burst sets her dying (step 7): idempotent; a load on step 7 calls it too. */
	void BeginDying();
	bool IsDying() const { return State == EBargeState::Dying; }

	/** BargeFall's control: her pose Seconds into the fall, and its end state (hidden for good, Pilot.BargeFell set). */
	void ApplyFallAt(float Seconds);
	void FinishFall();

	// --- The build script's getters (world, from her placed transform: callable in the editor) ---

	/** The fight's spawn spots: BargeDeck (nine, behind the deck's cover) or FurnaceHall (eight, four at each stokehold). */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	TArray<FVector> GetFightSpots(FName Fight) const;

	/** The stoker's desk top, where the manifest lies. */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	FVector GetOfficeSpot() const;

	/** The four horn posts' feet on the deck, each turned to face inboard. */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	TArray<FTransform> GetHornSpots() const;

	/** The furnace hatch's middle on the deck. */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	FVector GetHatchSpot() const;

	/** The deck's middle and the hall's floor middle (the spawners stand there). */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	FVector GetDeckMiddle() const;

	UFUNCTION(BlueprintCallable, Category = "Barge")
	FVector GetHallMiddle() const;

	/** Where her hose leaves the drum under her keel (world). */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	FVector GetHoseTop() const;

	// --- The beats (the furnace, the valves, the drum and the scenes tell her) ---

	EBargeState GetState() const { return State; }
	float GetDyingSeconds() const { return DyingClock; }

	/** The hose drum was jammed: the hose stops where it is (and stays stopped: Pilot.DrumJammed). */
	void NoteDrumJammed();
	bool IsDrumJammed() const;

	/** Her hose's reach toward the chapel's roof now (0-1). */
	float GetHoseReach() const { return HoseShare; }

	/** The cage burst (CageBurst's end): her pilot light gone. */
	void NoteCageBurst();

	/** A draft valve was shut (by the player's hold): the furnace dims a notch, the ring cools, the hall hears it. */
	void NoteValveShut(AFurnaceDraftValve& Valve);

	ABargeFurnace* GetFurnace() const { return Furnace; }
	const TArray<TObjectPtr<AFurnaceDraftValve>>& GetValves() const { return Valves; }
	ABargeHoseDrum* GetHoseDrum() const { return HoseDrum; }
	int32 CountValvesShut() const;

	/** The step of Main 6 she reads now (INDEX_NONE when it isn't being played). */
	int32 GetStoryStep() const { return StoryStep; }

	/** Reads the story again (the missions changed). */
	void RefreshStory();

	/** Plays her as if Main 6 were on Step, whatever the story says (the console, the tests); INDEX_NONE: back to the story. */
	void ForceStep(int32 Step);

	/** Moves her on by DeltaSeconds (the tick's work; tests call it with no play running). */
	void Advance(float DeltaSeconds);

	/** The walkable kit's pieces (for the tests: the deck is there, the rails stand, the hall's walls fit the hull). */
	const TArray<TObjectPtr<UPrimitiveComponent>>& GetWalkables() const { return Walkables; }
	const TArray<TObjectPtr<UBoxComponent>>& GetRailWalls() const { return RailWalls; }

	// --- Placed by the build script ---

	/** She's in the world while this holds (Main 6). Empty: always (a test level). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barge|Story")
	FStoryCondition ShownWhen;

	/** Where her hose reaches for (world): the chapel's roof ridge. Zero: straight down 20 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barge|Hose")
	FVector HoseTarget = FVector::ZeroVector;

	/** Where her fall ends (world): the canyon floor below the Rim, south of the boards, where the wreck lies. Zero: on west. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barge|Fall")
	FVector FallTarget = FVector::ZeroVector;

	/** Where the black streak that was Abel comes down (world): the burial boards on Gravewind Point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barge|Fall")
	FVector BoardsPoint = FVector::ZeroVector;

	/**
	 * The walkable kit raised (or lowered) this far over the hull (cm): BargeRules::DeckTop is read off the Tripo model's
	 * proportions by eye, so this fits the kit's deck onto the hull's own once she's seen placed. 0: as the rules have it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barge|Kit", meta = (Units = "cm"))
	float KitLift = 0.f;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Everything walkable, shot at and looked at hangs off this (KitLift raises it over the hull). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Kit;

	/** The hull's look (SM_DredgeBarge, or the stand-in box); no collision: the kit is what's walked and shot. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hull;

	/** The third stack's foot: it turns about this to fall in step 7. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> FallingStackPivot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FallingStack;

	/** The deck's work lamps on the stacks (shadowless): out as she dies. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UPointLightComponent>> DeckLamps;

	/** Instanced looks: rails, cover, the vents (cold, glowing, their fire), the hall's wall panels and catwalk, her hose's
	 *  sections, the stern's fire as she dies, and the soul coal raining off her in the fall. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> RailLook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> CoverLook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> VentCold;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> VentGlow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> VentFire;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> HallWallLook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> CatwalkLook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> HoseSections;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> SternFire;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> CoalRain;

	/** The office's radio set (Ned's call comes out of it). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Radio;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// --- BargeBoarding.cpp ---
	void SetState(EBargeState NewState);
	/** Shown or not: visibility, collision, tick, her pieces with her. */
	void ApplyPresence(bool bShown);
	void HandleMissionsChanged();
	bool StoryHolds(const FStoryCondition& Condition) const;
	/** A step began live (not on a load): its beat's lines. */
	void OnStepBegan(int32 Step);
	/** Her pieces that are actors of their own (the furnace, valves, drum, the hatch's marker), spawned and attached once. */
	void SpawnPieces();
	FVector ToWorld(const FVector& Local) const;
	/** Her placed pose (before any list or drift), the frame the getters and the dying pose start from. */
	FTransform RestTransform() const;

	// --- BargeBoardingKit.cpp ---
	void BuildKit();
	UStaticMeshComponent* MakePiece(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look, const FVector& Center,
		const FVector& Size, const FRotator& Turn = FRotator::ZeroRotator, bool bWalkable = true);
	UBoxComponent* MakeWall(const TCHAR* Name, const FVector& Center, const FVector& Half, const FRotator& Turn = FRotator::ZeroRotator);
	UInstancedStaticMeshComponent* MakeInstanced(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look);
	void BuildDeck();
	void BuildHall();
	/** The instanced looks that never change (rails, cover, walls, catwalk), laid once (OnConstruction and BeginPlay). */
	void LayStaticLooks();

	// --- BargeBoardingDeck.cpp ---
	void AdvanceVents(float DeltaSeconds);
	void AdvanceHose(float DeltaSeconds);
	void KeepPlayerAboard(float DeltaSeconds);
	/** Hurts every pawn inside a column (Radius, Height up from Base): the player by a share of their health, creatures too. */
	void HurtColumn(const FVector& Base, float Radius, float Height, float HealthShare, float Seconds);
	APawn* FindPlayer() const;

	// --- BargeBoardingDying.cpp ---
	void AdvanceDying(float DeltaSeconds);
	void PoseDying();
	void DropStack(float Alpha);
	void LayCoal(float Seconds);

	EBargeState State = EBargeState::Hanging;
	int32 StoryStep = INDEX_NONE;
	int32 ForcedStep = INDEX_NONE;
	bool bForced = false;
	bool bStoryRead = false;
	float StepClock = 0.f;

	// Vents.
	float VentClock = 0.f;
	uint32 VentGlowMask = 0u;
	uint32 VentFireMask = 0u;
	float VentHurtClock = 0.f;
	bool bVentLineSaid = false;

	// Hose.
	float HoseShare = 0.55f;
	bool bHoseFrozen = false;
	float HoseNativeLength = 100.f;

	// Keeping the player aboard.
	float OffFor = 0.f;

	// Dying and the fall.
	FTransform Placed = FTransform::Identity;
	bool bPlacedKnown = false;
	float DyingClock = 0.f;
	bool bStackDown = false;
	bool bStackHurtDone = false;
	FTransform FallFrom = FTransform::Identity;
	float FireWait = 0.f;
	bool bImpacted = false;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
	TWeakObjectPtr<UAudioComponent> FurnaceDrone;

	UPROPERTY(Transient)
	TObjectPtr<ABargeFurnace> Furnace;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFurnaceDraftValve>> Valves;

	UPROPERTY(Transient)
	TObjectPtr<ABargeHoseDrum> HoseDrum;

	UPROPERTY(Transient)
	TObjectPtr<ALandfallMarker> HatchMarker;

	/** Everything walkable (deck, floors, stairs, catwalk): what the tests look at. */
	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> Walkables;

	UPROPERTY()
	TArray<TObjectPtr<UBoxComponent>> RailWalls;

	UPROPERTY()
	TArray<TObjectPtr<UBoxComponent>> HallWalls;
};
