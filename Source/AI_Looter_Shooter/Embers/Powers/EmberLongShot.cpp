#include "Embers/Powers/EmberLongShot.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberDefinition.h"
#include "Player/ViewKick.h"
#include "UI/World/EmberMarkActor.h"
#include "Weapons/WeaponFX.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

namespace
{
	const FName MaxBonusName(TEXT("MaxBonus"));
	const FName ConeName(TEXT("ConeDegrees"));
	const FName MarkFromName(TEXT("MarkFrom"));

	/** A tink no closer than this to the last (seconds): a full-auto's hits don't stack them. */
	constexpr double TinkEvery = 0.06;
}

void UEmberLongShot::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Strength = Def && Def->Strength > 0.f ? Def->Strength : DefaultStrength;
	MaxBonus = FMath::Max(Def ? Def->Extra(MaxBonusName, DefaultMaxBonus) : DefaultMaxBonus, 0.f);
	Range = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	Cone = FMath::Clamp(Def ? Def->Extra(ConeName, DefaultCone) : DefaultCone, 1.f, 80.f);
	MarkFrom = FMath::Max(Def ? Def->Extra(MarkFromName, DefaultMarkFrom) : DefaultMarkFrom, 0.f);
	Marks.Reset();
	LastTink = -10.0;
	bSteady = true;
	Bind(true);
	MarkInSights();

	LooterSound::Play2D(Character, LooterSoundCue::Ember::LongShotStart);
	FViewKick Steady;
	Steady.Pitch = -0.2f;
	Steady.FieldOfView = -0.03f;
	Steady.Frequency = 3.f;
	Steady.Damping = 0.8f;
	Kick(Steady);
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Long Shot: +%.0f%% per 10 m (to +%.0f%%), %d marked"), Strength * 100.f, MaxBonus * 100.f, Marks.Num());
}

void UEmberLongShot::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	// The dead let go of their marks.
	for (int32 Index = Marks.Num() - 1; Index >= 0; --Index)
	{
		const ACreatureBase* Creature = Marks[Index].Creature.Get();
		if (!Creature || Creature->IsDead())
		{
			if (AEmberMarkActor* Tag = Marks[Index].Tag.Get())
			{
				Tag->Retire();
			}
			Marks.RemoveAt(Index);
		}
	}
}

void UEmberLongShot::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bSteady)
	{
		return;
	}
	bSteady = false;
	Bind(false);
	for (const FMarkOn& Each : Marks)
	{
		if (AEmberMarkActor* Tag = Each.Tag.Get())
		{
			Tag->Retire();
		}
	}
	Marks.Reset();
	UE_LOG(LogLooter, Log, TEXT("Long Shot: %s"), bInterrupted ? TEXT("cut short") : TEXT("over"));
}

void UEmberLongShot::Bind(bool bOn)
{
	if (UBulletSubsystem* Old = Bullets.Get())
	{
		Old->OnBulletLanding.Remove(LandingHandle);
	}
	LandingHandle.Reset();
	Bullets.Reset();
	UWorld* World = GetBodyWorld();
	UBulletSubsystem* Subsystem = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (bOn && Subsystem)
	{
		Bullets = Subsystem;
		LandingHandle = Subsystem->OnBulletLanding.AddUObject(this, &UEmberLongShot::NoteLanding);
	}
}

// ---------------------------------------------------------------------------
// Marks
// ---------------------------------------------------------------------------

bool UEmberLongShot::IsMarked(const ACreatureBase& Creature) const
{
	return Marks.ContainsByPredicate([&Creature](const FMarkOn& Each) { return Each.Creature.Get() == &Creature; });
}

void UEmberLongShot::MarkCreature(ACreatureBase& Creature)
{
	if (IsMarked(Creature) || Creature.IsDead())
	{
		return;
	}
	FMarkOn& Entry = Marks.AddDefaulted_GetRef();
	Entry.Creature = &Creature;
	Entry.Tag = AEmberMarkActor::Mark(&Creature, Color);
	// A glint of teal on it as it's marked.
	for (int32 Index = 0; Index < 8; ++Index)
	{
		Sparks.Spawn(Creature.GetActorLocation() + Random.GetUnitVector() * 25.f, Random.GetUnitVector() * 120.f + FVector(0.0, 0.0, 80.0),
			Random.FRandRange(0.3f, 0.6f), 1.3f, Color, 18.f, 50.f, 2.f);
	}
}

void UEmberLongShot::MarkInSights()
{
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	// The living in his sights: within the cone and the range, seen; the nearest the crosshair first.
	FVector Eye;
	FVector Aim;
	GetAim(*Character, Eye, Aim);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(Cone));
	TArray<TPair<float, ACreatureBase*>> Seen;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!Creature || Creature->IsDead() || Creature->IsHidden())
		{
			continue;
		}
		const FVector To = Creature->GetActorLocation() - Eye;
		const float Distance = static_cast<float>(To.Size());
		if (Distance > Range || Distance < 1.f)
		{
			continue;
		}
		const float Dot = static_cast<float>(FVector::DotProduct(To / Distance, Aim));
		if (Dot < MinDot || !CanSee(*World, Eye, Creature->GetActorLocation(), Creature))
		{
			continue;
		}
		Seen.Emplace(-Dot, Creature);
	}
	Seen.Sort([](const TPair<float, ACreatureBase*>& A, const TPair<float, ACreatureBase*>& B) { return A.Key < B.Key; });
	for (int32 Index = 0; Index < FMath::Min(Seen.Num(), MaxMarks); ++Index)
	{
		MarkCreature(*Seen[Index].Value);
	}
}

// ---------------------------------------------------------------------------
// The shots
// ---------------------------------------------------------------------------

float UEmberLongShot::ScaleAt(float Distance) const
{
	return 1.f + FMath::Min(Strength * FMath::Max(Distance, 0.f) / 1000.f, MaxBonus);
}

void UEmberLongShot::NoteLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing)
{
	if (!bSteady || !Body.IsValid() || Shot.Shooter.Get() != Body.Get() || Shot.bBlank)
	{
		return;
	}
	const float Scale = ScaleAt(Landing.Traveled);
	Landing.DamageScale *= Scale;
	ACreatureBase* Creature = Cast<ACreatureBase>(Hit.GetActor());
	if (!Creature || Creature->IsDead())
	{
		return;
	}
	// A long hit marks it; a marked one is always critical.
	if (Landing.Traveled >= MarkFrom)
	{
		MarkCreature(*Creature);
	}
	if (IsMarked(*Creature))
	{
		Landing.bCritical = true;
		for (const FMarkOn& Each : Marks)
		{
			if (Each.Creature.Get() == Creature)
			{
				if (AEmberMarkActor* Tag = Each.Tag.Get())
				{
					Tag->Flare();
				}
			}
		}
	}
	// Felt: a far tink, higher the farther it flew; sparks of teal as big as the bonus.
	const UWorld* World = Creature->GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastTink >= TinkEvery)
	{
		LastTink = Now;
		LooterSound::Play2D(Creature, LooterSoundCue::Ember::LongShotHit, 0.55f + 0.3f * (Scale - 1.f), 0.9f + 0.35f * (Scale - 1.f));
	}
	const int32 Count = 3 + FMath::RoundToInt(9.f * (Scale - 1.f));
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Out = (Hit.ImpactNormal + Random.GetUnitVector()).GetSafeNormal();
		Sparks.Spawn(Hit.ImpactPoint, Out * Random.FRandRange(150.f, 380.f), Random.FRandRange(0.15f, 0.3f), 1.1f, Color, 20.f, 400.f, 3.f);
	}
}

bool UEmberLongShot::Linger(float DeltaSeconds)
{
	Sparks.Tick(DeltaSeconds, Effects());
	return bSteady || !Sparks.IsEmpty();
}
