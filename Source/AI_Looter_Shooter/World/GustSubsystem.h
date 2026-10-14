#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/GustClock.h"
#include "World/GustEvent.h"
#include "GustSubsystem.generated.h"

class AFaunaCloth;
class AWindmill;
class UMaterialParameterCollection;
class USwingComponent;

/** The gusts' sound cues: Pass (2D, as loud as the gust is strong) and Rattle (a loose shutter or a hanging sign nearest the
 *  player knocking in it). */
namespace GustCue = LooterSoundCue::Gust;

/** Washing a gust is pushing, and its own settings to push from and put back (UGustSubsystem). */
struct FGustStirredCloth
{
	TWeakObjectPtr<AFaunaCloth> Cloth;
	float Swing = 0.f;
	float Flutter = 0.f;
	float Yaw = 0.f;
};

/** A windmill a gust is spinning up, and its own speed to put back (UGustSubsystem). */
struct FGustStirredWindmill
{
	TWeakObjectPtr<AWindmill> Windmill;
	float TurnsPerMinute = 0.f;
};

/**
 * Now and then a gust (Docs: the "make it alive" polish, board C item 6): every 40 to 120 s (FGustClock, seeded by the
 * level) the wind gets up from its prevailing quarter (the way the chimney smoke leans, yaw 19) for 2 to 5 s. As it
 * begins, OnGust tells everything that swings or flaps (USwingComponent and the hanging signs on it); while it blows it
 * writes its push to MPC_Wind (GustStrength, GustDirection) for the foliage, the grass and the chimney smoke to bow and
 * lean, rolls a low swirl of dust along the open ground upwind of the player and past them (FWeaponFX's dust puffs: no
 * draws of its own), flaps the washing harder, spins the windmill up, may send a tumbleweed rolling, plays the wind
 * swelling past (GustCue::Pass) and a shutter or sign knocking near the player (GustCue::Rattle). UAmbientAirSubsystem's
 * motes, pollen, fluff and leaves are carried with it. Nothing of it is heard or seen from the mine (UAmbientAirSubsystem::
 * IsSheltered), and Looter.Air 0 stills it with the air.
 *
 * It costs nothing between gusts: a timer wakes it when the next is due, and it ticks only while one blows. It also
 * keeps the swings near the player awake (a look every half second while any are in the level). Looter.Gust [strength]
 * [seconds] starts one now (World/AmbientAirDevCommands.cpp).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGustSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UGustSubsystem* Get(const UObject* WorldContextObject);

	/** The push of the gust blowing now in WorldContextObject's world (0 to 1; 0 between gusts and without one): for things that poll. */
	static float StrengthAt(const UObject* WorldContextObject);

	/** The prevailing wind's way (degrees; the smoke's lean: build_area.py WIND_YAW, the fauna's WindYaw). */
	static constexpr float DefaultWindYaw = 19.f;

	/** MPC_Wind (Tools/Unreal/wind_collection.py) and the parameters a gust writes to it. */
	static const TCHAR* CollectionPath;
	static const FName GustStrengthParameter;
	static const FName GustDirectionParameter;

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** Told once as each gust begins. */
	FOnGust OnGust;

	/** Moves the gusts on by DeltaSeconds: the tick and the timer do (by the world's clock); tests call it. */
	void Advance(float DeltaSeconds);

	/** A gust now (Looter.Gust, tests): Strength and Seconds below 0 roll their own. False while one blows. */
	bool Trigger(float Strength = -1.f, float Seconds = -1.f);

	/** Starts the clock over with Seed (tests; a level's own seed is its name's). */
	void ResetClock(int32 Seed);

	const FGustClock& GetClock() const { return Clock; }
	bool IsBlowing() const { return Clock.IsBlowing(); }
	/** The push now (0 to 1) and its way (the prevailing wind's between gusts). */
	float GetStrengthNow() const;
	FVector GetDirection() const;
	FVector GetPrevailingDirection() const;

	/** Swings near the player are kept awake; the rest sleep until the player comes (USwingComponent registers itself). */
	void RegisterSwing(USwingComponent* Swing);
	void UnregisterSwing(USwingComponent* Swing);
	int32 NumSwings() const { return Swings.Num(); }

	/** How often (s) the swings' distance from the view is looked at. */
	static constexpr float SwingWatchSeconds = 0.5f;

private:
	/** The world's clock since the last Advance, applied (the tick's and the timer's way in). */
	void StepByWorldClock();
	/** Arms the timer for the next gust (between gusts nothing ticks). */
	void ArmTimer();
	void BeginGust();
	void UpdateGust(float DeltaSeconds);
	void EndGust();
	/** Writes the push to MPC_Wind (0 between gusts). */
	void WriteCollection(float Strength, const FVector& Direction);
	/** The swings near the view awake, the rest left to settle and sleep. */
	void WatchSwings();
	void SetSwingWatch(bool bOn);
	bool FindView(FVector& OutLocation) const;
	bool IsSilenced() const;

	// --- What a gust stirs in the world (GustSubsystemEffects.cpp) ---
	/** Chooses the gust's dust swirl, rattle, washing and windmills as it begins; a tumbleweed may set off. */
	void StirWorld();
	/** Moves them on while it blows: the swirl's puffs, the rattle at its height, the washing's and windmills' push. */
	void StirWorldUpdate(float DeltaSeconds);
	/** Puts the washing and the windmills back as they were. */
	void StillWorld();
	void LaunchTumbleweed(const FVector& Player);

	FGustClock Clock;
	/** The gust blowing now is heard and seen (not silenced, not in the mine). */
	bool bStirring = false;
	bool bWasBlowing = false;
	/** One more write of zeros after a gust (the tick runs one frame past it). */
	bool bCollectionDirty = false;
	double LastStepTime = 0.0;
	bool bClockStarted = false;
	FTimerHandle NextGustTimer;
	FTimerHandle SwingWatchTimer;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> WindCollection;
	bool bCollectionLooked = false;

	TArray<TWeakObjectPtr<USwingComponent>> Swings;

	// The gust's stir (GustSubsystemEffects.cpp).
	TArray<FGustStirredCloth> StirredCloth;
	TArray<FGustStirredWindmill> StirredWindmills;
	/** The swirl: where its middle is (on the ground), whether it's rolling, its puffs owed, its ground looked at again. */
	FVector SwirlAt = FVector::ZeroVector;
	bool bSwirl = false;
	bool bSwirlOnDust = false;
	float SwirlPuffsOwed = 0.f;
	float SwirlGroundIn = 0.f;
	float SwirlSpin = 0.f;
	FLinearColor SwirlColor = FLinearColor::White;
	/** The rattle: where, and whether it's still to play. */
	FVector RattleAt = FVector::ZeroVector;
	bool bRattlePending = false;
	FRandomStream StirRandom{ 0x57124 };
};
