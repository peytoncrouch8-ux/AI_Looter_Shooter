#include "Weapons/GunshotSound.h"
#include "Audio/LooterSound.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponSounds.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

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
	if (!Definition)
	{
		return;
	}
	// A definition that sets a sound of its own keeps it.
	if (Definition->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(&Weapon, Definition->FireSound, Weapon.GetActorLocation());
		return;
	}
	// On the gun, so it follows it: the player's own shots stay with the view as it turns.
	LooterSound::PlayAttached(CueFor(Definition->Kind, EGunshotLayer::Fire), Weapon.GetRootComponent());
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
	LooterSound::Play2D(WorldContext, TEXT("Weapon.Revolver.CritRing"));
}
