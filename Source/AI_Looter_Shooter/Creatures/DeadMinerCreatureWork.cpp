// ADeadMinerCreature at work while nobody's about: it finds the nearest wall at chest height and swings its pick at it, a
// steady blow every couple of seconds (sparks off the rock and the iron's ring, its lamp on the face it works), or with no
// wall near it stands over its lunch pail, lifting the lid now and then. It never wanders off (WanderRadius 0): it works
// where it was put, and its first look round for its wall comes a moment after its neighbours'.

#include "Creatures/DeadMinerCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/EncounterSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** It looks for its wall this high over its middle (cm at size 1): about its chest. */
	constexpr float ChestRise = 35.f;

	/** A face steeper than this (its normal's height) is a wall to work; flatter is floor or roof. */
	constexpr float WallMaxUp = 0.6f;

	/** It turns to its wall this fast (degrees a second). */
	constexpr float WorkTurnRate = 200.f;

	/** Its blows are seen and heard only by a player this near (cm): a gallery far off costs nothing. */
	constexpr float WorkSeenWithin = 3500.f;

	/** At its pail, the lid comes up every this many seconds, give or take a third. */
	constexpr float PailEvery = 9.f;
}

bool ADeadMinerCreature::WantsToWork() const
{
	if (IsDead() || GetTarget() || IsPhasing())
	{
		return false;
	}
	const ECreatureState BrainState = GetCreatureState();
	return BrainState == ECreatureState::Idle || BrainState == ECreatureState::Wander;
}

void ADeadMinerCreature::TickWork(float DeltaSeconds)
{
	if (Act == EAct::Reel)
	{
		return;
	}
	if (!WantsToWork())
	{
		if (Act == EAct::Work || Act == EAct::Pail)
		{
			SetAct(EAct::None);
			NextWorkLook = 1.f;
		}
		return;
	}
	if (Act == EAct::None)
	{
		NextWorkLook -= DeltaSeconds;
		if (NextWorkLook <= 0.f)
		{
			StartWorking();
		}
		return;
	}
	// At work it stays where it is: nothing of the brain's steering moves it.
	ConsumeMovementInputVector();
	if (Act == EAct::Work)
	{
		FaceYaw(WorkYaw, DeltaSeconds);
		const float From = WorkTime;
		WorkTime += DeltaSeconds;
		if (DeadMinerRules::WorkBlowBetween(From, WorkTime, Tuning.WorkSwingSeconds))
		{
			WorkBlow();
		}
	}
	else if (Act == EAct::Pail)
	{
		PailClock -= DeltaSeconds;
		if (PailClock <= 0.f)
		{
			PailClock = PailEvery * FMath::FRandRange(0.67f, 1.33f);
			LooterSound::PlayAttached(LooterSoundCue::DeadMiner::Pail, Pail, NAME_None, 0.6f);
		}
	}
}

void ADeadMinerCreature::StartWorking()
{
	FVector Point;
	FVector Normal;
	if (FindWorkWall(Point, Normal))
	{
		WorkPoint = Point;
		WorkNormal = Normal;
		WorkYaw = static_cast<float>((Point - GetActorLocation()).Rotation().Yaw);
		// Out of step with the miner beside it: a gallery of them rings unevenly, as men working do.
		WorkTime = FMath::FRandRange(0.f, Tuning.WorkSwingSeconds);
		SetAct(EAct::Work);
		return;
	}
	PailClock = PailEvery * FMath::FRandRange(0.3f, 1.f);
	SetAct(EAct::Pail);
}

bool ADeadMinerCreature::FindWorkWall(FVector& OutPoint, FVector& OutNormal) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const float Scale = GetSizeScale();
	const FVector Chest = GetActorLocation() + FVector(0.0, 0.0, ChestRise * Scale);
	const float Reach = Tuning.WorkReach * Scale;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DeadMinerWork"), this);
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	// Eight ways round it, the way it faces first: the nearest steep face wins.
	double Nearest = TNumericLimits<double>::Max();
	bool bFound = false;
	for (int32 Step = 0; Step < 8; ++Step)
	{
		const FVector Way = Forward.RotateAngleAxis(45.f * static_cast<float>(Step), FVector::UpVector);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Chest, Chest + Way * Reach, FCollisionObjectQueryParams(ECC_WorldStatic), Params)
			&& FMath::Abs(Hit.ImpactNormal.Z) < WallMaxUp && Hit.Distance < Nearest)
		{
			Nearest = Hit.Distance;
			OutPoint = Hit.ImpactPoint;
			OutNormal = Hit.ImpactNormal;
			bFound = true;
		}
	}
	return bFound;
}

void ADeadMinerCreature::WorkBlow()
{
	++WorkBlows;
	// Seen and heard only near the player: a far gallery's blows cost nothing.
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	if (!Player)
	{
		Player = UGameplayStatics::GetPlayerPawn(this, 0);
	}
	if (!Player || FVector::Dist(Player->GetActorLocation(), WorkPoint) > WorkSeenWithin)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnImpact(WorkPoint, WorkNormal, -WorkNormal, EImpactSurface::World, false);
	}
	LooterSound::PlayAt(this, LooterSoundCue::DeadMiner::PickWall, WorkPoint, 0.75f, Voice->GetPitch() * FMath::FRandRange(0.95f, 1.05f));
}

void ADeadMinerCreature::FaceYaw(float Yaw, float DeltaSeconds)
{
	const FRotator Wanted(0.f, Yaw, 0.f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Wanted, DeltaSeconds, WorkTurnRate));
}
