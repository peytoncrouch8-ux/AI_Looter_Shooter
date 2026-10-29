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
// gun (since gun identity, 2026-10-28, each kind its own shape: a launcher's heave after its shove, a heavy gun's shake, the
// sights' settle as they arrive), and the jolt of being hurt, away from where the hit came from. Only what the player sees
// moves; the aim keeps the recoil (PlayerViewAim.cpp) and nothing else. A creature's death gives its killer the kill's
// punch (UCreatureHitReactionComponent). Looter.Gunfeel.Identity 0 plays today's kicks and none of the new ones.

void UPlayerViewComponent::AddShotKick(const AWeaponBase& Weapon)
{
	APlayerController* Player = Character.IsValid() ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Player || !Player->IsLocalController())
	{
		return;
	}
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition;
	const EWeaponKind Kind = Definition ? Definition->Kind : EWeaponKind::Rifle;
	const float Aim = GetAimAlpha();
	const float ShotRecoil = GunfeelKick::ShotRecoil(&Weapon);
	// The kind's own weight on top (Looter.Gunfeel.Kick.*). The first shot of a rifle's or SMG's burst is felt harder (Recoil
	// already counted it in HandleWeaponFired), an LMG still spinning up bullies the shooter and a spun one settles (its
	// dig-in), and an automatic's kicks stack under its sustained cap (FViewKickStack::Add) so a long burst never shakes the
	// view past what the sights can follow.
	const float Scale = GunfeelKick::ViewKickScale(Kind) * GunfeelKick::FirstShotKick(Kind, Recoil.GetShotInBurst()) * Weapon.GetKickDigIn();
	const bool bIdentity = GunfeelKick::IdentityOn();
	const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeel(Kind);
	FShotKickShape Shape;
	Shape.AimedTurn = Feel.AimedTurn;
	Shape.AimedFieldOfView = Feel.AimedFieldOfView;
	Shape.Lean = GunfeelKick::PatternLean(Kind, Recoil.GetShotInBurst());
	FViewKick Kick = (bIdentity ? ViewKicks::ForShot(Kind, ShotRecoil, Aim, KickRandom, Shape)
		: ViewKicks::ForShotToday(Kind, ShotRecoil, Aim, KickRandom)) * Scale;
	Kick.SustainCap = GunfeelKick::SustainCapFor(Kind);
	Kick.SustainFloor = GunfeelKick::SustainFloor();
	UCameraShakeModifier::Kick(Player, Kick);
	// The launcher's tube sags back a moment later (never counted against a cap: it pulls the view down).
	const float Heave = GunfeelKick::HeaveScale();
	if (Heave > 0.f)
	{
		const FViewKick Sag = ViewKicks::ForHeave(Kind, ShotRecoil, Aim, Shape) * (Scale * Heave);
		if (!Sag.IsNone())
		{
			UCameraShakeModifier::Kick(Player, Sag);
		}
	}
	// The heavy kinds' shot also shakes the camera a moment (the LMG's held small, its rattle never a blur).
	if (bIdentity)
	{
		if (UCameraShakeModifier* Modifier = UCameraShakeModifier::FindOrAdd(Player))
		{
			const GunfeelKick::FShotShake Shake = GunfeelKick::ShotShake(Kind, Aim, Modifier->GetAmount());
			Modifier->AddShake(Shake.Strength, Shake.Seconds);
		}
	}
	// And in the hands, on a gamepad.
	GunfeelKick::RumbleShot(Player, Kind);
}

void UPlayerViewComponent::AddAimSettleKick(const AWeaponBase& Weapon)
{
	// The sights arriving at the eye: a small settle, heavier for a heavy gun (Looter.Gunfeel.Aim.Settle.<Kind>; none with
	// Identity 0).
	APlayerController* Player = Character.IsValid() ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Player || !Player->IsLocalController() || !GunfeelKick::IdentityOn())
	{
		return;
	}
	const float Settle = GunfeelKick::KindFeel(GunfeelKick::KindOf(&Weapon)).AimSettle;
	if (Settle > 0.f)
	{
		UCameraShakeModifier::Kick(Player, ViewKicks::ForAimSettle(Settle));
	}
}

void GunfeelKick::BoltHomeBeat(const AWeaponBase& Weapon)
{
	// The sniper's bolt closing after its cycle: a nudge of the local player's view and a tick in the hands.
	const ACharacter* Holder = Cast<ACharacter>(Weapon.GetOwner());
	APlayerController* Player = Holder ? Cast<APlayerController>(Holder->GetController()) : nullptr;
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const float Scale = BoltHomeScale();
	if (!Player || !Player->IsLocalController() || !View || Scale <= 0.f)
	{
		return;
	}
	UCameraShakeModifier::Kick(Player, ViewKicks::ForBoltHome(View->GetAimAlpha()) * Scale);
	RumbleBeat(Player, Default::BoltHomeRumble * Scale, Default::BoltHomeRumbleSeconds);
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
