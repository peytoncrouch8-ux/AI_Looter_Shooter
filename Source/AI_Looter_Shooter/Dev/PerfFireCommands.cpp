// Developer console command that fires the player's gun into a horde for a stretch, so gunfire has a perf row (not in
// shipping builds).
//
// Looter.Perf.Horde puts a fight in view but never shoots, so the bullets, tracers, impacts, damage numbers, hit markers and
// kill feedback were in no perf row (audit 2026-11-03). Looter.Perf.Fire <kind> [seconds] [kill] makes the local player
// shoot: the gun of that kind given if they have none and taken in hand, then aimed at the nearest creature and fired as a
// player's clicking would, for seconds (the trigger is worked every frame; the gun's own cadence decides each shot).
// For perf.ps1 -Exec: "Looter.Perf.Horde Spider 10 Basic 0 -1400 90,Looter.Perf.Fire LMG 20".

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/ActorIndexSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The most world seconds to wait for the player to exist, the gun to be in hand, and then for a horde to show before firing blind. */
	constexpr double ReadyLimit = 30.0;
	constexpr double HordeLimit = 4.0;
	/** Creatures farther than this (cm) are not shot at; the nearest one is. */
	constexpr float TargetRange = 6000.f;
	/** How often (s) the ammo and the horde's health are made up. */
	constexpr float KeepUpSeconds = 0.5f;
	/** The gun that is given: Common (no effect or element part to add looks to the row), at this level. */
	constexpr int32 GunLevel = 5;

	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
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

	/** A gun kind the command takes by name, and the data asset of the plain gun of that kind. */
	struct FFireKind
	{
		const TCHAR* Name;
		EWeaponKind Kind;
		const TCHAR* Path;
	};

	const FFireKind FireKinds[] = {
		{ TEXT("Rifle"), EWeaponKind::Rifle, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle") },
		{ TEXT("Shotgun"), EWeaponKind::Shotgun, TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun") },
		{ TEXT("LMG"), EWeaponKind::LMG, TEXT("/Game/Weapons/Data/DA_LMG.DA_LMG") },
		{ TEXT("SMG"), EWeaponKind::SMG, TEXT("/Game/Weapons/Data/DA_SMG.DA_SMG") },
		{ TEXT("Revolver"), EWeaponKind::Revolver, TEXT("/Game/Weapons/Data/DA_Revolver.DA_Revolver") },
		{ TEXT("Sniper"), EWeaponKind::Sniper, TEXT("/Game/Weapons/Data/DA_Sniper.DA_Sniper") },
	};

	const FFireKind* FindFireKind(const FString& Word)
	{
		for (const FFireKind& Each : FireKinds)
		{
			if (Word.Equals(Each.Name, ESearchCase::IgnoreCase))
			{
				return &Each;
			}
		}
		return nullptr;
	}

	EWeaponKind KindOf(const AWeaponBase& Gun)
	{
		return Gun.GetInstance().Definition ? Gun.GetInstance().Definition->Kind : EWeaponKind::None;
	}

	/** One run of Looter.Perf.Fire: gets the gun in hand, waits for the horde, fires, reports. Stepped every frame by a ticker. */
	struct FFireRun : public TSharedFromThis<FFireRun>
	{
		enum class EPhase : uint8
		{
			/** The player, and the gun of the kind in hand. */
			Readying,
			/** The gun is up; a creature to shoot at is waited for (a few seconds). */
			Horde,
			Firing
		};

		TWeakObjectPtr<UWorld> World;
		const FFireKind* Kind = nullptr;
		double Seconds = 10.0;
		bool bLetDie = false;

		EPhase Phase = EPhase::Readying;
		/** World time the phase began (negative: not yet seen). */
		double PhaseStart = -1.0;
		double FireStart = 0.0;
		float KeepUpClock = 0.f;
		bool bGunAsked = false;
		/** The gun could not be given: the run ends. */
		bool bNoGun = false;
		bool bWasHurtable = true;

		// What the run saw, for the report.
		int32 Bullets = 0;
		int32 Hits = 0;
		int32 Crits = 0;
		float Damage = 0.f;
		TSet<TWeakObjectPtr<ACreatureBase>> Targeted;
		/** Health components made unkillable for the run (the horde stays on its feet: the same load every second). */
		TArray<TWeakObjectPtr<UHealthComponent>> Spared;
		TWeakObjectPtr<UBulletSubsystem> BulletSystem;
		FDelegateHandle FiringHandle;
		FDelegateHandle HitHandle;
		TWeakObjectPtr<UWeaponManagerComponent> Shooter;

		/** True while the run goes on. */
		bool Step(float DeltaTime);

	private:
		/** Asks once for the gun of the kind (given when the player has none) and reports it up and out of any swap. */
		bool GunReady(UWeaponManagerComponent& Inventory);
		ACreatureBase* FindTarget(UActorIndexSubsystem& Index, const FVector& From) const;
		void BeginFiring(UWorld& GameWorld, APawn& Pawn, UWeaponManagerComponent& Inventory);
		void KeepUp(UWeaponManagerComponent& Inventory, UActorIndexSubsystem& Index, const FVector& From);
		/** Puts everything back and says what happened; always false (the run is over). */
		bool End(const TCHAR* Why);
	};

	bool FFireRun::GunReady(UWeaponManagerComponent& Inventory)
	{
		if (!bGunAsked)
		{
			bGunAsked = true;
			const AWeaponBase* InHand = Inventory.GetActiveWeapon();
			if (!InHand || KindOf(*InHand) != Kind->Kind)
			{
				int32 Slot = INDEX_NONE;
				const TArray<AWeaponBase*> Carried = Inventory.GetWeapons();
				for (int32 Index = 0; Index < Carried.Num() && Slot == INDEX_NONE; ++Index)
				{
					Slot = Carried[Index] && KindOf(*Carried[Index]) == Kind->Kind ? Index : INDEX_NONE;
				}
				if (Slot == INDEX_NONE)
				{
					UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, Kind->Path);
					AWeaponBase* Given = Definition ? Inventory.GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Common, GunLevel)) : nullptr;
					if (!Given)
					{
						UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.Fire: no %s could be given."), Kind->Name);
						bNoGun = true;
						return false;
					}
					Slot = Inventory.GetWeapons().Find(Given);
				}
				// A swap in view takes a moment; the gun is in hand once it ends.
				Inventory.EquipSlot(Slot);
			}
		}
		const AWeaponBase* InHand = Inventory.GetActiveWeapon();
		return InHand && KindOf(*InHand) == Kind->Kind && !Inventory.IsSwapping();
	}

	ACreatureBase* FFireRun::FindTarget(UActorIndexSubsystem& Index, const FVector& From) const
	{
		ACreatureBase* Nearest = nullptr;
		double NearestSquared = FMath::Square(static_cast<double>(TargetRange));
		for (const FIndexedCreature& Entry : Index.GetCreatures())
		{
			ACreatureBase* Creature = Entry.Creature.Get();
			if (!Creature || Creature->IsDead() || Creature->IsHidden())
			{
				continue;
			}
			const double DistanceSquared = FVector::DistSquared(Creature->GetActorLocation(), From);
			if (DistanceSquared < NearestSquared)
			{
				NearestSquared = DistanceSquared;
				Nearest = Creature;
			}
		}
		return Nearest;
	}

	void FFireRun::BeginFiring(UWorld& GameWorld, APawn& Pawn, UWeaponManagerComponent& Inventory)
	{
		Phase = EPhase::Firing;
		FireStart = GameWorld.GetTimeSeconds();
		Shooter = &Inventory;
		// The player can't be hurt for the run (Looter.Perf.Horde does the same), put back after unless it was off already.
		bWasHurtable = Pawn.CanBeDamaged();
		Pawn.SetCanBeDamaged(false);
		// What the shots amounted to, for the report: the bullet subsystem tells of every bullet fired and every hit.
		if (UBulletSubsystem* Bullet = GameWorld.GetSubsystem<UBulletSubsystem>())
		{
			BulletSystem = Bullet;
			const TSharedRef<FFireRun> Self = AsShared();
			const TWeakObjectPtr<AActor> Player(&Pawn);
			FiringHandle = Bullet->OnBulletFiring.AddLambda([Self, Player](FBulletShot& Shot)
				{
					Self->Bullets += Shot.Shooter == Player ? 1 : 0;
				});
			HitHandle = Bullet->OnBulletHit.AddLambda([Self](const FHitResult& Hit, float Dealt, bool bCritical)
				{
					++Self->Hits;
					Self->Crits += bCritical ? 1 : 0;
					Self->Damage += Dealt;
				});
		}
		KeepUpClock = KeepUpSeconds;
	}

	void FFireRun::KeepUp(UWeaponManagerComponent& Inventory, UActorIndexSubsystem& Index, const FVector& From)
	{
		// The shots go on for the run, however many rounds that is.
		if (const AWeaponBase* Gun = Inventory.GetActiveWeapon())
		{
			if (Gun->GetInstance().Definition)
			{
				Inventory.AddAmmo(Gun->GetInstance().Definition->AmmoType, 100000);
			}
		}
		if (bLetDie)
		{
			return;
		}
		// The horde takes every hit (numbers, flinches, markers) and never loses health, so the run is the same load at
		// second 1 and second 20. "kill" lets them die instead.
		for (const FIndexedCreature& Entry : Index.GetCreatures())
		{
			const ACreatureBase* Creature = Entry.Creature.Get();
			UHealthComponent* Health = Creature && FVector::DistSquared(Creature->GetActorLocation(), From) < FMath::Square(static_cast<double>(TargetRange))
				? Creature->FindComponentByClass<UHealthComponent>() : nullptr;
			if (Health && !Health->bInvulnerable)
			{
				Health->bInvulnerable = true;
				Spared.Add(Health);
			}
		}
	}

	bool FFireRun::End(const TCHAR* Why)
	{
		if (UWeaponManagerComponent* Inventory = Shooter.Get())
		{
			Inventory->StopFire();
			if (APawn* Pawn = Cast<APawn>(Inventory->GetOwner()))
			{
				Pawn->SetCanBeDamaged(bWasHurtable);
			}
		}
		if (UBulletSubsystem* Bullet = BulletSystem.Get())
		{
			Bullet->OnBulletFiring.Remove(FiringHandle);
			Bullet->OnBulletHit.Remove(HitHandle);
		}
		for (const TWeakObjectPtr<UHealthComponent>& Entry : Spared)
		{
			if (UHealthComponent* Health = Entry.Get())
			{
				Health->bInvulnerable = false;
			}
		}
		Spared.Reset();
		UWorld* GameWorld = World.Get();
		const double Fired = GameWorld && Phase == EPhase::Firing && Shooter.IsValid() ? GameWorld->GetTimeSeconds() - FireStart : 0.0;
		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.Fire: %s, %s: %.1f s of fire, %d bullets, %d hits (%d critical, %.0f damage) on %d creatures%s."),
			Kind->Name, Why, Fired, Bullets, Hits, Crits, Damage, Targeted.Num(), bLetDie ? TEXT("") : TEXT(" kept on their feet"));
		return false;
	}

	bool FFireRun::Step(float DeltaTime)
	{
		UWorld* GameWorld = World.Get();
		if (!GameWorld || GameWorld->bIsTearingDown)
		{
			return End(TEXT("the world went away"));
		}
		const double Now = GameWorld->GetTimeSeconds();
		if (PhaseStart < 0.0)
		{
			PhaseStart = Now;
		}
		APlayerController* Controller = GameWorld->GetFirstPlayerController();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		UWeaponManagerComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
		UActorIndexSubsystem* Index = UActorIndexSubsystem::Get(GameWorld);
		if (!Inventory || !Index)
		{
			// From the command line the game may not have its player yet: look again next frame.
			return Now - PhaseStart < ReadyLimit ? true : End(TEXT("the game never had a player"));
		}

		if (Phase == EPhase::Readying)
		{
			if (!GunReady(*Inventory))
			{
				if (bNoGun)
				{
					return End(TEXT("no gun"));
				}
				return Now - PhaseStart < ReadyLimit ? true : End(TEXT("the gun never came up"));
			}
			Phase = EPhase::Horde;
			PhaseStart = Now;
		}
		if (Phase == EPhase::Horde)
		{
			// The horde Looter.Perf.Horde sends comes out of the ground as the player appears: wait for it a moment, then fire
			// at whatever is ahead (the same run measures a gun alone on open ground).
			if (!FindTarget(*Index, Pawn->GetActorLocation()) && Now - PhaseStart < HordeLimit)
			{
				return true;
			}
			BeginFiring(*GameWorld, *Pawn, *Inventory);
		}

		if (Now - FireStart >= Seconds)
		{
			return End(TEXT("done"));
		}
		KeepUpClock += DeltaTime;
		if (KeepUpClock >= KeepUpSeconds)
		{
			KeepUpClock = 0.f;
			KeepUp(*Inventory, *Index, Pawn->GetActorLocation());
		}
		if (ACreatureBase* Target = FindTarget(*Index, Pawn->GetActorLocation()))
		{
			Targeted.Add(Target);
			// At the middle of its body, from the eyes: the shots land on it wherever it runs.
			Controller->SetControlRotation((Target->GetComponentsBoundingBox().GetCenter() - Pawn->GetPawnViewLocation()).Rotation());
		}
		// A click every frame: the trigger takes a press only when the gun is ready (or about to be), so semi-auto guns fire at
		// their own pace and full-auto ones hold (WeaponTrigger::Decide).
		Inventory->StartFire();
		return true;
	}

	void FireCommand(const TArray<FString>& Args, UWorld* World)
	{
		const FFireKind* Kind = Args.Num() > 0 ? FindFireKind(Args[0]) : nullptr;
		if (!Kind)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.Fire <Rifle|Shotgun|LMG|SMG|Revolver|Sniper> [seconds] [kill]: no such gun."));
			return;
		}
		UWorld* GameWorld = FindGameWorld(World);
		if (!GameWorld)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.Fire: no game (start the game first)."));
			return;
		}
		const TSharedRef<FFireRun> Run = MakeShared<FFireRun>();
		Run->World = GameWorld;
		Run->Kind = Kind;
		Run->Seconds = Args.Num() > 1 && Args[1].IsNumeric() ? FMath::Clamp(FCString::Atod(*Args[1]), 1.0, 300.0) : 10.0;
		Run->bLetDie = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("kill"), ESearchCase::IgnoreCase); });
		FTSTicker::GetCoreTicker().AddTicker(TEXT("Looter.Perf.Fire"), 0.f, [Run](float DeltaTime) { return Run->Step(DeltaTime); });
	}

	FAutoConsoleCommandWithWorldAndArgs FireCommandRegistration(
		TEXT("Looter.Perf.Fire"),
		TEXT("Looter.Perf.Fire <Rifle|Shotgun|LMG|SMG|Revolver|Sniper> [seconds] [kill]: to measure gunfire, gives the player the gun if ")
		TEXT("they have none, takes it in hand, aims it at the nearest creature and fires it as a clicking player would for seconds ")
		TEXT("(10), the player unhurtable. The creatures take every hit but keep their health, so the load stays the same; kill lets ")
		TEXT("them die. Waits for the player and a horde (Looter.Perf.Horde) first. perf.ps1 -Map /Game/Maps/Lvl_RansomsRest -Frames ")
		TEXT("3600 -Exec \"Looter.Quality Medium,Looter.Perf.Horde Spider 10 Basic 0 -1400 90,Looter.Perf.Fire LMG 20\"."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FireCommand));
}

#endif
