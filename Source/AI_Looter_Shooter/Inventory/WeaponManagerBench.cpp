// UWeaponManagerComponent: the gunsmith's bench's side of it: the parts box, scrapping guns for a part, and fitting parts
// from the box onto guns of their kind (WeaponPartSwap has the rules).

#include "Inventory/WeaponManagerComponent.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponPartSwap.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "LooterBench"

namespace
{
	/** A part as the log names it: "DA_AssaultRifle Barrel=Marksman". */
	FString DescribePart(const FBoxedWeaponPart& Part)
	{
		return FString::Printf(TEXT("%s %s=%s"), Part.Definition ? *Part.Definition->GetName() : TEXT("(none)"), *Part.Slot.ToString(), *Part.Key.ToString());
	}

	/** Scrapping and fitting are the player's own work: the session keeps it soon, as it does other progress. */
	void SaveSoon(const UObject* WorldContext)
	{
		if (USessionSubsystem* Sessions = USessionSubsystem::Get(WorldContext))
		{
			Sessions->SaveSoon();
		}
	}
}

// ---------------------------------------------------------------------------
// The guns carried
// ---------------------------------------------------------------------------

const FWeaponInstanceData* UWeaponManagerComponent::FindCarriedGun(const FCarriedGun& Gun) const
{
	if (Gun.bBackpack)
	{
		return Backpack.IsValidIndex(Gun.Index) && Backpack[Gun.Index].Definition ? &Backpack[Gun.Index] : nullptr;
	}
	return Weapons.IsValidIndex(Gun.Index) && IsValid(Weapons[Gun.Index]) ? &Weapons[Gun.Index]->GetInstance() : nullptr;
}

int32 UWeaponManagerComponent::NumEquipped() const
{
	int32 Count = 0;
	for (const AWeaponBase* Weapon : Weapons)
	{
		Count += IsValid(Weapon) ? 1 : 0;
	}
	return Count;
}

int32 UWeaponManagerComponent::NumCarriedGuns() const
{
	int32 Count = NumEquipped();
	for (const FWeaponInstanceData& Instance : Backpack)
	{
		Count += Instance.Definition ? 1 : 0;
	}
	return Count;
}

// ---------------------------------------------------------------------------
// Scrapping
// ---------------------------------------------------------------------------

bool UWeaponManagerComponent::CanScrap(const FCarriedGun& Gun, FText* OutWhy) const
{
	auto Refuse = [OutWhy](const FText& Why) -> bool
	{
		if (OutWhy)
		{
			*OutWhy = Why;
		}
		return false;
	};
	const FWeaponInstanceData* Instance = FindCarriedGun(Gun);
	if (!Instance)
	{
		return Refuse(LOCTEXT("NoGun", "That gun isn't carried"));
	}
	if (!WeaponPartSwap::CanModify(*Instance))
	{
		return Refuse(LOCTEXT("Fixed", "A named gun keeps its own parts"));
	}
	// Never leave the player with nothing to shoot: not the last gun carried, nor the last one in the slots (a backpack gun
	// can't be drawn in a fight).
	if (NumCarriedGuns() <= 1)
	{
		return Refuse(LOCTEXT("OnlyGun", "It's the only gun you carry"));
	}
	if (!Gun.bBackpack && NumEquipped() <= 1)
	{
		return Refuse(LOCTEXT("LastEquipped", "Equip another gun before scrapping this one"));
	}
	if (PartsBox.Num() >= MaxBoxedParts)
	{
		return Refuse(FText::Format(LOCTEXT("BoxFull", "The parts box is full ({0} parts)"), FText::AsNumber(MaxBoxedParts)));
	}
	if (WeaponPartSwap::ScrapChoices(*Instance).IsEmpty())
	{
		return Refuse(LOCTEXT("NothingToKeep", "It has no part to keep"));
	}
	return true;
}

bool UWeaponManagerComponent::ScrapGun(const FCarriedGun& Gun, FName KeepSlot)
{
	if (!CanScrap(Gun))
	{
		return false;
	}
	const FWeaponInstanceData Instance = *FindCarriedGun(Gun);
	const TArray<FBoxedWeaponPart> Choices = WeaponPartSwap::ScrapChoices(Instance);
	const FBoxedWeaponPart* Kept = Choices.FindByPredicate([KeepSlot](const FBoxedWeaponPart& Part) { return Part.Slot == KeepSlot; });
	if (!Kept)
	{
		return false;
	}

	// The part goes in first, so whatever hears the gun go sees the box with it.
	PartsBox.Add(*Kept);
	if (Gun.bBackpack)
	{
		Backpack.RemoveAt(Gun.Index);
	}
	else if (AWeaponBase* Weapon = RemoveFromSlots(Gun.Index))
	{
		// In hand, the next gun in the slots is taken in hand (RemoveFromSlots).
		Weapon->Destroy();
	}
	UE_LOG(LogLooter, Log, TEXT("Bench: scrapped a %s %s (seed %d), kept %s; %d parts in the box."), *StaticEnum<EWeaponRarity>()->GetNameStringByValue(static_cast<int64>(Instance.Rarity)),
		Instance.Definition ? *Instance.Definition->GetName() : TEXT("gun"), Instance.Seed, *DescribePart(*Kept), PartsBox.Num());
	OnInventoryChanged.Broadcast();
	SaveSoon(this);
	return true;
}

// ---------------------------------------------------------------------------
// Fitting
// ---------------------------------------------------------------------------

WeaponPartSwap::ECheck UWeaponManagerComponent::CheckFit(const FCarriedGun& Gun, int32 BoxIndex) const
{
	const FWeaponInstanceData* Instance = FindCarriedGun(Gun);
	if (!Instance || !PartsBox.IsValidIndex(BoxIndex))
	{
		return WeaponPartSwap::ECheck::NoSuchPart;
	}
	return WeaponPartSwap::CanFit(*Instance, PartsBox[BoxIndex]);
}

bool UWeaponManagerComponent::FitPart(const FCarriedGun& Gun, int32 BoxIndex)
{
	const FWeaponInstanceData* Current = FindCarriedGun(Gun);
	if (!Current || !PartsBox.IsValidIndex(BoxIndex))
	{
		return false;
	}
	// The rounds in its magazine now: a backpack gun's as stored (a fresh one's full), a slot's as its gun has them.
	const int32 Loaded = Gun.bBackpack ? (Current->SavedMagazine >= 0 ? Current->SavedMagazine : Current->Stats.MagazineSize)
		: Weapons[Gun.Index]->GetCurrentMagazine();
	FWeaponInstanceData Refitted = *Current;
	const FBoxedWeaponPart Part = PartsBox[BoxIndex];
	FBoxedWeaponPart Replaced;
	if (!WeaponPartSwap::Fit(Refitted, Part, Replaced))
	{
		return false;
	}
	// A smaller magazine can't hold what the old one did: the rounds over go back to the ammo carried, never lost.
	const int32 Kept = FMath::Clamp(Loaded, 0, FMath::Max(Refitted.Stats.MagazineSize, 0));
	Refitted.SavedMagazine = Kept;

	if (Gun.bBackpack)
	{
		Backpack[Gun.Index] = Refitted;
	}
	else if (!ReplaceEquipped(Gun.Index, Refitted))
	{
		return false;
	}

	// The part that came off takes the fitted one's place in the box, so fitting it again puts the gun back as it was.
	PartsBox.RemoveAt(BoxIndex);
	if (!Replaced.Key.IsNone())
	{
		PartsBox.Insert(Replaced, BoxIndex);
	}
	if (Loaded > Kept && Refitted.Definition)
	{
		AddAmmo(Refitted.Definition->AmmoType, Loaded - Kept);
	}
	UE_LOG(LogLooter, Log, TEXT("Bench: fitted %s%s; %d parts in the box."), *DescribePart(Part),
		Replaced.Key.IsNone() ? TEXT(" into an empty slot") : *FString::Printf(TEXT(" in place of %s"), *Replaced.Key.ToString()), PartsBox.Num());
	OnInventoryChanged.Broadcast();
	SaveSoon(this);
	return true;
}

bool UWeaponManagerComponent::ReplaceEquipped(int32 SlotIndex, const FWeaponInstanceData& Instance)
{
	if (!Weapons.IsValidIndex(SlotIndex))
	{
		return false;
	}
	// Made first, so a failure leaves everything as it was.
	AActor* Owner = GetOwner();
	AWeaponBase* Incoming = UWeaponRollLibrary::SpawnWeapon(this, Instance, Owner ? Owner->GetActorTransform() : FTransform::Identity);
	if (!Incoming)
	{
		return false;
	}
	AWeaponBase* Outgoing = Weapons[SlotIndex];
	const bool bInHand = SlotIndex == ActiveSlot;
	if (IsValid(Outgoing))
	{
		Outgoing->OnHolstered();
	}
	Weapons[SlotIndex] = Incoming;
	AttachHolstered(Incoming);
	if (bInHand)
	{
		// Taken in hand at once, and told as a change of gun with the old one still there, so whatever listens (the HUD's
		// magazine) lets go of it before it goes.
		USceneComponent* HoldParent = nullptr;
		FName HoldSocket;
		FTransform HoldOffset;
		GetHold(Incoming, HoldParent, HoldSocket, HoldOffset);
		Incoming->OnEquipped(Cast<APawn>(GetOwner()), HoldParent, HoldSocket, HoldOffset);
		OnActiveWeaponChanged.Broadcast(Incoming, Outgoing);
	}
	if (IsValid(Outgoing))
	{
		Outgoing->Destroy();
	}
	return true;
}

// ---------------------------------------------------------------------------
// The box
// ---------------------------------------------------------------------------

bool UWeaponManagerComponent::DiscardPart(int32 BoxIndex)
{
	if (!PartsBox.IsValidIndex(BoxIndex))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("Bench: threw out %s."), *DescribePart(PartsBox[BoxIndex]));
	PartsBox.RemoveAt(BoxIndex);
	OnInventoryChanged.Broadcast();
	SaveSoon(this);
	return true;
}

bool UWeaponManagerComponent::AddToPartsBox(const FBoxedWeaponPart& Part)
{
	if (!Part.Definition || Part.Slot.IsNone() || Part.Key.IsNone() || PartsBox.Num() >= MaxBoxedParts)
	{
		return false;
	}
	PartsBox.Add(Part);
	OnInventoryChanged.Broadcast();
	return true;
}

void UWeaponManagerComponent::ClearPartsBox()
{
	if (!PartsBox.IsEmpty())
	{
		PartsBox.Reset();
		OnInventoryChanged.Broadcast();
	}
}

#undef LOCTEXT_NAMESPACE
