#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/FightEvent.h"
#include "Subsystems/WorldSubsystem.h"
#include "FightLog.generated.h"

class ACreatureBase;
class AController;
class AActor;
class UHealthComponent;
class UFightDirectorSubsystem;

/**
 * Looter.Fight.Log 1 (FIGHTS.md section 5, lane MEASURE): watches a fight against the player and reports how it went, so a
 * change to the creatures, the director or the guns is judged by numbers, not by feel. Dev only: nothing is made in a shipping
 * build, and nothing runs while it is off.
 *
 * It watches the player's health (every hit that lands, by what dealt it; the lowest their health and ward together got; how
 * fast they were hurt at the worst), the creatures near them (when each was first hurt and when it died, so a kill's time by
 * kind and rank), and the fight director's events (UFightDirectorSubsystem::OnFightEvent: turns, attacks started and landed,
 * and the new moves). Polled once a frame while on: ten creatures cost well under a tenth of a millisecond.
 *
 *   Looter.Fight.Log 1        start (clears what was seen)       Looter.Fight.Log 0        stop (keeps it for the report)
 *   Looter.Fight.Log report   log the report and write the CSV   Looter.Fight.Log reset    clear and keep watching
 *
 * The report goes to the log (lines start with FIGHT, for console.ps1 -Until) and to Saved/FightLog/<time>.csv as
 * section,key,value rows. Looter.Fight.Sim starts it for a measured fight.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UFightLogSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The log of WorldContext's world, or none (no such world, or a shipping build). */
	static UFightLogSubsystem* Get(const UObject* WorldContext);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bOn; }
	virtual TStatId GetStatId() const override;

	/** Clears what was seen and starts watching. */
	void Start();
	/** Stops watching; what was seen stays for Report. */
	void Stop();
	/** Clears what was seen (watching goes on if it was on). */
	void Reset();
	bool IsOn() const { return bOn; }

	/** Logs the report; with bWriteCsv also writes it under Saved/FightLog. Returns the CSV's full path (empty when none was written). */
	FString Report(bool bWriteCsv);

	/** What the log has seen so far, for tests and the report. */
	double GetFightSeconds() const;
	float GetLowestHealthShare() const { return LowestShare; }
	float GetPeakDamagePerSecond() const { return PeakDps; }
	float GetTotalDamageTaken() const { return TotalDamage; }
	int32 GetPlayerDeaths() const { return PlayerDeaths; }
	int32 GetPeakFighting() const { return PeakFighting; }
	int32 GetKills() const;
	int32 CountEvents(EFightEvent Event) const;

	/** An event's name as the report spells it ("AttackLanded"). */
	static FString EventName(EFightEvent Event);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** The player's health component's events (dynamic delegates: the signatures are the component's). */
	UFUNCTION()
	void HandlePlayerDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION()
	void HandlePlayerDeath(AController* Killer);

	/** The director's event (native delegate). */
	void HandleFightEvent(const ACreatureBase& Creature, EFightEvent Event, float Value);

	/** One creature seen in the fight: how it began and how it ended. */
	struct FWatch
	{
		TWeakObjectPtr<const ACreatureBase> Creature;
		FString Kind;
		ECreatureRank Rank = ECreatureRank::Basic;
		int32 Level = 1;
		float LastHealth = 0.f;
		double FirstHit = -1.0;
		double Died = -1.0;
	};

	/** What one kind of attacker did to the player. */
	struct FDamageIn
	{
		float Total = 0.f;
		float ToHealth = 0.f;
		int32 Hits = 0;
	};

	void BindPlayer(UHealthComponent* Health);
	void UnbindPlayer();
	void BindDirector();
	void UnbindDirector();
	void Poll(double Now);
	/** Damage dealt in the last WindowSeconds, from Now (the 5 s window the peak is taken over). */
	float WindowDamage(double Now) const;

	static FString KindOf(const AActor& Actor);

	bool bOn = false;
	TWeakObjectPtr<UHealthComponent> PlayerHealth;
	FDelegateHandle EventHandle;
	TWeakObjectPtr<UFightDirectorSubsystem> BoundDirector;

	// What was seen.
	TMap<uint32, FWatch> Watched;
	TMap<FString, FDamageIn> DamageByKind;
	TArray<TPair<double, float>> DamageTimes;
	TMap<FString, int32> EventsByName;
	TMap<FString, int32> StartedByKind;
	TMap<FString, int32> LandedByKind;
	double FightStart = -1.0;
	double LastDeath = -1.0;
	double LastSeen = -1.0;
	float LowestShare = 1.f;
	float PeakDps = 0.f;
	float TotalDamage = 0.f;
	float TotalToHealth = 0.f;
	int32 PlayerDeaths = 0;
	int32 PeakFighting = 0;
	int32 PeakMeleeTokens = 0;
	int32 PeakRangedTokens = 0;
};
