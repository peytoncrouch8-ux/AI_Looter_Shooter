#include "Creatures/UnpaidLifeTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Unpaid.Gait.*, .Rise.* and .Opening.* cvars of the Unpaid's 2026-10-31 round. Their defaults are
// FUnpaidLifeTuning's own field values, so the tests (a default-made tuning) and a fresh game agree.

namespace
{
	const FUnpaidLifeTuning LifeDefaults;

	TAutoConsoleVariable<int32> CVarGait(TEXT("Looter.Unpaid.Gait"), LifeDefaults.Gait,
		TEXT("1: the Unpaid lurch as they drift, breathe and loll their heads at rest, coil before a strike, twist through a rake, loll on a hit and fall four ways; 0 as before."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarLurch(TEXT("Looter.Unpaid.Gait.Lurch"), LifeDefaults.LurchScale,
		TEXT("How much of their lurch the Unpaid show (1 as designed, 0 none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTilt(TEXT("Looter.Unpaid.Gait.Tilt"), LifeDefaults.TiltScale,
		TEXT("How much of their heads' tilt the Unpaid show (1 as designed, 0 none)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarRiseRadius(TEXT("Looter.Unpaid.Rise.Radius"), LifeDefaults.RiseRadius,
		TEXT("An ambush's dead rise round the player this far (cm) when the ambush gives no distance of its own."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRiseFirst(TEXT("Looter.Unpaid.Rise.First"), LifeDefaults.RiseFirst,
		TEXT("The first hand breaks the ground this long (s) after the ambush springs."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRiseGap(TEXT("Looter.Unpaid.Rise.Gap"), LifeDefaults.RiseGap,
		TEXT("Each next one of an ambush's dead breaks the ground this long (s) after the one before."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRiseRumble(TEXT("Looter.Unpaid.Rise.Rumble"), LifeDefaults.RiseRumble,
		TEXT("The ground stirs over each buried one this long (s) before its hand breaks it (the tell)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRiseWindow(TEXT("Looter.Unpaid.Rise.Window"), LifeDefaults.RiseWindow,
		TEXT("Dead rising this near together in time (s) are one burst round the player."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRiseClear(TEXT("Looter.Unpaid.Rise.Clear"), LifeDefaults.RiseClear,
		TEXT("The dead never rise nearer the player than this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRiseSpacing(TEXT("Looter.Unpaid.Rise.Spacing"), LifeDefaults.RiseSpacing,
		TEXT("Two of a burst never rise nearer each other than this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRiseTurn(TEXT("Looter.Unpaid.Rise.Turn"), LifeDefaults.RiseTurn,
		TEXT("A burst's ring turns up to this far either way off the player's view (degrees)."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarOpening(TEXT("Looter.Unpaid.Opening"), LifeDefaults.Opening,
		TEXT("1: the Unpaid pack punishes a player who reloads, swaps guns, stands on an empty gun or drinks the flask near them; 0 never."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarOpeningGap(TEXT("Looter.Unpaid.Opening.Gap"), LifeDefaults.OpeningGap,
		TEXT("One punish per player every this many seconds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOpeningClaim(TEXT("Looter.Unpaid.Opening.Claim"), LifeDefaults.OpeningClaim,
		TEXT("An opening is punished only within this long (s) of its start."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOpeningRange(TEXT("Looter.Unpaid.Opening.Range"), LifeDefaults.OpeningRange,
		TEXT("The punisher is within this far (cm) of the player, behind them or at a side."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOpeningNear(TEXT("Looter.Unpaid.Opening.Near"), LifeDefaults.OpeningNear,
		TEXT("...or anywhere within this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOpeningFront(TEXT("Looter.Unpaid.Opening.Front"), LifeDefaults.OpeningFront,
		TEXT("Behind or at a side: a frontness of at most this (1 in front of the player's view, 0 behind)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOpeningFrenzy(TEXT("Looter.Unpaid.Opening.Frenzy"), LifeDefaults.OpeningFrenzy,
		TEXT("A punish opens its pack's frenzy at the player this long (s); 0 none."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOpeningPunish(TEXT("Looter.Unpaid.Opening.Punish"), LifeDefaults.OpeningPunish,
		TEXT("The punisher's next turn favours its punish this long (s) after its claim."), ECVF_Default);

	float Positive(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}

	float Share(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Clamp(Variable.GetValueOnGameThread(), 0.f, 1.f);
	}
}

FUnpaidLifeTuning FUnpaidLifeTuning::Live()
{
	FUnpaidLifeTuning Numbers;
	Numbers.Gait = CVarGait.GetValueOnGameThread();
	Numbers.LurchScale = Positive(CVarLurch);
	Numbers.TiltScale = Positive(CVarTilt);
	Numbers.RiseRadius = FMath::Max(CVarRiseRadius.GetValueOnGameThread(), 150.f);
	Numbers.RiseFirst = Positive(CVarRiseFirst);
	Numbers.RiseGap = Positive(CVarRiseGap);
	Numbers.RiseRumble = Positive(CVarRiseRumble);
	Numbers.RiseWindow = Positive(CVarRiseWindow);
	Numbers.RiseClear = Positive(CVarRiseClear);
	Numbers.RiseSpacing = Positive(CVarRiseSpacing);
	Numbers.RiseTurn = FMath::Clamp(CVarRiseTurn.GetValueOnGameThread(), 0.f, 90.f);
	Numbers.Opening = CVarOpening.GetValueOnGameThread();
	Numbers.OpeningGap = Positive(CVarOpeningGap);
	Numbers.OpeningClaim = Positive(CVarOpeningClaim);
	Numbers.OpeningRange = Positive(CVarOpeningRange);
	Numbers.OpeningNear = Positive(CVarOpeningNear);
	Numbers.OpeningFront = Share(CVarOpeningFront);
	Numbers.OpeningFrenzy = Positive(CVarOpeningFrenzy);
	Numbers.OpeningPunish = Positive(CVarOpeningPunish);
	return Numbers;
}
