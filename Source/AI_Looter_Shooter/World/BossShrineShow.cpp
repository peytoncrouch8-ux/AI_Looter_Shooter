// ABossShrine: its show and its frames: standing or rising out of the grave dirt, the bell's swing, the candles catching,
// wavering and going out, the summons' soul-light, and its tick's pace.

#include "World/BossShrine.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/MinimapSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The dust of the candles' smoke, the grave dirt's dust, the soul-light's pale gold (world colours, not the UI's). */
	const FLinearColor SmokeColor(0.30f, 0.29f, 0.28f);
	const FLinearColor DustColor(0.32f, 0.27f, 0.21f);
	const FLinearColor SoulColor(1.f, 0.86f, 0.6f);

	/** The bell's swing: its widest (degrees), how fast it swings (Hz) and dies away (s). */
	constexpr float BellSwingDegrees = 26.f;
	constexpr float BellSwingHz = 1.15f;
	constexpr float BellSettleSeconds = 1.4f;

	/** How near the player is for the lit candles to flicker by the frame (cm). */
	constexpr float FlickerRadius = 3000.f;
}


void ABossShrine::SetStanding(bool bStanding)
{
	SetActorHiddenInGame(!bStanding);
	SetActorEnableCollision(bStanding);
}

void ABossShrine::Rise(bool bShow)
{
	if (bRisen)
	{
		return;
	}
	bRisen = true;
	SetStanding(true);
	Tags.AddUnique(MinimapTags::Obstacle);
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->NoteBossShrineRisen(BossKey);
	}
	else
	{
		LocalRecord.bRisen = true;
	}
	if (!bShow)
	{
		Body->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		return;
	}
	// Out of the grave dirt: everything starts under the ground and comes up, grinding, the dirt thrown about its foot.
	bRising = true;
	RiseTime = 0.f;
	NextDirt = 0.f;
	TickRise(0.f);
	LooterSound::PlayAt(this, LooterSoundCue::Shrine::Rise, GetActorLocation());
	KickDirt(0.9f);
	BossCameraShake::Kick(this, GetActorLocation(), 0.18f, 0.9f);
	UE_LOG(LogLooter, Log, TEXT("%s: a vigil stone rises for %s."), *GetActorNameOrLabel(), *GetBossName().ToString());
	UpdateTickPace();
}

void ABossShrine::TickRise(float DeltaSeconds)
{
	if (!bRising)
	{
		return;
	}
	RiseTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(RiseTime / RiseSeconds, 0.f, 1.f);
	// Fast out of the ground, settling at the top; rocking a little as it comes, the rock dying away.
	const float Up = 1.f - FMath::Pow(1.f - Alpha, 3.f);
	const float Rock = (1.f - Alpha) * 1.6f;
	Body->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -RiseDepth * (1.f - Up)),
		FRotator(FMath::Sin(RiseTime * 17.f) * Rock, 0.f, FMath::Sin(RiseTime * 13.f + 1.f) * Rock));
	NextDirt -= DeltaSeconds;
	if (Alpha < 0.85f && NextDirt <= 0.f)
	{
		NextDirt = 0.3f;
		KickDirt(0.45f);
	}
	if (Alpha >= 1.f)
	{
		bRising = false;
		Body->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		KickDirt(0.35f);
		// Risen: if its boss can be called, the candles catch.
		Refresh();
	}
}

void ABossShrine::KickDirt(float Strength)
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FVector Foot = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	// Round its foot, a little off its sides: the earth it pushes up and aside.
	for (int32 Side = 0; Side < 4; ++Side)
	{
		const float Angle = Side * UE_HALF_PI + FMath::FRandRange(-0.4f, 0.4f);
		const FVector Out = Forward * FMath::Cos(Angle) * 55.f + Right * FMath::Sin(Angle) * 70.f;
		Effects.SpawnDirt(Foot + Out, (Out.GetSafeNormal() * 0.4f + FVector::UpVector).GetSafeNormal(), Strength);
	}
	Effects.SpawnDustPuff(Foot + FVector(0.0, 0.0, 20.0), FVector(0.0, 0.0, 40.0), DustColor, 0.45f * Strength, 60.f, 180.f, 2.2f);
}

void ABossShrine::SoulLight(const FVector& Where, float Size) const
{
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnDeathBurst(EDeathBurst::SoulLight, Where, FVector::UpVector, Size, SoulColor);
	}
}

void ABossShrine::SetCandles(bool bLit, bool bShow)
{
	bCandlesLit = bLit;
	Flames->SetVisibility(bLit && Flames->GetStaticMesh() != nullptr);
	CandleLight->SetVisibility(bLit);
	if (!bShow)
	{
		return;
	}
	if (bLit)
	{
		// They catch with a little flare.
		CandleFlare = FMath::Max(CandleFlare, 0.8f);
		return;
	}
	// Put out: a thread of smoke off each wick.
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		const FVector Wicks = CandleLight->GetComponentLocation();
		for (int32 Puff = 0; Puff < 5; ++Puff)
		{
			const FVector Off = GetActorRightVector() * FMath::FRandRange(-35.f, 35.f) + GetActorForwardVector() * FMath::FRandRange(-8.f, 8.f);
			Effects.SpawnDustPuff(Wicks + Off, FVector(0.0, 0.0, FMath::FRandRange(25.f, 45.f)), SmokeColor, 0.35f, 6.f, 40.f, 2.4f);
		}
	}
}

void ABossShrine::SwingBell(float Strength)
{
	BellStrength = FMath::Clamp(FMath::Max(Strength, BellStrength * FMath::Exp(-BellTime / BellSettleSeconds)), 0.f, 1.f);
	BellTime = 0.f;
	UpdateTickPace();
}

void ABossShrine::TickBell(float DeltaSeconds)
{
	if (BellStrength <= 0.f)
	{
		return;
	}
	BellTime += DeltaSeconds;
	const float Fade = FMath::Exp(-BellTime / BellSettleSeconds);
	if (Fade < 0.02f)
	{
		BellStrength = 0.f;
		Bell->SetRelativeRotation(BellRest);
		return;
	}
	// It swings side to side on its hook (about the crook's arm, the model's +X).
	const float Degrees = BellSwingDegrees * BellStrength * Fade * FMath::Sin(BellTime * UE_TWO_PI * BellSwingHz);
	Bell->SetRelativeRotation(BellRest * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Degrees)));
}

void ABossShrine::TickCandles(float DeltaSeconds)
{
	if (CandlesOutIn > 0.f)
	{
		CandlesOutIn -= DeltaSeconds;
		if (CandlesOutIn <= 0.f && bCandlesLit && Verdict.State != EBossShrineState::Ready)
		{
			SetCandles(false, /*bShow*/ true);
		}
	}
	CandleFlare = FMath::Max(CandleFlare - DeltaSeconds * 1.2f, 0.f);
	if (!bCandlesLit)
	{
		return;
	}
	FlickerClock += DeltaSeconds;
	// Candle flames waver: slow breathing and a quick flutter, brighter in a flare.
	const float Waver = 0.08f * FMath::PerlinNoise1D(FlickerClock * 1.7f) + 0.05f * FMath::PerlinNoise1D(FlickerClock * 9.f + 3.f);
	const float Candela = CandleCandela * (1.f + Waver + CandleFlare * 1.4f);
	if (!FMath::IsNearlyEqual(CandleLight->Intensity, Candela, 0.02f))
	{
		CandleLight->SetIntensity(Candela);
	}
}

void ABossShrine::PlaySummonShow(const FVector& ArenaSpot)
{
	// The bell rung hard, heard across the valley; the candles flare as the dead are called, and go out.
	LooterSound::PlayAt(this, LooterSoundCue::Shrine::Summon, Bell->GetComponentLocation());
	SwingBell(1.f);
	CandleFlare = 1.6f;
	CandlesOutIn = 0.7f;
	SoulLight(GetActorLocation() + FVector(0.0, 0.0, 70.0), 90.f);
	SoulLight(ArenaSpot + FVector(0.0, 0.0, 60.0), 180.f);
	BossCameraShake::Kick(this, GetActorLocation(), 0.25f, 0.6f);
	// Its boss answers from its arena a moment later.
	AnswerIn = 1.6f;
	AnswerAt = ArenaSpot;
	UpdateTickPace();
}

// ---------------------------------------------------------------------------
// Frame by frame
// ---------------------------------------------------------------------------

void ABossShrine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ABossShrine::Advance(float DeltaSeconds)
{
	SinceDefeat = FMath::Min(SinceDefeat + DeltaSeconds, 1.0e6f);
	TickRise(DeltaSeconds);
	TickBell(DeltaSeconds);
	TickCandles(DeltaSeconds);
	if (bAbelRematch)
	{
		TickAbelRematch();
	}
	if (AnswerIn > 0.f)
	{
		AnswerIn -= DeltaSeconds;
		if (AnswerIn <= 0.f && !AnswerCue.IsNone())
		{
			LooterSound::PlayAt(this, AnswerCue, AnswerAt);
		}
	}
	RefreshIn -= DeltaSeconds;
	if (RefreshIn <= 0.f)
	{
		RefreshIn = IdleTickSeconds;
		Refresh();
	}
}

void ABossShrine::UpdateTickPace()
{
	bool bNear = false;
	if (bCandlesLit)
	{
		const APawn* Player = FindPlayer();
		bNear = Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) < FMath::Square(FlickerRadius);
	}
	const bool bBusy = bRising || BellStrength > 0.f || CandleFlare > 0.f || CandlesOutIn > 0.f || AnswerIn > 0.f || bAbelRematch || bNear;
	const float Wanted = bBusy ? 0.f : IdleTickSeconds;
	if (!FMath::IsNearlyEqual(GetActorTickInterval(), Wanted))
	{
		SetActorTickInterval(Wanted);
	}
}
