#include "Loot/LootFanfareSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Loot/LootTossComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurses.h"
#include "World/LightBeam.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

bool ULootFanfareSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void ULootFanfareSubsystem::ExpectLanding(AWeaponBase* Weapon)
{
	UWorld* World = Weapon ? Weapon->GetWorld() : nullptr;
	if (ULootFanfareSubsystem* Fanfare = World ? World->GetSubsystem<ULootFanfareSubsystem>() : nullptr)
	{
		Fanfare->Expected.Add({ Weapon, 0.f });
	}
}

FName ULootFanfareSubsystem::DropCue(EWeaponRarity Rarity)
{
	switch (Rarity)
	{
	case EWeaponRarity::Uncommon: return LooterSoundCue::DropUncommon;
	case EWeaponRarity::Rare: return LooterSoundCue::DropRare;
	case EWeaponRarity::Epic: return LooterSoundCue::DropEpic;
	case EWeaponRarity::Legendary: return LooterSoundCue::DropLegendary;
	default: return NAME_None;
	}
}

bool ULootFanfareSubsystem::IsDropCueFlat(EWeaponRarity Rarity)
{
	return Rarity >= EWeaponRarity::Epic;
}

bool ULootFanfareSubsystem::FlaresBeam(EWeaponRarity Rarity)
{
	return Rarity >= EWeaponRarity::Rare;
}

bool ULootFanfareSubsystem::IsAnnounced(EWeaponRarity Rarity)
{
	return Rarity >= EWeaponRarity::Epic;
}

void ULootFanfareSubsystem::Tick(float DeltaTime)
{
	if (Expected.IsEmpty() && Flaring.IsEmpty())
	{
		return;
	}

	// Drops in the air: landed once their toss has stopped (it lets go of the gun then).
	for (int32 Index = Expected.Num() - 1; Index >= 0; --Index)
	{
		FExpected& Each = Expected[Index];
		AWeaponBase* Weapon = Each.Weapon.Get();
		// Gone, or taken in hand before it landed: nothing to celebrate.
		if (!Weapon || !Weapon->IsPickup())
		{
			Expected.RemoveAtSwap(Index);
			continue;
		}
		Each.Waited += DeltaTime;
		const ULootTossComponent* Toss = Weapon->FindComponentByClass<ULootTossComponent>();
		if (!Toss || !Toss->UpdatedComponent || Each.Waited >= LandingTimeout)
		{
			Expected.RemoveAtSwap(Index);
			Land(*Weapon);
		}
	}

	for (int32 Index = Flaring.Num() - 1; Index >= 0; --Index)
	{
		FFlaring& Each = Flaring[Index];
		UStaticMeshComponent* Beam = Each.Beam.Get();
		if (!Beam)
		{
			Flaring.RemoveAtSwap(Index);
			continue;
		}
		Each.Time += DeltaTime;
		// Past its end, once more: the beam left exactly as it was.
		LightBeams::Flare(Beam, Each.Time, Each.bGrand, Each.Glow, Each.Height, Each.Radius);
		if (Each.Time >= LightBeams::FlareSeconds)
		{
			Flaring.RemoveAtSwap(Index);
		}
	}
}

void ULootFanfareSubsystem::Land(AWeaponBase& Weapon)
{
	const UWorld* World = GetWorld();
	const EWeaponRarity Rarity = Weapon.GetRarity();
	const double Now = World ? World->GetTimeSeconds() : 0.0;

	// Heard by rarity; a chest's guns landing together sound once, for the rarest of them.
	const FName Cue = DropCue(Rarity);
	if (!Cue.IsNone() && (Now - LastCueTime >= CueGap || Rarity > LastCueRarity))
	{
		if (IsDropCueFlat(Rarity))
		{
			LooterSound::Play2D(this, Cue);
		}
		else
		{
			LooterSound::PlayAt(this, Cue, Weapon.GetActorLocation());
		}
		LastCueTime = Now;
		LastCueRarity = Rarity;
	}

	// A Rare or better's beam shoots up and flashes. A cursed iron's beam gutters like a dying flame instead (its own
	// tick drives it), and that's its whole show.
	UStaticMeshComponent* Beam = Weapon.GetLootBeam();
	if (FlaresBeam(Rarity) && !WeaponCurses::Of(Weapon.GetInstance()) && Beam && Beam->IsVisible())
	{
		FFlaring& Flare = Flaring.AddDefaulted_GetRef();
		Flare.Beam = Beam;
		Flare.bGrand = Rarity == EWeaponRarity::Legendary;
		// Its steady numbers, read off the beam as the gun set it up (LightBeams::Setup: a 100 x 100 cylinder).
		const FVector Scale = Beam->GetRelativeScale3D();
		Flare.Height = static_cast<float>(Scale.Z) * 100.f;
		Flare.Radius = static_cast<float>(Scale.X) * 50.f;
		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Beam->GetMaterial(0));
		Flare.Glow = Material ? Material->K2_GetScalarParameterValue(TEXT("Glow")) : 2.f;
		LightBeams::Flare(Beam, 0.f, Flare.bGrand, Flare.Glow, Flare.Height, Flare.Radius);
	}

	if (IsAnnounced(Rarity))
	{
		OnAnnounced.Broadcast(&Weapon);
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s landed (%s)"), *Weapon.GetName(), *UEnum::GetValueAsString(Rarity));
}
