#include "Embers/Powers/EmberRailSpike.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Embers/EmberDefinition.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
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

	/** The spike: white-hot iron in a blue-white streak, its length and width (cm). */
	const FLinearColor SpikeHot(0.9f, 0.95f, 1.f);
	const FLinearColor IronGlow(1.f, 0.45f, 0.12f);
	constexpr float SpikeLength = 90.f;
	constexpr float SpikeWidth = 4.5f;
	/** It joins the aim line from the forearm over this far (cm), as a tracer does. */
	constexpr float ConvergeDistance = 600.f;
	/** A breath of steam laid along its way every so far (cm). */
	constexpr float SteamEvery = 70.f;
	/** The arc of a shove (cm). */
	constexpr float ShoveHeight = 40.f;
	/**
	 * The line it leaves glowing along its way, fading and spreading, and the smoke thread left hanging: it flies 25 m in a
	 * few frames, so without them the shot was over before it was seen (2026-10-20 play check).
	 */
	constexpr float TrailSeconds = 0.4f;
	constexpr float TrailWidth = 2.5f;
	constexpr float TrailGlow = 9.f;
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

	// The piston's bang: steam bursting off the forearm, a hard kick back.
	LooterSound::Play2D(Character, LooterSoundCue::Ember::RailSpikeFire);
	if (FWeaponFX* FX = Effects())
	{
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector Out = (-Line.Direction * 0.4 + Random.GetUnitVector() + FVector(0.0, 0.0, 0.5)).GetSafeNormal();
			FX->SpawnWisp(VisualStart, Out * Random.FRandRange(120.f, 260.f), 0.3f, 6.f, Random.FRandRange(35.f, 60.f), Random.FRandRange(0.6f, 1.f));
		}
		FX->SpawnFlash(VisualStart + Line.Direction * 10.f, Line.Direction, 0.6f);
	}
	FViewKick Recoil;
	Recoil.Pitch = 1.1f;
	Recoil.Yaw = 0.3f;
	Recoil.FieldOfView = 0.02f;
	Recoil.Frequency = 11.f;
	Recoil.Damping = 0.55f;
	Kick(Recoil);
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
	FWeaponFX* FX = Effects();
	const FVector Offset = VisualStart - Line.Start;
	auto SeenAt = [this, &Offset](float Distance)
	{
		return Line.Start + Line.Direction * Distance + Offset * (1.f - FMath::SmoothStep(0.f, ConvergeDistance, Distance));
	};
	if (FX)
	{
		// The spike itself, white-hot, and a breath of steam laid behind it.
		const FVector Head = SeenAt(Flown);
		const FVector Tail = SeenAt(FMath::Max(Flown - SpikeLength, 0.f));
		FX->AddStreak(SeenAt(FMath::Max(Was - SpikeLength, 0.f)), Head, SpikeWidth * 0.6f, IronGlow, 10.f);
		FX->AddStreak(Tail, Head, SpikeWidth, SpikeHot, 34.f);
		while (SteamLaid + SteamEvery <= Flown)
		{
			SteamLaid += SteamEvery;
			FX->SpawnWisp(SeenAt(SteamLaid), Random.GetUnitVector() * 25.f + FVector(0.0, 0.0, 15.0), 0.16f, 5.f, Random.FRandRange(26.f, 40.f),
				Random.FRandRange(0.5f, 0.8f));
		}
	}
	if (Flown >= Line.Length)
	{
		bFlying = false;
		if (FX)
		{
			const FVector Joined = SeenAt(FMath::Min(ConvergeDistance, Flown));
			FX->AddThread(VisualStart, Joined, 3.f, 0.22f, 1.4f);
			if (Flown > ConvergeDistance)
			{
				FX->AddThread(Joined, SeenAt(Flown), 3.f, 0.22f, 1.4f);
			}
		}
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
	// Through and out the far side: sparks and steam thrown on along the line.
	for (int32 Index = 0; Index < 12; ++Index)
	{
		const FVector Out = (Line.Direction + Random.GetUnitVector() * 0.6).GetSafeNormal();
		Sparks.Spawn(At, Out * Random.FRandRange(300.f, 700.f), Random.FRandRange(0.15f, 0.35f), 1.4f, IronGlow, 24.f, 700.f, 2.f);
	}
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnWisp(At, Line.Direction * 80.f + FVector(0.0, 0.0, 20.0), 0.3f, 8.f, 40.f, 0.7f);
	}
	FViewKick Punch;
	Punch.Pitch = 0.25f;
	Punch.Frequency = 12.f;
	Punch.Damping = 0.6f;
	Kick(Punch);
}

void UEmberRailSpike::Embed()
{
	EmbedAge = 0.f;
	LooterSound::PlayAt(GetBodyWorld(), LooterSoundCue::Ember::RailSpikeEmbed, Line.End);
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnRicochet(Line.End, Line.EmbedNormal, 1.2f);
		const FLinearColor Dust = EmberFX::DustColor(Body.Get());
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector Out = (Line.EmbedNormal + Random.GetUnitVector() * 0.7).GetSafeNormal();
			FX->SpawnDustPuff(Line.End + Out * 10.f, Out * Random.FRandRange(120.f, 260.f), Dust, 0.4f, 14.f, Random.FRandRange(50.f, 80.f),
				Random.FRandRange(0.6f, 1.f));
			FX->SpawnGrit(Line.End, Out * Random.FRandRange(250.f, 450.f), Random.FRandRange(0.8f, 1.6f), Random.FRandRange(0.4f, 0.7f));
		}
	}
	for (int32 Index = 0; Index < 16; ++Index)
	{
		const FVector Out = (Line.EmbedNormal + Random.GetUnitVector()).GetSafeNormal();
		Sparks.Spawn(Line.End, Out * Random.FRandRange(250.f, 600.f), Random.FRandRange(0.2f, 0.5f), 1.4f, IronGlow, 26.f, 800.f, 2.f);
	}
}

bool UEmberRailSpike::Linger(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	StepSpike(Dt);
	FWeaponFX* FX = Effects();
	if (TrailAge >= 0.f)
	{
		// The way it came, glowing and spreading as it fades (straight from the forearm to where it joined the aim line).
		TrailAge += Dt;
		const float Fade = 1.f - FMath::Clamp(TrailAge / TrailSeconds, 0.f, 1.f);
		if (FX && Fade > 0.f && Flown > 0.f)
		{
			const FVector Joined = Line.Start + Line.Direction * FMath::Min(ConvergeDistance, Flown);
			const float Width = TrailWidth * (1.f + 1.5f * (1.f - Fade));
			const float Glow = TrailGlow * Fade * Fade;
			FX->AddStreak(VisualStart, Joined, Width, IronGlow, Glow);
			if (Flown > ConvergeDistance)
			{
				FX->AddStreak(Joined, Line.Start + Line.Direction * Flown, Width, IronGlow, Glow);
			}
		}
		if (Fade <= 0.f)
		{
			TrailAge = -1.f;
		}
	}
	if (EmbedAge >= 0.f)
	{
		// Stuck in the world, half its length showing, cooling from white to a dull red, a wisp off it now and then.
		EmbedAge += Dt;
		const float Cool = FMath::Clamp(EmbedAge / EmbedSeconds, 0.f, 1.f);
		if (FX)
		{
			const FVector Out = Line.End - Line.Direction * (SpikeLength * 0.5f);
			FX->AddStreak(Out, Line.End, SpikeWidth * 0.8f, FMath::Lerp(SpikeHot, IronGlow, FMath::Min(Cool * 2.f, 1.f)), 26.f * (1.f - Cool) + 2.f);
			if (Random.FRand() < Dt * 3.f)
			{
				FX->SpawnWisp(Out, FVector(0.0, 0.0, 30.0), 0.12f * (1.f - Cool), 4.f, 18.f, 0.8f);
			}
		}
		if (Cool >= 1.f)
		{
			EmbedAge = -1.f;
		}
	}
	if (FallAge >= 0.f)
	{
		// Its reach spent in open air: it tumbles down out of sight, cooling.
		FallAge += Dt;
		if (FX)
		{
			const FVector At = Line.End + FVector(0.0, 0.0, -490.f * FallAge * FallAge);
			const FVector Turn = Line.Direction.RotateAngleAxis(FallAge * 540.f, FVector::CrossProduct(Line.Direction, FVector::UpVector).GetSafeNormal());
			FX->AddStreak(At - Turn * (SpikeLength * 0.5f), At + Turn * (SpikeLength * 0.5f), SpikeWidth * 0.8f, IronGlow, 18.f * FMath::Max(1.f - FallAge, 0.f));
		}
		if (FallAge >= 1.f)
		{
			FallAge = -1.f;
		}
	}
	Sparks.Tick(Dt, FX);
	return bFlying || TrailAge >= 0.f || EmbedAge >= 0.f || FallAge >= 0.f || !Sparks.IsEmpty();
}
