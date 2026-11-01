// ASoulCoalField: the barge's fallen soul coal lying smoking across the Rest after Main 6, until Saint Ada wakes.

#include "World/SoulCoalField.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "World/CarriedLanternRules.h"
#include "World/LamplighterEffects.h"
#include "World/SoulCoal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* StandInPath = TEXT("/Game/Art/Rocks/SM_Rock_B.SM_Rock_B");
	const TCHAR* CharcoalPath = TEXT("/Game/Art/Materials/MI_Charcoal.MI_Charcoal");
	const TCHAR* CrustPath = TEXT("/Game/Art/Landfall/Materials/MI_Landfall_SoulSmogGlint.MI_Landfall_SoulSmogGlint");
	/** The crust: the lump's mesh smaller and set a little up into it, so its glint shows in the coal's facets. */
	constexpr double CrustScale = 0.82;
	constexpr double CrustUp = 0.14;
	/** Smoke: a puff off one lump within this reach of the player (cm), this often (s); the field looks this often. */
	constexpr double SmokeReach = 2500.0;
	constexpr float SmokeEvery = 0.9f;
	constexpr float LookEvery = 0.45f;

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

	UInstancedStaticMeshComponent* MakeLayer(AActor* Owner, const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look)
	{
		UInstancedStaticMeshComponent* Layer = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Layer->SetStaticMesh(Mesh);
		if (Look)
		{
			Layer->SetMaterial(0, Look);
		}
		Layer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Layer->SetCastShadow(false);
		Layer->SetCanEverAffectNavigation(false);
		Layer->bReceivesDecals = false;
		return Layer;
	}
}

ASoulCoalField::ASoulCoalField()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = LookEvery;
	SetCanBeDamaged(false);
	static UStaticMesh* const Tripo = FindIfMade<UStaticMesh>(ASoulCoalLump::ModelPath);
	static UStaticMesh* const StandIn = FindIfMade<UStaticMesh>(StandInPath);
	static UMaterialInterface* const Charcoal = FindIfMade<UMaterialInterface>(CharcoalPath);
	static UMaterialInterface* const CrustLook = FindIfMade<UMaterialInterface>(CrustPath);
	UStaticMesh* const Mesh = Tripo ? Tripo : StandIn;
	Coal = MakeLayer(this, TEXT("Coal"), Mesh, Tripo ? nullptr : Charcoal);
	RootComponent = Coal;
	Crust = MakeLayer(this, TEXT("Crust"), Mesh, CrustLook);
	Crust->SetupAttachment(Coal);
	ShownWhen.AfterMissions = { FName(TEXT("Main6")) };
	ShownWhen.WithoutFlags = { GivenToTheWind::SaintAdaAwakeFlag() };
}

void ASoulCoalField::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildInstances();
}

void ASoulCoalField::SetLumps(const TArray<FTransform>& InLumps)
{
	Lumps = InLumps;
	BuildInstances();
}

void ASoulCoalField::BuildInstances()
{
	Coal->ClearInstances();
	Crust->ClearInstances();
	Coal->SetCullDistances(0, static_cast<int32>(DrawDistance));
	Crust->SetCullDistances(0, static_cast<int32>(DrawDistance * 0.6f));
	const bool bCrust = bGlints && Crust->GetMaterial(0) != nullptr && Crust->GetStaticMesh() != nullptr;
	for (const FTransform& Each : Lumps)
	{
		Coal->AddInstance(Each, /*bWorldSpace*/ true);
		if (bCrust)
		{
			FTransform Inner = Each;
			Inner.SetScale3D(Each.GetScale3D() * CrustScale);
			Inner.AddToTranslation(FVector(0.0, 0.0, Each.GetScale3D().Z * 100.0 * CrustUp));
			Crust->AddInstance(Inner, /*bWorldSpace*/ true);
		}
	}
}

void ASoulCoalField::BeginPlay()
{
	Super::BeginPlay();
	SmokeWait = FMath::FRandRange(0.f, SmokeEvery);
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ASoulCoalField::HandleMissionsChanged);
	}
	RefreshStory();
}

void ASoulCoalField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ASoulCoalField::HandleMissionsChanged()
{
	RefreshStory();
}

void ASoulCoalField::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	bShown = ShownWhen.IsEmpty() || (Runner && ShownWhen.IsMet(Runner->GetCampaign(), Runner));
	SetActorHiddenInGame(!bShown);
	SetActorTickEnabled(bShown && !Lumps.IsEmpty());
}

void ASoulCoalField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SmokeWait -= DeltaSeconds;
	if (SmokeWait > 0.f)
	{
		return;
	}
	SmokeWait = SmokeEvery * FMath::FRandRange(0.6f, 1.4f);
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	if (!Player)
	{
		return;
	}
	// One lump near the player smokes a moment (a few tries at a random one; nothing near: nothing).
	const FVector Here = Player->GetActorLocation();
	for (int32 Try = 0; Try < 4 && !Lumps.IsEmpty(); ++Try)
	{
		const FTransform& Lump = Lumps[FMath::RandRange(0, Lumps.Num() - 1)];
		if (FVector::DistSquared(Lump.GetLocation(), Here) <= SmokeReach * SmokeReach)
		{
			LamplighterEffects::SmokePuff(this, Lump.GetLocation() + FVector(0.0, 0.0, 15.0), FVector(FMath::FRandRange(-12.f, 12.f), FMath::FRandRange(-12.f, 12.f), 40.f));
			break;
		}
	}
}
