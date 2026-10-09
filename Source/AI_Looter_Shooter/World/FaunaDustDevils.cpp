#include "World/FaunaDustDevils.h"
#include "World/WorldQueries.h"
#include "Audio/LooterSound.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** How often it picks a new way to wander (s), and how much of its motion is the wind's. */
	constexpr float GoalSecondsMin = 1.5f;
	constexpr float GoalSecondsMax = 3.5f;
	constexpr float WindShare = 0.5f;

	/** Rising, it starts this narrow and this low (shares of its full size); thinning, it widens by this share. */
	constexpr float StartWidth = 0.2f;
	constexpr float StartHeight = 0.5f;
	constexpr float FadeWiden = 0.25f;

	/** Its lean into the way it travels (degrees at full wander speed). */
	constexpr float Lean = 6.f;

	/** How many spots it tries for one in range before waiting for the next time. */
	constexpr int32 SpotTries = 6;
}

AFaunaDustDevils::AFaunaDustDevils()
{
	CullDistance = 9000.f;
	ShownIn = FName(TEXT("Day"));
}

void AFaunaDustDevils::BeginPlay()
{
	Super::BeginPlay();
	SetupDevils();
}

void AFaunaDustDevils::SetupDevils()
{
	if (bReady)
	{
		return;
	}
	bReady = true;
	Random.Initialize(Seed);
	Body = NewObject<UStaticMeshComponent>(this, TEXT("DustDevil"), RF_Transient);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
	Body->SetCastShadow(false);
	Body->bReceivesDecals = false;
	Body->SetStaticMesh(Mesh);
	Body->SetupAttachment(Root);
	Body->SetVisibility(false);
	Body->RegisterComponent();
	AddInstanceComponent(Body);
	// Its own instance of the dust's material, so it can thin in and out without touching the chimney smoke.
	if (UMaterialInterface* Material = Mesh ? Body->GetMaterial(0) : nullptr)
	{
		float Opacity = BaseOpacity;
		if (Material->GetScalarParameterValue(FHashedMaterialParameterInfo(OpacityParameter), Opacity))
		{
			BaseOpacity = Opacity;
		}
		Dust = UMaterialInstanceDynamic::Create(Material, this);
		Body->SetMaterial(0, Dust);
	}
	GroundParams = LooterWorld::StaticGeometryParams(GetWorld(), TEXT("FaunaDustDevilGround"), this, false);
	FBox Box(ForceInit);
	for (const FFaunaZone& Each : Spots)
	{
		Box += Each.Center + FVector(Each.Radius, Each.Radius, 0.0);
		Box += Each.Center - FVector(Each.Radius, Each.Radius, 0.0);
	}
	Center = Box.IsValid ? Box.GetCenter() : GetActorLocation();
	Reach = Box.IsValid ? static_cast<float>(Box.GetExtent().Size()) : 0.f;
	SpawnTimer = Random.FRandRange(SpawnSecondsMin * 0.5f, FMath::Max(SpawnSecondsMin, SpawnSecondsMax));
}

FVector AFaunaDustDevils::GetFaunaCenter() const
{
	return bActive ? Position : Center;
}

float AFaunaDustDevils::GetFaunaRadius() const
{
	return bActive ? 800.f * Scale : Reach;
}

float AFaunaDustDevils::GetStrength() const
{
	if (!bActive)
	{
		return 0.f;
	}
	const float In = FMath::Clamp(Age / FMath::Max(GrowSeconds, 0.01f), 0.f, 1.f);
	const float Out = FMath::Clamp((Life - Age) / FMath::Max(FadeSeconds, 0.01f), 0.f, 1.f);
	return FMath::SmoothStep(0.f, 1.f, FMath::Min(In, Out));
}

void AFaunaDustDevils::OnFaunaShownChanged(bool bShown)
{
	if (!bShown && bActive)
	{
		End();
	}
}

bool AFaunaDustDevils::Raise(int32 SpotIndex)
{
	if (!bReady)
	{
		SetupDevils();
	}
	if (!Spots.IsValidIndex(SpotIndex) || bActive)
	{
		return false;
	}
	const FFaunaZone& Where = Spots[SpotIndex];
	bActive = true;
	Spot = SpotIndex;
	Age = 0.f;
	Life = Random.FRandRange(LifeSecondsMin, FMath::Max(LifeSecondsMin, LifeSecondsMax));
	Scale = Random.FRandRange(ScaleMin, FMath::Max(ScaleMin, ScaleMax));
	Spin = Random.FRandRange(0.f, 360.f);
	const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
	const float Distance = FMath::Sqrt(Random.FRand()) * Where.Radius * 0.6f;
	Position = Where.Center + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.0);
	Goal = Position;
	GoalTimer = 0.f;
	if (Body)
	{
		Body->SetVisibility(true);
	}
	if (!WhirlCue.IsNone() && Body && IsFaunaShown())
	{
		Whirl = LooterSound::Start(this, WhirlCue, Body);
	}
	PushTransform();
	return true;
}

void AFaunaDustDevils::End()
{
	bActive = false;
	Spot = INDEX_NONE;
	LooterSound::Stop(Whirl.Get(), 0.8f);
	Whirl.Reset();
	if (Body)
	{
		Body->SetVisibility(false);
	}
	SpawnTimer = Random.FRandRange(SpawnSecondsMin, FMath::Max(SpawnSecondsMin, SpawnSecondsMax));
}

void AFaunaDustDevils::TryRaise(const FFaunaContext& Context)
{
	if (Spots.Num() == 0)
	{
		return;
	}
	for (int32 Try = 0; Try < SpotTries; ++Try)
	{
		const int32 Index = Random.RandRange(0, Spots.Num() - 1);
		const FVector& At = Spots[Index].Center;
		// Watchable: neither on top of the player nor beyond seeing, from every player there is.
		double Nearest = TNumericLimits<double>::Max();
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			if (Threat.bPlayer)
			{
				Nearest = FMath::Min(Nearest, FVector::Dist2D(Threat.Location, At));
			}
		}
		if (Nearest >= SpawnRangeMin && Nearest <= SpawnRangeMax && Raise(Index))
		{
			return;
		}
	}
}

void AFaunaDustDevils::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (!bReady)
	{
		SetupDevils();
	}
	const float DeltaSeconds = Tick.DeltaSeconds;
	if (!bActive)
	{
		SpawnTimer -= DeltaSeconds;
		if (SpawnTimer <= 0.f)
		{
			SpawnTimer = Random.FRandRange(SpawnSecondsMin, FMath::Max(SpawnSecondsMin, SpawnSecondsMax));
			TryRaise(Context);
		}
		return;
	}
	Age += DeltaSeconds;
	if (Age >= Life)
	{
		End();
		return;
	}
	// Wandering about its spot, drifting with the wind, its foot kept on the ground.
	const FFaunaZone& Where = Spots[Spot];
	GoalTimer -= DeltaSeconds;
	if (GoalTimer <= 0.f)
	{
		GoalTimer = Random.FRandRange(GoalSecondsMin, GoalSecondsMax);
		const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
		const float Distance = FMath::Sqrt(Random.FRand()) * Where.Radius;
		Goal = Where.Center + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.0);
	}
	const FVector Wind = FRotator(0.f, WindYaw, 0.f).Vector();
	FVector Toward = Goal - Position;
	Toward.Z = 0.0;
	const FVector Velocity = (Toward.GetSafeNormal() * (1.f - WindShare) + Wind * WindShare) * WanderSpeed;
	Position += Velocity * DeltaSeconds;
	if (const UWorld* World = GetWorld())
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Position + FVector(0.0, 0.0, 400.0), Position - FVector(0.0, 0.0, 800.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
		{
			Position.Z = Hit.ImpactPoint.Z;
		}
	}
	Spin = FMath::Fmod(Spin + SpinDegreesPerSecond * DeltaSeconds, 360.f);
	if (Tick.bOnScreen && IsFaunaShown())
	{
		PushTransform();
	}
}

void AFaunaDustDevils::PushTransform()
{
	if (!Body || !bActive)
	{
		return;
	}
	const float Strength = GetStrength();
	const float Grown = FMath::Clamp(Age / FMath::Max(GrowSeconds, 0.01f), 0.f, 1.f);
	const float Fading = FMath::Clamp(1.f - (Life - Age) / FMath::Max(FadeSeconds, 0.01f), 0.f, 1.f);
	// Rising out of a low swirl into the full column; thinning, it spreads and loses its shape.
	const float Width = Scale * (FMath::Lerp(StartWidth, 1.f, FMath::SmoothStep(0.f, 1.f, Grown)) + FadeWiden * Fading);
	const float Height = Scale * FMath::Lerp(StartHeight, 1.f, FMath::SmoothStep(0.f, 1.f, Grown));
	// Spinning about its own axis, that axis leaning downwind (the lean applied after the spin, so it doesn't turn with it).
	const FVector Wind = FRotator(0.f, WindYaw, 0.f).Vector();
	const FQuat Leaning(FVector::CrossProduct(FVector::UpVector, Wind).GetSafeNormal(), FMath::DegreesToRadians(Lean));
	const FQuat Spinning(FVector::UpVector, FMath::DegreesToRadians(Spin));
	Body->SetWorldTransform(FTransform(Leaning * Spinning, Position, FVector(Width, Width, Height)), false, nullptr,
		ETeleportType::TeleportPhysics);
	if (Dust)
	{
		Dust->SetScalarParameterValue(OpacityParameter, BaseOpacity * Strength);
	}
}
