// AStoneSentinelBoulder (StoneSentinelBoulder.h): torn up and carried in a Stone Sentinel's grip (shootable: broken there it
// bursts on the sentinel), thrown on an arc with its landing marked, dropped. Its landing, the hurt it does and its scar are
// StoneSentinelBoulderLanding.cpp's.

#include "Creatures/StoneSentinelBoulder.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherClod.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GroundCrack.h"
#include "Creatures/SpitterGlob.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Creatures/StoneSentinelFX.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/BreakableKinds.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const AStoneSentinelBoulder::RockPathA = TEXT("/Game/Art/Rocks/SM_Rock_A.SM_Rock_A");
const TCHAR* const AStoneSentinelBoulder::RockPathB = TEXT("/Game/Art/Rocks/SM_Rock_B.SM_Rock_B");
const FLinearColor AStoneSentinelBoulder::MarkColor(1.f, 0.36f, 0.1f);

namespace
{
	/** A rock that has flown this long without coming down (off a cliff's edge) is gone. */
	constexpr float MaxFlight = 4.5f;
	/** Its arc is looked along this far ahead as it's thrown, for where it will come down (s). */
	constexpr float PredictSeconds = 3.5f;
	/** It meets a player within this share of its size round its middle. */
	constexpr float BodyShare = 0.42f;
	/** Its trail: a puff of stone dust every this often (s; few: the rock flies at the camera), a pebble falling off every GritEvery. */
	constexpr float TrailEvery = 0.15f;
	constexpr float GritEvery = 0.2f;
	/** The landing's shadow: from this share of the burst's width to all of it, from this opacity to full, as it comes down. */
	constexpr float ShadowStartShare = 0.45f;
	constexpr float ShadowStartOpacity = 0.3f;
	/** The warning ring: from this share of the burst's reach to all of it, its glow and width growing as the rock nears. */
	constexpr float RingStartShare = 0.75f;
	constexpr float RingGlowStart = 0.5f;
	constexpr float RingGlowEnd = 2.f;
	constexpr float RingWidthStart = 7.f;
	constexpr float RingWidthEnd = 14.f;
	constexpr float RingLift = 6.f;
	constexpr int32 RingSegments = 40;
	/** The shadow's stand-in (no decal imported): a ring of dark cracks round this share of the burst. */
	constexpr float MarkCrackWidth = 9.f;
	constexpr float MarkCrackShare = 0.7f;
	const FLinearColor MarkCrackColor(0.03f, 0.025f, 0.02f);
	/** A shot's chip sound at most this often (s). */
	constexpr double ChipSoundGap = 0.06;

	/** An asset made after this code, found only once it exists: until then the class works without it. */
	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

AStoneSentinelBoulder::AStoneSentinelBoulder()
{
	PrimaryActorTick.bCanEverTick = true;
	// Its burst is no kill (the HUD's white marker), as the Gravemother's clod's.
	Tags.Add(FName(LooterBreakables::Tag));

	HitZone = CreateDefaultSubobject<USphereComponent>(TEXT("HitZone"));
	RootComponent = HitZone;
	HitZone->SetMobility(EComponentMobility::Movable);
	HitZone->InitSphereRadius(65.f);
	// Only bullets see it, and only while it's in the grip.
	HitZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitZone->SetCollisionObjectType(ECC_WorldDynamic);
	HitZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitZone->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	HitZone->SetGenerateOverlapEvents(false);
	HitZone->SetCanEverAffectNavigation(false);
	HitZone->ComponentTags.Add(FName(UBulletSubsystem::OwnLookTag));
	HitZone->ComponentTags.Add(GravemotherAssets::SurfaceStoneTag);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(HitZone);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCanEverAffectNavigation(false);
	Body->bReceivesDecals = false;

	static UStaticMesh* const FoundA = FindIfMade<UStaticMesh>(RockPathA);
	static UStaticMesh* const FoundB = FindIfMade<UStaticMesh>(RockPathB);
	RockA = FoundA;
	RockB = FoundB;

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 30.f;
	Health->bShowDamageNumbers = false;
}

AStoneSentinelBoulder* AStoneSentinelBoulder::Spawn(AStoneSentinelCreature& InThrower, const FVector& Where, const FSentinelBoulderTuning& InTuning,
	bool bInKeep)
{
	UWorld* World = InThrower.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AStoneSentinelBoulder* Rock = World->SpawnActor<AStoneSentinelBoulder>(AStoneSentinelBoulder::StaticClass(), FTransform(Where), Params);
	if (!Rock)
	{
		return nullptr;
	}
	Rock->Thrower = &InThrower;
	Rock->Tuning = InTuning;
	// One of the level's granite rocks, sized to the throw's; the engine sphere in granite until they're imported. Its middle
	// on the actor's (the rocks' pivots are at their feet), so it turns over round itself in flight.
	UStaticMesh* Mesh = FMath::RandBool() ? Rock->RockA.Get() : Rock->RockB.Get();
	Mesh = Mesh ? Mesh : (Rock->RockA ? Rock->RockA.Get() : Rock->RockB.Get());
	if (!Mesh)
	{
		Mesh = GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Sphere);
		Rock->Body->SetMaterial(0, GravemotherAssets::StandInLook(Rock, GravemotherAssets::GraniteColor()));
	}
	if (Mesh)
	{
		Rock->Body->SetStaticMesh(Mesh);
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		const float Across = FMath::Max(static_cast<float>(Bounds.BoxExtent.GetMax()) * 2.f, 1.f);
		const float Scale = InTuning.Size / Across;
		Rock->Body->SetRelativeScale3D(FVector(Scale));
		Rock->Body->SetRelativeLocation(-FVector(Bounds.Origin) * Scale);
		Rock->Body->SetRelativeRotation(FRotator(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(-25.f, 25.f)));
	}
	Rock->HitZone->SetSphereRadius(InTuning.HitRadius);
	Rock->SpinAxis = FMath::VRand();
	if (!Rock->HasActorBegunPlay())
	{
		Rock->DispatchBeginPlay();
	}
	Rock->Health->OnDeath.AddUniqueDynamic(Rock, &AStoneSentinelBoulder::HandleDeath);
	Rock->Health->OnDamaged.AddUniqueDynamic(Rock, &AStoneSentinelBoulder::HandleDamaged);
	Rock->bKeep = bInKeep;
	Rock->Retear(Where, InTuning);
	return Rock;
}

void AStoneSentinelBoulder::Retear(const FVector& Where, const FSentinelBoulderTuning& InTuning)
{
	// Torn up again (a kept rock: its mesh, sphere, health and decals reused, nothing new made), in the grip from now.
	Tuning = InTuning;
	Phase = EPhase::Held;
	bIdle = false;
	bBroken = false;
	bDropped = false;
	bScarShown = false;
	Velocity = FVector::ZeroVector;
	Damage = 0.f;
	FlightTime = 0.f;
	PlayersHit = 0;
	TrailIn = 0.f;
	GritIn = 0.f;
	ScarAge = 0.f;
	PlannedLanding = Where;
	LandedAt = Where;
	Position = Where;
	SpinAxis = FMath::VRand();
	ClearMark();
	GravemotherAssets::SetDecalOpacity(Scar, 0.f);
	SetLifeSpan(0.f);
	SetActorLocationAndRotation(Where, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f));
	HitZone->SetSphereRadius(Tuning.HitRadius);
	HitZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetHiddenInGame(false);
	// It takes a share of its thrower's health to break (two or three rifle shots at any level).
	if (const AStoneSentinelCreature* Its = Thrower.Get())
	{
		if (const UHealthComponent* Guard = Its->FindComponentByClass<UHealthComponent>())
		{
			Health->SetMaxHealth(FMath::Max(Guard->GetMaxHealth() * Tuning.HealthShare, 1.f));
		}
	}
	Health->ResetHealth();
	SetActorTickEnabled(true);
}

void AStoneSentinelBoulder::GoIdle()
{
	// Done: out of sight and out of the way until its thrower tears it up again; gone if nobody keeps it.
	ClearMark();
	GravemotherAssets::SetDecalOpacity(Scar, 0.f);
	bScarShown = false;
	Phase = EPhase::Landed;
	Body->SetHiddenInGame(true);
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorTickEnabled(false);
	if (!bKeep || !Thrower.IsValid())
	{
		SetLifeSpan(0.2f);
		return;
	}
	bIdle = true;
}

void AStoneSentinelBoulder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AStoneSentinelBoulder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearMark();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// In its grip
// ---------------------------------------------------------------------------

void AStoneSentinelBoulder::Hold(const FVector& Where, float Yaw)
{
	if (Phase != EPhase::Held)
	{
		return;
	}
	Position = Where;
	SetActorLocationAndRotation(Where, FRotator(0.f, Yaw, 0.f));
}

void AStoneSentinelBoulder::HandleDamaged(float InDamage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	// Every hit on it knocks stone off and sparks: the player sees it giving.
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		const FVector Out = (HitLocation - GetActorLocation()).GetSafeNormal();
		Effects->Chunks(EGraveChunk::StoneChip, HitLocation, Out.IsNearlyZero() ? FVector::UpVector : Out, bCritical ? 4 : 2, 360.f, 0.7f);
		Effects->Sparks(HitLocation, Out, -Out, bCritical ? 4 : 2);
	}
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastChipSound >= ChipSoundGap)
	{
		LastChipSound = Now;
		LooterSound::PlayAt(this, FName(StoneSentinelCue::Chip), HitLocation, 0.8f, 1.1f);
	}
}

void AStoneSentinelBoulder::HandleDeath(AController* Killer)
{
	if (Phase != EPhase::Held || bBroken)
	{
		return;
	}
	// Shot to pieces in the grip: it bursts over its thrower's head (its counter: StoneSentinelCreatureBoulder.cpp), and is gone.
	bBroken = true;
	Phase = EPhase::Landed;
	LandedAt = GetActorLocation();
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetHiddenInGame(true);
	if (AStoneSentinelCreature* Its = Thrower.Get())
	{
		Its->CounterBoulder(Killer, LandedAt);
	}
	GoIdle();
}

// ---------------------------------------------------------------------------
// The lob
// ---------------------------------------------------------------------------

void AStoneSentinelBoulder::Throw(const FVector& LaunchVelocity, float InDamage, AController* By, AActor* From)
{
	if (Phase != EPhase::Held)
	{
		return;
	}
	UWorld* World = GetWorld();
	Phase = EPhase::Flying;
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Position = GetActorLocation();
	Velocity = LaunchVelocity;
	Damage = InDamage;
	ThrowInstigator = By;
	ThrowCauser = From;
	FlightTime = 0.f;
	SpinSpeed = FMath::FRandRange(2.f, 3.5f);
	// It meets what the Spitter's globs meet (never its thrower, nor itself).
	WorldQuery = ASpitterGlob::MakeWorldQuery(World, Thrower.Get());
	WorldQuery.AddIgnoredActor(this);

	// Where it'll come down: the shadow and the warning ring lie there, growing as it falls.
	if (!AGravemotherClod::PredictLanding(World, Position, Velocity, Tuning.Gravity, PredictSeconds, WorldQuery, PlannedLanding, PlannedNormal,
		PlannedSeconds, 16))
	{
		PlannedSeconds = PredictSeconds;
	}
	const float Width = Tuning.SplashRadius * 2.f;
	// Its shadow decal made once and laid again at each throw (its size and darkness set by UpdateMark).
	if (Shadow)
	{
		Shadow->SetWorldLocation(PlannedLanding);
	}
	else
	{
		Shadow = GravemotherAssets::AddGroundDecal(*this, GravemotherAssets::DecalLandingShadow, PlannedLanding, FMath::FRandRange(0.f, 360.f),
			Width * ShadowStartShare, Width * ShadowStartShare, 3, 400.f);
	}
	if (!Shadow && World)
	{
		// The stand-in: a ring of dark cracks bursting out round the spot over the flight.
		if (AGroundCrack* Ring = AGroundCrack::Spawn(World, PlannedLanding, PlannedLanding + FVector::ForwardVector, MarkCrackWidth, MarkCrackColor,
			FMath::Rand()))
		{
			Ring->BurstSeconds = FMath::Max(PlannedSeconds, 0.2f);
			Ring->Burst(PlannedLanding, Tuning.SplashRadius * MarkCrackShare);
			MarkCrack = Ring;
		}
	}
	UpdateMark();
	LooterSound::PlayAttached(FName(StoneSentinelCue::BoulderFly), Body);
	UE_LOG(LogLooter, Verbose, TEXT("%s's boulder thrown: %.2f s to %s."), *GetNameSafe(Thrower.Get()), PlannedSeconds, *PlannedLanding.ToCompactString());
}

void AStoneSentinelBoulder::Drop()
{
	if (Phase != EPhase::Held)
	{
		return;
	}
	// The grip lost: it falls where it is and bursts harmlessly.
	Phase = EPhase::Dropping;
	bDropped = true;
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Position = GetActorLocation();
	Velocity = FVector(0.0, 0.0, 40.0);
	WorldQuery = ASpitterGlob::MakeWorldQuery(GetWorld(), Thrower.Get());
	WorldQuery.AddIgnoredActor(this);
	FlightTime = 0.f;
}

void AStoneSentinelBoulder::Advance(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	if (Phase == EPhase::Flying || Phase == EPhase::Dropping)
	{
		Fly(DeltaSeconds);
	}
	else if (Phase == EPhase::Landed && bScarShown)
	{
		TickScar(DeltaSeconds);
	}
}

void AStoneSentinelBoulder::Fly(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector From = Position;
	const FVector NewVelocity = Velocity - FVector(0.0, 0.0, Tuning.Gravity * DeltaSeconds);
	const FVector To = From + (Velocity + NewVelocity) * (0.5f * DeltaSeconds);
	FHitResult WorldHit;
	const bool bHitWorld = World->LineTraceSingleByObjectType(WorldHit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), WorldQuery);
	const FVector Reach = bHitWorld ? FVector(WorldHit.Location) : To;

	// A thrown rock comes down on a player it meets on the way (a dropped one only on the ground).
	ACharacter* Victim = nullptr;
	float VictimTime = 1.f;
	if (Phase == EPhase::Flying)
	{
		const AActor* By = ThrowCauser.IsValid() ? ThrowCauser.Get() : Thrower.Get();
		const float Radius = Tuning.Size * BodyShare;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = It->Get();
			ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
			const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
			float Time = 1.f;
			if (Capsule && UEnemyProjectileSubsystem::CanHurt(By, Character)
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
	if (Phase == EPhase::Flying)
	{
		Trail(DeltaSeconds);
		UpdateMark();
	}
	if (FlightTime >= MaxFlight)
	{
		// Off an edge, into nothing: done without a burst.
		Phase = EPhase::Landed;
		bBroken = true;
		LandedAt = Position;
		GoIdle();
	}
}

void AStoneSentinelBoulder::Trail(float DeltaSeconds)
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
		// A thin ribbon of stone dust hanging where it passed.
		FGraveDustBurst Ribbon;
		Ribbon.Kind = EGraveDust::Stone;
		Ribbon.Count = 1;
		Ribbon.Spread = 12.f;
		Ribbon.SpreadUp = 8.f;
		Ribbon.StartSize = 25.f;
		Ribbon.EndSize = 60.f;
		Ribbon.Life = 0.6f;
		Ribbon.Opacity = 0.2f;
		Ribbon.Drift = Velocity * 0.04f;
		Ribbon.Outward = 10.f;
		// Never at the lens (StoneSentinelFX): its last puffs would hang where it comes down on the player.
		StoneSentinelFX::Dust(this, Position - Velocity.GetSafeNormal() * (Tuning.Size * 0.4f), Ribbon);
	}
	GritIn -= DeltaSeconds;
	if (GritIn <= 0.f)
	{
		GritIn += GritEvery;
		// Earth and pebbles still falling off it from where it was torn.
		Effects->Chunks(EGraveChunk::Debris, Position, -Velocity.GetSafeNormal(), 1, 90.f, 0.4f);
	}
}

void AStoneSentinelBoulder::UpdateMark()
{
	const float Share = Phase == EPhase::Flying ? FMath::Clamp(FlightTime / FMath::Max(PlannedSeconds, 0.05f), 0.f, 1.f) : 0.f;
	const float Width = Tuning.SplashRadius * 2.f;
	if (Shadow)
	{
		const float Size = Width * FMath::Lerp(ShadowStartShare, 1.f, Share);
		GravemotherAssets::SetDecalSize(Shadow, Size, Size, 400.f);
		GravemotherAssets::SetDecalOpacity(Shadow, FMath::Lerp(ShadowStartOpacity, 1.f, Share));
	}
	// The warning ring, drawn this frame: the burst's reach, brighter and bolder as the rock comes down.
	UWorld* World = GetWorld();
	if (Phase == EPhase::Flying && World && World->IsGameWorld())
	{
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			Effects.AddGroundRing(PlannedLanding + PlannedNormal.GetSafeNormal() * RingLift, Tuning.SplashRadius * FMath::Lerp(RingStartShare, 1.f, Share),
				FMath::Lerp(RingWidthStart, RingWidthEnd, Share), MarkColor, FMath::Lerp(RingGlowStart, RingGlowEnd, Share * Share), RingSegments);
		}
	}
}
