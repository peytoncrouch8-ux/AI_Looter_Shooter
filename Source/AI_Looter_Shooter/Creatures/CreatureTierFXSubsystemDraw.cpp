// UCreatureTierFXSubsystem's shows frame by frame: the soul-light pillar shooting up and thinning out (an Ethereal's
// flickering), the rings racing out over the ground, the motes rising round the pillar, and the loot's pop (its sound, a
// flash and a fountain of glints). Everything but the pillars is FWeaponFX's streaks and glows, redrawn each frame.

#include "Creatures/CreatureTierFXSubsystem.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The pillar shoots up from a stub past its height in this long, settles back over the next TierPillarSettle. */
	/** The motes rise off a ring this many pillar radii round its foot at most. */
	constexpr float TierMoteSpread = 2.6f;
	constexpr float TierPillarRise = 0.2f;
	constexpr float TierPillarSettle = 0.4f;
	constexpr float TierPillarStub = 0.15f;
	constexpr float TierPillarOvershoot = 1.08f;
	/** Its flash as it goes up (a multiple of its glow, dying away), and its width: wide at the start, a thread at the end. */
	constexpr float TierPillarFlash = 2.5f;
	constexpr float TierPillarFlashSeconds = 0.12f;
	constexpr float TierPillarWideStart = 1.5f;
	constexpr float TierPillarThinEnd = 0.55f;
	/** It fades out over the show's later part (shares of the show). */
	constexpr float TierPillarFadeFrom = 0.45f;
	/** Its white-hot core: the tier's colour this far toward white. */
	constexpr float TierCoreWhite = 0.6f;

	/** The rings: a bright line narrowing as it runs, a soft halo round it. A tracer's glow is 30, the level-up ring's 7. */
	constexpr float TierRingStart = 40.f;
	constexpr float TierRingCoreWidthStart = 6.f;
	constexpr float TierRingCoreWidthEnd = 3.f;
	constexpr float TierRingHaloWidth = 22.f;
	constexpr float TierRingGlow = 9.f;
	constexpr float TierRingHaloShare = 0.18f;
	constexpr float TierRingOverGround = 4.f;
	constexpr float TierRingFadeIn = 0.05f;
	constexpr float TierRingOverlap = 0.08f;
	/** The Ethereal's second ring: this long after the first, this strong. */
	constexpr float TierSecondRingDelay = 0.28f;
	constexpr float TierSecondRingStrength = 0.7f;

	/** Motes: their glow, length (cm), how fast they slow, how far they sway; dimmed this near the eye. */
	constexpr float TierMoteGlow = 10.f;
	constexpr float TierMoteLength = 6.f;
	constexpr float TierMoteDrag = 0.5f;
	constexpr float TierMoteSway = 16.f;
	constexpr float TierNearEye = 30.f;
	constexpr float TierNearEyeFade = 70.f;
	/** Hot motes and glints: the tier's colour this far toward white. */
	constexpr float TierHotWhite = 0.55f;

	/** Glints: their glow, the pull down on them, their drag, their streak per speed (cm per cm/s) and its bounds. */
	constexpr float TierGlintGlow = 14.f;
	constexpr float TierGlintGravity = 600.f;
	constexpr float TierGlintDrag = 1.2f;
	constexpr float TierGlintStretch = 0.02f;
	constexpr float TierGlintShortest = 4.f;
	constexpr float TierGlintLongest = 14.f;
	/** The pop's flash: its glow and its size (cm) as it shrinks away over TierPopFlashSeconds. */
	constexpr float TierPopFlashGlow = 18.f;
	constexpr float TierPopFlashFrom = 90.f;
	constexpr float TierPopFlashTo = 20.f;
	constexpr float TierPopFlashSeconds = 0.25f;

	float TierEaseOut(float Alpha)
	{
		const float Left = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Left * Left * Left;
	}

	FVector2D TierRingWay(int32 Spoke)
	{
		const float Angle = UE_TWO_PI * static_cast<float>(Spoke) / static_cast<float>(UCreatureTierFXSubsystem::RingSegments);
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}

	/** A pillar's look this frame: its cylinder scaled and placed on Base (LightBeams' 100 x 100 cylinder), its glow. */
	void TierPlacePillar(UStaticMeshComponent* Beam, const FVector& Base, float Height, float Radius, float Glow, const FLinearColor& Color)
	{
		if (!Beam)
		{
			return;
		}
		const bool bShown = Glow > 0.01f && Height > 1.f && Radius > 0.1f;
		if (Beam->IsVisible() != bShown)
		{
			Beam->SetVisibility(bShown);
		}
		if (!bShown)
		{
			return;
		}
		Beam->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, Height / 100.f));
		Beam->SetRelativeLocation(Base + FVector(0.0, 0.0, Height * 0.5f));
		if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Beam->GetMaterial(0)))
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetScalarParameterValue(TEXT("Glow"), Glow);
			// The glow fades over the beam's height: keep the fade on the pillar as it grows.
			Material->SetScalarParameterValue(TEXT("GradHeight"), Height);
		}
	}
}

void UCreatureTierFXSubsystem::Advance(float DeltaSeconds)
{
	StreaksDrawn = 0;
	if (Shows.IsEmpty())
	{
		return;
	}
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	FWeaponFX* Effects = Bullets ? &Bullets->GetEffects() : nullptr;
	const FVector Camera = CameraLocation();
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	for (int32 Index = Shows.Num() - 1; Index >= 0; --Index)
	{
		FTierShow& Show = Shows[Index];
		Show.Time += Dt;
		if (Show.Time >= Show.Look.Seconds)
		{
			// Over: its pillars back in the pool, and nothing ticks once the last has gone.
			HidePillars(Show.Slot);
			Shows.RemoveAt(Index);
			continue;
		}
		if (Show.bLootPop && !Show.bPopped && Show.Time >= Show.Look.PopDelay)
		{
			PlayPop(Show);
		}
		DrawPillars(Show);
		if (Effects)
		{
			DrawRings(*Effects, Show);
			DrawSparks(*Effects, Show, Dt, Camera);
		}
	}
}

void UCreatureTierFXSubsystem::PlayPop(FTierShow& Show)
{
	// The loot bursting out of what's left of it: its own sound where the drops fly out, a beat after the death's.
	Show.bPopped = true;
	++PopsPlayed;
	LooterSound::PlayAt(this, CreatureTierLook::PopCue(Show.Rank), Show.LootSpot);
}

void UCreatureTierFXSubsystem::DrawPillars(FTierShow& Show)
{
	const FCreatureTierDeathShow& Look = Show.Look;
	const float T = Show.Time;
	const float Share = FMath::Clamp(T / FMath::Max(Look.Seconds, 0.01f), 0.f, 1.f);
	// Up fast from a stub past its height, then settling back; wide and blinding at first, a thinning thread at the end.
	const float Lift = T < TierPillarRise
		? FMath::Lerp(TierPillarStub, TierPillarOvershoot, TierEaseOut(T / TierPillarRise))
		: FMath::Lerp(TierPillarOvershoot, 1.f, FMath::SmoothStep(TierPillarRise, TierPillarRise + TierPillarSettle, T));
	const float Flash = 1.f + TierPillarFlash * FMath::Exp(-T / TierPillarFlashSeconds);
	const float Fade = 1.f - FMath::SmoothStep(TierPillarFadeFrom, 1.f, Share);
	const float Width = FMath::Lerp(TierPillarWideStart, TierPillarThinEnd, FMath::SmoothStep(0.f, 1.f, Share));
	// A ghost's light is never steady: two quick wavers and a slow breath under them.
	const float Flicker = Look.bFlicker ? 0.72f + 0.16f * FMath::Sin(T * 37.f + 1.3f) + 0.08f * FMath::Sin(T * 91.f) + 0.04f * FMath::Sin(T * 5.f)
		: 1.f;
	const float Height = Look.PillarHeight * Lift;
	const float Strength = Flash * Fade * Flicker;
	TierPlacePillar(PillarFor(Show.Slot, false), Show.Feet, Height, Look.PillarRadius * Width, Look.PillarGlow * Strength, Show.Color);
	TierPlacePillar(PillarFor(Show.Slot, true), Show.Feet, Height * 0.92f, Look.CoreRadius * Width, Look.CoreGlow * Strength,
		FMath::Lerp(Show.Color, FLinearColor::White, TierCoreWhite));
}

void UCreatureTierFXSubsystem::DrawRings(FWeaponFX& Effects, const FTierShow& Show)
{
	const FCreatureTierDeathShow& Look = Show.Look;
	const FLinearColor Hot = FMath::Lerp(Show.Color, FLinearColor::White, TierHotWhite);
	auto RingPoint = [&Show, &Look](int32 Spoke, float Radius)
	{
		const int32 Index = ((Spoke % RingSegments) + RingSegments) % RingSegments;
		const float Edge = Show.EdgeHeight.IsValidIndex(Index) ? Show.EdgeHeight[Index] : 0.f;
		const FVector2D Way = TierRingWay(Index);
		const float Out = Look.RingRadius > 0.f ? FMath::Clamp(Radius / Look.RingRadius, 0.f, 1.f) : 0.f;
		return Show.Feet + FVector(Way.X * Radius, Way.Y * Radius, FMath::Lerp(0.f, Edge, Out) + TierRingOverGround);
	};
	auto DrawRing = [&](float Delay, float Strength0)
	{
		const float T = Show.Time - Delay;
		if (T < 0.f || T >= Look.RingSeconds || Look.RingSeconds <= 0.f)
		{
			return;
		}
		const float Share = T / Look.RingSeconds;
		const float Radius = FMath::Lerp(TierRingStart, Look.RingRadius, TierEaseOut(Share));
		const float Strength = FMath::Clamp(T / TierRingFadeIn, 0.f, 1.f) * FMath::Pow(1.f - Share, 1.4f) * Strength0;
		const float CoreWidth = FMath::Lerp(TierRingCoreWidthStart, TierRingCoreWidthEnd, Share);
		for (int32 Spoke = 0; Spoke < RingSegments; ++Spoke)
		{
			const FVector From = RingPoint(Spoke, Radius);
			const FVector To = RingPoint(Spoke + 1, Radius);
			const FVector Past = (To - From) * TierRingOverlap;
			Effects.AddStreak(From - Past, To + Past, CoreWidth, Hot, TierRingGlow * Strength);
			Effects.AddStreak(From - Past, To + Past, TierRingHaloWidth, Show.Color, TierRingGlow * TierRingHaloShare * Strength);
			StreaksDrawn += 2;
		}
	};
	DrawRing(0.f, 1.f);
	if (Look.bSecondRing)
	{
		DrawRing(TierSecondRingDelay, TierSecondRingStrength);
	}
}

void UCreatureTierFXSubsystem::DrawSparks(FWeaponFX& Effects, FTierShow& Show, float DeltaSeconds, const FVector& Camera)
{
	const FLinearColor Hot = FMath::Lerp(Show.Color, FLinearColor::White, TierHotWhite);
	auto NearEye = [&Camera](const FVector& Where)
	{
		return FMath::Clamp((static_cast<float>(FVector::Dist(Where, Camera)) - TierNearEye) / TierNearEyeFade, 0.f, 1.f);
	};

	// Motes: rising off the pillar's foot, slowing and swaying as they dwindle.
	for (FTierSpark& Mote : Show.Motes)
	{
		if (!Mote.bLaunched && Show.Time >= Mote.Born)
		{
			const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
			const float Out = Random.FRandRange(0.3f, 1.f) * Show.Look.PillarRadius * TierMoteSpread;
			const FVector Way(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			Mote.Location = Show.Feet + Way * Out + FVector(0.0, 0.0, Random.FRandRange(5.f, Show.Size * 0.6f));
			Mote.Velocity = Way * Random.FRandRange(10.f, 40.f) + FVector(0.0, 0.0, Random.FRandRange(140.f, 300.f));
			Mote.bLaunched = true;
		}
		if (!Mote.bLaunched)
		{
			continue;
		}
		const float Age = Show.Time - Mote.Born;
		const FVector Sway = FVector(-Mote.Velocity.Y, Mote.Velocity.X, 0.0).GetSafeNormal()
			* (TierMoteSway * FMath::Sin(Age * 6.f + Mote.Phase));
		Mote.Velocity *= FMath::Exp(-TierMoteDrag * DeltaSeconds);
		Mote.Location += (Mote.Velocity + Sway) * DeltaSeconds;
		if (Age >= Mote.Life)
		{
			continue;
		}
		const float Flicker = 0.8f + 0.2f * FMath::Sin(Age * 23.f + Mote.Phase);
		const float Glow = TierMoteGlow * Mote.Glow * Flicker * NearEye(Mote.Location) * FMath::Clamp(Age / 0.1f, 0.f, 1.f)
			* FMath::Pow(1.f - Age / Mote.Life, 1.3f);
		const FVector Along = Mote.Velocity.IsNearlyZero() ? FVector::UpVector : Mote.Velocity.GetSafeNormal();
		Effects.AddStreak(Mote.Location - Along * TierMoteLength, Mote.Location, Mote.Width, Mote.bHot ? Hot : Show.Color, Glow);
		++StreaksDrawn;
	}

	if (!Show.bPopped)
	{
		return;
	}
	// The pop's flash where the drops fly out, shrinking away...
	const float SincePop = Show.Time - Show.Look.PopDelay;
	if (SincePop >= 0.f && SincePop < TierPopFlashSeconds)
	{
		const float Share = SincePop / TierPopFlashSeconds;
		Effects.AddGlow(Show.LootSpot, FMath::Lerp(TierPopFlashFrom, TierPopFlashTo, Share), Hot,
			TierPopFlashGlow * (1.f - Share) * NearEye(Show.LootSpot));
		++StreaksDrawn;
	}
	// ...and its glints flung up and out, falling back as they burn out.
	for (FTierSpark& Glint : Show.Glints)
	{
		if (!Glint.bLaunched && Show.Time >= Glint.Born)
		{
			const FVector Direction = (Random.GetUnitVector() * 0.8f + FVector::UpVector * 1.2f).GetSafeNormal();
			Glint.Velocity = Direction * Random.FRandRange(380.f, 700.f);
			Glint.Location = Show.LootSpot;
			Glint.bLaunched = true;
		}
		if (!Glint.bLaunched)
		{
			continue;
		}
		const float Age = Show.Time - Glint.Born;
		Glint.Velocity.Z -= TierGlintGravity * DeltaSeconds;
		Glint.Velocity *= FMath::Exp(-TierGlintDrag * DeltaSeconds);
		Glint.Location += Glint.Velocity * DeltaSeconds;
		if (Age >= Glint.Life)
		{
			continue;
		}
		const float Speed = static_cast<float>(Glint.Velocity.Size());
		const float Length = FMath::Clamp(Speed * TierGlintStretch, TierGlintShortest, TierGlintLongest);
		const FVector Along = Speed > 1.f ? Glint.Velocity / Speed : FVector::UpVector;
		const float Glow = TierGlintGlow * Glint.Glow * NearEye(Glint.Location) * FMath::Pow(1.f - Age / Glint.Life, 1.2f);
		Effects.AddStreak(Glint.Location - Along * Length, Glint.Location, Glint.Width, Glint.bHot ? Hot : Show.Color, Glow);
		++StreaksDrawn;
	}
}
