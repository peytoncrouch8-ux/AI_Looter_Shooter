// AStoneSentinelCreature: construction (its numbers, its model or the stand-in, its stone), its frame, its plinth, the head
// crit, knock-proofing. StoneSentinelCreatureHurt.cpp has its rank, hits, its open window, death and return,
// StoneSentinelCreatureWake.cpp its sleep and waking, StoneSentinelCreatureMoves.cpp its choice of attack, ...Slam.cpp the
// slam and its tread, ...Shock.cpp the shockwave, ...Boulder.cpp the thrown rock, StoneSentinelCreaturePose.cpp its pose,
// StoneSentinelCreatureChips.cpp the stone pieces and grit it sheds and its crumbling.

#include "Creatures/StoneSentinelCreature.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Creatures/FightTuning.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GroundCrack.h"
#include "Creatures/StoneSentinelBoulder.h"
#include "Creatures/StoneSentinelShockwave.h"
#include "Elements/ElementBody.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const AStoneSentinelCreature::ModelPath = TEXT("/Game/Art/Creatures/SK_StoneSentinel.SK_StoneSentinel");
const TCHAR* const AStoneSentinelCreature::PlinthPath = TEXT("/Game/Art/Creatures/SM_StoneSentinelPlinth.SM_StoneSentinelPlinth");
// The model's attach points are small bones (under head, hand_l, hand_r), not sockets: the mesh's socket lookups find bones too.
const FName AStoneSentinelCreature::HeadSocket(TEXT("SOCKET_Head"));
const FName AStoneSentinelCreature::FistLSocket(TEXT("SOCKET_FistL"));
const FName AStoneSentinelCreature::FistRSocket(TEXT("SOCKET_FistR"));
const FName AStoneSentinelCreature::PlinthTag(TEXT("SentinelPlinth"));

namespace
{
	/** Its capsule (cm): a 2.4 m mourner, a little broad in its robes. */
	constexpr float CapsuleRadius = 48.f;
	constexpr float CapsuleHalfHeight = 118.f;

	/** The stand-in: a granite post 80 cm across and 2 m tall, a ball for a head (the engine's shapes are 100 cm). */
	const FVector StandInBodyScale(0.8f, 0.8f, 2.f);
	constexpr float StandInBodyMiddle = -18.f;
	constexpr float StandInHeadSize = 0.36f;
	constexpr float StandInHeadMiddle = 98.f;

	/** The plinth's stand-in: a granite block 110 cm square and 55 cm high. */
	const FVector PlinthStandInScale(1.1f, 1.1f, 0.55f);

	/** An asset made after this code (lane E's model), found only once it exists: until then the class works without it. */
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

	/** Hit zones seen only by weapon and visibility traces, as every creature's: pawns walk through, shots stop. */
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

	UStaticMeshComponent* MakeStandInPart(AActor& Owner, const TCHAR* Name, USceneComponent* Parent)
	{
		UStaticMeshComponent* Part = Owner.CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		return Part;
	}
}

AStoneSentinelCreature::AStoneSentinelCreature()
{
	DisplayName = NSLOCTEXT("LooterStoneSentinel", "Name", "Stone Sentinel");
	// Stone (the elements' bodies): weak to Wardbreak and Thunder, resists Cinder, Rot does nothing. Never Touched: whatever
	// woke it isn't an element.
	ElementBody = EElementBody::Stone;
	bMayBeTouched = false;
	// Six dead miners' worth of stone (twice its first health, the user's "harder to kill", 2026-10-31), and a heavy blow.
	Health->MaxHealth = StoneSentinelRules::MaxHealth();
	AttackDamage = StoneSentinelRules::SlamDamage;
	XPReward = StoneSentinelRules::XPReward;
	// Slow: a walk you can always step away from. Its chase is what's felt after the fight's own chase scale (1.3 by default;
	// the cvar can move it, the felt speed then with it).
	WalkSpeed = StoneSentinelRules::WalkSpeed;
	ChaseSpeed = StoneSentinelRules::ChaseSpeedFor(StoneSentinelRules::FeltChaseSpeed, FFightTuning().ChaseScale);
	// It stands where it was put: no strolling off its plinth.
	WanderRadius = 0.f;
	AttackRange = StoneSentinelRules::AttackRange;
	AttackWindup = Tuning.SlamWindup;
	AttackRecovery = Tuning.SlamRecovery;
	AttackCooldown = Tuning.SlamCooldown;
	// It crumbles at once (CrumbleSeconds); the pieces lie a while, then the area may raise it again on its plinth.
	CorpseTime = 3.f;
	HealthBarHeight = 150.f;
	// The head and its attach bone under it (a shot's nearest bone may be either).
	CriticalSpotBones.Add(TEXT("head"));
	CriticalSpotBones.Add(HeadSocket);

	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	// Stone: slow to get going, stops dead, turns heavily.
	Movement->MaxAcceleration = 700.f;
	Movement->BrakingDecelerationWalking = 2400.f;
	Movement->GroundFriction = 8.f;
	Movement->RotationRate = FRotator(0.f, 140.f, 0.f);

	static USkeletalMesh* const Model = FindIfMade<USkeletalMesh>(ModelPath);
	static UStaticMesh* const PlinthModel = FindIfMade<UStaticMesh>(PlinthPath);
	PlinthMesh = PlinthModel;

	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMeshAsset(Model);
	// The model's origin is the ground under it (its armature's): the foot of the capsule. It faces +X (Art/README.md).
	Body->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleHalfHeight));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UCreaturePoseAnimInstance::StaticClass());
	// The hit zones follow the bones even while it's off screen; posed after this actor works out the frame's pose.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->PrimaryComponentTick.TickGroup = TG_PostPhysics;
	// Its physics asset's bodies are the hit zones.
	MakeHitZone(Body);

	// The stand-in until the model is imported: a granite post and a ball for its head, shot like the real one.
	StandInBody = MakeStandInPart(*this, TEXT("StandInBody"), GetCapsuleComponent());
	StandInHead = MakeStandInPart(*this, TEXT("StandInHead"), GetCapsuleComponent());
	if (!Model)
	{
		static UStaticMesh* const Post = FindIfMade<UStaticMesh>(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		static UStaticMesh* const Ball = FindIfMade<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		StandInBody->SetStaticMesh(Post);
		StandInBody->SetRelativeLocation(FVector(0.f, 0.f, StandInBodyMiddle));
		StandInBody->SetRelativeScale3D(StandInBodyScale);
		StandInHead->SetStaticMesh(Ball);
		StandInHead->SetRelativeLocation(FVector(0.f, 0.f, StandInHeadMiddle));
		StandInHead->SetRelativeScale3D(FVector(StandInHeadSize));
		MakeHitZone(StandInBody);
		MakeHitZone(StandInHead);
	}
	else
	{
		StandInBody->SetVisibility(false);
		StandInHead->SetVisibility(false);
	}
}

void AStoneSentinelCreature::BeginPlay()
{
	// A statue from the first frame: held back before anything can call it into a fight, its tag off.
	Phase = EStoneSentinelPhase::Dormant;
	SetPassive(true);
	bShowsHealthTag = false;
	// Its plinth under it first, so its start settles it on top and takes that as home.
	RaisePlinth();
	// Its rank (OnRankChanged: its traits kept for later, none shown), level and size, on the ground (its plinth).
	Super::BeginPlay();

	bStandIn = GetMesh()->GetSkeletalMeshAsset() == nullptr;
	if (bStandIn)
	{
		static bool bSaid = false;
		if (!bSaid)
		{
			bSaid = true;
			UE_LOG(LogLooter, Warning, TEXT("Stone Sentinel: %s isn't imported (lane E's StoneSentinel.py): a granite stand-in takes its place."),
				ModelPath);
		}
		SetupStandIn();
	}
	else if (!SetupRig())
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: its model has none of the sentinel's bones: it stands unposed."), *GetName());
	}
	SetupChips();
	ApplyStoneDefense();
	// Each in its own time, so a row of them doesn't shift and look in step.
	Clock = FMath::FRandRange(0.f, 10.f);
	LookClock = FMath::FRandRange(0.f, 0.2f);
	BecomeStatue();
}

void AStoneSentinelCreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The plinth it raised goes with it when it's removed (a runtime one's death, its level unloading), so no empty block is
	// left tagged Obstacle where a spawner may want to put the next one; one it found standing (the level's) stays. A world
	// shutting down takes everything anyway.
	AStaticMeshActor* Stand = Plinth.Get();
	if (Stand && bOwnsPlinth && !Stand->IsActorBeingDestroyed()
		&& (EndPlayReason == EEndPlayReason::Destroyed || EndPlayReason == EEndPlayReason::RemovedFromWorld))
	{
		Stand->Destroy();
	}
	Plinth.Reset();
	Chips.Reset();
	// A rock still in its grip goes with it (one in flight lands by itself); the shockwave's tell closes.
	if (AStoneSentinelBoulder* Rock = HeldBoulder.Get(); Rock && Rock->IsHeld() && !Rock->IsActorBeingDestroyed())
	{
		Rock->Destroy();
	}
	HeldBoulder.Reset();
	// Its kept rock and wave go with it when they lie idle; one still out finishes, finds its sentinel gone and goes too.
	if (AStoneSentinelBoulder* Rock = KeptBoulder.Get(); Rock && Rock->IsIdle() && !Rock->IsActorBeingDestroyed())
	{
		Rock->Destroy();
	}
	if (AStoneSentinelShockwave* Wave = KeptWave.Get(); Wave && Wave->IsIdle() && !Wave->IsActorBeingDestroyed())
	{
		Wave->Destroy();
	}
	KeptBoulder.Reset();
	KeptWave.Reset();
	EndShockTell();
	Super::EndPlay(EndPlayReason);
}

void AStoneSentinelCreature::SetupStandIn()
{
	UMaterialInterface* Look = GravemotherAssets::StandInLook(this, GravemotherAssets::GraniteColor());
	for (UStaticMeshComponent* Part : { StandInBody.Get(), StandInHead.Get() })
	{
		if (Part && Part->GetStaticMesh())
		{
			Part->SetMaterial(0, Look);
			Part->SetVisibility(true);
		}
	}
}

// ---------------------------------------------------------------------------
// The frame
// ---------------------------------------------------------------------------

void AStoneSentinelCreature::Tick(float DeltaSeconds)
{
	// The base's frame first: its brain (held while it wakes), the chase, the attack's wind-up and blow, its tag.
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	PhaseTime += DeltaSeconds;
	Jolt *= FMath::Exp(-10.f * DeltaSeconds);
	StayPut();
	// Its moves' clocks and the beats inside a wind-up (the rock torn up, the tell's tremor), then its open window.
	TickMoves(DeltaSeconds);
	TickExposed(DeltaSeconds);
	// Its tier's defense is set again whenever its stats are (a level change): its stone goes back on top (less while open).
	ApplyStoneDefense();

	switch (Phase)
	{
	case EStoneSentinelPhase::Dormant: TickDormant(DeltaSeconds); break;
	case EStoneSentinelPhase::Waking: TickWaking(DeltaSeconds); break;
	case EStoneSentinelPhase::Awake: TickAwake(DeltaSeconds); break;
	case EStoneSentinelPhase::Settling: TickSettling(DeltaSeconds); break;
	case EStoneSentinelPhase::Crumbling: TickCrumbling(DeltaSeconds); break;
	default: break;
	}
	TickGrit(DeltaSeconds);
	TickChips(DeltaSeconds);
	// Far away out of view it goes on unposed; its pose is worked out afresh from its state when it's seen again.
	if (!IsPoseFrozen())
	{
		AnimateBody(DeltaSeconds);
	}
}

// ---------------------------------------------------------------------------
// Stone: its defense, the head, nothing moves it
// ---------------------------------------------------------------------------

void AStoneSentinelCreature::ApplyStoneDefense()
{
	const float Wanted = StoneSentinelRules::CombinedDefense(StoneSentinelRules::StoneDefenseNow(IsExposed(), Tuning),
		UCreatureRankSettings::Get(GetRank()).Defense);
	if (!FMath::IsNearlyEqual(Health->GetDefense(), Wanted))
	{
		Health->SetDefense(Wanted);
	}
}

bool AStoneSentinelCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	const UPrimitiveComponent* Part = Hit.GetComponent();
	if (Part && Part == StandInHead.Get())
	{
		return true;
	}
	if (Super::IsCriticalSpot(Hit))
	{
		return true;
	}
	// Open, the cracks in its back and shoulders are as weak as its head (the soul-light shows through them).
	if (StoneSentinelRules::IsCritBone(Hit.BoneName, IsExposed(), Tuning))
	{
		return true;
	}
	// Its head's hull may share a body with the hood: a hit near the head's socket counts as the head's.
	const USkeletalMeshComponent* Body = GetMesh();
	if (!bStandIn && Part == Body && Body->DoesSocketExist(HeadSocket))
	{
		const float Radius = HeadCritRadius * GetSizeScale();
		return FVector::DistSquared(FVector(Hit.ImpactPoint), Body->GetSocketLocation(HeadSocket)) <= FMath::Square(Radius);
	}
	return false;
}

void AStoneSentinelCreature::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{
	// Knock-proof: a ton of stone isn't thrown by a blast, a shotgun or a boot. (Its own moves never launch it.)
	UE_LOG(LogLooter, VeryVerbose, TEXT("%s shrugs off a launch of %.0f cm/s."), *GetName(), LaunchVelocity.Size());
}

void AStoneSentinelCreature::StayPut()
{
	if (IsDead())
	{
		return;
	}
	// A knock that began on it (the knock asked for no launch it could take) is undone at once: no tumble, no dizzy spell.
	// A daze where it stands (a ward breaking) is left: that's no throw.
	if (Knock && Knock->IsKnocked())
	{
		Knock->Recover();
	}
	// A shove (a heavy hit's push) is speed added straight on: clamp it out, so it never slides faster than it walks.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement && Movement->IsMovingOnGround())
	{
		const float Most = FMath::Max(Movement->MaxWalkSpeed, GetChaseSpeedNow()) * 1.05f;
		const FVector Flat(Movement->Velocity.X, Movement->Velocity.Y, 0.0);
		if (Flat.SizeSquared() > FMath::Square(Most))
		{
			const FVector Kept = Flat.GetSafeNormal() * Most;
			Movement->Velocity.X = Kept.X;
			Movement->Velocity.Y = Kept.Y;
		}
	}
}

void AStoneSentinelCreature::SetHitVolumesEnabled(bool bEnabled)
{
	const ECollisionEnabled::Type Zones = bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
	GetMesh()->SetCollisionEnabled(Zones);
	if (bStandIn)
	{
		StandInBody->SetCollisionEnabled(Zones);
		StandInHead->SetCollisionEnabled(Zones);
	}
}

// ---------------------------------------------------------------------------
// Its plinth
// ---------------------------------------------------------------------------

void AStoneSentinelCreature::RaisePlinth()
{
	UWorld* World = GetWorld();
	if (Plinth.IsValid() || !World)
	{
		return;
	}
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Middle = GetActorLocation();
	// A sentinel's plinth already under it (the level's, or one left standing): it stands on that one, never a second.
	FHitResult Under;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SentinelPlinth"), this);
	if (World->LineTraceSingleByObjectType(Under, Middle + FVector(0.0, 0.0, 60.0), Middle - FVector(0.0, 0.0, HalfHeight + 400.f),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		AStaticMeshActor* Found = Cast<AStaticMeshActor>(Under.GetActor());
		if (Found && Found->ActorHasTag(PlinthTag))
		{
			Plinth = Found;
			bOwnsPlinth = false;
			return;
		}
	}
	if (!bBringsPlinth)
	{
		return;
	}
	FVector Ground;
	if (!FindGround(Middle, 60.f, HalfHeight + 400.f, Ground))
	{
		Ground = Middle - FVector(0.0, 0.0, HalfHeight);
	}
	// Its own block once made (its pivot at its foot, nothing buried: lane E's); a granite block until then. Placed by its
	// bounds' foot either way.
	UStaticMesh* BlockMesh = PlinthMesh ? PlinthMesh.Get() : GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Cube);
	if (!BlockMesh)
	{
		return;
	}
	const FVector Scale = PlinthMesh ? FVector::OneVector : PlinthStandInScale;
	const FBoxSphereBounds Bounds = BlockMesh->GetBounds();
	const float Bottom = static_cast<float>(Bounds.Origin.Z - Bounds.BoxExtent.Z) * static_cast<float>(Scale.Z);
	const float Height = static_cast<float>(Bounds.BoxExtent.Z) * 2.f * static_cast<float>(Scale.Z);
	const FTransform Place(FRotator(0.f, GetActorRotation().Yaw, 0.f), Ground - FVector(0.0, 0.0, Bottom), Scale);

	// An actor of its own (in its level, owned by it), so it stays when the sentinel walks off, dies or is hidden.
	AStaticMeshActor* Stand = World->SpawnActorDeferred<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Place, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Stand)
	{
		return;
	}
	UStaticMeshComponent* Block = Stand->GetStaticMeshComponent();
	Block->SetMobility(EComponentMobility::Movable);
	Block->SetStaticMesh(BlockMesh);
	if (!PlinthMesh)
	{
		Block->SetMaterial(0, GravemotherAssets::StandInLook(Stand, GravemotherAssets::GraniteColor() * 0.85f));
	}
	Block->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	// Solid stone to everything that reads tags: nothing spawns or blinks onto it; bullets and boots meet stone.
	Stand->Tags.Add(GravemotherAssets::ObstacleTag);
	Stand->Tags.Add(GravemotherAssets::SurfaceStoneTag);
	Stand->Tags.Add(PlinthTag);
	Stand->FinishSpawning(Place);
#if WITH_EDITOR
	Stand->SetActorLabel(FString::Printf(TEXT("%s_Plinth"), *GetName()));
#endif
	Plinth = Stand;
	bOwnsPlinth = true;
	// Up on top of it; the base's start settles its feet on the block.
	SetActorLocation(FVector(Middle.X, Middle.Y, Ground.Z + Height + HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
}

// ---------------------------------------------------------------------------
// Where things are
// ---------------------------------------------------------------------------

AStaticMeshActor* AStoneSentinelCreature::GetPlinth() const
{
	return Plinth.Get();
}

FVector AStoneSentinelCreature::GetFeet() const
{
	return GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

FVector AStoneSentinelCreature::GroundUnder(const FVector& Where) const
{
	const float Scale = GetSizeScale();
	FVector Ground;
	return FindGround(Where, 150.f * Scale, 300.f * Scale, Ground) ? Ground : Where;
}

FVector AStoneSentinelCreature::GetFistsPoint() const
{
	const USkeletalMeshComponent* Body = GetMesh();
	if (!bStandIn && Body->DoesSocketExist(FistLSocket) && Body->DoesSocketExist(FistRSocket))
	{
		return (Body->GetSocketLocation(FistLSocket) + Body->GetSocketLocation(FistRSocket)) * 0.5;
	}
	return GetFeet() + GetActorForwardVector() * (Tuning.SlamImpactAhead * GetSizeScale());
}

FVector AStoneSentinelCreature::BoneSpot(int32 BoneIndex, const FVector& Fallback) const
{
	const USkeletalMeshComponent* Body = GetMesh();
	if (!Rig.bReady || bStandIn || BoneIndex == INDEX_NONE || !Body)
	{
		return Fallback;
	}
	return Body->GetBoneLocation(Body->GetBoneName(BoneIndex));
}
