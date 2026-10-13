// ULostLootSubsystem: the guns lying on the ground: which are loose (never a rack's), their five minutes, handing them in
// one at a time as their time runs out or all at once as the player leaves, and the clock's beat that drives it.

#include "Loot/LostLootSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Loot/LostLootRules.h"
#include "Loot/WeaponRack.h"
#include "Session/SessionSubsystem.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	TArray<AWeaponRack*> FindRacks(UWorld* World)
	{
		TArray<AWeaponRack*> Racks;
		for (TActorIterator<AWeaponRack> It(World); It; ++It)
		{
			Racks.Add(*It);
		}
		return Racks;
	}
}

bool ULostLootSubsystem::IsGroundWeapon(const AWeaponBase& Weapon, const TArray<AWeaponRack*>& Racks)
{
	// Loot with a kind, not already going; a rack's gun is the rack's to offer (it restocks it), never lost property.
	if (!Weapon.IsPickup() || Weapon.IsActorBeingDestroyed() || !Weapon.GetInstance().Definition || Weapon.GetOwner())
	{
		return false;
	}
	return !Racks.ContainsByPredicate([&Weapon](const AWeaponRack* Rack) { return Rack && Rack->Offers(&Weapon); });
}

TArray<AWeaponBase*> ULostLootSubsystem::FindGroundWeapons() const
{
	TArray<AWeaponBase*> Found;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Found;
	}
	const TArray<AWeaponRack*> Racks = FindRacks(World);
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		if (IsGroundWeapon(**It, Racks))
		{
			Found.Add(*It);
		}
	}
	return Found;
}

float ULostLootSubsystem::GetLifeLeft(const AWeaponBase& Weapon) const
{
	const float* Left = GroundLife.Find(TWeakObjectPtr<AWeaponBase>(const_cast<AWeaponBase*>(&Weapon)));
	return Left ? *Left : LostLootRules::GroundLifeSeconds;
}

void ULostLootSubsystem::SetLifeLeft(AWeaponBase& Weapon, float Seconds)
{
	GroundLife.Add(&Weapon, Seconds);
}

// ---------------------------------------------------------------------------
// The clock
// ---------------------------------------------------------------------------

void ULostLootSubsystem::HandleClock()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// The level's own time: paused play (the pause menu) doesn't count, slow motion counts slowly.
	const double Now = World->GetTimeSeconds();
	const float Delta = static_cast<float>(FMath::Max(Now - LastClock, 0.0));
	LastClock = Now;
	Advance(Delta);
	TryAnnounce();
}

TOptional<FVector> ULostLootSubsystem::FindPlayerPlace() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return Pawn ? TOptional<FVector>(Pawn->GetActorLocation()) : TOptional<FVector>();
}

void ULostLootSubsystem::Advance(float DeltaSeconds)
{
	AdvanceAt(DeltaSeconds, FindPlayerPlace());
}

void ULostLootSubsystem::AdvanceAt(float DeltaSeconds, const TOptional<FVector>& PlayerAt)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Every gun lying loose now; the ones the clock knew that aren't any more (picked up, gone) are forgotten, so a gun
	// dropped again gets a whole five minutes.
	const TArray<AWeaponRack*> Racks = FindRacks(World);
	TMap<TWeakObjectPtr<AWeaponBase>, float> Seen;
	TArray<AWeaponBase*> Due;
	const double NearSquared = FMath::Square(static_cast<double>(LostLootRules::KeepNearRadius));
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		AWeaponBase* Weapon = *It;
		if (!IsGroundWeapon(*Weapon, Racks))
		{
			continue;
		}
		const float* Known = GroundLife.Find(Weapon);
		float Left = Known ? *Known : LostLootRules::GroundLifeSeconds;
		// A gun nobody can see yet (a scene keeps it hidden until it's shown) isn't lying anywhere the player could have
		// left it: it doesn't age.
		if (!Weapon->IsHidden())
		{
			Left -= FMath::Max(DeltaSeconds, 0.f);
		}
		Seen.Add(Weapon, Left);
		const bool bNear = PlayerAt.IsSet() && FVector::DistSquared(PlayerAt.GetValue(), Weapon->GetActorLocation()) < NearSquared;
		if (LostLootRules::IsDue(Left, bNear))
		{
			Due.Add(Weapon);
		}
	}
	GroundLife = MoveTemp(Seen);

	for (AWeaponBase* Weapon : Due)
	{
		const FWeaponInstanceData Gun = Weapon->GetInstanceForStorage();
		if (Send(*Weapon, ELostLootReason::LeftLying) && !Store.bExplained && !bExplainWaiting)
		{
			// The first gun ever to go: the HUD says where it went, once. After that they go quietly (the user: not every
			// despawn).
			bExplainWaiting = true;
			ExplainGun = Gun;
		}
	}
}

// ---------------------------------------------------------------------------
// Handing in
// ---------------------------------------------------------------------------

bool ULostLootSubsystem::Send(AWeaponBase& Weapon, ELostLootReason Reason)
{
	UWorld* World = GetWorld();
	if (!World || !IsGroundWeapon(Weapon, FindRacks(World)))
	{
		return false;
	}
	TArray<FLostLootArrival> One;
	One.Add({ Weapon.GetInstanceForStorage(), GetLifeLeft(Weapon) });
	// Off the ground as it goes on the shelves: never both, never neither.
	GroundLife.Remove(&Weapon);
	Weapon.Destroy();
	Receive(MoveTemp(One), Reason, USessionSubsystem::MapOf(World), /*bAnnounce*/ false);
	return true;
}

int32 ULostLootSubsystem::SendAllGroundWeapons(ELostLootReason Reason, bool bAnnounce)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}
	TArray<FLostLootArrival> Arrivals;
	for (AWeaponBase* Weapon : FindGroundWeapons())
	{
		Arrivals.Add({ Weapon->GetInstanceForStorage(), GetLifeLeft(*Weapon) });
		GroundLife.Remove(Weapon);
		Weapon->Destroy();
	}
	if (Arrivals.IsEmpty())
	{
		return 0;
	}
	return Receive(MoveTemp(Arrivals), Reason, USessionSubsystem::MapOf(World), bAnnounce);
}
