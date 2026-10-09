#include "Loot/LootDropComponent.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Loot/LootLibrary.h"
#include "Loot/SoulMotePickup.h"
#include "Combat/HealthComponent.h"
#include "Combat/PlayerVitalsSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Loot/GrenadePickup.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/RandomStream.h"

namespace
{
	/**
	 * Rolls a grave-salt grenade for a kill (FThrowRules::DropChance: by rank, doubled with none, nothing when full) and pops
	 * it out of the body as the ammo is. Only creatures have a rank, and nobody to pick one up (no player, no throw
	 * component, as in most test levels) means none is made.
	 */
	void DropGrenade(const ACreatureBase& Creature)
	{
		UWorld* World = Creature.GetWorld();
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		const UPlayerThrowComponent* Throw = Player ? UPlayerThrowComponent::Find(Player->GetPawn()) : nullptr;
		if (!Throw || FMath::FRand() >= FThrowRules::DropChance(Creature.GetRank(), Throw->GetGrenades()))
		{
			return;
		}
		// A little lower and slower than the guns, like the ammo: it's one tin, not a fan of drops.
		const FVector Outward = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector();
		const FVector Velocity = Outward * FMath::FRandRange(150.f, 300.f) * 0.8f + FVector(0.f, 0.f, FMath::FRandRange(400.f, 550.f) * 0.8f);
		if (AGrenadePickup* Pickup = AGrenadePickup::SpawnGrenades(World, 1, Creature.GetActorLocation() + FVector(0.f, 0.f, 60.f)))
		{
			Pickup->Toss(Velocity);
		}
	}
}

void ULootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	// Anything killable without its own table (every creature, the target dummy, future enemies) uses the default one.
	if (!LootTable)
	{
		LootTable = ULootLibrary::GetDefaultLootTable();
	}

	if (UHealthComponent* Health = GetOwner()->FindComponentByClass<UHealthComponent>())
	{
		Health->OnDeath.AddDynamic(this, &ULootDropComponent::HandleOwnerDeath);
	}
}

TArray<ASoulMotePickup*> ULootDropComponent::DropSoulMotes(FRandomStream& Random) const
{
	// Only creatures have a rank to roll by (a target dummy, a chest or a barrel leaves none).
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (!bDropSoulMotes || !Creature)
	{
		return {};
	}
	const FRecoverySettings& Settings = UPlayerVitalsSubsystem::SettingsFor(this);
	const int32 Count = Settings.RollMotes(Creature->GetRank(), Random);
	return ASoulMotePickup::SpawnMotes(GetWorld(), Creature->GetActorLocation(), Count);
}

void ULootDropComponent::HandleOwnerDeath(AController* Killer)
{
	// The motes are apart from the loot: a practice area leaves them as it leaves ammo, and their roll doesn't move the
	// loot's. A creature whose loot is off leaves none, but a boss's loot is thrown by its own shower (bDropOnDeath off).
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (Creature && (bDropOnDeath || Creature->GetRank() == ECreatureRank::Boss))
	{
		FRandomStream MoteRoll(FMath::Rand());
		DropSoulMotes(MoteRoll);
		// The grave-salt grenade is rolled apart from the table too, and a boss always leaves one (its own shower throws the
		// rest).
		DropGrenade(*Creature);
	}

	// Asked at the death rather than when play began, so turning the drop off after its owner has spawned still counts.
	if (!bDropOnDeath)
	{
		return;
	}
	// The ammo leans toward the class of the gun that landed the killing shot, so the gun in use keeps itself fed.
	const AWeaponBase* KillWeapon = AWeaponBase::FindKillWeapon(GetOwner());
	const TOptional<EAmmoType> KillAmmo = KillWeapon ? TOptional<EAmmoType>(KillWeapon->GetAmmoType()) : TOptional<EAmmoType>();
	// A practice area the player has already left once (Skyreach on a return visit) drops only ammo: practice, not a farm.
	const bool bWeapons = UAreaRulesSubsystem::DropsGunsAt(this);
	// A Greedy iron's kills roll their loot luckier.
	const float Luck = ExtraLuck + WeaponCurseEffects::KillLootLuck(KillWeapon);
	ULootLibrary::SpawnKillLoot(this, LootTable, GetOwner()->GetActorLocation(), Level, Luck, KillAmmo, bWeapons);
}

TArray<AActor*> ULootDropComponent::DropLoot()
{
	return ULootLibrary::SpawnLoot(this, LootTable, GetOwner()->GetActorLocation(), Level, ExtraLuck);
}
