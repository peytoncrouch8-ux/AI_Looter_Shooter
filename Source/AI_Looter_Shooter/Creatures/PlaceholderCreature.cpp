// APlaceholderCreature: its clay body, look, sounds, hurt and death (its attacks and distance: PlaceholderCreatureMoves.cpp).

#include "Creatures/PlaceholderCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundRules.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureKillLook.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Creatures/PlaceholderCreatureDefinition.h"
#include "Weapons/WeaponFX.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* BlobPath = TEXT("/Game/Art/Creatures/SK_Slime.SK_Slime");
	const TCHAR* ClayPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	const TCHAR* BallPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	/** The engine's sphere is this wide at scale 1 (cm). */
	constexpr float BallSize = 100.f;
	/** The clay's surface variation (M_StylizedSurface's Variation): a hand-pressed look, not a smooth toy. */
	constexpr float ClayVariation = 0.25f;
	/** Its tag floats this far over its crown (cm). */
	constexpr float TagClearance = 35.f;
	/** The two lumps: where on the body (shares of its half-length, half-width and height) and how big (share of its narrower side). */
	const FVector LumpSpots[] = { FVector(0.25f, 0.55f, 0.55f), FVector(-0.45f, -0.45f, 0.4f) };
	const float LumpSizes[] = { 0.42f, 0.36f };
	/** A hit's jolt (degrees for a full-strength hit) and how fast it eases out (a second). */
	constexpr float JoltPitch = 10.f;
	constexpr float JoltRoll = 8.f;
	constexpr float JoltEase = 9.f;
	/** Its hit sound at most this often (s: one for a shotgun's blast), its hurt cry this often and this likely. */
	constexpr double HitSoundGap = 0.05;
	constexpr float HurtCryChance = 0.3f;

	/** A scale along the mesh's axes, as the bone's own (its rest turn maps its axes onto the mesh's): the slime's rule. */
	FVector BoneScale(const FQuat& Rotation, const FVector& MeshScale)
	{
		FVector Result;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			FVector Unit = FVector::ZeroVector;
			Unit[Axis] = 1.f;
			const FVector Along = Rotation.RotateVector(Unit);
			Result[Axis] = Along.X * Along.X * MeshScale.X + Along.Y * Along.Y * MeshScale.Y + Along.Z * Along.Z * MeshScale.Z;
		}
		return Result;
	}

	/** Hit zones seen only by weapon and visibility traces (as the slime's), so pawns walk through and shots stop. */
	void MakeHitZone(UPrimitiveComponent* Part)
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

APlaceholderCreature::APlaceholderCreature()
{
	DisplayName = NSLOCTEXT("Placeholder", "DefaultName", "Placeholder");
	Health->MaxHealth = 150.f;
	GetCapsuleComponent()->InitCapsuleSize(40.f, 45.f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Blob(BlobPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Clay(ClayPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Ball(BallPath);

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMeshAsset(Blob.Object);
	Body->SetRelativeLocation(FVector(0.f, 0.f, -45.f));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UCreaturePoseAnimInstance::StaticClass());
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->PrimaryComponentTick.TickGroup = TG_PostPhysics;
	// The physics asset's hulls (the dome and its core) are the body's hit zones.
	MakeHitZone(Body);
	if (Clay.Object)
	{
		// Opaque clay on every slot (the gel's translucency would read as a slime), tinted per kind as play begins.
		for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
		{
			Body->SetMaterial(Slot, Clay.Object);
		}
	}

	// Its head: a clay ball on the body bone, the critical spot (placed and sized for its kind as play begins).
	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(Body, BodyBone);
	Head->SetStaticMesh(Ball.Object);
	Head->SetMaterial(0, Clay.Object);
	Head->SetRelativeScale3D(FVector(0.3f));
	MakeHitZone(Head);

	// Two lumps: looks only (no collision, no shadow of their own: the body's covers them).
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(LumpSizes)); ++Index)
	{
		UStaticMeshComponent* Lump = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Lump%d"), Index));
		Lump->SetupAttachment(Body, BodyBone);
		Lump->SetStaticMesh(Ball.Object);
		Lump->SetMaterial(0, Clay.Object);
		Lump->SetRelativeScale3D(FVector(0.25f));
		Lump->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Lump->SetGenerateOverlapEvents(false);
		Lump->SetCanEverAffectNavigation(false);
		Lump->SetCastShadow(false);
		Lumps.Add(Lump);
	}
}

void APlaceholderCreature::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Before play begins (its rank and size are applied in BeginPlay, on this shape): the capsule, the stretch, the look.
	ApplyShape();
	ApplyLook();
	bShapeApplied = true;
}

void APlaceholderCreature::BeginPlay()
{
	// A level that never initialised its actors (a test level) skipped PostInitializeComponents: the shape first, before its
	// rank and size are applied on it.
	if (!bShapeApplied)
	{
		ApplyShape();
		ApplyLook();
		bShapeApplied = true;
	}
	Super::BeginPlay();
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	const int32 BodyIndex = Model ? Model->GetRefSkeleton().FindBoneIndex(BodyBone) : INDEX_NONE;
	bRigReady = BodyIndex != INDEX_NONE;
	if (!bRigReady)
	{
		UE_LOG(LogLooter, Error, TEXT("%s needs SK_Slime with its body bone (Art/Models/Creatures/Slime.py)."), *GetName());
	}
	else
	{
		BodyRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), BodyIndex);
	}
	if (!Definition)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s has no placeholder definition: a plain clay blob (make its kind with create_placeholder_creatures.py)."),
			*GetName());
	}
	PlaceParts();
	// Each one breathes in its own time, so a group doesn't move in step.
	Clock = FMath::FRandRange(0.f, 10.f);
	AnimateBody(0.f);
}

void APlaceholderCreature::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	DeadTime = IsDead() ? DeadTime + DeltaSeconds : 0.f;
	EntranceLeft = FMath::Max(0.f, EntranceLeft - DeltaSeconds);
	Jolt *= FMath::Exp(-JoltEase * DeltaSeconds);
	if (GetRole() == EPlaceholderRole::Ranged)
	{
		TickKeepAway(DeltaSeconds);
	}
	// Far away out of view it goes on unposed; its pose is worked out afresh from its state when it's seen again.
	if (!IsPoseFrozen())
	{
		AnimateBody(DeltaSeconds);
	}
}

// ---------------------------------------------------------------------------
// Its kind: the definition written into it
// ---------------------------------------------------------------------------

bool APlaceholderCreature::BakeDefinition(APlaceholderCreature* Creature)
{
	const UPlaceholderCreatureDefinition* Kind = Creature ? Creature->Definition.Get() : nullptr;
	if (!Kind)
	{
		return false;
	}
	Creature->Modify();
	Creature->Health->Modify();
	Creature->DisplayName = Kind->GetTagName();
	Creature->Health->MaxHealth = Kind->MaxHealth;
	Creature->AttackDamage = Kind->AttackDamage;
	Creature->XPReward = Kind->XPReward;
	Creature->WalkSpeed = Kind->WalkSpeed;
	Creature->ChaseSpeed = Kind->ChaseSpeed;
	Creature->AttackRange = Kind->Role == EPlaceholderRole::Ranged ? PlaceholderRules::RangedAttackRange(Kind->CloseInTo) : Kind->AttackRange;
	Creature->AttackWindup = Kind->AttackWindup;
	Creature->AttackRecovery = Kind->AttackRecovery;
	Creature->AttackCooldown = Kind->AttackCooldown;
	Creature->AggroRadius = Kind->AggroRadius;
	Creature->PackAlertRadius = Kind->PackAlertRadius;
	Creature->PackTag = Kind->PackTag;
	return true;
}

bool APlaceholderCreature::MatchesDefinition(TArray<FString>& OutDifferences) const
{
	const UPlaceholderCreatureDefinition* Kind = Definition.Get();
	if (!Kind)
	{
		OutDifferences.Add(TEXT("no definition"));
		return false;
	}
	const int32 Before = OutDifferences.Num();
	auto Same = [&OutDifferences](const TCHAR* What, float Mine, float Its)
	{
		if (!FMath::IsNearlyEqual(Mine, Its, 0.01f))
		{
			OutDifferences.Add(FString::Printf(TEXT("%s %.2f, its definition's %.2f"), What, Mine, Its));
		}
	};
	if (!DisplayName.EqualTo(Kind->GetTagName()))
	{
		OutDifferences.Add(FString::Printf(TEXT("name '%s', its definition's '%s'"), *DisplayName.ToString(), *Kind->GetTagName().ToString()));
	}
	Same(TEXT("health"), Health->MaxHealth, Kind->MaxHealth);
	Same(TEXT("attack"), AttackDamage, Kind->AttackDamage);
	Same(TEXT("experience"), static_cast<float>(XPReward), static_cast<float>(Kind->XPReward));
	Same(TEXT("walk"), WalkSpeed, Kind->WalkSpeed);
	Same(TEXT("chase"), ChaseSpeed, Kind->ChaseSpeed);
	Same(TEXT("attack range"), AttackRange,
		Kind->Role == EPlaceholderRole::Ranged ? PlaceholderRules::RangedAttackRange(Kind->CloseInTo) : Kind->AttackRange);
	Same(TEXT("wind-up"), AttackWindup, Kind->AttackWindup);
	Same(TEXT("recovery"), AttackRecovery, Kind->AttackRecovery);
	Same(TEXT("cooldown"), AttackCooldown, Kind->AttackCooldown);
	Same(TEXT("aggro"), AggroRadius, Kind->AggroRadius);
	Same(TEXT("pack call"), PackAlertRadius, Kind->PackAlertRadius);
	if (PackTag != Kind->PackTag)
	{
		OutDifferences.Add(TEXT("pack tag"));
	}
	return OutDifferences.Num() == Before;
}

EPlaceholderRole APlaceholderCreature::GetRole() const
{
	return Definition ? Definition->Role : EPlaceholderRole::Melee;
}

// ---------------------------------------------------------------------------
// Body and look
// ---------------------------------------------------------------------------

void APlaceholderCreature::ApplyShape()
{
	if (!Definition)
	{
		return;
	}
	float Radius = 0.f;
	float HalfHeight = 0.f;
	PlaceholderRules::CapsuleFor(Definition->Length, Definition->Width, Definition->Height, Radius, HalfHeight);
	GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
	Stretch = PlaceholderRules::BodyStretch(Definition->Length, Definition->Width, Definition->Height);
	// The dome's origin is the ground under its middle: the capsule's foot.
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -HalfHeight));
	// The stretch rides on the body bone (AnimateBody), not the mesh component: a component's uneven scale would reach its
	// hit hulls and attached parts along the bone's own axes (Unreal composes scale per axis, without shear), not the mesh's.
	GetMesh()->SetRelativeScale3D(FVector::OneVector);
	HealthBarHeight = Definition->Height - HalfHeight + Definition->HoverHeight + TagClearance;
	// Its tag: "Name (plain type)" (the user's call), as its class's defaults already say once baked.
	DisplayName = Definition->GetTagName();
}

void APlaceholderCreature::ApplyLook()
{
	USkeletalMeshComponent* Body = GetMesh();
	UMaterialInterface* Base = Body->GetNumMaterials() > 0 ? Body->GetMaterial(0) : nullptr;
	if (!Base || !Definition)
	{
		return;
	}
	ClayLook = UMaterialInstanceDynamic::Create(Base, this);
	ClayLook->SetVectorParameterValue(TEXT("Color"), Definition->ClayColor);
	ClayLook->SetScalarParameterValue(TEXT("Glow"), 0.f);
	ClayLook->SetScalarParameterValue(TEXT("Variation"), ClayVariation);
	for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
	{
		Body->SetMaterial(Slot, ClayLook);
	}
	Head->SetMaterial(0, ClayLook);
	for (UStaticMeshComponent* Lump : Lumps)
	{
		Lump->SetMaterial(0, ClayLook);
	}
}

void APlaceholderCreature::PlaceParts()
{
	if (!Definition)
	{
		return;
	}
	const UPlaceholderCreatureDefinition& Kind = *Definition;
	// A part wanted at Spot (cm from the ground under its middle) Size wide, on the body bone, which carries the dome's
	// stretch in its own axes (AnimateBody): its place is taken back through the bone's turn and that stretch, and its scale
	// undoes the stretch so it stays round (Unreal composes an attached part's scale per axis of its parent).
	auto Place = [this](UStaticMeshComponent* Part, const FVector& Spot, float Size)
	{
		const FVector BoneStretch = BodyRest.GetScale3D() * BoneScale(BodyRest.GetRotation(), Stretch);
		const FVector FromBone = Spot - BodyRest.GetLocation();
		Part->SetRelativeLocation(BodyRest.GetRotation().UnrotateVector(FromBone) / BoneStretch);
		Part->SetRelativeRotation(FQuat::Identity);
		Part->SetRelativeScale3D(FVector(Size / BallSize) / BoneStretch);
	};
	Place(Head, PlaceholderRules::HeadSpot(Kind.Length, Kind.Height, Kind.HeadHeight, Kind.HeadForward), Kind.HeadSize);
	const float Narrow = FMath::Min(Kind.Length, Kind.Width);
	for (int32 Index = 0; Index < Lumps.Num() && Index < static_cast<int32>(UE_ARRAY_COUNT(LumpSizes)); ++Index)
	{
		const FVector Share = LumpSpots[Index];
		Place(Lumps[Index], FVector(Share.X * Kind.Length * 0.5f, Share.Y * Kind.Width * 0.5f, Share.Z * Kind.Height), Narrow * LumpSizes[Index]);
	}
}

void APlaceholderCreature::AnimateBody(float DeltaSeconds)
{
	if (!bRigReady)
	{
		return;
	}
	FPlaceholderPoseInput In;
	In.Role = GetRole();
	In.Length = Definition ? Definition->Length : 90.f;
	In.Height = Definition ? Definition->Height : 80.f;
	In.Hover = Definition && In.Role == EPlaceholderRole::Flyer ? Definition->HoverHeight : 0.f;
	In.Clock = Clock;
	In.Speed01 = static_cast<float>(GetVelocity().Size2D()) / FMath::Max(ChaseSpeed, 1.f);
	In.bAttacking = GetCreatureState() == ECreatureState::Attack;
	In.AttackTime = GetStateTime();
	In.Windup = AttackWindup;
	In.Recovery = AttackRecovery;
	In.bShot = bShotAttack;
	In.Notice = GetNoticeStrength();
	In.EntranceLeft = EntranceLength > 0.f ? EntranceLeft / EntranceLength : 0.f;
	In.bDead = IsDead();
	In.DeadTime = DeadTime;
	In.CorpseTime = CorpseTime;
	PoseNow = PlaceholderRules::PoseAt(In);

	// Its pose on the body bone, about its foot: the dome stretched to its kind and squashed (keeping its volume), lifted and
	// lunged (cm at size 1), turned by its leans and a hit's jolt.
	const FQuat Lean = FRotator(-(PoseNow.LeanPitch + Jolt.X), 0.f, PoseNow.LeanRoll + Jolt.Y).Quaternion();
	const FVector Offset(PoseNow.Lunge, 0.f, PoseNow.Lift);
	const FQuat RestTurn = BodyRest.GetRotation();
	const FTransform Posed(Lean * RestTurn, BodyRest.GetLocation() + Offset,
		BodyRest.GetScale3D() * BoneScale(RestTurn, Stretch * PlaceholderRules::SquashScale(PoseNow.Squash)));
	BonePose.Reset();
	BonePose.Add(FCreatureBonePose{ BodyBone, Posed });
}

FVector APlaceholderCreature::GetMuzzle() const
{
	const float Ahead = (Definition ? Definition->HeadSize : 30.f) * 0.6f * GetSizeScale();
	return Head->GetComponentLocation() + GetActorForwardVector() * Ahead;
}

bool APlaceholderCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	return Hit.GetComponent() == Head || Super::IsCriticalSpot(Hit);
}

ACreatureBase::FFootprint APlaceholderCreature::GetFootprint() const
{
	FFootprint Footprint;
	Footprint.Radius = Definition ? FMath::Max(Definition->Length, Definition->Width) * 0.4f : 40.f;
	return Footprint;
}

void APlaceholderCreature::SetHitVolumesEnabled(bool bEnabled)
{
	const ECollisionEnabled::Type Zones = bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
	GetMesh()->SetCollisionEnabled(Zones);
	Head->SetCollisionEnabled(Zones);
}

// ---------------------------------------------------------------------------
// Sounds (borrowed cries until its kind has its own; hits of wet clay)
// ---------------------------------------------------------------------------

FName APlaceholderCreature::CueFor(int32 Which) const
{
	using namespace LooterSoundCue;
	// 0 alert, 1 attack (its tell), 2 hurt, 3 death.
	static const TCHAR* const Beast[] = { SpiderAlert, SpiderAttack, SpiderHurt, SpiderDeath };
	static const TCHAR* const Dead[] = { UnpaidAlert, UnpaidShriek, UnpaidHurt, UnpaidDeath };
	static const TCHAR* const Clay[] = { nullptr, SlimeAttack, SlimeHurt, SlimeDeath };
	const EPlaceholderVoice KindVoice = Definition ? Definition->Voice : EPlaceholderVoice::Clay;
	const TCHAR* const* Set = KindVoice == EPlaceholderVoice::Beast ? Beast : KindVoice == EPlaceholderVoice::Dead ? Dead : Clay;
	const TCHAR* Cue = Which >= 0 && Which < 4 ? Set[Which] : nullptr;
	return Cue ? FName(Cue) : NAME_None;
}

void APlaceholderCreature::PlayCue(FName Cue, float Volume) const
{
	if (!Cue.IsNone())
	{
		LooterSound::PlayAttached(Cue, GetRootComponent(), NAME_None, Volume, LooterSoundRules::PitchForSize(GetSizeScale()));
	}
}

// ---------------------------------------------------------------------------
// Hurt, noticing, coming in, death
// ---------------------------------------------------------------------------

void APlaceholderCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	// Rocked away from the hit, harder for a crit or a heavy burst.
	const FCreatureHit& Hit = GetLastHit();
	const float Strength = FMath::Clamp(0.6f + Hit.BurstShare * 4.f, 0.6f, 2.f) * (bCritical ? 1.5f : 1.f);
	const FVector Way = GetActorQuat().UnrotateVector(Hit.Direction.IsNearlyZero() ? -GetActorForwardVector() : Hit.Direction);
	Jolt.X -= static_cast<float>(Way.X) * JoltPitch * Strength;
	Jolt.Y += static_cast<float>(Way.Y) * JoltRoll * Strength;

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastHitSound >= HitSoundGap)
	{
		LastHitSound = Now;
		LooterSound::PlayAt(this, LooterSoundCue::SlimeHit, HitLocation, bCritical ? 1.f : 0.8f, LooterSoundRules::PitchForSize(GetSizeScale()));
	}
	// Its cry now and then, never on the killing blow (the death cry follows at once).
	if (Health->GetHealth() > 0.f && FMath::FRand() < HurtCryChance)
	{
		PlayCue(CueFor(2), 0.8f);
	}
}

void APlaceholderCreature::OnNoticed()
{
	PlayCue(CueFor(0));
}

void APlaceholderCreature::PlayEntrance()
{
	// Up out of the ground, the dirt thrown up round it, its brain held until it's out.
	EntranceLeft = EntranceLength;
	HoldBrain(EntranceLength);
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		Effects.SpawnDirt(GetMesh()->GetComponentLocation(), FVector::UpVector, 0.6f);
	}
}

void APlaceholderCreature::OnDied()
{
	DeadTime = 0.f;
	SetKeepingDistance(false);
	PlayCue(CueFor(3));
	LooterSound::PlayAt(this, LooterSoundCue::SlimeSplat, GetActorLocation(), 0.9f, LooterSoundRules::PitchForSize(GetSizeScale()));
	// A burst of wet clay in its colour, thrown along the killing shot, and a puff of dust where it slumps.
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		const float Size = (Definition ? FMath::Max(Definition->Length, Definition->Width) : 90.f) * GetSizeScale();
		const FLinearColor Tint = Definition ? Definition->ClayColor : FLinearColor(0.42f, 0.24f, 0.14f);
		Effects.SpawnDeathBurst(EDeathBurst::Gel, GetMesh()->Bounds.Origin, GetLastHit().Direction, Size, Tint);
		Effects.SpawnDirt(GetMesh()->GetComponentLocation(), FVector::UpVector, 0.4f);
	}
	// The kill's own layers on top (clods by the gun, its dust, its tier's flare) and its clay splat on the ground.
	CreatureKillLook::Play(*this, GetLastHit(), /*bSplat*/ true);
}

void APlaceholderCreature::OnRespawned()
{
	DeadTime = 0.f;
	Jolt = FVector2D::ZeroVector;
	bShotAttack = false;
	Keep = EPlaceholderKeep::None;
	SetKeepingDistance(false);
}
