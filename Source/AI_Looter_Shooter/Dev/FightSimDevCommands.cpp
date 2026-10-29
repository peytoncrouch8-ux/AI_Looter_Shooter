// Developer console commands that set a fight up and measure it (FIGHTS.md section 5; not in shipping builds):
// Looter.Fight.Sim and Looter.Fight.Sim.Place. The numbers come out through Looter.Fight.Log (Dev/FightLog.h).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Dev/FightLog.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/WorldQueries.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Where Looter.Fight.Sim puts the player unless at= says otherwise: Ransom's Rest's town (the farm is a safe zone), facing north. */
	constexpr float DefaultX = 0.f;
	constexpr float DefaultY = -1400.f;
	constexpr float DefaultYaw = 90.f;
	/** The pack comes in over an arc ahead, alternately nearer and farther; the Spitters stand back of it. */
	constexpr float NearestPack = 800.f;
	constexpr float SpitterRing = 1500.f;

	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	/** Runs an existing console command as typed in the game (through the player, as the HUD shots do). */
	void RunConsole(UWorld& World, const FString& Command)
	{
		if (APlayerController* Controller = World.GetFirstPlayerController())
		{
			Controller->ConsoleCommand(Command);
		}
		else if (GEngine)
		{
			GEngine->Exec(&World, *Command);
		}
	}

	/** The ground under a spot (terrain and solid props, never volumes), from well above it to well below. */
	bool FindGround(UWorld* World, const FVector& Spot, FVector& OutGround)
	{
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("FightSim"));
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Spot + FVector(0.f, 0.f, 800.f), Spot - FVector(0.f, 0.f, 3000.f),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			OutGround = Hit.ImpactPoint;
			return true;
		}
		return false;
	}

	TSubclassOf<ACreatureBase> FindKind(const FString& Kind)
	{
		if (Kind.Equals(TEXT("Spider"), ESearchCase::IgnoreCase))
		{
			return ASpiderCreature::StaticClass();
		}
		if (Kind.Equals(TEXT("Slime"), ESearchCase::IgnoreCase))
		{
			return ASlimeCreature::StaticClass();
		}
		if (Kind.Equals(TEXT("Unpaid"), ESearchCase::IgnoreCase))
		{
			return AUnpaidCreature::StaticClass();
		}
		if (Kind.Equals(TEXT("Spitter"), ESearchCase::IgnoreCase))
		{
			return ASpitterSpider::StaticClass();
		}
		return nullptr;
	}

	/** The weapon definition whose asset name contains the text ("AssaultRifle" finds DA_AssaultRifle). */
	UWeaponDefinition* FindDefinition(const FString& Text)
	{
		TArray<FAssetData> Assets;
		IAssetRegistry::GetChecked().GetAssetsByClass(UWeaponDefinition::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Contains(Text))
			{
				return Cast<UWeaponDefinition>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	/** An Uncommon assault rifle of Level, in hand, and every ammo pool full: the level-matched gun the targets are set for. */
	void ArmPlayer(UWeaponManagerComponent& Manager, int32 Level)
	{
		if (UWeaponDefinition* Definition = FindDefinition(TEXT("AssaultRifle")))
		{
			const FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Uncommon, Level);
			// A free slot takes it; with every slot full the gun in hand is replaced (UWeaponManagerComponent::AddWeapon).
			if (const AWeaponBase* Rifle = Manager.GiveWeapon(Instance))
			{
				const int32 Slot = Manager.GetWeapons().IndexOfByKey(Rifle);
				if (Slot != INDEX_NONE)
				{
					Manager.EquipSlot(Slot);
				}
			}
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Fight.Sim: no assault rifle definition; the player keeps the gun they have."));
		}
		for (const EAmmoType Type : LooterAmmo::AllTypes())
		{
			Manager.AddAmmo(Type, FMath::Max(0, Manager.GetMaxAmmo(Type) - Manager.GetAmmo(Type)));
		}
	}

	/**
	 * Sets the player up as a level-matched player: Level, full health, a Common ward of that level worn full, the rifle, ammo,
	 * and able to be hurt.
	 */
	void PreparePlayer(UWorld* GameWorld, APawn& Pawn, const APlayerController& Controller, int32 Level)
	{
		if (const ULocalPlayer* LocalPlayer = Controller.GetLocalPlayer())
		{
			if (UPlayerProgressionSubsystem* Progression = LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>())
			{
				Progression->SetLevel(Level);
			}
		}
		UHealthComponent* Health = Pawn.FindComponentByClass<UHealthComponent>();
		if (Health)
		{
			Health->bInvulnerable = false;
			Health->ResetHealth();
		}
		Pawn.SetCanBeDamaged(true);
		if (UWeaponManagerComponent* Manager = Pawn.FindComponentByClass<UWeaponManagerComponent>())
		{
			ArmPlayer(*Manager, Level);
		}
		// The ward of the player's level worn full (the standard build: no lean to the numbers), as the first gun brings one.
		RunConsole(*GameWorld, FString::Printf(TEXT("Looter.Shield.Give Common %d Standard wear"), Level));
	}

	/** "x,y,yaw" of at=; false when it isn't three numbers. */
	bool ParseAt(const FString& Text, float& OutX, float& OutY, float& OutYaw)
	{
		TArray<FString> Parts;
		Text.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() != 3)
		{
			return false;
		}
		OutX = FCString::Atof(*Parts[0]);
		OutY = FCString::Atof(*Parts[1]);
		OutYaw = FCString::Atof(*Parts[2]);
		return true;
	}

	void SimUsage()
	{
		UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.Fight.Sim <Spider|Slime|Unpaid|Spitter> <count> [rank] [level] [leader 0|1] [spitters n] [at=x,y,yaw] ")
			TEXT("(rank: Basic, Restless, Gravebound, Hallowed, Ethereal)."));
	}

	/** Puts Count of Class on an arc ahead of the player (like Looter.Perf.Horde), on the player, from Nearest out; returns how many stood. */
	int32 SpawnArc(UWorld* GameWorld, APawn& Pawn, float Yaw, TSubclassOf<ACreatureBase> Class, int32 Count, const ACreatureBase::FRuntimeSpawn& Spawn,
		float Nearest, float Spread, bool bFirstLeads, ACreatureBase** OutFirst)
	{
		const FVector Ahead = FRotator(0.f, Yaw, 0.f).Vector();
		int32 Spawned = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Across = Count > 1 ? FMath::Lerp(-Spread, Spread, static_cast<float>(Index) / (Count - 1)) : 0.f;
			const float Distance = Nearest + static_cast<float>(Index % 3) * 300.f;
			FVector Ground;
			if (!FindGround(GameWorld, Pawn.GetActorLocation() + Ahead.RotateAngleAxis(Across, FVector::UpVector) * Distance, Ground))
			{
				continue;
			}
			const float FacingYaw = static_cast<float>((Pawn.GetActorLocation() - Ground).Rotation().Yaw);
			if (ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(GameWorld, Class, Ground, FacingYaw, Spawn))
			{
				Creature->AlertTo(&Pawn);
				if (bFirstLeads && Spawned == 0)
				{
					if (UCreaturePackComponent* Pack = Creature->GetPack())
					{
						Pack->SetLeader(true);
					}
					if (OutFirst)
					{
						*OutFirst = Creature;
					}
				}
				++Spawned;
			}
		}
		return Spawned;
	}

	/**
	 * Looter.Fight.Sim <kind> <count> [rank] [level] [leader 0|1] [spitters n] [at=x,y,yaw]: a measured fight. The player becomes a
	 * level-matched player (Level, an Uncommon rifle, a ward, full health and ammo, able to be hurt), stands at (0, -1400) facing
	 * 90 (or at=) on the ground there, the pack comes at them over an arc ahead at Level, and the fight log starts. Read the
	 * result with Looter.Fight.Log report.
	 */
	void Sim(const TArray<FString>& AllArgs, UWorld* World)
	{
		TArray<FString> Args;
		float X = DefaultX;
		float Y = DefaultY;
		float Yaw = DefaultYaw;
		for (const FString& Arg : AllArgs)
		{
			FString Key;
			FString Value;
			if (Arg.Split(TEXT("="), &Key, &Value))
			{
				if (!Key.Equals(TEXT("at"), ESearchCase::IgnoreCase) || !ParseAt(Value, X, Y, Yaw))
				{
					SimUsage();
					return;
				}
			}
			else
			{
				Args.Add(Arg);
			}
		}
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Fight.Sim: no player (start the game first)."));
			return;
		}
		const TSubclassOf<ACreatureBase> Class = Args.Num() > 0 ? FindKind(Args[0]) : nullptr;
		ECreatureRank Rank = ECreatureRank::Basic;
		if (!Class || (Args.Num() > 2 && !UCreatureRankSettings::ParseRank(Args[2], Rank)))
		{
			SimUsage();
			return;
		}
		const int32 Count = Args.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 40) : 6;
		const int32 Level = Args.Num() > 3 ? FMath::Clamp(FCString::Atoi(*Args[3]), 1, 65) : 1;
		const bool bLeader = Args.Num() > 4 && FCString::Atoi(*Args[4]) != 0;
		const int32 Spitters = Args.Num() > 5 ? FMath::Clamp(FCString::Atoi(*Args[5]), 0, 8) : 0;

		// The player first, so the log starts on a full, level-matched player and the pack sees them where they will stand.
		FVector Ground;
		if (FindGround(GameWorld, FVector(X, Y, Pawn->GetActorLocation().Z), Ground))
		{
			const float HalfHeight = Pawn->GetRootComponent()->Bounds.BoxExtent.Z;
			Pawn->SetActorLocation(Ground + FVector(0.f, 0.f, HalfHeight + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
			Controller->SetControlRotation(FRotator(-5.f, Yaw, 0.f));
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Fight.Sim: no ground at (%.0f, %.0f); the fight comes to the player where they stand."), X, Y);
			Yaw = static_cast<float>(Controller->GetControlRotation().Yaw);
		}
		PreparePlayer(GameWorld, *Pawn, *Controller, Level);

		UFightLogSubsystem* Log = UFightLogSubsystem::Get(GameWorld);
		if (Log)
		{
			Log->Start();
		}

		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = Rank;
		Spawn.Level = Level;
		ACreatureBase* Head = nullptr;
		int32 Spawned = SpawnArc(GameWorld, *Pawn, Yaw, Class, Count, Spawn, NearestPack, 70.f, bLeader, &Head);
		int32 SpitterCount = 0;
		if (Spitters > 0)
		{
			ACreatureBase::FRuntimeSpawn SpitterSpawn;
			SpitterSpawn.Level = Level;
			SpitterCount = SpawnArc(GameWorld, *Pawn, Yaw, ASpitterSpider::StaticClass(), Spitters, SpitterSpawn, SpitterRing, 40.f, false, nullptr);
		}
		UE_LOG(LogLooter, Display, TEXT("FIGHT sim: %d of %d %s (%s, level %d%s)%s hunting a level %d player at (%.0f, %.0f, %.0f), who can be hurt; the log is %s. ")
			TEXT("Looter.Fight.Log report when it is over."),
			Spawned, Count, *Class->GetName(), *UCreatureRankSettings::GetRankName(Rank), Level, bLeader ? TEXT(", the first leads") : TEXT(""),
			Spitters > 0 ? *FString::Printf(TEXT(" and %d of %d Spitters"), SpitterCount, Spitters) : TEXT(""), Level, Pawn->GetActorLocation().X,
			Pawn->GetActorLocation().Y, Pawn->GetActorLocation().Z, Log ? TEXT("on") : TEXT("not available"));
	}

	/**
	 * Looter.Fight.Sim.Place <spawner id>: the fight a placed spawner makes, measured: the log starts, the player is healed and able
	 * to be hurt, and the spawner's next wave starts now whatever its story says (Looter.Encounter.Wave <id> force). The player
	 * goes there themselves (tools\play.py goto), at the level the game has them.
	 */
	void SimPlace(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn || Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.Fight.Sim.Place <spawner id> (Looter.Encounter.List shows the ids)"));
			return;
		}
		if (UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>())
		{
			Health->bInvulnerable = false;
			Health->ResetHealth();
		}
		Pawn->SetCanBeDamaged(true);
		if (UFightLogSubsystem* Log = UFightLogSubsystem::Get(GameWorld))
		{
			Log->Start();
		}
		RunConsole(*GameWorld, FString::Printf(TEXT("Looter.Encounter.Wave %s force"), *Args[0]));
		UE_LOG(LogLooter, Display, TEXT("FIGHT sim: spawner %s forced and the log is on. Looter.Fight.Log report when it is over."), *Args[0]);
	}

	FAutoConsoleCommandWithWorldAndArgs SimCommand(
		TEXT("Looter.Fight.Sim"),
		TEXT("Looter.Fight.Sim <Spider|Slime|Unpaid|Spitter> <count> [rank] [level] [leader 0|1] [spitters n] [at=x,y,yaw]: a measured fight. ")
		TEXT("The player becomes a level-matched player (the level, an Uncommon assault rifle of it, a Common ward of it, full health and ammo, able ")
		TEXT("to be hurt) at (0, -1400) facing 90 or at=, the pack of that rank and level comes at them over an arc ahead (leader 1: the first ")
		TEXT("leads), spitters adds that many Spitters farther out, and the fight log starts: Looter.Fight.Log report reads it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Sim));

	FAutoConsoleCommandWithWorldAndArgs SimPlaceCommand(
		TEXT("Looter.Fight.Sim.Place"),
		TEXT("Looter.Fight.Sim.Place <spawner id>: starts the fight log, heals the player and forces that spawner's next wave ")
		TEXT("(Looter.Encounter.Wave <id> force), to measure a placed fight; go there first (play.py goto)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SimPlace));
}

#endif
