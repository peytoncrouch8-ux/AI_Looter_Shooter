// AAmosWhitlock: Amos at his fence in Whitlock Fields, a friendly ghost only Ellis can see (Side 2).

#include "Story/AmosWhitlock.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionRunner.h"
#include "Story/SpeakerPointComponent.h"
#include "AnimationRuntime.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "ReferenceSkeleton.h"
#include "UObject/ConstructorHelpers.h"

const FName AAmosWhitlock::SpeakerTag(TEXT("Speaker_Amos"));
const FName AAmosWhitlock::MissionId(TEXT("Side2"));
const TCHAR* const AAmosWhitlock::ModelPath = TEXT("/Game/Art/Creatures/SK_Amos.SK_Amos");
const TCHAR* const AAmosWhitlock::HatPath = TEXT("/Game/Art/Creatures/SM_AmosHat.SM_AmosHat");
const TCHAR* const AAmosWhitlock::ForkPath = TEXT("/Game/Art/Creatures/SM_AmosFork.SM_AmosFork");
const FName AAmosWhitlock::SpeakerSocket(TEXT("Speaker"));

namespace
{
	/** Side 2 opens once Main 4, "Hallowed Ground", is finished: he's at his fence from then on. */
	const FName OpensAfter(TEXT("Main4"));

	/** His hull round his body for the Interact key's line (cm): its radius, and how far under his pelvis and over his head it reaches. */
	constexpr double HullRadius = 27.5;
	constexpr double HullBelowPelvis = 55.0;
	constexpr double HullOverHead = 28.0;

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

	/** A prop on its bone as the model has it at rest: at the bone, turned back by the bone's rest turn (as the Unpaid's hat). */
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

	/** His jaw while he speaks: open half to all the way, the talk pose's 8 degrees (shut at rest). */
	float SpeakingJaw(float Clock)
	{
		return 0.75f + 0.25f * FMath::Sin(Clock * 11.f) * FMath::Abs(FMath::Sin(Clock * 2.3f));
	}

	/** Eased in and out, so he lifts off the rail's far side and settles on it gently. */
	float Ease(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

FLinearColor AAmosWhitlock::EmberColor()
{
	return FLinearColor::FromSRGBColor(FColor(0x96, 0x42, 0x1F));
}

AAmosWhitlock::AAmosWhitlock()
{
	// Ticks only while something of him moves (UpdateTicking): talking, turning back, settling onto the rail.
	PrimaryActorTick.bStartWithTickEnabled = false;
	// The story character's own body pose (a breath, a turn of the whole body) isn't his: his head turns instead.
	TurnSpeed = 0.f;
	BreathHeight = 0.f;
	SpeakerPoint->SpeakerName = NSLOCTEXT("LooterAmos", "AmosName", "Amos Whitlock");
	SpeakerPoint->Reach = 400.f;
	Tags.Add(SpeakerTag);
	// At his fence once Side 2 opens; on the rail once it's done (the build script sets the same).
	ShownWhen.AfterMissions = { OpensAfter };
	SitWhen.AfterMissions = { MissionId };

	static USkeletalMesh* const Model = FindIfMade<USkeletalMesh>(ModelPath);
	static UStaticMesh* const HatModel = FindIfMade<UStaticMesh>(HatPath);
	static UStaticMesh* const ForkModel = FindIfMade<UStaticMesh>(ForkPath);

	// The hull: hidden, about his body, found only by the Interact key's line (Visibility). He's a ghost: the player, shots,
	// pellets and the camera pass through him. World dynamic, so traces for the ground never land on him.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
	Body->SetHiddenInGame(Model != nullptr);
	Body->SetCastShadow(false);

	Figure = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Figure"));
	Figure->SetupAttachment(Root);
	Figure->SetSkinnedAsset(Model);
	Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// A ghost casts no shadow (as every Unpaid). Its bounds grow a little: the lean reaches over the rail, the sit lies
	// higher and its shroud trails back under it.
	Figure->SetCastShadow(false);
	Figure->SetBoundsScale(1.4f);
	// Nothing of him moves most of the time: a look a few times a second is plenty for his LODs.
	Figure->SetComponentTickInterval(0.25f);

	Hat = MakeProp(this, TEXT("Hat"), HatModel);
	WearOnBone(Hat, Figure, Model, TEXT("hat"));
	Fork = MakeProp(this, TEXT("Fork"), ForkModel);
	WearOnBone(Fork, Figure, Model, TEXT("fork"));

	// His words come from his mouth: the hat's socket (skinned meshes export without sockets), riding his head.
	if (HatModel && HatModel->FindSocket(SpeakerSocket))
	{
		SpeakerPoint->SetupAttachment(Hat, SpeakerSocket);
		SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
	}
}

void AAmosWhitlock::BeginPlay()
{
	// Leaning on the rail until the story says otherwise; the story character's start reads it (RefreshShown).
	Seat = EAmosPose::Lean;
	bSettling = false;
	ApplyLook();
	PoseFigure();
	PlaceHull();
	if (SpeakerPoint)
	{
		SpeakerPoint->OnTalked.AddUObject(this, &AAmosWhitlock::HandleTalked);
	}
	Super::BeginPlay();
	bBegun = true;
}

void AAmosWhitlock::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SpeakerPoint)
	{
		SpeakerPoint->OnTalked.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool AAmosWhitlock::HasModel() const
{
	return Figure && Figure->GetSkinnedAsset() != nullptr;
}

// ---------------------------------------------------------------------------
// The story: there or not, leaning or sitting
// ---------------------------------------------------------------------------

bool AAmosWhitlock::WantsToSit() const
{
	if (bSatByHand)
	{
		return true;
	}
	if (SitWhen.IsEmpty())
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && SitWhen.IsMet(Runner->GetCampaign(), Runner);
}

void AAmosWhitlock::RefreshShown()
{
	SetShown(IsStoryShown());
	if (IsShown())
	{
		const bool bWants = WantsToSit();
		if (bWants && Seat == EAmosPose::Lean)
		{
			// As the level begins he's simply on the rail; later he settles there once his words are done (UpdatePose
			// waits for them while he's still talking).
			if (!bBegun)
			{
				StartSitting(/*bAtOnce*/ true);
			}
			else if (!(SpeakerPoint && SpeakerPoint->IsTalking()))
			{
				StartSitting(/*bAtOnce*/ false);
			}
		}
		else if (!bWants && Seat == EAmosPose::Sit)
		{
			LeanNow();
		}
	}
	UpdateTicking();
}

void AAmosWhitlock::SitNow(bool bAtOnce)
{
	bSatByHand = true;
	if (Seat == EAmosPose::Lean || (bAtOnce && bSettling))
	{
		StartSitting(bAtOnce);
	}
	UpdateTicking();
}

void AAmosWhitlock::LeanNow()
{
	bSatByHand = false;
	Seat = EAmosPose::Lean;
	bSettling = false;
	SettleClock = 0.f;
	PoseFigure();
	PlaceHull();
	UpdateTicking();
}

void AAmosWhitlock::StartSitting(bool bAtOnce)
{
	Seat = EAmosPose::Sit;
	bSettling = !bAtOnce && SitSeconds > 0.f;
	SettleClock = 0.f;
	// The hull and his words' place go to the rail at once: there's no talking to him on the way.
	PlaceHull();
	PoseFigure();
	if (bBegun)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: Amos %s on his fence to wait for the saint to come back."), *GetActorNameOrLabel(),
			bSettling ? TEXT("settles") : TEXT("sits"));
	}
}

// ---------------------------------------------------------------------------
// Talking, and the pose
// ---------------------------------------------------------------------------

void AAmosWhitlock::HandleTalked(USpeakerPointComponent& Point, AActor* InListener)
{
	Listener = InListener;
	SpeakClock = 0.f;
	UpdateTicking();
}

float AAmosWhitlock::PoseHeadYaw() const
{
	return AmosPoses::HeadYawOf(Seat);
}

FTransform AAmosWhitlock::GetSeatTransform(EAmosPose InSeat) const
{
	// The actor stands on the fence's line midway between two posts, facing across it: leaning, he stands behind the line
	// square to it; sitting, he's on it, turned to his right (Amos.py's LEAN_FRAME and SIT_FRAME).
	if (InSeat == EAmosPose::Sit)
	{
		return FTransform(FRotator(0.0, AmosPoses::SitTurn(), 0.0), FVector::ZeroVector);
	}
	return FTransform(FRotator::ZeroRotator, FVector(-AmosPoses::LeanBack(), 0.0, 0.0));
}

FVector AAmosWhitlock::GetHeadLocation() const
{
	const int32 Head = AmosPoses::FindBone(TEXT("head"));
	FAmosPoseLayer Layer;
	Layer.HeadYaw = HeadYaw;
	const FAmosPoseHeads Solved = AmosPoses::SolveHeads(Seat, Layer);
	const FVector InFigure = Solved.Heads.IsValidIndex(Head) ? Solved.Heads[Head] : FVector(0.0, 0.0, 160.0);
	return GetActorTransform().TransformPosition(GetSeatTransform(Seat).TransformPosition(InFigure));
}

void AAmosWhitlock::UpdatePose(float DeltaSeconds)
{
	// Not the story character's own pose (a breath, the whole body turning): his hull stays where the seat puts it.
	const bool bTalking = SpeakerPoint && SpeakerPoint->IsTalking();
	float WantedYaw = 0.f;
	const AActor* Hearing = Listener.Get();
	if (bTalking && Hearing)
	{
		// To whoever he's talking to, as far as his neck allows, from where the pose has him looking (the sit: the sun).
		const FVector To = Hearing->GetActorLocation() - GetHeadLocation();
		const float Facing = static_cast<float>(GetActorRotation().Yaw + GetSeatTransform(Seat).Rotator().Yaw) + PoseHeadYaw();
		WantedYaw = FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(To.Rotation().Yaw - Facing)), -HeadTurnLimit, HeadTurnLimit);
	}
	SpeakClock += DeltaSeconds;
	const float WantedJaw = bTalking ? SpeakingJaw(SpeakClock) : 0.f;
	const float NewYaw = FMath::FixedTurn(HeadYaw, WantedYaw, HeadTurnSpeed * DeltaSeconds);
	float NewJaw = FMath::FInterpTo(JawOpen, WantedJaw, DeltaSeconds, 12.f);
	if (!bTalking && NewJaw < 0.01f)
	{
		NewJaw = 0.f;
	}
	bool bChanged = !FMath::IsNearlyEqual(NewYaw, HeadYaw, 0.05f) || !FMath::IsNearlyEqual(NewJaw, JawOpen, 0.005f);
	HeadYaw = NewYaw;
	JawOpen = NewJaw;

	// His last words over, and the story has him on the rail: he settles there.
	if (!bTalking && Seat == EAmosPose::Lean && WantsToSit())
	{
		StartSitting(/*bAtOnce*/ false);
		bChanged = true;
	}
	if (bSettling)
	{
		SettleClock += DeltaSeconds;
		bSettling = SettleClock < SitSeconds;
		bChanged = true;
	}
	// Only when something changed: still, nothing is posed again.
	if (bChanged)
	{
		PoseFigure();
	}
	UpdateTicking();
}

void AAmosWhitlock::PoseFigure()
{
	if (!Figure)
	{
		return;
	}
	FAmosPoseLayer Layer;
	Layer.HeadYaw = HeadYaw;
	Layer.JawOpen = JawOpen;
	const float Share = bSettling && SitSeconds > 0.f ? Ease(SettleClock / SitSeconds) : 1.f;

	// Where the figure stands: behind the rail leaning, on it sitting, drifting between them while he settles.
	FTransform Place = GetSeatTransform(Seat);
	if (bSettling)
	{
		const FTransform From = GetSeatTransform(EAmosPose::Lean);
		Place = FTransform(FQuat::Slerp(From.GetRotation(), Place.GetRotation(), Share),
			FMath::Lerp(From.GetLocation(), Place.GetLocation(), static_cast<double>(Share)));
	}
	Figure->SetRelativeTransform(Place);

	const USkeletalMesh* Model = Cast<USkeletalMesh>(Figure->GetSkinnedAsset());
	if (!Model)
	{
		return;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	TArray<FTransform> Bones = AmosPoses::Solve(Skeleton, Seat, Layer);
	if (bSettling)
	{
		Bones = AmosPoses::Blend(AmosPoses::Solve(Skeleton, EAmosPose::Lean, Layer), Bones, Share);
	}
	// Parents first: each bone set in component space against where its parent already is.
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		Figure->SetBoneTransformByName(Skeleton.GetBoneName(Index), Bones[Index], EBoneSpaces::ComponentSpace);
	}
	Figure->RefreshBoneTransforms();
}

void AAmosWhitlock::PlaceHull()
{
	const int32 Pelvis = AmosPoses::FindBone(TEXT("pelvis"));
	const int32 Head = AmosPoses::FindBone(TEXT("head"));
	const FAmosPoseHeads Solved = AmosPoses::SolveHeads(Seat);
	if (!Solved.Heads.IsValidIndex(Pelvis) || !Solved.Heads.IsValidIndex(Head))
	{
		return;
	}
	const FTransform Place = GetSeatTransform(Seat);
	const FVector Low = Solved.Heads[Pelvis];
	const FVector High = Solved.Heads[Head];
	const double Bottom = Low.Z - HullBelowPelvis;
	const double Top = High.Z + HullOverHead;
	// Upright round the line from his pelvis to his head (the engine's cylinder: 1 m across and tall, about its middle).
	const FVector Middle((Low.X + High.X) * 0.5, (Low.Y + High.Y) * 0.5, (Bottom + Top) * 0.5);
	Body->SetRelativeLocationAndRotation(Place.TransformPosition(Middle), Place.GetRotation());
	Body->SetRelativeScale3D(FVector(HullRadius / 50.0, HullRadius / 50.0, (Top - Bottom) / 100.0));

	// Without the hat's socket his words come from where his mouth is in the seat.
	if (SpeakerPoint && SpeakerPoint->GetAttachParent() != Hat.Get())
	{
		const FVector Mouth = High + Solved.Turns[Head].RotateVector(AmosPoses::SpeakerFromHead());
		SpeakerPoint->SetRelativeLocation(Place.TransformPosition(Mouth));
	}
}

void AAmosWhitlock::ApplyLook()
{
	// His coal banked low: a dull ember under ash (Amos.py's EMBER_COLOR, its strength and heat), on his body and props.
	const FLinearColor Ember = EmberColor();
	const FVector4 RankColor(Ember.R, Ember.G, Ember.B, EmberStrength);
	const TArray<UPrimitiveComponent*> Parts = { Figure.Get(), Hat.Get(), Fork.Get() };
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

// ---------------------------------------------------------------------------
// Ticking only while something moves
// ---------------------------------------------------------------------------

bool AAmosWhitlock::NeedsTick() const
{
	if (!IsShown())
	{
		return false;
	}
	const bool bTalking = SpeakerPoint && SpeakerPoint->IsTalking();
	return bTalking || bSettling || !FMath::IsNearlyZero(HeadYaw, 0.05f) || JawOpen > 0.f;
}

void AAmosWhitlock::UpdateTicking()
{
	SetActorTickEnabled(NeedsTick());
}
