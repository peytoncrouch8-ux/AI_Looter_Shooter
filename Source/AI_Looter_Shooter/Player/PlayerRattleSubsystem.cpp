#include "Player/PlayerRattleSubsystem.h"
#include "Combat/MovementSlowComponent.h"
#include "Player/CameraShakeModifier.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UPlayerRattleSubsystem: the player rattled by a scream (2026-10-29, the Unpaid's failed-lunge shriek). The sway moves the
// real aim (the control rotation), a little and briefly, and gives back every degree it took, so a rattle never leaves the
// aim off where the player put it; the slow is the shriek ring's slow component, never a stun.

namespace
{
	/** The wobble's strongest scene fringe (the engine's chromatic aberration, 0-5): a flicker of colour at the edges. */
	constexpr float FringeMost = 3.5f;
	/** It wobbles this fast (Hz). */
	constexpr float FringeHz = 7.f;
	/** A rattle's frost rises over this long (s). */
	constexpr float RiseSeconds = 0.06f;
}

UPlayerRattleSubsystem* UPlayerRattleSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UPlayerRattleSubsystem>() : nullptr;
}

bool UPlayerRattleSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

float UPlayerRattleSubsystem::Envelope(float Age, float Seconds)
{
	if (Seconds <= 0.f || Age < 0.f || Age >= Seconds)
	{
		return 0.f;
	}
	const float In = FMath::Clamp(Age / RiseSeconds, 0.f, 1.f);
	// It holds a moment, then eases out over the rest.
	const float Out = 1.f - FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp((Age - 0.25f * Seconds) / (0.75f * Seconds), 0.f, 1.f), 2.f);
	return In * Out;
}

FRotator UPlayerRattleSubsystem::SwayAt(float Age, float Seconds, float Degrees, float Seed)
{
	if (Seconds <= 0.f || Age <= 0.f || Age >= Seconds)
	{
		return FRotator::ZeroRotator;
	}
	// Two uneven wobbles, yaw wider than pitch, under a hump that is 0 at both ends: the aim comes back where it was.
	const float Alpha = Age / Seconds;
	const float Hump = FMath::Sin(UE_PI * Alpha) * (1.f - 0.4f * Alpha);
	const float Yaw = FMath::Sin(2.f * UE_PI * (2.3f * Alpha) + Seed * 6.f) + 0.45f * FMath::Sin(2.f * UE_PI * (4.1f * Alpha) + Seed * 3.f);
	const float Pitch = FMath::Sin(2.f * UE_PI * (1.7f * Alpha) + Seed * 4.f + 1.3f);
	return FRotator(0.6f * Degrees * Hump * Pitch, Degrees * Hump * Yaw / 1.45f, 0.f);
}

float UPlayerRattleSubsystem::ShakeSetting(const APawn* Pawn)
{
	const APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const ULocalPlayer* Local = Player ? Player->GetLocalPlayer() : nullptr;
	const UControlSettingsSubsystem* Controls = Local ? Local->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
	return Controls ? Controls->GetCameraShake() : 1.f;
}

void UPlayerRattleSubsystem::Rattle(APawn* Pawn, const FVector& Source, const FPlayerRattle& How)
{
	APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!Player || !Player->IsLocalController() || How.Strength <= 0.f)
	{
		return;
	}
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	FRattled* Found = Rattles.FindByPredicate([Pawn](const FRattled& Each) { return Each.Pawn.Get() == Pawn; });
	FRattled& Rattled = Found ? *Found : Rattles.AddDefaulted_GetRef();
	Rattled.Pawn = Pawn;
	Rattled.Source = Source;
	// A stronger rattle on top of one still showing wins; a weaker one only renews the arc and frost.
	const bool bStronger = !Found || How.Strength >= Rattled.How.Strength * Envelope(Rattled.Age, Rattled.How.FrostSeconds);
	if (bStronger)
	{
		const double KeptSway = Rattled.LastSway;
		const float KeptSwayAge = Rattled.SwayAge;
		const FRotator KeptShown = Rattled.SwayShown;
		Rattled.How = How;
		Rattled.LastSway = KeptSway;
		Rattled.SwayAge = KeptSwayAge;
		Rattled.SwayShown = KeptShown;
	}
	Rattled.Age = 0.f;
	Rattled.Seed = FMath::FRand();
	const float Strength = FMath::Clamp(How.Strength, 0.f, 1.f);

	// The sway and the slow come once a gap, however many scream: it punishes being caught near, it never locks the player up.
	if (Now - Rattled.LastSway >= How.Gap && (How.bSway || How.bSlow))
	{
		Rattled.LastSway = Now;
		if (How.bSway && How.SwayDegrees > 0.f)
		{
			Rattled.SwayAge = 0.f;
		}
		if (How.bSlow && How.SlowSeconds > 0.f)
		{
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				UMovementSlowComponent::Apply(Character, FMath::Lerp(1.f, How.SlowShare, Strength), How.SlowSeconds);
			}
		}
	}
	if (How.Shake > 0.f)
	{
		if (UCameraShakeModifier* Shaker = UCameraShakeModifier::FindOrAdd(Player))
		{
			Shaker->AddShake(How.Shake * Strength, How.ShakeSeconds);
		}
	}
}

bool UPlayerRattleSubsystem::GetRattle(const APawn* Pawn, float& OutFrost, FVector& OutSource, float& OutAge) const
{
	const FRattled* Found = Pawn ? Rattles.FindByPredicate([Pawn](const FRattled& Each) { return Each.Pawn.Get() == Pawn; }) : nullptr;
	if (!Found)
	{
		return false;
	}
	OutFrost = Envelope(Found->Age, Found->How.FrostSeconds) * FMath::Clamp(Found->How.Strength, 0.f, 1.f);
	OutSource = Found->Source;
	OutAge = Found->Age;
	return Found->Age < FMath::Max(Found->How.FrostSeconds, ArcSeconds);
}

void UPlayerRattleSubsystem::ShowFringe(FRattled& Rattled, float Amount)
{
	APawn* Pawn = Rattled.Pawn.Get();
	if (Amount > 0.f && Rattled.Cameras.IsEmpty() && Pawn)
	{
		// Every camera of the player's (first person and the shoulder's), each with its own setting kept.
		TInlineComponentArray<UCameraComponent*> Cameras(Pawn);
		for (UCameraComponent* Camera : Cameras)
		{
			FFringeCamera& Kept = Rattled.Cameras.AddDefaulted_GetRef();
			Kept.Camera = Camera;
			Kept.bHadOverride = Camera->PostProcessSettings.bOverride_SceneFringeIntensity;
			Kept.Fringe = Camera->PostProcessSettings.SceneFringeIntensity;
		}
	}
	for (const FFringeCamera& Kept : Rattled.Cameras)
	{
		UCameraComponent* Camera = Kept.Camera.Get();
		if (!Camera)
		{
			continue;
		}
		if (Amount <= 0.f)
		{
			Camera->PostProcessSettings.bOverride_SceneFringeIntensity = Kept.bHadOverride;
			Camera->PostProcessSettings.SceneFringeIntensity = Kept.Fringe;
			continue;
		}
		const float Before = Kept.bHadOverride ? Kept.Fringe : 0.f;
		Camera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
		Camera->PostProcessSettings.SceneFringeIntensity = FMath::Max(Before, Amount);
	}
	if (Amount <= 0.f)
	{
		Rattled.Cameras.Reset();
	}
}

void UPlayerRattleSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (int32 Index = Rattles.Num() - 1; Index >= 0; --Index)
	{
		FRattled& Rattled = Rattles[Index];
		APawn* Pawn = Rattled.Pawn.Get();
		Rattled.Age += DeltaTime;
		const float Setting = ShakeSetting(Pawn);
		// The aim's sway: this frame's offset less what was laid before, so the aim ends where it began.
		APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		if (Rattled.SwayAge >= 0.f)
		{
			Rattled.SwayAge += DeltaTime;
			const float Strength = FMath::Clamp(Rattled.How.Strength, 0.f, 1.f);
			const FRotator Wanted = SwayAt(Rattled.SwayAge, Rattled.How.SwaySeconds, Rattled.How.SwayDegrees * Strength * Setting, Rattled.Seed);
			if (Player)
			{
				Player->SetControlRotation(Player->GetControlRotation() + (Wanted - Rattled.SwayShown));
			}
			Rattled.SwayShown = Wanted;
			if (Rattled.SwayAge >= Rattled.How.SwaySeconds)
			{
				Rattled.SwayAge = -1.f;
				Rattled.SwayShown = FRotator::ZeroRotator;
			}
		}
		// The wobble: colour fringes flickering at the edges while the frost shows.
		const float Frost = Envelope(Rattled.Age, Rattled.How.FrostSeconds) * FMath::Clamp(Rattled.How.Strength, 0.f, 1.f);
		const float Wobble = Frost * Setting * FringeMost * (0.65f + 0.35f * FMath::Sin(2.f * UE_PI * FringeHz * Rattled.Age));
		ShowFringe(Rattled, Pawn ? Wobble : 0.f);
		const bool bDone = !Pawn || (Rattled.Age >= FMath::Max(Rattled.How.FrostSeconds, ArcSeconds) && Rattled.SwayAge < 0.f);
		if (bDone)
		{
			ShowFringe(Rattled, 0.f);
			Rattles.RemoveAtSwap(Index);
		}
	}
}

bool UPlayerRattleSubsystem::IsTickable() const
{
	return !Rattles.IsEmpty();
}

TStatId UPlayerRattleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayerRattleSubsystem, STATGROUP_Tickables);
}

void UPlayerRattleSubsystem::Deinitialize()
{
	for (FRattled& Rattled : Rattles)
	{
		ShowFringe(Rattled, 0.f);
	}
	Rattles.Reset();
	Super::Deinitialize();
}
