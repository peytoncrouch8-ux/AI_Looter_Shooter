#pragma once

#include "CoreMinimal.h"
#include "Creatures/UnpaidMoveRules.h"
#include "Player/DustLight.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnpaidPackSubsystem.generated.h"

class AActor;
class APawn;
class UPointLightComponent;

/** What the Unpaid's tally counts (Looter.Unpaid.Moves; the play-check's move mix). */
enum class EUnpaidTally : uint8
{
	Lunge,
	Rake,
	Feint,
	PhaseStrike,
	LowSwipe,
	HangBack,
	Flank,
	Cinder,
	Rally,
	Shriek,
	ShriekInterrupted,
	Count
};

/** How many EUnpaidTally entries there are (its arrays are this long). */
inline constexpr int32 NumUnpaidTallies = static_cast<int32>(EUnpaidTally::Count);

/**
 * The Unpaid packs' level-wide side of their varied fight (2026-10-29): which move each Unpaid commits at each player now
 * (FUnpaidMoveBoard: two never commit the same move at once), each pack's last failed-lunge shriek at each player (one a
 * Looter.Unpaid.Shriek.PackGap), the move tally (Looter.Unpaid.Moves), and the shriek's show: a small pool of its coal's
 * lights and the ground wave's lifted dust, grass and leaves as the ring rolls out. Ticks only while a wave rolls or a light
 * fades. Played worlds and the editor preview worlds the tests build (no lights there).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UUnpaidPackSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UUnpaidPackSubsystem* Get(const UObject* WorldContextObject);

	// --- The board (UnpaidCreatureVariety.cpp) ---

	/** Id commits Move at Target now (None: nothing). */
	void Commit(const APawn& Target, uint32 Id, EUnpaidMove Move);
	/** Id commits nothing, at anyone. */
	void Release(uint32 Id);
	/** Target's board (none: an empty one). */
	const FUnpaidMoveBoard& BoardFor(const APawn& Target) const;

	// --- The pack's shriek gate ---

	/** Seconds since Pack last shrieked at Target (a large number: never). */
	float SincePackShriek(const APawn& Target, FName Pack) const;
	void NotePackShriek(const APawn& Target, FName Pack);

	// --- The tally ---

	void Count(EUnpaidTally What);
	/** A move's blow landed (Lunge, Rake, PhaseStrike, LowSwipe). */
	void CountLanded(EUnpaidTally What);
	/** The tally as one line per move since its reset, and the seconds it covers. */
	FString Report() const;
	void ResetTally();
	int32 GetCount(EUnpaidTally What) const { return Tally[static_cast<int32>(What)]; }
	/** Counts What at WorldContext's world's subsystem, if it has one. */
	static void Note(const UObject* WorldContext, EUnpaidTally What, bool bLanded = false);

	// --- The shriek's show (UnpaidPackSubsystemShow.cpp) ---

	/** One of the pool's lights for Holder's shriek (INDEX_NONE: none free, a test level, or Looter.Unpaid.Shriek.Light 0); freed if Holder goes. */
	int32 TakeLight(const AActor* Holder);
	/** Light Handle shines at Where in Color, Candelas strong, Radius far (cm); 0 candelas hides it. */
	void SetLight(int32 Handle, const FVector& Where, const FLinearColor& Color, float Candelas, float Radius);
	/** Gives Handle back: it fades out over a moment and is free again. */
	void GiveLight(int32 Handle);
	/** A ground wave rolling out from Feet to Radius over Seconds: the dust, grass and leaves lifted along its front. */
	void StartWave(const FVector& Feet, float Radius, float Seconds, float Strength, const AActor* Source);

	/** At most this many shriek lights at once (three shrieking at once each has one; a fourth goes dark). */
	static constexpr int32 MaxLights = 3;

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FTargetBoard
	{
		FUnpaidMoveBoard Board;
		TMap<FName, double> PackShriek;
	};

	struct FLight
	{
		TWeakObjectPtr<UPointLightComponent> Light;
		bool bTaken = false;
		/** Whose it is while taken: one gone without giving it back frees it. */
		TWeakObjectPtr<const AActor> Holder;
		/** Its strength as last set, fading after it's given back. */
		float Candelas = 0.f;
	};

	struct FWave
	{
		FVector Feet = FVector::ZeroVector;
		float Radius = 0.f;
		float Seconds = 0.f;
		float Age = 0.f;
		float Strength = 1.f;
		/** Pieces owed from earlier frames, and the light its dust is shown in. */
		float Owed = 0.f;
		FLinearColor Light = FLinearColor::White;
		/** The angle round it the next pieces start from (spread evenly, jittered). */
		float Turn = 0.f;
		TWeakObjectPtr<const AActor> Source;
	};

	double Now() const;
	/** Moves the waves on, throwing their front's pieces (UnpaidPackSubsystemShow.cpp). */
	void AdvanceWaves(float DeltaSeconds);
	/** Fades the lights given back. */
	void AdvanceLights(float DeltaSeconds);
	/** The pool's light Index, made on first use. */
	UPointLightComponent* LightAt(int32 Index);

	TMap<TWeakObjectPtr<const APawn>, FTargetBoard> Boards;
	int32 Tally[NumUnpaidTallies] = {};
	int32 Landed[NumUnpaidTallies] = {};
	double TallySince = 0.0;

	FLight Lights[MaxLights];
	TWeakObjectPtr<AActor> LightOwner;
	TArray<FWave> Waves;
	FDustLight DustLight;
};
