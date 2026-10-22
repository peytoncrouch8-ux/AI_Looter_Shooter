#include "Embers/Powers/EmberSundown.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Core/LooterCharacter.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberVfx.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName PitchName(TEXT("Pitch"));

	/** The world's effective speed (its dilation with any demo's or sequence's on top), never zero. */
	float WorldSpeed(const AWorldSettings& Settings)
	{
		return FMath::Max(Settings.GetEffectiveTimeDilation(), 0.01f);
	}
}

EEmberDenied UEmberSundown::CanActivate(const FEmberContext& Context) const
{
	const UWorld* World = Context.Character ? Context.Character->GetWorld() : nullptr;
	const AWorldSettings* Settings = World ? World->GetWorldSettings() : nullptr;
	if (!Settings)
	{
		return EEmberDenied::Refused;
	}
	if (bSlowing)
	{
		return EEmberDenied::Active;
	}
	// Time is already bent (a boss's slow-motion death): that moment is its own.
	return Settings->TimeDilation < 0.999f ? EEmberDenied::Refused : EEmberDenied::None;
}

void UEmberSundown::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	AWorldSettings* Settings = World ? World->GetWorldSettings() : nullptr;
	if (!Settings)
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	const float Strength = Def && Def->Strength > 0.f ? FMath::Clamp(Def->Strength, 0.05f, 1.f) : DefaultStrength;
	Pitch = FMath::Clamp(Def ? Def->Extra(PitchName, DefaultPitch) : DefaultPitch, 0.3f, 1.f);
	Body = Character;

	// The world slows...
	PreviousDilation = Settings->TimeDilation;
	UGameplayStatics::SetGlobalTimeDilation(World, Strength);
	AppliedDilation = Settings->TimeDilation;
	// ...and the player, and all that acts for them, are put back to full speed.
	Compensation = 1.f / WorldSpeed(*Settings);
	bSlowing = true;
	Retimed.Reset();
	CompensatePlayer();
	// The world's sound sinks with it.
	UGameplayStatics::SetGlobalPitchModulation(World, Pitch, PitchGlide);

	LooterSound::Play2D(Character, LooterSoundCue::Ember::SundownStart);
	ShowStart();
	UE_LOG(LogLooter, Log, TEXT("Sundown: the world at %.2f, the player's clock x%.2f"), AppliedDilation, Compensation);
}

void UEmberSundown::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	const ACharacter* Character = Body.Get();
	const UWorld* World = Character ? Character->GetWorld() : nullptr;
	const AWorldSettings* Settings = World ? World->GetWorldSettings() : nullptr;
	if (!bSlowing || !Settings)
	{
		return;
	}
	// Something else took the clock meanwhile (a boss's slow-motion death): the player keeps to full speed under it.
	const float Wanted = 1.f / WorldSpeed(*Settings);
	if (!FMath::IsNearlyEqual(Wanted, Compensation, 0.001f))
	{
		Compensation = Wanted;
		for (const TPair<TWeakObjectPtr<AActor>, float>& Each : Retimed)
		{
			if (AActor* Actor = Each.Key.Get())
			{
				Actor->CustomTimeDilation = Compensation;
			}
		}
	}
	// A gun swapped in (or anything newly carried) runs at the player's speed too.
	CompensatePlayer();
}

void UEmberSundown::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bSlowing)
	{
		return;
	}
	bSlowing = false;
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World && Context.Character)
	{
		World = Context.Character->GetWorld();
	}
	// Only ours to undo: a clock someone else has taken since keeps their say.
	if (AWorldSettings* Settings = World ? World->GetWorldSettings() : nullptr)
	{
		if (FMath::IsNearlyEqual(Settings->TimeDilation, AppliedDilation, 0.001f))
		{
			UGameplayStatics::SetGlobalTimeDilation(World, PreviousDilation);
		}
	}
	for (const TPair<TWeakObjectPtr<AActor>, float>& Each : Retimed)
	{
		AActor* Actor = Each.Key.Get();
		if (Actor && FMath::IsNearlyEqual(Actor->CustomTimeDilation, Compensation, 0.001f))
		{
			Actor->CustomTimeDilation = Each.Value;
		}
	}
	Retimed.Reset();
	Compensation = 1.f;
	if (World)
	{
		// The sound comes back up whatever ended it (the audio device outlives the level).
		UGameplayStatics::SetGlobalPitchModulation(World, 1.f, World->bIsTearingDown ? 0.f : PitchGlide);
		if (!World->bIsTearingDown)
		{
			LooterSound::Play2D(World, LooterSoundCue::Ember::SundownEnd);
			if (APlayerController* Player = Character ? Cast<APlayerController>(Character->GetController()) : nullptr)
			{
				// Time catching up: a quick shove outward.
				FViewKick Catch;
				Catch.Pitch = 0.3f;
				Catch.FieldOfView = 0.03f;
				Catch.Frequency = 6.f;
				Catch.Damping = 0.6f;
				UCameraShakeModifier::Kick(Player, Catch);
			}
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Sundown: %s, the world back at %.2f"), bInterrupted ? TEXT("cut short") : TEXT("over"),
		World && World->GetWorldSettings() ? World->GetWorldSettings()->TimeDilation : 1.f);
}

void UEmberSundown::Retime(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}
	if (!Retimed.Contains(Actor))
	{
		Retimed.Add(Actor, Actor->CustomTimeDilation);
	}
	Actor->CustomTimeDilation = Compensation;
}

void UEmberSundown::CompensatePlayer()
{
	ACharacter* Character = Body.Get();
	if (!Character)
	{
		return;
	}
	Retime(Character);
	TArray<AActor*> Carried;
	Character->GetAttachedActors(Carried, true, true);
	for (AActor* Each : Carried)
	{
		Retime(Each);
	}
	AController* Controller = Character->GetController();
	Retime(Controller);
	if (APlayerController* Player = Cast<APlayerController>(Controller))
	{
		Retime(Player->PlayerCameraManager);
		Retime(Player->GetHUD());
	}
}

void UEmberSundown::ShowStart()
{
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Character)
	{
		return;
	}
	if (APlayerController* Player = Cast<APlayerController>(Character->GetController()))
	{
		// The view draws in slowly as the world sinks.
		FViewKick Sink;
		Sink.Pitch = -0.4f;
		Sink.FieldOfView = -0.04f;
		Sink.Frequency = 2.5f;
		Sink.Damping = 0.8f;
		UCameraShakeModifier::Kick(Player, Sink);
	}
	if (!Bullets)
	{
		return;
	}
	// A warm burst of dusk motes round the feet, rising slowly through the crawl, and a low ring of amber haze rolling out.
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FLinearColor Dusk(1.f, 0.45f, 0.08f);
	const float HalfHeight = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Feet = Character->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);
	Effects.SpawnDeathBurst(EDeathBurst::SoulLight, Feet + FVector(0.0, 0.0, 40.0), FVector::UpVector, 90.f, Dusk);
	// (The VFX pass) the dusk sweeping out: a wide amber ring racing over the ground, and its slower echo.
	EmberVfx::Activation(Effects, Feet, Dusk, 1.3f);
	FRandomStream Random(static_cast<int32>(GetTypeHash(Feet)));
	constexpr int32 Ring = 12;
	for (int32 Index = 0; Index < Ring; ++Index)
	{
		const float Angle = 2.f * UE_PI * (Index + Random.FRandRange(-0.3f, 0.3f)) / Ring;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Effects.SpawnDustPuff(Feet + Out * 40.f + FVector(0.0, 0.0, 15.0), Out * Random.FRandRange(260.f, 380.f) + FVector(0.0, 0.0, 30.0),
			Dusk * 0.55f, 0.3f, 30.f, Random.FRandRange(120.f, 170.f), Random.FRandRange(1.2f, 1.6f));
	}
}
