// ACreatureBase: being hurt (its attention, a full update rate for a while), its pack's call on whoever hurt it, and the
// tag over it (name, level, rank word, health, and its rank's sting flashing the word). Moved out of CreatureBase.cpp to keep
// it under the size guide.

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/World/CreatureHealthBarWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"

void ACreatureBase::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogLooter, Verbose, TEXT("%s took %.1f%s (%.0f / %.0f)"), *GetName(), Damage, bCritical ? TEXT(" CRIT") : TEXT(""),
		Health->GetHealth(), Health->GetMaxHealth());
	HealthBarTime = 6.f;
	// A hurt creature updates every frame for a while, wherever it is, so its flinch (and death) plays smoothly. Before
	// OnHurt: a frozen body is set up afresh first.
	FullRateTime = UpdateRate.HurtFullRateTime;
	WakeUpdateRate();
	OnHurt(bCritical, HitLocation);

	if (State == ECreatureState::Dead)
	{
		return;
	}
	// Getting shot always gets its attention, even from beyond its sight range.
	APawn* Attacker = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	if (!bPassive && !Target.IsValid() && IsValidTarget(Attacker))
	{
		Target = Attacker;
		if (State != ECreatureState::Attack)
		{
			SetState(ECreatureState::Chase);
		}
	}
	// A pack turns on whoever hurts one of them: every creature of its pack (its PackTag) within its call, which a rank
	// can widen (a Gravebound spider calls every spider near it).
	const float CallRadius = GetPackCallRadius();
	if (CallRadius > 0.f && IsValidTarget(Attacker))
	{
		for (TActorIterator<ACreatureBase> It(GetWorld()); It; ++It)
		{
			if (*It != this && It->SharesPackWith(*this)
				&& FVector::DistSquared(It->GetActorLocation(), GetActorLocation()) <= FMath::Square(CallRadius))
			{
				It->AlertTo(Attacker);
			}
		}
	}
}

void ACreatureBase::AlertTo(APawn* Attacker)
{
	if (bPassive || State == ECreatureState::Dead || Target.IsValid() || !IsValidTarget(Attacker))
	{
		return;
	}
	Target = Attacker;
	if (State != ECreatureState::Attack)
	{
		SetState(ECreatureState::Chase);
	}
	UPlayerProgressionSubsystem::RecordEncounter(Attacker->GetController(), this);
}

void ACreatureBase::UpdateHealthBar(float DeltaSeconds)
{
	HealthBarTime = FMath::Max(0.f, HealthBarTime - DeltaSeconds);
	const bool bShow = bShowsHealthTag && State != ECreatureState::Dead && (HealthBarTime > 0.f || Target.IsValid());
	HealthBar->SetVisibility(bShow);
	if (bShow)
	{
		if (UCreatureHealthBarWidget* Bar = Cast<UCreatureHealthBarWidget>(HealthBar->GetUserWidgetObject()))
		{
			// A Legendary monster with a name of its own shows its name in the rank's color, with no word before it.
			const FCreatureRankInfo& RankInfo = UCreatureRankSettings::Get(CurrentRank);
			Bar->SetCreature(DisplayName, Level, bNameIsRankWord ? FText::GetEmpty() : RankInfo.Word, RankInfo.Color);
			Bar->SetHealth(Health->GetHealth(), Health->GetMaxHealth());
			// A ranked one's sting (as it turns on a player) flashes its rank's word.
			Bar->SetRankStingAge(Pack->GetRankStingAge());
		}
	}
}
