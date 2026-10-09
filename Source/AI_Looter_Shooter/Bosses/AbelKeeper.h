#pragma once

#include "CoreMinimal.h"
#include "Bosses/AbelPoses.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossTypes.h"
#include "Creatures/UnpaidCreature.h"
#include "Story/StoryCondition.h"
#include "AbelKeeper.generated.h"

class AAbelOnBoard;
class AKeeperLanternPost;
class UBossComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMissionRunner;
class UPointLightComponent;
class UStaticMeshComponent;
struct FFallRecoveryEvent;

/** What Abel is doing besides fighting as an Unpaid does (his own moments; None: the creature's brain has him). */
UENUM(BlueprintType)
enum class EAbelMove : uint8
{
	None,
	/** His fight's start: turned to the player, his lantern raised and flaring (AbelKeeperShow.cpp); any moment cuts it short. */
	Intro,
	/** Turned to the sunset, the lantern lowered: his coal open (phase one's grief, every 12 s for 3 s). */
	Grieve,
	/** The lantern's flare before the buckshot (1 s), then the shot. */
	Flare,
	Fire,
	/** Drifting out over the canyon at the bell (phase two), hanging there in the fog, dragged back by the lanterns. */
	DriftOut,
	InFog,
	DragBack,
	/** Walking off into the wind (phase three), then the dark saint's pull: a stun, his coal open. */
	WalkOff,
	Pulled,
	/** Staggered by crits on his coal (his boss's stagger): down on a knee facing the player, his coal still open. */
	Staggered,
	/** At zero: knelt, his coal sunk to an ember; the scene has him from there. */
	Kneel,
	Scene,
};

/**
 * Abel Ransom, the Keeper (Docs/Areas/RansomsRest.md, "The boss: Abel Ransom, the Keeper"; Main 6, "The Gravewind"): the
 * area's boss, an Unpaid in his Sunday keeper's coat with his ghost lantern and the spectral twin of his Ranchhand pump.
 * SK_Abel on the Unpaid's rig (Art/Models/Creatures/Abel.py) at 1.3, posed by AUnpaidCreature's code with his pose table
 * laid over it (AbelKeeperPose.cpp, AbelPoses): the fight's idle, the flare, the shot, the stock lunge, the turn to the
 * sunset, the kneel and the sit. Boss rank, 40 Unpaid tough (about 10,500 health at level 9), his fight his UBossComponent's
 * (AbelRules::MakePhases), his own moments its Custom events (AbelKeeperFight.cpp):
 *
 *  - "You brought them here": he fights as an Unpaid (the stock lunge up close) and fires spectral buckshot after a
 *    one-second lantern flare; two Unpaid rise through the deck every 25 s (at most 4); every 12 s he turns to the sunset
 *    for 3 s, and only then is his coal open: a gold crit spot (6 cm), which his lantern arm covers the rest of the time.
 *  - "The bell" (60%): the chapel bell tolls, he drifts out into the fog over the canyon where he can't be hurt, the deck's
 *    three lanterns go dark and 8 Unpaid rise in two waves. Each lantern relit (hold Interact 1.5 s) drags him a third of
 *    the way back; with all three he's on the deck again, stunned a moment, his coal open.
 *  - "Let me go" (25%): the Gravewind pours off the point in gusts toward the deck's open end (AbelKeeperWind.cpp); he
 *    tries to walk off into it and the dark saint pulls him back, stunning him 3 s with his coal open; between pulls he
 *    fights faster. A fall off the deck is fall recovery's (its outside rule), the wind taking the player down past the end.
 *  - At zero he kneels, his coal sinks to an ember, and the scene plays (Scenes/SitWithPa): he lights the Keeper's Lantern
 *    from his ghost light, its flame leaning north-east, and sits on his board, where AAbelOnBoard takes his place.
 *  - The player's death starts it all over: he goes home healed, his adds go, the lanterns are lit, the wall drops; he
 *    waits until the player walks back onto the deck. The fog wall is the Keeper's Gate's (ABossSeal, placed).
 *  - The show (AbelKeeperShow.cpp, AbelRules::MakeShow): his lantern raised as his bar sweeps in, shots from the fog while
 *    the lanterns are dark, a barrage in the wind, a knee when crits on his coal stagger him, his coal's flare as he falls;
 *    his Boss table's loot bursts from the deck's middle once he has sat with Ellis.
 *
 * He's in the world during Main 6 only (PresentWhen): hidden, still and ticking nothing otherwise. Until the Keeper's
 * Lantern hangs on the keeper's post (FightWhen) he walks the boards, unhurt by anything; then a player on the deck starts
 * his fight. He's fought once: a session loaded at Main 6's scene step finds the ending as the scene leaves it.
 */
UCLASS(Config = Game)
class AI_LOOTER_SHOOTER_API AAbelKeeper : public AUnpaidCreature
{
	GENERATED_BODY()

public:
	/** The tag missions find him by (Main 6's "Defeat Abel"), his boss id in the campaign, his mission and its steps (from 0). */
	static const FName BossTag;
	static const FName BossId;
	static const FName Mission;
	static constexpr int32 HangStep = 1;
	static constexpr int32 FightStep = 2;
	static constexpr int32 SceneStep = 3;

	/** His model and props (Art/Models/Creatures/Abel.py). */
	static const TCHAR* const ModelPath;
	static const TCHAR* const HatPath;
	static const TCHAR* const LanternPath;
	static const TCHAR* const PumpPath;

	AAbelKeeper();

	virtual void Tick(float DeltaSeconds) override;

	/** His coal is critical only while it's open (his grief, a pull's stun): then as an Unpaid's, by the shot's line. */
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	UBossComponent* GetBoss() const { return Boss; }

	// --- In the story (AbelKeeper.cpp) ---

	/** He's in the world now: Main 6 (PresentWhen), or forced by the console. */
	bool IsPresent() const { return bPresent; }

	/** Reads the story again: there or not, his fight able to start, the ending's state on a load past it. */
	void RefreshStory();

	/** In the world whatever the story says (the console, the tests), or back to the story's word. */
	void ForcePresent(bool bForce);

	/** His fight was won and the scene has sat him on his board (or the story is past it): he's gone from here for good. */
	bool IsEndingDone() const { return bEndingDone; }

	// --- His fight (AbelKeeperFight.cpp; each moment frame by frame, AbelKeeperMoves.cpp) ---

	EAbelMove GetMove() const { return Move; }
	float GetMoveTime() const { return MoveTime; }

	/** His coal is open to shots: grieving, stunned by a pull, or staggered. */
	bool IsCoalOpen() const { return Move == EAbelMove::Grieve || Move == EAbelMove::Pulled || Move == EAbelMove::Staggered; }

	/** Out over the canyon (drifting there, in the fog, or dragged back). */
	bool IsOutInFog() const { return Move == EAbelMove::DriftOut || Move == EAbelMove::InFog || Move == EAbelMove::DragBack; }

	/** His moments, as his phases' events start them (public for the console and the tests). False when he can't now. */
	bool Grieve();
	bool StartFlare();
	bool DriftOut();
	bool WalkOff();
	void Pull();

	/** The deck's lanterns: how many are lit, and how far back toward the deck they have dragged him (0-1). */
	int32 NumLitLanterns() const;
	float GetDragShare() const;
	const TArray<TObjectPtr<AKeeperLanternPost>>& GetLanternPosts() const { return LanternPosts; }

	/** Where he hangs in the fog over the canyon, and which way the sunset (the deck's open end) is from his spot (flat). */
	FVector GetFogSpot() const;
	FVector GetSunsetDirection() const;

	/** He's fighting faster between the dark saint's pulls (phase three). */
	bool IsHastened() const { return bHastened; }

	// --- The Gravewind (AbelKeeperWind.cpp) ---

	/** The wind blows (phase three). */
	bool IsWindBlowing() const { return bWindBlowing; }
	/** How hard it blows now (0 between gusts, 1 at a gust's height). */
	float GetGust() const { return GustNow; }
	void StartWind();
	void StopWind();
	/** A gust now, whatever its schedule (the console). */
	void ForceGust();
	/** Moves the wind on by DeltaSeconds on its fight's player (the tick does; the tests call it). */
	void TickWind(float DeltaSeconds);

	// --- His pose (AbelKeeperPose.cpp) ---

	/** The pose he's heading for now, and how far into the blend toward it (0-1). */
	EAbelPose GetPoseNow() const { return TargetPose; }
	float GetPoseBlend() const;

	/** Heads for NewPose over BlendSeconds (the scene's choice; in the fight his moments choose). */
	void SetPose(EAbelPose NewPose, float BlendSeconds);

	/** He's speaking: his jaw moves (6 to 10 degrees) while his lines play. */
	void SetSpeaking(bool bInSpeaking) { bSpeaking = bInSpeaking; }

	/** His lantern's light, from its glow (0) to its flare (1) or down to an ember (negative). */
	void SetLanternFlare(float Share);

	/** Where his lantern's globe is now (world): its light. */
	FVector GetLanternGlobe() const;

	// --- His ending (AbelKeeperMoves.cpp; the scene is Scenes/SitWithPa) ---

	/** The scene's hold on him: placed by it each frame, his pose its choice. */
	void BeginScene();
	void SetScenePlace(const FVector& Location, float Yaw);

	/** Where he sits on his board: AAbelOnBoard's place, his middle over it at his size; false without it. */
	bool GetBoardPlace(FVector& OutLocation, float& OutYaw) const;

	/** He sits on his board for good: AAbelOnBoard shows there and he's gone (the scene's end, played or skipped). */
	void SitOnBoard();

	/** The ending's end state without the scene (scenes off, a load past it): the lantern lit, him on his board. */
	void FinishEndingAtOnce();

	/** Back on the boards from his board, alive at his spot (the console, after the scene played alone). False once he's dead. */
	bool ComeBack();

	/** The Keeper's Lantern's post: the keeper's post among LanternPosts. */
	AKeeperLanternPost* GetKeepersPost() const;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBossComponent> Boss;

	/**
	 * His ghost lantern on the lantern bone, and its light at its globe (SOCKET_Light): shadowless, his one light. It shines
	 * on the ghost light's own lighting channel (AbelRules::GhostLightChannel), so it lights the deck, the biers, the posts
	 * and his adds round him and never his own body and props, a hand's width from it.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lantern;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> LanternLight;

	/** The spectral pump on the gun bone: the buckshot leaves its SOCKET_Muzzle. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Pump;

	/** The Gravewind's wisps over the deck (one instanced mesh, in the world's frame), drawn only while it blows. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Wisps;

	// --- Settings: the story ---

	/** He's in the world while this holds (Main 6). Empty: always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Story")
	FStoryCondition PresentWhen;

	/** His fight can start (a player on the deck starts it) while this holds: the lantern hung (Main 6 from its third step). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Story")
	FStoryCondition FightWhen;

	/** The ending is due: Main 6's scene step. A session loaded there finds the ending's end state (FinishEndingAtOnce). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Story")
	FStoryCondition EndingWhen;

	/** Once the lantern hangs, a player this near his spot (cm) starts the fight: anywhere on the deck. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Story", meta = (ClampMin = "0", Units = "cm"))
	float DeckRadius = 1400.f;

	// --- Settings: the deck ---

	/** The deck's three lantern posts (none set: those in the level tagged LanternPost_Deck, nearest first). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Deck")
	TArray<TObjectPtr<AKeeperLanternPost>> LanternPosts;

	/** Where he sits after the fight (none: the level's AAbelOnBoard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Deck")
	TObjectPtr<AAbelOnBoard> OnBoard;

	/** Where he hangs in the fog (world). Zero: FogDistance ahead of his spot toward the sunset, FogRise up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Deck")
	FVector FogSpot = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Deck", meta = (ClampMin = "0", Units = "cm"))
	float FogDistance = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Deck", meta = (Units = "cm"))
	float FogRise = 150.f;

	/** How far the deck's open end is from his spot toward the sunset (cm): his walk into the wind stops short of it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Deck", meta = (ClampMin = "100", Units = "cm"))
	float OpenEndDistance = 1000.f;

	// --- Settings: the fight ---

	/** The spectral buckshot (AbelRules::Buckshot): its pellets leave the pump's muzzle after his flare. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight")
	FBossVolley Buckshot;

	/** How long he grieves, flares, shows the shot, drifts out, is dragged back, walks off, is stunned (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "0.1", Units = "s"))
	float GrieveSeconds = AbelRules::GrieveSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "0.1", Units = "s"))
	float FlareSeconds = AbelRules::FlareSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "0.05", Units = "s"))
	float FireSeconds = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "0.5", Units = "s"))
	float DriftSeconds = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "0.2", Units = "s"))
	float DragBackSeconds = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "0.5", Units = "s"))
	float WalkOffSeconds = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "0.1", Units = "s"))
	float StunSeconds = AbelRules::PullStunSeconds;

	/** His walk into the wind (cm/s at size 1), and how far a pull drags him back (cm, in 0.4 s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "10", Units = "cm/s"))
	float WalkOffSpeed = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight", meta = (ClampMin = "0", Units = "cm"))
	float PullDistance = 450.f;

	/** How much faster he fights between pulls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight")
	FAbelPace Pace;

	/** The Gravewind's gusts and wisps (phase three). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight")
	FAbelGustRules Gust;

	/** His entrance, his shots from the fog, his barrage in the wind (AbelKeeperShow.cpp). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Fight")
	FAbelShowRules ShowRules;

	// --- Settings: the look ---

	/** His lantern's light: its glow, its flare before the buckshot, its ember once he kneels (candela), and its reach (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Look", meta = (ClampMin = "0"))
	float LanternCandela = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Look", meta = (ClampMin = "0"))
	float FlareCandela = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Look", meta = (ClampMin = "0"))
	float EmberCandela = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Look", meta = (ClampMin = "100", Units = "cm"))
	float LanternRadius = 750.f;

	/** His coal sunk to an ember at zero (M_Ghost's flare: -1 out, 0 as it burns). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Look", meta = (ClampMin = "-1", ClampMax = "0"))
	float EmberHeat = -0.65f;

	/**
	 * The lunge turns the pump on its bone so its butt leads (the pose table's); blending into it spins it half a turn. Off:
	 * the pump keeps its idle turn in the lunge (the art session's fallback if the spin looks wrong).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel|Look")
	bool bLungeFlipsPump = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual bool CanStartAttack() const override;
	virtual void GetExtraRigBones(TArray<FName>& OutBones) const override;
	virtual void LayerPose(float DeltaSeconds) override;
	virtual void UpdateDeathLook(float DeltaSeconds) override;

private:
	// --- In the story (AbelKeeper.cpp) ---
	void SetPresent(bool bInPresent);
	void HandleMissionsChanged();
	bool StoryHolds(const FStoryCondition& Condition) const;
	/** Puts the props on their bones as the model has them at rest. */
	void WearProps();
	/** The deck's lantern posts, found once and listened to. */
	void FindLanternPosts();

	// --- His fight (AbelKeeperFight.cpp; each moment frame by frame, AbelKeeperMoves.cpp) ---
	void HandleFightStarted();
	void HandleFightReset();
	void HandleFightWon();
	void HandlePhaseChanged(int32 NewPhase, int32 OldPhase);
	void HandleCustomEvent(FName EventName);
	void HandleLanternRelit(AKeeperLanternPost& Post);
	/** Starts one of his moments now (its clock from 0). */
	void BeginMove(EAbelMove NewMove);
	void EndMove();
	/** Moves his moment on: where he is, which way he faces, what he does at its end. */
	void TickMove(float DeltaSeconds);
	/** A moment is free to start: alive, fighting, not lunging or mid-attack, nothing else under way. */
	bool IsFreeForMoment() const;
	/** Held still for a moment: his brain's steering unused, his walk stopped (and while scripted, off the ground's rules). */
	void HoldStill(bool bScripted);
	void LetGo();
	void FaceYaw(float Yaw, float DeltaSeconds, float DegreesPerSecond);
	void ReleaseBuckshot();
	/** A moment his phase asked for while he was busy, started once he's free (or dropped after a few seconds). */
	void TryWantedMoments(float DeltaSeconds);
	/** Back at the player after a moment that held him. */
	void Rejoin();
	/** Where the lanterns have dragged him so far: between the fog and his spot. */
	FVector DragTarget() const;
	float SunsetYaw() const;
	/** The scene after the fight, once his kneel has settled (Scenes/SitWithPa), or its end state when scenes are off. */
	void PlayEnding();
	void RingTheBell();
	void ApplyPace(bool bFaster);
	/** Every lantern lit again at once, without a word (a reset, a win). */
	void LightAllLanterns();
	/** New adds rise through the boards: they fade in where they stand. */
	void RiseNewAdds();
	void Say(const FStoryLine& Line) const;

	// --- His entrance, his stagger, his fog shots, his barrage, his death's flare (AbelKeeperShow.cpp) ---
	void BeginIntro();
	void HandleStaggered(bool bStaggered);
	bool CanBeStaggered() const;
	void StartFogShot();
	void TickFogShot(float DeltaSeconds);
	/** The barrage's next shot (the first at the player, the second leading their run, the third trailing it). */
	void FireBarrageShot();
	void FlareCoalAtDeath();

	// --- The Gravewind (AbelKeeperWind.cpp) ---
	void DrawWisps(float DeltaSeconds);
	void ClearWisps();
	void HandleFallRecovered(const FFallRecoveryEvent& Event);
	/** Past the deck's open end: outside the level's playable area, or without one OpenEndDistance on from his spot. */
	bool IsPastOpenEnd(const FVector& Location) const;

	// --- His pose (AbelKeeperPose.cpp) ---
	/** The pose his state asks for now, and how quickly to blend to it. */
	EAbelPose WantedPose(float& OutBlendSeconds) const;
	/** Maps the table's bones to the rig's. */
	void BuildTableMap();

	// Story
	bool bPresent = true;
	bool bForcedPresent = false;
	bool bEndingDone = false;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;

	// Fight
	EAbelMove Move = EAbelMove::None;
	float MoveTime = 0.f;
	FVector MoveFrom = FVector::ZeroVector;
	FVector MoveTo = FVector::ZeroVector;
	bool bScriptedMove = false;
	bool bBuckshotWanted = false;
	float BuckshotWaited = 0.f;
	bool bGrieveWanted = false;
	float GrieveWaited = 0.f;
	bool bWalkOffWanted = false;
	float WalkOffWaited = 0.f;
	bool bHastened = false;
	/** Who his last fight was against (his boss forgets them as it's won): he kneels facing them. */
	TWeakObjectPtr<APawn> LastFighter;
	float KneelYaw = 0.f;
	bool bEndingAsked = false;
	/** His own pace, as his rank and level left it, for giving back. */
	bool bPaceCaptured = false;
	float OwnCooldown = 1.3f;
	float OwnWindup = 0.6f;
	float OwnChase = 470.f;
	float OwnLunge = 1400.f;
	float LanternShare = 0.f;
	TArray<TWeakObjectPtr<ACreatureBase>> KnownAdds;
	/** A fog shot's flare under way (s left), and the barrage's shots fired so far (0: a single shot). */
	float FogFlareLeft = 0.f;
	int32 BarrageFired = 0;

	// Wind
	bool bWindBlowing = false;
	float WindTime = 0.f;
	float GustNow = 0.f;
	float ForcedGustLeft = 0.f;
	bool bHobSpokeOfFall = false;
	FDelegateHandle FallHandle;
	struct FWisp
	{
		FVector Start = FVector::ZeroVector;
		float Along = 0.f;
		float Speed = 600.f;
		float Length = 100.f;
		float Width = 2.f;
	};
	TArray<FWisp> WispState;
	FRandomStream WispRandom;

	/** The wisps' glow: M_StylizedSurface's, as the fog wall's curtain (opaque, emissive: no translucency). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> WispBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WispMaterial;

	// Pose
	EAbelPose TargetPose = EAbelPose::Idle;
	float PoseBlendTime = 0.f;
	float PoseBlendSeconds = 0.f;
	/** The table's bones: their rig index, and the turn and shift they had as the blend set out (and have now). */
	TArray<int32> TableToRig;
	TArray<FQuat> FromOwn;
	TArray<FVector> FromShift;
	TArray<FQuat> NowOwn;
	TArray<FVector> NowShift;
	float FromLying = 0.f;
	float NowLying = 0.f;
	bool bSpeaking = false;
	float SpeakClock = 0.f;
	float JawNow = 0.f;
};
