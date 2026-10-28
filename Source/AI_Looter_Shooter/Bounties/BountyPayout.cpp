// BountyPayout: a paid bounty's loot thrown out of the board toward the player (the strongbox's rolls, no strongbox).

#include "Bounties/BountyPayout.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Bounties/BountyBoard.h"
#include "Bounties/BountyRules.h"
#include "Bounties/BountyTuning.h"
#include "Economy/SilverDrops.h"
#include "Loot/AmmoPickup.h"
#include "Loot/Chest.h"
#include "Loot/GrenadePickup.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Shields/ShieldPickup.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/RandomStream.h"
#include "UObject/Package.h"

namespace
{
	/**
	 * The throw out of the board (cm, cm/s, degrees): from this far in front of its face (clear of the board's own
	 * collision), out and up so a pickup lands about 1.5-2 m out, where a player reading the board stands, the lot fanned
	 * across HandOverFan so nothing stacks on another. A chest's throw (AChest::TossFan 70, TossUp 430) a little flatter:
	 * the board's face is higher than a chest's lid.
	 */
	constexpr double HandOverOut = 60.0;
	constexpr float HandOverForward = 150.f;
	constexpr float HandOverUp = 380.f;
	constexpr float HandOverFan = 80.f;
	/** Each throw's own wobble off its share of the fan (degrees), and its speed's (a share either way). */
	constexpr float HandOverWobble = 6.f;
	constexpr float HandOverSpeedSpread = 0.15f;

	/** Without a board (the console's Collect away from it): this far in front of the player, this high over their middle. */
	constexpr double NoBoardAhead = 150.0;
	constexpr double NoBoardRise = 60.0;

	/** The player's level for the loot: their progress's, 1 without one (a test level). */
	int32 LevelOf(const APawn* Player)
	{
		const APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
		const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
		const UPlayerProgressionSubsystem* Progression = Local ? Local->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
		return Progression ? FMath::Max(Progression->GetLevel(), 1) : 1;
	}

	/** Notes a gun handed over: the rarest so far is the banner's. */
	void NoteGun(FBountyHaul& Haul, const FWeaponInstanceData& Instance)
	{
		if (!Haul.bGun || static_cast<uint8>(Instance.Rarity) > static_cast<uint8>(Haul.BestGun))
		{
			Haul.bGun = true;
			Haul.BestGun = Instance.Rarity;
			Haul.BestGunColor = UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity);
		}
	}
}

ULootTable* BountyPayout::MakeTable(EBountyTier Tier, const FBountyTuning& Tuning, bool bGunsDropHere)
{
	// As AChest::MakeLootTable makes a Strongbox's with the bounty's override: the game's guns by the default table's weights,
	// its ammo classes, its wards; nothing kill-made leans the ammo.
	const FChestLootOverride Loot = BountyRules::RewardLoot(Tier, Tuning);
	ULootTable* Table = NewObject<ULootTable>(GetTransientPackage(), NAME_None, RF_Transient);
	if (const ULootTable* Default = ULootLibrary::GetDefaultLootTable())
	{
		Table->Entries = Default->Entries;
		Table->AmmoTypes = Default->AmmoTypes;
		Table->bDropsShields = Default->bDropsShields;
	}
	Table->WeaponDropChance = bGunsDropHere ? FMath::Clamp(Loot.GunChance, 0.f, 1.f) : 0.f;
	Table->MinWeaponDrops = Loot.Guns;
	Table->MaxWeaponDrops = Loot.Guns;
	Table->Luck = Loot.Luck;
	Table->AmmoDropChance = Loot.AmmoPickups > 0 ? 1.f : 0.f;
	Table->MinAmmoDrops = Loot.AmmoPickups;
	Table->MaxAmmoDrops = Loot.AmmoPickups;
	Table->KillWeaponAmmoBias = 1.f;
	// A bounty's reward keeps its full legendary odds: the early areas' cap is for the rank and file ("bosses and their
	// bounties still can", the user, 2026-10-27).
	Table->bKeepsLegendaryOdds = true;
	// The user's rule: a chest's ammo pickups always hold 36 rounds (these are the strongbox's).
	Table->UseChestAmmoAmount();
	return Table;
}

FVector BountyPayout::TossVelocity(int32 Index, int32 Count, const FVector& Toward, FRandomStream& Random)
{
	const float Share = Count > 1 ? static_cast<float>(Index) / static_cast<float>(Count - 1) - 0.5f : 0.f;
	const float Turn = Share * HandOverFan + Random.FRandRange(-HandOverWobble, HandOverWobble);
	FVector Flat = Toward.GetSafeNormal2D();
	Flat = Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
	const FVector Out = Flat.RotateAngleAxis(Turn, FVector::UpVector);
	return Out * HandOverForward * Random.FRandRange(1.f - HandOverSpeedSpread, 1.f + HandOverSpeedSpread)
		+ FVector::UpVector * HandOverUp * Random.FRandRange(0.9f, 1.1f);
}

FBountyHaul BountyPayout::HandOver(UWorld* World, const ABountyBoard* Board, const APawn* Player, EBountyTier Tier,
	const FBountyTuning& Tuning)
{
	FBountyHaul Haul;
	if (!World)
	{
		return Haul;
	}
	// Where it comes from and which way: out of the board's face toward the player reading it.
	FVector Face = FVector::ZeroVector;
	FVector Toward = FVector::ForwardVector;
	if (Board)
	{
		Face = Board->GetInteractionLocation().Get(Board->GetActorLocation());
		Toward = Player ? (Player->GetActorLocation() - Face).GetSafeNormal2D() : FVector::ZeroVector;
		Toward = Toward.IsNearlyZero() ? Board->GetActorForwardVector().GetSafeNormal2D() : Toward;
	}
	else if (Player)
	{
		Toward = Player->GetActorForwardVector().GetSafeNormal2D();
		Face = Player->GetActorLocation() + Toward * NoBoardAhead + FVector(0.0, 0.0, NoBoardRise);
		// Thrown back toward them, as the board would.
		Toward = -Toward;
	}
	else
	{
		return Haul;
	}
	const FVector From = Face + Toward * HandOverOut;

	// The player's level kept inside the area's band, as a chest's loot (AChest::GetLootLevel).
	const int32 PlayerLevel = LevelOf(Player);
	const UAreaRulesSubsystem* Rules = World->GetSubsystem<UAreaRulesSubsystem>();
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	const int32 Level = Area ? Area->LevelFor(PlayerLevel, 0, PlayerLevel) : PlayerLevel;

	const ULootTable* Table = MakeTable(Tier, Tuning, UAreaRulesSubsystem::DropsGunsAt(World));
	if (Table->Entries.IsEmpty())
	{
		UE_LOG(LogLooter, Warning, TEXT("Bounty pay: no guns to hand over (the default loot table, DA_LootTable_Default, lists none)."));
	}
	FRandomStream Random(FMath::Rand());
	FLootRoll Roll = ULootLibrary::RollLoot(Table, Level, 0.f, Random);
	ULootLibrary::KeepRocketsForLaunchers(Roll, Table, Random, ULootLibrary::AnyPlayerCarriesLauncher(World));
	// A grave-salt grenade at a Strongbox's chance, rolled after the table's own rolls so theirs don't move.
	const UPlayerThrowComponent* Throw = Player ? UPlayerThrowComponent::Find(Player) : nullptr;
	const bool bGrenade = Throw && Random.FRand() < FThrowRules::ChestChance(/*bStrongbox*/ true, Throw->GetGrenades());
	// A Strongbox's silver, on a stream of its own (as the chest's), none where guns don't drop.
	FRandomStream SilverRoll(FMath::Rand());
	const int32 Silver = SilverDrops::RollForContainer(World, EChestKind::Strongbox, Level, SilverRoll);

	const int32 Count = Roll.Weapons.Num() + Roll.Shields.Num() + Roll.Ammo.Num() + (bGrenade ? 1 : 0) + (Silver > 0 ? 1 : 0);
	int32 Thrown = 0;
	TArray<FString> Items;
	for (const FWeaponInstanceData& Instance : Roll.Weapons)
	{
		const FVector Velocity = TossVelocity(Thrown++, Count, Toward, Random);
		if (AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), From)))
		{
			// Loot like any other: it lands and settles, its beam and label with it, a save keeps it, its rarity heard as it lands.
			Gun->Toss(Velocity);
			ULootFanfareSubsystem::ExpectLanding(Gun);
			NoteGun(Haul, Instance);
			++Haul.Items;
			Items.Add(FString::Printf(TEXT("%s %s"), *UEnum::GetDisplayValueAsText(Instance.Rarity).ToString(), *GetNameSafe(Instance.Definition)));
		}
	}
	for (const FShieldInstanceData& Ward : Roll.Shields)
	{
		const FVector Velocity = TossVelocity(Thrown++, Count, Toward, Random);
		if (AShieldPickup* Pickup = AShieldPickup::SpawnWard(World, Ward, From))
		{
			Pickup->Toss(Velocity);
			++Haul.Items;
			Items.Add(FString::Printf(TEXT("%s ward"), *UEnum::GetDisplayValueAsText(Ward.Rarity).ToString()));
		}
	}
	for (const FAmmoDrop& Drop : Roll.Ammo)
	{
		const FVector Velocity = TossVelocity(Thrown++, Count, Toward, Random);
		if (AAmmoPickup* Ammo = AAmmoPickup::SpawnAmmo(World, Drop.Type, Drop.Amount, From))
		{
			Ammo->Toss(Velocity);
			++Haul.Items;
			Items.Add(FString::Printf(TEXT("%d %s"), Drop.Amount, LooterAmmo::GetInfo(Drop.Type).Name));
		}
	}
	if (bGrenade)
	{
		const FVector Velocity = TossVelocity(Thrown++, Count, Toward, Random);
		if (AGrenadePickup* Grenade = AGrenadePickup::SpawnGrenades(World, 1, From))
		{
			Grenade->Toss(Velocity);
			++Haul.Items;
			Items.Add(TEXT("1 grave-salt grenade"));
		}
	}
	if (Silver > 0)
	{
		// It comes to the player by itself, and the session keeps it if it lies.
		if (SilverDrops::Spill(World, Silver, From, TossVelocity(Thrown++, Count, Toward, SilverRoll)))
		{
			Haul.Silver = Silver;
			Items.Add(FString::Printf(TEXT("%d silver"), Silver));
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Bounty pay (%s): the board handed over %s (level %d)."), *BountyRules::TierName(Tier),
		Items.IsEmpty() ? TEXT("nothing") : *FString::Join(Items, TEXT(", ")), Level);
	return Haul;
}
