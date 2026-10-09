#include "World/FaunaTumbleweeds.h"
#include "World/FaunaRules.h"
#include "World/WorldQueries.h"
#include "Audio/LooterSound.h"
#include "Components/BrushComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Volume.h"

namespace
{
	/** The longest step of a roll (s): longer updates are split, so a sweep never jumps a fence. */
	constexpr float MaxStep = 1.f / 30.f;

	constexpr float Gravity = 980.f;

	/** How quickly the wind brings it to its own speed (a share a second), and the ground slows it rolling. */
	constexpr float WindDrag = 1.1f;
	constexpr float GroundFriction = 0.35f;

	/** The share of gravity that pulls it down a slope. */
	constexpr float SlopePull = 0.6f;

	/** Its bounces along the ground: how fast it leaves (cm/s up) and how often (s). */
	constexpr float HopMin = 110.f;
	constexpr float HopMax = 240.f;
	constexpr float HopEveryMin = 0.35f;
	constexpr float HopEveryMax = 1.2f;

	/** A surface steeper than this (its normal's Z under it) is something to glance off, not ground to roll on. */
	constexpr float WallNormalZ = 0.6f;

	/** The turn (degrees either way) a glance adds, so it never bounces dead straight back. */
	constexpr float Deflect = 25.f;

	/** Slower than this (cm/s) for StuckSeconds, it's stuck against something and fades. */
	constexpr float StuckSpeed = 40.f;
	constexpr float StuckSeconds = 3.f;

	/** Its fading away (s), and how far past its lane's end it may roll first (cm). */
	constexpr float FadeSeconds = 1.f;
	constexpr float LaneOverrun = 1500.f;

	/** Its bounce sounds within this of a player (cm), no closer together than SoundCooldown (s). */
	constexpr float SoundRange = 2500.f;
	constexpr float SoundCooldownSeconds = 0.35f;
}

AFaunaTumbleweeds::AFaunaTumbleweeds()
{
	CullDistance = 9000.f;
}

void AFaunaTumbleweeds::BeginPlay()
{
	Super::BeginPlay();
	SetupTumbleweeds();
}

void AFaunaTumbleweeds::SetupTumbleweeds()
{
	if (bReady)
	{
		return;
	}
	bReady = true;
	Random.Initialize(Seed);
	Tumbles.SetNum(MaxActive);
	for (int32 Index = 0; Index < MaxActive; ++Index)
	{
		UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(this, FName(*FString::Printf(TEXT("Tumbleweed%d"), Index)), RF_Transient);
		Body->SetMobility(EComponentMobility::Movable);
		Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->SetGenerateOverlapEvents(false);
		Body->SetCanEverAffectNavigation(false);
		// A rolling ball needs its shadow under it to sit on the ground; it's small, and at most two.
		Body->SetCastShadow(true);
		Body->bReceivesDecals = false;
		Body->bAffectDistanceFieldLighting = false;
		Body->SetStaticMesh(Mesh);
		Body->SetupAttachment(Root);
		Body->SetVisibility(false);
		Body->RegisterComponent();
		AddInstanceComponent(Body);
		Bodies.Add(Body);
	}
	UWorld* World = GetWorld();
	GroundParams = LooterWorld::StaticGeometryParams(World, TEXT("FaunaTumbleGround"), this, false);
	// Glancing off things: everything that stops a walking pawn but the volumes' own boxes (the PCG volume's scattered
	// rocks stay solid; its box doesn't).
	SweepParams = FCollisionQueryParams(SCENE_QUERY_STAT(FaunaTumbleSweep), false, this);
	if (World)
	{
		for (TActorIterator<AVolume> It(World); It; ++It)
		{
			SweepParams.AddIgnoredComponent(It->GetBrushComponent());
		}
	}
	FBox Box(ForceInit);
	for (const FFaunaLane& Lane : Lanes)
	{
		Box += Lane.Start;
		Box += Lane.End;
	}
	Center = Box.IsValid ? Box.GetCenter() : GetActorLocation();
	Reach = Box.IsValid ? static_cast<float>(Box.GetExtent().Size()) : 0.f;
	// The first comes sooner than the rest.
	SpawnTimer = Random.FRandRange(SpawnSecondsMin * 0.3f, FMath::Max(SpawnSecondsMin, SpawnSecondsMax) * 0.6f);
}

FVector AFaunaTumbleweeds::GetFaunaCenter() const
{
	return Center;
}

float AFaunaTumbleweeds::GetFaunaRadius() const
{
	return Reach;
}

bool AFaunaTumbleweeds::IsBusy() const
{
	for (const FFaunaTumble& Tumble : Tumbles)
	{
		if (Tumble.bActive)
		{
			return true;
		}
	}
	return false;
}

bool AFaunaTumbleweeds::FindGround(const FVector& Point, float Above, float Below, FVector& OutGround, FVector& OutNormal) const
{
	const UWorld* World = GetWorld();
	FHitResult Hit;
	if (World && World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, Above), Point - FVector(0.0, 0.0, Below),
		FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
	{
		OutGround = Hit.ImpactPoint;
		OutNormal = Hit.ImpactNormal;
		return true;
	}
	return false;
}

bool AFaunaTumbleweeds::Launch(int32 LaneIndex)
{
	if (!bReady)
	{
		SetupTumbleweeds();
	}
	if (!Lanes.IsValidIndex(LaneIndex))
	{
		return false;
	}
	FFaunaTumble* Free = nullptr;
	for (FFaunaTumble& Tumble : Tumbles)
	{
		if (!Tumble.bActive)
		{
			Free = &Tumble;
			break;
		}
	}
	if (!Free)
	{
		return false;
	}
	const FFaunaLane& Lane = Lanes[LaneIndex];
	const FVector Along = (Lane.End - Lane.Start).GetSafeNormal2D();
	const FVector Across(-Along.Y, Along.X, 0.0);
	FFaunaTumble& Tumble = *Free;
	Tumble = FFaunaTumble();
	Tumble.bActive = true;
	Tumble.Lane = LaneIndex;
	Tumble.Scale = Random.FRandRange(ScaleMin, FMath::Max(ScaleMin, ScaleMax));
	const float Size = Radius * Tumble.Scale;
	Tumble.Position = Lane.Start + Across * Random.FRandRange(-200.f, 200.f) + FVector(0.0, 0.0, Size);
	FVector Ground;
	FVector Normal;
	if (FindGround(Tumble.Position, 300.f, 600.f, Ground, Normal))
	{
		Tumble.Position.Z = Ground.Z + Size;
		Tumble.bFoundGround = true;
		Tumble.bGrounded = true;
		Tumble.GroundNormal = Normal;
	}
	Tumble.Velocity = Along * (WindSpeed * 0.5f);
	Tumble.Spin = FRotator(Random.FRandRange(-90.f, 90.f), Random.FRandRange(-180.f, 180.f), Random.FRandRange(-180.f, 180.f)).Quaternion();
	Tumble.NextHop = Random.FRandRange(HopEveryMin, HopEveryMax);
	Tumble.GustPhase = Random.FRandRange(0.f, 100.f);
	return true;
}

void AFaunaTumbleweeds::TrySpawn(const FFaunaContext& Context)
{
	if (Lanes.Num() == 0)
	{
		return;
	}
	TArray<bool> Busy;
	Busy.Init(false, Lanes.Num());
	for (const FFaunaTumble& Tumble : Tumbles)
	{
		if (Tumble.bActive && Busy.IsValidIndex(Tumble.Lane))
		{
			Busy[Tumble.Lane] = true;
		}
	}
	const int32 First = Random.RandRange(0, Lanes.Num() - 1);
	for (int32 Step = 0; Step < Lanes.Num(); ++Step)
	{
		const int32 Index = (First + Step) % Lanes.Num();
		const FFaunaLane& Lane = Lanes[Index];
		if (Busy[Index])
		{
			continue;
		}
		bool bPlayerNear = false;
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			bPlayerNear |= Threat.bPlayer && FVector::Dist2D(Threat.Location, Lane.Start) < SpawnRange;
		}
		if (!bPlayerNear)
		{
			continue;
		}
		// Close up, its start must be out of sight: nobody sees one appear from nothing.
		if (Context.View.bValid && FVector::Dist(Context.View.Location, Lane.Start) < HiddenSpawnRange
			&& FCreatureUpdateRate::IsInView(Context.View.Location, Context.View.Direction, Context.View.FieldOfView, Lane.Start,
				Radius * ScaleMax * 2.f, 5.f))
		{
			continue;
		}
		if (Launch(Index))
		{
			return;
		}
	}
}

void AFaunaTumbleweeds::Bounced(FFaunaTumble& Tumble, const FFaunaContext& Context)
{
	if (BounceCue.IsNone() || Tumble.SoundCooldown > 0.f || !IsFaunaShown())
	{
		return;
	}
	for (const FFaunaThreat& Threat : Context.Threats)
	{
		if (Threat.bPlayer && FVector::DistSquared(Threat.Location, Tumble.Position) < FMath::Square(SoundRange))
		{
			const float Loud = FMath::Clamp(static_cast<float>(Tumble.Velocity.Size()) / FMath::Max(WindSpeed, 1.f), 0.4f, 1.2f);
			LooterSound::PlayAt(this, BounceCue, Tumble.Position, Loud);
			Tumble.SoundCooldown = SoundCooldownSeconds;
			return;
		}
	}
}

void AFaunaTumbleweeds::Roll(FFaunaTumble& Tumble, float DeltaSeconds, const FFaunaContext& Context)
{
	const FFaunaLane& Lane = Lanes[Tumble.Lane];
	const FVector Along = (Lane.End - Lane.Start).GetSafeNormal2D();
	const float Size = Radius * Tumble.Scale;
	UWorld* World = GetWorld();

	// The wind along its lane, gusting on its own clock.
	const float Gust = FMath::Max(0.2f, 1.f + Gustiness * 2.f * FMath::PerlinNoise1D(WindTime * 0.25f + Tumble.GustPhase));
	const FVector Wind = Along * (WindSpeed * Gust);
	FVector Flat(Tumble.Velocity.X, Tumble.Velocity.Y, 0.0);
	Flat += (Wind - Flat) * FMath::Min(1.f, WindDrag * DeltaSeconds);
	double Rise = Tumble.Velocity.Z;
	if (Tumble.bGrounded)
	{
		// Down the slope it's on, slowed a little by the ground, bouncing along now and then (more when it's fast).
		Flat += FVector(Tumble.GroundNormal.X, Tumble.GroundNormal.Y, 0.0) * (Gravity * SlopePull * DeltaSeconds);
		Flat *= FMath::Max(0.f, 1.f - GroundFriction * DeltaSeconds);
		Tumble.NextHop -= DeltaSeconds;
		if (Tumble.NextHop <= 0.f)
		{
			Rise = Random.FRandRange(HopMin, HopMax) * FMath::Clamp(static_cast<float>(Flat.Size()) / FMath::Max(WindSpeed, 1.f), 0.4f, 1.3f);
			Tumble.NextHop = Random.FRandRange(HopEveryMin, HopEveryMax);
			Tumble.bGrounded = false;
			Bounced(Tumble, Context);
		}
	}
	if (Tumble.bFoundGround)
	{
		Rise -= Gravity * DeltaSeconds;
	}

	// Glancing off whatever stands in its way (the pawn channel: fences, rocks, walls, people, the playable area's walls).
	const FVector From = Tumble.Position;
	FVector To = From + Flat * DeltaSeconds;
	FHitResult Hit;
	if (World && !Flat.IsNearlyZero() && World->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(Size * 0.8f), SweepParams) && !Hit.bStartPenetrating && Hit.ImpactNormal.Z < WallNormalZ)
	{
		const FVector Normal = FVector(Hit.ImpactNormal.X, Hit.ImpactNormal.Y, 0.0).GetSafeNormal();
		To = Hit.Location + Normal * 2.0;
		Flat = FaunaRules::Bounce(Flat, Normal, Restitution).RotateAngleAxis(Random.FRandRange(-Deflect, Deflect), FVector::UpVector);
		Rise = FMath::Max(Rise, static_cast<double>(Random.FRandRange(60.f, 140.f)));
		Bounced(Tumble, Context);
	}

	// Up and down: in the air under gravity, then onto the ground it meets (a little rebound, then rolling).
	To.Z = From.Z + Rise * DeltaSeconds;
	FVector Ground;
	FVector GroundNormal;
	if (FindGround(To, Size + 80.f, Size + 400.f, Ground, GroundNormal))
	{
		Tumble.bFoundGround = true;
		if (To.Z - Size <= Ground.Z + 1.0)
		{
			To.Z = Ground.Z + Size;
			Rise = Rise < 0.0 ? -Rise * 0.25 : Rise;
			if (Rise < 30.0)
			{
				Rise = 0.0;
				Tumble.bGrounded = true;
			}
			Tumble.GroundNormal = GroundNormal;
		}
	}
	else if (!Tumble.bFoundGround)
	{
		// Nothing under it anywhere (a test level): it rolls level.
		To.Z = From.Z;
		Rise = 0.0;
		Tumble.bGrounded = true;
	}

	// It turns as far as it rolled.
	const FVector Moved(To.X - From.X, To.Y - From.Y, 0.0);
	const double Distance = Moved.Size();
	if (Distance > 0.01)
	{
		const FVector Axis = FVector::CrossProduct(FVector::UpVector, Moved / Distance);
		const double Turn = Distance / FMath::Max(Size, 1.f);
		Tumble.Spin = FQuat(Axis, Turn) * Tumble.Spin;
		Tumble.Spin.Normalize();
		Tumble.Rolled += static_cast<float>(Turn);
	}
	Tumble.Position = To;
	Tumble.Velocity = FVector(Flat.X, Flat.Y, Rise);
	Tumble.Age += DeltaSeconds;
	Tumble.SoundCooldown -= DeltaSeconds;
	Tumble.Stuck = Flat.Size() < StuckSpeed ? Tumble.Stuck + DeltaSeconds : 0.f;
}

void AFaunaTumbleweeds::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (!bReady)
	{
		SetupTumbleweeds();
	}
	WindTime += Tick.DeltaSeconds;
	SpawnTimer -= Tick.DeltaSeconds;
	if (SpawnTimer <= 0.f)
	{
		SpawnTimer = Random.FRandRange(SpawnSecondsMin, FMath::Max(SpawnSecondsMin, SpawnSecondsMax));
		TrySpawn(Context);
	}
	for (FFaunaTumble& Tumble : Tumbles)
	{
		if (!Tumble.bActive)
		{
			continue;
		}
		float Left = Tick.DeltaSeconds;
		while (Left > 0.f)
		{
			const float Step = FMath::Min(Left, MaxStep);
			Left -= Step;
			Roll(Tumble, Step, Context);
		}
		if (Tumble.Fade > 0.f)
		{
			Tumble.Fade += Tick.DeltaSeconds;
			Tumble.bActive = Tumble.Fade < FadeSeconds;
			continue;
		}
		// Done: past its lane, stuck, old, or nobody near any more.
		const FFaunaLane& Lane = Lanes[Tumble.Lane];
		const FVector Along = (Lane.End - Lane.Start).GetSafeNormal2D();
		const double Gone = FVector::DotProduct(Tumble.Position - Lane.Start, Along);
		bool bPlayerNear = false;
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			bPlayerNear |= Threat.bPlayer && FVector::Dist2D(Threat.Location, Tumble.Position) < DespawnRange;
		}
		if (Gone > FVector::Dist2D(Lane.Start, Lane.End) + LaneOverrun || Tumble.Stuck > StuckSeconds || Tumble.Age > MaxLifeSeconds
			|| (!bPlayerNear && Context.Threats.Num() > 0))
		{
			Tumble.Fade = UE_KINDA_SMALL_NUMBER;
		}
	}
	PushTransforms();
}

void AFaunaTumbleweeds::PushTransforms()
{
	for (int32 Index = 0; Index < Tumbles.Num() && Index < Bodies.Num(); ++Index)
	{
		UStaticMeshComponent* Body = Bodies[Index];
		const FFaunaTumble& Tumble = Tumbles[Index];
		if (!Body)
		{
			continue;
		}
		if (!Tumble.bActive)
		{
			if (Body->IsVisible())
			{
				Body->SetVisibility(false);
			}
			continue;
		}
		const float Shrink = Tumble.Fade > 0.f ? FMath::Max(0.01f, 1.f - Tumble.Fade / FadeSeconds) : 1.f;
		Body->SetWorldTransform(FTransform(Tumble.Spin, Tumble.Position, FVector(Tumble.Scale * Shrink)), false, nullptr,
			ETeleportType::TeleportPhysics);
		if (!Body->IsVisible())
		{
			Body->SetVisibility(true);
		}
	}
}
