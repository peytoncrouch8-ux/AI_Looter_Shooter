// GravemotherTuning (GravemotherTuning.h): the playtest's cvars for the Gravemother's fight.

#include "Creatures/GravemotherTuning.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<float> CVarHealthScale(TEXT("Looter.Gravemother.HealthScale"), GravemotherTuning::DefaultHealthScale,
		TEXT("The Gravemother's health against her first tuning (300 at a spider's level 1). Applies to the next one spawned."));

	TAutoConsoleVariable<float> CVarDamageScale(TEXT("Looter.Gravemother.DamageScale"), GravemotherTuning::DefaultDamageScale,
		TEXT("The Gravemother's hits (bite, Furrow, Gravequake, Grave Clod) against her first tuning. Applies to the next one spawned."));

	TAutoConsoleVariable<int32> CVarStoneGuards(TEXT("Looter.Gravemother.StoneGuards"), 1,
		TEXT("1: nothing hurts the Gravemother while her headstone stands (it must be shot off first). 0: hits on her count as before."));

	TAutoConsoleVariable<int32> CVarRimShrug(TEXT("Looter.Gravemother.RimShrug"), 1,
		TEXT("1: hits from a shooter off the Gravemother's hunting ground (the Sink's rim) do her and her stone no harm."));

	TAutoConsoleVariable<float> CVarRimSlack(TEXT("Looter.Gravemother.RimSlack"), GravemotherTuning::DefaultRimSlack,
		TEXT("How far past the Gravemother's hunting ground (cm, out and up) a shooter still hurts her (a jump on a berm; the rim is 12 m up)."));

	TAutoConsoleVariable<float> CVarMarkSeconds(TEXT("Looter.Gravemother.StoneMarkSeconds"), GravemotherTuning::DefaultMarkSeconds,
		TEXT("Seconds the Gravemother's headstone glows as the target after a hit her hide shrugged off (0: never)."));

	TAutoConsoleVariable<float> CVarMarkAtFight(TEXT("Looter.Gravemother.StoneMarkAtFight"), GravemotherTuning::DefaultMarkAtFightSeconds,
		TEXT("Seconds the Gravemother's headstone glows as her fight starts (0: never)."));

	TAutoConsoleVariable<int32> CVarHintAfter(TEXT("Looter.Gravemother.ShrugHintAfter"), GravemotherTuning::DefaultHintAfter,
		TEXT("Shrugged hits of a kind before Hob's hint (come down into the Sink; break the headstone), once a Gravemother. 0: never."));
}

namespace GravemotherTuning
{
	float HealthScale()
	{
		return FMath::Max(CVarHealthScale.GetValueOnGameThread(), 0.05f);
	}

	float DamageScale()
	{
		return FMath::Max(CVarDamageScale.GetValueOnGameThread(), 0.05f);
	}

	bool StoneGuards()
	{
		return CVarStoneGuards.GetValueOnGameThread() != 0;
	}

	bool RimShrug()
	{
		return CVarRimShrug.GetValueOnGameThread() != 0;
	}

	float RimSlack()
	{
		return FMath::Max(CVarRimSlack.GetValueOnGameThread(), 0.f);
	}

	float MarkSeconds()
	{
		return FMath::Max(CVarMarkSeconds.GetValueOnGameThread(), 0.f);
	}

	float MarkAtFightSeconds()
	{
		return FMath::Max(CVarMarkAtFight.GetValueOnGameThread(), 0.f);
	}

	int32 HintAfter()
	{
		return FMath::Max(CVarHintAfter.GetValueOnGameThread(), 0);
	}
}
