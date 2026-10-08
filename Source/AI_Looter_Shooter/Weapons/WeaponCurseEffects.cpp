#include "Weapons/WeaponCurseEffects.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurses.h"
#include "GameFramework/Actor.h"

namespace
{
	/** Gun's curse while its drawback is in force, else null. */
	const FWeaponCurse* ActiveDrawback(const FWeaponInstanceData& Gun)
	{
		return WeaponCurses::DrawbackActive(Gun) ? WeaponCurses::Of(Gun) : nullptr;
	}

	UHealthComponent* HolderHealth(const AWeaponBase& Gun)
	{
		const AActor* Holder = Gun.GetOwner();
		return IsValid(Holder) ? Holder->FindComponentByClass<UHealthComponent>() : nullptr;
	}
}

WeaponCurseEffects::FShotPlan WeaponCurseEffects::PlanShot(const FWeaponInstanceData& Gun, int32 RoundsInMagazine, FRandomStream& Rolls)
{
	FShotPlan Plan;
	if (const FWeaponCurse* Curse = ActiveDrawback(Gun))
	{
		// Greedy eats two a shot; the last round in the magazine still fires on its own.
		Plan.Rounds = FMath::Clamp(Curse->RoundsPerShot, 1, FMath::Max(RoundsInMagazine, 1));
		Plan.bMisfire = Curse->MisfireChance > 0.f && Rolls.FRand() < Curse->MisfireChance;
	}
	return Plan;
}

float WeaponCurseEffects::ReloadHealthCost(const FWeaponInstanceData& Gun, float MaxHealth)
{
	const FWeaponCurse* Curse = ActiveDrawback(Gun);
	return Curse ? FMath::Max(Curse->ReloadHealthCostShare, 0.f) * FMath::Max(MaxHealth, 0.f) : 0.f;
}

float WeaponCurseEffects::MaxHealthMultiplier(const FWeaponInstanceData& Gun)
{
	const FWeaponCurse* Curse = ActiveDrawback(Gun);
	return Curse ? FMath::Clamp(Curse->MaxHealthMultiplier, 0.05f, 1.f) : 1.f;
}

float WeaponCurseEffects::KillHeal(const FWeaponInstanceData& Gun, float MaxHealth)
{
	const FWeaponCurse* Curse = WeaponCurses::Of(Gun);
	return Curse ? FMath::Max(Curse->KillHealShare, 0.f) * FMath::Max(MaxHealth, 0.f) : 0.f;
}

float WeaponCurseEffects::KillLootLuck(const AWeaponBase* KillWeapon)
{
	const FWeaponCurse* Curse = IsValid(KillWeapon) ? WeaponCurses::Of(KillWeapon->GetInstance()) : nullptr;
	return Curse ? FMath::Max(Curse->KillLootLuck, 0.f) : 0.f;
}

void WeaponCurseEffects::RefreshHolder(AActor* Holder, const AWeaponBase* Leaving)
{
	UHealthComponent* Health = IsValid(Holder) ? Holder->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!Health)
	{
		return;
	}
	// Asked of the holder rather than told by the gun: the manager equips a gun it puts away as well as one it takes in
	// hand, so only the gun actually in hand counts.
	const UWeaponManagerComponent* Manager = Holder->FindComponentByClass<UWeaponManagerComponent>();
	const AWeaponBase* InHand = Manager ? Manager->GetActiveWeapon() : nullptr;
	const bool bHeld = IsValid(InHand) && InHand != Leaving;
	Health->SetMaxHealthScale(bHeld ? MaxHealthMultiplier(InHand->GetInstance()) : 1.f);
}

void WeaponCurseEffects::PayReload(const AWeaponBase& Gun)
{
	UHealthComponent* Health = HolderHealth(Gun);
	const float Cost = Health ? ReloadHealthCost(Gun.GetInstance(), Health->GetMaxHealth()) : 0.f;
	if (Cost > 0.f)
	{
		const float Taken = Health->Drain(Cost);
		UE_LOG(LogLooter, Verbose, TEXT("%s's reload fed on its holder: %.1f health"), *Gun.GetName(), Taken);
	}
}

void WeaponCurseEffects::ApplyKillPerks(const AWeaponBase& Gun)
{
	UHealthComponent* Health = HolderHealth(Gun);
	const float Amount = Health ? KillHeal(Gun.GetInstance(), Health->GetMaxHealth()) : 0.f;
	if (Amount > 0.f)
	{
		Health->Heal(Amount);
	}
}
