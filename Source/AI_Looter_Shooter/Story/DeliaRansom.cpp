// ADeliaRansom: Delia on her porch (Landfall's turn-in giver). Her body, her talk, her look and her pose here; her fight
// DeliaRansomFight.cpp; her shotgun DeliaRansomShotgun.cpp; the fight's rules DeliaRansomRules.

#include "Story/DeliaRansom.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Missions/MissionRunner.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Story/DeliaRansomRules.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLineSet.h"
#include "Story/WardenCrewAnimInstance.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"

const FName ADeliaRansom::SpeakerTag(TEXT("Speaker_Delia"));
const TCHAR* const ADeliaRansom::FightBarksPath = TEXT("/Game/Data/Story/DA_Lines_DeliaFight.DA_Lines_DeliaFight");

namespace
{
	/** Her own model once it's in (Tripo 1: in her seventies, small and tough, 1.6 m), and the stand-in until then. */
	FWardenLook DeliaLook()
	{
		FWardenLook Look;
		Look.Model = FSoftObjectPath(TEXT("/Game/Art/Characters/SK_Delia.SK_Delia"));
		// Odessa's body (1.72 m) at 0.79: 1.36 m in the player's 0.85 world. Her crimson coat to a near-black shawl, her
		// hair to grey.
		Look.StandInHero = TEXT("Odessa");
		Look.StandInScale = 0.79f;
		Look.Cloth = FLinearColor(0.3f, 0.28f, 0.3f);
		Look.Hair = FLinearColor(1.8f, 1.8f, 1.8f);
		return Look;
	}

	/** Her capsule (cm): her height (1.6 m in the 0.85 world) and a small woman's girth. */
	constexpr float DeliaHalfHeight = 68.f;
	constexpr float DeliaRadius = 26.f;
	/** The full-size body's eyes and mouth over its feet (cm), scaled with her. */
	const FVector DeliaEyes(8.0, 0.0, 165.0);
	constexpr float DeliaSpeakerHeight = 152.f;
	/** The player this near and in front is watched. */
	constexpr float DeliaNoticeRadius = 600.f;
	/** Turning (degrees a second): onto a target, to someone talking, back to her road. */
	constexpr float DeliaFightTurn = 220.f;
	constexpr float DeliaTalkTurn = 140.f;
	constexpr float DeliaCalmTurn = 70.f;
	/** The chest twists ahead of the feet onto the aim by at most this (degrees). */
	constexpr float DeliaChestLead = 35.f;
	/** How often the player's distance is looked at for the body's detail (s). */
	constexpr float DeliaDetailEvery = 0.25f;
}

FText ADeliaRansom::SpeakerNameText()
{
	return NSLOCTEXT("LooterDelia", "DeliaName", "Delia Ransom");
}

ADeliaRansom::ADeliaRansom()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	// A character only so the creatures she hurts can turn on her (the hunt takes a character with health); she has an AI
	// controller to be blamed for her shots, and never walks.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	bUseControllerRotationYaw = false;
	SetCanBeDamaged(true);
	Tags.Add(SpeakerTag);

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(DeliaRadius, DeliaHalfHeight);
	// Solid to the player and creatures, seen by the Interact key's line and creatures' eyes; shots and the camera pass
	// through her (no friendly fire, no camera pop as the player fights at her side).
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	Capsule->SetCanEverAffectNavigation(false);

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetRelativeLocation(FVector(0.0, 0.0, -DeliaHalfHeight));
	Figure = WardenCrewBody::MakeBody(*this, *Body);
	Body->SetRelativeScale3D(FVector(DeliaLook().StandInScale));

	MakeShotgun();

	SpeakerPoint = CreateDefaultSubobject<USpeakerPointComponent>(TEXT("SpeakerPoint"));
	SpeakerPoint->SetupAttachment(GetCapsuleComponent());
	SpeakerPoint->SpeakerName = SpeakerNameText();
	SpeakerPoint->SetRelativeLocation(FVector(0.0, 0.0, DeliaSpeakerHeight * DeliaLook().StandInScale - DeliaHalfHeight));

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 500.f;
	Health->bInvulnerable = true;
	Health->bShowDamageNumbers = false;

	FightBarks = TSoftObjectPtr<UStoryLineSet>(FSoftObjectPath(FightBarksPath));
}

float ADeliaRansom::BodyScale() const
{
	return GetMesh() ? static_cast<float>(GetMesh()->GetRelativeScale3D().Z) : 1.f;
}

void ADeliaRansom::BeginPlay()
{
	USkeletalMeshComponent* Body = GetMesh();
	const FWardenLook Look = DeliaLook();
	WardenCrewBody::WearLook(*Body, Figure, Look);
	SpeakerPoint->SetRelativeLocation(FVector(0.0, 0.0, DeliaSpeakerHeight * BodyScale() - DeliaHalfHeight));
	Life = FWardenLife(static_cast<int32>(GetTypeHash(GetFName())));
	Random.Initialize(static_cast<int32>(GetTypeHash(GetFName()) ^ 0xDE11A));
	PoseInput.IdleTime = Random.FRandRange(0.f, 10.f);
	WearShotgun();
	Super::BeginPlay();
	WardenCrewBody::DressStandIn(Figure, Look);

	Porch = GetActorLocation();
	PorchYaw = static_cast<float>(GetActorRotation().Yaw);
	SocketInHand = Body->DoesSocketExist(FName(WardenCrewPose::GunSocket))
		? Body->GetSocketTransform(FName(WardenCrewPose::GunSocket), RTS_ParentBoneSpace) : FTransform::Identity;
	// She stands where she's put: no walking, no falling off her porch.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_None);
		Movement->SetComponentTickEnabled(false);
	}
	// Her pose is set as she ticks, before the body animates the same frame.
	Body->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);

	Barks = !FightBarks.IsNull() && FPackageName::DoesPackageExist(FightBarks.ToSoftObjectPath().GetLongPackageName())
		? FightBarks.LoadSynchronous() : nullptr;
	UE_CLOG(!Barks, LogLooter, Warning, TEXT("%s: no barks at %s: Delia fights in silence."), *GetActorNameOrLabel(), *FightBarks.ToString());
	SpeakerPoint->OnTalked.AddUObject(this, &ADeliaRansom::HandleTalked);
	Health->OnDamaged.AddDynamic(this, &ADeliaRansom::HandleDamaged);
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		LandingHandle = Bullets->OnBulletLanding.AddUObject(this, &ADeliaRansom::HandleBulletLanding);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ADeliaRansom::HandleMissionsChanged);
	}
	RefreshShown();
}

void ADeliaRansom::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		Bullets->OnBulletLanding.Remove(LandingHandle);
	}
	if (SpeakerPoint)
	{
		SpeakerPoint->OnTalked.RemoveAll(this);
	}
	if (Health)
	{
		Health->OnDamaged.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// In the story or not
// ---------------------------------------------------------------------------

void ADeliaRansom::HandleMissionsChanged()
{
	RefreshShown();
}

void ADeliaRansom::RefreshShown()
{
	bool bWanted = true;
	if (!ShownWhen.IsEmpty())
	{
		const UMissionRunner* Runner = UMissionRunner::Get(this);
		bWanted = Runner && ShownWhen.IsMet(Runner->GetCampaign(), Runner);
	}
	SetShown(bWanted);
}

void ADeliaRansom::SetShown(bool bInShown)
{
	if (bShown == bInShown && bShownApplied)
	{
		return;
	}
	bShown = bInShown;
	bShownApplied = true;
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
	SetActorTickEnabled(bShown);
	if (!bShown && GetMesh())
	{
		WardenCrewBody::ApplyDetail(*GetMesh(), Figure, EWardenDetail::Held, Detail);
	}
	DetailClock = 0.f;
	UE_LOG(LogLooter, Log, TEXT("%s: Delia %s (%s)."), *GetActorNameOrLabel(), bShown ? TEXT("on her porch") : TEXT("not about"), *ShownWhen.Describe());
}

// ---------------------------------------------------------------------------
// Talking: her speaker point's, never mid-fight
// ---------------------------------------------------------------------------

FInteractionOptions ADeliaRansom::GetInteractionOptions(const UInteractionComponent& User) const
{
	// With the gun up and something to shoot at she's busy: talk once the porch is clear.
	if (!bShown || (Fight == EDeliaFight::Ready && Target.IsValid()))
	{
		return FInteractionOptions::None();
	}
	return SpeakerPoint ? SpeakerPoint->GetInteractionOptions(User) : FInteractionOptions::None();
}

bool ADeliaRansom::Interact(UInteractionComponent& User, bool bHeld)
{
	return bShown && SpeakerPoint && SpeakerPoint->Interact(User, bHeld);
}

TOptional<FVector> ADeliaRansom::GetInteractionLocation() const
{
	return SpeakerPoint ? SpeakerPoint->GetInteractionLocation() : TOptional<FVector>();
}

void ADeliaRansom::HandleTalked(USpeakerPointComponent& Point, AActor* InListener)
{
	Listener = InListener;
}

void ADeliaRansom::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{
	// A blow rocks her back on her heels: the flinch, not a flight off the porch.
	Flinch = 1.f;
	FlinchSide = (LaunchVelocity | GetActorRightVector()) >= 0.0 ? 1.f : -1.f;
}

void ADeliaRansom::SetFighting(bool bFight)
{
	FightOverride = bFight;
}

void ADeliaRansom::ClearFightingOverride()
{
	FightOverride.Reset();
}

// ---------------------------------------------------------------------------
// Each frame
// ---------------------------------------------------------------------------

void ADeliaRansom::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Step(DeltaSeconds);
}

void ADeliaRansom::Step(float DeltaSeconds)
{
	if (!bShown)
	{
		return;
	}
	USkeletalMeshComponent* Body = GetMesh();
	DetailClock -= DeltaSeconds;
	if (DetailClock <= 0.f && Body)
	{
		DetailClock = DeliaDetailEvery;
		WardenCrewBody::ApplyDetail(*Body, Figure, WardenCrewBody::DetailFor(WardenCrewBody::PlayerDistance(*this, GetActorLocation())), Detail);
	}
	StepFight(DeltaSeconds);
	BuildPose(DeltaSeconds);
	if (UWardenCrewAnimInstance* Anim = Body ? Cast<UWardenCrewAnimInstance>(Body->GetAnimInstance()) : nullptr)
	{
		Anim->SetPoseInput(PoseInput);
	}
}

void ADeliaRansom::BuildPose(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	const float Scale = BodyScale();
	PoseInput.IdleTime += Dt;
	Life.UpdateBrace(Dt);

	// Whom she faces: her target in a fight; whoever talks to her while her words last; else her road, as she was placed.
	const ACreatureBase* Aimed = Fight == EDeliaFight::Ready ? Target.Get() : nullptr;
	const AActor* Talker = Listener.Get();
	const bool bTalking = Talker && SpeakerPoint->IsTalking() && !Aimed;
	float WantYaw = PorchYaw;
	float TurnRate = DeliaCalmTurn;
	FVector AimAt = GetActorLocation() + GetActorForwardVector() * 1000.0;
	if (Aimed)
	{
		AimAt = Aimed->GetActorLocation();
		WantYaw = static_cast<float>((AimAt - GetActorLocation()).Rotation().Yaw);
		TurnRate = DeliaFightTurn;
	}
	else if (bTalking)
	{
		WantYaw = static_cast<float>((Talker->GetActorLocation() - GetActorLocation()).Rotation().Yaw);
		TurnRate = DeliaTalkTurn;
	}
	const float Yaw = static_cast<float>(GetActorRotation().Yaw);
	const float Left = FRotator::NormalizeAxis(WantYaw - Yaw);
	SetActorRotation(FRotator(0.f, Yaw + FMath::Clamp(Left, -TurnRate * Dt, TurnRate * Dt), 0.f));
	// The chest leads the feet onto the aim, so the gun tracks quicker than she turns.
	const float Lead = Aimed ? FMath::Clamp(FRotator::NormalizeAxis(WantYaw - static_cast<float>(GetActorRotation().Yaw)), -DeliaChestLead, DeliaChestLead) : 0.f;
	PoseInput.ChestYaw = FMath::FInterpTo(PoseInput.ChestYaw, Lead * Shoulder, Dt, 10.f);
	PoseInput.BodyYaw = 0.f;

	// The eyes: down her gun at the target; the one she's talking to; the player coming up; the road and the orchard.
	FWardenLookSense Sense;
	Sense.Eye = GetMesh() ? GetMesh()->GetComponentLocation() + GetActorRotation().RotateVector(DeliaEyes * Scale) : GetActorLocation();
	Sense.Forward = GetActorForwardVector();
	Sense.bHasPlayer = WardenCrewBody::PlayerEyes(*this, Sense.Player);
	Sense.NoticeRadius = DeliaNoticeRadius;
	if (Aimed)
	{
		Sense.bHasFocus = true;
		Sense.bFocusOnly = true;
		Sense.Focus = AimAt;
	}
	else if (bTalking)
	{
		Sense.bHasListener = true;
		Sense.Listener = WardenCrewBody::EyesOf(*Talker);
	}
	Life.UpdateLook(Dt, Sense);
	PoseInput.LookAt = Life.GetLookAt();
	PoseInput.LookWeight = Life.GetLookWeight();

	// The gun: carried low, up at her shoulder on the target, broken open for shells, kicked by a shot.
	float Break = 0.f;
	float ToPocket = 0.f;
	if (Fight == EDeliaFight::Reloading)
	{
		DeliaFightRules::ReloadPose(ReloadClock / DeliaFightRules::ReloadSeconds, Break, ToPocket);
	}
	FWardenGunHold& Gun = PoseInput.Gun;
	Gun.Weight = 1.f;
	Gun.AimAt = AimAt;
	Gun.Shoulder = Shoulder;
	Gun.Break = Break;
	Gun.LeftToPocket = ToPocket;
	Gun.Kick = Kick;
	Gun.SocketInHand = SocketInHand;
	Gun.Forend = Forend;

	// Her stance: a little stooped at rest, set into the gun when it's up; a blow or a kick rocks her chest back.
	PoseInput.Lean = 3.f + 4.f * Shoulder;
	PoseInput.Crouch = 3.f * Shoulder;
	PoseInput.ChestBack = 5.f * Kick + 7.f * Flinch;
	PoseInput.Roll = 4.f * Flinch * FlinchSide;
}
