#include "Embers/Powers/EmberLastRites.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "Player/ViewKick.h"
#include "UI/World/EmberMarkActor.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName StrikeName(TEXT("Strike"));
	/** The power's Inked icon (create_ember_assets.py's IconName): the sigil over the marked one. */
	const FName SigilIcon(TEXT("LastRites"));

	/** The pale wash on the marked body (the hit flash's overlay at this strength). */
	constexpr float PaleIntensity = 0.35f;
	/** The burst's ring racing out, and the strike's column of light, last this long. */
	constexpr float RingSeconds = 0.35f;
	constexpr float ColumnSeconds = 0.45f;
	constexpr float ColumnHeight = 900.f;
	/** The burst throws its pack back this far (cm; arc top), dazed a moment. */
	constexpr float BurstThrow = 220.f;
	constexpr float BurstThrowHeight = 70.f;
	constexpr float BurstDaze = 0.5f;
}

EEmberDenied UEmberLastRites::CanActivate(const FEmberContext& Context) const
{
	if (!Context.Character)
	{
		return EEmberDenied::Refused;
	}
	if (bMarking)
	{
		return EEmberDenied::Active;
	}
	const float Reach = Context.Definition && Context.Definition->Range > 0.f ? Context.Definition->Range : DefaultRange;
	return FindInSights(*Context.Character, Reach, SightsDegrees) ? EEmberDenied::None : EEmberDenied::Refused;
}

void UEmberLastRites::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	if (!Character || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	const float Reach = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	BurstDamage = ScaledDamage(Context, DefaultDamage);
	StrikeDamage = (Def ? Def->Extra(StrikeName, DefaultStrike) : DefaultStrike) * LevelScale;
	Radius = Def && Def->Radius > 0.f ? Def->Radius : DefaultRadius;
	Seconds = Def && Def->DurationSeconds > 0.f ? Def->DurationSeconds : 8.f;
	Owner = Context.Component;
	Marked = FindInSights(*Character, Reach, SightsDegrees);
	Time = 0.f;
	bBurst = bStruck = false;
	ACreatureBase* Creature = Marked.Get();
	bMarking = Creature != nullptr;
	if (!Creature)
	{
		return;
	}
	LastAt = Creature->GetActorLocation();

	// Its sigil over it with the seconds it has, its body washed pale.
	Sigil = AEmberMarkActor::Mark(Creature, Color, SigilIcon);
	if (AEmberMarkActor* Tag = Sigil.Get())
	{
		Tag->SetShowsDistance(false);
		Tag->SetCountdown(Seconds);
	}
	if (!PaleLook)
	{
		PaleLook = EmberFX::FEmberTint::MakeMaterial(this, Color);
	}
	Pale.SetMaterial(PaleLook);
	Pale.SetIntensity(PaleIntensity);
	Pale.Wear(*Creature);

	LooterSound::Play2D(Character, LooterSoundCue::Ember::LastRitesMark);
	// The mark falling on it: a thread of pale light dropping onto it, motes off it.
	ColumnAt = Creature->GetActorLocation();
	ColumnAge = ColumnSeconds * 0.5f;
	for (int32 Index = 0; Index < 14; ++Index)
	{
		Sparks.Spawn(Creature->GetActorLocation() + Random.GetUnitVector() * 30.f, FVector(0.0, 0.0, Random.FRandRange(60.f, 120.f)),
			Random.FRandRange(0.6f, 1.f), 1.6f, Color, 16.f, -20.f, 0.6f);
	}
	FViewKick Bow;
	Bow.Pitch = -0.35f;
	Bow.Frequency = 3.5f;
	Bow.Damping = 0.75f;
	Kick(Bow);
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Last Rites: %s marked for %.0f s (burst %.1f within %.0f cm, else strike %.1f)"), *Creature->GetName(), Seconds,
		BurstDamage, Radius, StrikeDamage);
}

void UEmberLastRites::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	// Settled by a death: the power is done now (the HUD's arc stops with it).
	if (StepMark(DeltaSeconds) && bBurst)
	{
		if (UEmberComponent* Embers = Owner.Get())
		{
			Embers->Finish();
		}
	}
}

void UEmberLastRites::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bMarking)
	{
		return;
	}
	ACreatureBase* Creature = Marked.Get();
	const UWorld* World = GetBodyWorld();
	const bool bShow = World && !World->bIsTearingDown;
	if (!bInterrupted && bShow)
	{
		// The time is up: the one that lived is struck (one that died between frames still bursts).
		if (Creature && !Creature->IsDead())
		{
			Strike(*Creature);
		}
		else
		{
			Burst(Creature ? Creature->GetActorLocation() : LastAt);
		}
	}
	ClearMark();
}

bool UEmberLastRites::StepMark(float DeltaSeconds)
{
	if (!bMarking)
	{
		return true;
	}
	Time += FMath::Max(DeltaSeconds, 0.f);
	ACreatureBase* Creature = Marked.Get();
	if (!Creature || Creature->IsDead())
	{
		Burst(Creature ? Creature->GetActorLocation() : LastAt);
		ClearMark();
		return true;
	}
	LastAt = Creature->GetActorLocation();
	if (AEmberMarkActor* Tag = Sigil.Get())
	{
		Tag->SetCountdown(FMath::Max(Seconds - Time, 0.f));
	}
	// A hit flash may have held its body a moment: the pale wash goes back on once it's free.
	Pale.Wear(*Creature);
	return false;
}

void UEmberLastRites::ClearMark()
{
	bMarking = false;
	Pale.RemoveAll();
	if (AEmberMarkActor* Tag = Sigil.Get())
	{
		Tag->Retire();
	}
	Sigil.Reset();
}

// ---------------------------------------------------------------------------
// Settled
// ---------------------------------------------------------------------------

void UEmberLastRites::Burst(const FVector& At)
{
	bBurst = true;
	UWorld* World = GetBodyWorld();
	if (!World)
	{
		return;
	}
	const ACreatureBase* Dead = Marked.Get();
	// Its soul on its pack: everyone near it, in sight of the burst, hurt and thrown back.
	const FVector Heart = At + FVector(0.0, 0.0, 30.0);
	int32 Hit = 0;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Other = *It;
		if (!Other || Other == Dead || Other->IsDead())
		{
			continue;
		}
		FVector Middle;
		float BodyRadius = 0.f;
		float HalfHeight = 0.f;
		BodyOf(*Other, Middle, BodyRadius, HalfHeight);
		if (FVector::Dist(Heart, Middle) - BodyRadius > Radius || !CanSee(*World, Heart, Middle, Other))
		{
			continue;
		}
		const FVector Away = (Middle - Heart).GetSafeNormal2D();
		Hurt(*Other, BurstDamage * Random.FRandRange(0.9f, 1.1f), Middle - Away * BodyRadius, Away);
		++Hit;
		if (!Other->IsDead() && !IsBoss(*Other))
		{
			if (UCreatureKnockComponent* Knock = Other->GetKnock())
			{
				Knock->Knock(Away, BurstThrow, BurstThrowHeight, BurstDaze);
			}
		}
	}
	LooterSound::PlayAt(World, LooterSoundCue::Ember::LastRitesBurst, Heart);
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnDeathBurst(EDeathBurst::SoulLight, Heart, FVector::UpVector, 130.f, Color);
	}
	for (int32 Index = 0; Index < 36; ++Index)
	{
		const float Angle = 2.f * UE_PI * Index / 36.f;
		const FVector Out = FVector(FMath::Cos(Angle), FMath::Sin(Angle), Random.FRandRange(0.f, 0.3f)).GetSafeNormal();
		Sparks.Spawn(Heart, Out * Random.FRandRange(900.f, 1300.f), Random.FRandRange(0.35f, 0.55f), 2.f, Color, 22.f, 0.f, 3.5f);
	}
	RingAt = Dead ? FeetOf(*Dead) + FVector(0.0, 0.0, 10.0) : At;
	RingAge = 0.f;
	FViewKick Shock;
	Shock.Pitch = 0.4f;
	Shock.FieldOfView = 0.02f;
	Shock.Frequency = 7.f;
	Shock.Damping = 0.6f;
	Kick(Shock);
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Last Rites: the soul burst on %d of its pack"), Hit);
}

void UEmberLastRites::Strike(ACreatureBase& Creature)
{
	bStruck = true;
	const FVector Middle = Creature.GetActorLocation();
	Hurt(Creature, StrikeDamage * Random.FRandRange(0.9f, 1.1f), Middle + FVector(0.0, 0.0, 20.0), -FVector::UpVector);
	LooterSound::PlayAt(&Creature, LooterSoundCue::Ember::LastRitesStrike, Middle);
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnFlash(Middle + FVector(0.0, 0.0, 40.0), -FVector::UpVector, 1.6f);
		const FVector Feet = FeetOf(Creature);
		const FLinearColor Dust = EmberFX::DustColor(&Creature);
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const float Angle = 2.f * UE_PI * Index / 10.f;
			const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			FX->SpawnDustPuff(Feet + Out * 30.f + FVector(0.0, 0.0, 10.0), Out * Random.FRandRange(220.f, 360.f) + FVector(0.0, 0.0, 50.0), Dust, 0.45f, 22.f,
				Random.FRandRange(90.f, 130.f), Random.FRandRange(0.8f, 1.2f));
		}
	}
	for (int32 Index = 0; Index < 20; ++Index)
	{
		const FVector Out = (Random.GetUnitVector() + FVector(0.0, 0.0, 0.6)).GetSafeNormal();
		Sparks.Spawn(Middle, Out * Random.FRandRange(300.f, 650.f), Random.FRandRange(0.25f, 0.5f), 1.6f, Color, 24.f, 600.f, 2.f);
	}
	ColumnAt = Middle;
	ColumnAge = 0.f;
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Last Rites: %s lived, and the mark struck it for %.1f"), *Creature.GetName(), StrikeDamage);
}

bool UEmberLastRites::Linger(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	FWeaponFX* FX = Effects();
	const ACreatureBase* Creature = Marked.Get();
	if (FX && bMarking && Creature)
	{
		// The ring at its feet, turning: twelve dashes, every other one lit.
		const float Size = FMath::Max(Creature->GetSizeScale(), 0.5f);
		const FVector Feet = FeetOf(*Creature) + FVector(0.0, 0.0, 8.0);
		const float Turn = Time * 1.6f;
		const float Ring = 70.f * Size;
		for (int32 Index = 0; Index < 12; Index += 2)
		{
			const float A0 = Turn + 2.f * UE_PI * Index / 12.f;
			const float A1 = Turn + 2.f * UE_PI * (Index + 1) / 12.f;
			FX->AddStreak(Feet + FVector(FMath::Cos(A0), FMath::Sin(A0), 0.f) * Ring, Feet + FVector(FMath::Cos(A1), FMath::Sin(A1), 0.f) * Ring, 2.4f * Size,
				Color, 9.f);
		}
	}
	if (FX && RingAge >= 0.f)
	{
		// The burst's ring racing out to its reach, fading as it goes.
		RingAge += Dt;
		const float U = FMath::Clamp(RingAge / RingSeconds, 0.f, 1.f);
		const float Out = Radius * (1.f - FMath::Square(1.f - U));
		constexpr int32 Pieces = 32;
		for (int32 Index = 0; Index < Pieces; ++Index)
		{
			const float A0 = 2.f * UE_PI * Index / Pieces;
			const float A1 = 2.f * UE_PI * (Index + 1) / Pieces;
			FX->AddStreak(RingAt + FVector(FMath::Cos(A0), FMath::Sin(A0), 0.f) * Out, RingAt + FVector(FMath::Cos(A1), FMath::Sin(A1), 0.f) * Out, 5.f, Color,
				20.f * (1.f - U));
		}
		if (U >= 1.f)
		{
			RingAge = -1.f;
		}
	}
	if (FX && ColumnAge >= 0.f)
	{
		// The column of light down onto it: a wide pale beam with a white core, gone in a breath.
		ColumnAge += Dt;
		const float Fade = 1.f - FMath::Clamp(ColumnAge / ColumnSeconds, 0.f, 1.f);
		const FVector Top = ColumnAt + FVector(0.0, 0.0, ColumnHeight);
		FX->AddStreak(Top, ColumnAt, 26.f * Fade + 4.f, Color, 14.f * Fade);
		FX->AddStreak(Top, ColumnAt, 7.f * Fade + 1.f, FLinearColor::White, 30.f * Fade);
		if (Fade <= 0.f)
		{
			ColumnAge = -1.f;
		}
	}
	Sparks.Tick(Dt, FX);
	return bMarking || RingAge >= 0.f || ColumnAge >= 0.f || !Sparks.IsEmpty();
}
