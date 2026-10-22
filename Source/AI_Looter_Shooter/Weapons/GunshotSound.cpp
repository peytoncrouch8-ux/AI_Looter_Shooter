#include "Weapons/GunshotSound.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/GunfeelSoundTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponSounds.h"
#include "World/WorldQueries.h"

// A shot as layers (Gunfeel step 2, GUNFEEL-SOUND): the shooter's own gun plays the punch (Core), its action (Mech), the
// chest thump (Low) and the land's answer by its surroundings (a tail); anyone else's the punch and the tail, or the far
// shot past Looter.Gunfeel.Sound.DistantFrom. The surroundings come from a few short traces round the shooter, looked at
// again at most every quarter second per gun. The rules are GunfeelSoundTuning's.

namespace
{
	const TCHAR* KindWord(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return TEXT("Shotgun");
		case EWeaponKind::Revolver: return TEXT("Revolver");
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
		default: return TEXT("Fire");
		}
	}

	FName CoreCue(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return LooterSoundCue::Gunshot::ShotgunCore;
		case EWeaponKind::Revolver: return LooterSoundCue::Gunshot::RevolverCore;
		default: return LooterSoundCue::Gunshot::RifleCore;
		}
	}

	/** A gun's last look at its surroundings: a burst fires many shots from the same spot. */
	struct FProbeMemory
	{
		double At = -1.0;
		GunfeelSound::ESurroundings Surroundings = GunfeelSound::ESurroundings::Open;
	};

	TMap<TWeakObjectPtr<const AWeaponBase>, FProbeMemory> GProbes;

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

	GunfeelSound::ESurroundings SurroundingsOf(const AWeaponBase& Weapon, UWorld& World)
	{
		const double Now = World.GetTimeSeconds();
		FProbeMemory& Memory = GProbes.FindOrAdd(TWeakObjectPtr<const AWeaponBase>(&Weapon));
		if (GunfeelSound::ProbeStale(Now, Memory.At))
		{
			Memory.Surroundings = Probe(Weapon, World);
			Memory.At = Now;
		}
		const GunfeelSound::ESurroundings Surroundings = Memory.Surroundings;
		// Guns come and go (drops, swaps, levels): the ones gone are let go of now and then (Memory isn't used past here:
		// removing from the map moves its entries).
		if (GProbes.Num() > 32)
		{
			for (auto It = GProbes.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid())
				{
					It.RemoveCurrent();
				}
			}
		}
		return Surroundings;
	}

	/** How far the shot is from whoever's listening (the first local player's ears); 0 with no one to ask. */
	float DistanceToListener(const AWeaponBase& Weapon, UWorld& World)
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
		return static_cast<float>(FVector::Dist(Ears, Weapon.GetActorLocation()));
	}
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

	const APawn* Holder = Cast<APawn>(Weapon.GetOwner());
	const bool bOwnGun = Holder && Holder->IsLocallyControlled() && Holder->IsPlayerControlled();
	const float Distance = bOwnGun ? 0.f : DistanceToListener(Weapon, *World);
	const GunfeelSound::FShotLayers Layers =
		GunfeelSound::Choose(bOwnGun, Distance, GunfeelSound::DistantFrom(), SurroundingsOf(Weapon, *World));

	if (Layers.bDistant)
	{
		LooterSound::PlayAt(&Weapon, CueFor(Kind, EGunshotLayer::Distant), Weapon.GetActorLocation());
		return;
	}
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
		if (bOwnGun)
		{
			LooterSound::PlayAttached(CueFor(Kind, Layers.Tail), Gun, NAME_None, Tail);
		}
		else
		{
			LooterSound::PlayAt(&Weapon, CueFor(Kind, Layers.Tail), Weapon.GetActorLocation(), Tail);
		}
	}
}

void GunshotSound::PlayLowAmmo(const AWeaponBase& Weapon, int32 RoundsLeft, int32 Magazine)
{
	// The share and the pitch are GunfeelKick::LowAmmoPitch's rule (Looter.Gunfeel.LowAmmo.*); GUNFEEL-SOUND makes the cue.
	const float Pitch = GunfeelKick::LowAmmoPitch(RoundsLeft, Magazine);
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition;
	const APawn* Holder = Cast<APawn>(Weapon.GetOwner());
	// The player's own gun only: nobody else hears their magazine.
	if (Pitch <= 0.f || !Definition || !Holder || !Holder->IsLocallyControlled())
	{
		return;
	}
	LooterSound::PlayAttached(CueFor(Definition->Kind, EGunshotLayer::LowAmmo), Weapon.GetRootComponent(), NAME_None, 1.f, Pitch);
}

void GunshotSound::PlayCritRing(const UObject* WorldContext)
{
	LooterSound::Play2D(WorldContext, LooterSoundCue::Revolver::CritRing);
}
