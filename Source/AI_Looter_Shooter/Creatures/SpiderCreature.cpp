#include "Creatures/SpiderCreature.h"
#include "Combat/HealthComponent.h"
#include "Environment/StylizedMeshKit.h"
#include "Environment/StylizedSurface.h"
#include "Components/CapsuleComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/SphereComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "UDynamicMesh.h"

using namespace StylizedMesh;
using StylizedColors::Hex;

namespace
{
	// Material slots shared by every body part.
	namespace SpiderSlot
	{
		constexpr int32 Body = 0;
		constexpr int32 Mark = 1;
		constexpr int32 Belly = 2;
		constexpr int32 Eye = 3;
	}

	// Leg layout per pair, front (0) to back (3). Angles are degrees from straight ahead.
	constexpr float HipAngle[4] = { 38.f, 72.f, 106.f, 140.f };
	constexpr float RestAngle[4] = { 40.f, 74.f, 108.f, 146.f };
	constexpr float RestRadius[4] = { 170.f, 152.f, 150.f, 172.f };
	constexpr float FemurLength[4] = { 98.f, 88.f, 88.f, 100.f };
	constexpr float TibiaLength[4] = { 122.f, 110.f, 110.f, 126.f };
	constexpr float FemurRadius = 9.5f;
	constexpr float TibiaRadius = 8.f;

	void SetupHitVolume(UPrimitiveComponent* Shape, FName Tag)
	{
		// Only weapon traces (and Build Mode's picking/visibility traces) see the body parts; they never
		// block movement, loot, or anything else.
		Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Shape->SetCollisionObjectType(ECC_WorldDynamic);
		Shape->SetCollisionResponseToAllChannels(ECR_Ignore);
		Shape->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
		Shape->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Shape->SetGenerateOverlapEvents(false);
		Shape->SetCanEverAffectNavigation(false);
		Shape->ComponentTags.Add(Tag);
	}

	UDynamicMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, FName Name)
	{
		UDynamicMeshComponent* Part = Owner->CreateDefaultSubobject<UDynamicMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		return Part;
	}

	/** Builds geometry into a scratch mesh, then hands it to the component in one update. */
	void BuildPart(UObject* Outer, UDynamicMeshComponent* Component, float SmoothAngle, TFunctionRef<void(UDynamicMesh*)> Build)
	{
		UDynamicMesh* Scratch = NewObject<UDynamicMesh>(Outer, NAME_None, RF_Transient);
		Build(Scratch);
		FinishNormals(Scratch, SmoothAngle);
		UE::Geometry::FDynamicMesh3 Result;
		Scratch->ProcessMesh([&Result](const UE::Geometry::FDynamicMesh3& Built) { Result = Built; });
		Component->SetMesh(MoveTemp(Result));
	}

	FRotator PointZ(const FVector& Direction)
	{
		return FRotationMatrix::MakeFromZ(Direction.GetSafeNormal()).Rotator();
	}

	/** Coarse hairs lying toward the tip of a limb segment built along +X. */
	void AddBristles(UDynamicMesh* Mesh, FRandomStream& Random, float Length, float Radius, int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float X = Length * Random.FRandRange(0.15f, 0.9f);
			const float Around = FMath::DegreesToRadians(Random.FRandRange(-110.f, 110.f)); // mostly top and sides
			const FVector Normal(0.f, FMath::Sin(Around), FMath::Cos(Around));
			const FVector Direction = Normal * 0.6f + FVector(0.75f, 0.f, 0.f);
			Cone(Mesh, SpiderSlot::Mark, FTransform(PointZ(Direction), FVector(X, 0.f, 0.f) + Normal * Radius * 0.85f), 1.3f, 0.f, Random.FRandRange(7.f, 11.f), 3, 0);
		}
	}

	void BuildFemur(UDynamicMesh* Mesh, float Length, FRandomStream& Random)
	{
		Ball(Mesh, SpiderSlot::Belly, FTransform(FVector::ZeroVector), 9.5f, 4, 6);
		Cone(Mesh, SpiderSlot::Body, FTransform(AlongX()), 8.5f, 6.5f, Length, 7, 2);
		Cylinder(Mesh, SpiderSlot::Mark, FTransform(AlongX(), FVector(Length * 0.62f, 0.f, 0.f)), 7.6f, Length * 0.14f, 7);
		AddBristles(Mesh, Random, Length, 7.5f, 6);
	}

	void BuildTibia(UDynamicMesh* Mesh, float Length, FRandomStream& Random)
	{
		const float Shaft = Length * 0.92f;
		Ball(Mesh, SpiderSlot::Belly, FTransform(FVector::ZeroVector), 8.f, 4, 6);
		Cone(Mesh, SpiderSlot::Body, FTransform(AlongX()), 6.5f, 2.6f, Shaft, 7, 2);
		Cylinder(Mesh, SpiderSlot::Mark, FTransform(AlongX(), FVector(Length * 0.3f, 0.f, 0.f)), 6.1f, Length * 0.1f, 7);
		Cylinder(Mesh, SpiderSlot::Mark, FTransform(AlongX(), FVector(Length * 0.65f, 0.f, 0.f)), 4.6f, Length * 0.08f, 7);
		Cone(Mesh, SpiderSlot::Mark, FTransform(AlongX(), FVector(Shaft, 0.f, 0.f)), 2.6f, 0.4f, Length - Shaft, 5, 0);
		AddBristles(Mesh, Random, Length * 0.8f, 5.5f, 5);
	}

	/**
	 * Two-bone IK: places the knee so both segments keep their length, bending toward Pole.
	 * Unreachable targets are clamped along the hip-to-target line.
	 */
	void SolveTwoBone(const FVector& Hip, const FVector& Target, float Upper, float Lower, const FVector& Pole, FVector& OutKnee, FVector& OutFoot)
	{
		const FVector ToTarget = Target - Hip;
		float Distance = ToTarget.Size();
		const FVector Direction = Distance > KINDA_SMALL_NUMBER ? ToTarget / Distance : FVector::DownVector;
		Distance = FMath::Clamp(Distance, FMath::Abs(Upper - Lower) + 1.f, (Upper + Lower) * 0.999f);
		OutFoot = Hip + Direction * Distance;

		const float Along = (Upper * Upper - Lower * Lower + Distance * Distance) / (2.f * Distance);
		const float Height = FMath::Sqrt(FMath::Max(Upper * Upper - Along * Along, 0.f));
		FVector Bend = Pole - Direction * FVector::DotProduct(Pole, Direction);
		if (!Bend.Normalize())
		{
			Bend = FVector::UpVector;
		}
		OutKnee = Hip + Direction * Along + Bend * Height;
	}
}

ASpiderCreature::ASpiderCreature()
{
	DisplayName = FText::FromString(TEXT("Brown Spider"));
	Health->MaxHealth = 150.f;
	// The head is the critical spot; legs, thorax, abdomen and fangs take base damage.
	CriticalSpotTags = { TEXT("Head") };

	WalkSpeed = 170.f;
	ChaseSpeed = 540.f;
	AttackRange = 220.f;
	AttackDamage = 12.f;
	HealthBarHeight = 120.f;

	BodyColor = Hex(0x6e4a2c);
	MarkingColor = Hex(0x33200f);
	BellyColor = Hex(0xb88c5c);

	GetCapsuleComponent()->InitCapsuleSize(62.f, 62.f);

	BodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BodyRoot"));
	BodyRoot->SetupAttachment(GetCapsuleComponent());
	BodyRoot->SetRelativeLocation(FVector(0.f, 0.f, RideHeight - 62.f));

	ThoraxMesh = MakePart(this, BodyRoot, TEXT("Thorax"));
	HeadMesh = MakePart(this, BodyRoot, TEXT("Head"));
	FangLeft = MakePart(this, BodyRoot, TEXT("FangLeft"));
	FangLeft->SetRelativeLocation(FVector(58.f, -8.f, -2.f));
	FangRight = MakePart(this, BodyRoot, TEXT("FangRight"));
	FangRight->SetRelativeLocation(FVector(58.f, 8.f, -2.f));

	AbdomenPivot = CreateDefaultSubobject<USceneComponent>(TEXT("AbdomenPivot"));
	AbdomenPivot->SetupAttachment(BodyRoot);
	AbdomenPivot->SetRelativeLocation(FVector(-30.f, 0.f, 6.f));
	AbdomenMesh = MakePart(this, AbdomenPivot, TEXT("Abdomen"));

	HeadHit = CreateDefaultSubobject<USphereComponent>(TEXT("HeadHit"));
	HeadHit->SetupAttachment(BodyRoot);
	HeadHit->InitSphereRadius(25.f);
	HeadHit->SetRelativeLocation(FVector(44.f, 0.f, 10.f));
	SetupHitVolume(HeadHit, TEXT("Head"));

	ThoraxHit = CreateDefaultSubobject<USphereComponent>(TEXT("ThoraxHit"));
	ThoraxHit->SetupAttachment(BodyRoot);
	ThoraxHit->InitSphereRadius(32.f);
	SetupHitVolume(ThoraxHit, TEXT("Body"));

	AbdomenHit = CreateDefaultSubobject<UCapsuleComponent>(TEXT("AbdomenHit"));
	AbdomenHit->SetupAttachment(AbdomenPivot);
	AbdomenHit->InitCapsuleSize(42.f, 66.f);
	AbdomenHit->SetRelativeLocationAndRotation(FVector(-52.f, 0.f, 12.f), FRotator(90.f, 0.f, 0.f));
	SetupHitVolume(AbdomenHit, TEXT("Abdomen"));

	// The chelicerae and fangs hang below and ahead of the head, out of reach of the head and thorax shapes. Each
	// capsule rides on its fang mesh, so it follows the fangs as they spread and snap shut.
	for (UDynamicMeshComponent* Fang : { FangLeft.Get(), FangRight.Get() })
	{
		// Fang space (see BuildMeshes): from the top of the chelicera down past the fang tip.
		const FVector Top(1.5f, 0.f, -2.5f);
		const FVector Bottom(4.f, 0.f, -22.f);
		constexpr float Radius = 8.f;
		UCapsuleComponent* Shape = CreateDefaultSubobject<UCapsuleComponent>(FName(*(Fang->GetName() + TEXT("Hit"))));
		Shape->SetupAttachment(Fang);
		Shape->InitCapsuleSize(Radius, FVector::Dist(Top, Bottom) * 0.5f + Radius);
		Shape->SetRelativeLocationAndRotation((Top + Bottom) * 0.5f, FRotationMatrix::MakeFromZX(Top - Bottom, FVector::ForwardVector).Rotator());
		SetupHitVolume(Shape, TEXT("Fang"));
		FangHits.Add(Shape);
	}

	for (int32 Pair = 0; Pair < 4; ++Pair)
	{
		for (const float Side : { -1.f, 1.f })
		{
			FLeg Leg;
			Leg.Side = Side;
			Leg.Pair = Pair;
			// Alternating tetrapod: L0 R1 L2 R3 move together, then R0 L1 R2 L3.
			Leg.Group = (Pair + (Side > 0.f ? 1 : 0)) % 2;
			const float Hip = FMath::DegreesToRadians(HipAngle[Pair]);
			const float Rest = FMath::DegreesToRadians(RestAngle[Pair]);
			Leg.Hip = FVector(FMath::Cos(Hip) * 30.f, Side * FMath::Sin(Hip) * 26.f, -2.f);
			Leg.Rest = FVector(FMath::Cos(Rest) * RestRadius[Pair], Side * FMath::Sin(Rest) * RestRadius[Pair], 0.f);
			Leg.FemurLength = FemurLength[Pair];
			Leg.TibiaLength = TibiaLength[Pair];
			Legs.Add(Leg);

			const FString Prefix = FString::Printf(TEXT("Leg%d%s"), Pair, Side < 0.f ? TEXT("L") : TEXT("R"));
			for (const TCHAR* Part : { TEXT("Femur"), TEXT("Tibia") })
			{
				LegMeshes.Add(MakePart(this, BodyRoot, FName(*(Prefix + Part))));
				UCapsuleComponent* Shape = CreateDefaultSubobject<UCapsuleComponent>(FName(*(Prefix + Part + TEXT("Hit"))));
				Shape->SetupAttachment(BodyRoot);
				SetupHitVolume(Shape, TEXT("Leg"));
				LegHits.Add(Shape);
			}
		}
	}
}

void ASpiderCreature::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildMeshes();
}

void ASpiderCreature::BeginPlay()
{
	Super::BeginPlay();

	// IK keeps every segment its exact length, so the leg hit shapes never need resizing.
	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		LegHits[Index * 2]->SetCapsuleSize(FemurRadius, Legs[Index].FemurLength * 0.5f + FemurRadius);
		LegHits[Index * 2 + 1]->SetCapsuleSize(TibiaRadius, Legs[Index].TibiaLength * 0.5f + TibiaRadius);
	}

	PlantLegs();
	// Pose immediately so it never shows a frame in the default pose (and previews look right).
	AnimateBody(0.f);
	AnimateLegs(0.f);
}

void ASpiderCreature::BuildMeshes()
{
	TArray<FStylizedSurface> Surfaces;
	Surfaces.Add(FStylizedSurface::Solid(BodyColor, 0.14f));
	Surfaces.Add(FStylizedSurface::Solid(MarkingColor, 0.08f));
	Surfaces.Add(FStylizedSurface::Solid(BellyColor, 0.1f));
	FStylizedSurface Eyes = FStylizedSurface::Solid(Hex(0x3a1606), 0.f);
	Eyes.Glow = 6.f; // a dim ember glint so the eyes read at range
	Surfaces.Add(Eyes);

	const TArray<UMaterialInterface*> Materials = StylizedSurfaces::CreateMaterials(this, Surfaces);
	BodyMaterials.Reset();
	for (UMaterialInterface* Material : Materials)
	{
		BodyMaterials.Add(Material);
	}

	FRandomStream Random(1847);

	BuildPart(this, ThoraxMesh, 60.f, [&](UDynamicMesh* Geometry)
	{
		Ball(Geometry, SpiderSlot::Body, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.3f, 1.05f, 0.62f)), 34.f, 7, 12);
		Ball(Geometry, SpiderSlot::Mark, FTransform(FRotator::ZeroRotator, FVector(-2.f, 0.f, 15.f), FVector(1.25f, 0.32f, 0.28f)), 28.f, 5, 8);
		for (const float Side : { -1.f, 1.f })
		{
			Ball(Geometry, SpiderSlot::Belly, FTransform(FRotator(0.f, Side * 8.f, 0.f), FVector(-4.f, Side * 22.f, 6.f), FVector(1.1f, 0.25f, 0.3f)), 26.f, 4, 8);
		}
	});

	BuildPart(this, HeadMesh, 60.f, [&](UDynamicMesh* Geometry)
	{
		Ball(Geometry, SpiderSlot::Body, FTransform(FRotator(-8.f, 0.f, 0.f), FVector(36.f, 0.f, 9.f), FVector(0.95f, 1.f, 0.8f)), 25.f, 6, 10);
		Ball(Geometry, SpiderSlot::Mark, FTransform(FRotator::ZeroRotator, FVector(34.f, 0.f, 24.f), FVector(1.f, 0.35f, 0.2f)), 16.f, 4, 6);

		// Wolf-spider eyes: a big forward pair, a row of four small ones below, a pair on top.
		struct FEye { FVector Location; float Radius; };
		const FEye EyeLayout[] = {
			{ FVector(56.f, 8.f, 17.f), 5.5f }, { FVector(56.f, -8.f, 17.f), 5.5f },
			{ FVector(59.f, 4.f, 9.f), 2.8f }, { FVector(59.f, -4.f, 9.f), 2.8f },
			{ FVector(57.f, 11.f, 9.f), 2.5f }, { FVector(57.f, -11.f, 9.f), 2.5f },
			{ FVector(46.f, 11.f, 25.f), 4.f }, { FVector(46.f, -11.f, 25.f), 4.f } };
		for (const FEye& Eye : EyeLayout)
		{
			Ball(Geometry, SpiderSlot::Eye, FTransform(Eye.Location), Eye.Radius, 4, 6);
		}

		// Pedipalps: short two-part feelers reaching forward and down.
		for (const float Side : { -1.f, 1.f })
		{
			const FVector Start(54.f, Side * 13.f, 2.f);
			const FVector First = FVector(0.8f, Side * 0.25f, -0.55f).GetSafeNormal();
			const FVector Elbow = Start + First * 26.f;
			const FVector Second = FVector(0.5f, Side * 0.1f, -0.85f).GetSafeNormal();
			Cone(Geometry, SpiderSlot::Body, FTransform(PointZ(First), Start), 4.5f, 3.f, 26.f, 6, 0);
			Cone(Geometry, SpiderSlot::Body, FTransform(PointZ(Second), Elbow), 3.f, 2.2f, 22.f, 6, 0);
			Ball(Geometry, SpiderSlot::Mark, FTransform(Elbow + Second * 22.f), 3.2f, 3, 5);
		}
	});

	for (UDynamicMeshComponent* Fang : { FangLeft.Get(), FangRight.Get() })
	{
		BuildPart(this, Fang, 60.f, [&](UDynamicMesh* Geometry)
		{
			// Chelicera with a curved fang hooking inward underneath.
			Ball(Geometry, SpiderSlot::Body, FTransform(FRotator::ZeroRotator, FVector(2.f, 0.f, -6.f), FVector(0.9f, 0.85f, 1.35f)), 8.f, 5, 8);
			Cone(Geometry, SpiderSlot::Mark, FTransform(PointZ(FVector(0.35f, 0.f, -1.f)), FVector(4.f, 0.f, -15.f)), 3.2f, 0.3f, 13.f, 5, 0);
		});
	}

	BuildPart(this, AbdomenMesh, 60.f, [&](UDynamicMesh* Geometry)
	{
		const FVector Center(-52.f, 0.f, 12.f);
		const FVector Radii(46.f * 1.35f, 46.f, 46.f * 0.88f);
		Ball(Geometry, SpiderSlot::Body, FTransform(FRotator::ZeroRotator, Center, FVector(1.35f, 1.f, 0.88f)), 46.f, 9, 14);
		Ball(Geometry, SpiderSlot::Belly, FTransform(FRotator::ZeroRotator, FVector(-50.f, 0.f, -4.f), FVector(1.2f, 0.78f, 0.5f)), 42.f, 6, 10);

		// Chevrons: pairs of dark marks angled down the back, shrinking toward the spinnerets.
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float X = -28.f - Index * 20.f;
			const float Along = (X - Center.X) / Radii.X;
			const float TopZ = Center.Z + Radii.Z * FMath::Sqrt(FMath::Max(0.f, 1.f - Along * Along));
			const float Size = 14.f - Index * 2.2f;
			for (const float Side : { -1.f, 1.f })
			{
				Ball(Geometry, SpiderSlot::Mark, FTransform(FRotator(0.f, Side * 35.f, 0.f), FVector(X, Side * Size * 0.55f, TopZ - 3.f), FVector(1.2f, 0.35f, 0.22f)), Size, 4, 6);
			}
		}
		// Heart mark up front, spinnerets at the back.
		Ball(Geometry, SpiderSlot::Mark, FTransform(FRotator::ZeroRotator, FVector(-14.f, 0.f, 44.f), FVector(1.1f, 0.3f, 0.2f)), 14.f, 4, 6);
		Cone(Geometry, SpiderSlot::Mark, FTransform(PointZ(FVector(-1.f, 0.f, -0.2f)), FVector(-110.f, 0.f, 8.f)), 6.f, 1.5f, 10.f, 6, 0);
		Displace(Geometry, 0, 1.2f, 1.f / 25.f, 7.f);
	});

	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		const FLeg& Leg = Legs[Index];
		BuildPart(this, LegMeshes[Index * 2], 50.f, [&](UDynamicMesh* Geometry) { BuildFemur(Geometry, Leg.FemurLength, Random); });
		BuildPart(this, LegMeshes[Index * 2 + 1], 50.f, [&](UDynamicMesh* Geometry) { BuildTibia(Geometry, Leg.TibiaLength, Random); });
	}

	for (UDynamicMeshComponent* Part : { ThoraxMesh.Get(), HeadMesh.Get(), FangLeft.Get(), FangRight.Get(), AbdomenMesh.Get() })
	{
		Part->ConfigureMaterialSet(Materials);
	}
	for (UDynamicMeshComponent* Part : LegMeshes)
	{
		Part->ConfigureMaterialSet(Materials);
	}
}

// ---------------------------------------------------------------------------
// Procedural animation
// ---------------------------------------------------------------------------

void ASpiderCreature::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AnimTime += DeltaSeconds;
	AnimateBody(DeltaSeconds);
	AnimateLegs(DeltaSeconds);
}

FVector ASpiderCreature::GroundUnder(const FVector& Point) const
{
	// Search only a little above the body: feet find footing on bumps and steps, but never climb walls.
	FVector Ground;
	if (FindGround(Point, 60.f, 400.f, Ground))
	{
		return Ground;
	}
	const float GroundZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return FVector(Point.X, Point.Y, GroundZ);
}

void ASpiderCreature::PlantLegs()
{
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	for (FLeg& Leg : Legs)
	{
		Leg.Foot = GroundUnder(GetActorLocation() + Yaw.RotateVector(Leg.Rest));
		Leg.bStepping = false;
		Leg.bNeedsReset = false;
		Leg.StepAlpha = 1.f;
	}
	bBodyInitialized = false;
}

void ASpiderCreature::AnimateBody(float DeltaSeconds)
{
	const FVector ActorLocation = GetActorLocation();
	const float GroundZ = ActorLocation.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const ECreatureState CurrentState = GetCreatureState();
	const float Time = GetStateTime();

	// The body settles over its planted feet: their average sets the height, the differences tilt it.
	float Sum = 0.f, Front = 0.f, Back = 0.f, Left = 0.f, Right = 0.f;
	for (const FLeg& Leg : Legs)
	{
		Sum += Leg.Foot.Z;
		(Leg.Pair <= 1 ? Front : Back) += Leg.Foot.Z;
		(Leg.Side > 0.f ? Right : Left) += Leg.Foot.Z;
	}
	const float Half = FMath::Max(1.f, Legs.Num() * 0.5f);
	float TargetZ = FMath::Clamp(Sum / (Half * 2.f) + RideHeight, GroundZ + 25.f, GroundZ + RideHeight + 45.f);
	float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2((Front - Back) / Half, 200.f));
	float TargetRoll = FMath::RadiansToDegrees(FMath::Atan2((Left - Right) / Half, 220.f));
	float Lunge = 0.f;
	float TargetFang = 3.f + FMath::Max(0.f, FMath::Sin(AnimTime * 3.1f)) * 4.f;

	if (CurrentState == ECreatureState::Attack)
	{
		// Rear up with fangs spread during the wind-up, then lunge and snap shut.
		const float Windup = FMath::Clamp(Time / AttackWindup, 0.f, 1.f);
		const float Strike = FMath::Clamp((Time - AttackWindup) / AttackRecovery, 0.f, 1.f);
		const float Rear = FMath::InterpEaseInOut(0.f, 1.f, Windup, 2.f) * (1.f - Strike);
		TargetPitch += 24.f * Rear - 12.f * FMath::Sin(Strike * UE_PI);
		TargetZ += 16.f * Rear;
		Lunge = -14.f * Rear + 30.f * FMath::Sin(Strike * UE_PI);
		TargetFang = Strike > 0.f ? 0.f : 30.f * Windup;
	}
	else if (CurrentState == ECreatureState::Dead)
	{
		// Collapse, then sink out of sight after the corpse time.
		const float Collapse = FMath::Clamp(Time / 0.6f, 0.f, 1.f);
		TargetZ = FMath::Lerp(TargetZ, GroundZ + 20.f, Collapse);
		TargetPitch = FMath::Lerp(TargetPitch, -5.f, Collapse);
		TargetRoll += 10.f * Collapse;
		TargetFang = 25.f;
		if (Time > CorpseTime)
		{
			TargetZ -= (Time - CorpseTime) * 45.f;
		}
	}
	else
	{
		TargetZ += FMath::Sin(AnimTime * 2.2f) * 1.5f; // breathing
	}

	if (!bBodyInitialized || DeltaSeconds <= 0.f)
	{
		BodyZ = TargetZ;
		BodyPitch = TargetPitch;
		BodyRoll = TargetRoll;
		FangOpen = TargetFang;
		bBodyInitialized = true;
	}
	BodyZ = FMath::FInterpTo(BodyZ, TargetZ, DeltaSeconds, CurrentState == ECreatureState::Dead ? 6.f : 10.f);
	BodyPitch = FMath::FInterpTo(BodyPitch, TargetPitch, DeltaSeconds, 8.f);
	BodyRoll = FMath::FInterpTo(BodyRoll, TargetRoll, DeltaSeconds, 8.f);
	FangOpen = FMath::FInterpTo(FangOpen, TargetFang, DeltaSeconds, 14.f);
	HurtOffset = FMath::VInterpTo(HurtOffset, FVector::ZeroVector, DeltaSeconds, 10.f);
	AbdomenKick = FMath::FInterpTo(AbdomenKick, 0.f, DeltaSeconds, 8.f);

	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const FVector Location = FVector(ActorLocation.X, ActorLocation.Y, BodyZ) + Yaw.RotateVector(FVector(Lunge, 0.f, 0.f)) + HurtOffset;
	BodyRoot->SetWorldLocationAndRotation(Location, Yaw * FRotator(BodyPitch, 0.f, BodyRoll).Quaternion());

	// Abdomen: slow breathing sway plus a jolt when hit.
	const float Breath = FMath::Sin(AnimTime * 1.7f);
	AbdomenPivot->SetRelativeRotation(FRotator(Breath * 2.5f + AbdomenKick, FMath::Sin(AnimTime * 0.9f) * 2.f, 0.f));
	AbdomenPivot->SetRelativeScale3D(FVector(1.f, 1.f + Breath * 0.015f, 1.f + Breath * 0.02f));

	FangLeft->SetRelativeRotation(FRotator(FangOpen * 0.4f, -FangOpen, 0.f));
	FangRight->SetRelativeRotation(FRotator(FangOpen * 0.4f, FangOpen, 0.f));
}

void ASpiderCreature::AnimateLegs(float DeltaSeconds)
{
	const FTransform Body = BodyRoot->GetComponentTransform();
	const FVector BodyCenter = Body.GetLocation();
	const FVector Up = Body.GetRotation().GetUpVector();
	const FVector ActorLocation = GetActorLocation();
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const FVector Velocity(GetVelocity().X, GetVelocity().Y, 0.f);
	const float Speed = Velocity.Size();
	const ECreatureState CurrentState = GetCreatureState();
	const float Time = GetStateTime();

	// Faster = quicker, longer, higher steps.
	const float StepDuration = FMath::Clamp(0.26f - Speed * 0.00018f, 0.12f, 0.26f);
	const float StepThreshold = 32.f + Speed * 0.07f;
	const float StepHeight = 18.f + Speed * 0.025f;

	// Tetrapod gait: a group may only lift while the other group is planted.
	bool bGroupStepping[2] = { false, false };
	for (const FLeg& Leg : Legs)
	{
		bGroupStepping[Leg.Group] |= Leg.bStepping;
	}

	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		FLeg& Leg = Legs[Index];
		const FVector Hip = Body.TransformPosition(Leg.Hip);
		const FVector Outward = (Hip - BodyCenter).GetSafeNormal2D();
		FVector Pole = Up + Outward * 0.4f; // high, arched knees

		if (CurrentState == ECreatureState::Dead)
		{
			// Legs curl in under the body with the knees folded high.
			const FVector Curled = Body.TransformPosition(FVector(Leg.Hip.X * 0.6f + 10.f, Leg.Hip.Y * 1.6f, -8.f));
			Leg.Foot = FMath::VInterpTo(Leg.Foot, Curled, DeltaSeconds, 6.f);
			Leg.bStepping = false;
			Pole = Up * 1.5f + Outward * 0.2f;
		}
		else if (CurrentState == ECreatureState::Attack && Leg.Pair == 0)
		{
			// Front legs rise with the body, then slam down ahead of it on the strike.
			const float Strike = FMath::Clamp((Time - AttackWindup) / (AttackRecovery * 0.5f), 0.f, 1.f);
			const FVector Raised = Body.TransformPosition(FVector(Leg.Hip.X + 80.f, Leg.Hip.Y * 1.5f, 50.f));
			const FVector Slam = GroundUnder(ActorLocation + Yaw.RotateVector(FVector(150.f, Leg.Side * 50.f, 0.f)));
			Leg.Foot = DeltaSeconds > 0.f ? FMath::VInterpTo(Leg.Foot, FMath::Lerp(Raised, Slam, Strike), DeltaSeconds, 16.f) : Leg.Foot;
			Leg.bStepping = false;
			Leg.bNeedsReset = true;
		}
		else if (Leg.bStepping)
		{
			Leg.StepAlpha = FMath::Min(1.f, Leg.StepAlpha + DeltaSeconds / StepDuration);
			const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Leg.StepAlpha, 2.f);
			Leg.Foot = FMath::Lerp(Leg.StepFrom, Leg.StepTo, Eased) + FVector(0.f, 0.f, FMath::Sin(Leg.StepAlpha * UE_PI) * StepHeight);
			if (Leg.StepAlpha >= 1.f)
			{
				Leg.Foot = Leg.StepTo;
				Leg.bStepping = false;
				Leg.LastStepTime = AnimTime;
			}
		}
		else
		{
			// Where this foot wants to be: its rest spot, led by the velocity so it lands ahead of the body.
			const FVector Rest = ActorLocation + Yaw.RotateVector(Leg.Rest) + Velocity * (StepDuration * 0.9f);
			const float Offset = FVector::Dist2D(Leg.Foot, Rest);
			if (Offset > 450.f)
			{
				Leg.Foot = GroundUnder(Rest); // teleported (respawn, Build Mode move)
			}
			else
			{
				const bool bSettle = Speed < 10.f && Offset > 12.f && AnimTime - Leg.LastStepTime > 0.6f;
				if ((Offset > StepThreshold || bSettle || Leg.bNeedsReset) && !bGroupStepping[1 - Leg.Group])
				{
					Leg.StepFrom = Leg.Foot;
					Leg.StepTo = GroundUnder(Rest);
					Leg.StepAlpha = 0.f;
					Leg.bStepping = true;
					Leg.bNeedsReset = false;
					bGroupStepping[Leg.Group] = true;
				}
			}
		}

		FVector Knee;
		FVector Foot;
		SolveTwoBone(Hip, Leg.Foot, Leg.FemurLength, Leg.TibiaLength, Pole, Knee, Foot);
		PoseSegment(Index * 2, Hip, Knee, Pole, FemurRadius);
		PoseSegment(Index * 2 + 1, Knee, Foot, Pole, TibiaRadius);
	}
}

void ASpiderCreature::PoseSegment(int32 Segment, const FVector& From, const FVector& To, const FVector& Pole, float Radius)
{
	const FVector Axis = To - From;
	if (Axis.SizeSquared() < 1.f)
	{
		return;
	}
	// Meshes are modeled along +X; capsules run along their own Z.
	LegMeshes[Segment]->SetWorldLocationAndRotation(From, FRotationMatrix::MakeFromXZ(Axis, Pole).Rotator());
	LegHits[Segment]->SetWorldLocationAndRotation((From + To) * 0.5f, FRotationMatrix::MakeFromZX(Axis, Pole).Rotator());
}

// ---------------------------------------------------------------------------
// Reactions
// ---------------------------------------------------------------------------

void ASpiderCreature::OnAttackStarted()
{
	AbdomenKick = -6.f; // abdomen tips down as the front rears up
}

void ASpiderCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	// Flinch away from the hit; headshots rock it harder.
	HurtOffset = (BodyRoot->GetComponentLocation() - HitLocation).GetSafeNormal() * (bCritical ? 14.f : 7.f);
	AbdomenKick = bCritical ? 10.f : 6.f;
}

void ASpiderCreature::OnDied()
{
	for (FLeg& Leg : Legs)
	{
		Leg.bStepping = false;
	}
}

void ASpiderCreature::OnRespawned()
{
	PlantLegs();
	AnimateBody(0.f);
	AnimateLegs(0.f);
}

void ASpiderCreature::SetHitVolumesEnabled(bool bEnabled)
{
	const ECollisionEnabled::Type Mode = bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
	HeadHit->SetCollisionEnabled(Mode);
	ThoraxHit->SetCollisionEnabled(Mode);
	AbdomenHit->SetCollisionEnabled(Mode);
	for (UCapsuleComponent* Shape : FangHits)
	{
		Shape->SetCollisionEnabled(Mode);
	}
	for (UCapsuleComponent* Shape : LegHits)
	{
		Shape->SetCollisionEnabled(Mode);
	}
}
