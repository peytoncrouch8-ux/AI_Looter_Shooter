#include "Bosses/BossCameraShake.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

float BossCameraShake::Falloff(float Strength, float Distance, float Radius)
{
	if (Radius <= 0.f)
	{
		return Strength;
	}
	const float Near = Radius * 0.2f;
	const float Share = 1.f - FMath::Clamp((Distance - Near) / FMath::Max(Radius - Near, 1.f), 0.f, 1.f);
	return Strength * Share * Share;
}

void BossCameraShake::Kick(const UObject* WorldContext, const FVector& Source, float Strength, float Seconds, float Radius)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || !World->IsGameWorld() || Strength <= 0.f)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		const APlayerCameraManager* Camera = Controller && Controller->IsLocalController() ? Controller->PlayerCameraManager.Get() : nullptr;
		if (!Camera)
		{
			continue;
		}
		// Felt from where the player stands (the view may be a scene's camera far off; the shake is the player's).
		const APawn* Pawn = Controller->GetPawn();
		const FVector Where = Pawn ? Pawn->GetActorLocation() : Camera->GetCameraLocation();
		const float Felt = Falloff(Strength, static_cast<float>(FVector::Dist(Where, Source)), Radius);
		if (Felt <= 0.01f)
		{
			continue;
		}
		if (UCameraShakeModifier* Shaker = UCameraShakeModifier::FindOrAdd(Controller))
		{
			Shaker->AddShake(Felt, Seconds);
		}
	}
}
