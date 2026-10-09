#include "Audio/CreatureVoiceDirector.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Creatures/CreatureBase.h"
#include "UI/World/CreatureBarkActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UCreatureVoiceDirector* UCreatureVoiceDirector::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject && GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UCreatureVoiceDirector>() : nullptr;
}

bool UCreatureVoiceDirector::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UCreatureVoiceDirector::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IdleTimer);
	}
	Voices.Reset();
	RecentLines.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// The voices
// ---------------------------------------------------------------------------

void UCreatureVoiceDirector::Register(UCreatureVoiceComponent* Voice)
{
	if (!Voice)
	{
		return;
	}
	Voices.RemoveAll([](const TWeakObjectPtr<UCreatureVoiceComponent>& Each) { return !Each.IsValid(); });
	Voices.AddUnique(Voice);
	UpdateTimer();
}

void UCreatureVoiceDirector::Unregister(UCreatureVoiceComponent* Voice)
{
	Voices.RemoveAll([Voice](const TWeakObjectPtr<UCreatureVoiceComponent>& Each) { return !Each.IsValid() || Each.Get() == Voice; });
	UpdateTimer();
}

int32 UCreatureVoiceDirector::NumVoices() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<UCreatureVoiceComponent>& Each : Voices)
	{
		Count += Each.IsValid() ? 1 : 0;
	}
	return Count;
}

void UCreatureVoiceDirector::UpdateTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (Voices.IsEmpty())
	{
		Timers.ClearTimer(IdleTimer);
	}
	else if (!Timers.IsTimerActive(IdleTimer))
	{
		Timers.SetTimer(IdleTimer, this, &UCreatureVoiceDirector::CheckIdle, IdleCheckSeconds, /*bLoop*/ true,
			Random.FRandRange(0.5f, IdleCheckSeconds));
	}
}

TOptional<FVector> UCreatureVoiceDirector::GetListener() const
{
	if (const AActor* StandIn = TestListener.Get())
	{
		return StandIn->GetActorLocation();
	}
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (!Controller)
	{
		return {};
	}
	if (const APawn* Pawn = Controller->GetPawn())
	{
		return Pawn->GetActorLocation();
	}
	if (const APlayerCameraManager* Camera = Controller->PlayerCameraManager)
	{
		return Camera->GetCameraLocation();
	}
	return {};
}

void UCreatureVoiceDirector::SetTestListener(AActor* StandIn)
{
	TestListener = StandIn;
}

// ---------------------------------------------------------------------------
// Barks
// ---------------------------------------------------------------------------

bool UCreatureVoiceDirector::RequestBark(UCreatureVoiceComponent& Speaker, ECreatureBark Situation)
{
	UWorld* World = GetWorld();
	ACreatureBase* Creature = Cast<ACreatureBase>(Speaker.GetOwner());
	const TOptional<FVector> Listener = GetListener();
	if (!World || !Creature || !Listener.IsSet() || !Speaker.CanBark())
	{
		return false;
	}
	const double Now = World->GetTimeSeconds();
	const uint32 Id = Creature->GetUniqueID();
	if (Board.Check(Id, Situation, Creature->GetActorLocation(), Listener.GetValue(), Now) != FCreatureBarkBoard::EVerdict::Allowed)
	{
		return false;
	}
	const int32 Line = CreatureBarks::PickLine(Situation, Creature->GetRank(), Random, RecentLines);
	if (Line == INDEX_NONE)
	{
		return false;
	}
	const FString Text = CreatureBarks::AllLines()[Line].Text;

	// One voice at a time too: a bark cutting in over another stops the other's murmur where it is.
	UCreatureVoiceComponent* Previous = LastSpeaker.Get();
	if (Previous && Previous != &Speaker && Board.IsShowing(Now))
	{
		Previous->StopMurmur();
	}
	Speaker.SpeakMurmur(Text, Situation);
	const TArray<FMurmurGrain>& Murmur = Speaker.GetMurmur();
	const float MurmurSeconds = Murmur.IsEmpty() ? 0.f : Murmur.Last().Time + 0.25f;
	const float Seconds = CreatureBarks::DisplaySeconds(Text, MurmurSeconds);

	Board.Start(Id, Situation, Creature->GetActorLocation(), Now, Seconds);
	RecentLines.Add(Line);
	if (RecentLines.Num() > CreatureBarks::RecentLines)
	{
		RecentLines.RemoveAt(0, RecentLines.Num() - CreatureBarks::RecentLines);
	}
	LastSpeaker = &Speaker;
	if (ACreatureBarkActor* Words = FindOrSpawnBarkActor())
	{
		Words->Show(Creature, FText::FromString(Text), Seconds, CreatureBarks::IsWhispered(Situation));
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s says: \"%s\""), *Creature->GetName(), *Text);
	return true;
}

void UCreatureVoiceDirector::NotifyDeath(const UCreatureVoiceComponent& Dead)
{
	const ACreatureBase* Fallen = Cast<ACreatureBase>(Dead.GetOwner());
	if (!Fallen)
	{
		return;
	}
	const FVector Where = Fallen->GetActorLocation();
	UCreatureVoiceComponent* Nearest = nullptr;
	double NearestSquared = FMath::Square(static_cast<double>(PackmateRadius));
	for (const TWeakObjectPtr<UCreatureVoiceComponent>& Each : Voices)
	{
		UCreatureVoiceComponent* Voice = Each.Get();
		const ACreatureBase* Creature = Voice ? Cast<ACreatureBase>(Voice->GetOwner()) : nullptr;
		if (!Creature || Voice == &Dead || Creature->IsDead() || !Voice->CanBark() || !Creature->SharesPackWith(*Fallen))
		{
			continue;
		}
		const double Squared = FVector::DistSquared(Creature->GetActorLocation(), Where);
		if (Squared <= NearestSquared)
		{
			NearestSquared = Squared;
			Nearest = Voice;
		}
	}
	if (Nearest)
	{
		Nearest->TryBark(ECreatureBark::PackmateDeath);
	}
}

void UCreatureVoiceDirector::CheckIdle()
{
	UWorld* World = GetWorld();
	const TOptional<FVector> Listener = GetListener();
	if (!World || !Listener.IsSet())
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const double ReachSquared = FMath::Square(static_cast<double>(IdleRadius));
	TArray<UCreatureVoiceComponent*, TInlineAllocator<16>> AtRest;
	for (const TWeakObjectPtr<UCreatureVoiceComponent>& Each : Voices)
	{
		UCreatureVoiceComponent* Voice = Each.Get();
		const ACreatureBase* Creature = Voice ? Cast<ACreatureBase>(Voice->GetOwner()) : nullptr;
		if (!Creature || Creature->IsDead() || Creature->IsHidden())
		{
			continue;
		}
		const ECreatureState State = Creature->GetCreatureState();
		if ((State == ECreatureState::Idle || State == ECreatureState::Wander)
			&& FVector::DistSquared(Creature->GetActorLocation(), Listener.GetValue()) <= ReachSquared)
		{
			AtRest.Add(Voice);
		}
	}
	if (AtRest.IsEmpty())
	{
		return;
	}
	UCreatureVoiceComponent* Picked = AtRest[Random.RandRange(0, AtRest.Num() - 1)];
	if (Picked->CanBark())
	{
		// An Unpaid mutters a line to itself (its own chance and rest decide).
		Picked->TryBark(ECreatureBark::Idle);
		return;
	}
	if (Picked->HasIdleCall() && Now >= NextIdleCall && Now >= Picked->GetNextIdleCall() && Random.FRand() < IdleCallChance)
	{
		Picked->PlayIdleCall();
		NextIdleCall = Now + IdleCallGap;
		Picked->SetNextIdleCall(Now + Random.FRandRange(IdleCallRestMin, IdleCallRestMax));
	}
}

ACreatureBarkActor* UCreatureVoiceDirector::FindOrSpawnBarkActor()
{
	if (ACreatureBarkActor* Existing = BarkActor.Get())
	{
		return Existing;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACreatureBarkActor* Spawned = World->SpawnActor<ACreatureBarkActor>(ACreatureBarkActor::StaticClass(), FTransform::Identity, Params);
	BarkActor = Spawned;
	return Spawned;
}
