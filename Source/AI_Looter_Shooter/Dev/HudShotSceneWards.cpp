// The ward's states in Looter.HudShots (SHIELD-POLISH, 2026-10-22): the frame with a ward full, chipped by a hit it took
// whole (the cyan edges and glint, no red), broken by a hit (both, the slivers flying) and lying broken (the empty track
// pulsing). Made through the game's own paths: a ward worn, then plain damage. Not in shipping builds.

#include "Dev/HudShotScene.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldTypes.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** The chipping hit, as a share of the ward's capacity: plainly short of breaking it. */
	constexpr float HudShotWardChipShare = 0.45f;
	/** The breaking hit goes this far past what the ward holds (points; half of it reaches health: BreakSpill). */
	constexpr float HudShotWardBreakPast = 12.f;

	APawn* HudShotWardPawn(UWorld& World)
	{
		const APlayerController* Controller = World.GetFirstPlayerController();
		return Controller ? Controller->GetPawn() : nullptr;
	}

	UPlayerShieldComponent* HudShotFindWard(UWorld& World)
	{
		return UPlayerShieldComponent::Find(HudShotWardPawn(World));
	}

	void HudShotWardHurt(UWorld& World, float Amount)
	{
		if (APawn* Pawn = HudShotWardPawn(World); Pawn && Amount > 0.f)
		{
			// No instigator and no causer, as the health steps' hits: only the player's own HUD reacts.
			UGameplayStatics::ApplyDamage(Pawn, Amount, nullptr, nullptr, UDamageType::StaticClass());
		}
	}
}

namespace HudShotScene::Wards
{
	void TakeOff(UWorld& World)
	{
		if (UPlayerShieldComponent* Shield = HudShotFindWard(World))
		{
			// Nothing worn or carried, with the starting ward counted as given, so the rifle's first-gun ward doesn't come
			// back on mid-run: the health pictures stay as they were before wards.
			FShieldInventorySave None;
			None.bStarterGiven = true;
			Shield->RestoreWards(None);
		}
	}

	void BeginFull(UWorld& World)
	{
		UPlayerShieldComponent* Shield = HudShotFindWard(World);
		if (!Shield)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: the player has no ward component, so the ward pictures show none."));
			return;
		}
		if (const APawn* Pawn = HudShotWardPawn(World))
		{
			if (UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>())
			{
				Health->ResetHealth();
			}
		}
		// The starting ward (Common, plain, level 1: the same numbers every run), worn full.
		Shield->Equip(ShieldRules::Starter(UShieldDefinition::GetWardBox(), 1), /*bFull*/ true);
	}

	void BeginChipped(UWorld& World)
	{
		if (const UPlayerShieldComponent* Shield = HudShotFindWard(World))
		{
			HudShotWardHurt(World, Shield->GetCapacity() * HudShotWardChipShare);
		}
	}

	void BeginBreak(UWorld& World)
	{
		if (const UPlayerShieldComponent* Shield = HudShotFindWard(World))
		{
			HudShotWardHurt(World, Shield->GetCharge() + HudShotWardBreakPast);
		}
	}

	bool IsBroken(UWorld& World)
	{
		const UPlayerShieldComponent* Shield = HudShotFindWard(World);
		return Shield && Shield->IsBroken();
	}

	void End(UWorld& World)
	{
		// A run without quit plays on as the player had it: the starting ward full, full health.
		BeginFull(World);
	}
}

#endif
