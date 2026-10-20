// UEmberRailSpike (EmberRailSpike.h): the line, the flight and what it does to what it goes through. Every look (the spike,
// its rings, steam, sparks, the ground's dust and scar, the kicks) is EmberRailSpikeLook.cpp's.

#include "Embers/Powers/EmberRailSpike.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Embers/EmberDefinition.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

namespace
{
	const FName ShoveName(TEXT("Shove"));
	const FName DizzyName(TEXT("Dizzy"));

	/** The arc of a shove (cm). */
	constexpr float ShoveHeight = 40.f;
}

FRailSpikeLine UEmberRailSpike::PlanLine(const ACharacter& Character, float Range, float HalfWidth)
{
	FRailSpikeLine Plan;
	UWorld* World = Character.GetWorld();
	GetAim(Character, Plan.Start, Plan.Direction);
	Plan.End = Plan.Start + Plan.Direction * Range;
	Plan.Length = Range;
	if (!World)
	{
		return Plan;
	}
	// The world's solid shapes stop it (creatures don't: it goes through them).
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("RailSpike"), &Character);
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Plan.Start, Plan.End, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Plan.End = Hit.ImpactPoint;
		Plan.Length = static_cast<float>(Hit.Distance);
		Plan.bEmbeds = true;
		Plan.EmbedNormal = Hit.ImpactNormal;
	}
	// Every living body on the line, nearest first.
	TArray<TPair<float, ACreatureBase*>> Found;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!Creature || Creature->IsDead() || Creature->IsHidden())
		{
			continue;
		}
		FVector OnLine;
		FVector OnBody;
		if (DistanceToBody(*Creature, Plan.Start, Plan.End, OnLine, OnBody) > HalfWidth)
		{
			continue;
		}
		Found.Emplace(static_cast<float>(FVector::Dist(Plan.Start, OnLine)), Creature);
	}
	Found.Sort([](const TPair<float, ACreatureBase*>& A, const TPair<float, ACreatureBase*>& B) { return A.Key < B.Key; });
	for (const TPair<float, ACreatureBase*>& Each : Found)
	{
		Plan.Creatures.Add(Each.Value);
		Plan.Along.Add(Each.Key);
	}
	return Plan;
}

void UEmberRailSpike::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	if (!Character || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Damage = ScaledDamage(Context, DefaultDamage);
	const float Range = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	const float HalfWidth = Def && Def->Radius > 0.f ? Def->Radius : DefaultRadius;
	Shove = FMath::Max(Def ? Def->Extra(ShoveName, DefaultShove) : DefaultShove, 0.f);
	Dizzy = FMath::Max(Def ? Def->Extra(DizzyName, DefaultDizzy) : DefaultDizzy, 0.f);
	Line = PlanLine(*Character, Range, HalfWidth);
	VisualStart = HandPoint(*Character, 22.f, 26.f, 30.f);
	Flown = 0.f;
	SteamLaid = 0.f;
	NextCreature = 0;
	Pierced = 0;
	EmbedAge = -1.f;
	FallAge = -1.f;
	TrailAge = 0.f;
	bFlying = true;

	// The piston's bang; the look of it leaving the arm (steam, rings, the kick) and the ground under its way.
	LooterSound::Play2D(Character, LooterSoundCue::Ember::RailSpikeFire);
	BeginLook();
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Rail Spike: %.0f cm line, %d creature(s) on it, %.1f each%s"), Line.Length, Line.Creatures.Num(), Damage,
		Line.bEmbeds ? TEXT(", embeds") : TEXT(""));
}

void UEmberRailSpike::End(const FEmberContext& Context, bool bInterrupted)
{
	// Instant: over the moment it's driven; the spike flies on its own (StepSpike).
}

// ---------------------------------------------------------------------------
// The flight
// ---------------------------------------------------------------------------

void UEmberRailSpike::StepSpike(float DeltaSeconds)
{
	if (!bFlying)
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	const float Was = Flown;
	Flown = FMath::Min(Flown + SpikeSpeed * Dt, Line.Length);
	// Through each body it reaches this frame, in order.
	while (NextCreature < Line.Creatures.Num() && Line.Along[NextCreature] <= Flown)
	{
		if (ACreatureBase* Creature = Line.Creatures[NextCreature].Get(); Creature && !Creature->IsDead())
		{
			Pierce(*Creature, Line.Along[NextCreature]);
		}
		++NextCreature;
	}
	if (Flown >= Line.Length)
	{
		bFlying = false;
	}
	LookInFlight(Was);
	if (!bFlying)
	{
		if (Line.bEmbeds)
		{
			Embed();
		}
		else
		{
			FallAge = 0.f;
		}
	}
}

void UEmberRailSpike::Pierce(ACreatureBase& Creature, float Along)
{
	++Pierced;
	const FVector At = Line.Start + Line.Direction * Along;
	Hurt(Creature, Damage * Random.FRandRange(0.9f, 1.1f), At, Line.Direction);
	LooterSound::PlayAt(&Creature, LooterSoundCue::Ember::RailSpikeHit, At, 1.f, Random.FRandRange(0.95f, 1.05f));
	if (!Creature.IsDead() && !IsBoss(Creature) && Shove > 0.f)
	{
		if (UCreatureKnockComponent* Knock = Creature.GetKnock())
		{
			Knock->Knock(Line.Direction, Shove, ShoveHeight, Dizzy);
		}
	}
	LookAtPierce(Creature, At);
}

void UEmberRailSpike::Embed()
{
	EmbedAge = 0.f;
	LooterSound::PlayAt(GetBodyWorld(), LooterSoundCue::Ember::RailSpikeEmbed, Line.End);
	LookAtEmbed();
}

bool UEmberRailSpike::Linger(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	StepSpike(Dt);
	const bool bShowing = LingerLook(Dt);
	return bFlying || bShowing;
}
