#include "Bosses/BossLootShower.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossRules.h"
#include "Combat/BulletSubsystem.h"
#include "Loot/AmmoPickup.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootTable.h"
#include "Scenes/SceneSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Weapons/WeaponFX.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

namespace
{
	/** A shower that never finishes (a scene that never ends) goes after this long (s). */
	constexpr float ShowerLifeSeconds = 240.f;

	/** The burst's flash, and each piece's, against a gun's muzzle flash (FWeaponFX::SpawnFlash). */
	constexpr float BurstFlashScale = 1.8f;
	constexpr float PieceFlashScale = 0.45f;
}

ABossLootShower::ABossLootShower()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetCanBeDamaged(false);
}

FLootRoll ABossLootShower::Plan(const ULootTable* Table, int32 Level, float Luck, FRandomStream& Random, TOptional<EAmmoType> KillAmmo,
	bool bWeapons, int32 BonusAmmo)
{
	FLootRoll Planned = ULootLibrary::RollLoot(Table, Level, Luck, Random, KillAmmo, bWeapons);
	if (!Table)
	{
		return Planned;
	}
	// A boss's thanks on top of its table: full boxes, the gun that did the work fed first.
	for (int32 Box = 0; Box < BonusAmmo; ++Box)
	{
		FAmmoDrop& Drop = Planned.Ammo.AddDefaulted_GetRef();
		Drop.Type = ULootLibrary::PickAmmoType(Table, Random, KillAmmo);
		Drop.Amount = LooterLoot::ChestAmmoAmount;
	}
	return Planned;
}

TArray<int32> ABossLootShower::Order(const FLootRoll& Planned)
{
	TArray<int32> Pieces;
	for (int32 Box = 0; Box < Planned.Ammo.Num(); ++Box)
	{
		Pieces.Add(-1 - Box);
	}
	TArray<int32> Guns;
	for (int32 Gun = 0; Gun < Planned.Weapons.Num(); ++Gun)
	{
		Guns.Add(Gun);
	}
	// Stable, so guns of one rarity keep the roll's order.
	Guns.StableSort([&Planned](int32 A, int32 B) { return Planned.Weapons[A].Rarity < Planned.Weapons[B].Rarity; });
	Pieces.Append(Guns);
	return Pieces;
}

ABossLootShower* ABossLootShower::Throw(AActor& Boss, const FVector& From, const FBossLootShowerSettings& Settings)
{
	UWorld* World = Boss.GetWorld();
	const ULootDropComponent* Loot = Boss.FindComponentByClass<ULootDropComponent>();
	if (!World || !Loot)
	{
		return nullptr;
	}
	// Rolled now, as the loot drop component rolls a death: the gun that landed the killing shot is known only now.
	const ULootTable* Table = Loot->LootTable ? Loot->LootTable.Get() : ULootLibrary::GetDefaultLootTable();
	const AWeaponBase* KillWeapon = AWeaponBase::FindKillWeapon(&Boss);
	const TOptional<EAmmoType> KillAmmo = KillWeapon ? TOptional<EAmmoType>(KillWeapon->GetAmmoType()) : TOptional<EAmmoType>();
	const float Luck = Loot->ExtraLuck + WeaponCurseEffects::KillLootLuck(KillWeapon);
	const bool bWeapons = UAreaRulesSubsystem::DropsGunsAt(&Boss);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABossLootShower* Shower = World->SpawnActor<ABossLootShower>(From, FRotator::ZeroRotator, Params);
	if (!Shower)
	{
		return nullptr;
	}
	FRandomStream Random(FMath::Rand());
	Shower->Roll = Plan(Table, Loot->Level, Luck, Random, KillAmmo, bWeapons, Settings.BonusAmmo);
	Shower->Queue = Order(Shower->Roll);
	Shower->Settings = Settings;
	Shower->Wait = Settings.Delay;
	Shower->StartYaw = Random.FRandRange(0.f, 360.f);
	Shower->SetLifeSpan(ShowerLifeSeconds);
	UE_LOG(LogLooter, Log, TEXT("%s's loot shower: %d guns and %d ammo boxes (%d bonus)%s."), *Boss.GetActorNameOrLabel(), Shower->Roll.Weapons.Num(),
		Shower->Roll.Ammo.Num(), Settings.BonusAmmo, Settings.bAfterScene ? TEXT(", after the scene") : TEXT(""));
	return Shower;
}

void ABossLootShower::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ABossLootShower::Advance(float DeltaSeconds)
{
	if (Thrown >= Queue.Num())
	{
		Destroy();
		return;
	}
	// A scene holds it (the player can't see or take anything meanwhile); once it's over, a moment's grace.
	if (Settings.bAfterScene)
	{
		const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
		if (Scenes && Scenes->IsPlaying())
		{
			Wait = FMath::Max(Wait, AfterSceneWait);
			return;
		}
	}
	Wait -= DeltaSeconds;
	if (Wait > 0.f)
	{
		return;
	}
	if (!bBurst)
	{
		Burst();
	}
	ThrowPiece(Queue[Thrown], Thrown);
	++Thrown;
	if (Thrown < Queue.Num())
	{
		// A gun waits a beat longer, so each one is seen leaving.
		Wait += Settings.Interval + (Queue[Thrown] >= 0 ? Settings.GunPause : 0.f);
	}
}

void ABossLootShower::Burst()
{
	bBurst = true;
	UWorld* World = GetWorld();
	const FVector Where = GetActorLocation() + FVector(0.0, 0.0, PopHeight);
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnFlash(Where, FVector::UpVector, BurstFlashScale);
	}
	LooterSound::PlayAt(this, LooterSoundCue::BossLootBurst, Where);
}

void ABossLootShower::ThrowPiece(int32 Piece, int32 Index)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector Start = GetActorLocation() + FVector(0.0, 0.0, PopHeight);
	const FVector Velocity = BossRules::ShowerThrow(Index, StartYaw, Settings, static_cast<float>(FMath::Abs(World->GetGravityZ())));
	float Pitch = 1.f;
	if (Piece >= 0 && Roll.Weapons.IsValidIndex(Piece))
	{
		const FWeaponInstanceData& Gun = Roll.Weapons[Piece];
		const FTransform Placed(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Start);
		if (AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(this, Gun, Placed))
		{
			Weapon->Toss(Velocity);
		}
		// Each rarity a little brighter.
		Pitch = 1.f + 0.06f * static_cast<float>(Gun.Rarity);
	}
	else
	{
		const int32 Box = -1 - Piece;
		if (Roll.Ammo.IsValidIndex(Box))
		{
			if (AAmmoPickup* Pickup = AAmmoPickup::SpawnAmmo(World, Roll.Ammo[Box].Type, Roll.Ammo[Box].Amount, Start))
			{
				Pickup->Toss(Velocity);
			}
		}
	}
	if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnFlash(Start, Velocity.GetSafeNormal(), PieceFlashScale);
	}
	LooterSound::PlayAt(this, LooterSoundCue::BossLootPop, Start, 1.f, Pitch);
}
