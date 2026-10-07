// AAbelOnBoard: Abel sat on his burial board after his fight, a friend for the rest of the game.

#include "Story/AbelOnBoard.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/AbelPoses.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/UnpaidCreature.h"
#include "Story/SpeakerPointComponent.h"
#include "AnimationRuntime.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "ReferenceSkeleton.h"
#include "UObject/ConstructorHelpers.h"

const FName AAbelOnBoard::SpeakerTag(TEXT("Speaker_Abel"));
const TCHAR* const AAbelOnBoard::ModelPath = TEXT("/Game/Art/Creatures/SK_Abel.SK_Abel");
const TCHAR* const AAbelOnBoard::HatPath = TEXT("/Game/Art/Creatures/SM_AbelHat.SM_AbelHat");
const TCHAR* const AAbelOnBoard::LanternPath = TEXT("/Game/Art/Creatures/SM_AbelLantern.SM_AbelLantern");
const TCHAR* const AAbelOnBoard::PumpPath = TEXT("/Game/Art/Creatures/SM_AbelPump.SM_AbelPump");

namespace
{
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

	/** A prop on its bone as the model has it at rest: at the bone, turned back by the bone's rest turn. */
	void WearOnBone(UStaticMeshComponent* Part, USceneComponent* Body, const USkeletalMesh* Model, FName Bone)
	{
		Part->SetupAttachment(Body, Bone);
		const int32 Index = Model ? Model->GetRefSkeleton().FindBoneIndex(Bone) : INDEX_NONE;
		if (Index != INDEX_NONE)
		{
			Part->SetRelativeRotation(FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), Index).GetRotation().Inverse());
		}
	}

	UStaticMeshComponent* MakeProp(AActor* Owner, const TCHAR* Name, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetStaticMesh(Mesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCastShadow(false);
		return Part;
	}

	/** His head over the board in the sit, before his size (cm): the table's head, a little up to his eyes. */
	FVector SeatedHead()
	{
		const int32 Head = AbelPoses::FindBone(TEXT("head"));
		const FAbelPoseHeads Sit = AbelPoses::SolveHeads(EAbelPose::Sit);
		return Sit.Heads.IsValidIndex(Head) ? Sit.Heads[Head] + FVector(0.0, 0.0, 10.0) : FVector(4.0, 0.0, 80.0);
	}

	/** His jaw moves 6 to 10 degrees while he speaks (his jaw is closed at rest). */
	float SpeakingJaw(float Clock)
	{
		return 8.f + 2.f * FMath::Sin(Clock * 11.f) * FMath::Abs(FMath::Sin(Clock * 2.3f));
	}
}

AAbelOnBoard::AAbelOnBoard()
{
	// A seated figure: he never turns his body or breathes it; only his head and jaw move while he talks.
	TurnSpeed = 0.f;
	BreathHeight = 0.f;
	SpeakerPoint->SpeakerName = NSLOCTEXT("LooterAbel", "AbelName", "Abel");
	SpeakerPoint->Reach = 400.f;
	Tags.Add(SpeakerTag);
	// After his fight, for good (the build script sets the same).
	ShownWhen.AfterMissions = { TEXT("Main6") };

	static USkeletalMesh* const Model = FindIfMade<USkeletalMesh>(ModelPath);
	static UStaticMesh* const HatModel = FindIfMade<UStaticMesh>(HatPath);
	static UStaticMesh* const LanternModel = FindIfMade<UStaticMesh>(LanternPath);
	static UStaticMesh* const PumpModel = FindIfMade<UStaticMesh>(PumpPath);

	// The hull: hidden, about his seated size on the board, solid to the player and the Interact line; shots, pellets
	// and the camera pass through it.
	Body->SetRelativeLocation(FVector(0.0, 0.0, 55.0));
	Body->SetRelativeScale3D(FVector(0.6, 0.6, 1.1));
	Body->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2 /*Weapon: bullets*/, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel1 /*Projectile: pellets*/, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Body->SetHiddenInGame(Model != nullptr);
	Body->SetCastShadow(false);

	Figure = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Figure"));
	Figure->SetupAttachment(Root);
	Figure->SetSkinnedAsset(Model);
	Figure->SetRelativeScale3D(FVector(BodyScale));
	Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// A ghost casts no shadow (as every Unpaid). Its bounds grow a little: the sit lies lower and longer than the rest pose.
	Figure->SetCastShadow(false);
	Figure->SetBoundsScale(1.4f);
	// Nothing of him moves but his head now and then: a look a few times a second is plenty for his LODs.
	Figure->SetComponentTickInterval(0.25f);

	Hat = MakeProp(this, TEXT("Hat"), HatModel);
	WearOnBone(Hat, Figure, Model, TEXT("hat"));
	Lantern = MakeProp(this, TEXT("Lantern"), LanternModel);
	WearOnBone(Lantern, Figure, Model, TEXT("lantern"));
	Pump = MakeProp(this, TEXT("Pump"), PumpModel);
	WearOnBone(Pump, Figure, Model, TEXT("gun"));

	LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight"));
	LanternLight->SetupAttachment(Lantern, TEXT("Light"));
	LanternLight->SetCastShadows(false);
	LanternLight->SetIntensityUnits(ELightUnits::Candelas);
	LanternLight->SetIntensity(5.f);
	LanternLight->SetAttenuationRadius(550.f);
	// His ghost light: a pale, warm white (Abel.py's flame), no rarity color.
	LanternLight->SetLightColor(FLinearColor::FromSRGBColor(FColor(0xFF, 0xF0, 0xD6)));

	// His captions come from his head, where the player looks to talk to him.
	SpeakerPoint->SetRelativeLocation(SeatedHead() * BodyScale);
}

void AAbelOnBoard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Figure->SetRelativeScale3D(FVector(BodyScale));
	ApplySeat();
}

void AAbelOnBoard::BeginPlay()
{
	Figure->SetRelativeScale3D(FVector(BodyScale));
	SpeakerPoint->SetRelativeLocation(SeatedHead() * BodyScale);
	if (SpeakerPoint)
	{
		SpeakerPoint->OnTalked.AddUObject(this, &AAbelOnBoard::HandleTalked);
	}
	ApplySeat();
	ApplyEmber();
	// The story character's start reads the story: shown or not as the level begins.
	Super::BeginPlay();
}

void AAbelOnBoard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SpeakerPoint)
	{
		SpeakerPoint->OnTalked.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool AAbelOnBoard::ApplySeat()
{
	const USkeletalMesh* Model = Cast<USkeletalMesh>(Figure->GetSkinnedAsset());
	if (!Model)
	{
		return false;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	Seated = AbelPoses::Solve(Skeleton, EAbelPose::Sit);
	Rest.SetNum(Skeleton.GetNum());
	for (int32 Index = 0; Index < Skeleton.GetNum(); ++Index)
	{
		Rest[Index] = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index);
	}
	HeadIndex = Skeleton.FindBoneIndex(TEXT("head"));
	JawIndex = Skeleton.FindBoneIndex(TEXT("jaw"));
	// Parents first: each bone set in component space against where its parent already is.
	for (int32 Index = 0; Index < Seated.Num(); ++Index)
	{
		Figure->SetBoneTransformByName(Skeleton.GetBoneName(Index), Seated[Index], EBoneSpaces::ComponentSpace);
	}
	Figure->RefreshBoneTransforms();
	bSeated = Seated.Num() > 0;
	HeadYaw = 0.f;
	JawOpen = 0.f;
	return bSeated;
}

void AAbelOnBoard::PoseHead(float Yaw, float Jaw)
{
	if (!bSeated || !Seated.IsValidIndex(HeadIndex) || !Rest.IsValidIndex(HeadIndex))
	{
		return;
	}
	const USkeletalMesh* Model = Cast<USkeletalMesh>(Figure->GetSkinnedAsset());
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	// The head's turn in the sit, turned about the body's up at its own joint; the jaw's under it, opened about its hinge.
	const FQuat HeadTurned = FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw)) * (Seated[HeadIndex].GetRotation() * Rest[HeadIndex].GetRotation().Inverse());
	Figure->SetBoneTransformByName(Skeleton.GetBoneName(HeadIndex),
		FTransform(HeadTurned * Rest[HeadIndex].GetRotation(), Seated[HeadIndex].GetLocation()), EBoneSpaces::ComponentSpace);
	if (Seated.IsValidIndex(JawIndex))
	{
		const FQuat JawTurned = HeadTurned * FQuat(FVector::RightVector, FMath::DegreesToRadians(Jaw));
		const FVector JawAt = Seated[HeadIndex].GetLocation() + HeadTurned.RotateVector(Rest[JawIndex].GetLocation() - Rest[HeadIndex].GetLocation());
		Figure->SetBoneTransformByName(Skeleton.GetBoneName(JawIndex), FTransform(JawTurned * Rest[JawIndex].GetRotation(), JawAt),
			EBoneSpaces::ComponentSpace);
	}
	Figure->RefreshBoneTransforms();
}

void AAbelOnBoard::UpdatePose(float DeltaSeconds)
{
	Super::UpdatePose(DeltaSeconds);
	const bool bTalking = SpeakerPoint && SpeakerPoint->IsTalking();
	float WantedYaw = 0.f;
	const AActor* Hearing = Listener.Get();
	if (bTalking && Hearing)
	{
		// To whoever he's talking to, as far as his neck allows, from the way he sits (the sunset).
		const FVector To = Hearing->GetActorLocation() - GetHeadLocation();
		WantedYaw = FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(To.Rotation().Yaw - GetActorRotation().Yaw)), -HeadTurnLimit, HeadTurnLimit);
	}
	SpeakClock += DeltaSeconds;
	const float WantedJaw = bTalking ? SpeakingJaw(SpeakClock) : 0.f;
	const float NewYaw = FMath::FixedTurn(HeadYaw, WantedYaw, HeadTurnSpeed * DeltaSeconds);
	const float NewJaw = FMath::FInterpTo(JawOpen, WantedJaw, DeltaSeconds, 12.f);
	// Only when something changed: sat still, nothing is posed again.
	if (!FMath::IsNearlyEqual(NewYaw, HeadYaw, 0.05f) || !FMath::IsNearlyEqual(NewJaw, JawOpen, 0.05f))
	{
		HeadYaw = NewYaw;
		JawOpen = NewJaw;
		PoseHead(HeadYaw, JawOpen);
	}
}

void AAbelOnBoard::HandleTalked(USpeakerPointComponent& Point, AActor* InListener)
{
	Listener = InListener;
}

FVector AAbelOnBoard::GetHeadLocation() const
{
	return GetActorTransform().TransformPosition(SeatedHead() * BodyScale);
}

void AAbelOnBoard::ApplyEmber()
{
	// The Boss rank's gold, glowing low; the coal sunk to an ember. The same on his props, as the boss wears them.
	const FLinearColor Gold = UCreatureRankSettings::Get(ECreatureRank::Boss).Color;
	const FVector4 RankColor(Gold.R, Gold.G, Gold.B, 0.6f);
	const TArray<UPrimitiveComponent*> Parts = { Figure.Get(), Hat.Get(), Lantern.Get(), Pump.Get() };
	for (UPrimitiveComponent* Part : Parts)
	{
		if (Part)
		{
			Part->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, RankColor);
			Part->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, 0.f);
			Part->SetCustomPrimitiveDataFloat(UnpaidLook::HeatIndex, EmberHeat);
		}
	}
}

void AAbelOnBoard::RefreshShown()
{
	const bool bStory = IsStoryShown();
	// Once the story has him there, the scene's word is no longer needed.
	if (bStory)
	{
		bShownByScene = false;
	}
	SetShown(bStory || bShownByScene);
}

void AAbelOnBoard::ForgetScene()
{
	bShownByScene = false;
	RefreshShown();
}

void AAbelOnBoard::ShowNow()
{
	bShownByScene = true;
	if (!bSeated)
	{
		ApplySeat();
	}
	ApplyEmber();
	SetShown(true);
	UE_LOG(LogLooter, Log, TEXT("%s: Abel sits on his board, facing the sunset."), *GetActorNameOrLabel());
}
