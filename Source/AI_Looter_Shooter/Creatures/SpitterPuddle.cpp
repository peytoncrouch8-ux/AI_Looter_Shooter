#include "Creatures/SpitterPuddle.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/CombatRules.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/EnemyShotDamageType.h"
#include "Creatures/SpitterGlob.h"
#include "Creatures/SpitterRules.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The engine sphere is 100 cm across: the scale that makes it Radius round. */
	float SphereScale(float Radius)
	{
		return Radius / 50.f;
	}

	/** Its pool is a few flattened blobs: the main one, and these round it (a splash's ragged edge). */
	constexpr int32 SideBlobs = 5;
	/** The pool's and the core's thickness (cm): domes that stand just proud of the ground. */
	constexpr float PoolThickness = 2.2f;
	constexpr float CoreThickness = 3.2f;
	/** A side blob lies on the ground under it unless that ground is farther than this off the middle's plane (a step, a hole). */
	constexpr float BlobLevel = 35.f;
	/** Bubbles rising in it at once. */
	constexpr int32 NumBubbles = 6;

	/** How its parts glow (M_StylizedSurface's emissive multiplier): the pool dark, the core and bubbles bright venom. */
	constexpr float PoolGlow = 1.2f;
	constexpr float CoreGlow = 3.f;
	constexpr float BubbleGlow = 5.f;
	/** The pool is the venom's color this dark. */
	constexpr float PoolShade = 0.22f;
}

ASpitterPuddle::ASpitterPuddle()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
	GlowMaterial = Glow.Object;
	auto MakeLayer = [this, Root](const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* Layer = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Layer->SetupAttachment(Root);
		Layer->SetMobility(EComponentMobility::Movable);
		Layer->SetStaticMesh(SphereMesh.Object);
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetGenerateOverlapEvents(false);
		Layer->SetCastShadow(false);
		Layer->SetCanEverAffectNavigation(false);
		Layer->bReceivesDecals = false;
		return Layer;
	};
	Pool = MakeLayer(TEXT("Pool"));
	Core = MakeLayer(TEXT("Core"));
	Bubbles = MakeLayer(TEXT("Bubbles"));
}

ASpitterPuddle* ASpitterPuddle::Spawn(UWorld* World, const FVector& Ground, const FVector& Normal, const FSpitterPuddleSpec& Spec)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	Params.bDeferConstruction = true;
	ASpitterPuddle* Puddle = World->SpawnActor<ASpitterPuddle>(ASpitterPuddle::StaticClass(), FTransform(Ground), Params);
	if (!Puddle)
	{
		return nullptr;
	}
	Puddle->Spec = Spec;
	Puddle->Spec.Radius = FMath::Max(Spec.Radius, 10.f);
	Puddle->Spec.Seconds = FMath::Max(Spec.Seconds, 0.5f);
	Puddle->Spec.Every = FMath::Max(Spec.Every, 0.1f);
	Puddle->Random.Initialize(static_cast<int32>(GetTypeHash(Ground)));
	Puddle->FinishSpawning(FTransform(Ground));
	Puddle->Lay(Normal);
	// A test level never begins its actors' play: the puddle starts as play would start it.
	if (!Puddle->HasActorBegunPlay())
	{
		Puddle->DispatchBeginPlay();
	}
	Puddle->Draw(0.f);
	LooterSound::PlayAt(Puddle, LooterSoundCue::Spitter::Sizzle, Ground);
	return Puddle;
}

void ASpitterPuddle::BeginPlay()
{
	Super::BeginPlay();
	// The look: M_StylizedSurface's opaque glow, as the enemy pellets wear it, one instance per part.
	if (UMaterialInterface* Base = GlowMaterial.Get())
	{
		auto Look = [Base, this](UInstancedStaticMeshComponent* Layer, const FLinearColor& Color, float Glow)
		{
			UMaterialInstanceDynamic* Made = UMaterialInstanceDynamic::Create(Base, this);
			Made->SetVectorParameterValue(TEXT("Color"), Color);
			Made->SetScalarParameterValue(TEXT("Glow"), Glow);
			Made->SetScalarParameterValue(TEXT("Variation"), 0.f);
			Layer->SetMaterial(0, Made);
			return Made;
		};
		PoolLook = Look(Pool, Spec.Color * PoolShade, PoolGlow);
		CoreLook = Look(Core, Spec.Color, CoreGlow);
		BubbleLook = Look(Bubbles, FMath::Lerp(Spec.Color, FLinearColor::White, 0.25f), BubbleGlow);
	}
	// The first burn comes a moment after it lands: the glob's own splash has just hurt anyone it caught.
	BurnIn = Spec.Every * 0.5f;
}

// ---------------------------------------------------------------------------
// Laying it on the ground
// ---------------------------------------------------------------------------

void ASpitterPuddle::Lay(const FVector& Normal)
{
	bLaid = true;
	Up = Normal.GetSafeNormal();
	if (Up.Z < 0.5)
	{
		Up = FVector::UpVector;
	}
	Flat = FQuat::FindBetweenNormals(FVector::UpVector, Up);
	const FVector Middle = GetActorLocation();
	const float Radius = Spec.Radius;

	// The main blob in the middle, a little short of the puddle's reach (its side blobs make the ragged edge).
	PoolBlobs.Reset();
	CoreBlobs.Reset();
	PoolBlobs.Add({ Middle, Flat, Radius * 0.72f });
	const float Turn = Random.FRandRange(0.f, 360.f);
	for (int32 Index = 0; Index < SideBlobs; ++Index)
	{
		const float Angle = Turn + 360.f * static_cast<float>(Index) / SideBlobs + Random.FRandRange(-20.f, 20.f);
		const FVector Out = Flat.RotateVector(FVector::ForwardVector.RotateAngleAxis(Angle, FVector::UpVector));
		const float Size = Radius * Random.FRandRange(0.3f, 0.48f);
		const FVector Spot = Middle + Out * (Radius - Size) * Random.FRandRange(0.8f, 1.f);
		FBlob Blob;
		if (LayBlob(Spot, Size, Blob))
		{
			PoolBlobs.Add(Blob);
		}
	}
	// The brighter core: off the middle a little, and a drop of it beside.
	FBlob Heart;
	const FVector Drift = Flat.RotateVector(FVector(Random.FRandRange(-0.12f, 0.12f), Random.FRandRange(-0.12f, 0.12f), 0.f)) * Radius;
	if (LayBlob(Middle + Drift, Radius * 0.42f, Heart))
	{
		CoreBlobs.Add(Heart);
	}
	FBlob Drop;
	const FVector Aside = Flat.RotateVector(FVector::ForwardVector.RotateAngleAxis(Turn + 150.f, FVector::UpVector)) * Radius * 0.5f;
	if (LayBlob(Middle + Aside, Radius * 0.16f, Drop))
	{
		CoreBlobs.Add(Drop);
	}

	BubbleState.SetNum(NumBubbles);
	for (FBubble& Bubble : BubbleState)
	{
		NewBubble(Bubble);
		// Not all at once: each starts part way through its life.
		Bubble.Age = Random.FRandRange(0.f, Bubble.Life);
	}
}

bool ASpitterPuddle::LayBlob(const FVector& Spot, float Radius, FBlob& Out) const
{
	UWorld* World = GetWorld();
	const FVector Middle = GetActorLocation();
	Out.Radius = Radius;
	Out.Location = Spot;
	Out.Rotation = Flat;
	if (!World)
	{
		return true;
	}
	// Its own ground, found the way a glob meets the world (rocks and props too, not volumes); on the middle's plane
	// where there's none (a test level's flat nothing, an edge it hangs over a little).
	FHitResult Hit;
	const FCollisionQueryParams Query = ASpitterGlob::MakeWorldQuery(World, this);
	if (World->LineTraceSingleByObjectType(Hit, Spot + Up * 60.f, Spot - Up * 120.f, FCollisionObjectQueryParams(ECC_WorldStatic), Query)
		&& !Hit.bStartPenetrating)
	{
		const float Off = static_cast<float>(FVector::DotProduct(Hit.ImpactPoint - Middle, Up));
		if (FMath::Abs(Off) > BlobLevel || Hit.ImpactNormal.Z < 0.5)
		{
			return false;
		}
		Out.Location = Hit.ImpactPoint;
		Out.Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Hit.ImpactNormal);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Burning, drying, drawing
// ---------------------------------------------------------------------------

void ASpitterPuddle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (IsDry())
	{
		Destroy();
		return;
	}
	BurnIn -= DeltaSeconds;
	if (BurnIn <= 0.f)
	{
		BurnIn += Spec.Every;
		Burn();
	}
	Draw(DeltaSeconds);
}

float ASpitterPuddle::GetSpread() const
{
	return SpitterRules::PuddleSpread(Age, Spec.Seconds);
}

bool ASpitterPuddle::Touches(const ACharacter& Body) const
{
	const UCapsuleComponent* Capsule = Body.GetCapsuleComponent();
	if (!Capsule || GetSpread() <= 0.f)
	{
		return false;
	}
	const FVector Feet = Capsule->GetComponentLocation() - FVector(0.0, 0.0, Capsule->GetScaledCapsuleHalfHeight());
	// Its reach shrinks with it as it dries, so the last moment of a puddle doesn't burn from bare ground.
	return SpitterRules::PuddleTouches(GetActorLocation(), Spec.Radius * FMath::Max(GetSpread(), 0.35f), Feet,
		Capsule->GetScaledCapsuleRadius());
}

void ASpitterPuddle::Burn()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// Only players: the same rule as the pellets', so creatures (its own Spitter too) walk through it.
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Body = *It;
		if (!Body || !UEnemyProjectileSubsystem::CanHurt(Spec.Spitter.Get(), Body) || !Touches(*Body))
		{
			continue;
		}
		++Burns;
		const float Damage = LooterCombat::RollHitDamage(Spec.Damage, false);
		const FVector Feet = Body->GetActorLocation() - FVector(0.0, 0.0, Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		LooterSound::PlayAt(this, LooterSoundCue::Spitter::Burn, Feet);
		UE_LOG(LogLooter, Verbose, TEXT("%s burned %s for %.1f"), *GetName(), *GetNameSafe(Body), Damage);
		// Its own damage, from the puddle under them (the hit's direction points down at it, not off at the Spitter).
		UGameplayStatics::ApplyDamage(Body, Damage, Spec.Instigator.Get(), this, UEnemyShotDamageType::StaticClass());
	}
}

void ASpitterPuddle::NewBubble(FBubble& Bubble)
{
	const FVector2D Spot = FMath::RandPointInCircle(Spec.Radius * 0.6f);
	Bubble.Offset = Spot;
	Bubble.Age = 0.f;
	Bubble.Life = Random.FRandRange(0.4f, 0.85f);
	Bubble.Size = Random.FRandRange(2.5f, 5.f) * FMath::Clamp(Spec.Radius / 130.f, 0.6f, 1.6f);
}

void ASpitterPuddle::Draw(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	const float Spread = GetSpread();
	auto Layer = [Spread](UInstancedStaticMeshComponent* Component, const TArray<FBlob>& Blobs, float Thickness)
	{
		TArray<FTransform> Transforms;
		Transforms.Reserve(Blobs.Num());
		for (const FBlob& Blob : Blobs)
		{
			const float Across = SphereScale(Blob.Radius * Spread);
			Transforms.Emplace(Blob.Rotation, Blob.Location, FVector(Across, Across, SphereScale(Thickness * FMath::Max(Spread, 0.2f))));
		}
		if (Component->GetInstanceCount() != Transforms.Num())
		{
			Component->ClearInstances();
			Component->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
		}
		else
		{
			Component->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
		}
	};
	// The pool and the core only change while it spreads and dries.
	const bool bChanging = Age < 0.25f || Spread < 0.999f || Pool->GetInstanceCount() == 0;
	if (bChanging)
	{
		Layer(Pool, PoolBlobs, PoolThickness);
		Layer(Core, CoreBlobs, CoreThickness);
	}

	// Bubbles swell and pop, and new ones rise while it's fresh; none as it dries.
	TArray<FTransform> Rising;
	const FVector Middle = GetActorLocation();
	for (FBubble& Bubble : BubbleState)
	{
		Bubble.Age += DeltaSeconds;
		if (Bubble.Age >= Bubble.Life)
		{
			NewBubble(Bubble);
		}
		if (Spread < 0.6f)
		{
			continue;
		}
		const float Through = Bubble.Age / Bubble.Life;
		const float Size = Bubble.Size * FMath::Sqrt(Through) * Spread;
		const FVector Spot = Middle + Flat.RotateVector(FVector(Bubble.Offset.X, Bubble.Offset.Y, CoreThickness * 0.3f + Size * 0.4f));
		Rising.Emplace(Flat, Spot, FVector(SphereScale(Size)));
	}
	if (Bubbles->GetInstanceCount() != Rising.Num())
	{
		Bubbles->ClearInstances();
		Bubbles->AddInstances(Rising, false, true, false);
	}
	else if (Rising.Num() > 0)
	{
		Bubbles->BatchUpdateInstancesTransforms(0, Rising, true, true, true);
	}

	// It dims as it dries.
	if (PoolLook && Spread < 0.999f)
	{
		PoolLook->SetScalarParameterValue(TEXT("Glow"), PoolGlow * Spread);
		CoreLook->SetScalarParameterValue(TEXT("Glow"), CoreGlow * Spread);
	}
}
