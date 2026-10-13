#pragma once

#include "CoreMinimal.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/ForemanFight.h"
#include "ForemanCreature.generated.h"

class AForemanFuseStick;
class AForemanRockFall;
class AForemanTimberProp;
class APostProcessVolume;
class ULightComponent;
class UBossComponent;

/** Which of his moves the Foreman is at (each is the creature's one attack, wound up, struck and recovered, with its own timing). */
UENUM(BlueprintType)
enum class EForemanMove : uint8
{
	/** The pick handle brought down on whoever stands in front of him: the dead miners' swing, his size. */
	Swing,
	/** His fight's start: turned from the fall, the pick handle raised and slammed on the floor, his line. */
	Intro,
	/** A fuse stick lit in his raised fist (sparks and a hiss, his coal open), then lobbed. */
	Fuse,
	/** The pick handle hammered into a timber prop (or the floor), and the roof answers. */
	Roof,
};

/**
 * The Foreman, Cyrus Teague (Docs/Areas/RansomsRest_Expansion.md, section 6 "The Providence" and Back Pay): Ransom's Rest's
 * second Legendary monster, who brought the stope's roof down on eleven striking men to save the company a quarter's wages
 * and has dug at the fall every night since. A dead miner (ADeadMinerCreature) at 1.5x in a foreman's long coat, a bowler
 * with a carbide lamp, a pick handle and a satchel of fuse sticks; Legendary ("The Foreman" in orange on his tag; Soulfed's
 * stats, the Legendary loot table), at a set level 7 (his kind's 5 in the area's levels and Soulfed's 2); about 3,400
 * health at level 7, tuned to a two or three minute fight with his shift. His Ledger page is his own.
 *
 * He waits in his stope digging at the fall (his work), until a player comes into the stope or shoots him; then his boss
 * fight (UBossComponent; ForemanFight has its phases, pace and show):
 *  - The entrance (ForemanCreature.cpp): he turns from the fall, raises the pick handle and slams it on the floor (dust off
 *    the roof, the view jolted) as his bar sweeps in: "Nobody leaves till the drift's cleared!"
 *  - Fire in the hole (ForemanCreatureFuse.cpp), all fight long at a player who keeps off: a fuse lit in his raised fist for
 *    2 s (sparks, the hiss, his coal open), then lobbed; it lands, burns a moment over a ring of cracks and bursts 6 m wide.
 *    Up close, the pick handle.
 *  - The roof (ForemanCreatureRoof.cpp), every 18 s or so below 66%: the pick handle hammered into the nearest timber prop
 *    (or the floor); dust sifts on four marked spots (the struck prop, the player's spot, the prop they hide behind, one
 *    more near them) for 2 s, then rock falls there and the props it lands on splinter. His coal is open 3 s after.
 *  - The shift (ForemanCreatureShift.cpp), at 66% and 33%: three dead miners rise out of the fall's rubble, his adds, who
 *    crumble with him. "Back to work!"
 *  - The dark, at 50%: the stope's lanterns (actors tagged StopeLantern) blow out for 12 s; his lamp, the miners' lamps and
 *    their coals are what's left, and the exposure lifts so it reads dark, not black. "I can dig them out. I can still dig
 *    them out."
 *  - His coal (1.5x crits, the game's rule) is open only while he lights a fuse, for 3 s after a roof strike, and while
 *    staggered; crits on it close together stagger him. The rest of the time his coat covers it: shots there are plain.
 *  - His death: he kneels where he falls, his lamp guttering, and says his last word, "...Tell the company it was the
 *    roof," before he fades; the stope's lanterns come back, and his loot bursts out of him after.
 * His lair (an AEncounterSpawner, or the shift's own ADeadMinerShiftSpawner, with LegendaryId Foreman) brings him, and his
 * vigil stone calls him back. A player gone from his stope a while stands his fight down: he heals and goes back to the fall.
 *
 * His parts are found by name once made, else the dead miners' (or nothing), each where Intermediate/ForemanModel/
 * Foreman_parts.json puts it: SK_Foreman (38 bones, the Unpaid's among them, with the pick bone under hand_r),
 * SM_ForemanBowler on the hat bone (its SOCKET_Lamp for the lamp), SM_ForemanLamp, SM_ForemanPickHandle on the pick bone
 * (else in his right fist as modelled), SM_ForemanSatchel modelled in place on his chest (its pivot at spine_02's head),
 * SM_ForemanFuseStick lit in his left fist (its pivot at its middle, fuse along +Z to its SOCKET_Fuse).
 */
UCLASS(Config = Game)
class AI_LOOTER_SHOOTER_API AForemanCreature : public ADeadMinerCreature
{
	GENERATED_BODY()

public:
	AForemanCreature();

	virtual void Tick(float DeltaSeconds) override;

	/** His coal is critical only while it's open (a fuse lit, after a roof strike, staggered): then as an Unpaid's. */
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	static const TCHAR* const ModelPath;
	static const TCHAR* const BowlerPath;
	static const TCHAR* const LampPath;
	static const TCHAR* const PickHandlePath;
	static const TCHAR* const SatchelPath;

	/** His voice's lines (CreatureVoiceBarkLines.cpp's own for him). */
	static const FName BarkKind;

	UBossComponent* GetBoss() const { return Boss; }

	// --- His fight (ForemanCreature.cpp) ---

	EForemanMove GetMove() const { return Move; }

	/** His coal is open to crits now. */
	bool IsCoalOpen() const;

	/** His next attack is his entrance. */
	bool IsIntroWanted() const { return bIntroWanted; }

	/** The phase his pace is set for, and the pace. */
	int32 GetPacePhase() const { return PacePhase; }
	const ForemanFight::FPace& GetPace() const { return Pace; }
	void ApplyPace(int32 Phase);

	// --- Fuse sticks (ForemanCreatureFuse.cpp) ---

	bool IsFuseReady() const { return FuseCooldownLeft <= 0.f; }
	void ReadyFuse() { FuseCooldownLeft = 0.f; }
	bool IsLightingFuse() const;
	/** The stick lit in his fist, while he holds it. */
	AForemanFuseStick* GetHeldStick() const { return HeldStick.Get(); }
	int32 GetFusesThrown() const { return FusesThrown; }

	// --- The roof (ForemanCreatureRoof.cpp) ---

	bool IsRoofReady() const { return RoofCooldownLeft <= 0.f; }
	void ReadyRoof() { RoofCooldownLeft = 0.f; }
	int32 GetRoofStrikes() const { return RoofStrikes; }
	/** The rock coming down from his last strike (still in the level). */
	TArray<AForemanRockFall*> GetRockFalls() const;
	/** The prop his roof strike is aimed at now (within his reach), if any. */
	AForemanTimberProp* GetStruckProp() const { return StruckProp.Get(); }
	/** His stope's timber props: those within ArenaRadius of his spot (standing only if asked). */
	TArray<AForemanTimberProp*> GetProps(bool bStandingOnly) const;

	// --- The shift and the dark (ForemanCreatureShift.cpp) ---

	/** Calls his shift now, as his health does at 66% and 33%: up to three dead miners out of the rubble. How many rose. */
	int32 CallShift();
	int32 GetShiftsCalled() const { return ShiftsCalled; }
	/** His shift alive now. */
	TArray<ADeadMinerCreature*> GetShift() const;

	/** Blows out the stope's lanterns for DarkSeconds (his 50%), or lights them again (bAtOnce: no flicker). */
	void BeginDark();
	void EndDark(bool bAtOnce);
	bool IsDark() const { return DarkLeft > 0.f; }
	float GetDarkLeft() const { return DarkLeft; }
	/** The stope's lights the dark put out (the tests). */
	int32 NumDarkenedLights() const { return DarkenedLights.Num(); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBossComponent> Boss;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Satchel;

	// --- Settings: every distance at an Unpaid's size, growing with his ---

	/** His stope: its props and the roof's spots within this of his spot (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman", meta = (ClampMin = "500", Units = "cm"))
	float ArenaRadius = 1500.f;

	/**
	 * A player this near his spot (cm, flat) and on his level (EngageRise up or down) starts his fight: inside the stope, past
	 * the barricade. His own check rather than his boss's, which is flat only: the bench's surface lies over the mine.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman", meta = (ClampMin = "0", Units = "cm"))
	float EngageRadius = 1300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman", meta = (ClampMin = "100", Units = "cm"))
	float EngageRise = 400.f;

	/** His entrance: the pick handle raised, then slammed (s), then up again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Intro", meta = (ClampMin = "0.1", Units = "s"))
	float IntroWindup = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Intro", meta = (ClampMin = "0.1", Units = "s"))
	float IntroRecovery = 0.8f;

	/** He lobs a fuse at a player between these distances (cm); nearer, the pick handle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Fuse", meta = (ClampMin = "0", Units = "cm"))
	float FuseMinDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Fuse", meta = (ClampMin = "100", Units = "cm"))
	float FuseMaxDistance = 1500.f;

	/** The fuse lit in his fist (the tell, his coal open), and his recovery after the lob (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Fuse", meta = (ClampMin = "0.2", Units = "s"))
	float FuseLightSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Fuse", meta = (ClampMin = "0.1", Units = "s"))
	float FuseRecovery = 0.6f;

	/** The lob's flight (s, a longer throw a little longer), the floor's fuse, and the burst: its reach and its strength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Fuse", meta = (ClampMin = "0.3", Units = "s"))
	float FuseFlightSeconds = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Fuse", meta = (ClampMin = "0.1", Units = "s"))
	float FuseGroundSeconds = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Fuse", meta = (ClampMin = "50", Units = "cm"))
	float FuseRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Fuse", meta = (ClampMin = "0"))
	float FuseStrength = 1.5f;

	/** The roof: his wind-up and recovery, the dust's time, the rock's reach and strength, the props he can strike. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "0.2", Units = "s"))
	float RoofWindup = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "0.1", Units = "s"))
	float RoofRecovery = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "0.5", Units = "s"))
	float RoofDustSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "50", Units = "cm"))
	float RoofRockRadius = 170.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "0"))
	float RoofStrength = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "50", Units = "cm"))
	float RoofPropReach = 450.f;

	/** How many spots, how far apart, and how far from the player the rest are put (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "1", ClampMax = "8"))
	int32 RoofSpots = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "50", Units = "cm"))
	float RoofSpotSpacing = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "50", Units = "cm"))
	float RoofFillDistance = 340.f;

	/** His coal stays open this long after a roof strike (s), and the cover the player uses counts within this of them (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "0", Units = "s"))
	float RoofCoalSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Roof", meta = (ClampMin = "0", Units = "cm"))
	float CoverReach = 500.f;

	/** The shift rises within this of the rubble (cm), on his level of ground. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Shift", meta = (ClampMin = "100", Units = "cm"))
	float ShiftRadius = 450.f;

	/** The dark: how long (s), and how much the view's exposure lifts meanwhile (stops; 0: none). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Dark", meta = (ClampMin = "1", Units = "s"))
	float DarkSeconds = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Dark", meta = (ClampMin = "0", ClampMax = "3"))
	float DarkExposureLift = 1.f;

	/** His death: knelt this long with his last word before he fades, over FadeSeconds (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Death", meta = (ClampMin = "0", Units = "s"))
	float KneelSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foreman|Death", meta = (ClampMin = "0.1", Units = "s"))
	float FadeSeconds = 2.5f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual bool CanStartAttack() const override;
	virtual float GetAttackStartRange() const override;
	virtual bool TracksTargetInWindup() const override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void UpdateDeathLook(float DeltaSeconds) override;
	virtual void WantedArms(FMinerArms& Out) const override;
	virtual bool WantsToWork() const override;
	virtual bool UsesLampBudget() const override { return false; }
	virtual float WantedLampFlare() const override;
	virtual bool HoldsPoseInDeath() const override { return true; }
	virtual bool HoldsPickOnLine() const override;

private:
	// --- His fight (ForemanCreature.cpp; his boss's events and ClearFightState in ForemanCreatureEvents.cpp) ---
	void HandleFightStarted();
	void HandleFightReset();
	void HandlePhaseChanged(int32 NewPhase, int32 OldPhase);
	void HandleStaggered(bool bStaggered);
	void BeginIntro();
	void IntroSlam();
	/** His moves' clocks, his coal's look. */
	void TickMoves(float DeltaSeconds);
	/** Everything of a fight under way put away: the stick in his fist, the dark, the props mended. */
	void ClearFightState(bool bMendProps);
	void Say(const FStoryLine& Line) const;
	/** Dust shaken off the roof over the stope at Count spots round his spot. */
	void ShakeRoofDust(int32 Count) const;
	/** A player come into his stope (EngageRadius, on his level) starts his fight; looked at a few times a second. */
	void TickEngage(float DeltaSeconds);

	// --- Fuse sticks (ForemanCreatureFuse.cpp) ---
	bool WantsFuse() const;
	void BeginFuse();
	void LobFuse();
	/** The stick in his fist let fall (a stagger, his death, a reset puts it out). */
	void DropHeldStick(bool bPutOut);

	// --- The roof (ForemanCreatureRoof.cpp) ---
	bool WantsRoof() const;
	void BeginRoof();
	void RoofStrike();
	void FaceStruckProp(float DeltaSeconds);

	// --- The shift and the dark (ForemanCreatureShift.cpp) ---
	void PruneShift();
	void TickDark(float DeltaSeconds);
	/** The stope's lanterns on or off (their lights and glowing parts), and the exposure's lift. */
	void SetLanterns(bool bOn);
	void SetExposureLift(float Share);

	EForemanMove Move = EForemanMove::Swing;
	bool bIntroWanted = false;
	int32 PacePhase = 0;
	ForemanFight::FPace Pace;
	bool bOwnPaceCaptured = false;
	float OwnChase = 330.f;
	float OwnSwingWindup = 0.8f;
	bool bStaggeredNow = false;
	float EngageCheck = 0.f;

	// Fuses
	float FuseCooldownLeft = 0.f;
	int32 FusesThrown = 0;
	float SinceFuseLine = 1000.f;
	TWeakObjectPtr<AForemanFuseStick> HeldStick;

	// The roof
	float RoofCooldownLeft = 0.f;
	float SinceRoofStrike = -1.f;
	int32 RoofStrikes = 0;
	TWeakObjectPtr<AForemanTimberProp> StruckProp;
	TArray<TWeakObjectPtr<AForemanRockFall>> RockFalls;

	// The shift
	int32 ShiftsCalled = 0;
	TArray<TWeakObjectPtr<ADeadMinerCreature>> Shift;

	// The dark
	float DarkLeft = 0.f;
	float RelightLeft = 0.f;
	float ExposureShare = 0.f;
	TArray<TWeakObjectPtr<ULightComponent>> DarkenedLights;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> DarkenedGlows;
	TWeakObjectPtr<APostProcessVolume> DarkVolume;

	// His death
	bool bLastWordSaid = false;
	float CoalPulse = 0.f;
};
