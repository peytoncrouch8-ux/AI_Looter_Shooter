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
		float ColumnHeight = 4500.f;
		float ColumnRadius = 30.f;
		float ColumnGlow = 3.f;
		float NamePopSeconds = 4.5f;
		int32 Motes = 24;
	};
	const FDropTuning DropDefaults;
	const FLegendaryBurst BurstDefaults;

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

	// A Legendary's landing burst (FLegendaryBurst has what each piece is).
	TAutoConsoleVariable<float> CVarBurstScale(TEXT("Looter.Loot.Drop.Burst.Scale"), BurstDefaults.Scale,
		TEXT("A Legendary's landing burst: every size, times (0 = no burst)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstGlow(TEXT("Looter.Loot.Drop.Burst.Glow"), BurstDefaults.Glow,
		TEXT("The burst's strength, times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstLift(TEXT("Looter.Loot.Drop.Burst.Lift"), BurstDefaults.Lift,
		TEXT("The burst's heart over the ground (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstFlashSize(TEXT("Looter.Loot.Drop.Burst.FlashSize"), BurstDefaults.FlashSize,
		TEXT("The burst's white crack, cm across."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstFlashSeconds(TEXT("Looter.Loot.Drop.Burst.FlashSeconds"), BurstDefaults.FlashSeconds,
		TEXT("How long the white crack shows (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstStarSize(TEXT("Looter.Loot.Drop.Burst.StarSize"), BurstDefaults.StarSize,
		TEXT("The burst's star of its colour, cm across."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstStarSeconds(TEXT("Looter.Loot.Drop.Burst.StarSeconds"), BurstDefaults.StarSeconds,
		TEXT("How long the star shows (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstCoreSize(TEXT("Looter.Loot.Drop.Burst.CoreSize"), BurstDefaults.CoreSize,
		TEXT("The glowing core blooms to this size (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstCoreSeconds(TEXT("Looter.Loot.Drop.Burst.CoreSeconds"), BurstDefaults.CoreSeconds,
		TEXT("How long the core takes to cool (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstRingReach(TEXT("Looter.Loot.Drop.Burst.RingReach"), BurstDefaults.RingReach,
		TEXT("The burst's first ground ring runs out this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstRingSeconds(TEXT("Looter.Loot.Drop.Burst.RingSeconds"), BurstDefaults.RingSeconds,
		TEXT("...over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstOuterReach(TEXT("Looter.Loot.Drop.Burst.OuterReach"), BurstDefaults.OuterReach,
		TEXT("The second, slower ground ring runs out this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstOuterSeconds(TEXT("Looter.Loot.Drop.Burst.OuterSeconds"), BurstDefaults.OuterSeconds,
		TEXT("...over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstTiltReach(TEXT("Looter.Loot.Drop.Burst.TiltReach"), BurstDefaults.TiltReach,
		TEXT("The tilted ring (the galaxy's disc) grows to this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstTiltSeconds(TEXT("Looter.Loot.Drop.Burst.TiltSeconds"), BurstDefaults.TiltSeconds,
		TEXT("...over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstTiltDegrees(TEXT("Looter.Loot.Drop.Burst.TiltDegrees"), BurstDefaults.TiltDegrees,
		TEXT("...tilted this far off the ground (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstShockReach(TEXT("Looter.Loot.Drop.Burst.ShockReach"), BurstDefaults.ShockReach,
		TEXT("The shock in the air, facing the eye, grows to this (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstShockSeconds(TEXT("Looter.Loot.Drop.Burst.ShockSeconds"), BurstDefaults.ShockSeconds,
		TEXT("...over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarBurstArms(TEXT("Looter.Loot.Drop.Burst.Arms"), BurstDefaults.Arms,
		TEXT("The burst's spiral arms of star sparks."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarBurstArmSparks(TEXT("Looter.Loot.Drop.Burst.ArmSparks"), BurstDefaults.ArmSparks,
		TEXT("Star sparks in each arm."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstArmReach(TEXT("Looter.Loot.Drop.Burst.ArmReach"), BurstDefaults.ArmReach,
		TEXT("The arms swirl out this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstArmTurns(TEXT("Looter.Loot.Drop.Burst.ArmTurns"), BurstDefaults.ArmTurns,
		TEXT("...winding this far round from the heart to their ends (turns)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstArmSeconds(TEXT("Looter.Loot.Drop.Burst.ArmSeconds"), BurstDefaults.ArmSeconds,
		TEXT("...over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarBurstFountain(TEXT("Looter.Loot.Drop.Burst.Fountain"), BurstDefaults.Fountain,
		TEXT("Star sparks thrown up out of the burst."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstFountainSpeed(TEXT("Looter.Loot.Drop.Burst.FountainSpeed"), BurstDefaults.FountainSpeed,
		TEXT("...this fast (cm/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstFountainSeconds(TEXT("Looter.Loot.Drop.Burst.FountainSeconds"), BurstDefaults.FountainSeconds,
		TEXT("...falling back over this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstSparkWidth(TEXT("Looter.Loot.Drop.Burst.SparkWidth"), BurstDefaults.SparkWidth,
		TEXT("A star spark's trail, cm wide."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstSparkTrail(TEXT("Looter.Loot.Drop.Burst.SparkTrail"), BurstDefaults.SparkTrail,
		TEXT("...as long as this much of its flight (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstSparkHead(TEXT("Looter.Loot.Drop.Burst.SparkHead"), BurstDefaults.SparkHead,
		TEXT("A star spark's twinkling head, cm across."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstSparkMinAngle(TEXT("Looter.Loot.Drop.Burst.SparkMinAngle"), BurstDefaults.SparkMinAngle,
		TEXT("A star spark is never thinner on screen than this (radians: 0.0025 is about two pixels at 1080p), so it reads from 40 m."),
		ECVF_Default);

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
			Moment.bBurst = true;
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

	/** A size or time cvar, never below Least. */
	float AtLeast(const TAutoConsoleVariable<float>& Variable, float Least)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), Least);
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

FLegendaryBurst LootDropMoments::Burst()
{
	// Times are kept off zero (a piece with no life is dropped by FWeaponFX, and a spark's flight divides by it).
	FLegendaryBurst Live;
	Live.Scale = AtLeast(CVarBurstScale, 0.f);
	Live.Glow = AtLeast(CVarBurstGlow, 0.f);
	Live.Lift = AtLeast(CVarBurstLift, 0.f);
	Live.FlashSize = AtLeast(CVarBurstFlashSize, 0.f);
	Live.FlashSeconds = AtLeast(CVarBurstFlashSeconds, 0.01f);
	Live.StarSize = AtLeast(CVarBurstStarSize, 0.f);
	Live.StarSeconds = AtLeast(CVarBurstStarSeconds, 0.01f);
	Live.CoreSize = AtLeast(CVarBurstCoreSize, 0.f);
	Live.CoreSeconds = AtLeast(CVarBurstCoreSeconds, 0.01f);
	Live.RingReach = AtLeast(CVarBurstRingReach, 0.f);
	Live.RingSeconds = AtLeast(CVarBurstRingSeconds, 0.01f);
	Live.OuterReach = AtLeast(CVarBurstOuterReach, 0.f);
	Live.OuterSeconds = AtLeast(CVarBurstOuterSeconds, 0.01f);
	Live.TiltReach = AtLeast(CVarBurstTiltReach, 0.f);
	Live.TiltSeconds = AtLeast(CVarBurstTiltSeconds, 0.01f);
	Live.TiltDegrees = FMath::Clamp(CVarBurstTiltDegrees.GetValueOnGameThread(), 0.f, 90.f);
	Live.ShockReach = AtLeast(CVarBurstShockReach, 0.f);
	Live.ShockSeconds = AtLeast(CVarBurstShockSeconds, 0.01f);
	// The sparks are drawn every frame by the fanfare: kept to a few hundred at most.
	Live.Arms = FMath::Clamp(CVarBurstArms.GetValueOnGameThread(), 0, 8);
	Live.ArmSparks = FMath::Clamp(CVarBurstArmSparks.GetValueOnGameThread(), 0, 40);
	Live.ArmReach = AtLeast(CVarBurstArmReach, 0.f);
	Live.ArmTurns = CVarBurstArmTurns.GetValueOnGameThread();
	Live.ArmSeconds = AtLeast(CVarBurstArmSeconds, 0.05f);
	Live.Fountain = FMath::Clamp(CVarBurstFountain.GetValueOnGameThread(), 0, 80);
	Live.FountainSpeed = AtLeast(CVarBurstFountainSpeed, 0.f);
	Live.FountainSeconds = AtLeast(CVarBurstFountainSeconds, 0.05f);
	Live.SparkWidth = AtLeast(CVarBurstSparkWidth, 0.5f);
	Live.SparkTrail = AtLeast(CVarBurstSparkTrail, 0.f);
	Live.SparkHead = AtLeast(CVarBurstSparkHead, 0.f);
	Live.SparkMinAngle = AtLeast(CVarBurstSparkMinAngle, 0.f);
	return Live;
}

FLegendaryBurst LootDropMoments::DefaultBurst()
{
	return BurstDefaults;
}
