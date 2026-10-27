#include "Player/PlayerViewComponent.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Combat/HealthComponent.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// UPlayerViewComponent's view kicks (ViewKicks, played by UCameraShakeModifier on the player's camera): each shot's kick by
// gun, and the jolt of being hurt, away from where the hit came from. Only what the player sees moves; the aim keeps the
// recoil (PlayerViewAim.cpp) and nothing else. A creature's death gives its killer the kill's punch
// (UCreatureHitReactionComponent).

void UPlayerViewComponent::AddShotKick(const AWeaponBase& Weapon)
{
	APlayerController* Player = Character.IsValid() ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Player || !Player->IsLocalController())
	{
		return;
	}
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition;
	const EWeaponKind Kind = Definition ? Definition->Kind : EWeaponKind::Rifle;
	// The kind's own weight on top: a shotgun shoves, a six-gun snaps (Looter.Gunfeel.Kick.*). The first shot of a rifle's or
	// SMG's burst is felt harder (Recoil already counted it in HandleWeaponFired), and an automatic's kicks stack under a
	// sustained cap (FViewKickStack::Add) so a long burst never shakes the view past what the sights can follow.
	const float FirstShot = GunfeelKick::FirstShotKick(Kind, Recoil.GetShotInBurst());
	FViewKick Kick = ViewKicks::ForShot(Kind, GunfeelKick::ShotRecoil(&Weapon), GetAimAlpha(), KickRandom) * (GunfeelKick::KickScale(Kind) * FirstShot);
	Kick.SustainCap = GunfeelKick::SustainCapFor(Kind);
	Kick.SustainFloor = GunfeelKick::SustainFloor();
	UCameraShakeModifier::Kick(Player, Kick);
	// And in the hands, on a gamepad.
	GunfeelKick::RumbleShot(Player, Kind);
}

void GunfeelKick::RackBeat(const AWeaponBase& Weapon)
{
	// The shotgun's pump: a small dip of the local player's view as the fore-end goes back and home (GF-RELOAD calls it).
	const ACharacter* Holder = Cast<ACharacter>(Weapon.GetOwner());
	APlayerController* Player = Holder ? Cast<APlayerController>(Holder->GetController()) : nullptr;
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const float Scale = RackKickScale();
	if (!Player || !Player->IsLocalController() || !View || Scale <= 0.f)
	{
		return;
	}
	UCameraShakeModifier::Kick(Player, ViewKicks::ForRack(View->GetAimAlpha()) * Scale);
}

void UPlayerViewComponent::BindOwnerHealth(bool bBind)
{
	if (UHealthComponent* Old = OwnerHealth.Get())
	{
		Old->OnDamaged.RemoveDynamic(this, &UPlayerViewComponent::HandleOwnerDamaged);
	}
	OwnerHealth.Reset();
	if (!bBind)
	{
		return;
	}
	if (UHealthComponent* Found = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Found->OnDamaged.AddUniqueDynamic(this, &UPlayerViewComponent::HandleOwnerDamaged);
		OwnerHealth = Found;
	}
}

void UPlayerViewComponent::HandleOwnerDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	const ACharacter* Owner = Character.Get();
	APlayerController* Player = Owner ? Cast<APlayerController>(Owner->GetController()) : nullptr;
	const UHealthComponent* Health = OwnerHealth.Get();
	if (!Player || !Player->IsLocalController() || !Health || Damage <= 0.f)
	{
		return;
	}
	// Where it came from: what dealt it (the creature, the boss's pellet's shooter), else whoever was behind it. Unknown:
	// straight on, so the jolt only tips the view back.
	const AActor* Source = DamageCauser ? DamageCauser : (InstigatedBy ? InstigatedBy->GetPawn() : nullptr);
	float Side = 0.f;
	float Ahead = 1.f;
	if (Source && Source != Owner)
	{
		const FRotator View(0.f, Player->GetControlRotation().Yaw, 0.f);
		const FVector Toward = (Source->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
		if (!Toward.IsNearlyZero())
		{
			Side = static_cast<float>(FVector::DotProduct(Toward, FRotationMatrix(View).GetUnitAxis(EAxis::Y)));
			Ahead = static_cast<float>(FVector::DotProduct(Toward, View.Vector()));
		}
	}
	const float Share = Damage / FMath::Max(Health->GetMaxHealth(), 1.f);
	UCameraShakeModifier::Kick(Player, ViewKicks::ForHurt(Share, Side, Ahead));
	GunfeelKick::RumbleHurt(Player, Share);
}
