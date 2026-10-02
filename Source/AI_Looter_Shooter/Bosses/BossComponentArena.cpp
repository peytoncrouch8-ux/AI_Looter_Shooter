#include "Bosses/BossComponent.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossSeal.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UBossComponent's arena: the fog wall that closes it and the bar at the top of the screen. The fight's course is in
// BossComponent.cpp.

// ---------------------------------------------------------------------------
// The wall
// ---------------------------------------------------------------------------

ABossSeal* UBossComponent::GetActiveSeal() const
{
	return Seal ? Seal.Get() : SpawnedSeal.Get();
}

bool UBossComponent::IsSealRaised() const
{
	const ABossSeal* ActiveSeal = GetActiveSeal();
	return ActiveSeal && ActiveSeal->IsRaised();
}

ABossSeal* UBossComponent::EnsureSeal()
{
	if (Seal)
	{
		return Seal;
	}
	if (SealRadius <= 0.f)
	{
		return nullptr;
	}
	// The ring stands on the ground at its spot (the home is the capsule's middle).
	const ACreatureBase* Creature = GetCreature();
	const float Half = Creature ? Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	const FVector Center = Spot - FVector(0.0, 0.0, Half);
	if (!SpawnedSeal)
	{
		SpawnedSeal = ABossSeal::SpawnRing(GetWorld(), Center, SealRadius);
	}
	else if (!SpawnedSeal->IsRaised())
	{
		SpawnedSeal->SetActorLocation(Center);
		SpawnedSeal->Radius = SealRadius;
	}
	return SpawnedSeal;
}

void UBossComponent::TryRaiseSeal()
{
	if (IsSealRaised())
	{
		return;
	}
	ABossSeal* ActiveSeal = EnsureSeal();
	const ACreatureBase* Creature = GetCreature();
	const APawn* Fighter = FightPlayer.Get();
	if (!ActiveSeal || !Creature || !Fighter)
	{
		return;
	}
	// Only with both inside: nobody is shut out, and nobody is caught in the wall as it closes.
	if (ActiveSeal->IsInside(Fighter->GetActorLocation(), SealMargin) && ActiveSeal->IsInside(Creature->GetActorLocation(), 0.f))
	{
		ActiveSeal->Raise();
		UE_LOG(LogLooter, Log, TEXT("Boss %s: the fog wall closes."), *GetLabel());
	}
}

void UBossComponent::DropSeal()
{
	if (ABossSeal* ActiveSeal = GetActiveSeal())
	{
		ActiveSeal->Drop();
	}
}

// ---------------------------------------------------------------------------
// The bar
// ---------------------------------------------------------------------------

TArray<float> UBossComponent::GetPhaseShares() const
{
	TArray<float> Shares;
	for (const FBossPhase& Each : FightPhases)
	{
		Shares.Add(Each.HealthShare);
	}
	return Shares;
}

void UBossComponent::ShowBar()
{
	bBarWanted = true;
	BarHideTime = 0.f;
	UWorld* World = GetWorld();
	APlayerController* Viewer = World ? World->GetFirstPlayerController() : nullptr;
	// A test level has nobody to show it to.
	if (!bShowBar || !Viewer || !Viewer->IsLocalController())
	{
		return;
	}
	if (!Bar)
	{
		Bar = CreateWidget<UHudBossBarWidget>(Viewer, UHudBossBarWidget::StaticClass());
	}
	if (!Bar)
	{
		return;
	}
	if (!Bar->IsInViewport())
	{
		Bar->AddToViewport(UHudBossBarWidget::ViewportZOrder);
	}
	const ACreatureBase* Creature = GetCreature();
	const FText ShownName = !BossName.IsEmpty() ? BossName : (Creature ? Creature->DisplayName : FText::GetEmpty());
	const ECreatureRank Rank = Creature ? Creature->GetRank() : ECreatureRank::Boss;
	Bar->SetBoss(ShownName, Creature ? Creature->Level : 1, UCreatureRankSettings::Get(Rank).Color, GetPhaseShares());
	Bar->SetShown(true);
	UpdateBar();
}

void UBossComponent::HideBar()
{
	bBarWanted = false;
	BarHideTime = 0.f;
	if (Bar)
	{
		Bar->SetShown(false);
	}
}

void UBossComponent::UpdateBar()
{
	if (!Bar || !bBarWanted)
	{
		return;
	}
	if (const UHealthComponent* BossHealth = GetBossHealth())
	{
		Bar->SetHealth(BossHealth->GetHealth(), BossHealth->GetMaxHealth());
	}
	const FBossPhase* Current = FightPhases.IsValidIndex(Phase) ? &FightPhases[Phase] : nullptr;
	Bar->SetPhase(FMath::Max(Phase, 0), Current ? Current->Name : FText::GetEmpty());
	Bar->SetUntargetable(bUntargetable, ActiveSpell.Hint);
}
