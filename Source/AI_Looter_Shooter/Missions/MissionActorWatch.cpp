#include "Missions/MissionActorWatch.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionRunner.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"

void UMissionActorWatch::Watch(UMissionRunner* InRunner, AActor* InActor, UHealthComponent* Health)
{
	Runner = InRunner;
	Watched = InActor;
	if (Health)
	{
		Health->OnDeath.AddUniqueDynamic(this, &UMissionActorWatch::HandleDeath);
		Health->OnDamaged.AddUniqueDynamic(this, &UMissionActorWatch::HandleDamaged);
	}
}

void UMissionActorWatch::HandleDeath(AController* Killer)
{
	UMissionRunner* RunnerPtr = Runner.Get();
	AActor* Victim = Watched.Get();
	if (RunnerPtr && Victim)
	{
		RunnerPtr->HandleKill(*Victim, Killer);
	}
}

void UMissionActorWatch::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	UMissionRunner* RunnerPtr = Runner.Get();
	AActor* Victim = Watched.Get();
	if (RunnerPtr && Victim)
	{
		RunnerPtr->HandleHit(*Victim, InstigatedBy);
	}
}
