#include "Creatures/BellRingerGraves.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

const FName UBellRingerGraves::RaisedTag(TEXT("BellRaised"));

UBellRingerGraves* UBellRingerGraves::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject && GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UBellRingerGraves>() : nullptr;
}

bool UBellRingerGraves::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UBellRingerGraves::Deinitialize()
{
	Fallen.Reset();
	Super::Deinitialize();
}

bool UBellRingerGraves::CanBeRaised(const ACreatureBase& Creature)
{
	if (!Creature.IsA<AUnpaidCreature>() || Creature.IsA<ABellRingerCreature>() || Creature.ActorHasTag(RaisedTag))
	{
		return false;
	}
	const ECreatureRank Rank = Creature.GetRank();
	if (Rank == ECreatureRank::Boss || Rank == ECreatureRank::Legendary || Creature.FindComponentByClass<UBossComponent>())
	{
		return false;
	}
	// A placed one comes back at home by itself: a raised copy would make two.
	return !Creature.WillRespawn();
}

void UBellRingerGraves::Watch(double Now, bool bNow)
{
	if (!bNow && Now - LastWatch < WatchSeconds)
	{
		return;
	}
	LastWatch = Now;
	Prune(Now);
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World))
	{
		for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
		{
			const ACreatureBase* Creature = Each.Get();
			if (Creature && Creature->IsDead() && !Creature->IsActorBeingDestroyed())
			{
				RecordFallen(*Creature, Now);
			}
		}
		return;
	}
	for (TActorIterator<AUnpaidCreature> It(World); It; ++It)
	{
		if (It->IsDead())
		{
			RecordFallen(**It, Now);
		}
	}
}

bool UBellRingerGraves::RecordFallen(const ACreatureBase& Dead, double Now)
{
	const uint32 Id = Dead.GetUniqueID();
	if (!CanBeRaised(Dead) || Fallen.ContainsByPredicate([Id](const FBellRingerFallen& Each) { return Each.Id == Id; }))
	{
		return false;
	}
	FBellRingerFallen& Noted = Fallen.AddDefaulted_GetRef();
	Noted.Id = Id;
	Noted.Class = Dead.GetClass();
	const UCapsuleComponent* Capsule = Dead.GetCapsuleComponent();
	Noted.Feet = Dead.GetActorLocation() - FVector(0.f, 0.f, Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f);
	Noted.Yaw = static_cast<float>(Dead.GetActorRotation().Yaw);
	Noted.Rank = Dead.GetRank();
	Noted.OwnLevel = FMath::Max(1, Dead.Level - UCreatureRankSettings::Get(Noted.Rank).LevelOffset);
	Noted.BodyScale = Dead.BodyScale;
	Noted.Ground = Dead.HuntingGround;
	if (Noted.Ground.IsSet() && Noted.Ground.bAroundHome)
	{
		// Its ground was round its own spot; the raised one's home is its grave, so the middle is fixed where it was.
		Noted.Ground.Center = Noted.Ground.GetMiddle(Dead.GetHome().GetLocation());
		Noted.Ground.bAroundHome = false;
	}
	Noted.DiedAt = Now;
	UE_LOG(LogLooter, Verbose, TEXT("Bell-Ringers: %s fell at %s."), *Dead.GetName(), *Noted.Feet.ToCompactString());
	return true;
}

int32 UBellRingerGraves::FindRaisable(const FVector& Ringer, double Now, const FVector& Player, const FBellRingerTuning& Tuning) const
{
	return BellRingerRules::ChooseFallen(Fallen, Ringer, Now, Player, Tuning);
}

bool UBellRingerGraves::Claim(int32 Index)
{
	if (!Fallen.IsValidIndex(Index) || Fallen[Index].bRaised)
	{
		return false;
	}
	Fallen[Index].bRaised = true;
	return true;
}

void UBellRingerGraves::Prune(double Now)
{
	Fallen.RemoveAll([Now](const FBellRingerFallen& Each) { return Now - Each.DiedAt > KeepSeconds; });
}
