// ALanternFlame: the lit Keeper's Lantern's flame, leaning toward the next saint's light.

#include "World/LanternFlame.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/KeeperLanternPost.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The gunfire's additive glow on the engine's plane (R, G, B, strength, shape: 0 a streak, 1 round). */
	const TCHAR* GlowPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;
	constexpr float StreakShape = 0.f;
	constexpr float RoundShape = 1.f;

	/** Abel lights it at the end of Main 6, "The Gravewind". */
	const FName LitAfter(TEXT("Main6"));

	/** The tongues and the core: three cards each, 60 degrees apart about the flame's axis, so it reads from every side. */
	constexpr int32 CardsAround = 3;

	/** An asset found only once it's in this checkout, so the class works, and its tests run, without it. In a constructor. */
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

FVector LanternLean::Axis(float BearingDegrees, float LeanDegrees)
{
	const double Bearing = FMath::DegreesToRadians(static_cast<double>(BearingDegrees));
	const double Lean = FMath::DegreesToRadians(static_cast<double>(LeanDegrees));
	const FVector Toward(FMath::Cos(Bearing), FMath::Sin(Bearing), 0.0);
	return (FVector::UpVector * FMath::Cos(Lean) + Toward * FMath::Sin(Lean)).GetSafeNormal();
}

FQuat LanternLean::Turn(float BearingDegrees, float LeanDegrees)
{
	return FQuat::FindBetweenNormals(FVector::UpVector, Axis(BearingDegrees, LeanDegrees));
}

ALanternFlame::ALanternFlame()
{
	PrimaryActorTick.bCanEverTick = false;

	static UStaticMesh* const Plane = FindIfMade<UStaticMesh>(PlanePath);
	static UMaterialInterface* const Glow = FindIfMade<UMaterialInterface>(GlowPath);
	CardMesh = Plane;
	GlowMaterial = Glow;

	// Its origin is the wick: the build script hangs it on the lantern's SOCKET_Light, and it goes where the lantern goes.
	USceneComponent* Wick = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Wick->SetMobility(EComponentMobility::Movable);
	RootComponent = Wick;

	Flame = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Wick);
	Flame->SetMobility(EComponentMobility::Movable);
	Flame->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Flame->SetGenerateOverlapEvents(false);
	Flame->SetCanEverAffectNavigation(false);
	Flame->SetCastShadow(false);
	Flame->bReceivesDecals = false;
	Flame->SetNumCustomDataFloats(GlowFloats);
	Flame->SetVisibility(false);
	// It goes where its lantern goes, but leans the same way in the world however the lantern swings or tilts on its hook.
	Flame->SetUsingAbsoluteRotation(true);

	ShownWhen.AfterMissions = { LitAfter };
}

void ALanternFlame::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildCards();
	Lean();
	Flame->SetVisibility(bLit);
}

void ALanternFlame::BeginPlay()
{
	Super::BeginPlay();
	BuildCards();
	Lean();
	Flame->SetVisibility(bLit);
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ALanternFlame::HandleMissionsChanged);
	}
	if (AKeeperLanternPost* Post = FindPost())
	{
		BoundPost = Post;
		LanternLitHandle = Post->OnKeepersLanternLit.AddUObject(this, &ALanternFlame::HandleLanternLit);
	}
	RefreshStory();
}

void ALanternFlame::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	if (AKeeperLanternPost* Post = BoundPost.Get())
	{
		Post->OnKeepersLanternLit.Remove(LanternLitHandle);
	}
	BoundPost.Reset();
	LanternLitHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void ALanternFlame::BuildCards()
{
	Flame->SetStaticMesh(CardMesh);
	Flame->SetMaterial(0, GlowMaterial);
	Flame->SetNumCustomDataFloats(GlowFloats);
	Flame->ClearInstances();
	// The engine's plane is 100 x 100 in its XY, facing +Z; a streak's bright head is at its +X end. Pitched down a quarter
	// turn, its head is at the wick and its tail up the flame, its face out to the side.
	for (int32 Card = 0; Card < CardsAround; ++Card)
	{
		const FRotator Around(-90.0, 180.0 / CardsAround * Card, 0.0);
		const float Tongue[] = { Color.R, Color.G, Color.B, Strength, StreakShape };
		Flame->SetCustomData(Flame->AddInstance(FTransform(Around, FVector(0.0, 0.0, Length * 0.5), FVector(Length / 100.f, Width / 100.f, 1.f))),
			Tongue);
		// A brighter round heart low in the flame, at the wick.
		const float Core[] = { Color.R, Color.G * 0.9f, Color.B * 0.6f, Strength * 1.25f, RoundShape };
		Flame->SetCustomData(Flame->AddInstance(FTransform(Around, FVector(0.0, 0.0, Length * 0.18), FVector(Width * 1.2f / 100.f))), Core);
	}
}

void ALanternFlame::Lean()
{
	// In the world (its turn is its own, not the lantern's), whichever way the lantern it hangs from is turned.
	Flame->SetWorldRotation(LanternLean::Turn(Bearing, LeanDegrees));
}

FVector ALanternFlame::GetFlameAxis() const
{
	return Flame->GetComponentQuat().GetUpVector();
}

void ALanternFlame::SetLit(bool bInLit)
{
	if (bLit == bInLit)
	{
		return;
	}
	bLit = bInLit;
	Lean();
	Flame->SetVisibility(bLit);
	if (bLit)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: the lantern's flame leans toward bearing %.0f."), *GetActorNameOrLabel(), Bearing);
	}
}

bool ALanternFlame::ShouldBeLit(const FCampaignRecord& Campaign, const UMissionRunner* Runner) const
{
	return ShownWhen.IsMet(Campaign, Runner);
}

void ALanternFlame::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const AKeeperLanternPost* Post = BoundPost.Get();
	const bool bPostLit = Post && Post->IsKeepersLanternLit();
	if (Runner || bPostLit)
	{
		SetLit(bPostLit || (Runner && ShouldBeLit(Runner->GetCampaign(), Runner)));
	}
}

void ALanternFlame::HandleMissionsChanged()
{
	RefreshStory();
}

void ALanternFlame::HandleLanternLit(AKeeperLanternPost& /*LitPost*/)
{
	SetLit(true);
}

AKeeperLanternPost* ALanternFlame::FindPost() const
{
	return Cast<AKeeperLanternPost>(GetAttachParentActor());
}
