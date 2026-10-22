#include "Loot/LootDropMoments.h"
#include "Audio/LooterSoundCues.h"
#include "HAL/IConsoleManager.h"

// The drop moments' numbers, each a Looter.Loot.Drop.* cvar (live in PIE); the defaults below are the starting values.

namespace
{
	struct FDropTuning
	{
		// The beams' heights (cm), by rarity: the old 350 / 650 / 1000 / 1600 each a good step taller.
		float BeamUncommon = 450.f;
		float BeamRare = 900.f;
		float BeamEpic = 1400.f;
		float BeamLegendary = 2200.f;
		float SpinRate = 900.f;
		float SpinSeconds = 0.9f;
		float RingEpic = 260.f;
		float RingLegendary = 420.f;
		float RingSeconds = 0.7f;
		float SlowSeconds = 0.25f;
		float SlowDilation = 0.3f;
		float ColumnHeight = 4500.f;
		float ColumnRadius = 30.f;
		float ColumnGlow = 3.f;
		float NamePopSeconds = 4.5f;
		int32 Motes = 24;
	};
	const FDropTuning DropDefaults;

	TAutoConsoleVariable<float> CVarBeamUncommon(TEXT("Looter.Loot.Drop.Beam.Uncommon"), DropDefaults.BeamUncommon,
		TEXT("An Uncommon gun's loot beam, cm tall."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBeamRare(TEXT("Looter.Loot.Drop.Beam.Rare"), DropDefaults.BeamRare,
		TEXT("A Rare gun's loot beam, cm tall."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBeamEpic(TEXT("Looter.Loot.Drop.Beam.Epic"), DropDefaults.BeamEpic,
		TEXT("An Epic gun's loot beam, cm tall."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBeamLegendary(TEXT("Looter.Loot.Drop.Beam.Legendary"), DropDefaults.BeamLegendary,
		TEXT("A Legendary gun's loot beam, cm tall."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSpinRate(TEXT("Looter.Loot.Drop.Spin.Rate"), DropDefaults.SpinRate,
		TEXT("An Epic or Legendary drop spins this fast as it lands (deg/s), easing back to its own."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSpinSeconds(TEXT("Looter.Loot.Drop.Spin.Seconds"), DropDefaults.SpinSeconds,
		TEXT("How long the landing's spin takes to ease back (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingEpic(TEXT("Looter.Loot.Drop.Ring.Epic"), DropDefaults.RingEpic,
		TEXT("An Epic's landing ring runs out this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingLegendary(TEXT("Looter.Loot.Drop.Ring.Legendary"), DropDefaults.RingLegendary,
		TEXT("A Legendary's landing ring runs out this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingSeconds(TEXT("Looter.Loot.Drop.Ring.Seconds"), DropDefaults.RingSeconds,
		TEXT("How long a landing ring takes to run out and fade (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSlowSeconds(TEXT("Looter.Loot.Drop.Slow.Seconds"), DropDefaults.SlowSeconds,
		TEXT("A Legendary's drop slows the world for this long (real s; 0 = off)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSlowDilation(TEXT("Looter.Loot.Drop.Slow.Dilation"), DropDefaults.SlowDilation,
		TEXT("The world's speed during a Legendary's slow-down."), ECVF_Default);
	TAutoConsoleVariable<float> CVarColumnHeight(TEXT("Looter.Loot.Drop.Column.Height"), DropDefaults.ColumnHeight,
		TEXT("A landed Legendary's column of light, cm tall (0 = none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarColumnRadius(TEXT("Looter.Loot.Drop.Column.Radius"), DropDefaults.ColumnRadius,
		TEXT("The Legendary's column, cm round."), ECVF_Default);
	TAutoConsoleVariable<float> CVarColumnGlow(TEXT("Looter.Loot.Drop.Column.Glow"), DropDefaults.ColumnGlow,
		TEXT("The Legendary's column's glow."), ECVF_Default);
	TAutoConsoleVariable<float> CVarNamePopSeconds(TEXT("Looter.Loot.Drop.NamePop.Seconds"), DropDefaults.NamePopSeconds,
		TEXT("A landed Legendary's name pops up over it for this long (s; 0 = none)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarMotes(TEXT("Looter.Loot.Drop.Motes"), DropDefaults.Motes,
		TEXT("Motes rising round a landed Legendary."), ECVF_Default);

	FDropTuning LiveTuning()
	{
		FDropTuning Tuning;
		Tuning.BeamUncommon = FMath::Max(CVarBeamUncommon.GetValueOnGameThread(), 0.f);
		Tuning.BeamRare = FMath::Max(CVarBeamRare.GetValueOnGameThread(), 0.f);
		Tuning.BeamEpic = FMath::Max(CVarBeamEpic.GetValueOnGameThread(), 0.f);
		Tuning.BeamLegendary = FMath::Max(CVarBeamLegendary.GetValueOnGameThread(), 0.f);
		Tuning.SpinRate = FMath::Max(CVarSpinRate.GetValueOnGameThread(), 0.f);
		Tuning.SpinSeconds = FMath::Max(CVarSpinSeconds.GetValueOnGameThread(), 0.f);
		Tuning.RingEpic = FMath::Max(CVarRingEpic.GetValueOnGameThread(), 0.f);
		Tuning.RingLegendary = FMath::Max(CVarRingLegendary.GetValueOnGameThread(), 0.f);
		Tuning.RingSeconds = FMath::Max(CVarRingSeconds.GetValueOnGameThread(), 0.05f);
		Tuning.SlowSeconds = FMath::Clamp(CVarSlowSeconds.GetValueOnGameThread(), 0.f, 2.f);
		Tuning.SlowDilation = FMath::Clamp(CVarSlowDilation.GetValueOnGameThread(), 0.05f, 1.f);
		Tuning.ColumnHeight = FMath::Max(CVarColumnHeight.GetValueOnGameThread(), 0.f);
		Tuning.ColumnRadius = FMath::Max(CVarColumnRadius.GetValueOnGameThread(), 1.f);
		Tuning.ColumnGlow = FMath::Max(CVarColumnGlow.GetValueOnGameThread(), 0.f);
		Tuning.NamePopSeconds = FMath::Max(CVarNamePopSeconds.GetValueOnGameThread(), 0.f);
		Tuning.Motes = FMath::Clamp(CVarMotes.GetValueOnGameThread(), 0, 80);
		return Tuning;
	}

	FDropMoment Build(EWeaponRarity Rarity, const FDropTuning& Tuning)
	{
		FDropMoment Moment;
		const bool bEpicUp = Rarity == EWeaponRarity::Epic || Rarity == EWeaponRarity::Legendary;
		switch (Rarity)
		{
		case EWeaponRarity::Uncommon:
			Moment.BeamHeight = Tuning.BeamUncommon;
			Moment.Stinger = LooterSoundCue::DropUncommon;
			break;
		case EWeaponRarity::Rare:
			Moment.BeamHeight = Tuning.BeamRare;
			Moment.Stinger = LooterSoundCue::DropRare;
			break;
		case EWeaponRarity::Epic:
			Moment.BeamHeight = Tuning.BeamEpic;
			Moment.Stinger = LooterSoundCue::DropEpic;
			Moment.RingRadius = Tuning.RingEpic;
			break;
		case EWeaponRarity::Legendary:
			Moment.BeamHeight = Tuning.BeamLegendary;
			Moment.Stinger = LooterSoundCue::DropLegendary;
			Moment.bStingerAtDrop = true;
			Moment.LandCue = LootDropMoments::LegendaryLandCue;
			Moment.RingRadius = Tuning.RingLegendary;
			Moment.SlowSeconds = Tuning.SlowSeconds;
			Moment.SlowDilation = Tuning.SlowDilation;
			Moment.ColumnHeight = Tuning.ColumnHeight;
			Moment.ColumnRadius = Tuning.ColumnRadius;
			Moment.ColumnGlow = Tuning.ColumnGlow;
			Moment.NamePopSeconds = Tuning.NamePopSeconds;
			Moment.Motes = Tuning.Motes;
			break;
		default:
			break;
		}
		Moment.BeamGlow = bEpicUp ? 3.f : 2.f;
		Moment.BeamRadius = Rarity == EWeaponRarity::Legendary ? 11.f : (bEpicUp ? 9.f : 6.f);
		Moment.bStingerFlat = bEpicUp;
		if (bEpicUp)
		{
			Moment.SpinRate = Tuning.SpinRate;
			Moment.SpinSeconds = Tuning.SpinSeconds;
			Moment.RingSeconds = Tuning.RingSeconds;
		}
		return Moment;
	}
}

FDropMoment LootDropMoments::For(EWeaponRarity Rarity)
{
	return Build(Rarity, LiveTuning());
}

FDropMoment LootDropMoments::Defaults(EWeaponRarity Rarity)
{
	return Build(Rarity, DropDefaults);
}

bool LootDropMoments::MaySlowWorld(float CurrentDilation)
{
	return FMath::IsNearlyEqual(CurrentDilation, 1.f, 0.001f);
}
