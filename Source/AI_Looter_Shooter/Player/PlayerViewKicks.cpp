#include "Player/PlayerViewComponent.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Combat/HealthComponent.h"
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
	UCameraShakeModifier::Kick(Player, ViewKicks::ForShot(Kind, Weapon.GetStats().Recoil, GetAimAlpha(), KickRandom));
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
}
