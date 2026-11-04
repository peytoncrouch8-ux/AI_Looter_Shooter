#include "World/ActorIndexSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Economy/SilverPickup.h"
#include "Loot/AmmoPickup.h"
#include "Loot/GrenadePickup.h"
#include "Shields/ShieldPickup.h"
#include "Weapons/WeaponBase.h"
#include "World/PlayableArea.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Volume.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

// UActorIndexSubsystem: the lists of loot, creatures, characters and volumes that replace per-frame TActorIterator scans.

namespace
{
	TAutoConsoleVariable<float> CVarActorIndexInterval(TEXT("Looter.ActorIndex.Interval"), UActorIndexSubsystem::DefaultInterval,
		TEXT("Seconds between the actor index's full looks through the world (the HUD map, loot labels, interaction and enemy ")
		TEXT("shots read its lists instead of scanning every frame). Spawned actors join at once either way."), ECVF_Default);

	/** One TActorIterator pass over the world for a class: the list is rebuilt, not added to, so the destroyed fall out. */
	template <typename ActorType>
	void Look(UWorld* World, TArray<TWeakObjectPtr<ActorType>>& OutList)
	{
		OutList.Reset();
		for (TActorIterator<ActorType> It(World); It; ++It)
		{
			OutList.Add(*It);
		}
	}

	void AddCreature(TArray<FIndexedCreature>& OutList, ACreatureBase* Creature)
	{
		FIndexedCreature& Entry = OutList.AddDefaulted_GetRef();
		Entry.Creature = Creature;
		Entry.bSentinel = Creature->IsA<AStoneSentinelCreature>();
	}
}

UActorIndexSubsystem* UActorIndexSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<UActorIndexSubsystem>() : nullptr;
}

float UActorIndexSubsystem::ClampInterval(float Wanted)
{
	return FMath::Clamp(Wanted, MinInterval, MaxInterval);
}

bool UActorIndexSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build. The editor's own world isn't indexed: its
	// actors come and go behind the spawn hook's back (undo, the level tools), and nothing there reads these lists.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UActorIndexSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UWorld* World = GetWorld())
	{
		// The earliest hook a spawn has (the actor is in its level already): an actor can't be spawned and missed, and is
		// listed before its own BeginPlay asks anything of the world.
		SpawnHandle = World->AddOnActorPreSpawnInitialization(FOnActorSpawned::FDelegate::CreateUObject(this, &UActorIndexSubsystem::HandleActorSpawned));
	}
}

void UActorIndexSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer);
		World->RemoveOnActorPreSpawnInitialization(SpawnHandle);
	}
	SpawnHandle.Reset();
	Guns.Reset();
	AmmoPickups.Reset();
	GrenadePickups.Reset();
	SilverPickups.Reset();
	ShieldPickups.Reset();
	Creatures.Reset();
	Characters.Reset();
	Volumes.Reset();
	PlayableAreas.Reset();
	Super::Deinitialize();
}

void UActorIndexSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The level's own actors are all set up now: the one look at them that the spawn hook can't give, then the timer's.
	ArmTimer();
	RefreshNow();
}

void UActorIndexSubsystem::ArmTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	ArmedSeconds = ClampInterval(CVarActorIndexInterval.GetValueOnGameThread());
	World->GetTimerManager().SetTimer(RefreshTimer, this, &UActorIndexSubsystem::RefreshNow, ArmedSeconds, /*bLoop*/ true);
}

float UActorIndexSubsystem::GetTimerRate() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimerManager().GetTimerRate(RefreshTimer) : -1.f;
}

void UActorIndexSubsystem::EnsureLooked()
{
	if (!bLooked)
	{
		RefreshNow();
	}
}

void UActorIndexSubsystem::RefreshNow()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	bLooked = true;
	++RefreshCount;

	Look(World, Guns);
	Look(World, AmmoPickups);
	Look(World, GrenadePickups);
	Look(World, SilverPickups);
	Look(World, ShieldPickups);
	Look(World, Volumes);
	Look(World, PlayableAreas);
	// Creatures are characters: one pass for both lists.
	Creatures.Reset();
	Characters.Reset();
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Character = *It;
		if (ACreatureBase* Creature = Cast<ACreatureBase>(Character))
		{
			// The living only: corpses lie about long after a fight and no reader wants one. (One that gets up again is listed
			// at the next look.)
			if (!Creature->IsDead())
			{
				AddCreature(Creatures, Creature);
			}
		}
		else
		{
			Characters.Add(Character);
		}
	}

	// A tuned interval takes hold here, at the timer's next look, without a restart.
	if (RefreshTimer.IsValid() && !FMath::IsNearlyEqual(ArmedSeconds, ClampInterval(CVarActorIndexInterval.GetValueOnGameThread())))
	{
		ArmTimer();
	}
}

void UActorIndexSubsystem::HandleActorSpawned(AActor* Actor)
{
	// Before the first look the look itself finds it.
	if (bLooked && Actor)
	{
		Note(*Actor);
	}
}

void UActorIndexSubsystem::Note(AActor& Actor)
{
	// Plain Add, not AddUnique: an actor spawns once, and the next look rebuilds each list from the world.
	if (AWeaponBase* Gun = Cast<AWeaponBase>(&Actor))
	{
		Guns.Add(Gun);
	}
	else if (ACharacter* Character = Cast<ACharacter>(&Actor))
	{
		if (ACreatureBase* Creature = Cast<ACreatureBase>(Character))
		{
			AddCreature(Creatures, Creature);
		}
		else
		{
			Characters.Add(Character);
		}
	}
	else if (AAmmoPickup* Ammo = Cast<AAmmoPickup>(&Actor))
	{
		AmmoPickups.Add(Ammo);
	}
	else if (AGrenadePickup* Grenade = Cast<AGrenadePickup>(&Actor))
	{
		GrenadePickups.Add(Grenade);
	}
	else if (ASilverPickup* Silver = Cast<ASilverPickup>(&Actor))
	{
		SilverPickups.Add(Silver);
	}
	else if (AShieldPickup* Ward = Cast<AShieldPickup>(&Actor))
	{
		ShieldPickups.Add(Ward);
	}
	else if (AVolume* Volume = Cast<AVolume>(&Actor))
	{
		Volumes.Add(Volume);
	}
	else if (APlayableArea* Area = Cast<APlayableArea>(&Actor))
	{
		PlayableAreas.Add(Area);
	}
}
