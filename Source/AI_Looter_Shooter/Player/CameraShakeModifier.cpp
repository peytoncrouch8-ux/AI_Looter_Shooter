#include "Player/CameraShakeModifier.h"
#include "AI_Looter_Shooter.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** How fast the shakes' noise runs (its features a second): quick enough to read as a jolt, not a sway. */
	constexpr float ShakeFrequency = 16.f;

	/** The noise for one channel at Time: each channel reads its own stretch of the same smooth noise. */
	float ShakeNoise(float Time, int32 Channel)
	{
		return FMath::PerlinNoise1D(Time + 31.7f * static_cast<float>(Channel + 1));
	}

	/**
	 * Looter.CameraShake [0-1]: the camera shake setting for this run (every local player's; the settings menu's slider
	 * saves it), or with no number, what it is.
	 */
	void CameraShakeCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* Player = It->Get();
			const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
			UControlSettingsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
			if (!Controls)
			{
				continue;
			}
			if (Args.Num() > 0)
			{
				Controls->SetCameraShake(FCString::Atof(*Args[0]), /*bSave*/ false);
			}
			UE_LOG(LogLooter, Display, TEXT("Camera shake: %d%%"), FMath::RoundToInt32(Controls->GetCameraShake() * 100.f));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs CameraShakeConsoleCommand(
		TEXT("Looter.CameraShake"),
		TEXT("Looter.CameraShake [0-1]: scales every shake and kick of the view (0 turns them off), as the settings' Camera shake ")
		TEXT("slider does, for this run; with no number, prints it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CameraShakeCommand));
}

UCameraShakeModifier* UCameraShakeModifier::FindOrAdd(APlayerController* Player)
{
	APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
	if (!Camera)
	{
		return nullptr;
	}
	if (UCameraShakeModifier* Found = Cast<UCameraShakeModifier>(Camera->FindCameraModifierByClass(UCameraShakeModifier::StaticClass())))
	{
		return Found;
	}
	return Cast<UCameraShakeModifier>(Camera->AddNewCameraModifier(UCameraShakeModifier::StaticClass()));
}

void UCameraShakeModifier::Kick(APlayerController* Player, const FViewKick& Kick)
{
	if (!Player || !Player->IsLocalController())
	{
		return;
	}
	if (UCameraShakeModifier* Modifier = FindOrAdd(Player))
	{
		Modifier->AddKick(Kick);
	}
}

void UCameraShakeModifier::AddShake(float Strength, float Seconds)
{
	if (Strength <= 0.f || Seconds <= 0.f)
	{
		return;
	}
	if (Shakes.Num() >= MaxShakes)
	{
		// The one nearest its end gives way.
		int32 Oldest = 0;
		for (int32 Index = 1; Index < Shakes.Num(); ++Index)
		{
			if (Shakes[Index].Life - Shakes[Index].Age < Shakes[Oldest].Life - Shakes[Oldest].Age)
			{
				Oldest = Index;
			}
		}
		Shakes.RemoveAtSwap(Oldest);
	}
	FShake& Added = Shakes.AddDefaulted_GetRef();
	Added.Strength = FMath::Min(Strength, 1.f);
	Added.Life = Seconds;
}

float UCameraShakeModifier::GetAmount() const
{
	float Amount = 0.f;
	for (const FShake& Shake : Shakes)
	{
		const float Left = 1.f - FMath::Clamp(Shake.Age / FMath::Max(Shake.Life, KINDA_SMALL_NUMBER), 0.f, 1.f);
		Amount += Shake.Strength * Left * Left;
	}
	return FMath::Min(Amount, MaxAmount);
}

float UCameraShakeModifier::GetShakeScale() const
{
	const APlayerController* Player = CameraOwner ? CameraOwner->GetOwningPlayerController() : nullptr;
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	const UControlSettingsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
	return Controls ? Controls->GetCameraShake() : 1.f;
}

bool UCameraShakeModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	// Both kinds age whatever the setting, so turning it back up never replays an old shake.
	Clock += DeltaTime;
	for (int32 Index = Shakes.Num() - 1; Index >= 0; --Index)
	{
		Shakes[Index].Age += DeltaTime;
		if (Shakes[Index].Age >= Shakes[Index].Life)
		{
			Shakes.RemoveAtSwap(Index);
		}
	}
	Kicks.Tick(DeltaTime);

	const float Scale = GetShakeScale();
	if (Scale <= 0.f)
	{
		return false;
	}
	const float Amount = GetAmount() * Scale;
	if (Amount > 0.f)
	{
		const float Time = Clock * ShakeFrequency;
		InOutPOV.Rotation += FRotator(ShakeNoise(Time, 0) * MaxPitch, ShakeNoise(Time, 1) * MaxYaw, ShakeNoise(Time, 2) * MaxRoll) * Amount;
		InOutPOV.Location += FVector(ShakeNoise(Time, 3), ShakeNoise(Time, 4), ShakeNoise(Time, 5)) * (MaxShift * Amount);
	}
	if (!Kicks.IsSettled())
	{
		InOutPOV.Rotation += Kicks.GetRotation() * Scale;
		// Wider or narrower by a share of whatever the view is (a sight's zoom included), so a kick feels the same zoomed in.
		InOutPOV.FOV = FMath::Clamp(InOutPOV.FOV * (1.f + Kicks.GetFieldOfView() * Scale), 5.f, 170.f);
	}
	// The modifiers after this one still run.
	return false;
}
