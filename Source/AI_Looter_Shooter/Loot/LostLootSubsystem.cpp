// ULostLootSubsystem: Lost Property's shelves while a level plays (put back from the session, guns handed in, named guns
// found and taken), the clock the tickets count by, and the HUD's word of what was handed in. The ground's guns and their
// five minutes are LostLootGround.cpp's.

#include "Loot/LostLootSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LostLootRules.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/WeaponText.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

namespace
{
	/** The HUD says nothing of Lost Property until the level has been up this long (s): past the fade in and the arrival. */
	constexpr double AnnounceAfter = 3.0;

	/** Two of its lines never land closer than this (s): the message line shows one at a time. */
	constexpr double LineGap = 3.5;
}

ULostLootSubsystem* ULostLootSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<ULostLootSubsystem>() : nullptr;
}

bool ULostLootSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void ULostLootSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Only a played level keeps the ground's clock: the tests' levels (editor previews) run it by hand, so no test's gun
	// is ever handed in behind its back.
	if (InWorld.WorldType != EWorldType::Game && InWorld.WorldType != EWorldType::PIE)
	{
		return;
	}
	LastClock = InWorld.GetTimeSeconds();
	BeganAt = LastClock;
	InWorld.GetTimerManager().SetTimer(ClockTimer, FTimerDelegate::CreateUObject(this, &ULostLootSubsystem::HandleClock),
		LostLootRules::ClockSeconds, /*bLoop*/ true);
}

void ULostLootSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	GroundLife.Reset();
	OnChanged.Clear();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// The shelves
// ---------------------------------------------------------------------------

void ULostLootSubsystem::SetStore(const FLostLootStore& InStore)
{
	Store = InStore;
	// A gun whose kind is gone from the game (its data asset deleted) can't be claimed or shown: it goes, as a saved gun
	// of a missing kind never comes back anywhere else.
	const int32 Before = Store.Entries.Num();
	Store.Entries.RemoveAll([](const FLostLootEntry& Entry) { return Entry.Weapon.Definition == nullptr; });
	if (Store.Entries.Num() != Before)
	{
		UE_LOG(LogLooter, Warning, TEXT("Lost Property: %d guns of kinds no longer in the game left the shelves."), Before - Store.Entries.Num());
	}
	Store.Upgrades = FMath::Clamp(Store.Upgrades, 0, LostLootRules::MaxUpgrades);
	// The counter never hands out a ticket that's on a shelf already.
	for (const FLostLootEntry& Entry : Store.Entries)
	{
		Store.NextTicket = FMath::Max(Store.NextTicket, Entry.Ticket + 1);
	}
	NotifyChanged();
}

void ULostLootSubsystem::NotifyChanged()
{
	OnChanged.Broadcast();
}

bool ULostLootSubsystem::HoldsNamed(const UNamedWeaponDefinition* Named) const
{
	return Store.FindNamed(Named) != INDEX_NONE;
}

bool ULostLootSubsystem::TakeNamed(const UNamedWeaponDefinition* Named, FWeaponInstanceData& OutGun)
{
	if (!Store.RemoveAt(Store.FindNamed(Named), &OutGun))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("Lost Property: %s taken off the shelves to go back to the player."), *LooterWeaponText::Name(OutGun));
	NotifyChanged();
	return true;
}

int32 ULostLootSubsystem::Receive(TArray<FLostLootArrival> Arrivals, ELostLootReason Reason, const FString& Map, bool bAnnounce)
{
	FLostLootStore::SortOldestFirst(Arrivals);
	const double Now = GetClock();
	int32 Added = 0;
	int32 Gone = 0;
	for (const FLostLootArrival& Arrival : Arrivals)
	{
		bool bGone = false;
		if (Store.Add(Arrival.Weapon, Reason, Map, Now, nullptr, &bGone))
		{
			++Added;
			Gone += bGone ? 1 : 0;
		}
	}
	if (Added == 0)
	{
		return 0;
	}
	if (bAnnounce)
	{
		Store.Unannounced += Added;
		Store.UnannouncedGone += Gone;
	}
	UE_LOG(LogLooter, Log, TEXT("Lost Property: %d guns handed in from %s (%s)%s; %d of %d on the shelves."), Added,
		Map.IsEmpty() ? TEXT("somewhere") : *FPackageName::GetShortName(Map), *UEnum::GetValueAsString(Reason),
		Gone > 0 ? *FString::Printf(TEXT(", the %d oldest gone to make room"), Gone) : TEXT(""), Store.Num(), Store.GetCapacity());
	NotifyChanged();
	return Added;
}

int32 ULostLootSubsystem::ClearStore()
{
	const int32 Gone = Store.Clear();
	NotifyChanged();
	return Gone;
}

double ULostLootSubsystem::GetClock() const
{
	// The session's time played, as the vendors' day counts it, so a ticket's age carries across loads; else this level's.
	const UWorld* World = GetWorld();
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	if (Sessions && Sessions->IsPlayingSession())
	{
		return Sessions->GetPlayedSecondsNow(World);
	}
	return World ? World->GetTimeSeconds() : 0.0;
}

// ---------------------------------------------------------------------------
// The HUD's word
// ---------------------------------------------------------------------------

bool ULostLootSubsystem::SayToPlayer(const FText& Line) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	if (!Manager)
	{
		return false;
	}
	// The weapon manager's message line, where "sent to backpack" shows: the player's own news about their guns.
	Manager->OnMessage.Broadcast(Line);
	LooterSound::Play2D(this, LooterSoundCue::LostLoot::Sent);
	return true;
}

bool ULostLootSubsystem::TryAnnounce()
{
	const bool bSentWaiting = Store.Unannounced > 0;
	if (!bSentWaiting && !bExplainWaiting)
	{
		return false;
	}
	UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() - BeganAt < AnnounceAfter || World->GetTimeSeconds() - LastSaidAt < LineGap)
	{
		return false;
	}
	// Only where the player sees it: not under a scene (the cold open, a train's arrival) or a menu.
	const APlayerController* Controller = World->GetFirstPlayerController();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const ALooterHUD* HUD = ALooterHUD::FindFor(Pawn);
	if (!Pawn || USceneSubsystem::HidesGameplayHUD(World) || (HUD && HUD->IsMenuOpen()))
	{
		return false;
	}
	if (bSentWaiting)
	{
		if (!SayToPlayer(LostLootRules::SentLine(Store.Unannounced, Store.UnannouncedGone)))
		{
			return false;
		}
		Store.Unannounced = 0;
		Store.UnannouncedGone = 0;
		// Told where its guns go, the player needs no explaining later.
		Store.bExplained = true;
		bExplainWaiting = false;
	}
	else
	{
		if (!SayToPlayer(LostLootRules::ExplainLine(ExplainGun)))
		{
			return false;
		}
		Store.bExplained = true;
		bExplainWaiting = false;
	}
	LastSaidAt = World->GetTimeSeconds();
	return true;
}
