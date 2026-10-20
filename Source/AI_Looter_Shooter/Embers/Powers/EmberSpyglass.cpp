#include "Embers/Powers/EmberSpyglass.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberDefinition.h"
#include "Player/PlayerViewComponent.h"
#include "Player/ViewKick.h"
#include "UI/World/EmberMarkActor.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

const FName UEmberSpyglass::ScaleSource(TEXT("Spyglass"));

namespace
{
	/** The sweep's ring: the lens's cyan glint, its points round, how wide and bright. */
	const FLinearColor LensCyan(0.45f, 0.9f, 1.f);
	constexpr int32 SweepPoints = 56;
	constexpr float SweepWidth = 9.f;
	constexpr float SweepGlow = 16.f;
	/** The orange wash on a marked body. */
	constexpr float WashIntensity = 0.3f;
	/** Pings no closer than this (seconds); creatures that come within reach later are looked for this often. */
	constexpr float PingEvery = 0.05f;
	constexpr float RecheckEvery = 0.5f;
	/** The lens comes in over this long, holds through the sweep, and clears over LensOut. */
	constexpr float LensIn = 0.2f;
	constexpr float LensOut = 0.6f;

	float Ease(float U)
	{
		const float X = FMath::Clamp(U, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}
}

void UEmberSpyglass::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	if (!Character || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Range = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	DamageScale = 1.f + (Def && Def->Strength > 0.f ? Def->Strength : DefaultStrength);
	Time = 0.f;
	SweepRadius = 0.f;
	PingClock = 0.f;
	Pings = 0;
	RecheckClock = RecheckEvery;
	Marked.Reset();
	Lensed.Reset();
	if (!MarkLook)
	{
		MarkLook = EmberFX::FEmberTint::MakeMaterial(this, Color);
	}
	Washed.SetMaterial(MarkLook);
	Washed.SetIntensity(WashIntensity);
	bLooking = true;

	LooterSound::Play2D(Character, LooterSoundCue::Ember::SpyglassStart);
	FViewKick Raise;
	Raise.Pitch = 0.3f;
	Raise.FieldOfView = -0.02f;
	Raise.Frequency = 4.f;
	Raise.Damping = 0.75f;
	Kick(Raise);
	UE_LOG(LogLooter, Log, TEXT("Spyglass: sweeping %.0f cm; marked take x%.2f"), Range, DamageScale);
}

void UEmberSpyglass::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	StepSight(DeltaSeconds);
}

void UEmberSpyglass::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bLooking)
	{
		return;
	}
	bLooking = false;
	const TArray<FMarked> All = Marked;
	for (const FMarked& Each : All)
	{
		if (ACreatureBase* Creature = Each.Creature.Get())
		{
			Unmark(*Creature, false);
		}
		else if (AEmberMarkActor* Tag = Each.Mark.Get())
		{
			Tag->Retire();
		}
	}
	Marked.Reset();
	Washed.RemoveAll();
	SetLens(0.f);
	const UWorld* World = GetBodyWorld();
	if (World && !World->bIsTearingDown)
	{
		LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::SpyglassEnd);
	}
	UE_LOG(LogLooter, Log, TEXT("Spyglass: shut%s"), bInterrupted ? TEXT(" (cut short)") : TEXT(""));
}

bool UEmberSpyglass::IsMarked(const ACreatureBase& Creature) const
{
	return Marked.ContainsByPredicate([&Creature](const FMarked& Each) { return Each.Creature.Get() == &Creature; });
}

// ---------------------------------------------------------------------------
// The sight, frame by frame
// ---------------------------------------------------------------------------

void UEmberSpyglass::StepSight(float DeltaSeconds)
{
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!bLooking || !World)
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	Time += Dt;
	PingClock -= Dt;
	const FVector Origin = FeetOf(*Character);
	const bool bSweeping = SweepRadius < Range;
	if (bSweeping)
	{
		// The ring runs out fast and eases at its reach.
		SweepRadius = Range * (1.f - FMath::Square(1.f - FMath::Clamp(Time / SweepSeconds, 0.f, 1.f)));
		if (Time >= SweepSeconds)
		{
			SweepRadius = Range;
		}
	}

	// Marked as the ring reaches them (and, once it has run its reach, any that comes within it).
	RecheckClock -= Dt;
	if (bSweeping || RecheckClock <= 0.f)
	{
		RecheckClock = RecheckEvery;
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			ACreatureBase* Creature = *It;
			if (Creature && !Creature->IsDead() && !Creature->IsHidden() && FVector::Dist2D(Creature->GetActorLocation(), Origin) <= SweepRadius
				&& !IsMarked(*Creature))
			{
				Mark(*Creature);
			}
		}
	}
	// The dead let go of their marks (and of the extra damage, before anything brings them back).
	for (int32 Index = Marked.Num() - 1; Index >= 0; --Index)
	{
		ACreatureBase* Creature = Marked[Index].Creature.Get();
		if (!Creature || Creature->IsDead())
		{
			if (Creature)
			{
				Unmark(*Creature, false);
			}
			else if (AEmberMarkActor* Tag = Marked[Index].Mark.Get())
			{
				Tag->Retire();
			}
			Marked.RemoveAt(Index);
			continue;
		}
		// A hit flash may have held its body a moment: the wash goes back on once it's free.
		Washed.Wear(*Creature);
	}

	// The lens through the sweep, then clearing; the ring on the ground while it runs.
	const float LensNow = Time < LensIn ? Ease(Time / LensIn) : (Time < SweepSeconds ? 1.f : 1.f - Ease((Time - SweepSeconds) / LensOut));
	SetLens(LensNow);
	FWeaponFX* FX = bSweeping ? Effects() : nullptr;
	if (FX && SweepRadius > 50.f)
	{
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SpyglassSweep"), Character);
		TArray<FVector> Ring;
		Ring.Reserve(SweepPoints);
		for (int32 Index = 0; Index < SweepPoints; ++Index)
		{
			const float Angle = 2.f * UE_PI * Index / SweepPoints;
			FVector Point = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * SweepRadius;
			FHitResult Ground;
			if (World->LineTraceSingleByObjectType(Ground, Point + FVector(0.0, 0.0, 1500.0), Point - FVector(0.0, 0.0, 3000.0),
					FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				Point.Z = Ground.ImpactPoint.Z;
			}
			Ring.Add(Point + FVector(0.0, 0.0, 12.0));
		}
		const float Fade = 1.f - FMath::Clamp((SweepRadius / Range - 0.85f) / 0.15f, 0.f, 1.f);
		EmberFX::DrawLine(*FX, Ring, true, SweepWidth * (1.f + SweepRadius / Range), LensCyan, SweepGlow * Fade);
	}
}

void UEmberSpyglass::Mark(ACreatureBase& Creature)
{
	FMarked& Entry = Marked.AddDefaulted_GetRef();
	Entry.Creature = &Creature;
	Entry.Mark = AEmberMarkActor::Mark(&Creature, Color);
	if (UHealthComponent* Life = Creature.FindComponentByClass<UHealthComponent>())
	{
		Life->SetDamageTakenScale(ScaleSource, DamageScale);
	}
	Washed.Wear(Creature);
	// A soft ping as each is found, a little higher with each one.
	if (PingClock <= 0.f)
	{
		PingClock = PingEvery;
		LooterSound::Play2D(Body.Get(), LooterSoundCue::Ember::SpyglassMark, 0.6f, FMath::Min(1.f + 0.025f * Pings, 1.3f));
		++Pings;
	}
}

void UEmberSpyglass::Unmark(ACreatureBase& Creature, bool bRemoveFromList)
{
	if (UHealthComponent* Life = Creature.FindComponentByClass<UHealthComponent>())
	{
		Life->SetDamageTakenScale(ScaleSource, 1.f);
	}
	Washed.Remove(Creature);
	for (int32 Index = Marked.Num() - 1; Index >= 0; --Index)
	{
		if (Marked[Index].Creature.Get() != &Creature)
		{
			continue;
		}
		if (AEmberMarkActor* Tag = Marked[Index].Mark.Get())
		{
			Tag->Retire();
		}
		if (bRemoveFromList)
		{
			Marked.RemoveAt(Index);
		}
	}
}

void UEmberSpyglass::SetLens(float Amount)
{
	const float Wanted = FMath::Clamp(Amount, 0.f, 1.f);
	ACharacter* Character = Body.Get();
	if (FMath::IsNearlyEqual(Wanted, Lens) && (Wanted > 0.f || Lensed.IsEmpty()))
	{
		return;
	}
	Lens = Wanted;
	if (UPlayerViewComponent* View = Character ? Character->FindComponentByClass<UPlayerViewComponent>() : nullptr)
	{
		View->SetPowerFieldOfView(-LensDegrees * Lens);
	}
	if (Lens > 0.f && Lensed.IsEmpty() && Character)
	{
		// Every camera of the player's (first person and the shoulder's), each with its own setting kept.
		TInlineComponentArray<UCameraComponent*> Cameras(Character);
		for (UCameraComponent* Camera : Cameras)
		{
			FLensCamera& Kept = Lensed.AddDefaulted_GetRef();
			Kept.Camera = Camera;
			Kept.bHadOverride = Camera->PostProcessSettings.bOverride_VignetteIntensity;
			Kept.Vignette = Camera->PostProcessSettings.VignetteIntensity;
		}
	}
	for (const FLensCamera& Kept : Lensed)
	{
		UCameraComponent* Camera = Kept.Camera.Get();
		if (!Camera)
		{
			continue;
		}
		const float Before = Kept.bHadOverride ? Kept.Vignette : 0.4f;
		Camera->PostProcessSettings.bOverride_VignetteIntensity = true;
		Camera->PostProcessSettings.VignetteIntensity = FMath::Lerp(Before, LensVignette, Lens);
	}
	if (Lens <= 0.f)
	{
		// Clear: every camera as it was.
		for (const FLensCamera& Kept : Lensed)
		{
			if (UCameraComponent* Camera = Kept.Camera.Get())
			{
				Camera->PostProcessSettings.bOverride_VignetteIntensity = Kept.bHadOverride;
				Camera->PostProcessSettings.VignetteIntensity = Kept.Vignette;
			}
		}
		Lensed.Reset();
	}
}
