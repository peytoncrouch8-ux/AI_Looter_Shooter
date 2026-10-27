// FBossIntroGuard: the player untouchable through a boss intro and a moment after it.

#include "Scenes/BossIntroGuard.h"
#include "Combat/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

void FBossIntroGuard::Begin(APawn* InPlayer)
{
	UHealthComponent* Guarded = InPlayer ? InPlayer->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!Guarded || bGuarding)
	{
		return;
	}
	Player = InPlayer;
	Health = Guarded;
	bWasInvulnerable = Guarded->bInvulnerable;
	Guarded->bInvulnerable = true;
	bGuarding = true;
}

void FBossIntroGuard::EndAfter(float Seconds)
{
	if (!bGuarding)
	{
		return;
	}
	bGuarding = false;
	APawn* Guarded = Player.Get();
	UHealthComponent* Kept = Health.Get();
	const bool bInvulnerableBefore = bWasInvulnerable;
	UWorld* World = Guarded ? Guarded->GetWorld() : nullptr;
	if (!Guarded || !World || Seconds <= 0.f)
	{
		if (Kept)
		{
			Kept->bInvulnerable = bInvulnerableBefore;
		}
		return;
	}
	// The scene gave CanBeDamaged back as it let them go: that's theirs, held off again until the grace ends.
	const bool bCouldBeDamaged = Guarded->CanBeDamaged();
	Guarded->SetCanBeDamaged(false);
	const TWeakObjectPtr<APawn> WeakPlayer = Guarded;
	const TWeakObjectPtr<UHealthComponent> WeakHealth = Kept;
	FTimerHandle Grace;
	World->GetTimerManager().SetTimer(Grace, FTimerDelegate::CreateLambda([WeakPlayer, WeakHealth, bInvulnerableBefore, bCouldBeDamaged]()
	{
		if (UHealthComponent* Back = WeakHealth.Get())
		{
			Back->bInvulnerable = bInvulnerableBefore;
		}
		if (APawn* Pawn = WeakPlayer.Get())
		{
			Pawn->SetCanBeDamaged(bCouldBeDamaged);
		}
	}), Seconds, false);
}
