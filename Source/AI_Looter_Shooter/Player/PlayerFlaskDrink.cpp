#include "Player/PlayerFlaskComponent.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Player/CameraShakeModifier.h"
#include "Player/FlaskFX.h"
#include "Player/PlayerFlaskMotion.h"
#include "Weapons/WeaponBase.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UPlayerFlaskComponent's drink: its clock, the heal over its window, the gun dipping out of the way (the grenade throw's
// way: AWeaponBase's melee swing state and pose) and the moments it's felt (the press, the sip).

void UPlayerFlaskComponent::StartDrink()
{
	UHealthComponent* Health = GetHealth();
	++DrinkCount;
	bDrinking = true;
	bPouring = true;
	bSipped = false;
	Clock = 0.f;
	HealedShare = 0.f;
	// The heal's size is set at the press: a level-up or a cursed iron mid-drink doesn't change what this one gives.
	HealAmount = Health ? Health->GetMaxHealth() * FFlaskRules::HealShare : 0.f;
	// The charge goes at the press: once started it finishes (a hit doesn't stop it), so it's spent now.
	SetCharges(Charges - 1, EFlaskChange::Drunk);

	// With a gun in hand it dips out of the way and holds its fire; a reload under way is cut short (it starts over after),
	// exactly as for a throw.
	TakeWeapon(GetWeaponInHand());

	// Walking on, slower (FFlaskRules::MoveScale), through the drink's hands: the slow component sits on top of whatever
	// the sprint and the sights set, and a stronger slow (a shriek) still wins.
	if (ACharacter* Body = Cast<ACharacter>(GetOwner()))
	{
		UMovementSlowComponent::Apply(Body, FFlaskRules::MoveScale, FFlaskRules::DrinkSeconds);
	}

	// Heard and felt the moment the key goes down: the cork's pop, the flask's light brightening, the view lifting a touch.
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		FlaskFX::Uncork(*Owner);
	}
	if (APlayerController* Player = GetPlayer())
	{
		UCameraShakeModifier::Kick(Player, FlaskMotion::Lift());
	}
	SetComponentTickEnabled(true);
	UpdatePoses();
	UE_LOG(LogLooter, Log, TEXT("Flask: drink %d, %.0f health over %.2f s, %d charge(s) left"), DrinkCount, HealAmount, FFlaskRules::HealSeconds,
		Charges);
}

void UPlayerFlaskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UPlayerFlaskComponent::Advance(float DeltaSeconds)
{
	if (!bDrinking && !bPouring)
	{
		SetComponentTickEnabled(false);
		return;
	}
	const UHealthComponent* Health = GetHealth();
	if (!Health || Health->IsDead())
	{
		// Dead mid-drink: the gun goes back to its hold with nothing following on, and the rest of the heal is lost (the
		// respawn fills the flask anyway).
		EndDrink(false);
		bPouring = false;
		SetComponentTickEnabled(false);
		return;
	}
	Clock += FMath::Clamp(DeltaSeconds, 0.f, 0.1f);

	if (!bSipped && Clock >= FFlaskRules::SipAt)
	{
		Sip();
	}
	HealTo(FFlaskRules::HealShareAt(Clock));
	if (Clock >= FFlaskRules::HealEndSeconds)
	{
		bPouring = false;
	}

	if (bDrinking)
	{
		// The gun left the hand mid-drink (a swap, a drop): the old one back in its hold with nothing following on, and
		// whatever came up takes its place, held back the same, so no shot leaves it while the flask is at the lips.
		if (DrinkWeapon.Get() != GetWeaponInHand())
		{
			ReleaseWeapon(false);
			TakeWeapon(GetWeaponInHand());
		}
		UpdatePoses();
		if (Clock >= FFlaskRules::DrinkSeconds)
		{
			EndDrink(true);
		}
	}
	if (!bDrinking && !bPouring)
	{
		SetComponentTickEnabled(false);
	}
}

void UPlayerFlaskComponent::Sip()
{
	bSipped = true;
	// The gulp and the soul's rush from the flask's mouth (FX), and the head tipping back with it.
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		FlaskFX::Drink(*Owner, GetMouthLocation());
	}
	if (APlayerController* Player = GetPlayer())
	{
		UCameraShakeModifier::Kick(Player, FlaskMotion::Sip());
	}
}

void UPlayerFlaskComponent::HealTo(float Share)
{
	const float Wanted = FMath::Clamp(Share, 0.f, 1.f);
	if (Wanted <= HealedShare)
	{
		return;
	}
	// Given as the curve rises, frame by frame, so the bar swells; what would go past the most is simply not given.
	const float Step = (Wanted - HealedShare) * HealAmount;
	HealedShare = Wanted;
	if (UHealthComponent* Health = GetHealth())
	{
		Health->Heal(Step);
	}
}

void UPlayerFlaskComponent::EndDrink(bool bResume)
{
	if (!bDrinking)
	{
		return;
	}
	bDrinking = false;
	const UWorld* World = GetWorld();
	LastDrinkEnd = World ? World->GetTimeSeconds() : 0.0;
	HideModels();
	ReleaseWeapon(bResume);
	UpdateGlow();
}

void UPlayerFlaskComponent::TakeWeapon(AWeaponBase* Gun)
{
	DrinkWeapon = Gun;
	if (Gun)
	{
		const bool bCutReload = Gun->BeginMeleeSwing();
		UE_LOG(LogLooter, Verbose, TEXT("Flask: %s held back%s"), *Gun->GetName(), bCutReload ? TEXT(", reload cut short") : TEXT(""));
	}
}

void UPlayerFlaskComponent::ReleaseWeapon(bool bResume)
{
	AWeaponBase* Gun = DrinkWeapon.Get();
	DrinkWeapon.Reset();
	if (Gun)
	{
		Gun->EndMeleeSwing(bResume);
	}
}

void UPlayerFlaskComponent::UpdatePoses()
{
	if (AWeaponBase* Gun = DrinkWeapon.Get())
	{
		const FThrowPose Pose = FlaskMotion::GunPose(Clock);
		Gun->SetMeleePose(Pose.Offset, Pose.Rotation);
	}
	UpdateModels();
}
