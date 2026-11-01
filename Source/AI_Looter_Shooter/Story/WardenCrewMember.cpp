// AWardenCrewMember: the Lamplighter's crew on deck (Captain Holloway, Brother Finch, Junie Calloway). Their table (who
// they are, their stand-ins, their stations) is WardenCrewStations.cpp; the pose layer WardenCrewPose.

#include "Story/WardenCrewMember.h"
#include "AI_Looter_Shooter.h"
#include "Embers/HeroDefinition.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/WardenCrewAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The full-size mannequin's height and eyes (cm, its feet to its crown; ahead of and over its feet). */
	constexpr float CrewFullHeight = 180.f;
	const FVector CrewEyes(8.0, 0.0, 165.0);
	/** Where its words come from: about the mouth, on the full-size body. */
	constexpr float CrewSpeakerHeight = 152.f;
	/** The hull the player bumps into and talks to: this wide (cm). */
	constexpr float CrewHullWidth = 55.f;
	/** The player this near (cm) and in front is looked at. */
	constexpr float CrewNoticeRadius = 450.f;
	/** A talk's turn comes on and goes off at this rate (shares a second). */
	constexpr float CrewTalkRate = 2.5f;
	/** Braced: knees bent this far (cm) and the body leaned in (degrees). */
	constexpr float CrewBraceCrouch = 11.f;
	constexpr float CrewBraceLean = 6.f;
	/** Watching the player, the chest turns a share of the way to them, at most this far (degrees). */
	constexpr float CrewWatchChestShare = 0.3f;
	constexpr float CrewWatchChestMax = 20.f;
	/** How often the player's distance is looked at for the body's detail (s). */
	constexpr float CrewDetailEvery = 0.25f;

	/** The stand-in wrench (cm): a steel handle in the fist and a jaw at its top end. */
	const FVector WrenchHandleScale(0.025, 0.025, 0.26);
	const FVector WrenchHandleAt(0.0, 0.0, 5.0);
	const FVector WrenchJawScale(0.055, 0.02, 0.05);
	const FVector WrenchJawAt(0.0, 0.0, 19.0);
	const FLinearColor WrenchSteel(0.32f, 0.33f, 0.36f);

	const TCHAR* const CrewSurfacePath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");

	UStaticMeshComponent* MakeWrenchPart(AActor& Owner, USceneComponent* Hand, const TCHAR* Name, UStaticMesh* Mesh, const FVector& At,
		const FVector& Scale)
	{
		UStaticMeshComponent* Part = Owner.CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Hand, FName(WardenCrewPose::GunSocket));
		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(At);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->bReceivesDecals = false;
		return Part;
	}

	int32 SeedOf(const AActor& Actor)
	{
		return static_cast<int32>(GetTypeHash(Actor.GetFName()) ^ 0x5EED);
	}
}

AWardenCrewMember::AWardenCrewMember()
{
	// The story character's own posing (a breath of the hull, a turn of it) isn't theirs: the body's pose is.
	TurnSpeed = 0.f;
	BreathHeight = 0.f;

	Mannequin = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mannequin"));
	Mannequin->SetupAttachment(Root);
	Figure = WardenCrewBody::MakeBody(*this, *Mannequin);

	// The hull: unseen, solid to the player and found by the Interact key's line; shots and the camera pass through (no
	// friendly fire on deck, no camera popping in a fight beside them).
	Body->SetVisibility(false);
	Body->SetCastShadow(false);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Body->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(CrewSurfacePath);
	Wrench = MakeWrenchPart(*this, Mannequin, TEXT("Wrench"), Cylinder.Object, WrenchHandleAt, WrenchHandleScale);
	WrenchJaw = MakeWrenchPart(*this, Mannequin, TEXT("WrenchJaw"), Cube.Object, WrenchJawAt, WrenchJawScale);
	if (Surface.Object)
	{
		Wrench->SetMaterial(0, Surface.Object);
		WrenchJaw->SetMaterial(0, Surface.Object);
	}

	// They talk when the story gives them words (STORY's topics on their speaker point): no empty prompt on deck otherwise.
	SpeakerPoint->bMissionsOnly = true;
	SpeakerPoint->Reach = 350.f;
	ApplyCrew();
}

void AWardenCrewMember::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCrew();
}

void AWardenCrewMember::SetCrew(EWardenCrew InCrew)
{
	Crew = InCrew;
	ApplyCrew();
	if (HasActorBegunPlay() && Figure && !bOwnModel)
	{
		// In play: the new person's stand-in, put on now, then dressed.
		const FWardenLook Look = LookFor(Crew);
		Mannequin->SetRelativeScale3D(FVector(Look.StandInScale));
		Figure->SetShownHero(UHeroDefinition::Load(Look.StandInHero));
		WardenCrewBody::DressStandIn(Figure, Look);
		ApplyCrew();
	}
}

void AWardenCrewMember::SetAtStation(bool bInAtStation)
{
	bAtStation = bInAtStation;
}

void AWardenCrewMember::Brace(float Seconds)
{
	Life.Brace(Seconds);
}

void AWardenCrewMember::BraceAll(const UObject* Context, float Seconds)
{
	UWorld* World = Context ? Context->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	for (TActorIterator<AWardenCrewMember> It(World); It; ++It)
	{
		It->Brace(Seconds);
	}
}

float AWardenCrewMember::BodyScale() const
{
	return Mannequin ? static_cast<float>(Mannequin->GetRelativeScale3D().Z) : 1.f;
}

void AWardenCrewMember::ApplyCrew()
{
	SpeakerPoint->SpeakerName = NameFor(Crew);
	for (const EWardenCrew Each : { EWardenCrew::Captain, EWardenCrew::Keeper, EWardenCrew::Engineer })
	{
		Tags.Remove(SpeakerTagFor(Each));
	}
	Tags.AddUnique(SpeakerTagFor(Crew));
	if (!HasActorBegunPlay())
	{
		Mannequin->SetRelativeScale3D(FVector(LookFor(Crew).StandInScale));
	}
	// The hull and the voice at their height.
	const float Scale = BodyScale();
	const float Height = CrewFullHeight * Scale;
	Body->SetRelativeLocation(FVector(0.0, 0.0, Height * 0.5f));
	Body->SetRelativeScale3D(FVector(CrewHullWidth / 100.f, CrewHullWidth / 100.f, Height / 100.f));
	SpeakerPoint->SetRelativeLocation(FVector(0.0, 0.0, CrewSpeakerHeight * Scale));
	const bool bWrench = Crew == EWardenCrew::Engineer;
	Wrench->SetVisibility(bWrench);
	WrenchJaw->SetVisibility(bWrench);
}

void AWardenCrewMember::BeginPlay()
{
	// Their own model if it's in, else the stand-in, sized; before the components begin, so the figure wears it at once.
	const FWardenLook Look = LookFor(Crew);
	bOwnModel = WardenCrewBody::WearLook(*Mannequin, Figure, Look);
	ApplyCrew();
	Life = FWardenLife(SeedOf(*this));
	PoseInput.IdleTime = FRandomStream(SeedOf(*this)).FRandRange(0.f, 10.f);
	Super::BeginPlay();
	WardenCrewBody::DressStandIn(Figure, Look);
	SpeakerPoint->OnTalked.AddUObject(this, &AWardenCrewMember::HandleTalkedTo);
	// Their pose is set as the actor ticks, before the body animates the same frame.
	Mannequin->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
	for (UStaticMeshComponent* Part : { Wrench.Get(), WrenchJaw.Get() })
	{
		if (UMaterialInstanceDynamic* Steel = Part ? Part->CreateDynamicMaterialInstance(0) : nullptr)
		{
			Steel->SetVectorParameterValue(TEXT("Color"), WrenchSteel);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %s, %s."), *GetActorNameOrLabel(), *NameFor(Crew).ToString(),
		bOwnModel ? TEXT("in their own model") : *FString::Printf(TEXT("%s's body standing in"), *Look.StandInHero.ToString()));
}

void AWardenCrewMember::RefreshShown()
{
	Super::RefreshShown();
	// Out of the story the body needn't move at all; back in, the next pose looks at how far the player is again.
	if (!IsShown() && Mannequin)
	{
		WardenCrewBody::ApplyDetail(*Mannequin, Figure, EWardenDetail::Held, Detail);
	}
	DetailClock = 0.f;
}

void AWardenCrewMember::HandleTalkedTo(USpeakerPointComponent& Point, AActor* InListener)
{
	Listener = InListener;
}

void AWardenCrewMember::UpdatePose(float DeltaSeconds)
{
	if (!Mannequin)
	{
		return;
	}
	DetailClock -= DeltaSeconds;
	if (DetailClock <= 0.f)
	{
		DetailClock = CrewDetailEvery;
		WardenCrewBody::ApplyDetail(*Mannequin, Figure, WardenCrewBody::DetailFor(WardenCrewBody::PlayerDistance(*this, GetActorLocation())), Detail);
	}
	BuildPose(DeltaSeconds);
	if (UWardenCrewAnimInstance* Anim = Cast<UWardenCrewAnimInstance>(Mannequin->GetAnimInstance()))
	{
		Anim->SetPoseInput(PoseInput);
	}
}

void AWardenCrewMember::BuildPose(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	const float Scale = BodyScale();
	const FTransform& Frame = GetActorTransform();
	const FWardenStation& Station = StationFor(Crew);
	WorkClock += Dt;
	PoseInput.IdleTime += Dt;

	// A talk: they turn to whoever talked to them while their words last.
	const AActor* Talker = Listener.Get();
	const bool bTalking = Talker && SpeakerPoint->IsTalking();
	TalkAlpha = FMath::FInterpConstantTo(TalkAlpha, bTalking ? 1.f : 0.f, Dt, CrewTalkRate);
	const float Talk = FMath::SmoothStep(0.f, 1.f, TalkAlpha);

	// The eyes: the one they talk to, the player near, glances, their work.
	Life.UpdateBrace(Dt);
	FWardenLookSense Sense;
	Sense.Eye = Frame.TransformPosition(CrewEyes * Scale);
	Sense.Forward = GetActorForwardVector();
	Sense.bHasPlayer = WardenCrewBody::PlayerEyes(*this, Sense.Player);
	Sense.NoticeRadius = CrewNoticeRadius;
	if (bTalking)
	{
		Sense.bHasListener = true;
		Sense.Listener = WardenCrewBody::EyesOf(*Talker);
	}
	if (bAtStation)
	{
		Sense.bHasFocus = true;
		Sense.Focus = Frame.TransformPosition(Station.Focus * Scale);
		Sense.FocusWeight = Station.FocusWeight;
	}
	Life.UpdateLook(Dt, Sense);
	PoseInput.LookAt = Life.GetLookAt();
	PoseInput.LookWeight = Life.GetLookWeight();

	// The hands at the station, working it (in bursts for some work); let go of in a talk, but a captain's wheel hand.
	float Wave = FMath::Sin(2.f * UE_PI * WorkClock / FMath::Max(Station.WorkSeconds, 0.1f));
	if (Station.BurstSeconds > 0.f)
	{
		Wave *= FMath::Clamp(FMath::Sin(UE_PI * WorkClock / Station.BurstSeconds) * 2.5f, 0.f, 1.f);
	}
	for (int32 Side = 0; Side < 2; ++Side)
	{
		FWardenHand& Hand = PoseInput.Hands[Side];
		const bool bKept = Side == 1 && Station.bKeepsRightHand;
		Hand.Weight = bAtStation ? (bKept ? 1.f : 1.f - Talk) : 0.f;
		Hand.Wrist = Frame.TransformPosition((Station.Hands[Side] + Station.Work[Side] * Wave * (1.f - Talk)) * Scale + HandsShift);
		Hand.Elbow = Frame.TransformPosition(Station.Elbows[Side] * Scale + HandsShift);
	}

	// The body: turned to the one they talk to on planted feet (as far as their station lets them), the chest a little to the
	// player watched, leaned into the work, braced and swaying when the ship lurches.
	const float TalkYaw = bTalking ? FMath::Clamp(WardenCrewBody::YawTo(*this, Talker->GetActorLocation()), -Station.MaxTalkTurn, Station.MaxTalkTurn) : 0.f;
	PoseInput.BodyYaw = FMath::FInterpTo(PoseInput.BodyYaw, TalkYaw * Talk, Dt, 5.f);
	const float WatchYaw = Life.IsWatchingPlayer() && !bTalking
		? FMath::Clamp(WardenCrewBody::YawTo(*this, Sense.Player) * CrewWatchChestShare, -CrewWatchChestMax, CrewWatchChestMax) : 0.f;
	ChestTurn = FMath::FInterpTo(ChestTurn, WatchYaw, Dt, 3.f);
	PoseInput.ChestYaw = ChestTurn;
	const float Braced = Life.GetBrace();
	const float Working = bAtStation ? 1.f - Talk : 0.f;
	PoseInput.Lean = Station.Lean * Working + CrewBraceLean * Braced + Life.GetSwayPitch();
	PoseInput.Crouch = Station.Crouch * Working + CrewBraceCrouch * Braced;
	PoseInput.Roll = Life.GetSwayRoll();
	PoseInput.ChestBack = 0.f;
	PoseInput.Gun = FWardenGunHold();
}
