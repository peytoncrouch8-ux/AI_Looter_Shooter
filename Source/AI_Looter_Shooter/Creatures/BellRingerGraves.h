#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Creatures/BellRingerRules.h"
#include "Subsystems/WorldSubsystem.h"
#include "BellRingerGraves.generated.h"

class ACreatureBase;

/**
 * The Unpaid fallen lately in a level, for the Bell-Ringers' tolls to raise (ABellRingerCreature). A fallen body lingers
 * about three and a half seconds before it's taken away, so a look every WatchSeconds while a ringer lives notes every
 * death in time: where it fell, what it was (class, rank, level, size, its hunting ground) and when. Shared by every
 * ringer in the level, so two never raise the same one. Nothing walks the level's actors where the encounters keep their
 * list of creatures (UEncounterSubsystem::GetTrackedCreatures); no tick, the ringers' frames call Watch. Played worlds,
 * and the editor preview worlds the automated tests build.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBellRingerGraves : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UBellRingerGraves* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	/**
	 * Looks over the level's creatures for the Unpaid fallen since its last look (Now is world time): at most every
	 * WatchSeconds, or at once with bNow (a toll about to land looks again first).
	 */
	void Watch(double Now, bool bNow = false);

	/** Notes Dead as fallen at Now: false for one that can't be raised (CanBeRaised) or noted already. Watch's; the tests'. */
	bool RecordFallen(const ACreatureBase& Dead, double Now);

	/**
	 * Whether a toll may raise this creature once it falls: one of the Unpaid, but never a Bell-Ringer, a boss's or a
	 * Legendary's rank, a fight's boss, one raised already (it falls for good the second time), or a placed one that comes
	 * back by itself.
	 */
	static bool CanBeRaised(const ACreatureBase& Creature);

	/** The fallen noted, oldest first (forgotten KeepSeconds after they fell). */
	TConstArrayView<FBellRingerFallen> GetFallen() const { return Fallen; }

	/** The one a toll from Ringer raises now (BellRingerRules::ChooseFallen), or INDEX_NONE. */
	int32 FindRaisable(const FVector& Ringer, double Now, const FVector& Player, const FBellRingerTuning& Tuning) const;

	/** Takes the fallen at Index for a toll (it won't be raised twice): false when it's gone or taken already. */
	bool Claim(int32 Index);

	/** Every creature a toll raised carries this tag (CanBeRaised: never again). */
	static const FName RaisedTag;

	static constexpr float WatchSeconds = 0.2f;
	static constexpr float KeepSeconds = 30.f;

private:
	/** Forgets the fallen older than KeepSeconds. */
	void Prune(double Now);

	TArray<FBellRingerFallen> Fallen;
	double LastWatch = -1.0e9;
};
