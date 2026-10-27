#include "Loot/LootFanfareSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Loot/LootDropMoments.h"
#include "Loot/LootTossComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurses.h"
#include "World/LightBeam.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

// ULootFanfareSubsystem: a drop's sounds, its beam's flare and its announcement. The landing shows (spin, ring, column,
// name pop) and their pools are in LootFanfareShows.cpp, a Legendary's burst in LootFanfareBurst.cpp.

bool ULootFanfareSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void ULootFanfareSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The column and the name pop are built now, as the level loads, not on the frame a Legendary lands.
	WarmPools();
}

void ULootFanfareSubsystem::Deinitialize()
{
	ClearShows();
	Super::Deinitialize();
}

void ULootFanfareSubsystem::ExpectLanding(AWeaponBase* Weapon)
{
	UWorld* World = Weapon ? Weapon->GetWorld() : nullptr;
	if (ULootFanfareSubsystem* Fanfare = World ? World->GetSubsystem<ULootFanfareSubsystem>() : nullptr)
	{
		FExpected& Each = Fanfare->Expected.AddDefaulted_GetRef();
		Each.Weapon = Weapon;
		bool bStung = false;
		Fanfare->Dropped(*Weapon, bStung);
		// (Dropped adds nothing to Expected, so Each still stands.)
		Fanfare->Expected.Last().bStung = bStung;
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
	Advance(DeltaTime);
}

void ULootFanfareSubsystem::Advance(float DeltaTime)
{
	if (Expected.IsEmpty() && Flaring.IsEmpty() && Spins.IsEmpty() && Rings.IsEmpty() && Columns.IsEmpty() && NamePops.IsEmpty()
		&& ArmSparks.IsEmpty() && FountainSparks.IsEmpty())
	{
		return;
	}

	// The shows under way first, so one a landing starts below begins at its start this frame.
	AdvanceShows(DeltaTime);

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
			const bool bStung = Each.bStung;
			Expected.RemoveAtSwap(Index);
			Land(*Weapon, bStung);
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

void ULootFanfareSubsystem::Dropped(AWeaponBase& Weapon, bool& bOutStung)
{
	bOutStung = false;
	const FDropMoment Moment = LootDropMoments::For(Weapon.GetRarity());
	if (!Moment.bStingerAtDrop)
	{
		return;
	}
	// A Legendary is felt before it's seen: its sting now, wherever the player looks. The world keeps its speed: the moment
	// is the burst as it lands (the slow it once had read to the user as the game stuttering).
	if (!Moment.Stinger.IsNone())
	{
		LooterSound::Play2D(this, Moment.Stinger);
		bOutStung = true;
		LastCueTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		LastCueRarity = Weapon.GetRarity();
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s dropped (Legendary): sting"), *Weapon.GetName());
}

void ULootFanfareSubsystem::Land(AWeaponBase& Weapon, bool bStung)
{
	const UWorld* World = GetWorld();
	const EWeaponRarity Rarity = Weapon.GetRarity();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	const FDropMoment Moment = LootDropMoments::For(Rarity);

	// Heard by rarity; a chest's guns landing together sound once, for the rarest of them. A Legendary stung as it dropped
	// lands with its impact.
	const FName Cue = bStung ? Moment.LandCue : DropCue(Rarity);
	if (!Cue.IsNone() && (bStung || Now - LastCueTime >= CueGap || Rarity > LastCueRarity))
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
	const bool bCursed = WeaponCurses::Of(Weapon.GetInstance()) != nullptr;
	if (FlaresBeam(Rarity) && !bCursed && Beam && Beam->IsVisible())
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

	// Its rarity's show: the spin and ring (Epic and up), a Legendary's column, name and motes. A cursed one keeps to its
	// own guttering.
	if (!bCursed)
	{
		StartShows(Weapon);
	}

	if (IsAnnounced(Rarity))
	{
		OnAnnounced.Broadcast(&Weapon);
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s landed (%s)"), *Weapon.GetName(), *UEnum::GetValueAsString(Rarity));
}
