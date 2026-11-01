// ADredgerCreature: its body (the mannequin in black rubber; the stand-in kit on its bones: DredgerCreatureKit.cpp), what it
// holds and how, and its body's input for the animation each frame (its hits, barks and death: DredgerCreatureReact.cpp).

#include "Creatures/DredgerCreature.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/DredgerBodyAnimInstance.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerRifleman.h"
#include "Creatures/DredgerTuning.h"
#include "Player/Animation/LooterStancePoseCarry.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

const FName ADredgerCreature::HoldSocket(TEXT("HandGrip_R"));

namespace
{
	/** The capsule (cm, full size): a 1.8 m man. */
	constexpr float DredgerRadius = 36.f;
	constexpr float DredgerHalfHeight = 92.f;
	/** Its chest over its capsule's middle (cm, full size): where the aim turns from. */
	constexpr float ChestRise = 40.f;
	/** How far its aim may turn from its facing (degrees) before its body has turned after it. */
	constexpr float AimYawSlack = 30.f;
	/** Its aim's pitch at most, and its look at ease (degrees). */
	constexpr float AimPitchMost = 60.f;
	constexpr float CalmPitch = -8.f;
	/** How fast the body eases (shares a second): into a crouch, a lean, the brace, the reload; the gun up and down. */
	constexpr float CrouchRate = 5.f;
	constexpr float LeanRate = 4.f;
	constexpr float BraceRate = 6.f;
	constexpr float ReloadInRate = 7.f;
	constexpr float ReloadOutRate = 5.f;
	constexpr float ShoulderRate = 7.f;
	constexpr float LowerRate = 1.5f;
	/** A kick's ease (per second) and a rifle round's kick (cm back, degrees up). */
	constexpr float KickEase = 14.f;
	constexpr float KickBackRound = 3.2f;
	constexpr float KickPitchRound = 5.f;
	/** The crouch behind cover: the head settles this high over the feet (cm, full size), the torso leans this far. */
	constexpr float CrouchHeadHeight = 112.f;
	/** Holding its ground, its attack range (cm): its brain then never walks it toward its target (SetHoldsGround). */
	constexpr float HoldGroundRange = 12000.f;

	/** Hit zones seen only by weapon and visibility traces (as the placeholder's), so pawns walk through and shots stop. */
	void MakeDredgerHitZone(UPrimitiveComponent* Part)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Part->SetCollisionObjectType(ECC_WorldDynamic);
		Part->SetCollisionResponseToAllChannels(ECR_Ignore);
		Part->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
		Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
	}
}

ADredgerCreature::ADredgerCreature()
{
	DisplayName = NSLOCTEXT("Dredger", "DefaultName", "Dredger");
	ElementBody = EElementBody::Flesh;
	// Company men, not the restless dead: never Touched by an element (their own soul-smog is their look).
	bMayBeTouched = false;
	// Every Dredger answers every other's call: a shot one shouts for his crew.
	PackTag = TEXT("Dredger");
	PackAlertRadius = 3000.f;
	// The mask's lenses are on the head: a head shot is a crit.
	CriticalSpotBones = { FName(TEXT("head")) };
	HealthBarHeight = 112.f;
	CorpseTime = 7.f;
	GetCapsuleComponent()->InitCapsuleSize(DredgerRadius, DredgerHalfHeight);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	// Never off a ship's deck, a ledge or a porch's edge: they fight on the Lamplighter grappled over a canyon.
	Movement->bCanWalkOffLedges = false;
	Movement->RotationRate = FRotator(0.f, 420.f, 0.f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(DredgerLook::BodyMeshPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(DredgerLook::SpherePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(DredgerLook::CylinderPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(DredgerLook::CubePath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(DredgerLook::SurfacePath);
	SphereMesh = Sphere.Object;
	CylinderMesh = Cylinder.Object;
	CubeMesh = Cube.Object;
	SurfaceMaster = Surface.Object;

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMeshAsset(BodyMesh.Object);
	// The mannequin faces +Y with its feet at its origin.
	Body->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -DredgerHalfHeight), FRotator(0.f, -90.f, 0.f));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UDredgerBodyAnimInstance::StaticClass());
	// Its hit zones (the mannequin's physics asset) must follow the pose wherever a shot comes from, seen or not.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->PrimaryComponentTick.TickGroup = TG_PostPhysics;
	MakeDredgerHitZone(Body);

	// What it holds hangs from the right hand's grip socket, turned each frame to its aim (PlaceHeld).
	Held = CreateDefaultSubobject<USceneComponent>(TEXT("Held"));
	Held->SetupAttachment(Body, HoldSocket);

	// The stand-in gas mask and company cap (placed on the head bone as play begins: PlaceKit).
	MaskFace = MakePart(TEXT("MaskFace"), TEXT("head"));
	MaskSnout = MakePart(TEXT("MaskSnout"), TEXT("head"));
	MaskFilter = MakePart(TEXT("MaskFilter"), TEXT("head"));
	LensLeft = MakePart(TEXT("LensLeft"), TEXT("head"));
	LensRight = MakePart(TEXT("LensRight"), TEXT("head"));
	CapCrown = MakePart(TEXT("CapCrown"), TEXT("head"));
	CapBrim = MakePart(TEXT("CapBrim"), TEXT("head"));
	MaskFace->SetStaticMesh(SphereMesh);
	MaskSnout->SetStaticMesh(CylinderMesh);
	MaskFilter->SetStaticMesh(CylinderMesh);
	LensLeft->SetStaticMesh(CylinderMesh);
	LensRight->SetStaticMesh(CylinderMesh);
	CapCrown->SetStaticMesh(CylinderMesh);
	CapBrim->SetStaticMesh(CylinderMesh);
}

TSubclassOf<ADredgerCreature> ADredgerCreature::ClassFor(EDredgerRole InRole)
{
	switch (InRole)
	{
	case EDredgerRole::HoseMan:
		return ADredgerHoseMan::StaticClass();
	case EDredgerRole::Rifleman:
	default:
		return ADredgerRifleman::StaticClass();
	}
}

UStaticMeshComponent* ADredgerCreature::MakePart(const TCHAR* Name, FName Bone)
{
	// Constructor only (a default subobject). Looks only: no collision, and the body's shadow is enough for the small ones.
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(GetMesh(), Bone);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	Part->bReceivesDecals = false;
	return Part;
}

void ADredgerCreature::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyBodyLook();
}

void ADredgerCreature::BeginPlay()
{
	// A level that never initialised its actors (a test level) skipped PostInitializeComponents: the look first.
	if (!RubberLook)
	{
		ApplyBodyLook();
	}
	Super::BeginPlay();
	MeshRest = GetMesh()->GetRelativeTransform();
	PlaceKit();
	PlaceJobKit();
	// Each breathes on its own clock, so a squad doesn't move in step.
	BreathTime = FMath::FRandRange(0.f, 10.f);
	if (!GetMesh()->DoesSocketExist(HoldSocket))
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: its body has no %s socket: what it holds hangs from the hand as it is."), *GetName(),
			*HoldSocket.ToString());
	}
}

// ---------------------------------------------------------------------------
// Its place, its hold, its spots (its look and kit: DredgerCreatureKit.cpp)
// ---------------------------------------------------------------------------

bool ADredgerCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	// The head's hit zone (the mask and its lenses); each job adds its own (the hose-man's tank).
	return Super::IsCriticalSpot(Hit);
}

ACreatureBase::FFootprint ADredgerCreature::GetFootprint() const
{
	FFootprint Footprint;
	Footprint.Radius = 38.f;
	return Footprint;
}

void ADredgerCreature::SetHoldsGround(bool bHold)
{
	if (bHold == bHoldsGround)
	{
		return;
	}
	bHoldsGround = bHold;
	UpdateHoldRange();
}

void ADredgerCreature::SetPost(float Radius)
{
	Super::SetPost(Radius);
	UpdateHoldRange();
}

void ADredgerCreature::UpdateHoldRange()
{
	// Its brain walks it in only from past three quarters of its attack range: holding its ground (or a rifleman at his post,
	// whose own tactics keep him in it), that is past any fight it can see (it only turns and fires); let go, its own range.
	const bool bWide = bHoldsGround || (PostRadius > 0.f && FightsFromPost());
	if (bWide == bRangeWidened)
	{
		return;
	}
	bRangeWidened = bWide;
	if (bWide)
	{
		RangeBeforeHold = AttackRange;
		AttackRange = FMath::Max(AttackRange, HoldGroundRange);
	}
	else if (RangeBeforeHold > 0.f)
	{
		AttackRange = RangeBeforeHold;
	}
}

void ADredgerCreature::SetHitVolumesEnabled(bool bEnabled)
{
	if (!bLimp)
	{
		GetMesh()->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

void ADredgerCreature::OnPoseThawed()
{
	// Seen again after a frozen spell: no flinch or kick left over from before it.
	FlinchSpring.Reset();
	HeadSpring.Reset();
	KickBack = 0.f;
	KickPitch = 0.f;
}

FVector ADredgerCreature::GetChestPoint() const
{
	return GetActorLocation() + FVector(0.f, 0.f, ChestRise * GetSizeScale());
}

FVector ADredgerCreature::GetAimPoint() const
{
	if (const APawn* Victim = GetTarget())
	{
		return Victim->GetActorLocation() + FVector(0.f, 0.f, 25.f);
	}
	return GetChestPoint() + GetActorForwardVector() * 1000.f;
}

FVector ADredgerCreature::GetMuzzle() const
{
	return Held ? Held->GetComponentTransform().TransformPosition(GetHeldMuzzle()) : GetChestPoint() + GetActorForwardVector() * 60.f;
}

void ADredgerCreature::KickRecoil(float Strength)
{
	KickBack = FMath::Max(KickBack, KickBackRound * Strength);
	KickPitch = FMath::Max(KickPitch, KickPitchRound * Strength);
}

void ADredgerCreature::FlareLenses(float Seconds, float Strength)
{
	FlareLength = FMath::Max(Seconds, 0.05f);
	FlareLeft = FlareLength;
	FlareStrength = Strength;
}

// ---------------------------------------------------------------------------
// Each frame
// ---------------------------------------------------------------------------

void ADredgerCreature::Tick(float DeltaSeconds)
{
	if (!IsDead())
	{
		TickBeforeBrain(DeltaSeconds);
	}
	Super::Tick(DeltaSeconds);
	if (!IsDead())
	{
		TickJob(DeltaSeconds);
	}
	TickBody(DeltaSeconds);
}

void ADredgerCreature::TickBody(float DeltaSeconds)
{
	DeadTime = IsDead() ? DeadTime + DeltaSeconds : 0.f;
	if (IsDead())
	{
		TickCorpse(DeltaSeconds);
		return;
	}
	BreathTime += DeltaSeconds;
	FlinchSpring.Step(DeltaSeconds);
	HeadSpring.Step(DeltaSeconds);
	CrouchNow = FMath::FInterpConstantTo(CrouchNow, FMath::Clamp(Wants.Crouch, 0.f, 1.f), DeltaSeconds, CrouchRate);
	LeanNow = FMath::FInterpConstantTo(LeanNow, FMath::Clamp(Wants.Lean, -1.f, 1.f), DeltaSeconds, LeanRate);
	BraceNow = FMath::FInterpConstantTo(BraceNow, FMath::Clamp(Wants.Brace, 0.f, 1.f), DeltaSeconds, BraceRate);
	const float ReloadTarget = FMath::Clamp(Wants.ReloadWeight, 0.f, 1.f);
	ReloadWeightNow = FMath::FInterpConstantTo(ReloadWeightNow, ReloadTarget, DeltaSeconds, ReloadTarget > ReloadWeightNow ? ReloadInRate : ReloadOutRate);
	const float Ease = FMath::Exp(-KickEase * DeltaSeconds);
	KickBack *= Ease;
	KickPitch *= Ease;

	// Fighting, the gun comes to the shoulder at once; at ease it sinks slowly to the low-ready.
	const bool bFighting = Wants.bShouldered || IsNoticing() || (GetTarget() && IsFightState(GetCreatureState()));
	Shouldered = FMath::FInterpConstantTo(Shouldered, bFighting ? 1.f : 0.f, DeltaSeconds, bFighting ? ShoulderRate : LowerRate);

	// The lenses: a faint glow, flaring up to their tell's end, then back down.
	if (LensLook && (FlareLeft > 0.f || FlareLength > 0.f))
	{
		FlareLeft = FMath::Max(0.f, FlareLeft - DeltaSeconds);
		const float Up = FlareLength > 0.f ? 1.f - FlareLeft / FlareLength : 1.f;
		const float Glow = FlareLeft > 0.f ? FMath::Lerp(DredgerLook::LensGlow, DredgerLook::LensFlareGlow * FlareStrength, Up * Up)
			: DredgerLook::LensGlow;
		LensLook->SetScalarParameterValue(TEXT("Glow"), Glow);
		if (FlareLeft <= 0.f)
		{
			FlareLength = 0.f;
		}
	}

	// Far off out of view its body isn't posed (it moves on unposed until seen again: OnPoseThawed).
	if (IsPoseFrozen())
	{
		return;
	}
	const FVector Chest = GetChestPoint();
	const float Yaw = static_cast<float>(GetActorRotation().Yaw);
	FRotator Aim = (GetAimPoint() - Chest).Rotation();
	if (!bFighting)
	{
		Aim = FRotator(CalmPitch, Yaw, 0.f);
	}
	const float YawOff = static_cast<float>(FRotator::NormalizeAxis(Aim.Yaw - static_cast<double>(Yaw)));
	Aim.Yaw = Yaw + FMath::Clamp(YawOff, -AimYawSlack, AimYawSlack);
	Aim.Pitch = FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(Aim.Pitch)), -AimPitchMost, AimPitchMost);
	Aim.Roll = 0.f;
	const FQuat Aiming = Aim.Quaternion();
	// Reloading tips the gun down and rolls it toward the body, where the reload clip's hands work on it (the player's turn).
	const FQuat Worked = FQuat::Slerp(Aiming, Aiming * FRotator(-20.f, 15.f, 45.f).Quaternion(), ReloadWeightNow);
	const FQuat GunWorld = PlaceHeld(Worked, Yaw);
	FillStance(Aiming, GunWorld, static_cast<float>(Aim.Pitch));

	const FVector Velocity = GetVelocity();
	const FVector Local = GetActorQuat().UnrotateVector(Velocity);
	BodyInput.Speed = static_cast<float>(Velocity.Size2D()) / FMath::Max(GetSizeScale(), 0.1f);
	BodyInput.MoveAngle = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Local.Y), static_cast<float>(Local.X)));
	BodyInput.bCheap = GetUpdateInterval() > 0.f;
	BodyInput.Flinch = FlinchSpring.Offset;
	BodyInput.HeadWhip = FVector2D(HeadSpring.Offset.X, HeadSpring.Offset.Y);
	BodyInput.Stagger = GetStaggerAlpha();
	BodyInput.Wound = static_cast<float>(WoundStage(Health->GetHealthPercent()));
	BodyInput.Lean = LeanNow;
	BodyInput.Brace = BraceNow;
	BodyInput.Tug = Wants.Tug;
	BodyInput.Notice = GetNoticeStrength();
	BodyInput.BreathTime = BreathTime;
	BodyInput.ReloadShare = Wants.ReloadShare;
	BodyInput.Fall = 0.f;
}

FQuat ADredgerCreature::PlaceHeld(const FQuat& Aiming, float Yaw)
{
	// Shouldered along the aim or lowered to the low-ready (the player's own carry), then kicked up about the grip.
	FLooterCarry Carry;
	Carry.LowReady = 1.f - Shouldered;
	const FQuat Carried = LooterCarry::CarriedGun(Aiming, Yaw, Carry);
	const FQuat Kicked = Carried * FRotator(KickPitch, 0.f, 0.f).Quaternion();
	const USkeletalMeshComponent* Body = GetMesh();
	if (Held && Body && Body->DoesSocketExist(HoldSocket))
	{
		// The animation moves the hand; what it holds keeps its grip in it but points where it aims (as the player's gun).
		const FTransform Socket = Body->GetSocketTransform(HoldSocket);
		const FQuat Relative = Socket.GetRotation().Inverse() * Kicked;
		Held->SetRelativeLocationAndRotation(Relative.RotateVector(-GetHeldGrip()), Relative);
	}
	return Kicked;
}

void ADredgerCreature::FillStance(const FQuat& Aiming, const FQuat& GunWorld, float AimPitch)
{
	FLooterStanceInput& Stance = BodyInput.Stance;
	Stance = FLooterStanceInput();
	const USkeletalMeshComponent* Body = GetMesh();
	const FQuat ToMesh = Body->GetComponentQuat().Inverse();
	const FVector Forward = Body->GetComponentTransform().InverseTransformVectorNoScale(GetActorForwardVector()).GetSafeNormal2D();
	Stance.Forward = Forward.IsNearlyZero() ? FVector::YAxisVector : Forward;

	// Behind cover: the player's crouch (hips down, the legs solved back onto the planted feet, the torso over the gun).
	Stance.CrouchAlpha = CrouchNow;
	Stance.CrouchedHeadHeight = CrouchHeadHeight;
	Stance.CrouchTorsoLean = 14.f;
	Stance.CrouchHipsBack = 6.f;
	Stance.MaxHipDrop = 50.f;
	Stance.KneeSplay = 14.f;

	// The torso follows the aim's pitch (less at the low-ready), the left hand finds what it holds, each round kicks it.
	Stance.bAimWithTorso = true;
	Stance.AimPitch = AimPitch * FMath::Lerp(0.35f, 1.f, Shouldered);
	if (Held && Body->DoesSocketExist(HoldSocket))
	{
		Stance.bHandOnForegrip = true;
		Stance.HoldBone = Body->GetSocketBoneName(HoldSocket);
		Stance.HoldSocketLocal = Body->GetSocketTransform(HoldSocket, RTS_ParentBoneSpace);
		Stance.WeaponGrip = GetHeldGrip();
		Stance.WeaponForegrip = GetHeldForegrip();
		Stance.WeaponScale = GetHeldScale();
		Stance.WeaponKind = EWeaponKind::Rifle;
		Stance.WeaponRotation = ToMesh * GunWorld;
		Stance.Carry.LowReady = 1.f - Shouldered;
		Stance.AimRotation = ToMesh * Aiming;
		Stance.CarryRotation = ToMesh * LooterCarry::CarriedGun(Aiming, static_cast<float>(GetActorRotation().Yaw), Stance.Carry);
	}
	Stance.RecoilBack = KickBack;
	Stance.RecoilPitch = KickPitch;
	Stance.ReloadWeight = ReloadWeightNow;
}
