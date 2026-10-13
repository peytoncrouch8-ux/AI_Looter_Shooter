// ABellRingerCreature: construction (its numbers, its look, the bell and the rope on its fists, the bell's hit zone), its
// frame, hits, death and return. BellRingerCreatureToll.cpp has the toll (its tell, what it raises or steadies, the bell
// shot), BellRingerCreatureMoves.cpp its hanging back and the swipe, BellRingerCreaturePose.cpp the bell's arm and the
// props swinging from its fists.

#include "Creatures/BellRingerCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Creatures/BellRingerGraves.h"
#include "Creatures/BellRingerRing.h"
#include "Player/PlayerSize.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "ReferenceSkeleton.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const ABellRingerCreature::BellPath = TEXT("/Game/Art/Creatures/SM_UnpaidBell.SM_UnpaidBell");
const TCHAR* const ABellRingerCreature::RopePath = TEXT("/Game/Art/Creatures/SM_UnpaidBellRope.SM_UnpaidBellRope");
const FName ABellRingerCreature::BarkKind(TEXT("BellRinger"));

namespace
{
	/** The bell's body hangs this far under the fist (cm at size 1): the middle of its hit zone (UnpaidBell.py's BODY_DROP). */
	constexpr float BellBodyDrop = 15.f;

	/** The creature brain's chase stops at this share of its attack range: the range is set so it stops at HoldDistance. */
	constexpr float ChaseStopShare = 0.75f;

	/** Where its fists hold the props without the model (its own space, the ground under it): about its hands at rest. */
	const FVector RightGripWithoutRig(17.f, 23.f, 79.f);
	const FVector LeftGripWithoutRig(19.f, -22.f, 81.f);

	/** A hit zone has this much fade left before a shot passes through it (as the Unpaid's body's: GoneEnoughToMiss). */
	constexpr float GoneEnoughToMiss = 0.6f;

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

	/**
	 * A prop hung plumb from a fist as the model rests (arms hanging): where it sits on the hand's bone, so the bestiary's
	 * stand (which reads the class's defaults) shows it in the hand. Fallback (the model's own space) without the model.
	 */
	FTransform HungAtRest(const USkeletalMesh* Model, const FUnpaidArmBones& Arm, const FVector& Fallback)
	{
		const FReferenceSkeleton* Skeleton = Model ? &Model->GetRefSkeleton() : nullptr;
		const int32 Hand = Skeleton && !Arm.Hand.IsNone() ? Skeleton->FindBoneIndex(Arm.Hand) : INDEX_NONE;
		if (Hand == INDEX_NONE)
		{
			return FTransform(Fallback);
		}
		const FTransform HandRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(*Skeleton, Hand);
		const int32 Knuckle = Arm.Fingers.IsValidIndex(2) ? Skeleton->FindBoneIndex(Arm.Fingers[2]) : INDEX_NONE;
		const FVector KnuckleAt = Knuckle != INDEX_NONE
			? FAnimationRuntime::GetComponentSpaceTransformRefPose(*Skeleton, Knuckle).GetLocation() : HandRest.GetLocation();
		const FVector Grip = BellRingerRules::GripPoint(HandRest.GetLocation(), KnuckleAt);
		return FTransform(FQuat::Identity, Grip).GetRelativeTransform(HandRest);
	}

	/** A prop a fist holds: no collision of its own (the bell's hit zone is a ball of its own), and a ghost casts no shadow. */
	UStaticMeshComponent* MakeProp(AActor& Owner, const TCHAR* Name, USceneComponent* Body, FName Bone, const FTransform& OnBone,
		UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Prop = Owner.CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Prop->SetupAttachment(Body, Bone);
		Prop->SetRelativeTransform(OnBone);
		Prop->SetStaticMesh(Mesh);
		Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Prop->SetGenerateOverlapEvents(false);
		Prop->SetCanEverAffectNavigation(false);
		Prop->SetCastShadow(false);
		return Prop;
	}
}

ABellRingerCreature::ABellRingerCreature()
{
	DisplayName = NSLOCTEXT("LooterBellRinger", "BellRingerName", "Bell-Ringer");
	// Fragile next to the others (an Unpaid has 160) and weak in a fight: what it does to a fight is its bell.
	Health->MaxHealth = 110.f;
	AttackDamage = 6.f;
	// Worth more than one of the Unpaid: killing it first should pay.
	XPReward = 15;
	// Slower than the pack it follows, so it trails behind them, and slower than a player backing off.
	WalkSpeed = 130.f;
	ChaseSpeed = 400.f * LooterPlayerSize::MoveScale;
	// The brain's chase stops it at HoldDistance (BeginPlay sets it again from the tuning a level may have changed).
	AttackRange = Tuning.HoldDistance / ChaseStopShare;
	// Its attack is the swipe, run down: quick to wind up, slow to recover.
	AttackWindup = 0.55f;
	AttackRecovery = 0.7f;
	AttackCooldown = 1.f;
	// It never phase-steps nearer than it means to stand, and only steps up when it has fallen well behind.
	PhaseStep.NearestToTarget = Tuning.HoldDistance - 100.f;
	PhaseStep.FarBehind = 2000.f;
	// Its Sunday clothes, always (the Unpaid's second set, MI_Ghost_B), and bare-headed: no slouch hat, so it reads apart.
	Clothes = 1;
	GetHat()->SetStaticMesh(nullptr);
	GetHat()->SetVisibility(false);
	Voice->BarkKind = BarkKind;

	USkeletalMeshComponent* Body = GetMesh();
	const USkeletalMesh* Model = Body->GetSkeletalMeshAsset();
	static UStaticMesh* const BellModel = FindIfMade<UStaticMesh>(BellPath);
	static UStaticMesh* const RopeModel = FindIfMade<UStaticMesh>(RopePath);
	Bell = MakeProp(*this, TEXT("Bell"), Body, Rig.RightArm.Hand, HungAtRest(Model, Rig.RightArm, RightGripWithoutRig), BellModel);
	Rope = MakeProp(*this, TEXT("Rope"), Body, Rig.LeftArm.Hand, HungAtRest(Model, Rig.LeftArm, LeftGripWithoutRig), RopeModel);

	// The bell's hit zone: a ball round its body, seen by bullets (and sight) only, as the body's hit zones are.
	BellHitZone = CreateDefaultSubobject<USphereComponent>(TEXT("BellHitZone"));
	BellHitZone->SetupAttachment(Bell);
	BellHitZone->SetRelativeLocation(FVector(0.f, 0.f, -BellBodyDrop));
	BellHitZone->InitSphereRadius(Tuning.BellHitRadius);
	BellHitZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BellHitZone->SetCollisionObjectType(ECC_WorldDynamic);
	BellHitZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	BellHitZone->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	BellHitZone->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BellHitZone->SetGenerateOverlapEvents(false);
	BellHitZone->SetCanEverAffectNavigation(false);
}

FLinearColor ABellRingerCreature::TollColor()
{
	// Soul-light, pale and cold: the Unpaid's rim, which is no rarity color.
	return FLinearColor::FromSRGBColor(FColor(0xD4, 0xEC, 0xF2));
}

FLinearColor ABellRingerCreature::RaiseColor()
{
	return FLinearColor::FromSRGBColor(FColor(0xB8, 0xE0, 0xE4));
}

void ABellRingerCreature::BeginPlay()
{
	// The bell and the rope wear the body's look (its dissolve, its rank's ember), as Abel's lantern does: listed before the
	// Unpaid's start applies it.
	LookParts.AddUnique(Bell);
	LookParts.AddUnique(Rope);
	// Its rank, level, size, clothes and rig first.
	Super::BeginPlay();

	AttackRange = Tuning.HoldDistance / ChaseStopShare;
	PhaseStep.NearestToTarget = FMath::Max(Tuning.HoldDistance - 100.f, 0.f);
	BellHitZone->SetSphereRadius(Tuning.BellHitRadius);
	// On the fists the configured rig names (DefaultGame.ini may rename them), as the Unpaid puts its hat on.
	Bell->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Rig.RightArm.Hand);
	Rope->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Rig.LeftArm.Hand);
	SetupArmLayer();
	if (bRigReady)
	{
		UpdateProps(0.f);
	}
	TollWait = Tuning.FirstTollDelay;
	ApplyBellHitZone();
}

void ABellRingerCreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ABellRingerRing* Ring = Halo.Get())
	{
		Ring->Finish();
	}
	Halo.Reset();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// The frame
// ---------------------------------------------------------------------------

void ABellRingerCreature::Tick(float DeltaSeconds)
{
	// The Unpaid's frame first: its brain (drifting in to hold its distance, the swipe when run down), its phase-step, and
	// its body, with the bell's arm laid over it (LayerPose).
	Super::Tick(DeltaSeconds);

	RingerClock += DeltaSeconds;
	if (!IsDead())
	{
		if (UBellRingerGraves* Graves = UBellRingerGraves::Get(this))
		{
			Graves->Watch(GetWorld()->GetTimeSeconds());
		}
		TickStance(DeltaSeconds);
		TickToll(DeltaSeconds);
	}
	// The props hang from the fists as this frame posed them.
	if (bRigReady && !IsPoseFrozen())
	{
		UpdateProps(DeltaSeconds);
	}
	ApplyBellHitZone();
}

// ---------------------------------------------------------------------------
// The bell as a target
// ---------------------------------------------------------------------------

bool ABellRingerCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	// The bell is its weak spot: any shot on it is critical, besides the Unpaid's coal.
	if (Hit.GetComponent() && Hit.GetComponent() == BellHitZone.Get())
	{
		return true;
	}
	return Super::IsCriticalSpot(Hit);
}

FVector ABellRingerCreature::GetBellCenter() const
{
	// Where this frame's pose has it: the mesh takes the same pose (a test level's never does, so it's read off the pose).
	if (bArmLayerReady && Bones.IsValidIndex(RightHand))
	{
		const FTransform OnBody = BellHitZone->GetRelativeTransform() * Bell->GetRelativeTransform() * Bones[RightHand].Posed;
		return GetMesh()->GetComponentTransform().TransformPosition(OnBody.GetLocation());
	}
	return BellHitZone->GetComponentLocation();
}

bool ABellRingerCreature::IsBellHit(const FVector& Point) const
{
	return BellRingerRules::IsBellHit(GetBellCenter(), BellHitZone->GetScaledSphereRadius(), Point);
}

FVector2D ABellRingerCreature::GetBellSwing() const
{
	return FVector2D(FMath::RadiansToDegrees(BellSwing.Angle.X), FMath::RadiansToDegrees(BellSwing.Angle.Y));
}

void ABellRingerCreature::SetHitVolumesEnabled(bool bEnabled)
{
	Super::SetHitVolumesEnabled(bEnabled);
	bBellZoneWanted = bEnabled;
	ApplyBellHitZone();
}

void ABellRingerCreature::ApplyBellHitZone()
{
	// As the body's: a shot passes through what has mostly faded away.
	const bool bOn = bBellZoneWanted && !IsDead() && GetPhase() < GoneEnoughToMiss;
	if (bOn != bBellZoneOn)
	{
		BellHitZone->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		bBellZoneOn = bOn;
	}
}

// ---------------------------------------------------------------------------
// Hits, death and coming back
// ---------------------------------------------------------------------------

void ABellRingerCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	// The Unpaid's flinch first (its arms flung, its coal flaring on a crit); then the bell, if that's where it was hit.
	Super::OnHurt(bCritical, HitLocation);
	if (IsBellHit(HitLocation))
	{
		BellStruck(HitLocation);
	}
}

void ABellRingerCreature::OnDied()
{
	Super::OnDied();
	// The bell's last knock as it falls; a toll it was ringing is lost.
	AbandonToll();
	StopBackingOff();
	LooterSound::PlayAt(this, LooterSoundCue::BellRinger::Struck, GetBellCenter(), 0.6f, 0.8f * Voice->GetPitch());
}

void ABellRingerCreature::OnRespawned()
{
	Super::OnRespawned();
	// A new life: its raises and its tolls start again.
	SetAct(EAct::None);
	TollWait = Tuning.FirstTollDelay;
	TollCount = 0;
	RaisedCount = 0;
	LastToll = EBellToll::None;
	Raised.Reset();
	bWasHunting = false;
	StopBackingOff();
	SinceBackOff = 1000.f;
	BellSwing = FSwing();
	RopeSwing = FSwing();
	bArmsStarted = false;
	LayerWeight = 1.f;
	bBellZoneWanted = true;
	ApplyBellHitZone();
}

TArray<ACreatureBase*> ABellRingerCreature::GetRaised() const
{
	TArray<ACreatureBase*> Living;
	for (const TWeakObjectPtr<ACreatureBase>& Each : Raised)
	{
		if (ACreatureBase* Creature = Each.Get())
		{
			Living.Add(Creature);
		}
	}
	return Living;
}
