// AIntakeCharge: a clamp spot on the barge's intake collar, where one of Junie's salt charges goes on.

#include "World/IntakeCharge.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/LamplighterEffects.h"
#include "World/SinkIntake.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "IntakeCharge"

const FName AIntakeCharge::ChargeTag(TEXT("Intake_Charge"));
const TCHAR* const AIntakeCharge::ChargeModelPath = TEXT("/Game/Art/Tripo/SM_SaltCharge.SM_SaltCharge");

namespace
{
	const TCHAR* ChargeCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* ChargeCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* ChargeIronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");
	const TCHAR* ChargeSaltPath = TEXT("/Game/Art/Materials/MI_ClothLinen.MI_ClothLinen");
	const TCHAR* ChargeScorchPath = TEXT("/Game/Art/Materials/MI_GunScorched.MI_GunScorched");

	/** Its fuse: sparks this often (s), its light (candelas, cm), its colour. */
	constexpr float SparkEvery = 0.12f;
	constexpr float FuseCandelas = 6.f;
	constexpr float FuseRadius = 260.f;
	const FLinearColor FuseColor(1.f, 0.35f, 0.1f);

	template <typename T>
	T* ChargeAssetIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	UStaticMeshComponent* MakeChargePart(AActor& Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh,
		UMaterialInterface* Look, const FVector& At, const FRotator& Turn, const FVector& Scale)
	{
		UStaticMeshComponent* Part = Owner.CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetStaticMesh(Mesh);
		if (Look)
		{
			Part->SetMaterial(0, Look);
		}
		Part->SetRelativeLocationAndRotation(At, Turn);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		return Part;
	}
}

AIntakeCharge::AIntakeCharge()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Tags.Add(ChargeTag);

	static UStaticMesh* const Cube = ChargeAssetIfMade<UStaticMesh>(ChargeCubePath);
	static UStaticMesh* const Cylinder = ChargeAssetIfMade<UStaticMesh>(ChargeCylinderPath);
	static UStaticMesh* const Tripo = ChargeAssetIfMade<UStaticMesh>(ChargeModelPath);
	static UMaterialInterface* const Iron = ChargeAssetIfMade<UMaterialInterface>(ChargeIronPath);
	static UMaterialInterface* const Salt = ChargeAssetIfMade<UMaterialInterface>(ChargeSaltPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The lug on the collar's face (its +X out of the collar): an iron block the clamp bites on.
	Lug = MakeChargePart(*this, Root, TEXT("Lug"), Cube, Iron, FVector(-6.0, 0.0, 0.0), FRotator::ZeroRotator, FVector(0.12, 0.32, 0.32));
	// The charge: the Tripo model (its clamp's face -X, on the lug), else three jars stood side by side and the clamp.
	Charge = MakeChargePart(*this, Root, TEXT("Charge"), Tripo ? Tripo : Cylinder, Tripo ? nullptr : Iron, FVector(4.0, 0.0, 0.0),
		Tripo ? FRotator::ZeroRotator : FRotator(90.0, 0.0, 0.0), Tripo ? FVector::OneVector : FVector(0.3, 0.3, 0.04));
	if (!Tripo)
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FString Name = FString::Printf(TEXT("Jar%d"), Index);
			Jars.Add(MakeChargePart(*this, Root, *Name, Cylinder, Salt, FVector(14.0, (Index - 1) * 11.0, 0.0), FRotator::ZeroRotator,
				FVector(0.1, 0.1, 0.3)));
		}
	}
	FuseLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FuseLight"));
	FuseLight->SetupAttachment(Root);
	FuseLight->SetMobility(EComponentMobility::Movable);
	FuseLight->SetRelativeLocation(FVector(16.0, 0.0, 18.0));
	FuseLight->SetIntensityUnits(ELightUnits::Candelas);
	FuseLight->SetIntensity(FuseCandelas);
	FuseLight->SetAttenuationRadius(FuseRadius);
	FuseLight->SetLightColor(FuseColor);
	FuseLight->SetCastShadows(false);
	FuseLight->SetVisibility(false);

	Prompt = LOCTEXT("Set", "Set a salt charge");
}

void AIntakeCharge::BeginPlay()
{
	Super::BeginPlay();
	ResetCharge();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AIntakeCharge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	LooterSound::Stop(HissSound.Get(), 0.f);
	HissSound.Reset();
	Super::EndPlay(EndPlayReason);
}

void AIntakeCharge::ResetCharge()
{
	bSet = false;
	bLit = false;
	bBlown = false;
	ShowCharge(false);
	if (Lug)
	{
		Lug->SetVisibility(true);
	}
	if (FuseLight)
	{
		FuseLight->SetVisibility(false);
	}
	LooterSound::Stop(HissSound.Get(), 0.1f);
	HissSound.Reset();
	SetActorTickEnabled(false);
}

void AIntakeCharge::ShowCharge(bool bShown)
{
	if (Charge)
	{
		Charge->SetVisibility(bShown);
	}
	for (UStaticMeshComponent* Jar : Jars)
	{
		if (Jar)
		{
			Jar->SetVisibility(bShown);
		}
	}
}

bool AIntakeCharge::CanSet() const
{
	if (bSet || bBlown)
	{
		return false;
	}
	if (SetWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return !Runner || SetWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AIntakeCharge::SetCharge(AActor* By, bool bForce)
{
	if (bSet || bBlown || (!bForce && !CanSet()))
	{
		return false;
	}
	bSet = true;
	ShowCharge(true);
	// Slammed on: iron on iron, the magnet's bite, a few sparks off the lug.
	const FVector At = GetActorLocation();
	LooterSound::PlayAt(this, LooterSoundCue::ImpactMetal, At, 1.1f, 0.8f);
	LooterSound::PlayAt(this, LooterSoundCue::AnchorSet, At, 0.5f, 1.4f);
	LamplighterEffects::Sparks(this, At + GetActorForwardVector() * 10.0, GetActorForwardVector(), 5, 0.5f);
	UE_LOG(LogLooter, Log, TEXT("%s: a salt charge set on the intake's collar."), *GetActorNameOrLabel());
	if (ASinkIntake* Intake = FindIntake())
	{
		Intake->NoteChargeSet(*this);
	}
	return true;
}

void AIntakeCharge::LightFuse()
{
	if (!bSet || bLit || bBlown)
	{
		return;
	}
	bLit = true;
	SparkClock = 0.f;
	if (FuseLight)
	{
		FuseLight->SetVisibility(true);
	}
	HissSound = LooterSound::Start(this, LooterSoundCue::Foreman::FuseHiss, Root, NAME_None, 0.8f, 1.f);
	SetActorTickEnabled(true);
}

void AIntakeCharge::Blow()
{
	bBlown = true;
	bLit = false;
	ShowCharge(false);
	if (FuseLight)
	{
		FuseLight->SetVisibility(false);
	}
	LooterSound::Stop(HissSound.Get(), 0.05f);
	HissSound.Reset();
	// The lug blackened by the blast.
	static UMaterialInterface* const Scorch = LoadObject<UMaterialInterface>(nullptr, ChargeScorchPath);
	if (Lug && Scorch)
	{
		Lug->SetMaterial(0, Scorch);
	}
	SetActorTickEnabled(false);
}

void AIntakeCharge::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bLit)
	{
		SetActorTickEnabled(false);
		return;
	}
	// The fuse spits and flickers as it burns down.
	SparkClock += DeltaSeconds;
	if (SparkClock >= SparkEvery)
	{
		SparkClock = 0.f;
		LamplighterEffects::Sparks(this, GetFuse(), FVector::UpVector + GetActorForwardVector(), 2, 0.35f);
	}
	if (FuseLight)
	{
		FuseLight->SetIntensity(FuseCandelas * FMath::FRandRange(0.6f, 1.2f));
	}
}

FVector AIntakeCharge::GetFuse() const
{
	return GetActorTransform().TransformPosition(FVector(16.0, 0.0, 18.0));
}

FInteractionOptions AIntakeCharge::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!CanSet())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = Prompt;
	return Options;
}

bool AIntakeCharge::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && SetCharge(User.GetOwner());
}

TOptional<FVector> AIntakeCharge::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(FVector(10.0, 0.0, 0.0)));
}

TOptional<FInteractionBox> AIntakeCharge::GetInteractionBox() const
{
	// The lug and a hand's width round it on the collar.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(FVector(8.0, 0.0, 0.0)), FVector(25.0, 35.0, 35.0), GetActorQuat());
}

ASinkIntake* AIntakeCharge::FindIntake() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ASinkIntake> It(World); It; ++It)
	{
		if (IntakeTag.IsNone() || It->ActorHasTag(IntakeTag))
		{
			return *It;
		}
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
