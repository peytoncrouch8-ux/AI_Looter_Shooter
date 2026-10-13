// ADeadFriend: a friendly dead man on the Unpaid's rig, posed by code from his pose table (Hiram Coyle, Thaddeus Bright).

#include "Story/DeadFriend.h"
#include "Creatures/UnpaidCreature.h"
#include "Story/SpeakerPointComponent.h"
#include "AnimationRuntime.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ReferenceSkeleton.h"
#include "UObject/ConstructorHelpers.h"

const FName ADeadFriend::SpeakerSocket(TEXT("Speaker"));

namespace
{
	/** The Unpaid's body, his until his own is made. */
	const TCHAR* const UnpaidPath = TEXT("/Game/Art/Creatures/SK_Unpaid.SK_Unpaid");

	/** The body's slot his tint goes on (the Unpaid's), and its coal's (which keeps its own). */
	const FName BodySlot(TEXT("Ghost_A"));

	/** His hull round his body for the Interact key's line (cm): its radius, and how far over his head it reaches. */
	constexpr double HullRadius = 30.0;
	constexpr double HullOverHead = 28.0;

	/** His mouth from the head bone, where the pose has his head (cm, the head's own frame: X forward, Z up). */
	const FVector MouthFromHead(11.0, 0.0, 4.0);

	/** He looks for a player near him this often (s), and moves his hull and words' place with his pose this often. */
	constexpr float NearCheckEvery = 0.5f;
	constexpr float HullEvery = 0.5f;

	/** His jaw while he speaks: open half to all the way. */
	float SpeakingJaw(float Clock)
	{
		return 0.75f + 0.25f * FMath::Sin(Clock * 11.f) * FMath::Abs(FMath::Sin(Clock * 2.3f));
	}

	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!ObjectPath || !FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

USkeletalMesh* ADeadFriend::FindModel(const TCHAR* Path)
{
	return FindIfMade<USkeletalMesh>(Path);
}

UStaticMesh* ADeadFriend::FindPart(const TCHAR* Path)
{
	return FindIfMade<UStaticMesh>(Path);
}

UMaterialInterface* ADeadFriend::FindTint(const TCHAR* Path)
{
	return FindIfMade<UMaterialInterface>(Path);
}

FLinearColor ADeadFriend::EmberColor()
{
	return FLinearColor::FromSRGBColor(FColor(0x96, 0x42, 0x1F));
}

ADeadFriend::ADeadFriend()
{
	// The story character's own body pose (a breath, the whole body turning) isn't his: his head turns instead.
	TurnSpeed = 0.f;
	BreathHeight = 0.f;
	SpeakerPoint->Reach = 400.f;

	// The hull: hidden once he has a body, found only by the Interact key's line (Visibility). He's a ghost: the player,
	// shots, pellets and the camera pass through him. World dynamic, so traces for the ground never land on him.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
	Body->SetCastShadow(false);

	Figure = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Figure"));
	Figure->SetupAttachment(Root);
	Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// A ghost casts no shadow (as every Unpaid); its bounds grow a little for a sit's or a lean's reach.
	Figure->SetCastShadow(false);
	Figure->SetBoundsScale(1.4f);

	Hat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hat"));
	Hat->SetupAttachment(Figure);
	Hat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hat->SetGenerateOverlapEvents(false);
	Hat->SetCanEverAffectNavigation(false);
	Hat->SetCastShadow(false);
}

void ADeadFriend::WearModel(USkeletalMesh* Model, UMaterialInterface* Tint)
{
	static USkeletalMesh* const Unpaid = FindModel(UnpaidPath);
	USkeletalMesh* Worn = Model ? Model : Unpaid;
	Figure->SetSkinnedAsset(Worn);
	WornModel = Worn;
	Body->SetHiddenInGame(Worn != nullptr);
	if (Tint && Worn)
	{
		const int32 Slot = Worn->GetMaterials().IndexOfByPredicate([](const FSkeletalMaterial& Each) { return Each.MaterialSlotName == BodySlot; });
		if (Slot != INDEX_NONE)
		{
			Figure->SetMaterial(Slot, Tint);
		}
	}
}

void ADeadFriend::WearOnBone(UStaticMeshComponent* Part, UStaticMesh* Mesh, FName Bone)
{
	Part->SetStaticMesh(Mesh);
	Part->SetupAttachment(Figure, Bone);
	const USkeletalMesh* Model = WornModel;
	const int32 Index = Model ? Model->GetRefSkeleton().FindBoneIndex(Bone) : INDEX_NONE;
	if (Index != INDEX_NONE)
	{
		Part->SetRelativeRotation(FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), Index).GetRotation().Inverse());
	}
	Part->SetVisibility(Mesh != nullptr);
}

void ADeadFriend::BeginPlay()
{
	ApplyLook();
	// His words from his mouth: the hat's socket when it has one (skinned meshes export without sockets), riding his head.
	if (Hat->GetStaticMesh() && Hat->DoesSocketExist(SpeakerSocket))
	{
		SpeakerPoint->AttachToComponent(Hat, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SpeakerSocket);
	}
	SpeakerPoint->OnTalked.AddUObject(this, &ADeadFriend::HandleTalked);
	PoseNow();
	Super::BeginPlay();
}

void ADeadFriend::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SpeakerPoint)
	{
		SpeakerPoint->OnTalked.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool ADeadFriend::HasModel() const
{
	return Figure && Figure->GetSkinnedAsset() != nullptr;
}

bool ADeadFriend::IsTalking() const
{
	return SpeakerPoint && SpeakerPoint->IsTalking();
}

const APawn* ADeadFriend::FindPlayer() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

void ADeadFriend::HandleTalked(USpeakerPointComponent& Point, AActor* InListener)
{
	Listener = InListener;
	SpeakClock = 0.f;
}

// ---------------------------------------------------------------------------
// The pose
// ---------------------------------------------------------------------------

void ADeadFriend::PoseNow()
{
	Pose = WantedPose(FriendClock);
	bPoseStarted = true;
	ApplyPose();
	PlaceHull();
}

void ADeadFriend::UpdatePose(float DeltaSeconds)
{
	// Only near a player (looked at a few times a second), or while he talks.
	NearCheck -= DeltaSeconds;
	if (NearCheck <= 0.f)
	{
		NearCheck = NearCheckEvery;
		const APawn* Player = FindPlayer();
		bPlayerNear = !Player || FVector::Dist(Player->GetActorLocation(), GetActorLocation()) <= AnimateRadius;
	}
	const bool bTalking = IsTalking();
	if (!bPlayerNear && !bTalking)
	{
		return;
	}
	FriendClock += DeltaSeconds;
	TickFriend(DeltaSeconds);

	// His head to whoever he's talking to, as far as his neck allows; his jaw while his lines play.
	float WantedYaw = 0.f;
	const AActor* Hearing = Listener.Get();
	if (bTalking && Hearing)
	{
		const FVector To = Hearing->GetActorLocation() - GetHeadLocation();
		const USkeletalMesh* Model = Cast<USkeletalMesh>(Figure->GetSkinnedAsset());
		const float PoseYaw = Model ? DeadFriendPoses::BoneYaw(Model->GetRefSkeleton(), Solved, DeadFriendPoses::Head) - HeadYaw : 0.f;
		const float Facing = static_cast<float>(GetActorRotation().Yaw + Figure->GetRelativeRotation().Yaw) + PoseYaw;
		WantedYaw = FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(To.Rotation().Yaw - Facing)), -HeadTurnLimit, HeadTurnLimit);
	}
	SpeakClock += DeltaSeconds;
	HeadYaw = FMath::FixedTurn(HeadYaw, WantedYaw, HeadTurnSpeed * DeltaSeconds);
	JawOpen = FMath::FInterpTo(JawOpen, bTalking ? JawDegrees * SpeakingJaw(SpeakClock) : 0.f, DeltaSeconds, 12.f);

	// Eased toward the pose his moment wants.
	const FDeadFriendPose Wanted = WantedPose(FriendClock);
	Pose = bPoseStarted ? FDeadFriendPose::Blend(Pose, Wanted, 1.f - FMath::Exp(-PoseEase * DeltaSeconds)) : Wanted;
	bPoseStarted = true;
	ApplyPose();
	HullClock -= DeltaSeconds;
	if (HullClock <= 0.f)
	{
		HullClock = HullEvery;
		PlaceHull();
	}
}

void ADeadFriend::ApplyPose()
{
	const USkeletalMesh* Model = Cast<USkeletalMesh>(Figure ? Figure->GetSkinnedAsset() : nullptr);
	if (!Model)
	{
		Solved.Reset();
		return;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	FDeadFriendLayer Layer;
	Layer.HeadYaw = HeadYaw;
	Layer.JawOpen = JawOpen;
	Solved = DeadFriendPoses::Solve(Skeleton, Pose, Layer);
	// Parents first: each bone set in component space against where its parent already is.
	for (int32 Index = 0; Index < Solved.Num(); ++Index)
	{
		Figure->SetBoneTransformByName(Skeleton.GetBoneName(Index), Solved[Index], EBoneSpaces::ComponentSpace);
	}
	Figure->RefreshBoneTransforms();
}

FVector ADeadFriend::GetHeadLocation() const
{
	const USkeletalMesh* Model = Cast<USkeletalMesh>(Figure ? Figure->GetSkinnedAsset() : nullptr);
	if (Model && Solved.Num() > 0)
	{
		const FVector InFigure = DeadFriendPoses::BoneLocation(Model->GetRefSkeleton(), Solved, DeadFriendPoses::Head);
		return Figure->GetComponentTransform().TransformPosition(InFigure);
	}
	return GetActorLocation() + FVector(0.0, 0.0, 160.0);
}

void ADeadFriend::PlaceHull()
{
	const USkeletalMesh* Model = Cast<USkeletalMesh>(Figure ? Figure->GetSkinnedAsset() : nullptr);
	if (!Model || Solved.Num() == 0)
	{
		return;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	const int32 HeadIndex = Skeleton.FindBoneIndex(DeadFriendPoses::Head);
	if (!Solved.IsValidIndex(HeadIndex))
	{
		return;
	}
	const FTransform& HeadPose = Solved[HeadIndex];
	const FTransform Place = Figure->GetRelativeTransform();
	const FVector Head = HeadPose.GetLocation();
	const double Top = Head.Z + HullOverHead;
	// Upright from the floor to over his head, round his middle (the engine's cylinder: 1 m across and tall, about its middle).
	Body->SetRelativeLocationAndRotation(Place.TransformPosition(FVector(Head.X * 0.5, Head.Y * 0.5, Top * 0.5)), Place.GetRotation());
	Body->SetRelativeScale3D(FVector(HullRadius / 50.0, HullRadius / 50.0, Top / 100.0));
	// Without the hat's socket his words come from where the pose has his mouth.
	if (SpeakerPoint && SpeakerPoint->GetAttachParent() != Hat.Get())
	{
		const FQuat FromRest = HeadPose.GetRotation() * FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, HeadIndex).GetRotation().Inverse();
		SpeakerPoint->SetRelativeLocation(Place.TransformPosition(Head + FromRest.RotateVector(MouthFromHead)));
	}
}

void ADeadFriend::ApplyLook()
{
	// His coal banked low: a dull ember under ash, on his body and all his things (not the hidden hull).
	const FLinearColor Ember = EmberColor();
	const FVector4 RankColor(Ember.R, Ember.G, Ember.B, EmberStrength);
	TArray<UPrimitiveComponent*> Parts;
	GetComponents<UPrimitiveComponent>(Parts);
	for (UPrimitiveComponent* Part : Parts)
	{
		if (Part && Part != Body.Get())
		{
			Part->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, RankColor);
			Part->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, 0.f);
			Part->SetCustomPrimitiveDataFloat(UnpaidLook::HeatIndex, EmberHeat);
		}
	}
}
