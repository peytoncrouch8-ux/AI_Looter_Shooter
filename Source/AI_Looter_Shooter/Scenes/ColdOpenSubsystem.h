#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ColdOpenSubsystem.generated.h"

class AColdOpenSet;
struct FCampaignRecord;

namespace GraveWake
{
	struct FState;
}

/**
 * The story's opening on its first area (Docs/Areas/RansomsRest.md, Main 1 "Seven Days"): the cold open, then the grave
 * wake-up, once per session. The level's travel calls it as the level begins (UAreaTravelSubsystem): where the level has
 * the cold open's set (AColdOpenSet: Ransom's Rest) and the story has just begun (the first cast-off, or "Skip the
 * tutorial", which counts as one) without having seen it, it takes the screen: the white held through the trip (held
 * here if nothing held it) is revealed with REVENANT on the gang's skiff, the dusk scene plays, and Ellis claws out of the
 * grave. Its end, played or skipped, is recorded in the campaign (bColdOpenSeen), so it never plays again; a session
 * quit before the claw-out was done sees it again from the start.
 *
 * Where it can't play (scenes off in a tour or perf run, no grave, a level played without the story begun), it's passed:
 * recorded as seen, and the missions told both scenes have played, so Main 1 goes on to the headboard. A level whose story
 * is past it tells its missions the same as it begins. Looter.Scene.ColdOpen and Looter.Scene.GraveWake play it by hand.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UColdOpenSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UColdOpenSubsystem* Get(const UObject* WorldContextObject);

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** The cold open is due in Campaign: the story has begun (the first cast-off) and it hasn't been seen. */
	static bool IsDue(const FCampaignRecord& Campaign);

	/**
	 * As the level begins (the travel subsystem's arrival): plays the cold open when the level has its set and it's due,
	 * taking the screen; passes it where it's due but can't play. True when it plays (the caller leaves the white to it).
	 */
	bool BeginIfDue();

	/**
	 * Plays the cold open and then the grave wake-up. With bRecord their end is recorded in the campaign (an arrival's);
	 * without, nothing is (a console command's). False when it can't: no set in the level, a scene playing, scenes off.
	 */
	bool PlayColdOpen(bool bRecord);

	/** Plays the grave wake-up alone (the claw-out). False when it can't: no grave in the level, a scene playing, scenes off. */
	bool PlayGraveWake(bool bRecord);

	/** A claw for the wake-up playing now, as Jump is (the console, the tests). False when none plays, or it didn't count. */
	bool Claw();

	/** How many claws the wake-up playing now has taken (0 when none plays). */
	int32 GetClaws() const;

	/** The cold open and the wake-up are behind the player: recorded seen (with bRecord) and the missions told both played. */
	void PassColdOpen(bool bRecord);

private:
	/** The story being played here (the mission runner's record: the session's, or a test's). */
	FCampaignRecord* FindCampaign() const;

	/** The wake-up has ended, played or skipped (or couldn't play after the cold open): recorded, the screen back. */
	void FinishOpening(bool bRecord);

	/** The wake-up playing now, to take claws from outside its keys. */
	TSharedPtr<GraveWake::FState> WakeState;
};
