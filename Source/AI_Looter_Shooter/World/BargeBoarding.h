#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "World/BargeRules.h"
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
 *  - Her hull: the user's Tripo SM_DredgeBarge jumboed (BARGE_LAYOUT.md), its own collision: a well deck, the forecastle,
 *    house walks to the cross walk and its hatch. Her decks are zones (BargeRules::DeckZones); the kit adds rail walls,
 *    cover, the hose drum's gantry, the falling third stack, horn posts (on the starboard rim), work lamps, four furnace
 *    vents breathing fire in a running pattern: the deck fight's ground, three pushes (BargeFightRules.h).
 *  - The furnace hatch (a Reach marker tagged Barge_FurnaceHatch at the stair's top) on the cross walk: its steep stair
 *    runs down through the coaming's hole onto the hall's catwalk, by the stoker's corner (the manifest lies on his desk;
 *    the radio on it carries Ned's call once the manifest is taken).
 *  - The furnace hall below the deck in the hull's hollow under the casing (15 m round: the user's "below deck, smaller"),
 *    9 m high, its catwalk ring at 5.8 m with two stairs down to the floor, two stokehold doors (the FurnaceHall pushes
 *    come through them), wall lanterns, and in the middle ABargeFurnace (the soul cage, the grate ring) and three
 *    AFurnaceDraftValves at the wall.
 *  - Her hose, laid each frame as instanced sections from the drum under her keel toward the chapel's roof (HoseTarget),
 *    snaking lower through the fight (BargeRules::HoseReach), and stopped where it is when the drum is jammed.
 *  - The cage's burst (Scenes/CageBurst) sets her dying (step 7): every light out, the deck listing 12 degrees to her berth
 *    side and drifting west on the dusk wind, her back half burning, the third stack toppling across the well deck; the
 *    Lamplighter hangs under the broken gangway (GetJumpBerth) for the jump.
 *  - BargeFall (Scenes/BargeFall) carries her over the Rest raining soul coal and down into Gravewind Canyon (FallTarget):
 *    the fireball, then Pilot.BargeFell, and she's gone (the build script's wreck shows from that flag).
 *
 * She's in the world during Main 6 only (ShownWhen, and never once Pilot.BargeFell is set): hidden, without collision and
 * not ticking otherwise. A session loaded on any step finds her as that step leaves her (valves open on step 5's start,
 * all shut past it; the cage burst from step 7; dying in step 7). A player off both her and the Lamplighter during steps
 * 3-7 is put back aboard (a fall, a death's wake at a lantern post far below). Split by topic (BargeBoarding.cpp's head).
 *
 * The hull below the deck is a hollow with nothing inside it (and no deck over it inside the casing), so the hall builds its
 * own room in it (floor, walls, a ceiling closing it, alcoves, lintels) in the kit's iron (BargeLooks::KitIron): nobody sees
 * into the shell. Without the hull (not imported) she's an iron box and the kit lays its own deck plates, coaming and rails on it.
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
	/** The fights' spawner ids (their spots: GetFightSpots): the deck's three pushes (BargeFightRules.h: the deck's own, the
	 *  roof's and the tower's droppers, the stern's), the hall's. */
	static const FName DeckFight;
	static const FName RoofFight;
	static const FName TowerFight;
	static const FName SternFight;
	static const FName HallFight;
	/** The reworked Tripo hull (one path; without it an iron box of its size stands in). */
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

	/** The two points on her bulwark's top the Lamplighter's grapples bite (world), Index 0 fore, 1 aft. */
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

	/** The fight's spawn spots: BargeDeck (nine, behind the deck's cover), the droppers' landings (BargeDeckRoof, three;
	 *  BargeDeckTower, two), the stern's (BargeDeckStern, four) or FurnaceHall (eight, four at each stokehold). */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	TArray<FVector> GetFightSpots(FName Fight) const;

	/** The droppers' ropes' tops for BargeDeckRoof or BargeDeckTower (their ARappelSpawner's RopeTops), each its landing's. */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	TArray<FVector> GetDropRopeTops(FName Fight) const;

	/** The stoker's desk top, where the manifest lies. */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	FVector GetOfficeSpot() const;

	/** The four horn posts' feet (against the house's fore face, on the starboard rim), each turned its way. */
	UFUNCTION(BlueprintCallable, Category = "Barge")
	TArray<FTransform> GetHornSpots() const;

	/** The furnace hatch on the deck: its stair's top, at the deck's height (the build script's log reads its Z). */
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

	/** The unseen pawn-only rail walls (one per BargeRules::RailWallLayout entry, in its order). */
	const TArray<TObjectPtr<UBoxComponent>>& GetRailWalls() const { return RailWalls; }
	/** The kit's floor plates as built: over the stand-in box only, one per deck zone cut round the hatch's hole (none over
	 *  the hull: its decks are walked as they are). */
	const TArray<TObjectPtr<UPrimitiveComponent>>& GetDeckPlates() const { return PlateParts; }
	/** The cover's props as laid (each barrel of a cluster, each sandbag wall of a stack), in BargeRules::CoverPieces' order. */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetCoverParts() const { return CoverParts; }
	/** The deck's lights: the work lamp on the house's fore face, then the work lamps (BargeRules::WorkLamps' order). */
	const TArray<TObjectPtr<UPointLightComponent>>& GetDeckLamps() const { return DeckLamps; }
	/** The lanterns' lights in the hall (the four under the catwalk, then the one over the stoker's desk). */
	const TArray<TObjectPtr<UPointLightComponent>>& GetLanterns() const { return Lanterns; }
	/** Where a player at OnHer (her frame) is for her lights: 0 on her deck or near, 1 below it, 2 far. */
	static int32 LightViewAt(const FVector& OnHer);
	/** Her lights for that view: the deck's for 0, the hall's and the cage's for 1, none for 2 (the tick calls it as the
	 *  player moves; tests call it). */
	void ShowLightsFor(int32 View);
	/** The deck fight's pushes she has started (1: the first, which starts on its own). */
	int32 GetPushesStarted() const { return PushesStarted; }
	/** The fights each deck push brings (0 to NumDeckPushes - 1). */
	static TArray<FName> PushFights(int32 Push);
	/** True when the reworked hull is hers (not the stand-in box): its decks, rails and superstructure carry their collision. */
	bool HasHull() const;

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
	 * The walkable kit raised (or lowered) this far over the hull (cm). The decks are fitted to SM_DredgeBarge's own in
	 * BargeRules::DeckZones (measured: Docs/Work/2026-11-04-Art/BARGE_LAYOUT.md), not here, so this stays 0; a placed barge
	 * given another hull would use it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barge|Kit", meta = (Units = "cm"))
	float KitLift = 0.f;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Everything walkable, shot at and looked at hangs off this (KitLift raises it over the hull). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Kit;

	/** The hull: our SM_DredgeBarge, its look and its collision (walked and shot on), or the stand-in box (seen only). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hull;

	/** The third stack's foot: it turns about this to fall in step 7. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> FallingStackPivot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FallingStack;

	/** The doubler plate it stood on: stays on the deck when it falls (the model only). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StackFoot;

	/** The deck's lights (shadowless): the work lamp on the house's fore face, then the work lamps; out as she dies. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UPointLightComponent>> DeckLamps;

	/** The wall lanterns' lights below the deck (shadowless, warm). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UPointLightComponent>> Lanterns;

	/** Instanced looks: rails, the stairs' treads and stringers, the vents (cold, their rims and slats, glowing, their fire),
	 *  the hall's wall panels and catwalk, her hose's sections, the stern's fire as she dies, and the soul coal raining off
	 *  her in the fall. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> RailLook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> StairLook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> VentCold;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> VentFrame;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> VentGlow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> VentFire;

	/** The vents' hoods under the deck seen from the hall (the model only): dark, or glowing with their vent. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> HoodCold;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> HoodGlow;

	/** The work lamps' brackets (iron) and their lanterns. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> WorkLampIron;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> WorkLampLanterns;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> HallWallLook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> CatwalkLook;

	/** The hand rails' iron (the catwalk's inner edge, the hatch stair's sides): rails and stanchions. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> HandrailLook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> HoseSections;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> SternFire;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> CoalRain;

	/** The stoker's radio set on his desk (Ned's call comes out of it). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Radio;

	/** The stoker's desk (the manifest lies on its top: GetOfficeSpot reads it). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Desk;

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

	// --- BargeBoardingSpots.cpp ---
	/** A point in her frame (BargeRules', Z from the keel) in the world, with KitLift. */
	FVector ToWorld(const FVector& Local) const;
	/** Her placed pose (before any list or drift), the frame the getters and the dying pose start from. */
	FTransform RestTransform() const;

	// --- BargeBoardingKit.cpp ---
	void BuildKit();
	UStaticMeshComponent* MakePiece(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look, const FVector& Center,
		const FVector& Size, const FRotator& Turn = FRotator::ZeroRotator, bool bWalkable = true);
	/** An unseen solid box (the hall's iron, a hidden plate): stops pawns and shots alike; any look is laid apart. */
	UBoxComponent* MakeWall(const TCHAR* Name, const FVector& Center, const FVector& Half, const FRotator& Turn = FRotator::ZeroRotator);
	/** A stair's unseen ramp (Width wide, its top face on the run's line: walked smoothly; its treads are looks). */
	UBoxComponent* MakeRamp(const TCHAR* Name, const BargeRules::FStairRun& Stair, float Width = BargeRules::StairWidth);
	UInstancedStaticMeshComponent* MakeInstanced(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look);
	void BuildDeck();
	/** The instanced looks that never change (rails, stairs, vents; the hall's through LayHallLooks), laid once
	 *  (OnConstruction and BeginPlay). */
	void LayStaticLooks();
	/** A vent's plate (cold or glowing) and its column's fire at Spot (her frame), as the vent and fire models or the
	 *  stand-in shapes lay them; its hood under the deck, and whether it has one (inside the hall's round). */
	FTransform VentPlate(const FVector& Spot) const;
	FTransform VentColumn(const FVector& Spot) const;
	static FTransform VentHood(const FVector& Spot);
	static bool HasHood(int32 Vent);

	// --- BargeBoardingProps.cpp ---
	/** A prop at its own size, its bounds' bottom middle at Foot, turned Yaw: with bSolid, its mesh's simple collision, or an
	 *  unseen box of its bounds where it has none. Without the mesh, a block of FallbackHalf in Look. */
	UStaticMeshComponent* MakeProp(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look, const FVector& Foot, float Yaw,
		const FVector& FallbackHalf, bool bSolid);
	/** The deck's cover (BargeRules::CoverPieces). */
	void BuildCover();
	/** Below the deck: the stoker's desk, safe and radio on the catwalk, the coal, the wall lanterns and their lights. */
	void BuildDressing();
	/** The deck's work lamps (BargeRules::WorkLamps): their lights, and the instanced looks LayWorkLamps lays. */
	void BuildWorkLamps();
	void LayWorkLamps();

	// --- BargeBoardingHall.cpp ---
	/** The furnace hall: floor, wall panels and lintels, ceiling, catwalk and its stairs, stokehold alcoves. */
	void BuildHall();
	/** The hatch's stair from the cross walk down onto the catwalk. */
	void BuildHatchStair();
	/** The hall's instanced looks (its walls, alcoves and lintels as their boxes stand; the catwalk's grating). */
	void LayHallLooks();

	// --- BargeBoardingDeck.cpp (and Pushes.cpp) ---
	void AdvancePushes(float DeltaSeconds);
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
	/** The deck fight's pushes (AdvancePushes): the step they were counted for, started, since the last began, mostly down. */
	int32 PushStep = INDEX_NONE;
	int32 PushesStarted = 1;
	float PushClock = 0.f;
	float MostlyDownClock = 0.f;
	/** The light view last shown (ShowLightsFor; none: show it again). */
	int32 LightView = INDEX_NONE;

	// Vents (and which looks were found: the kit's vent and fire models, else engine shapes).
	bool bVentModel = false;
	bool bFireModel = false;
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

	UPROPERTY()
	TArray<TObjectPtr<UBoxComponent>> RailWalls;

	/** The hall's solid iron with a look laid over it (wall panels, lintels, the alcoves' floors, walls and roofs). */
	UPROPERTY()
	TArray<TObjectPtr<UBoxComponent>> HallWalls;

	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> PlateParts;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> CoverParts;
};
