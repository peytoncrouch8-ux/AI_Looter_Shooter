#include "Loot/LootBeamCrowd.h"
#include "Weapons/WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

TArray<bool> LootBeamCrowd::ChooseShown(TConstArrayView<FVector> Small, TConstArrayView<FVector> Guns, float Merge, float NearGun)
{
	TArray<bool> Shown;
	Shown.Init(false, Small.Num());
	const double MergeSquared = FMath::Square(static_cast<double>(Merge));
	const double NearGunSquared = FMath::Square(static_cast<double>(NearGun));
	TArray<FVector, TInlineAllocator<32>> Kept;
	for (int32 Index = 0; Index < Small.Num(); ++Index)
	{
		const FVector& At = Small[Index];
		const bool bByGun = Guns.ContainsByPredicate([&At, NearGunSquared](const FVector& Gun) { return FVector::DistSquared(Gun, At) <= NearGunSquared; });
		const bool bByShown = Kept.ContainsByPredicate([&At, MergeSquared](const FVector& Other) { return FVector::DistSquared(Other, At) <= MergeSquared; });
		if (!bByGun && !bByShown)
		{
			Shown[Index] = true;
			Kept.Add(At);
		}
	}
	return Shown;
}

void ULootBeamCrowdSubsystem::Join(AActor* Owner, UStaticMeshComponent* Beam)
{
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	ULootBeamCrowdSubsystem* Crowd = World ? World->GetSubsystem<ULootBeamCrowdSubsystem>() : nullptr;
	if (Crowd && Beam)
	{
		FEntry& Entry = Crowd->Entries.AddDefaulted_GetRef();
		Entry.Owner = Owner;
		Entry.Beam = Beam;
		// Sorted at the next look, a quarter second at most: a new drop's beam may stand that long beside another's.
	}
}

void ULootBeamCrowdSubsystem::Refresh()
{
	Entries.RemoveAll([](const FEntry& Entry)
	{
		const AActor* Owner = Entry.Owner.Get();
		return !Owner || Owner->IsActorBeingDestroyed() || !Entry.Beam.IsValid();
	});
	// Only beams the drop shows itself are sorted (a silver pile flying to the player has put its own out).
	TArray<int32> Sorted;
	TArray<FVector> Small;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].Beam->GetVisibleFlag())
		{
			Sorted.Add(Index);
			Small.Add(Entries[Index].Owner->GetActorLocation());
		}
	}
	if (Small.IsEmpty())
	{
		return;
	}
	TArray<FVector> Guns;
	for (TActorIterator<AWeaponBase> It(GetWorld()); It; ++It)
	{
		if (It->IsPickup() && !It->IsActorBeingDestroyed())
		{
			Guns.Add(It->GetActorLocation());
		}
	}
	const TArray<bool> Shown = LootBeamCrowd::ChooseShown(Small, Guns);
	for (int32 Pick = 0; Pick < Sorted.Num(); ++Pick)
	{
		FEntry& Entry = Entries[Sorted[Pick]];
		if (Entry.bCrowdHidden == Shown[Pick])
		{
			Entry.bCrowdHidden = !Shown[Pick];
			Entry.Beam->SetHiddenInGame(Entry.bCrowdHidden);
		}
	}
}

bool ULootBeamCrowdSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void ULootBeamCrowdSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Clock += DeltaTime;
	if (Clock >= LootBeamCrowd::LookSeconds)
	{
		Clock = 0.f;
		Refresh();
	}
}

bool ULootBeamCrowdSubsystem::IsTickable() const
{
	// Nothing to sort while no small drop is about.
	return !Entries.IsEmpty();
}

TStatId ULootBeamCrowdSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULootBeamCrowdSubsystem, STATGROUP_Tickables);
}
