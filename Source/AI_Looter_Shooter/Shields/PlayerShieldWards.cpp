// UPlayerShieldComponent: the wards it carries (one worn, MaxSpares spare), swapped, stowed and dropped, and into a
// session's save and back.

#include "Shields/PlayerShieldComponent.h"
#include "AI_Looter_Shooter.h"
#include "Shields/ShieldPickup.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldSounds.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A dropped ward lands this far in front of the player, thrown a little up and out. */
	constexpr float DropAhead = 110.f;
	constexpr float DropToss = 260.f;
	constexpr float DropLift = 320.f;
}

bool UPlayerShieldComponent::Equip(const FShieldInstanceData& NewWard, bool bFull)
{
	if (!NewWard.IsSet())
	{
		return false;
	}
	if (HasWard())
	{
		if (HasSpareRoom())
		{
			Spares.Add(Ward);
		}
		else
		{
			// No room among the spares: the old one goes on the ground, never lost unseen.
			AShieldPickup::DropWard(GetWorld(), Ward, DropSpot(), GetOwner() ? GetOwner()->GetActorForwardVector() * DropToss + FVector(0.f, 0.f, DropLift) : FVector::ZeroVector);
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
	if (!HasSpareRoom())
	{
		return false;
	}
	FShieldInstanceData Spare = NewWard;
	Spare.Stats = ShieldRules::ComputeStats(Spare);
	Spares.Add(Spare);
	PlayCue(ShieldCue::Pickup);
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
	if (HasWard())
	{
		// The worn one takes the spare's place, so the list keeps its order.
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
	NotifyWardsChanged();
	return true;
}

bool UPlayerShieldComponent::StowWard()
{
	if (!HasWard() || !HasSpareRoom())
	{
		return false;
	}
	Spares.Add(Ward);
	Ward = FShieldInstanceData();
	ResetWardState(false);
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
	for (const FShieldInstanceData& Spare : Save.Spares)
	{
		if (Spare.IsSet() && HasSpareRoom())
		{
			FShieldInstanceData Kept = Spare;
			Kept.Stats = ShieldRules::ComputeStats(Kept);
			Spares.Add(Kept);
		}
	}
	bStarterGiven = Save.bStarterGiven;
	// It comes back full, as health does.
	ResetWardState(true);
	NotifyWardsChanged();
	// A save from before the wards, loaded with guns carried: the first gun's starting ward now.
	HandleInventoryChanged();
}

void UPlayerShieldComponent::EmptyWards()
{
	Ward = FShieldInstanceData();
	Spares.Reset();
	ResetWardState(false);
	NotifyWardsChanged();
}
