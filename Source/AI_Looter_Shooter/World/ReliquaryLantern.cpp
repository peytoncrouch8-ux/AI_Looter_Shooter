// AReliquaryLantern: Abel's lantern set in the lid of Saint Ada's Reliquary (Main 7's last step), and kept there lit after.

#include "World/ReliquaryLantern.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SaintAdaWakes.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "World/CarriedLantern.h"
#include "World/CarriedLanternRules.h"
#include "World/ChapelReliquary.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* LanternPath = TEXT("/Game/Art/Props/SM_KeepersLantern.SM_KeepersLantern");
	const TCHAR* DarkGlassPath = TEXT("/Game/Art/Materials/MI_HouseTrim.MI_HouseTrim");
	/** The keeper's light as it burns "like a sunrise": warm gold, small and shadowless (candelas, cm). */
	const FLinearColor KeeperLight(1.f, 0.82f, 0.52f);
	constexpr float LitCandelas = 14.f;
	constexpr float LightRadius = 520.f;
	/** The lantern's foot this far over the ember's spot (its recess), and its globe this high over its foot (cm). */
	constexpr double SeatUp = 1.0;
	constexpr double GlobeUp = 18.0;
	/** The recess's box for the Interact line (half sizes, cm). */
	const FVector GripHalf(26.0, 26.0, 26.0);

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

AReliquaryLantern::AReliquaryLantern()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static UStaticMesh* const Model = FindIfMade<UStaticMesh>(LanternPath);
	static UMaterialInterface* const Dark = FindIfMade<UMaterialInterface>(DarkGlassPath);
	DarkGlass = Dark;
	Lantern = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lantern"));
	Lantern->SetupAttachment(Root);
	Lantern->SetMobility(EComponentMobility::Movable);
	Lantern->SetStaticMesh(Model);
	Lantern->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Lantern->SetCastShadow(false);
	Lantern->SetVisibility(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.0, 0.0, GlobeUp));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetLightColor(KeeperLight);
	Light->SetAttenuationRadius(LightRadius);
	Light->SetCastShadows(false);
	Light->SetVisibility(false);

	Grip = CreateDefaultSubobject<UBoxComponent>(TEXT("Grip"));
	Grip->SetupAttachment(Root);
	Grip->SetBoxExtent(GripHalf);
	Grip->SetRelativeLocation(FVector(0.0, 0.0, GripHalf.Z));
	Grip->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Grip->SetCollisionResponseToAllChannels(ECR_Ignore);
	Grip->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	SetWhen.DuringMission = GivenToTheWind::Mission();
	SetWhen.FromStep = GivenToTheWind::ReliquaryStep;
	SetWhen.BeforeStep = GivenToTheWind::ReadyStep;
	ShownWhen.WithFlags = { GivenToTheWind::SaintAdaAwakeFlag() };
	Tags.Add(GivenToTheWind::ReliquaryLanternTag());
}

void AReliquaryLantern::BeginPlay()
{
	Super::BeginPlay();
	if (Lantern->GetStaticMesh())
	{
		const int32 Slot = Lantern->GetMaterialIndex(GlassSlot);
		LitGlass = Slot != INDEX_NONE ? Lantern->GetMaterial(Slot) : nullptr;
	}
	SeatOnReliquary();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AReliquaryLantern::HandleMissionsChanged);
	}
	RefreshStory();
}

void AReliquaryLantern::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

AChapelReliquary* AReliquaryLantern::GetReliquary() const
{
	if (Reliquary.IsValid())
	{
		return Reliquary.Get();
	}
	AChapelReliquary* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	for (TActorIterator<AChapelReliquary> It(GetWorld()); It; ++It)
	{
		const double Distance = FVector::DistSquared(It->GetActorLocation(), GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = *It;
		}
	}
	const_cast<AReliquaryLantern*>(this)->Reliquary = Best;
	return Best;
}

void AReliquaryLantern::SeatOnReliquary()
{
	if (AChapelReliquary* Saint = GetReliquary())
	{
		// In her lid's recess where her ember lies, turned as she faces (the nave).
		SetActorLocationAndRotation(Saint->GetEmberStart() + FVector(0.0, 0.0, SeatUp), Saint->GetActorRotation());
	}
}

FVector AReliquaryLantern::GetGlobe() const
{
	return GetActorLocation() + FVector(0.0, 0.0, GlobeUp);
}

bool AReliquaryLantern::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

void AReliquaryLantern::HandleMissionsChanged()
{
	RefreshStory();
}

void AReliquaryLantern::RefreshStory()
{
	// After Saint Ada woke (a load after it), he's there in his lantern, lit, for good.
	if (!ShownWhen.IsEmpty() && StoryHolds(ShownWhen))
	{
		bSet = true;
		ShowLantern(true, 1.f);
	}
}

bool AReliquaryLantern::CanSet() const
{
	const UCarriedLanternSubsystem* Carried = UCarriedLanternSubsystem::Get(this);
	return !bSet && Carried && Carried->IsCarried() && StoryHolds(SetWhen);
}

FInteractionOptions AReliquaryLantern::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!CanSet())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = NSLOCTEXT("LooterReliquary", "SetLantern", "Set Abel's lantern on the Reliquary");
	Options.Reach = Reach;
	return Options;
}

bool AReliquaryLantern::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && SetLantern(User.GetOwner());
}

TOptional<FVector> AReliquaryLantern::GetInteractionLocation() const
{
	return TOptional<FVector>(GetGlobe());
}

TOptional<FInteractionBox> AReliquaryLantern::GetInteractionBox() const
{
	return InteractionFocus::BoxAround(GetActorLocation() + FVector(0.0, 0.0, GripHalf.Z), GripHalf, GetActorQuat());
}

bool AReliquaryLantern::SetLantern(AActor* ByWhom, bool bForce, bool bQuiet)
{
	if (bSet || (!bForce && !CanSet()))
	{
		return false;
	}
	bSet = true;
	SeatOnReliquary();
	// Set down in her recess: iron on stone, his light steady in it; every line off it.
	ShowLantern(true, 0.6f);
	LooterSound::PlayAt(this, LooterSoundCue::Impact::Metal, GetActorLocation(), 0.5f, 0.8f);
	LooterSound::PlayAt(this, LooterSoundCue::LanternLean, GetGlobe(), 0.8f, 0.9f);
	if (UCarriedLanternSubsystem* Carried = UCarriedLanternSubsystem::Get(this))
	{
		Carried->SetDown();
	}
	UE_LOG(LogLooter, Log, TEXT("%s: Abel's lantern set on Saint Ada's Reliquary."), *GetActorNameOrLabel());
	if (!bQuiet && !SaintAdaWakes::Play(*this))
	{
		// No scene (scenes off, or another playing): the world as it would leave it, and the missions hear it played.
		SaintAdaWakes::FinishAtOnce(*this);
	}
	return true;
}

void AReliquaryLantern::ShowLantern(bool bShown, float LitShare)
{
	Lantern->SetVisibility(bShown && Lantern->GetStaticMesh() != nullptr);
	Light->SetVisibility(bShown);
	Light->SetIntensity(LitCandelas * FMath::Clamp(LitShare, 0.f, 2.f));
	const int32 Slot = Lantern->GetStaticMesh() ? Lantern->GetMaterialIndex(GlassSlot) : INDEX_NONE;
	if (Slot != INDEX_NONE)
	{
		UMaterialInterface* Glass = LitShare > 0.05f ? LitGlass.Get() : DarkGlass.Get();
		if (Glass)
		{
			Lantern->SetMaterial(Slot, Glass);
		}
	}
}
