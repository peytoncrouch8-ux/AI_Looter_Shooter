// UPlayerShieldComponent: the wards it carries (one worn, the spares in the backpack, sharing its places with the guns: the
// user, 2026-10-31), swapped, stowed and dropped, and into a session's save and back. The weapon manager keeps the
// backpack's room and order; this tells it whenever the spares change (NoteSparesChanged).

#include "Shields/PlayerShieldComponent.h"
#include "AI_Looter_Shooter.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Shields/ShieldPickup.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldSounds.h"
#include "Shields/ShieldText.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** A dropped ward lands this far in front of the player, thrown a little up and out. */
	constexpr float DropAhead = 110.f;
	constexpr float DropToss = 260.f;
	constexpr float DropLift = 320.f;
}

UWeaponManagerComponent* UPlayerShieldComponent::FindManager() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
}

bool UPlayerShieldComponent::HasBackpackRoom() const
{
	if (const UWeaponManagerComponent* Manager = FindManager())
	{
		return Manager->HasBackpackRoom();
	}
	return Spares.Num() < NoManagerRoom;
}

void UPlayerShieldComponent::NoteSparesChanged(int32 RemovedSpare)
{
	if (UWeaponManagerComponent* Manager = FindManager())
	{
		Manager->NoteWardsChanged(RemovedSpare);
	}
}

void UPlayerShieldComponent::SayToHud(const FString& Message) const
{
	if (const UWeaponManagerComponent* Manager = FindManager())
	{
		Manager->OnMessage.Broadcast(FText::FromString(Message));
	}
}

bool UPlayerShieldComponent::Equip(const FShieldInstanceData& NewWard, bool bFull)
{
	if (!NewWard.IsSet())
	{
		return false;
	}
	bool bStowedOld = false;
	if (HasWard())
	{
		const FString OldName = ShieldText::Name(Ward);
		if (HasBackpackRoom())
		{
			Spares.Add(Ward);
			bStowedOld = true;
			SayToHud(FString::Printf(TEXT("%s sent to backpack"), *OldName));
		}
		else
		{
			// No room in the backpack: the old one goes on the ground, never lost unseen (a gun's swap does the same).
			AShieldPickup::DropWard(GetWorld(), Ward, DropSpot(), GetOwner() ? GetOwner()->GetActorForwardVector() * DropToss + FVector(0.f, 0.f, DropLift) : FVector::ZeroVector);
			SayToHud(FString::Printf(TEXT("Backpack full: %s dropped"), *OldName));
		}
	}
	Ward = NewWard;
	Ward.Stats = ShieldRules::ComputeStats(Ward);
	// A ward put on mid-fight comes on empty and waits its delay: swapping isn't a free refill.
	ResetWardState(bFull);
	if (!bFull)
	{
		Charge.Hurt();
	}
	PlayCue(ShieldCue::Pickup);
	if (bStowedOld)
	{
		NoteSparesChanged();
	}
	NotifyWardsChanged();
	return true;
}

bool UPlayerShieldComponent::Take(const FShieldInstanceData& NewWard)
{
	if (!NewWard.IsSet())
	{
		return false;
	}
	if (!HasWard())
	{
		return Equip(NewWard, false);
	}
	if (!HasBackpackRoom())
	{
		return false;
	}
	FShieldInstanceData Spare = NewWard;
	Spare.Stats = ShieldRules::ComputeStats(Spare);
	Spares.Add(Spare);
	PlayCue(ShieldCue::Pickup);
	SayToHud(FString::Printf(TEXT("%s sent to backpack"), *ShieldText::Name(Spare)));
	NoteSparesChanged();
	NotifyWardsChanged();
	return true;
}

bool UPlayerShieldComponent::EquipSpare(int32 Index)
{
	if (!Spares.IsValidIndex(Index))
	{
		return false;
	}
	const FShieldInstanceData Chosen = Spares[Index];
	const bool bSwap = HasWard();
	if (bSwap)
	{
		// The worn one takes the spare's place, so the backpack keeps its order.
		Spares[Index] = Ward;
	}
	else
	{
		Spares.RemoveAt(Index);
	}
	Ward = Chosen;
	Ward.Stats = ShieldRules::ComputeStats(Ward);
	ResetWardState(false);
	Charge.Hurt();
	PlayCue(ShieldCue::Pickup);
	if (!bSwap)
	{
		NoteSparesChanged(Index);
	}
	NotifyWardsChanged();
	return true;
}

bool UPlayerShieldComponent::StowWard()
{
	if (!HasWard() || !HasBackpackRoom())
	{
		return false;
	}
	Spares.Add(Ward);
	Ward = FShieldInstanceData();
	ResetWardState(false);
	NoteSparesChanged();
	NotifyWardsChanged();
	return true;
}

bool UPlayerShieldComponent::DropWard()
{
	if (!HasWard())
	{
		return false;
	}
	const FVector Toss = GetOwner() ? GetOwner()->GetActorForwardVector() * DropToss + FVector(0.f, 0.f, DropLift) : FVector::ZeroVector;
	AShieldPickup::DropWard(GetWorld(), Ward, DropSpot(), Toss);
	Ward = FShieldInstanceData();
	ResetWardState(false);
	NotifyWardsChanged();
	return true;
}

bool UPlayerShieldComponent::DropSpare(int32 Index)
{
	if (!Spares.IsValidIndex(Index))
	{
		return false;
	}
	const FVector Toss = GetOwner() ? GetOwner()->GetActorForwardVector() * DropToss + FVector(0.f, 0.f, DropLift) : FVector::ZeroVector;
	AShieldPickup::DropWard(GetWorld(), Spares[Index], DropSpot(), Toss);
	Spares.RemoveAt(Index);
	NoteSparesChanged(Index);
	NotifyWardsChanged();
	return true;
}

FVector UPlayerShieldComponent::DropSpot() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}
	// In front, at the waist: it pops out and lands at the player's feet ahead.
	return Owner->GetActorLocation() + Owner->GetActorForwardVector() * DropAhead;
}

void UPlayerShieldComponent::SaveWards(FShieldInventorySave& OutSave) const
{
	OutSave.Equipped = Ward;
	OutSave.Spares = Spares;
	OutSave.bStarterGiven = bStarterGiven;
}

void UPlayerShieldComponent::RestoreWards(const FShieldInventorySave& Save)
{
	EmptyWards();
	// A ward whose kind is gone (its data asset taken out) can't come back; numbers are rebuilt from the rest, so balance
	// changes reach every ward already found.
	if (Save.Equipped.IsSet())
	{
		Ward = Save.Equipped;
		Ward.Stats = ShieldRules::ComputeStats(Ward);
	}
	// Every spare comes back, even past the backpack's room: an old save's three spares had their own room, and the user's
	// rule (2026-10-31) is that nothing is thrown away. The weapon manager's load places them and logs an overflow.
	for (const FShieldInstanceData& Spare : Save.Spares)
	{
		if (Spare.IsSet())
		{
			FShieldInstanceData Kept = Spare;
			Kept.Stats = ShieldRules::ComputeStats(Kept);
			Spares.Add(Kept);
		}
	}
	bStarterGiven = Save.bStarterGiven;
	// It comes back full, as health does.
	ResetWardState(true);
	NoteSparesChanged();
	NotifyWardsChanged();
	// A save from before the wards, loaded with guns carried: the first gun's starting ward now.
	HandleInventoryChanged();
}

void UPlayerShieldComponent::EmptyWards()
{
	const bool bHadSpares = !Spares.IsEmpty();
	Ward = FShieldInstanceData();
	Spares.Reset();
	ResetWardState(false);
	if (bHadSpares)
	{
		NoteSparesChanged();
	}
	NotifyWardsChanged();
}
