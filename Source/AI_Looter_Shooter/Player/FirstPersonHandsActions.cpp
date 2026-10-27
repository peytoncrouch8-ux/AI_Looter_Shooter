#include "Player/FirstPersonHandsComponent.h"
#include "Embers/EmberComponent.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerViewComponent.h"
#include "UI/HUD/LooterHUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UFirstPersonHandsComponent's actions beyond the gun: the hero's melee tool in the right hand (the user's note 5,
// 2026-10-27), an ember power's cast by the left (note 27), and both with no gun in hand at all.

FHandPlacement UFirstPersonHandsComponent::RightOnTool(const FHandPlacement& OnGrip)
{
	FMeleeToolHold Hold;
	const UPlayerMeleeComponent* Striker = Melee.Get();
	bToolInHand = Striker && Striker->GetToolHold(Hold);
	if (!bToolInHand)
	{
		return OnGrip;
	}
	const FHandPlacement OnTool = FirstPersonHandsMotion::OnMeleeTool(Hold.Grip, Hold.HandleRadius, Hold.Kind == EMeleeGripKind::Forearm);
	return FHandPlacement::Blend(OnGrip, OnTool, FMath::SmoothStep(0.f, 1.f, Hold.HandShare));
}

bool UFirstPersonHandsComponent::CastSupport(const FHandPlacement& OnGun, FHandPlacement& OutPlacement) const
{
	const UEmberComponent* Power = Embers.Get();
	const float CastTime = Power ? Power->GetCastTime() : -1.f;
	if (CastTime < 0.f)
	{
		return false;
	}
	OutPlacement = FirstPersonHandsMotion::SupportInCast(OnGun, FirstPersonHandsMotion::CastKindOf(Power->GetCastId()), CastTime,
		Power->GetCastLead());
	return true;
}

bool UFirstPersonHandsComponent::PlaceBareHands()
{
	// Only the first-person view's own hands, never behind a menu.
	const APawn* Owner = Cast<APawn>(GetOwner());
	const UPlayerViewComponent* PlayerView = View.Get();
	if (!Owner || !PlayerView || !PlayerView->IsFirstPerson())
	{
		return false;
	}
	const APlayerController* Player = Cast<APlayerController>(Owner->GetController());
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	if (Hud && Hud->IsMenuOpen())
	{
		return false;
	}
	const int32 Left = FirstPersonHands::Index(EFirstPersonHand::Left);
	const int32 Right = FirstPersonHands::Index(EFirstPersonHand::Right);
	// Each hand rests under the frame (out of view) until it has something to do: the right swings the tool, the left casts.
	const FHandPlacement RightRest = FirstPersonHandsMotion::BareRest(EFirstPersonHand::Right);
	const FHandPlacement LeftRest = FirstPersonHandsMotion::BareRest(EFirstPersonHand::Left);
	Targets[Right] = RightOnTool(RightRest);
	FHandPlacement CastHand;
	const bool bCast = CastSupport(LeftRest, CastHand);
	Targets[Left] = bCast ? CastHand : LeftRest;
	// No gun's jobs to ease between.
	bHaveSupport = false;
	CatchUp = 0.f;
	SupportJob = bCast ? EHandsSupportJob::Cast : EHandsSupportJob::Foregrip;
	OneHandBlend = 0.f;
	return bToolInHand || bCast;
}
