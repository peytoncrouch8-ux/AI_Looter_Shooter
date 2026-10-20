// AGravemotherRubble (GravemotherRubble.h): a Grave Clod's pile, its cover, its loose clods and scar, and its settling away.

#include "Creatures/GravemotherRubble.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherFX.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

namespace
{
	/** The stand-in pile: the engine sphere made 160 x 180 x 80 and half sunk. */
	const FVector StandInPileScale(1.6, 1.8, 0.8);
	constexpr float StandInSink = 0.25f;
	/** Loose clods round it: how many, how far out (cm), sunk this far into the ground. */
	constexpr int32 LooseCount = 3;
	constexpr float LooseMin = 95.f;
	constexpr float LooseMax = 135.f;
	constexpr float LooseSink = 8.f;
	constexpr float StandInLooseSize = 0.35f;
	/** The scar under it (cm across), fading as it settles. */
	constexpr float ScarSize = 260.f;
}

AGravemotherRubble::AGravemotherRubble()
{
	PrimaryActorTick.bCanEverTick = true;
	// Nothing spawns on it (spawn probes, phase steps), as on a rock.
	Tags.Add(GravemotherAssets::ObstacleTag);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Pile = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pile"));
	Pile->SetupAttachment(Root);
	Pile->SetMobility(EComponentMobility::Movable);
	// Cover, as a rock is to every query (bullets, sight, lobs, steering, feet); pawns once it's clear of them; never the
	// camera, which would snap in on it.
	Pile->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Pile->SetCollisionObjectType(ECC_WorldStatic);
	Pile->SetCollisionResponseToAllChannels(ECR_Block);
	Pile->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Pile->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Pile->SetGenerateOverlapEvents(false);
	Pile->SetCanEverAffectNavigation(false);
	Pile->ComponentTags.Add(GravemotherAssets::SurfaceDirtTag);
	// Its own scar (and a furrow's crack) lies on the ground round it, not painted over the pile's top.
	Pile->bReceivesDecals = false;
}

AGravemotherRubble* AGravemotherRubble::Spawn(UWorld* World, const FVector& InGround, float Yaw, float InSeconds, float InSettleSeconds)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AGravemotherRubble* Rubble = World->SpawnActor<AGravemotherRubble>(AGravemotherRubble::StaticClass(),
		FTransform(FRotator(0.f, Yaw, 0.f), InGround), Params);
	if (!Rubble)
	{
		return nullptr;
	}
	Rubble->Ground = InGround;
	Rubble->Seconds = FMath::Max(InSeconds, 0.1f);
	Rubble->SettleSeconds = FMath::Max(InSettleSeconds, 0.05f);
	if (UStaticMesh* Mesh = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::Rubble))
	{
		Rubble->Pile->SetStaticMesh(Mesh);
		Rubble->PileScale = FVector::OneVector;
		Rubble->PileOffset = FVector::ZeroVector;
		Rubble->PileHeight = static_cast<float>(Mesh->GetBounds().BoxExtent.Z * 2.0);
	}
	else
	{
		Rubble->Pile->SetStaticMesh(GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Sphere));
		Rubble->Pile->SetMaterial(0, GravemotherAssets::StandInLook(Rubble, GravemotherAssets::EarthColor()));
		Rubble->PileScale = StandInPileScale;
		Rubble->PileOffset = FVector(0.0, 0.0, 50.0 * StandInPileScale.Z * (1.0 - 2.0 * StandInSink));
		Rubble->PileHeight = 100.f * static_cast<float>(StandInPileScale.Z) * (1.f - StandInSink);
	}

	// Loose clods round it, half sunk.
	UStaticMesh* LooseMesh = GravemotherAssets::Load<UStaticMesh>(GravemotherAssets::ClodSmall);
	const float FirstAngle = FMath::FRandRange(0.f, 360.f);
	for (int32 Index = 0; Index < LooseCount; ++Index)
	{
		UStaticMeshComponent* Clod = NewObject<UStaticMeshComponent>(Rubble, NAME_None, RF_Transient);
		Clod->SetupAttachment(Rubble->Root);
		Clod->SetMobility(EComponentMobility::Movable);
		Clod->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Clod->SetCanEverAffectNavigation(false);
		const float Angle = FirstAngle + 360.f * static_cast<float>(Index) / LooseCount + FMath::FRandRange(-25.f, 25.f);
		const FVector Out = FVector::ForwardVector.RotateAngleAxis(Angle, FVector::UpVector) * FMath::FRandRange(LooseMin, LooseMax);
		const FRotator Turn(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(-20.f, 20.f));
		if (LooseMesh)
		{
			Clod->SetStaticMesh(LooseMesh);
			Clod->SetRelativeTransform(FTransform(Turn, Out - FVector(0.0, 0.0, LooseSink), FVector(FMath::FRandRange(0.7f, 1.f))));
		}
		else
		{
			Clod->SetStaticMesh(GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Sphere));
			Clod->SetMaterial(0, GravemotherAssets::StandInLook(Rubble, GravemotherAssets::EarthColor()));
			Clod->SetRelativeTransform(FTransform(Turn, Out, FVector(StandInLooseSize)));
		}
		Clod->RegisterComponent();
		Rubble->Loose.Add(Clod);
	}
	Rubble->Scar = GravemotherAssets::AddGroundDecal(*Rubble, GravemotherAssets::DecalImpactScar, InGround, FMath::FRandRange(0.f, 360.f), ScarSize,
		ScarSize, 2, 200.f);
	GravemotherAssets::SetDecalOpacity(Rubble->Scar, 1.f);
	if (!Rubble->HasActorBegunPlay())
	{
		Rubble->DispatchBeginPlay();
	}
	Rubble->Advance(0.f);
	return Rubble;
}

void AGravemotherRubble::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

bool AGravemotherRubble::Advance(float DeltaSeconds)
{
	if (bSettled || IsActorBeingDestroyed())
	{
		return false;
	}
	Age += FMath::Max(DeltaSeconds, 0.f);
	// It pops up out of the burst, stands, and sinks back into the ground over its last moments.
	const float Rise = FMath::InterpEaseOut(0.3f, 1.f, FMath::Clamp(Age / RiseSeconds, 0.f, 1.f), 2.f);
	const float Settle = FMath::Clamp((Age - Seconds) / SettleSeconds, 0.f, 1.f);
	const float Sink = FMath::SmoothStep(0.f, 1.f, Settle) * PileHeight;
	Pile->SetRelativeTransform(FTransform(FQuat::Identity, PileOffset - FVector(0.0, 0.0, Sink), FVector(PileScale.X, PileScale.Y, PileScale.Z * Rise)));
	for (UStaticMeshComponent* Clod : Loose)
	{
		if (Clod)
		{
			Clod->SetVisibility(Settle < 0.6f);
		}
	}
	GravemotherAssets::SetDecalOpacity(Scar, 1.f - Settle);
	if (Settle > 0.f && Age - DeltaSeconds < Seconds)
	{
		// It settles: a last breath of dust as it sinks.
		if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
		{
			FGraveDustBurst Dust;
			Dust.Count = 5;
			Dust.Spread = 70.f;
			Dust.StartSize = 60.f;
			Dust.EndSize = 140.f;
			Dust.Life = 1.2f;
			Dust.Opacity = 0.22f;
			Dust.Outward = 60.f;
			Effects->Puffs(Ground, Dust);
		}
	}
	ApplyCollision();
	if (Settle >= 1.f)
	{
		bSettled = true;
		SetActorTickEnabled(false);
		Destroy();
		return false;
	}
	return true;
}

bool AGravemotherRubble::IsPlayerInside() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const FBox Box = Pile->Bounds.GetBox();
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		const UCapsuleComponent* Capsule = It->GetCapsuleComponent();
		if (!Capsule)
		{
			continue;
		}
		const FBox Body = FBox::BuildAABB(Capsule->GetComponentLocation(),
			FVector(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()));
		if (Box.Intersect(Body))
		{
			return true;
		}
	}
	return false;
}

void AGravemotherRubble::ApplyCollision()
{
	if (IsSettling())
	{
		if (Pile->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			Pile->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		bBlocksPawns = false;
		return;
	}
	// A player it came down on isn't trapped in it: it stops pawns once none is inside.
	if (!bBlocksPawns && !IsPlayerInside())
	{
		bBlocksPawns = true;
		Pile->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
}
