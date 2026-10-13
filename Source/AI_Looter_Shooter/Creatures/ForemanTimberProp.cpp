#include "Creatures/ForemanTimberProp.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AForemanTimberProp::PropTag(TEXT("TimberProp"));
const TCHAR* const AForemanTimberProp::ModelPath = TEXT("/Game/Art/Props/SM_RR_MineProp.SM_RR_MineProp");
const TCHAR* const AForemanTimberProp::StumpPath = TEXT("/Game/Art/Props/SM_RR_MinePropStump.SM_RR_MinePropStump");

namespace
{
	const TCHAR* const CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/** The stand-in stump's height (cm): a jagged foot left standing. */
	constexpr float StumpHeight = 55.f;

	/** A crack's shudder: how long (s), how far it rocks (degrees) and how fast (Hz). */
	constexpr float ShudderSeconds = 0.6f;
	constexpr float ShudderDegrees = 2.5f;
	constexpr float ShudderHz = 14.f;

	/** Dust off old pine: dim (the smoke material is unlit), as a breakable crate's. */
	const FLinearColor DustColor(0.26f, 0.22f, 0.17f);

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

	FWeaponFX* EffectsIn(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}
}

AForemanTimberProp::AForemanTimberProp()
{
	// Ticks only while it shudders.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Tags.Add(PropTag);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static UStaticMesh* const PostModel = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const StumpModel = FindIfMade<UStaticMesh>(StumpPath);
	static UStaticMesh* const Cylinder = FindIfMade<UStaticMesh>(CylinderPath);
	bUsesStandIn = PostModel == nullptr;

	// Solid cover: the player, creatures and shots stop at it; world dynamic, so ground traces never land on it.
	Post = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Post"));
	Post->SetupAttachment(Root);
	Post->SetMobility(EComponentMobility::Movable);
	Post->SetStaticMesh(PostModel ? PostModel : Cylinder);
	Post->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Post->SetCanEverAffectNavigation(false);

	Stump = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stump"));
	Stump->SetupAttachment(Root);
	Stump->SetMobility(EComponentMobility::Movable);
	Stump->SetStaticMesh(StumpModel ? StumpModel : Cylinder);
	Stump->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Stump->SetCanEverAffectNavigation(false);
	Stump->SetVisibility(false);
	ApplyModels();
}

void AForemanTimberProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyModels();
}

void AForemanTimberProp::ApplyModels()
{
	if (bUsesStandIn)
	{
		// The engine's cylinder is 1 m across and tall, about its middle: stood on the prop's foot.
		Post->SetRelativeLocation(FVector(0.0, 0.0, Height * 0.5f));
		Post->SetRelativeScale3D(FVector(Width / 100.f, Width / 100.f, Height / 100.f));
	}
	if (Stump->GetStaticMesh() && Stump->GetStaticMesh()->GetName() == TEXT("Cylinder"))
	{
		Stump->SetRelativeLocation(FVector(0.0, 0.0, StumpHeight * 0.5f));
		Stump->SetRelativeScale3D(FVector(Width / 100.f, Width / 100.f, StumpHeight / 100.f));
	}
	PostRest = Post->GetRelativeTransform();
}

TArray<AForemanTimberProp*> AForemanTimberProp::FindNear(const UWorld* World, const FVector& Center, float Radius, bool bStandingOnly)
{
	TArray<AForemanTimberProp*> Found;
	if (!World)
	{
		return Found;
	}
	for (TActorIterator<AForemanTimberProp> It(const_cast<UWorld*>(World)); It; ++It)
	{
		AForemanTimberProp* Prop = *It;
		if (Prop && !Prop->IsActorBeingDestroyed() && (!bStandingOnly || Prop->IsStanding())
			&& FVector::Dist2D(Prop->GetActorLocation(), Center) <= Radius)
		{
			Found.Add(Prop);
		}
	}
	Found.Sort([&Center](const AForemanTimberProp& A, const AForemanTimberProp& B)
	{
		return FVector::DistSquared2D(A.GetActorLocation(), Center) < FVector::DistSquared2D(B.GetActorLocation(), Center);
	});
	return Found;
}

// ---------------------------------------------------------------------------
// Struck, splintered, mended
// ---------------------------------------------------------------------------

void AForemanTimberProp::Crack()
{
	if (bSplintered)
	{
		return;
	}
	// The Foreman's blow on it: it shudders, dust sifting off its cap.
	ShudderClock = 0.f;
	SetActorTickEnabled(true);
	if (FWeaponFX* Effects = EffectsIn(GetWorld()))
	{
		const FVector Cap = GetActorLocation() + FVector(0.0, 0.0, Height * 0.85f);
		for (int32 Puff = 0; Puff < 4; ++Puff)
		{
			Effects->SpawnDustPuff(Cap + FMath::VRand() * 15.f, FVector(0.0, 0.0, -60.0) + FMath::VRand() * 20.f, DustColor, 0.5f, 18.f, 70.f, 1.6f);
		}
	}
}

bool AForemanTimberProp::Splinter(const FVector& From)
{
	if (bSplintered)
	{
		return false;
	}
	bSplintered = true;
	ShudderClock = -1.f;
	SetActorTickEnabled(false);
	Post->SetRelativeTransform(PostRest);
	Post->SetVisibility(false);
	Post->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stump->SetVisibility(true);
	SplinterEffects(From);
	UE_LOG(LogLooter, Log, TEXT("%s splinters under the roof's rock."), *GetActorNameOrLabel());
	return true;
}

void AForemanTimberProp::SplinterEffects(const FVector& From)
{
	// A loud crack of splitting pine, splinters and grit thrown away from the rock, dust hanging where it stood.
	const FVector Middle = GetActorLocation() + FVector(0.0, 0.0, Height * 0.4f);
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::PropSplinter, Middle);
	FWeaponFX* Effects = EffectsIn(GetWorld());
	if (!Effects)
	{
		return;
	}
	FVector Away = Middle - From;
	Away.Z = 0.0;
	Away = Away.IsNearlyZero() ? FVector::ForwardVector : Away.GetSafeNormal();
	for (int32 Chip = 0; Chip < 14; ++Chip)
	{
		const FVector At = GetActorLocation() + FVector(0.0, 0.0, FMath::FRandRange(40.f, Height * 0.8f));
		const FVector Throw = (Away * FMath::FRandRange(120.f, 320.f)) + FMath::VRand() * 160.f + FVector(0.0, 0.0, FMath::FRandRange(60.f, 220.f));
		Effects->SpawnGrit(At, Throw, FMath::FRandRange(3.f, 7.f), FMath::FRandRange(1.2f, 2.2f));
	}
	for (int32 Puff = 0; Puff < 6; ++Puff)
	{
		const FVector At = GetActorLocation() + FVector(0.0, 0.0, FMath::FRandRange(20.f, Height * 0.6f));
		Effects->SpawnDustPuff(At, FMath::VRand() * 50.f, DustColor, 0.55f, 30.f, 140.f, 2.4f);
	}
}

void AForemanTimberProp::Mend()
{
	bSplintered = false;
	ShudderClock = -1.f;
	SetActorTickEnabled(false);
	Post->SetRelativeTransform(PostRest);
	Post->SetVisibility(true);
	Post->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Stump->SetVisibility(false);
}

// ---------------------------------------------------------------------------
// The shudder
// ---------------------------------------------------------------------------

void AForemanTimberProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AForemanTimberProp::Advance(float DeltaSeconds)
{
	if (ShudderClock < 0.f)
	{
		SetActorTickEnabled(false);
		return;
	}
	ShudderClock += DeltaSeconds;
	const float Left = 1.f - ShudderClock / ShudderSeconds;
	if (Left <= 0.f || bSplintered)
	{
		ShudderClock = -1.f;
		Post->SetRelativeTransform(PostRest);
		SetActorTickEnabled(false);
		return;
	}
	// Rocking on its foot, dying away.
	const float Angle = ShudderDegrees * Left * FMath::Sin(2.f * UE_PI * ShudderHz * ShudderClock);
	const FQuat Rock(FVector::ForwardVector, FMath::DegreesToRadians(Angle));
	const FVector Foot = FVector::ZeroVector;
	Post->SetRelativeLocationAndRotation(Foot + Rock.RotateVector(PostRest.GetLocation() - Foot), Rock * PostRest.GetRotation());
}
