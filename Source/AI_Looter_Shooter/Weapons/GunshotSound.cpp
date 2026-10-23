#include "Weapons/GunshotSound.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Weapons/GunfeelSoundTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponSounds.h"
#include "World/WorldQueries.h"

// A shot as layers (Gunfeel step 2, GUNFEEL-SOUND): the shooter's own gun plays the punch (Core), its action (Mech), the
// chest thump (Low) and the land's answer by its surroundings (a tail); anyone else's the punch and the tail, or the far
// shot past Looter.Gunfeel.Sound.DistantFrom. The surroundings come from a few short traces round the shooter, looked at
// again at most every quarter second per gun. An automatic's burst plays the tail on every Nth shot and one long at its end
// (the gun feel pass: at 900 rpm every shot's tail was 60 voices a second, cutting each other, and no burst ended in a tail).
// The rules are GunfeelSoundTuning's.

namespace
{
	const TCHAR* KindWord(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return TEXT("Shotgun");
		case EWeaponKind::Revolver: return TEXT("Revolver");
		case EWeaponKind::SMG: return TEXT("SMG");
		case EWeaponKind::Sniper: return TEXT("Sniper");
		case EWeaponKind::LMG: return TEXT("LMG");
		case EWeaponKind::Launcher: return TEXT("Launcher");
		default: return TEXT("Rifle");
		}
	}

	const TCHAR* LayerWord(EGunshotLayer Layer)
	{
		switch (Layer)
		{
		case EGunshotLayer::Mech: return TEXT("Mech");
		case EGunshotLayer::Low: return TEXT("Low");
		case EGunshotLayer::TailOpen: return TEXT("TailOpen");
		case EGunshotLayer::TailTown: return TEXT("TailTown");
		case EGunshotLayer::TailInside: return TEXT("TailInside");
		case EGunshotLayer::Distant: return TEXT("Distant");
		case EGunshotLayer::LowAmmo: return TEXT("LowAmmo");
		case EGunshotLayer::LastRound: return TEXT("LastRound");
		case EGunshotLayer::HeatHiss: return TEXT("HeatHiss");
		case EGunshotLayer::Rack: return TEXT("Rack");
		default: return TEXT("Fire");
		}
	}

	FName CoreCue(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return LooterSoundCue::Gunshot::ShotgunCore;
		case EWeaponKind::Revolver: return LooterSoundCue::Gunshot::RevolverCore;
		case EWeaponKind::SMG: return LooterSoundCue::Gunshot::SMGCore;
		case EWeaponKind::Sniper: return LooterSoundCue::Gunshot::SniperCore;
		case EWeaponKind::LMG: return LooterSoundCue::Gunshot::LMGCore;
		case EWeaponKind::Launcher: return LooterSoundCue::Gunshot::LauncherCore;
		default: return LooterSoundCue::Gunshot::RifleCore;
		}
	}

	/**
	 * What a gun remembers between shots: its last look at its surroundings (a burst fires many shots from the same spot)
	 * and its burst (when it last fired, how many shots into the burst, how many since a tail last played).
	 */
	struct FShooterMemory
	{
		double ProbeAt = -1.0;
		GunfeelSound::ESurroundings Surroundings = GunfeelSound::ESurroundings::Open;
		double LastShotAt = -1.0;
		int32 BurstShots = 0;
		int32 ShotsSinceTail = 0;
	};

	TMap<TWeakObjectPtr<const AWeaponBase>, FShooterMemory> GMemories;

	FShooterMemory& MemoryOf(const AWeaponBase& Weapon)
	{
		return GMemories.FindOrAdd(TWeakObjectPtr<const AWeaponBase>(&Weapon));
	}

	/** Guns come and go (drops, swaps, levels): the ones gone are let go of now and then. Invalidates every FShooterMemory&. */
	void TrimMemories()
	{
		if (GMemories.Num() > 32)
		{
			for (auto It = GMemories.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid())
				{
					It.RemoveCurrent();
				}
			}
		}
	}

	/**
	 * What's round the shot, from up to six short world-static traces (volumes and the playable area's walls left out):
	 * straight up for a roof, a steep one forward and up (a ceiling the first missed through a gap; too steep to meet a
	 * wall's face), and four level ones round the shooter's facing for walls.
	 */
	GunfeelSound::ESurroundings Probe(const AWeaponBase& Weapon, UWorld& World)
	{
		const float Range = GunfeelSound::ProbeRange();
		if (Range <= 0.f)
		{
			return GunfeelSound::ESurroundings::Open;
		}
		const AActor* Holder = Weapon.GetOwner();
		const FVector From = Weapon.GetActorLocation();
		const double Yaw = Holder ? Holder->GetActorRotation().Yaw : Weapon.GetActorRotation().Yaw;
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("GunshotProbe"), Holder, false);
		Params.AddIgnoredActor(&Weapon);
		const FCollisionObjectQueryParams Static(ECC_WorldStatic);
		auto Hits = [&](const FVector& Direction)
		{
			return World.LineTraceTestByObjectType(From, From + Direction * Range, Static, Params);
		};

		bool bRoof = Hits(FVector::UpVector);
		if (!bRoof)
		{
			bRoof = Hits(FRotator(70.f, Yaw, 0.f).Vector());
		}
		int32 Walls = 0;
		for (int32 Side = 0; Side < 4; ++Side)
		{
			Walls += Hits(FRotator(0.f, Yaw + 90.f * Side, 0.f).Vector()) ? 1 : 0;
		}
		return GunfeelSound::Classify(bRoof, Walls);
	}

	GunfeelSound::ESurroundings SurroundingsOf(FShooterMemory& Memory, const AWeaponBase& Weapon, UWorld& World)
	{
		const double Now = World.GetTimeSeconds();
		if (GunfeelSound::ProbeStale(Now, Memory.ProbeAt))
		{
			Memory.Surroundings = Probe(Weapon, World);
			Memory.ProbeAt = Now;
		}
		return Memory.Surroundings;
	}

	/** One more shot of Kind's burst at Now; whether its tail plays (every Nth of an automatic's burst, else every one). */
	bool NoteShot(FShooterMemory& Memory, EWeaponKind Kind, double Now)
	{
		if (GunfeelSound::NewBurst(Now, Memory.LastShotAt, GunfeelSound::BurstGap()))
		{
			Memory.BurstShots = 0;
			Memory.ShotsSinceTail = 0;
		}
		const int32 ShotInBurst = Memory.BurstShots++;
		Memory.LastShotAt = Now;
		const bool bTail = GunfeelSound::TailPlays(ShotInBurst, GunfeelSound::AutoTailEvery(Kind));
		Memory.ShotsSinceTail = bTail ? 0 : Memory.ShotsSinceTail + 1;
		return bTail;
	}

	/** The player's own gun: a locally controlled, player-controlled holder's (an enemy's is heard, never felt). */
	bool IsOwnGun(const AWeaponBase& Weapon)
	{
		const APawn* Holder = Cast<APawn>(Weapon.GetOwner());
		return Holder && Holder->IsLocallyControlled() && Holder->IsPlayerControlled();
	}

	/** How far a sound at Where is from whoever's listening (the first local player's ears); 0 with no one to ask. */
	float DistanceToListener(const FVector& Where, UWorld& World)
	{
		APlayerController* Player = World.GetFirstPlayerController();
		if (!Player)
		{
			return 0.f;
		}
		FVector Ears;
		FVector Front;
		FVector Right;
		Player->GetAudioListenerPosition(Ears, Front, Right);
		return static_cast<float>(FVector::Dist(Ears, Where));
	}

	float DistanceToListener(const AWeaponBase& Weapon, UWorld& World)
	{
		return DistanceToListener(Weapon.GetActorLocation(), World);
	}

	/** Plays Kind's tail by Surroundings: on the gun for the player's own (it stays with the view), else where the shot was. */
	void PlayTail(const AWeaponBase& Weapon, EWeaponKind Kind, GunfeelSound::ESurroundings Surroundings, bool bOwnGun, float Volume)
	{
		const FName Cue = GunshotSound::CueFor(Kind, GunfeelSound::TailFor(Surroundings));
		if (bOwnGun)
		{
			LooterSound::PlayAttached(Cue, Weapon.GetRootComponent(), NAME_None, Volume);
		}
		else
		{
			LooterSound::PlayAt(&Weapon, Cue, Weapon.GetActorLocation(), Volume);
		}
	}

	/** The last round's sound, a moment after the shot (Looter.Gunfeel.Sound.LastRoundDelay) so the shot's own crack doesn't bury it. */
	void PlayLastRound(const AWeaponBase& Weapon, EWeaponKind Kind)
	{
		UWorld* World = Weapon.GetWorld();
		const FName Cue = GunshotSound::CueFor(Kind, EGunshotLayer::LastRound);
		const float Volume = GunfeelSound::LastRoundVolume();
		const float Delay = GunfeelSound::LastRoundDelay();
		if (!World || Delay <= 0.f)
		{
			LooterSound::PlayAttached(Cue, Weapon.GetRootComponent(), NAME_None, Volume);
			return;
		}
		const TWeakObjectPtr<const AWeaponBase> Gun(&Weapon);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Gun, Cue, Volume]()
		{
			if (const AWeaponBase* Held = Gun.Get())
			{
				LooterSound::PlayAttached(Cue, Held->GetRootComponent(), NAME_None, Volume);
			}
		}), Delay, false);
	}

	// A rocket's burst carries much farther than a shot: past this the far boom (Weapon.Launcher.Explosion.Distant) plays
	// instead of the near one (its debris and crack would be wrong out there).
	TAutoConsoleVariable<float> CVarExplosionDistantFrom(
		TEXT("Looter.Gunfeel.Sound.ExplosionDistantFrom"), 6000.f,
		TEXT("Past this many cm from the listener a rocket's explosion plays its distant version."), ECVF_Default);
}

float GunshotSound::ExplosionDistantFrom()
{
	return FMath::Max(0.f, CVarExplosionDistantFrom.GetValueOnGameThread());
}

bool GunshotSound::IsExplosionDistant(float Distance, float DistantFrom)
{
	return DistantFrom > 0.f && Distance > DistantFrom;
}

void GunshotSound::PlayExplosion(const UObject* WorldContext, const FVector& Location)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	const bool bDistant = IsExplosionDistant(DistanceToListener(Location, *World), ExplosionDistantFrom());
	LooterSound::PlayAt(WorldContext, bDistant ? FName(LooterSoundCue::Launcher::ExplosionDistant)
		: FName(LooterSoundCue::Launcher::Explosion), Location);
}

FName GunshotSound::CueFor(EWeaponKind Kind, EGunshotLayer Layer)
{
	if (Layer == EGunshotLayer::Fire)
	{
		return WeaponSounds::Fire(Kind);
	}
	return FName(*FString::Printf(TEXT("Weapon.%s.%s"), KindWord(Kind), LayerWord(Layer)));
}

void GunshotSound::PlayShot(const AWeaponBase& Weapon)
{
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition;
	UWorld* World = Weapon.GetWorld();
	if (!Definition || !World)
	{
		return;
	}
	// A definition that sets a sound of its own keeps it.
	if (Definition->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(&Weapon, Definition->FireSound, Weapon.GetActorLocation());
		return;
	}
	const EWeaponKind Kind = Definition->Kind;
	USceneComponent* Gun = Weapon.GetRootComponent();
	// Until the bank has the layers (an older bank), the whole shot in one sound, as before.
	const ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(&Weapon);
	if (!Sounds || !Sounds->HasSounds(CoreCue(Kind)))
	{
		LooterSound::PlayAttached(CueFor(Kind, EGunshotLayer::Fire), Gun);
		return;
	}

	const bool bOwnGun = IsOwnGun(Weapon);
	const float Distance = bOwnGun ? 0.f : DistanceToListener(Weapon, *World);
	FShooterMemory& Memory = MemoryOf(Weapon);
	const GunfeelSound::ESurroundings Surroundings = SurroundingsOf(Memory, Weapon, *World);
	GunfeelSound::FShotLayers Layers = GunfeelSound::Choose(bOwnGun, Distance, GunfeelSound::DistantFrom(), Surroundings);

	if (Layers.bDistant)
	{
		LooterSound::PlayAt(&Weapon, CueFor(Kind, EGunshotLayer::Distant), Weapon.GetActorLocation());
		return;
	}
	// Which shot of its burst this is: an automatic skips the tails between its Nth shots (the burst's end plays the long one).
	Layers.bTail = NoteShot(Memory, Kind, World->GetTimeSeconds()) && Layers.bTail;

	// The player's own layers ride on the gun, so they stay with the view as it turns; someone else's tail stays where the
	// shot was, as the land's answer does.
	if (Layers.bCore)
	{
		LooterSound::PlayAttached(CoreCue(Kind), Gun);
	}
	const float Mech = GunfeelSound::MechVolume();
	if (Layers.bMech && Mech > 0.f)
	{
		LooterSound::PlayAttached(CueFor(Kind, EGunshotLayer::Mech), Gun, NAME_None, Mech);
	}
	const float Low = GunfeelSound::LowVolume();
	if (Layers.bLow && Low > 0.f)
	{
		LooterSound::PlayAttached(CueFor(Kind, EGunshotLayer::Low), Gun, NAME_None, Low);
	}
	const float Tail = GunfeelSound::TailVolume();
	if (Layers.bTail && Tail > 0.f)
	{
		PlayTail(Weapon, Kind, Surroundings, bOwnGun, Tail);
	}
	TrimMemories();
}

void GunshotSound::PlayBurstEnd(const AWeaponBase& Weapon)
{
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition;
	UWorld* World = Weapon.GetWorld();
	FShooterMemory* Found = GMemories.Find(TWeakObjectPtr<const AWeaponBase>(&Weapon));
	if (!Definition || !World || !Found)
	{
		return;
	}
	// The burst is over whether or not it sounds anything (a second call, or the release after a swap, finds nothing).
	FShooterMemory& Memory = *Found;
	const int32 BurstShots = Memory.BurstShots;
	const int32 ShotsSinceTail = Memory.ShotsSinceTail;
	const bool bLive = !GunfeelSound::NewBurst(World->GetTimeSeconds(), Memory.LastShotAt, GunfeelSound::BurstGap());
	Memory.BurstShots = 0;
	Memory.ShotsSinceTail = 0;
	if (!bLive)
	{
		return;
	}
	const EWeaponKind Kind = Definition->Kind;
	const bool bOwnGun = IsOwnGun(Weapon);
	if (!bOwnGun && DistanceToListener(Weapon, *World) > GunfeelSound::DistantFrom())
	{
		return;
	}
	const float TailGain = GunfeelSound::BurstEndTailVolume() * GunfeelSound::TailVolume();
	if (GunfeelSound::BurstEndTailPlays(BurstShots, ShotsSinceTail, TailGain))
	{
		PlayTail(Weapon, Kind, SurroundingsOf(Memory, Weapon, *World), bOwnGun, TailGain);
	}
	// A hot LMG barrel ticks and hisses once the firing stops; the hotter, the louder.
	const float Heat = Kind == EWeaponKind::LMG ? Weapon.GetBarrelHeat() : 0.f;
	const float Hiss = GunfeelSound::HissVolume();
	if (bOwnGun && GunfeelSound::HeatHissPlays(Kind, Heat, GunfeelSound::HissFrom(), BurstShots, Hiss))
	{
		LooterSound::PlayAttached(CueFor(Kind, EGunshotLayer::HeatHiss), Weapon.GetRootComponent(), NAME_None,
			Hiss * GunfeelSound::HeatHissLoudness(Heat, GunfeelSound::HissFrom()));
	}
}

void GunshotSound::PlayRack(const AWeaponBase& Weapon)
{
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition;
	UWorld* World = Weapon.GetWorld();
	const float Volume = GunfeelSound::RackVolume();
	// Only the shotgun has a fore-end to rack.
	if (!Definition || !World || Definition->Kind != EWeaponKind::Shotgun || Volume <= 0.f)
	{
		return;
	}
	const bool bOwnGun = IsOwnGun(Weapon);
	if (GunfeelSound::RackAudible(bOwnGun, bOwnGun ? 0.f : DistanceToListener(Weapon, *World), GunfeelSound::DistantFrom()))
	{
		LooterSound::PlayAttached(CueFor(Definition->Kind, EGunshotLayer::Rack), Weapon.GetRootComponent(), NAME_None, Volume);
	}
}

void GunshotSound::PlayLowAmmo(const AWeaponBase& Weapon, int32 RoundsLeft, int32 Magazine)
{
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition;
	// The player's own gun only: nobody else hears their magazine.
	if (!Definition || !IsOwnGun(Weapon))
	{
		return;
	}
	const EWeaponKind Kind = Definition->Kind;
	if (RoundsLeft <= 0)
	{
		// The shot that emptied it: the last-round sound, and the burst ends here (the trigger may be held through the reload).
		if (GunfeelSound::LastRoundPlays(Kind, RoundsLeft, Magazine, GunfeelSound::LastRoundVolume()))
		{
			PlayLastRound(Weapon, Kind);
		}
		PlayBurstEnd(Weapon);
		return;
	}
	const int32 Window = GunfeelSound::LowAmmoWindow(GunfeelSound::LowAmmoRounds(Kind), Magazine);
	const float Pitch = GunfeelSound::LowAmmoPitch(RoundsLeft, Window, GunfeelSound::LowAmmoPitchTop());
	if (Pitch > 0.f)
	{
		LooterSound::PlayAttached(CueFor(Kind, EGunshotLayer::LowAmmo), Weapon.GetRootComponent(), NAME_None, 1.f, Pitch);
	}
}

void GunshotSound::PlayCritRing(const UObject* WorldContext)
{
	LooterSound::Play2D(WorldContext, LooterSoundCue::Revolver::CritRing);
}
