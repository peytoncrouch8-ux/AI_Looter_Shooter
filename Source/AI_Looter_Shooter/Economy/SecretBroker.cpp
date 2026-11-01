// USecretBrokerComponent: who sells the level's secrets, and when the Interact key trades rather than talks.

#include "Economy/SecretBroker.h"
#include "AI_Looter_Shooter.h"
#include "Economy/SecretSubsystem.h"
#include "Missions/MissionRunner.h"
#include "UI/Vendor/SecretsWidget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

USecretBrokerComponent::USecretBrokerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Prompt = NSLOCTEXT("LooterSecrets", "BrokerPrompt", "Buy a secret");
}

bool USecretBrokerComponent::IsOpen() const
{
	const USecretSubsystem* Secrets = USecretSubsystem::Get(this);
	return Secrets && Secrets->IsOpen();
}

bool USecretBrokerComponent::HasStoryBusiness() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const AActor* Owner = GetOwner();
	return Runner && Owner && (Runner->FindTurnInAt(Owner) || !Runner->GetOffersAt(Owner).IsEmpty());
}

bool USecretBrokerComponent::ShouldTrade() const
{
	const AActor* Owner = GetOwner();
	return Owner && !Owner->IsHidden() && IsOpen() && !HasStoryBusiness();
}

bool USecretBrokerComponent::OpenFor(AActor* Player)
{
	if (!ShouldTrade())
	{
		return false;
	}
	APlayerController* Controller = Cast<APlayerController>(Player);
	if (!Controller)
	{
		const APawn* Pawn = Cast<APawn>(Player);
		Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	}
	if (!USecretsWidget::OpenFor(Controller, this))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: selling secrets."), *GetOwner()->GetActorNameOrLabel());
	return true;
}
