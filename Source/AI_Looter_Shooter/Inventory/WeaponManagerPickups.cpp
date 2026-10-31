// UWeaponManagerComponent: picking loot up (a tap of the interact key) or equipping it in place of the gun in hand (a
// hold), the loot it offers the player's interaction component, and the loot labels.

#include "Inventory/WeaponManagerComponent.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Weapons/WeaponBase.h"
#include "EnhancedPlayerInput.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// ---------------------------------------------------------------------------
// Pickups
// ---------------------------------------------------------------------------

bool UWeaponManagerComponent::TryPickup(AWeaponBase* Pickup)
{
	if (!IsValid(Pickup) || !Pickup->IsPickup())
	{
		return false;
	}

	// Slots full but backpack has room: stash it like Borderlands does.
	if (Weapons.Num() >= MaxWeapons && HasBackpackRoom())
	{
		BackpackAdd(Pickup->GetInstanceForStorage());
		SendMessage(FString::Printf(TEXT("%s sent to backpack"), *Pickup->GetDisplayName().ToString()));
		ReleaseGun(Pickup);
		OnInventoryChanged.Broadcast();
		return true;
	}

	if (!AddWeapon(Pickup))
	{
		return false;
	}

	// Picking something up means you want to use it.
	EquipSlot(Weapons.IndexOfByKey(Pickup));
	return true;
}

bool UWeaponManagerComponent::EquipPickup(AWeaponBase* Pickup)
{
	if (!IsValid(Pickup) || !Pickup->IsPickup())
	{
		return false;
	}
	// Already coming: the gun in hand is on its way down for it.
	if (SwapPickup.Get() == Pickup && SwapClock.IsLowering())
	{
		return true;
	}

	// A free slot takes it, the same as a tap.
	if (Weapons.Num() < MaxWeapons)
	{
		if (!AddWeapon(Pickup))
		{
			return false;
		}
		EquipSlot(Weapons.IndexOfByKey(Pickup));
		return true;
	}

	// Every slot full: in view, the gun in hand goes down first and they change places out of sight (FinishHolster). With
	// the backpack full too, the old gun is thrown down at once instead: its toss shows it going.
	if (HasBackpackRoom() && ShouldAnimateSwap())
	{
		BeginSwap(nullptr, Pickup);
		return true;
	}
	PutLootInHand(Pickup);
	return true;
}

void UWeaponManagerComponent::PutLootInHand(AWeaponBase* Pickup)
{
	if (Weapons.Num() < MaxWeapons)
	{
		// (A slot freed while the gun in hand went down for it.)
		if (AddWeapon(Pickup) && Weapons.IndexOfByKey(Pickup) != ActiveSlot)
		{
			SetActiveSlot(Weapons.IndexOfByKey(Pickup));
		}
		return;
	}

	// Every slot is full: it takes the slot in use, and the weapon that was in it goes to the backpack, or onto the
	// ground when the backpack is full too.
	const int32 Slot = FMath::Max(ActiveSlot, 0);
	if (AWeaponBase* Outgoing = Weapons[Slot])
	{
		const FString OutgoingName = Outgoing->GetDisplayName().ToString();
		if (HasBackpackRoom())
		{
			Outgoing->OnHolstered();
			BackpackAdd(Outgoing->GetInstanceForStorage());
			ReleaseGun(Outgoing);
			SendMessage(FString::Printf(TEXT("%s sent to backpack"), *OutgoingName));
		}
		else
		{
			Outgoing->OnDropped();
			TossWeaponAway(Outgoing);
			SendMessage(FString::Printf(TEXT("Backpack full: %s dropped"), *OutgoingName));
		}
	}
	Weapons[Slot] = Pickup;
	AttachHolstered(Pickup);
	ActiveSlot = INDEX_NONE; // the weapon that was in hand is gone; SetActiveSlot takes out the new one
	SetActiveSlot(Slot);
	OnInventoryChanged.Broadcast();
}

// ---------------------------------------------------------------------------
// Loot as things to use (IInteractionSource)
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::GatherInteractions(const FInteractionView& View, TArray<FInteractionCandidate>& OutCandidates) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double RangeSquared = FMath::Square(static_cast<double>(PickupRange));
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		AWeaponBase* Weapon = *It;
		// A hidden gun (one a scene hasn't shown yet) is never offered: Delia's Heirloom was once taken unseen this way.
		if (!Weapon->IsPickup() || Weapon->IsHidden()
			|| FVector::DistSquared(Weapon->GetActorLocation(), View.ReachOrigin) > RangeSquared)
		{
			continue;
		}
		FInteractionCandidate& Candidate = OutCandidates.AddDefaulted_GetRef();
		Candidate.Actor = Weapon;
		Candidate.Location = Weapon->GetActorLocation();
		Candidate.Options = GetOfferOptions(*Weapon);
		Candidate.Reach = PickupRange;
		// Loot never needed a clear line from the eyes (it lies in the grass, on tables), and still doesn't.
		Candidate.bCheckSight = false;
	}
}

FInteractionOptions UWeaponManagerComponent::GetOfferOptions(const AActor& Offer) const
{
	// A tap picks it up and a hold of PickupHoldSeconds takes it in hand; they differ only when every slot is full, and
	// the HUD's loot card spells out what each does then.
	const AWeaponBase* Weapon = Cast<AWeaponBase>(&Offer);
	FInteractionOptions Options;
	Options.bUsable = Weapon && Weapon->IsPickup();
	Options.bTap = true;
	Options.bHold = true;
	Options.HoldSeconds = PickupHoldSeconds;
	Options.TapPrompt = FText::FromString(TEXT("Pick up"));
	Options.HoldPrompt = FText::FromString(TEXT("Equip"));
	Options.Reach = PickupRange;
	return Options;
}

bool UWeaponManagerComponent::UseOffer(AActor& Offer, bool bHeld)
{
	AWeaponBase* Weapon = Cast<AWeaponBase>(&Offer);
	if (!Weapon)
	{
		return false;
	}
	// Where it lay, before it goes to a slot, the backpack or away.
	const FVector Where = Weapon->GetActorLocation();
	const bool bTaken = bHeld ? EquipPickup(Weapon) : TryPickup(Weapon);
	// Taken: the grab (the gun coming up has its own sound); refused (every slot and the backpack full): a refusal.
	if (bTaken)
	{
		LooterSound::PlayAt(this, LooterSoundCue::GunPickup, Where);
	}
	else
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
	}
	return bTaken;
}

// ---------------------------------------------------------------------------
// Loot labels
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::HandleInteractionFocusChanged(AActor* Focused)
{
	UpdateLootLabels();
}

void UWeaponManagerComponent::UpdateLootLabels()
{
	APlayerController* PC = InputBinding.GetController();
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return;
	}

	// Safety net for a missed trigger release (focus changes, alt-tab): if fire isn't actually held, stop.
	if (const UEnhancedPlayerInput* PlayerInput = Cast<UEnhancedPlayerInput>(PC->PlayerInput))
	{
		if (FireAction && !PlayerInput->GetActionValue(FireAction).Get<bool>())
		{
			StopFire();
		}
	}

	// Labels show over loot within LabelRange of the eyes; the loot the interaction component focuses (looked at, in
	// reach) shows its stats and the prompt.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const FVector Eyes = Pawn ? Pawn->GetPawnViewLocation() : PC->GetFocalLocation();
	const UInteractionComponent* Interaction = LabelFocus.Get();
	const AActor* Focused = Interaction ? Interaction->GetFocusedActor() : nullptr;
	TArray<TWeakObjectPtr<AWeaponBase>> NowLabeled;
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		AWeaponBase* Weapon = *It;
		if (Weapon->IsPickup() && FVector::Dist(Weapon->GetActorLocation(), Eyes) <= LabelRange)
		{
			NowLabeled.Add(Weapon);
		}
	}

	// Hide labels for loot that went out of range, then refresh the ones in range.
	for (const TWeakObjectPtr<AWeaponBase>& Old : LabeledPickups)
	{
		if (Old.IsValid() && !NowLabeled.Contains(Old))
		{
			Old->SetLabelState(false, false);
		}
	}
	for (const TWeakObjectPtr<AWeaponBase>& Weapon : NowLabeled)
	{
		Weapon->SetLabelState(true, Weapon.Get() == Focused);
	}
	LabeledPickups = MoveTemp(NowLabeled);
}
