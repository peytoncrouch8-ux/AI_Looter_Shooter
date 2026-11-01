// ACompanyBanner: the Company's banner across Main Street: its two ropes shot through, its sag and its fall into the
// street, the missions told.

#include "World/CompanyBanner.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/LamplighterEffects.h"
#include "World/StoryProp.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const ACompanyBanner::ModelPath = TEXT("/Game/Art/Tripo/SM_CompanyBanner.SM_CompanyBanner");

namespace
{
	const TCHAR* BannerCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* BannerCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* BannerCanvasPath = TEXT("/Game/Art/Materials/MI_Canvas.MI_Canvas");

	/** The cloth's thickness, the ropes' width and their hits' girth (cm). */
	constexpr float ClothThick = 4.f;
	constexpr float RopeWidth = 3.5f;
	constexpr float RopeHitRadius = 18.f;
	/** A cut rope's stub left hanging from its tie (cm). */
	constexpr float StubLength = 90.f;
	/** The words: their height (cm) and ink (lamp-black on the canvas). */
	constexpr float BannerLetters = 72.f;
	const FColor BannerInk(28, 22, 20);
	/** A freed end drops over this long (s), overshooting a little as it swings; both cut, it falls over this long. */
	constexpr float SagSeconds = 0.6f;
	constexpr float FallSeconds = 0.75f;
	/** It lies this far over the street (cm), clear of the dirt. */
	constexpr float LieOver = 4.f;

	/** Bullets' trace channel (DefaultEngine.ini's Weapon). */
	constexpr ECollisionChannel BannerWeaponChannel = ECC_GameTraceChannel2;

	UStaticMesh* BannerModelIfMade()
	{
		const FString Path(ACompanyBanner::ModelPath);
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(ACompanyBanner::ModelPath);
		return Finder.Object;
	}

	/** An ease out with a little overshoot: a freed end swinging past its rest and settling. */
	float EaseOutBack(float T)
	{
		const float C1 = 1.70158f;
		const float C3 = C1 + 1.f;
		const float U = T - 1.f;
		return 1.f + C3 * U * U * U + C1 * U * U;
	}

	/** A cylinder part stretched from A to B (the banner's frame), Width across. */
	void StretchRope(UStaticMeshComponent* Rope, const FVector& A, const FVector& B, float Width)
	{
		if (!Rope)
		{
			return;
		}
		const FVector Span = B - A;
		Rope->SetRelativeLocationAndRotation((A + B) * 0.5, FRotationMatrix::MakeFromZ(Span.GetSafeNormal()).Rotator());
		Rope->SetRelativeScale3D(FVector(Width / 100.0, Width / 100.0, FMath::Max(Span.Size(), 1.0) / 100.0));
	}

	void FitHit(UCapsuleComponent* Hit, const FVector& A, const FVector& B, bool bLive)
	{
		if (!Hit)
		{
			return;
		}
		const FVector Span = B - A;
		Hit->SetRelativeLocationAndRotation((A + B) * 0.5, FRotationMatrix::MakeFromZ(Span.GetSafeNormal()).Rotator());
		Hit->SetCapsuleSize(RopeHitRadius, FMath::Max(static_cast<float>(Span.Size()) * 0.5f, RopeHitRadius));
		Hit->SetCollisionEnabled(bLive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

ACompanyBanner::ACompanyBanner()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(BannerCubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(BannerCylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Canvas(BannerCanvasPath);
	static UStaticMesh* const Model = BannerModelIfMade();

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	ClothFrame = CreateDefaultSubobject<USceneComponent>(TEXT("ClothFrame"));
	ClothFrame->SetupAttachment(Root);
	ClothFrame->SetMobility(EComponentMobility::Movable);

	// Seen only: the cloth stops nothing (shots pass through canvas; only the ropes are the target).
	Cloth = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cloth"));
	Cloth->SetupAttachment(ClothFrame);
	Cloth->SetStaticMesh(Model ? Model : Cube.Object.Get());
	if (!Model)
	{
		Cloth->SetMaterial(0, Canvas.Object);
		Cloth->SetRelativeScale3D(FVector(ClothThick / 100.0, ClothLength / 100.0, ClothHeight / 100.0));
	}
	Cloth->SetMobility(EComponentMobility::Movable);
	Cloth->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cloth->SetCanEverAffectNavigation(false);

	auto MakeWords = [&](const TCHAR* Name, float Side)
	{
		UTextRenderComponent* Made = CreateDefaultSubobject<UTextRenderComponent>(Name);
		Made->SetupAttachment(ClothFrame);
		Made->SetRelativeLocationAndRotation(FVector(Side * (ClothThick * 0.5 + 0.6), 0.0, 0.0), FRotator(0.0, Side > 0.f ? 0.0 : 180.0, 0.0));
		Made->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		Made->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		Made->SetWorldSize(BannerLetters);
		Made->SetTextRenderColor(BannerInk);
		Made->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Made->SetCastShadow(false);
		// Drawn on both faces (the Tripo banner comes blank: TRIPO_MODELS.md M23-5).
		return Made;
	};
	WordsFront = MakeWords(TEXT("WordsFront"), 1.f);
	WordsBack = MakeWords(TEXT("WordsBack"), -1.f);

	auto MakeRope = [&](const TCHAR* Name)
	{
		UStaticMeshComponent* Rope = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Rope->SetupAttachment(Root);
		Rope->SetStaticMesh(Cylinder.Object);
		Rope->SetMaterial(0, Canvas.Object);
		Rope->SetMobility(EComponentMobility::Movable);
		Rope->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Rope->SetCanEverAffectNavigation(false);
		Rope->SetCastShadow(false);
		return Rope;
	};
	RopeLeft = MakeRope(TEXT("RopeLeft"));
	RopeRight = MakeRope(TEXT("RopeRight"));

	auto MakeHit = [&](const TCHAR* Name)
	{
		UCapsuleComponent* Hit = CreateDefaultSubobject<UCapsuleComponent>(Name);
		Hit->SetupAttachment(Root);
		Hit->InitCapsuleSize(RopeHitRadius, 100.f);
		Hit->SetMobility(EComponentMobility::Movable);
		Hit->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Hit->SetCollisionObjectType(ECC_WorldDynamic);
		Hit->SetCollisionResponseToAllChannels(ECR_Ignore);
		Hit->SetCollisionResponseToChannel(BannerWeaponChannel, ECR_Block);
		Hit->SetGenerateOverlapEvents(false);
		Hit->SetCanEverAffectNavigation(false);
		Hit->SetHiddenInGame(true);
		return Hit;
	};
	RopeLeftHit = MakeHit(TEXT("RopeLeftHit"));
	RopeRightHit = MakeHit(TEXT("RopeRightHit"));
}

void ACompanyBanner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (UTextRenderComponent* Words : { WordsFront.Get(), WordsBack.Get() })
	{
		if (Words)
		{
			Words->SetText(Text);
		}
	}
	Pose();
}

void ACompanyBanner::BeginPlay()
{
	Super::BeginPlay();
	OnTakePointDamage.AddUniqueDynamic(this, &ACompanyBanner::HandlePointDamage);
	for (UTextRenderComponent* Words : { WordsFront.Get(), WordsBack.Get() })
	{
		if (Words)
		{
			Words->SetText(Text);
		}
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &ACompanyBanner::HandleRunnerChanged);
	}
	RefreshStory();
}

void ACompanyBanner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void ACompanyBanner::HandleRunnerChanged()
{
	RefreshStory();
}

void ACompanyBanner::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const FCampaignRecord* Campaign = Runner ? &Runner->GetCampaign() : nullptr;
	const bool bShown = AStoryProp::Applies(ShownWhen.IsEmpty() ? TArray<FStoryCondition>() : TArray<FStoryCondition>{ ShownWhen },
		HiddenWhen, Campaign, Runner);
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
	// Cut down before (its flag kept): it lies in the street from the start, nothing more to cut or tell.
	if (bShown && !bDown && !DownFlag.IsNone() && Runner && Runner->HasFlag(DownFlag))
	{
		bCut[0] = bCut[1] = true;
		FirstCut = 0;
		Sag = 1.f;
		Fall = 1.f;
		bDown = true;
		bTold = true;
		SetActorTickEnabled(false);
	}
	Pose();
}

FVector ACompanyBanner::CornerNow(float Side) const
{
	const FVector Corner(0.0, Side * ClothLength * 0.5, ClothHeight * 0.5);
	return ClothFrame ? ClothFrame->GetRelativeTransform().TransformPosition(Corner) : Corner;
}

void ACompanyBanner::Pose()
{
	if (!ClothFrame)
	{
		return;
	}
	// One end cut: the cloth swings down about the other end's top corner, its free end coming to rest on the street.
	FVector Location = FVector::ZeroVector;
	FQuat Turn = FQuat::Identity;
	if (FirstCut != INDEX_NONE)
	{
		const float PivotSide = FirstCut == 0 ? 1.f : -1.f;
		const FVector Pivot(0.0, PivotSide * ClothLength * 0.5, ClothHeight * 0.5);
		const double Reach = FMath::Clamp((StreetBelow - ClothHeight * 0.5 - 10.0) / ClothLength, 0.0, 0.95);
		const double Angle = FMath::Asin(Reach) * EaseOutBack(FMath::Clamp(Sag, 0.f, 1.f)) * PivotSide;
		Turn = FQuat(FVector::ForwardVector, Angle);
		Location = Pivot + Turn.RotateVector(-Pivot);
	}
	// Both cut: it drops into the street and lies flat, words up (its thin side turned to the sky).
	if (NumCut() == 2)
	{
		const float Drop = FMath::Clamp(Fall, 0.f, 1.f);
		const float Eased = Drop * Drop;
		const FVector Lying(0.0, 0.0, -StreetBelow + ClothThick * 0.5 + LieOver);
		const FQuat Flat = FQuat(FRotator(90.0, 0.0, 0.0));
		Location = FMath::Lerp(Location, Lying, static_cast<double>(Eased));
		Turn = FQuat::Slerp(Turn, Flat, Eased);
	}
	ClothFrame->SetRelativeLocationAndRotation(Location, Turn);

	// The ropes: whole from their ties to the cloth's corners, or a cut stub hanging from the tie.
	const FVector Anchors[2] = { AnchorLeft, AnchorRight };
	UStaticMeshComponent* Ropes[2] = { RopeLeft.Get(), RopeRight.Get() };
	UCapsuleComponent* Hits[2] = { RopeLeftHit.Get(), RopeRightHit.Get() };
	for (int32 Rope = 0; Rope < 2; ++Rope)
	{
		const FVector Tie = Anchors[Rope];
		const FVector End = bCut[Rope] ? Tie - FVector(0.0, 0.0, StubLength) : CornerNow(Rope == 0 ? -1.f : 1.f);
		StretchRope(Ropes[Rope], Tie, End, RopeWidth);
		FitHit(Hits[Rope], Tie, End, !bCut[Rope] && !bDown);
	}
}

bool ACompanyBanner::CutRope(int32 Rope, AController* By)
{
	if (Rope < 0 || Rope > 1 || bCut[Rope] || bDown || IsHidden())
	{
		return false;
	}
	bCut[Rope] = true;
	if (FirstCut == INDEX_NONE)
	{
		FirstCut = Rope;
		Sag = 0.f;
	}
	const FVector Tie = GetActorTransform().TransformPosition(Rope == 0 ? AnchorLeft : AnchorRight);
	// The rope parting with a crack, the canvas slapping as it drops.
	LooterSound::PlayAt(this, LooterSoundCue::GrappleCut, Tie, 0.7f, 1.3f);
	LooterSound::PlayAt(this, LooterSoundCue::Fauna::ClothFlap, GetActorLocation());
	if (NumCut() == 2)
	{
		Fall = 0.f;
	}
	SetActorTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("%s: its %s rope cut%s."), *GetActorNameOrLabel(), Rope == 0 ? TEXT("left") : TEXT("right"),
		By ? TEXT(" by the player") : TEXT(""));
	return true;
}

void ACompanyBanner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ACompanyBanner::Advance(float DeltaSeconds)
{
	if (FirstCut == INDEX_NONE || bDown)
	{
		SetActorTickEnabled(false);
		return;
	}
	if (NumCut() == 1 || Sag < 1.f)
	{
		Sag = FMath::Min(Sag + DeltaSeconds / SagSeconds, 1.f);
	}
	if (NumCut() == 2)
	{
		Fall = FMath::Min(Fall + DeltaSeconds / FallSeconds, 1.f);
	}
	Pose();
	if (NumCut() == 1 && Sag >= 1.f)
	{
		// Hanging by one rope: still until the other goes.
		SetActorTickEnabled(false);
		return;
	}
	if (NumCut() < 2 || Fall < 1.f)
	{
		return;
	}
	// In the street: a slap, its dust, and the news.
	bDown = true;
	SetActorTickEnabled(false);
	const FVector Landed = GetActorTransform().TransformPosition(FVector(0.0, 0.0, -StreetBelow));
	LooterSound::PlayAt(this, LooterSoundCue::Fauna::ClothFlap, Landed, 1.f, 0.8f);
	LamplighterEffects::DustSkirt(this, Landed, ClothLength * 0.5f, 10);
	if (bTold)
	{
		return;
	}
	bTold = true;
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(DownEvent, this));
		if (!DownFlag.IsNone())
		{
			Runner->SetFlag(DownFlag);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: down in the street."), *GetActorNameOrLabel());
}

void ACompanyBanner::HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	if (HitComponent == RopeLeftHit)
	{
		CutRope(0, InstigatedBy);
	}
	else if (HitComponent == RopeRightHit)
	{
		CutRope(1, InstigatedBy);
	}
}
