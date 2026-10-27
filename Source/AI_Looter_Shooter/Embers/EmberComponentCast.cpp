#include "Embers/EmberComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Player/PlayerViewComponent.h"
#include "Scenes/SceneSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// UEmberComponent's cast (the user's note 27, 2026-10-27: "action skill activation animation"): a press that passes the
// gate begins the hands' cast (FirstPersonHandsMotion::SupportInCast: the left hand raised, flared, released), and the
// power fires at the release, CastLeadSeconds after the press (0.1 s; never over 0.15 s, so a power never feels late). In
// third person, or for a body no local player controls, there is no lead: the power fires at the press.

namespace
{
	TAutoConsoleVariable<float> CVarCastLead(TEXT("Looter.Embers.CastLead"), 0.1f,
		TEXT("Seconds an ember power waits after the press while the first-person hands raise for the cast (it fires at the release; "
			 "at most 0.15, 0 fires at the press)."));
}

float UEmberComponent::CastLeadSeconds()
{
	return FMath::Clamp(CVarCastLead.GetValueOnGameThread(), 0.f, MaxCastLead);
}

bool UEmberComponent::ShouldLead() const
{
	// Only where the cast is seen: the local player's own first-person hands.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!Player || !Player->IsLocalController())
	{
		return false;
	}
	const UPlayerViewComponent* View = Pawn->FindComponentByClass<UPlayerViewComponent>();
	return View && View->IsFirstPerson();
}

void UEmberComponent::BeginCast(int32 Index)
{
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = GetOwned();
	const UWorld* World = GetWorld();
	CastStart = World ? World->GetTimeSeconds() : 0.0;
	CastId = Owned.IsValidIndex(Index) && Owned[Index] ? Owned[Index]->Id : NAME_None;
	CastLead = ShouldLead() ? CastLeadSeconds() : 0.f;
	if (CastLead <= 0.f)
	{
		Fire(Index);
		return;
	}
	PendingIndex = Index;
	PendingLeft = CastLead;
	SetComponentTickEnabled(true);
}

void UEmberComponent::TickCast(float DeltaTime)
{
	if (PendingIndex == INDEX_NONE)
	{
		return;
	}
	PendingLeft -= DeltaTime;
	if (PendingLeft > 0.f)
	{
		return;
	}
	const int32 Index = PendingIndex;
	PendingIndex = INDEX_NONE;
	PendingLeft = 0.f;
	// Weighed again at the release: death or a scene in the lead drops it; so does the power's own say (what it needed in
	// the sights may have gone), with the soft click a refused press gets.
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (IsOwnerDead() || (Scenes && (Scenes->IsPlaying() || Scenes->IsHoldingPlayer())))
	{
		CancelCast();
		return;
	}
	const UEmberPower* Power = Powers.IsValidIndex(Index) ? Powers[Index].Get() : nullptr;
	if (!Power || Index != EquippedIndex)
	{
		CancelCast();
		return;
	}
	const EEmberDenied Why = Power->CanActivate(MakeContext());
	if (Why != EEmberDenied::None)
	{
		CancelCast();
		LooterSound::Play2D(this, FName(TEXT("Ember.Denied")), 0.5f);
		OnDenied.Broadcast(EEmberDenied::Refused);
		return;
	}
	Fire(Index);
}

void UEmberComponent::CancelCast()
{
	PendingIndex = INDEX_NONE;
	PendingLeft = 0.f;
	// The hands go back to the gun at once.
	CastStart = -1000.0;
}

void UEmberComponent::NoteFired(FName Id)
{
	// A cast already under way for this fire (its lead just ran out) keeps its clock.
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Id == CastId && Now - CastStart <= CastLead + 0.05)
	{
		return;
	}
	// A console use: the hands show the cast from its release.
	CastStart = Now;
	CastLead = 0.f;
	CastId = Id;
}

float UEmberComponent::GetCastTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return -1.f;
	}
	const float Since = static_cast<float>(World->GetTimeSeconds() - CastStart);
	return Since >= 0.f && Since <= FirstPersonHandsMotion::CastSeconds(CastLead) ? Since : -1.f;
}

bool UEmberComponent::IsCasting() const
{
	// The pose is a first-person one: in third person nothing holds the left hand.
	return PendingIndex != INDEX_NONE || (GetCastTime() >= 0.f && ShouldLead());
}
