// AGravemotherClod (GravemotherClod.h): torn up and held in her grip (shootable), thrown on a lob with its shadow and its
// trail, dropped. Its landing, its burst on her and the hurt it does are GravemotherClodLanding.cpp's.

#include "Creatures/GravemotherClod.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GroundCrack.h"
#include "Creatures/SpitterGlob.h"
#include "Creatures/SpitterRules.h"
#include "World/BreakableKinds.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

namespace
{
	/** A clod that has flown this long without coming down (off a cliff's edge) is gone. */
	constexpr float MaxFlight = 4.f;
	/** Its arc is looked along this far ahead as it's thrown, for where it will come down (s). */
	constexpr float PredictSeconds = 3.f;
	/** It meets a player within this share of its size round its middle. */
	constexpr float BodyShare = 0.4f;
	/** Its trail: a puff of earth every this often (s), a crumb falling off every CrumbEvery. */
	constexpr float TrailEvery = 0.05f;
	constexpr float CrumbEvery = 0.12f;
	/** Its shadow on the ground: from this size to that (cm) and this opacity to that, as it comes down. */
	constexpr float ShadowStartSize = 100.f;
	constexpr float ShadowEndSize = 260.f;
	constexpr float ShadowStartOpacity = 0.3f;
	constexpr float ShadowEndOpacity = 0.85f;
	/** The shadow's stand-in: a ring of dark cracks (AGroundCrack) this wide, round this share of the burst. */
	constexpr float ShadowCrackWidth = 8.f;
	constexpr float ShadowCrackShare = 0.6f;
	const FLinearColor ShadowCrackColor(0.03f, 0.025f, 0.02f);
}

AGravemotherClod::AGravemotherClod()
{
	PrimaryActorTick.bCanEverTick = true;
	// Its burst is no kill (the HUD's white marker).
	Tags.Add(FName(LooterBreakables::Tag));

	HitZone = CreateDefaultSubobject<USphereComponent>(TEXT("HitZone"));
	RootComponent = HitZone;
	HitZone->SetMobility(EComponentMobility::Movable);
	HitZone->InitSphereRadius(75.f);
	// Only bullets see it, and only while it's in her grip.
	HitZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitZone->SetCollisionObjectType(ECC_WorldDynamic);
	HitZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitZone->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	HitZone->SetGenerateOverlapEvents(false);
	HitZone->SetCanEverAffectNavigation(false);
	HitZone->ComponentTags.Add(FName(UBulletSubsystem::OwnLookTag));
	HitZone->ComponentTags.Add(GravemotherAssets::SurfaceDirtTag);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(HitZone);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCanEverAffectNavigation(false);
	Body->bReceivesDecals = false;

	// One hit bursts it.
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 1.f;
	Health->bShowDamageNumbers = false;
}

AGravemotherClod* AGravemotherClod::Spawn(AGravemotherCreature& InMother, const FVector& Where, const FGravemotherClodSettings& InSettings)
{
	UWorld* World = InMother.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AGravemotherClod* Clod = World->SpawnActor<AGravemotherClod>(AGravemotherClod::StaticClass(), FTransform(Where), Params);
	if (!Clod)
	{
		return nullptr;
	}
	Clod->Mother = &InMother;
	Clod->Settings = InSettings;
	if (UStaticMesh* Mesh = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::ClodLarge))
	{
		Clod->Body->SetStaticMesh(Mesh);
		Clod->MeshScale = 1.f;
	}
	else
	{
		// The stand-in: an earth ball as big as the clod.
		Clod->Body->SetStaticMesh(GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Sphere));
		Clod->Body->SetMaterial(0, GravemotherAssets::StandInLook(Clod, GravemotherAssets::EarthColor()));
		Clod->MeshScale = FullSize / 100.f;
	}
	Clod->SpinAxis = FMath::VRand();
	Clod->SetActorRotation(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f));
	if (!Clod->HasActorBegunPlay())
	{
		Clod->DispatchBeginPlay();
	}
	Clod->Health->OnDeath.AddUniqueDynamic(Clod, &AGravemotherClod::HandleDeath);
	Clod->Hold(Where, 0.3f);
	return Clod;
}

void AGravemotherClod::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AGravemotherClod::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearShadow();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// In her grip
// ---------------------------------------------------------------------------

void AGravemotherClod::Hold(const FVector& Where, float GrownShare)
{
	if (Phase != EPhase::Held)
	{
		return;
	}
	Grown = FMath::Clamp(GrownShare, 0.05f, 1.f);
	Position = Where;
	SetActorLocation(Where);
	Body->SetRelativeScale3D(FVector(MeshScale * Grown));
	// The forgiving reach round it: never under half of it, so it can be shot as soon as it's out of the ground.
	HitZone->SetSphereRadius(Settings.HitRadius * FMath::Max(Grown, 0.5f));
}

void AGravemotherClod::HandleDeath(AController* Killer)
{
	if (Phase != EPhase::Held || bCountered)
	{
		return;
	}
	// Shot in her grip: it bursts on her, and she's stunned.
	bCountered = true;
	BurstOnMother();
	if (AGravemotherCreature* Her = Mother.Get())
	{
		Her->CounterClod(Killer);
	}
}

// ---------------------------------------------------------------------------
// The lob
// ---------------------------------------------------------------------------

bool AGravemotherClod::PredictLanding(const UWorld* World, const FVector& From, const FVector& LaunchVelocity, float Gravity, float MaxSeconds,
	const FCollisionQueryParams& Query, FVector& OutWhere, FVector& OutNormal, float& OutSeconds, int32 Pieces)
{
	OutNormal = FVector::UpVector;
	FVector Last = From;
	const int32 Count = FMath::Max(Pieces, 1);
	for (int32 Piece = 1; Piece <= Count; ++Piece)
	{
		const float Time = MaxSeconds * static_cast<float>(Piece) / static_cast<float>(Count);
		const FVector Next = SpitterRules::LobPoint(From, LaunchVelocity, Time, Gravity);
		FHitResult Hit;
		if (World && World->LineTraceSingleByObjectType(Hit, Last, Next, FCollisionObjectQueryParams(ECC_WorldStatic), Query))
		{
			OutWhere = Hit.ImpactPoint;
			OutNormal = Hit.ImpactNormal;
			OutSeconds = MaxSeconds * (static_cast<float>(Piece - 1) + Hit.Time) / static_cast<float>(Count);
			return true;
		}
		Last = Next;
	}
	OutWhere = Last;
	OutSeconds = MaxSeconds;
	return false;
}

void AGravemotherClod::Throw(const FVector& LaunchVelocity, float InDamage, AController* By, AActor* From)
{
	if (Phase != EPhase::Held)
	{
		return;
	}
	UWorld* World = GetWorld();
	Phase = EPhase::Flying;
	Grown = 1.f;
	Body->SetRelativeScale3D(FVector(MeshScale));
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Position = GetActorLocation();
	Velocity = LaunchVelocity;
	Damage = InDamage;
	ThrowInstigator = By;
	ThrowCauser = From;
	FlightTime = 0.f;
	SpinSpeed = FMath::FRandRange(2.5f, 4.5f);
	// It meets what the Spitter's globs meet (never its thrower, nor itself).
	WorldQuery = ASpitterGlob::MakeWorldQuery(World, Mother.Get());
	WorldQuery.AddIgnoredActor(this);

	// Where it'll come down: its shadow lies there, growing as it falls.
	if (!PredictLanding(World, Position, Velocity, Settings.Gravity, PredictSeconds, WorldQuery, PlannedLanding, PlannedNormal, PlannedSeconds))
	{
		PlannedSeconds = PredictSeconds;
	}
	Shadow = GravemotherAssets::AddGroundDecal(*this, GravemotherAssets::DecalLandingShadow, PlannedLanding, FMath::FRandRange(0.f, 360.f),
		ShadowStartSize, ShadowStartSize, 3, 400.f);
	if (!Shadow && World)
	{
		// The stand-in: a ring of dark cracks bursting out round the spot over the flight.
		if (AGroundCrack* Ring = AGroundCrack::Spawn(World, PlannedLanding, PlannedLanding + FVector::ForwardVector, ShadowCrackWidth,
			ShadowCrackColor, FMath::Rand()))
		{
			Ring->BurstSeconds = FMath::Max(PlannedSeconds, 0.2f);
			Ring->Burst(PlannedLanding, Settings.BurstRadius * ShadowCrackShare);
			ShadowCrack = Ring;
		}
	}
	UpdateShadow();
	LooterSound::PlayAttached(LooterSoundCue::GravemotherClodThrow, Body);
	UE_LOG(LogLooter, Verbose, TEXT("%s's clod thrown: %.2f s to %s."), *GetNameSafe(Mother.Get()), PlannedSeconds, *PlannedLanding.ToCompactString());
}

void AGravemotherClod::Drop()
{
	if (Phase != EPhase::Held)
	{
		return;
	}
	// Her grip lost: it falls where it is and bursts harmlessly.
	Phase = EPhase::Dropping;
	bDropped = true;
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Position = GetActorLocation();
	Velocity = FVector(0.0, 0.0, 60.0);
	WorldQuery = ASpitterGlob::MakeWorldQuery(GetWorld(), Mother.Get());
	WorldQuery.AddIgnoredActor(this);
	FlightTime = 0.f;
}

void AGravemotherClod::Advance(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f || (Phase != EPhase::Flying && Phase != EPhase::Dropping))
	{
		return;
	}
	Fly(DeltaSeconds);
}

void AGravemotherClod::Fly(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector From = Position;
	const FVector NewVelocity = Velocity - FVector(0.0, 0.0, Settings.Gravity * DeltaSeconds);
	const FVector To = From + (Velocity + NewVelocity) * (0.5f * DeltaSeconds);
	FHitResult WorldHit;
	const bool bHitWorld = World->LineTraceSingleByObjectType(WorldHit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), WorldQuery);
	const FVector Reach = bHitWorld ? FVector(WorldHit.Location) : To;

	// A thrown clod comes down on a player it meets on the way (a dropped one only on the ground).
	ACharacter* Victim = nullptr;
	float VictimTime = 1.f;
	if (Phase == EPhase::Flying)
	{
		const AActor* Thrower = ThrowCauser.IsValid() ? ThrowCauser.Get() : Mother.Get();
		const float Radius = FullSize * BodyShare;
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			ACharacter* Character = *It;
			const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
			float Time = 1.f;
			if (Capsule && UEnemyProjectileSubsystem::CanHurt(Thrower, Character)
				&& UEnemyProjectileSubsystem::SweepHitsCapsule(From, Reach, Radius, Capsule->GetComponentLocation(), Capsule->GetScaledCapsuleRadius(),
					Capsule->GetScaledCapsuleHalfHeight(), Time)
				&& (!Victim || Time < VictimTime))
			{
				Victim = Character;
				VictimTime = Time;
			}
		}
	}

	Velocity = NewVelocity;
	FlightTime += DeltaSeconds;
	if (Victim)
	{
		// On a player: no surface's normal (its rubble goes on the ground under them).
		Position = FMath::Lerp(From, Reach, VictimTime);
		SetActorLocation(Position);
		Land(Position, FVector::ZeroVector);
		return;
	}
	if (bHitWorld)
	{
		Position = WorldHit.ImpactPoint;
		SetActorLocation(Position);
		Land(Position, WorldHit.ImpactNormal);
		return;
	}
	Position = To;
	SetActorLocationAndRotation(Position, FQuat(SpinAxis, SpinSpeed * DeltaSeconds) * GetActorQuat());
	Trail(DeltaSeconds);
	UpdateShadow();
	if (FlightTime >= MaxFlight)
	{
		// Off an edge, into nothing: gone without a burst.
		Phase = EPhase::Landed;
		LandedAt = Position;
		ClearShadow();
		Destroy();
	}
}

void AGravemotherClod::Trail(float DeltaSeconds)
{
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	if (!Effects)
	{
		return;
	}
	TrailIn -= DeltaSeconds;
	if (TrailIn <= 0.f)
	{
		TrailIn += TrailEvery;
		// A ribbon of earth dust hanging where it passed.
		FGraveDustBurst Ribbon;
		Ribbon.Count = 1;
		Ribbon.Spread = 15.f;
		Ribbon.SpreadUp = 10.f;
		Ribbon.StartSize = 35.f;
		Ribbon.EndSize = 95.f;
		Ribbon.Life = 0.9f;
		Ribbon.Opacity = 0.24f;
		Ribbon.Drift = Velocity * 0.05f;
		Ribbon.Outward = 12.f;
		Effects->Puffs(Position - Velocity.GetSafeNormal() * (FullSize * 0.4f), Ribbon);
	}
	CrumbIn -= DeltaSeconds;
	if (CrumbIn <= 0.f)
	{
		CrumbIn += CrumbEvery;
		// Crumbs falling off it.
		Effects->Chunks(EGraveChunk::Debris, Position, -Velocity.GetSafeNormal(), 1, 80.f, 0.45f);
	}
}

void AGravemotherClod::UpdateShadow()
{
	const float Share = Phase == EPhase::Flying ? FMath::Clamp(FlightTime / FMath::Max(PlannedSeconds, 0.05f), 0.f, 1.f) : 0.f;
	if (Shadow)
	{
		const float Size = FMath::Lerp(ShadowStartSize, ShadowEndSize, Share);
		GravemotherAssets::SetDecalSize(Shadow, Size, Size, 400.f);
		GravemotherAssets::SetDecalOpacity(Shadow, FMath::Lerp(ShadowStartOpacity, ShadowEndOpacity, Share));
	}
}
