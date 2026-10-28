// AUnpaidCreature: construction, its look (clothes, the rank's color, the dissolve), its coal, and its frame, hits, death
// and return. UnpaidCreatureAttack.cpp has the shriek and the lunge, UnpaidCreatureMoves.cpp its fighting kit,
// UnpaidCreaturePhase.cpp the phase-step and UnpaidCreatureRig.cpp the body.

#include "Creatures/UnpaidCreature.h"
#include "Combat/GunfeelReactTuning.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Creatures/CreatureTierLook.h"
#include "Player/PlayerSize.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The capsule: a gaunt man's height and width. The model's ground (its origin) is the capsule's foot. */
	constexpr float CapsuleRadius = 34.f;
	constexpr float CapsuleHalfHeight = 90.f;

	/** Its hit zones turn off once it's this far faded: a shot passes through what's mostly gone. */
	constexpr float GoneEnoughToMiss = 0.6f;

	/**
	 * Dying: it slumps and crumples for a moment (the pose's Crumple), then breaks up into drifting ash over this long while
	 * its coal goes out (ShedDeathAsh: UnpaidCreatureMoods.cpp).
	 */
	constexpr float DeathFadeDelay = 0.9f;
	constexpr float DeathFadeSeconds = 1.5f;

	/** A crit makes the coal flare this much, easing off at this rate. */
	constexpr float CritFlare = 0.8f;
	constexpr float FlareFade = 4.f;
	/** Any body hit stirs the coal this much at a flinch strength of 1 (never past a crit's flare). */
	constexpr float HitFlare = 0.25f;

	/**
	 * An asset made after this code (SK_Unpaid by Art/Models/Creatures/Unpaid.py, the clothes by
	 * build_creature_materials.py), found only once it exists: until then the class works, and its tests run, without it.
	 */
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
	 * The hat's turn on its bone. The hat was modelled where it sits, in the model's own axes, with its pivot on the bone:
	 * turned back by the bone's rest turn it sits as modelled, and from there it follows the head. False without the bone.
	 */
	bool HatTurnOnBone(const USkeletalMesh* Model, FName Bone, FQuat& OutTurn)
	{
		const int32 Index = Model && !Bone.IsNone() ? Model->GetRefSkeleton().FindBoneIndex(Bone) : INDEX_NONE;
		if (Index == INDEX_NONE)
		{
			return false;
		}
		OutTurn = FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), Index).GetRotation().Inverse();
		return true;
	}
}

AUnpaidCreature::AUnpaidCreature()
{
	DisplayName = FText::FromString(TEXT("Unpaid"));
	Health->MaxHealth = 160.f;
	AttackDamage = 8.f;
	// The coal is found by the shot's line (IsCriticalSpot), never by a bone's hit zone alone.
	CriticalSpotBones.Reset();

	// It drifts toward its prey at a little under a walk, and phase-steps when it falls behind. Tuned against the
	// full-size player's 600 walk (78% of it) and scaled with the player's speed and pace, as the spiders are: at 470
	// against the smaller player's 510 it nearly kept pace (the user's call, 2026-10-08).
	WalkSpeed = 150.f;
	ChaseSpeed = 470.f * LooterPlayerSize::MoveScale;
	WanderRadius = 600.f;
	// The dead come together: hurt one and those near it turn on you too.
	PackTag = TEXT("Unpaid");
	PackAlertRadius = 1500.f;
	// The lunge starts from a few meters out: the shriek, then the flight at its target.
	AttackRange = 380.f;
	AttackWindup = 0.6f;
	AttackRecovery = 0.7f;
	AttackCooldown = 1.3f;
	// It crumples and breaks up into ash (DeathFadeDelay, DeathFadeSeconds) before its body is taken away.
	CorpseTime = 2.6f;
	NoticeCue = CreatureReactCue::UnpaidNotice;
	HealthBarHeight = 115.f;
	BasicCoalColor = FLinearColor::FromSRGBColor(FColor(0xB0, 0x2A, 0x18));
	Traits = UnpaidRules::RankTraits(ECreatureRank::Basic);

	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	// Slow to gather speed and slow to stop: it glides rather than walks.
	Movement->MaxAcceleration = 1200.f;
	Movement->BrakingDecelerationWalking = 900.f;
	Movement->GroundFriction = 4.f;
	Movement->RotationRate = FRotator(0.f, 300.f, 0.f);

	static USkeletalMesh* const Model = FindIfMade<USkeletalMesh>(TEXT("/Game/Art/Creatures/SK_Unpaid.SK_Unpaid"));
	static UMaterialInterface* const SundayClothes = FindIfMade<UMaterialInterface>(TEXT("/Game/Art/Materials/MI_Ghost_B.MI_Ghost_B"));
	static UMaterialInterface* const HarvestClothes = FindIfMade<UMaterialInterface>(TEXT("/Game/Art/Materials/MI_Ghost_C.MI_Ghost_C"));
	for (UMaterialInterface* Worn : { SundayClothes, HarvestClothes })
	{
		if (Worn)
		{
			OtherClothes.Add(Worn);
		}
	}

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMeshAsset(Model);
	// The model's origin is the ground under it: the foot of the capsule. It floats above it by itself.
	Body->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalfHeight));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UCreaturePoseAnimInstance::StaticClass());
	// The hit zones follow the bones even while it's off screen; posed after this actor works out the frame's pose.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->PrimaryComponentTick.TickGroup = TG_PostPhysics;
	// A ghost casts no shadow: it reads as not quite there, and a crowd of dithered bodies costs no shadow passes.
	Body->SetCastShadow(false);

	// The physics asset's bodies are the hit zones, seen only by weapon and visibility traces.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);

	// The hat is a mesh of its own on its bone, worn from the start so what reads the defaults (the bestiary's stand) sees
	// it on; play puts it on again by the configured rig (PutOnHat). It has no collision: a shot meets the head under it.
	static UStaticMesh* const HatModel = FindIfMade<UStaticMesh>(TEXT("/Game/Art/Creatures/SM_UnpaidHat.SM_UnpaidHat"));
	Hat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hat"));
	Hat->SetupAttachment(Body, Rig.Hat);
	FQuat HatTurn;
	if (HatTurnOnBone(Model, Rig.Hat, HatTurn))
	{
		Hat->SetRelativeRotation(HatTurn);
	}
	Hat->SetStaticMesh(HatModel);
	Hat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hat->SetGenerateOverlapEvents(false);
	Hat->SetCanEverAffectNavigation(false);
	Hat->SetCastShadow(false);
}

void AUnpaidCreature::BeginPlay()
{
	// Its rank and level come first (ACreatureBase::BeginPlay, then OnRankChanged: the coal's color and the lunge's pace).
	Super::BeginPlay();

	GetMesh()->AddTickPrerequisiteActor(this);
	PoseSeed = FMath::FRandRange(0.f, 100.f);
	LastYaw = static_cast<float>(GetActorRotation().Yaw);
	PutOnClothes();
	bRigReady = SetupRig();
	PutOnHat();
	ApplyLook(true);
	if (bRigReady)
	{
		// Posed at once, so it never shows a frame in its rest pose.
		AnimateBody(0.f);
	}
}

// ---------------------------------------------------------------------------
// Look
// ---------------------------------------------------------------------------

void AUnpaidCreature::PutOnClothes()
{
	const int32 Choice = Clothes >= 0 ? Clothes : FMath::RandRange(0, OtherClothes.Num());
	UMaterialInterface* Worn = Choice > 0 && OtherClothes.IsValidIndex(Choice - 1) ? OtherClothes[Choice - 1].Get() : nullptr;
	const int32 Slot = GetMesh()->GetMaterialIndex(BodySlot);
	if (Worn && Slot != INDEX_NONE)
	{
		GetMesh()->SetMaterial(Slot, Worn);
		// The hat goes with the clothes.
		const int32 HatSlot = Hat->GetMaterialIndex(BodySlot);
		if (HatSlot != INDEX_NONE)
		{
			Hat->SetMaterial(HatSlot, Worn);
		}
	}
}

void AUnpaidCreature::PutOnHat()
{
	FQuat Turn;
	if (!HatTurnOnBone(GetMesh()->GetSkeletalMeshAsset(), Rig.Hat, Turn) || !Hat->GetStaticMesh())
	{
		Hat->SetVisibility(false);
		return;
	}
	Hat->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Rig.Hat);
	Hat->SetRelativeLocationAndRotation(FVector::ZeroVector, Turn);
	Hat->SetVisibility(true);
}

FLinearColor AUnpaidCreature::GetCoalColor() const
{
	// Its tier's glow (FCreatureRankInfo::GlowColor, the tag's colour unless set apart); a Regular one's dull red.
	const ECreatureRank Rank = GetRank();
	return Rank == ECreatureRank::Basic ? BasicCoalColor : CreatureTierLook::GlowColor(Rank);
}

void AUnpaidCreature::ApplyCoalColor()
{
	// The coal and the ember edge both read it (M_Ghost), on every slot of the body and the hat: no material instance per
	// creature. The top tiers burn harder (CreatureTierLook::CoalGlow): Ethereal's red is a dark colour.
	const FLinearColor Color = GetCoalColor();
	const FVector4 RankColor(Color.R, Color.G, Color.B, CreatureTierLook::CoalGlow(GetRank(), Traits.CoalGlow));
	GetMesh()->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, RankColor);
	Hat->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, RankColor);
	for (UPrimitiveComponent* Part : LookParts)
	{
		if (Part)
		{
			Part->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, RankColor);
		}
	}
}

void AUnpaidCreature::ApplyLook(bool bForce)
{
	// The hat dissolves with the body.
	USkeletalMeshComponent* Body = GetMesh();
	if (bForce || !FMath::IsNearlyEqual(PhaseAmount, ShownPhase, 0.002f))
	{
		Body->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, PhaseAmount);
		Hat->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, PhaseAmount);
		for (UPrimitiveComponent* Part : LookParts)
		{
			if (Part)
			{
				Part->SetCustomPrimitiveDataFloat(UnpaidLook::PhaseIndex, PhaseAmount);
			}
		}
		ShownPhase = PhaseAmount;
	}
	if (bForce || !FMath::IsNearlyEqual(Heat, ShownHeat, 0.01f))
	{
		Body->SetCustomPrimitiveDataFloat(UnpaidLook::HeatIndex, Heat);
		Hat->SetCustomPrimitiveDataFloat(UnpaidLook::HeatIndex, Heat);
		for (UPrimitiveComponent* Part : LookParts)
		{
			if (Part)
			{
				Part->SetCustomPrimitiveDataFloat(UnpaidLook::HeatIndex, Heat);
			}
		}
		ShownHeat = Heat;
	}
	// A shot passes through what has mostly faded away.
	const bool bOn = bHitVolumesWanted && PhaseAmount < GoneEnoughToMiss;
	if (bForce || bOn != bHitVolumesOn)
	{
		Body->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		bHitVolumesOn = bOn;
	}
}

void AUnpaidCreature::SetHitVolumesEnabled(bool bEnabled)
{
	bHitVolumesWanted = bEnabled;
	ApplyLook(true);
}

void AUnpaidCreature::OnRankChanged()
{
	// A Restless one's quicker lunge starts with a shorter shriek: its wind-up is a share of the class's (or the level's).
	Traits = UnpaidRules::RankTraits(GetRank());
	if (!bTimingCaptured)
	{
		bTimingCaptured = true;
		BaseWindup = AttackWindup;
		// The lunge's recovery, which its kit's rake replaces for a rake and puts back for the next lunge.
		Kit.BaseRecovery = AttackRecovery;
	}
	AttackWindup = BaseWindup * Traits.WindupScale;
	ApplyCoalColor();
}

// ---------------------------------------------------------------------------
// The coal
// ---------------------------------------------------------------------------

ACreatureBase::FFootprint AUnpaidCreature::GetFootprint() const
{
	// Unpaid.py: its shoulders and arms 25 cm either side, its shroud's top 45 cm behind its middle.
	FFootprint Footprint;
	Footprint.Front = 8.f;
	Footprint.Back = -30.f;
	Footprint.Radius = CapsuleRadius;
	return Footprint;
}

bool AUnpaidCreature::IsCoalShotBone(FName Bone) const
{
	// The coal shows through the chest: a shot that lands on an arm, a hand, the head or the shroud has something in the way.
	return !Bone.IsNone() && (Bone == Rig.Chest || Bone == Rig.Coal || Bone == Rig.Spine);
}

bool AUnpaidCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	if (Hit.GetComponent() != GetMesh() || !IsCoalShotBone(Hit.BoneName))
	{
		return false;
	}
	// Its coal is as much bigger as it is (a Gravebound one's by its rank's size).
	const float Scale = GetSizeScale();
	return UnpaidRules::IsCoalShot(GetCoalLocation(), GetCoalFacing(), Hit.ImpactPoint, Hit.TraceEnd - Hit.TraceStart,
		CoalCritRadius * Scale, CoalViewAngle, CoalReach * Scale);
}

FVector AUnpaidCreature::GetCoalLocation() const
{
	const USkeletalMeshComponent* Body = GetMesh();
	if (bRigReady && CoalBone != INDEX_NONE)
	{
		return Body->GetBoneLocation(Rig.Coal);
	}
	return Body->GetComponentTransform().TransformPosition(CoalPointWithoutRig);
}

FVector AUnpaidCreature::GetCoalFacing() const
{
	// Out of the chest as it's turned now (leaning into a lunge), or the way it faces.
	const USkeletalMeshComponent* Body = GetMesh();
	if (bRigReady && ChestBone != INDEX_NONE)
	{
		return Body->GetComponentTransform().TransformVectorNoScale(Bones[ChestBone].Turned.RotateVector(FVector::ForwardVector));
	}
	return GetActorForwardVector();
}

// ---------------------------------------------------------------------------
// The frame, hits, death and coming back
// ---------------------------------------------------------------------------

void AUnpaidCreature::Tick(float DeltaSeconds)
{
	// The brain: senses, chase, the attack (Strike starts the lunge).
	Super::Tick(DeltaSeconds);

	SinceSlowingShriek = FMath::Min(SinceSlowingShriek + DeltaSeconds, 1000.f);
	if (IsDead())
	{
		DeathTime += DeltaSeconds;
		UpdateDeathLook(DeltaSeconds);
	}
	else
	{
		Heat = FMath::FInterpTo(Heat, 0.f, DeltaSeconds, FlareFade);
		TickPhaseStep(DeltaSeconds);
		if (bLunging)
		{
			TickLunge(DeltaSeconds);
		}
		// Its fighting kit: the rake's second swipe, cinders and rallies on the ring, the flank, its pace (UnpaidCreatureMoves.cpp).
		TickFightKit(DeltaSeconds);
		if (IsPhasing() || Kit.Action == EUnpaidAction::Rally)
		{
			// It hangs where it is while it fades, or screams its pack on: the brain's steering goes unused.
			ConsumeMovementInputVector();
		}
	}
	// Its rise out of the ground, its glances, its coal flaring as it notices, its shroud's dust (UnpaidCreatureMoods.cpp).
	TickReactions(DeltaSeconds);
	if (bRigReady && !IsPoseFrozen())
	{
		AnimateBody(DeltaSeconds);
	}
	ApplyLook(false);
}

void AUnpaidCreature::UpdateDeathLook(float DeltaSeconds)
{
	// It slumps and crumples, then breaks up into drifting ash while its coal goes out.
	PhaseAmount = FMath::Clamp((DeathTime - DeathFadeDelay) / DeathFadeSeconds, 0.f, 1.f);
	Heat = -PhaseAmount;
	ShedDeathAsh(DeltaSeconds);
}

void AUnpaidCreature::RiseIn()
{
	if (IsDead())
	{
		return;
	}
	// The end of a phase-step without its start: gone, and fading back in where it stands, as it comes up out of the ground
	// clawing (RiseTime: the pose's RiseDrop and PoseTargets), the dirt thrown up round it.
	PhaseState = EPhaseState::In;
	PhaseTime = 0.f;
	PhaseAmount = 1.f;
	RiseTime = 0.f;
	ApplyLook(true);
	if (UCreatureReactComponent* Reacts = GetReact())
	{
		Reacts->ThrowGraveDirt(GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), GetSizeScale(), 0.9f);
	}
}

void AUnpaidCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	if (IsDead())
	{
		return;
	}
	// A flinch away from the hit, its arms flung the other way (UnpaidCreatureArms.cpp), sized by the hit (Gunfeel); a shot
	// on the coal makes it flare, and any hit stirs it a little.
	const float Strength = GetLastHit().FlinchStrength(FGunfeelReactTuning::Live());
	FlinchAway = GetActorRotation().UnrotateVector(GetActorLocation() - HitLocation).GetSafeNormal();
	ReactToHit(bCritical, Strength);
	if (bCritical)
	{
		Heat = CritFlare;
	}
	else if (GetLastHit().bFresh)
	{
		Heat = FMath::Max(Heat, FMath::Min(HitFlare * Strength, CritFlare));
	}
	// The shroud jerks with it.
	const float Jerk = (bCritical ? 160.f : 90.f) * FMath::Min(Strength, 2.f);
	for (FShroudChain& Chain : Chains)
	{
		for (FShroudChain::FLink& Link : Chain.Links)
		{
			Link.BackSpeed += Jerk * static_cast<float>(FlinchAway.X);
			Link.SideSpeed += Jerk * static_cast<float>(FlinchAway.Y);
		}
	}
}

void AUnpaidCreature::OnDied()
{
	if (Kit.Action != EUnpaidAction::None)
	{
		EndKitAction(true);
	}
	Kit.Reset();
	CommitLunge(false);
	bLunging = false;
	PhaseState = EPhaseState::None;
	PhaseTime = 0.f;
	DeathTime = 0.f;
	RiseTime = -1.f;
	GlanceYaw = 0.f;
	AshOwed = -1.f;
	ResetProgress();
}

void AUnpaidCreature::OnRespawned()
{
	PhaseState = EPhaseState::None;
	PhaseAmount = 0.f;
	Heat = 0.f;
	DeathTime = 0.f;
	RiseTime = -1.f;
	GlanceYaw = 0.f;
	GlanceLeft = 0.f;
	CommitLunge(false);
	bLunging = false;
	Kit.Reset();
	SinceLastStep = 1000.f;
	ResetProgress();
	for (FShroudChain& Chain : Chains)
	{
		Chain.Settle();
	}
	// Back still: no flinch, its arms at rest on their springs.
	Flinch = FlinchSpeed = FlinchKick = 0.f;
	for (FArmSway& Sway : ArmSway)
	{
		Sway = FArmSway();
	}
	LastLocalVelocity = FVector::ZeroVector;
	bPoseStarted = false;
	ApplyLook(true);
	if (bRigReady)
	{
		AnimateBody(0.f);
	}
}

void AUnpaidCreature::OnPoseThawed()
{
	// It drifted on unposed: its shroud starts again from rest under it, and the pose settles at once.
	for (FShroudChain& Chain : Chains)
	{
		Chain.Settle();
	}
	bPoseStarted = false;
	if (bRigReady)
	{
		AnimateBody(0.f);
	}
}
