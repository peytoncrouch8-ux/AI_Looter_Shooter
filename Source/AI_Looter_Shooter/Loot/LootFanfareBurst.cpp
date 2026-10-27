#include "Loot/LootFanfareSubsystem.h"
#include "Combat/BulletSubsystem.h"
#include "Loot/LootDropMoments.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponFX.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/RandomStream.h"

// ULootFanfareSubsystem's Legendary burst (the user, 2026-10-27: "When a legendary drops, the game stutters. There should be
// a galactic explosion instead of a game stutter."). All of it code-drawn light through the bullets' pooled effects
// (FWeaponFX; no lights, nothing Nanite, no assets), in the gun's colour, sized to read from 40 m (FLegendaryBurst has
// every size; Looter.Loot.Drop.Burst.*):
//  - the moment: a white crack and a star of its colour over the heart, a ball of its light blooming and cooling;
//  - its reach: two rings racing out over the ground, the second wider and slower, and a shock in the air facing the eye;
//  - the galaxy: a ring tilted off the ground like a galaxy's disc, star sparks swirling out in spiral arms in that disc
//    (each winds further round the further out it gets), and a fountain of them thrown up and falling;
//  - each star spark drawn each frame here: a short trail and a twinkling round head, never thinner on screen than
//    SparkMinAngle, so a burst 40 m off still glitters.
// The cost: about ten shapes for under 1.5 s and a hundred-odd one-frame streaks and glows (FWeaponFX's instanced quads);
// the big shapes are drawn smaller when the eye is inside them (a fill of the screen is all overdraw).

namespace
{
	/** White-hot, as the blasts' (FWeaponFX's BlastWhiteHot). */
	const FLinearColor BurstWhite(1.f, 0.95f, 0.85f);

	/**
	 * Strengths (times the burst's glow and Looter.Vfx.Glow): the crack and the star as bright as a crit's flare (20-26), the
	 * core low (additive fire keeps its colour in daylight only near 1-4: WeaponFXDraw), the rings as a kill tier's (16-18).
	 */
	constexpr float FlashStrength = 30.f;
	constexpr float StarStrength = 22.f;
	constexpr float CoreStrength = 3.f;
	constexpr float RingStrength = 18.f;
	constexpr float OuterStrength = 9.f;
	constexpr float TiltStrength = 14.f;
	constexpr float ShockStrength = 8.f;
	/** A star spark's trail and its head (the kills' embers burn 14-24). */
	constexpr float SparkStrength = 20.f;
	constexpr float HeadStrength = 10.f;

	/** The core starts this share of its size; it drifts up this fast (cm/s, times the scale). */
	constexpr float CoreStartShare = 0.3f;
	constexpr float CoreRise = 60.f;
	/** The rings' widths and first radii (cm, times the scale). */
	constexpr float RingWidth = 70.f;
	constexpr float RingStart = 60.f;
	constexpr float OuterWidth = 40.f;
	constexpr float OuterStart = 40.f;
	constexpr float TiltWidth = 45.f;
	constexpr float TiltStart = 80.f;
	constexpr float ShockWidth = 30.f;
	constexpr float ShockStart = 50.f;
	/** Spikes on the crack and on the star. */
	constexpr int32 FlashSpikes = 16;
	constexpr int32 StarSpikes = 12;

	/** The eye this many times the star's size away or nearer has the big shapes drawn smaller, down to this share. */
	constexpr float CloseShare = 1.5f;
	constexpr float CloseLeast = 0.35f;

	/** An arm's sparks start this far out along it (share of its reach), are scattered this far off it (radians). */
	constexpr float ArmInnerShare = 0.25f;
	constexpr float ArmScatter = 0.18f;
	/** The disc's thickness: a spark sits this share of its reach above or below it. */
	constexpr float DiscThickness = 0.06f;
	/** Each spark's life, this much shorter or longer at random. */
	constexpr float LifeLeast = 0.8f;
	constexpr float LifeMost = 1.15f;
	/** A spark's colour: its gun's, up to this share of the way to white (the brightest stars). */
	constexpr float WhitestShare = 0.6f;
	/** The arms slow as they go: their path's ease (1 - (1 - t)^this). */
	constexpr float ArmEase = 2.5f;
	/** Fading over its life (1 - t)^this. */
	constexpr float SparkFade = 1.2f;
	/** The fountain's throw: up, give or take this much any way, at this share to all of its speed. */
	constexpr float FountainSpread = 0.35f;
	constexpr float FountainSlowest = 0.6f;
	/** The fountain's sparks: air drag (1/s) and gravity (cm/s^2, light sparks falling softly). */
	constexpr float FountainDrag = 1.2f;
	constexpr float FountainGravity = 600.f;
	/** A head twinkles this fast (radians/s), between this share and all of its strength. */
	constexpr float TwinkleRate = 22.f;
	constexpr float TwinkleLeast = 0.55f;
	/** A head is at least this many times a trail's least width on screen. */
	constexpr float HeadOverTrail = 3.f;
	/** The most star sparks in flight at once (a boss's shower of Legendaries): the next burst throws none past it. */
	constexpr int32 MostSparks = 400;

	/** Where the player's eye is (none in a test level: nothing is widened then). */
	bool EyeOf(const UWorld* World, FVector& OutEye)
	{
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		if (Player && Player->PlayerCameraManager)
		{
			OutEye = Player->PlayerCameraManager->GetCameraLocation();
			return true;
		}
		return false;
	}

	/** The way across a disc whose up is Up at Angle (radians). */
	FVector AcrossDisc(const FVector& Up, float Angle)
	{
		FVector SideA, SideB;
		Up.FindBestAxisVectors(SideA, SideB);
		return SideA * FMath::Cos(Angle) + SideB * FMath::Sin(Angle);
	}
}

void ULootFanfareSubsystem::StartBurst(const FVector& Ground, const FLinearColor& Color, const FLegendaryBurst& Burst)
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets || Burst.Scale <= 0.f)
	{
		return;
	}
	FWeaponFX& FX = Bullets->GetEffects();
	FX.Initialize(World);
	const float Scale = Burst.Scale;
	const float Glow = Burst.Glow * FVfxTuning::Live().Glow;
	const FVector Up = FVector::UpVector;
	const FVector Heart = Ground + Up * Burst.Lift * Scale;
	const FLinearColor Hot = FMath::Lerp(Color, BurstWhite, 0.45f);
	FRandomStream Random(FMath::Rand());

	// The big shapes drawn smaller with the eye inside them (a Legendary at the player's feet): still a flash, not a whiteout.
	FVector Eye;
	const bool bEye = EyeOf(World, Eye);
	const float Near = bEye ? FMath::Clamp(static_cast<float>(FVector::Dist(Eye, Heart)) / FMath::Max(Burst.StarSize * Scale * CloseShare, 1.f),
		CloseLeast, 1.f) : 1.f;

	// The moment: a white crack, a star of its colour wider and slower, a ball of its light.
	FX.SpawnStarburst(Heart, Burst.FlashSize * Scale * Near, BurstWhite, FlashStrength * Glow, Burst.FlashSeconds, FlashSpikes);
	FX.SpawnStarburst(Heart, Burst.StarSize * Scale * Near, Color, StarStrength * Glow, Burst.StarSeconds, StarSpikes);
	FX.SpawnFireball(Heart, Up * CoreRise * Scale, Burst.CoreSize * CoreStartShare * Scale * Near, Burst.CoreSize * Scale * Near, Hot,
		CoreStrength * Glow, Burst.CoreSeconds);

	// Its reach: two rings over the ground, a shock in the air facing the eye.
	FX.SpawnShockRing(Ground + Up * 5.f, Up, RingStart * Scale, Burst.RingReach * Scale, RingWidth * Scale, Hot, RingStrength * Glow, Burst.RingSeconds);
	FX.SpawnShockRing(Ground + Up * 6.f, Up, OuterStart * Scale, Burst.OuterReach * Scale, OuterWidth * Scale, Color, OuterStrength * Glow,
		Burst.OuterSeconds);
	FX.SpawnShockRing(Heart, FVector::ZeroVector, ShockStart * Scale, Burst.ShockReach * Scale * Near, ShockWidth * Scale, BurstWhite,
		ShockStrength * Glow, Burst.ShockSeconds, true);

	// The galaxy: its disc tilted off the ground toward a way of its own, its ring, and its arms of star sparks in it.
	const float Yaw = Random.FRandRange(0.f, UE_TWO_PI);
	const float Tilt = FMath::DegreesToRadians(Burst.TiltDegrees);
	const FVector Disc = (Up * FMath::Cos(Tilt) + FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f) * FMath::Sin(Tilt)).GetSafeNormal();
	FX.SpawnShockRing(Heart, Disc, TiltStart * Scale, Burst.TiltReach * Scale, TiltWidth * Scale, Hot, TiltStrength * Glow, Burst.TiltSeconds);

	// The sparks draw by the numbers of the latest burst (a second Legendary's burst a moment later looks the same).
	SparkWidth = Burst.SparkWidth * Scale;
	SparkTrail = Burst.SparkTrail;
	SparkHead = Burst.SparkHead * Scale;
	SparkMinAngle = Burst.SparkMinAngle;
	SparkGlow = Glow;

	const float ArmReach = FMath::Max(Burst.ArmReach * Scale, 1.f);
	const float WindPerCm = UE_TWO_PI * Burst.ArmTurns / ArmReach;
	for (int32 Arm = 0; Arm < Burst.Arms; ++Arm)
	{
		const float ArmAngle = Yaw + UE_TWO_PI * static_cast<float>(Arm) / static_cast<float>(Burst.Arms);
		for (int32 Index = 0; Index < Burst.ArmSparks && NumBurstSparks() < MostSparks; ++Index)
		{
			// Strung along the arm from near the heart to its end, each a little off it.
			const float Along = (static_cast<float>(Index) + Random.FRandRange(0.2f, 1.f)) / static_cast<float>(FMath::Max(Burst.ArmSparks, 1));
			FArmSpark& Spark = ArmSparks.AddDefaulted_GetRef();
			Spark.Center = Heart;
			Spark.Up = Disc;
			Spark.Color = FMath::Lerp(Color, BurstWhite, Random.FRandRange(0.f, WhitestShare));
			Spark.Angle = ArmAngle + Random.FRandRange(-ArmScatter, ArmScatter);
			Spark.Reach = ArmReach * FMath::Lerp(ArmInnerShare, 1.f, Along);
			Spark.Height = Random.FRandRange(-1.f, 1.f) * DiscThickness * Spark.Reach;
			Spark.Wind = WindPerCm;
			Spark.Seconds = Burst.ArmSeconds * Random.FRandRange(LifeLeast, LifeMost);
			Spark.Seed = Random.FRandRange(0.f, UE_TWO_PI);
		}
	}

	// The fountain: thrown up from the heart, slowed by the air, falling back.
	for (int32 Index = 0; Index < Burst.Fountain && NumBurstSparks() < MostSparks; ++Index)
	{
		FFountainSpark& Spark = FountainSparks.AddDefaulted_GetRef();
		const FVector Way = (Up + Random.GetUnitVector() * FountainSpread).GetSafeNormal();
		Spark.Location = Heart;
		Spark.Velocity = Way * Burst.FountainSpeed * Scale * Random.FRandRange(FountainSlowest, 1.f);
		Spark.Color = FMath::Lerp(Color, BurstWhite, Random.FRandRange(0.f, WhitestShare));
		Spark.Seconds = Burst.FountainSeconds * Random.FRandRange(LifeLeast, LifeMost);
		Spark.Seed = Random.FRandRange(0.f, UE_TWO_PI);
	}
}

void ULootFanfareSubsystem::AdvanceBurst(float DeltaTime)
{
	if (ArmSparks.IsEmpty() && FountainSparks.IsEmpty())
	{
		return;
	}
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		ArmSparks.Reset();
		FountainSparks.Reset();
		return;
	}
	FWeaponFX& FX = Bullets->GetEffects();
	FX.Initialize(World);
	FVector Eye = FVector::ZeroVector;
	const bool bEye = EyeOf(World, Eye);

	// One spark this frame: its trail from Tail to Head and its twinkling head, at Fade of its strength, never thinner on
	// screen than the least angle (from 40 m a 6 cm trail is under a pixel).
	auto Draw = [this, &FX, &Eye, bEye](const FVector& Tail, const FVector& Head, const FLinearColor& Color, float Fade, float Time, float Seed)
	{
		const float Distance = bEye ? static_cast<float>(FVector::Dist(Eye, Head)) : 0.f;
		const float Least = Distance * SparkMinAngle;
		const float Twinkle = FMath::Lerp(TwinkleLeast, 1.f, 0.5f + 0.5f * FMath::Sin(Time * TwinkleRate + Seed));
		if (!Tail.Equals(Head, 0.5f))
		{
			FX.AddStreak(Tail, Head, FMath::Max(SparkWidth, Least), Color, SparkStrength * SparkGlow * Fade);
		}
		FX.AddGlow(Head, FMath::Max(SparkHead, Least * HeadOverTrail), Color, HeadStrength * SparkGlow * Fade * Twinkle);
	};

	// The arms: each spark's place worked out from its start, winding further round the further out it is.
	auto ArmPoint = [](const FArmSpark& Spark, float Time)
	{
		const float Along = FMath::Clamp(Time / Spark.Seconds, 0.f, 1.f);
		const float Out = 1.f - FMath::Pow(1.f - Along, ArmEase);
		const float Radius = Spark.Reach * Out;
		return Spark.Center + AcrossDisc(Spark.Up, Spark.Angle + Spark.Wind * Radius) * Radius + Spark.Up * Spark.Height * Out;
	};
	for (int32 Index = ArmSparks.Num() - 1; Index >= 0; --Index)
	{
		FArmSpark& Spark = ArmSparks[Index];
		Spark.Time += DeltaTime;
		if (Spark.Time >= Spark.Seconds)
		{
			ArmSparks.RemoveAtSwap(Index);
			continue;
		}
		const float Fade = FMath::Pow(1.f - Spark.Time / Spark.Seconds, SparkFade);
		Draw(ArmPoint(Spark, FMath::Max(Spark.Time - SparkTrail, 0.f)), ArmPoint(Spark, Spark.Time), Spark.Color, Fade, Spark.Time, Spark.Seed);
	}

	// The fountain: flown on (drag, gravity), its trail back along its way.
	const float Drag = FMath::Exp(-FountainDrag * DeltaTime);
	for (int32 Index = FountainSparks.Num() - 1; Index >= 0; --Index)
	{
		FFountainSpark& Spark = FountainSparks[Index];
		Spark.Time += DeltaTime;
		if (Spark.Time >= Spark.Seconds)
		{
			FountainSparks.RemoveAtSwap(Index);
			continue;
		}
		Spark.Velocity *= Drag;
		Spark.Velocity.Z -= FountainGravity * DeltaTime;
		Spark.Location += Spark.Velocity * DeltaTime;
		const float Fade = FMath::Pow(1.f - Spark.Time / Spark.Seconds, SparkFade);
		Draw(Spark.Location - Spark.Velocity * SparkTrail, Spark.Location, Spark.Color, Fade, Spark.Time, Spark.Seed);
	}
}
