// AChest's loot: what it gives (its kind's guns and ammo from the default loot table, at the area's level for the
// player), thrown out of SOCKET_Loot once as the lid opens.

#include "Loot/Chest.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Loot/AmmoPickup.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"
#include "UObject/Package.h"

namespace
{
	/** The local player's level, 1 without one (a test level). */
	int32 PlayerLevel(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
		const UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
		return Progression ? FMath::Max(Progression->GetLevel(), 1) : 1;
	}
}


ULootTable* AChest::MakeLootTable() const
{
	const FChestKindInfo Info = GetKindInfo();
	ULootTable* Table = NewObject<ULootTable>(GetTransientPackage(), NAME_None, RF_Transient);
	// The game's guns by the default table's weights, and its ammo classes; nothing kill-made leans the ammo.
	if (const ULootTable* Default = ULootLibrary::GetDefaultLootTable())
	{
		Table->Entries = Default->Entries;
		Table->AmmoTypes = Default->AmmoTypes;
	}
	Table->WeaponDropChance = 1.f;
	Table->MinWeaponDrops = Info.Guns;
	Table->MaxWeaponDrops = Info.Guns;
	Table->Luck = Info.Luck;
	Table->AmmoDropChance = Info.AmmoPickups > 0 ? 1.f : 0.f;
	Table->MinAmmoDrops = Info.AmmoPickups;
	Table->MaxAmmoDrops = Info.AmmoPickups;
	Table->KillWeaponAmmoBias = 1.f;
	// The user's rule: a chest's ammo pickups always hold 36 rounds.
	Table->UseChestAmmoAmount();
	return Table;
}

int32 AChest::GetLootLevel() const
{
	// As a creature here would be at the player's level (its spread aside), and a mission's reward gun: the player's level,
	// kept inside the area's band so a chest left for later doesn't outrun the area.
	const UWorld* World = GetWorld();
	const int32 Level = PlayerLevel(World);
	const UAreaRulesSubsystem* Rules = World ? World->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	return Area ? Area->LevelFor(Level, 0, Level) : Level;
}

TArray<AActor*> AChest::GetDroppedLoot() const
{
	TArray<AActor*> Loot;
	for (const TWeakObjectPtr<AActor>& Each : DroppedLoot)
	{
		if (AActor* Actor = Each.Get(); Actor && !Actor->IsActorBeingDestroyed())
		{
			Loot.Add(Actor);
		}
	}
	return Loot;
}

FVector AChest::GetLootOrigin() const
{
	return Body && Body->DoesSocketExist(LootSocket) ? Body->GetSocketLocation(LootSocket)
		: GetActorTransform().TransformPosition(GetKindInfo().LootPoint);
}

FVector AChest::TossVelocity(int32 Index, int32 Count, FRandomStream& Random) const
{
	// Fanned evenly across its front, each a few degrees off its share, a little faster or slower.
	const float Share = Count > 1 ? static_cast<float>(Index) / static_cast<float>(Count - 1) - 0.5f : 0.f;
	const float Turn = Share * TossFan + Random.FRandRange(-8.f, 8.f);
	const FVector Out = GetActorForwardVector().GetSafeNormal2D().RotateAngleAxis(Turn, FVector::UpVector);
	return Out * TossForward * Random.FRandRange(0.8f, 1.2f) + FVector::UpVector * TossUp * Random.FRandRange(0.9f, 1.1f);
}

void AChest::DropLoot()
{
	if (bLootGiven)
	{
		return;
	}
	bLootGiven = true;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const ULootTable* Table = MakeLootTable();
	if (Table->Entries.IsEmpty())
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no guns to give (the default loot table, DA_LootTable_Default, lists none)."),
			*GetActorNameOrLabel());
	}
	FRandomStream Random(FMath::Rand());
	const int32 Level = GetLootLevel();
	const FLootRoll Roll = ULootLibrary::RollLoot(Table, Level, 0.f, Random);
	const FVector From = GetLootOrigin();
	const int32 Count = Roll.Weapons.Num() + Roll.Ammo.Num();
	int32 Thrown = 0;
	TArray<FString> Items;
	for (const FWeaponInstanceData& Instance : Roll.Weapons)
	{
		const FVector Velocity = TossVelocity(Thrown++, Count, Random);
		const FTransform Spawn(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), From);
		if (AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(this, Instance, Spawn))
		{
			// Loot like any other: it lands and settles, its beam and label with it, and a save keeps it.
			Gun->Toss(Velocity);
			DroppedLoot.Add(Gun);
			Items.Add(FString::Printf(TEXT("%s %s"), *UEnum::GetDisplayValueAsText(Instance.Rarity).ToString(), *GetNameSafe(Instance.Definition)));
		}
	}
	for (const FAmmoDrop& Drop : Roll.Ammo)
	{
		const FVector Velocity = TossVelocity(Thrown++, Count, Random);
		if (AAmmoPickup* Ammo = AAmmoPickup::SpawnAmmo(World, Drop.Type, Drop.Amount, From))
		{
			Ammo->Toss(Velocity);
			DroppedLoot.Add(Ammo);
			Items.Add(FString::Printf(TEXT("%d %s"), Drop.Amount, LooterAmmo::GetInfo(Drop.Type).Name));
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s (%s): gave %s (level %d)."), *GetActorNameOrLabel(), *GetSaveKey().ToString(),
		Items.IsEmpty() ? TEXT("nothing") : *FString::Join(Items, TEXT(", ")), Level);
	// Progress: the session keeps it open with this map's world (FSavedMapWorld::OpenedChests), and the loot lying there.
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveSoon();
	}
}
