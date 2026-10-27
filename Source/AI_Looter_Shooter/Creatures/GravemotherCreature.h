#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBodyHulls.h"
#include "Creatures/GravemotherFight.h"
#include "Creatures/GravemotherTypes.h"
#include "Creatures/SpiderCreature.h"
#include "GravemotherCreature.generated.h"

class AGravemotherClod;
class AGravemotherFurrow;
class AGravemotherMound;
class AGravemotherStone;
class AGroundCrack;
class UAudioComponent;
class UBossComponent;
class UBulletSubsystem;
struct FBulletLanding;
struct FBulletShot;

/** The Gravemother's numbers that never change in play (Docs/Areas/RansomsRest.md, "Enemies by rank" and Side 3). */
namespace Gravemother
{
	/** Her size against a brown spider's, as seen: her BodyScale is this over her rank's own size. */
	inline constexpr float Size = 1.8f;

	/** The Legendary rank's size (UCreatureRankSettings' default) her BodyScale is worked out against. */
	inline constexpr float RankSize = 1.4f;

	/** Her brood: spiderlings, a brown spider at this size with this share of its health (60 at level 1), Basic. */
	inline constexpr float SpiderlingSize = 0.45f;
	// 60 health at level 1, as tuned: 0.2 of the spider's 300 before spiders were softened to 230 (2026-10-22).
	inline constexpr float SpiderlingHealthShare = 60.f / 230.f;

	/** Spiderlings one call brings, and the shares of her health at which she calls, each once a life: 66%, then 33%. */
	inline constexpr int32 BroodSize = 4;
	inline constexpr int32 NumBroodCalls = 2;
	inline constexpr float BroodCallShare(int32 Call) { return Call <= 0 ? 0.66f : 0.33f; }

	/** Her lair's id (AEncounterSpawner::SpawnerId and LegendaryId), and the tag on the den's place (Side 3's "Enter the den"). */
	inline const FName LairId(TEXT("Gravemother"));
	inline const FName DenPlaceTag(TEXT("Place_Den"));
}

/**
 * The Gravemother (Docs/Areas/RansomsRest.md, Side 3 and "Enemies by rank"): Ransom's Rest's Legendary monster, a brown
 * spider grown huge in the Sink's den, at Gravemother::Size (1.8x) a spider's size, Legendary (her name in orange on her
 * tag), twelve times a spider's health. Her fight is "Barrow", the user's pick (2026-10-15, built 2026-10-20; the sheets in
 * Saved/Concepts/Gravemother/Barrow_*.png): she wears a toppled granite headstone bound to her abdomen by roots
 * (AGravemotherStone: its own health, three states, cracking off), and her moves tear up the grave earth of her den:
 *  - the Furrow (GravemotherCreatureFurrow.cpp, AGravemotherFurrow): with her target 5 to 16 m off and her way clear, a 2 s
 *    tell, head low and front legs in the floor while the ground heaves and splits along her line toward them; then she
 *    ploughs it head-low, running down whoever is in it, and throws up a berm each side (cover for 20 s). Never off a drop
 *    or through a wall. Missed, she's slow to rise (the window).
 *  - the Grave Clod (GravemotherCreatureClod.cpp, AGravemotherClod): at a player 7 to 22 m off in her fight, her front legs
 *    dig in and lift a clod overhead (1.2 s), then she lobs it (1.5 s to 20 m, its shadow showing where): it bursts and
 *    leaves a rubble pile (cover, 10 s). Shot in her grip, it bursts on her and stuns her 2 s.
 *  - Unearthed (GravemotherCreatureUnearthed.cpp, AGravemotherMound): at 66% and 33% of her health she rears, screams and
 *    slams the floor: four mounds bulge for 1 s, then spiderlings (the brown spider at 0.45x) dig out; a mound shot while
 *    it bulges kills its spiderling.
 *  - the Gravequake for whoever hugs her (from her second phase), her bite, her roar: as before.
 * Crits (GravemotherCreatureStone.cpp): her head always; her abdomen only on its soft rear (under the stone's back edge)
 * while the stone stands, all of it once the stone is off. Shots on the stone hurt the stone, not her. Broken early, it
 * staggers her (her boss's stagger); at her 33% it cracks off by itself, and from then she runs a quarter faster.
 * Her hide shrugs off (GravemotherCreatureShrug.cpp; the user's calls, 2026-10-27) every hit while the stone stands (it must be
 * shot off first: it glows as the target) and every hit from a shooter off her hunting ground (the Sink's rim), with a
 * glancing look and sound, no number, a callout on her bar and, after a few, Hob's hint. Her health is 1.6x and her hits
 * 1.25x the first tuning's (GravemotherTuning, cvars Looter.Gravemother.*).
 * Her lair (AEncounterSpawner with a LegendaryId) brings her, and back only on an arrival 20 minutes of play after her last
 * death. Her boss fight (UBossComponent, GravemotherFight) gives her the bar, phases, staggers (she reels:
 * GravemotherCreatureMoves.cpp), her brood as adds, the loot shower and the stand-down; her poses for her own moves lay over
 * the spider's (GravemotherCreaturePose.cpp). Until her own model (SK_Gravemother) is imported she's SK_Spider in a pale
 * hide, and every piece of her fight has its stand-in (GravemotherAssets).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AGravemotherCreature : public ASpiderCreature
{
	GENERATED_BODY()

public:
	AGravemotherCreature();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetActorHiddenInGame(bool bNewHidden) override;
	/** Every hit on her comes here first: one her hide shrugs off (ShrugFor) does nothing but its glancing look (GravemotherCreatureShrug.cpp). */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator,
		AActor* DamageCauser) override;

	/** Her abdomen's bone: the stone rides it, its soft rear is a crit. */
	static const FName AbdomenBone;

	// --- Her brood: Unearthed (GravemotherCreatureUnearthed.cpp) ---

	/** How a spiderling starts: a Basic brown spider at Gravemother::SpiderlingSize with its share of health, at Level (0: its area's band). */
	static FRuntimeSpawn MakeSpiderlingSpawn(int32 Level = 0);

	/** Spawns a spiderling standing on Feet, facing Yaw: gone for good once killed, named for what it is, begun. Null when it couldn't. */
	static ASpiderCreature* SpawnSpiderling(UWorld* World, const FVector& Feet, float Yaw, int32 Level = 0);

	/** What a spiderling's tag calls it. */
	static FText SpiderlingName();

	/** Raises one call's mounds round her now (the console, the tests; her health's calls come through her roar). How many. */
	int32 CallBrood();

	/** Her calls this life (two at most), and those her roar hasn't raised yet. */
	int32 GetBroodCallsMade() const { return BroodCallsMade; }
	int32 GetPendingUnearths() const { return PendingUnearths; }

	/** Her brood alive now, and her mounds not yet gone (bulging, burst or caved). */
	TArray<ASpiderCreature*> GetBrood() const;
	TArray<AGravemotherMound*> GetMounds() const;

	/** A mound of hers bursts: its spiderling claws out (her brood, her boss's add). */
	ASpiderCreature* RaiseFromMound(AGravemotherMound& Mound);
	/** A mound of hers was shot while it bulged: one less. */
	void NoteMoundPopped(AGravemotherMound& Mound, AController* By);

	// --- Her Furrow (GravemotherCreatureFurrow.cpp) ---

	EGravemotherFurrow GetFurrowState() const { return FurrowState; }
	/** Its cooldown is done: the next time her target is far enough off and her way to them clear, she ploughs. */
	bool IsFurrowReady() const { return FurrowCooldownLeft <= 0.f; }
	/** Readies it at once (the tests, the console). */
	void ReadyFurrow();
	/** She has taken her aim on this furrow: its line is fixed and the ground heaving along it. */
	bool HasTakenAim() const { return bAimTaken; }
	/** The line this furrow runs (world): from where she stood as she took aim, along its direction, this far. */
	FVector GetFurrowStart() const { return FurrowStart; }
	FVector GetFurrowDirection() const { return FurrowDirection; }
	float GetFurrowLength() const { return FurrowLength; }
	/** How far along it the ground has split (cm). */
	float GetHeaveFront() const { return HeaveFront; }
	/** This furrow ran its target down (it hits once). */
	bool HasFurrowHit() const { return bFurrowHit; }
	/** This furrow's ground, while it's hers (it stands its berms' time after). */
	AGravemotherFurrow* GetFurrow() const { return Furrow.Get(); }
	/** Her plough's speed now (cm/s: her size, her pace). */
	float GetPloughSpeed() const;

	// --- Her Grave Clod (GravemotherCreatureClod.cpp) ---

	bool IsClodReady() const { return ClodCooldownLeft <= 0.f; }
	void ReadyClod() { ClodCooldownLeft = 0.f; LobCheckedAt = -1.f; }
	/** The clod in her grip, and the last one she threw (while it flies). */
	AGravemotherClod* GetHeldClod() const { return HeldClod.Get(); }
	AGravemotherClod* GetThrownClod() const { return ThrownClod.Get(); }
	/** Stunned by a clod burst on her (her boss's stagger, with stars over her head). */
	bool IsStunned() const { return bStunned; }
	/** The clod in her grip was shot (it calls this): it burst on her; she's stunned. */
	void CounterClod(AController* By);

	// --- Her headstone (GravemotherCreatureStone.cpp) ---

	AGravemotherStone* GetStone() const { return Stone.Get(); }
	bool IsStoneOn() const;
	/** Whether a point on her abdomen (world) is its soft rear (her physics asset's round zone there, or its far end). */
	bool IsSoftRear(const FVector& WorldPoint) const;
	/** The stone tells her of a hit on it, of a new crack, and of its breaking (bByHerThird: at her 33%). */
	void NoteStoneHit(AController* By);
	void OnStoneCracked(EGravemotherStone NewState);
	void OnStoneBroken(bool bByHerThird);
	/** Her head always; her abdomen's soft rear while the stone stands, all of it once it's off. */
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	// --- Her hide shrugging hits off (GravemotherCreatureShrug.cpp; the user's calls, 2026-10-27) ---

	/**
	 * Why a hit from By would do her no harm now: By shoots from off her hunting ground (the Sink's rim: OffGround), or her
	 * headstone still stands (Stone); None: it hurts. Off-ground first (it's the one to tell a rim shooter). Each rule has its
	 * cvar (GravemotherTuning).
	 */
	EGravemotherShrug ShrugFor(const AController* By) const;
	/** A hit shrugged off for Why (her TakeDamage; her stone's for a rim shot on it): she turns on By as a hit would turn
	 *  her, her bar calls it out, the stone glows as the target (Stone), and after a few Hob says what to do. */
	void NoteShrug(EGravemotherShrug Why, AController* By);
	/** The glancing whine and thud of a shrugged hit at Where (not twice in a moment). */
	void PlayShrugSound(const FVector& Where, EGravemotherShrug Why);
	/** A rim shot shrugged off, as the shooter up there sees and hears it: the glance heard on the rim (2D), and she rears
	 *  and screeches up at them (not too often). Her bar is hidden from up there (no fight), so this is the feedback. */
	void AnswerRimShot(const AController* By);
	/** Hits shrugged off for Why since she came (the tests), and whether Hob has hinted at it. */
	int32 GetShrugCount(EGravemotherShrug Why) const;
	bool WasHinted(EGravemotherShrug Why) const;

	// --- Her boss fight (GravemotherCreatureMoves.cpp) ---

	UBossComponent* GetBoss() const { return Boss; }
	/** Which of her attacks she's at (or was at last). */
	EGravemotherMove GetMove() const { return Move; }
	/** Her next attack is a roar (her fight's start, a later phase, her brood's call). */
	bool IsRoarWanted() const { return bRoarWanted; }
	/** She reels from a stagger (her boss's), sunk on her legs. */
	bool IsReeling() const { return bReeling; }
	/** The phase her pace is set for, and the pace. */
	int32 GetPacePhase() const { return PacePhase; }
	const GravemotherFight::FPace& GetPace() const { return Pace; }
	/** Sets her pace for a phase (her boss's phase changes do). */
	void ApplyPace(int32 Phase);
	/** Readies her quake at once (the tests, the console). */
	void ReadyQuake() { QuakeCooldownLeft = 0.f; }
	bool IsQuakeReady() const { return QuakeCooldownLeft <= 0.f; }
	/** The ring of cracks spreading under her through a quake's wind-up. */
	AGroundCrack* GetQuakeCrack() const { return QuakeCrack.Get(); }
	/** Whether a player at Where would be caught by her quake's burst (on her level, inside its ring). */
	bool IsInQuake(const FVector& Where) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBossComponent> Boss;

	// --- Settings (GravemotherTypes.h; distances "at a spider's size" grow with hers) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother")
	FGravemotherFurrowSettings Furrows;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother")
	FGravemotherClodSettings Clods;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother")
	FGravemotherUnearthSettings Unearth;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother")
	FGravemotherStoneSettings StoneSettings;

	/** Her roar: rearing and screaming this long, then the slam, then up again (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Roar", meta = (ClampMin = "0.1", Units = "s"))
	float RoarWindup = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Roar", meta = (ClampMin = "0.1", Units = "s"))
	float RoarRecovery = 0.6f;

	/** The Gravequake: she quakes at a target this near (cm, at a spider's size), and its burst reaches this far round her. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Quake", meta = (ClampMin = "50", Units = "cm"))
	float QuakeRange = 280.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Quake", meta = (ClampMin = "50", Units = "cm"))
	float QuakeRadius = 300.f;
	/** Its wind-up (the ring of cracks spreading out to its reach: the time to get out of it) and its recovery (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Quake", meta = (ClampMin = "0.2", Units = "s"))
	float QuakeWindup = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Quake", meta = (ClampMin = "0.1", Units = "s"))
	float QuakeRecovery = 0.9f;
	/** Its burst hits this many times as hard as her bite, damage and shove. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravemother|Quake", meta = (ClampMin = "0"))
	float QuakeStrength = 1.5f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;
	virtual bool CanStartAttack() const override;
	virtual float GetAttackStartRange() const override;
	virtual bool TracksTargetInWindup() const override;
	/** Her own moves' poses over the spider's (GravemotherCreaturePose.cpp). */
	virtual bool GetPoseOverride(FSpiderPoseOverride& Out) const override;

private:
	// --- The Furrow (GravemotherCreatureFurrow.cpp) ---
	bool WantsToFurrow() const;
	/** Her way to her target is clear for a furrow (looked at a few times a second, not every frame). */
	bool IsLaneClear() const;
	/** How far a furrow along Direction can run, up to Wanted (cm): short of a wall, a drop or a rise she couldn't walk. */
	float MeasureLane(const FVector& Direction, float Wanted) const;
	void BeginFurrow();
	/** Fixes the line: toward her target as they stand now, its length measured, its ground laid. */
	void TakeAim();
	void BeginPlough();
	void TickFurrow(float DeltaSeconds);
	void TickPlough(float DeltaSeconds);
	/** Runs down her target if her body passed within reach of them going From To (once a furrow). Whether it did now. */
	bool TryFurrowHit(const FVector& From, const FVector& To);
	/** The plough is over: she slams down where she is. */
	void EndPlough();
	/** Back to walking and biting, the cooldown started (when there was a furrow), its ground let go. */
	void FinishFurrow();
	/** Her feet: the ground under her middle. */
	FVector GetFeet() const;
	/** Grave dirt thrown up at Where, Strength 0 to 1 (UGravemotherFX's earth, or the weapons' grave dirt). */
	void KickDirt(const FVector& Where, float Strength);

	// --- The Grave Clod (GravemotherCreatureClod.cpp) ---
	bool WantsToClod() const;
	/** Her lob's way to her target is clear (sampled a few times a second). */
	bool IsLobClear() const;
	/** Where the clod sits in her grip now: between her front feet, a little over them. */
	FVector GripSpot() const;
	void BeginClod();
	void TickClod(float DeltaSeconds);
	void ThrowClod();
	/** Her grip lost (a stagger, her death, the attack cut short): the clod falls. */
	void DropClod();

	// --- Unearthed (GravemotherCreatureUnearthed.cpp) ---
	/** Raises Calls calls' mounds round her now, cracks racing out to them; how many. */
	int32 RaiseMounds(int32 Calls);
	/** A call her roar hasn't raised in time raises its mounds anyway. */
	void TickUnearth(float DeltaSeconds);
	/** Caves in every mound still bulging (her death, her fight over). */
	void CollapseMounds();
	void PruneBrood();

	// --- The stone (GravemotherCreatureStone.cpp) ---
	/** Her stone, made or set whole again for a new life. */
	void ResetStone();
	/** Where the stone sits on her abdomen bone (ART's placement, or one worked out from her abdomen's hull). */
	FTransform StonePlacement() const;
	/** Reads her abdomen's hit zones from her physics asset (the soft rear's). */
	void ReadAbdomenZones();

	// --- Her boss fight's moves (GravemotherCreatureMoves.cpp) ---
	bool WantsToQuake() const;
	void BeginRoar();
	void RoarPeak();
	void BeginQuake();
	void QuakeSlam();
	void TickMoves(float DeltaSeconds);
	void BeginReel();
	void EndReel();
	/** Sunk on her legs, trembling (the reel, a missed plough's slam): her body pushed down as a hit pushes it, every frame. */
	void Sink();
	void HandleFightStarted();
	void HandleFightReset();
	void HandlePhaseChanged(int32 NewPhase, int32 OldPhase);
	void HandleStaggered(bool bStaggered);
	void ShakeFrom(const FVector& Where, float Strength, float Seconds) const;
	void KickDirtRing(const FVector& Center, float Radius, int32 Count, float Strength);
	/** Her bar's thin stone bar, as the stone stands. */
	void UpdateStoneBar();

	// --- Her poses and her stars (GravemotherCreaturePose.cpp) ---
	/** Stars wheeling over her head while she's stunned. */
	void DrawStunStars();

	// --- Shrugged hits (GravemotherCreatureShrug.cpp) ---
	/** The playtest's health and hit scales as the cvars stand now, over the class's defaults (before her rank is applied). */
	void ApplyTuning();
	/** Listens to bullets landing on her, so a shot her hide will shrug off gets no flesh hit look (her own instead). */
	void BindShrugLook(bool bOn);
	void NoteBulletLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing);
	/** A shrugged hit's look at Where: a pale glint, a glancing spark or two, dried grave mud puffing off her hide. */
	void PlayShrugLook(const FVector& Where, const FVector& Normal, const FVector& ShotDirection, float Scale = 1.f);
	/** How big a shrugged hit's look is drawn for By: 1 near, growing with By's distance so it still reads from the rim. */
	float ShrugLookScale(const AController* By, const FVector& Where) const;
	/** Hob's line for Why, once, after a few. */
	void MaybeHint(EGravemotherShrug Why, const AController* By);

	// The Furrow
	EGravemotherFurrow FurrowState = EGravemotherFurrow::None;
	float FurrowCooldownLeft = 0.f;
	/** Her own clock, which runs as she ticks (the lane's and the lob's looks are timed by it). */
	float Clock = 0.f;
	mutable float LaneCheckedAt = -1.f;
	mutable bool bLaneClear = false;
	bool bAimTaken = false;
	bool bFurrowHit = false;
	bool bPloughRan = false;
	/** Her plough's first frame has set her going (its time counts from the next: TickPlough). */
	bool bPloughSetOff = false;
	FVector FurrowStart = FVector::ZeroVector;
	FVector FurrowDirection = FVector::ForwardVector;
	float FurrowLength = 0.f;
	float HeaveFront = 0.f;
	float PloughTime = 0.f;
	float PloughSeconds = 0.f;
	FVector PloughLast = FVector::ZeroVector;
	float ShakeIn = 0.f;
	TWeakObjectPtr<AGravemotherFurrow> Furrow;
	TWeakObjectPtr<UAudioComponent> HeaveSound;
	TWeakObjectPtr<UAudioComponent> PloughSound;
	/** Her bite's timing, which her moves' take the place of while she's at them. */
	float BiteWindup = 0.55f;
	float BiteRecovery = 0.45f;

	// The Grave Clod
	float ClodCooldownLeft = 0.f;
	mutable float LobCheckedAt = -1.f;
	mutable bool bLobClear = false;
	bool bClodTorn = false;
	bool bClodCountered = false;
	bool bStunned = false;
	float StunClock = 0.f;
	/** Her own reel's clock when her boss can't stagger her (not fighting): seconds left. */
	float OwnReelLeft = 0.f;
	float DigDirtIn = 0.f;
	float SiftIn = 0.f;
	TWeakObjectPtr<AGravemotherClod> HeldClod;
	TWeakObjectPtr<AGravemotherClod> ThrownClod;
	TWeakObjectPtr<UAudioComponent> DigSound;

	// Unearthed
	int32 BroodCallsMade = 0;
	int32 PendingUnearths = 0;
	float PendingFor = 0.f;
	TArray<TWeakObjectPtr<ASpiderCreature>> Brood;
	TArray<TWeakObjectPtr<AGravemotherMound>> Mounds;
	/** The cracks racing out to the mounds (the decal's stand-in). */
	TArray<TWeakObjectPtr<AGroundCrack>> MoundCracks;

	// The stone
	TWeakObjectPtr<AGravemotherStone> Stone;
	/** Her abdomen's hit zones (bone space), read once: its round shapes are the soft rear. */
	FCreatureBodyHulls AbdomenZones;
	TArray<int32> RearZones;
	int32 AbdomenHull = INDEX_NONE;
	bool bZonesRead = false;

	// Her boss fight
	EGravemotherMove Move = EGravemotherMove::Bite;
	bool bRoarWanted = false;
	float QuakeCooldownLeft = 0.f;
	TWeakObjectPtr<AGroundCrack> QuakeCrack;
	bool bReeling = false;
	float SinkClock = 0.f;
	int32 PacePhase = 0;
	GravemotherFight::FPace Pace;
	/** Her own furrow cooldown and chase, as her class (or the level) gave them, which her pace scales. */
	bool bOwnPaceCaptured = false;
	float OwnFurrowCooldown = 8.f;
	float OwnChaseSpeed = 480.f;

	// Shrugged hits
	int32 ShrugCounts[static_cast<int32>(EGravemotherShrug::Count)] = {};
	uint8 HintedKinds = 0;
	double LastShrugSound = -1.0;
	double LastShrugRicochet = -1.0;
	double LastRimScreech = -1.0;
	FVector LastShrugSoundAt = FVector::ZeroVector;
	uint64 LastShrugFlashFrame = 0;
	double LastShrugCallout = -1.0;
	TWeakObjectPtr<UBulletSubsystem> Bullets;
	FDelegateHandle LandingHandle;
};
