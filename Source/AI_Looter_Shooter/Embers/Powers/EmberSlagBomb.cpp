#include "Embers/Powers/EmberSlagBomb.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberPropActor.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const FName InnerRadiusName(TEXT("InnerRadius"));
	const FName OuterDamageName(TEXT("OuterDamage"));
	const FName BurnDamageName(TEXT("BurnDamage"));
	const FName BurnIntervalName(TEXT("BurnInterval"));
	const FName BurnLingerName(TEXT("BurnLinger"));
	const FName PoolSecondsName(TEXT("PoolSeconds"));

	/** The slag's colours: white-hot, molten (the look's cooling crust is EmberSlagBombLook.cpp's). */
	const FLinearColor WhiteHot(1.f, 0.85f, 0.55f);
	const FLinearColor Molten(1.f, 0.42f, 0.05f);
	/** The burn's orange on a body, and its flare on each burn. */
	const FLinearColor BurnOrange(1.f, 0.38f, 0.06f);
	constexpr float BurnTintBase = 0.35f;
	constexpr float BurnTintFlare = 0.9f;
	/** The pool laid out: hot blobs on it, its edge's points (drawn in EmberSlagBombLook.cpp). */
	constexpr int32 BlobCount = 36;
	constexpr int32 EdgePoints = 40;

	/** The flight's length and the arc's top for a lob Distance cm long: quick near, a little longer and higher far. */
	float FlightFor(float Distance)
	{
		return FMath::Clamp(0.45f + 0.025f * Distance / 100.f, 0.45f, 1.f);
	}
	float ApexFor(float Distance)
	{
		return 120.f + 0.12f * Distance;
	}
}

bool UEmberSlagBomb::FindLanding(const ACharacter& Character, float Range, FVector& OutLanding)
{
	const UWorld* World = Character.GetWorld();
	if (!World)
	{
		return false;
	}
	// The creature in its sights: onto its feet.
	if (const ACreatureBase* Creature = FindInSights(Character, Range, 5.f))
	{
		FVector Middle;
		float BodyRadius = 0.f;
		float HalfHeight = 0.f;
		BodyOf(*Creature, Middle, BodyRadius, HalfHeight);
		if (FindGround(*World, Middle, 50.f, HalfHeight + 200.f, OutLanding, nullptr, Creature))
		{
			return true;
		}
		OutLanding = Middle - FVector(0.0, 0.0, HalfHeight);
		return true;
	}
	// Where the aim meets the world (the ground under it, for a wall); else its full reach, down onto the ground.
	FVector Eye;
	FVector Aim;
	GetAim(Character, Eye, Aim);
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SlagBombAim"), &Character);
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Eye, Eye + Aim * Range, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		if (Hit.ImpactNormal.Z >= 0.6)
		{
			OutLanding = Hit.ImpactPoint;
			return true;
		}
		return FindGround(*World, Hit.ImpactPoint + Hit.ImpactNormal * 40.f, 20.f, 800.f, OutLanding, nullptr, &Character);
	}
	return FindGround(*World, Eye + Aim * Range, 300.f, 4000.f, OutLanding, nullptr, &Character);
}

EEmberDenied UEmberSlagBomb::CanActivate(const FEmberContext& Context) const
{
	if (!Context.Character)
	{
		return EEmberDenied::Refused;
	}
	if (bFlying || PoolLeft > 0.f)
	{
		return EEmberDenied::Active;
	}
	const float Reach = Context.Definition && Context.Definition->Range > 0.f ? Context.Definition->Range : DefaultRange;
	FVector Where;
	return FindLanding(*Context.Character, Reach, Where) ? EEmberDenied::None : EEmberDenied::Refused;
}

void UEmberSlagBomb::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	if (!Character || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	const float Reach = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	InnerDamage = ScaledDamage(Context, DefaultDamage);
	OuterDamage = (Def ? Def->Extra(OuterDamageName, DefaultOuterDamage) : DefaultOuterDamage) * LevelScale;
	Radius = Def && Def->Radius > 0.f ? Def->Radius : DefaultRadius;
	InnerRadius = FMath::Min(Def ? Def->Extra(InnerRadiusName, DefaultInnerRadius) : DefaultInnerRadius, Radius);
	BurnDamage = (Def ? Def->Extra(BurnDamageName, DefaultBurnDamage) : DefaultBurnDamage) * LevelScale;
	BurnInterval = FMath::Max(Def ? Def->Extra(BurnIntervalName, DefaultBurnInterval) : DefaultBurnInterval, 0.1f);
	BurnLinger = FMath::Max(Def ? Def->Extra(BurnLingerName, DefaultBurnLinger) : DefaultBurnLinger, 0.f);
	PoolSeconds = FMath::Max(Def ? Def->Extra(PoolSecondsName, DefaultPoolSeconds) : DefaultPoolSeconds, 0.5f);
	if (!FindLanding(*Character, Reach, Landing))
	{
		return;
	}
	// Lobbed from the firebox in its belly: low and out in front.
	From = HandPoint(*Character, 0.f, 45.f, 40.f);
	const float Distance = static_cast<float>(FVector::Dist(From, Landing));
	Flight = FlightFor(Distance);
	Apex = ApexFor(Distance);
	FlightTime = 0.f;
	BombAt = From;
	// A pool still burning out on its own from the last lob goes now.
	if (PoolLeft > 0.f)
	{
		EndPool();
	}
	bPoolOnLinger = false;
	bFlying = true;
	PoolLeft = 0.f;
	Burns = 0;
	LastInPool.Reset();
	RingAge = -1.f;

	LooterSound::Play2D(Character, LooterSoundCue::Ember::SlagBombThrow);
	FViewKick Heave;
	Heave.Pitch = 0.6f;
	Heave.FieldOfView = 0.01f;
	Heave.Frequency = 7.f;
	Heave.Damping = 0.6f;
	Kick(Heave);
	// The firebox's door thrown open: a gout of sparks.
	for (int32 Index = 0; Index < 14; ++Index)
	{
		const FVector Out = ((Landing - From).GetSafeNormal() + Random.GetUnitVector() * 0.7).GetSafeNormal();
		Sparks.Spawn(From, Out * Random.FRandRange(200.f, 500.f), Random.FRandRange(0.25f, 0.5f), 1.5f, Molten, 24.f, 600.f, 2.f);
	}
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Slag Bomb: lobbed %.0f cm in %.2f s; %.1f within %.0f, %.1f within %.0f, then %.1f every %.1f s for %.0f s"), Distance,
		Flight, InnerDamage, InnerRadius, OuterDamage, Radius, BurnDamage, BurnInterval, PoolSeconds);
}

void UEmberSlagBomb::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	StepBomb(DeltaSeconds);
}

void UEmberSlagBomb::End(const FEmberContext& Context, bool bInterrupted)
{
	if (bFlying)
	{
		// Still in the air when it's over: cut short, it's gone; its time up, it lands now.
		bFlying = false;
		if (!bInterrupted)
		{
			Burst(Landing);
		}
	}
	if (bInterrupted || PoolLeft <= 0.f)
	{
		EndPool();
		return;
	}
	// Its time up with slag still hot (a lob that landed late): the pool burns out its own time on the linger.
	bPoolOnLinger = true;
	StartLinger();
}

// ---------------------------------------------------------------------------
// The lob and the burst
// ---------------------------------------------------------------------------

void UEmberSlagBomb::StepBomb(float DeltaSeconds)
{
	UWorld* World = GetBodyWorld();
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	if (!World)
	{
		return;
	}
	if (bFlying)
	{
		FlightTime += Dt;
		const float U = FMath::Clamp(FlightTime / Flight, 0.f, 1.f);
		const FVector Next = FMath::Lerp(From, Landing, U) + FVector(0.0, 0.0, 4.f * Apex * U * (1.f - U));
		// Anything solid in its way bursts it early.
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SlagBombFlight"), Body.Get());
		FHitResult Hit;
		if (U < 0.98f && World->LineTraceSingleByObjectType(Hit, BombAt, Next, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			bFlying = false;
			FVector Ground = Hit.ImpactPoint;
			if (Hit.ImpactNormal.Z < 0.6)
			{
				FindGround(*World, Hit.ImpactPoint + Hit.ImpactNormal * 40.f, 20.f, 800.f, Ground, nullptr, Body.Get());
			}
			Burst(Ground);
			return;
		}
		// The ball: a white-hot core in a molten glow, a trail of sparks and smoke.
		if (FWeaponFX* FX = Effects())
		{
			const FVector Along = (Next - BombAt).GetSafeNormal();
			FX->AddStreak(Next - Along * 16.f, Next + Along * 4.f, 13.f, Molten, 30.f);
			FX->AddStreak(Next - Along * 8.f, Next + Along * 2.f, 7.f, WhiteHot, 42.f);
			if (Random.FRand() < 0.5f)
			{
				FX->SpawnWisp(Next, -Along * 40.f + FVector(0.0, 0.0, 20.0), 0.2f, 8.f, 30.f, 0.7f);
			}
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Sparks.Spawn(Next, -(Next - BombAt) / FMath::Max(Dt, 0.001f) * 0.1f + Random.GetUnitVector() * 120.f, Random.FRandRange(0.2f, 0.4f), 1.3f, Molten,
				22.f, 500.f, 2.f);
		}
		BombAt = Next;
		if (U >= 1.f)
		{
			bFlying = false;
			Burst(Landing);
		}
		return;
	}
	if (PoolLeft <= 0.f)
	{
		return;
	}
	PoolAge += Dt;
	PoolLeft -= Dt;

	// Who's in it this frame (a body passing between burns still counts), and the burns on their beat.
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!Creature || Creature->IsDead())
		{
			continue;
		}
		FVector Middle;
		float BodyRadius = 0.f;
		float HalfHeight = 0.f;
		BodyOf(*Creature, Middle, BodyRadius, HalfHeight);
		const float Size = FMath::Max(Creature->GetSizeScale(), 0.5f);
		const float FeetZ = static_cast<float>(Middle.Z) - HalfHeight;
		if (FVector::Dist2D(Middle, BurstAt) <= Radius + BodyRadius * 0.5f && FMath::Abs(FeetZ - BurstAt.Z) <= PoolHeight * Size)
		{
			LastInPool.Add(Creature, PoolAge);
		}
	}
	BurnClock -= Dt;
	if (BurnClock <= 0.f)
	{
		BurnClock += BurnInterval;
		BurnTick();
	}
	// The burn's orange fades from its flare between burns.
	Burning.SetIntensity(BurnTintBase + BurnTintFlare * FMath::Square(FMath::Clamp(BurnClock / BurnInterval, 0.f, 1.f)));
	DrawPool(Dt);
	if (PoolLeft <= 0.f)
	{
		EndPool();
	}
}

void UEmberSlagBomb::Burst(const FVector& At)
{
	UWorld* World = GetBodyWorld();
	if (!World)
	{
		return;
	}
	BurstAt = At;
	const FVector Heart = At + FVector(0.0, 0.0, 40.0);
	// The burst: near bodies take the most, the rest of its reach the less, measured to the body's nearest side.
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!Creature || Creature->IsDead())
		{
			continue;
		}
		FVector Middle;
		float BodyRadius = 0.f;
		float HalfHeight = 0.f;
		BodyOf(*Creature, Middle, BodyRadius, HalfHeight);
		const float Distance = FMath::Max(static_cast<float>(FVector::Dist(Heart, Middle)) - BodyRadius, 0.f);
		if (Distance > Radius || (!CanSee(*World, Heart, Middle, Creature) && !CanSee(*World, Heart, Middle + FVector(0.0, 0.0, HalfHeight * 0.8f), Creature)))
		{
			continue;
		}
		const float Damage = Distance <= InnerRadius ? InnerDamage : OuterDamage;
		Hurt(*Creature, Damage * Random.FRandRange(0.9f, 1.1f), Middle - (Middle - Heart).GetSafeNormal() * BodyRadius, (Middle - Heart).GetSafeNormal());
		LastInPool.Add(Creature, 0.f);
	}
	LooterSound::PlayAt(World, LooterSoundCue::Ember::SlagBombBurst, Heart);
	if (FWeaponFX* FX = Effects())
	{
		FX->SpawnFlash(Heart, FVector::UpVector, 2.4f);
		FX->SpawnDirt(At, FVector::UpVector, 1.f);
	}
	// Molten globs flung out, arcing down.
	for (int32 Index = 0; Index < 44; ++Index)
	{
		const FVector Out = (Random.GetUnitVector() * FVector(1.0, 1.0, 0.4) + FVector(0.0, 0.0, 0.7)).GetSafeNormal();
		Sparks.Spawn(Heart, Out * Random.FRandRange(350.f, 900.f), Random.FRandRange(0.5f, 1.f), Random.FRandRange(2.f, 4.f),
			Random.FRand() < 0.3f ? WhiteHot : Molten, 26.f, 980.f, 0.8f);
	}
	FViewKick Shock;
	Shock.Pitch = 0.5f;
	Shock.FieldOfView = 0.015f;
	Shock.Frequency = 8.f;
	Shock.Damping = 0.6f;
	Kick(Shock);

	// The pool: its edge and its hot blobs laid on the ground once, its bubbling on an anchor there.
	PoolLeft = PoolSeconds;
	PoolAge = 0.f;
	BurnClock = BurnInterval;
	RingAge = 0.f;
	Edge = EmberFX::GroundRing(*World, At, Radius, EdgePoints, 220.f, 4.f);
	Blobs.Reset();
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SlagBlobs"));
	for (int32 Index = 0; Index < BlobCount; ++Index)
	{
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		FVector Spot = At + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Radius * 0.9f * FMath::Sqrt(Random.FRand()));
		FHitResult Ground;
		if (World->LineTraceSingleByObjectType(Ground, Spot + FVector(0.0, 0.0, 200.0), Spot - FVector(0.0, 0.0, 220.0), FCollisionObjectQueryParams(ECC_WorldStatic),
				Params))
		{
			Spot.Z = Ground.ImpactPoint.Z;
		}
		Blobs.Add(Spot + FVector(0.0, 0.0, 3.0));
	}
	if (!BurnLook)
	{
		BurnLook = EmberFX::FEmberTint::MakeMaterial(this, BurnOrange);
	}
	Burning.SetMaterial(BurnLook);
	Anchor = AEmberPropActor::Place(World, nullptr, At, 0.f, FVector::OneVector);
	if (AEmberPropActor* Here = Anchor.Get())
	{
		Here->SetLight(Molten, 30.f, Radius * 1.6f, FVector(0.0, 0.0, 60.0));
		Bubbling = LooterSound::Start(Here, LooterSoundCue::Ember::SlagBombPool, Here->GetRootComponent());
	}
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Slag Bomb: burst at %s, the pool for %.0f s"), *At.ToCompactString(), PoolSeconds);
}

void UEmberSlagBomb::BurnTick()
{
	for (auto It = LastInPool.CreateIterator(); It; ++It)
	{
		ACreatureBase* Creature = It.Key().Get();
		if (!Creature || Creature->IsDead())
		{
			if (Creature)
			{
				Burning.Remove(*Creature);
			}
			It.RemoveCurrent();
			continue;
		}
		if (PoolAge - It.Value() > BurnLinger)
		{
			// Out of it long enough: the burning's over.
			Burning.Remove(*Creature);
			It.RemoveCurrent();
			continue;
		}
		const FVector Middle = Creature->GetActorLocation();
		Hurt(*Creature, BurnDamage * Random.FRandRange(0.9f, 1.1f), Middle, FVector::UpVector);
		++Burns;
		Burning.Wear(*Creature);
		LooterSound::PlayAt(Creature, LooterSoundCue::Ember::SlagBombSear, Middle, 0.7f, Random.FRandRange(0.94f, 1.06f));
		for (int32 Index = 0; Index < 5; ++Index)
		{
			Sparks.Spawn(Middle + Random.GetUnitVector() * 25.f, FVector(0.0, 0.0, Random.FRandRange(100.f, 220.f)) + Random.GetUnitVector() * 60.f,
				Random.FRandRange(0.3f, 0.55f), 1.2f, Molten, 20.f, 200.f, 1.5f);
		}
	}
}

void UEmberSlagBomb::EndPool()
{
	const bool bHadPool = PoolLeft > 0.f || Anchor.IsValid();
	PoolLeft = 0.f;
	Burning.RemoveAll();
	LastInPool.Reset();
	LooterSound::Stop(Bubbling.Get(), 0.5f);
	Bubbling.Reset();
	if (AEmberPropActor* Here = Anchor.Get())
	{
		Here->Remove();
	}
	Anchor.Reset();
	const UWorld* World = GetBodyWorld();
	FWeaponFX* FX = Effects();
	if (bHadPool && FX && World && !World->bIsTearingDown)
	{
		// Cooled to a crust: the last of its smoke.
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
			const FVector Spot = BurstAt + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Random.FRandRange(0.f, Radius * 0.8f);
			FX->SpawnWisp(Spot + FVector(0.0, 0.0, 10.0), FVector(0.0, 0.0, Random.FRandRange(30.f, 60.f)), 0.22f, 20.f, 90.f, Random.FRandRange(1.2f, 1.8f));
		}
	}
}

bool UEmberSlagBomb::Linger(float DeltaSeconds)
{
	if (bPoolOnLinger)
	{
		StepBomb(DeltaSeconds);
		bPoolOnLinger = PoolLeft > 0.f;
	}
	Sparks.Tick(DeltaSeconds, Effects());
	return bFlying || PoolLeft > 0.f || !Sparks.IsEmpty();
}
