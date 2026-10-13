// ADeadMinerCreature: construction (its numbers, its model and tints, its parts: the hat, the lamp with its light, glow and
// hit zone, the pick, the pail), its frame, hits, death and return. DeadMinerCreatureLamp.cpp has the lamp,
// DeadMinerCreatureSwing.cpp the pick's swing and the reel, DeadMinerCreatureWork.cpp its work at the walls,
// DeadMinerCreaturePose.cpp its arms and the pick in its fist.

#include "Creatures/DeadMinerCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Player/PlayerSize.h"
#include "Weapons/WeaponFX.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "ReferenceSkeleton.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const ADeadMinerCreature::ModelPath = TEXT("/Game/Art/Creatures/SK_DeadMiner.SK_DeadMiner");
const TCHAR* const ADeadMinerCreature::HatPath = TEXT("/Game/Art/Creatures/SM_DeadMinerHat.SM_DeadMinerHat");
const TCHAR* const ADeadMinerCreature::LampPath = TEXT("/Game/Art/Creatures/SM_DeadMinerLamp.SM_DeadMinerLamp");
const TCHAR* const ADeadMinerCreature::PickPath = TEXT("/Game/Art/Creatures/SM_DeadMinerPick.SM_DeadMinerPick");
const TCHAR* const ADeadMinerCreature::PailPath = TEXT("/Game/Art/Creatures/SM_DeadMinerPail.SM_DeadMinerPail");
const TCHAR* const ADeadMinerCreature::ClothesPaths[3] = {
	TEXT("/Game/Art/Materials/MI_DeadMiner_A.MI_DeadMiner_A"),
	TEXT("/Game/Art/Materials/MI_DeadMiner_B.MI_DeadMiner_B"),
	TEXT("/Game/Art/Materials/MI_DeadMiner_C.MI_DeadMiner_C"),
};
const FName ADeadMinerCreature::LampSocket(TEXT("Lamp"));
const FName ADeadMinerCreature::LightSocket(TEXT("Light"));
const FName ADeadMinerCreature::BarkKind(TEXT("DeadMiner"));

namespace
{
	/** The gunfire's additive glow on the engine's plane (R, G, B, strength, shape: 1 round), as the graves' lanterns burn. */
	const TCHAR* const GlowPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* const PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;

	/**
	 * An asset made after this code (the miners' models and tints are with the user as concepts), found only once it
	 * exists: until then the class works, and its tests run, without it.
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

	/** A part worn on a bone: no collision of its own, and a ghost's things cast no shadow. */
	UStaticMeshComponent* MakePart(AActor& Owner, const TCHAR* Name, USceneComponent* Body, FName Bone, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = Owner.CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Body, Bone);
		Part->SetStaticMesh(Mesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCastShadow(false);
		return Part;
	}

	/**
	 * A bone's rest turn taken back off: a part on that bone turned by it sits in the model's own axes at rest, as the
	 * Unpaid's hat does. False without the bone.
	 */
	bool RestTurnBack(const USkeletalMesh* Model, FName Bone, FQuat& OutTurn)
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

ADeadMinerCreature::ADeadMinerCreature()
{
	DisplayName = NSLOCTEXT("LooterDeadMiner", "DeadMinerName", "Dead Miner");
	// A shade tougher and heavier than the Unpaid (160 and 8): men who swung a pick for a living.
	Health->MaxHealth = 180.f;
	AttackDamage = 9.f;
	XPReward = 11;
	// A miner's tread: a steady drift a little slower than the Unpaid's, and no flying lunge. Tuned against the full-size
	// player's walk and scaled with the player's speed and pace, as every creature's chase is.
	WalkSpeed = 140.f;
	ChaseSpeed = 430.f * LooterPlayerSize::MoveScale;
	// It works where it was put: no wandering off from the wall it swings at.
	WanderRadius = 0.f;
	// It closes in to the pick's reach and plants itself (the brain stands it at three quarters of this), then heaves the
	// pick through a long, readable wind-up and leaves it in the floor a moment after.
	AttackRange = 230.f;
	AttackWindup = 0.8f;
	AttackRecovery = 0.9f;
	AttackCooldown = 1.2f;
	SwingWindup = AttackWindup;
	SwingRecovery = AttackRecovery;
	// The dead of a mine walk its drifts: they step up only when they fall well behind or are stuck.
	PhaseStep.FarBehind = 1800.f;
	PhaseStep.Cooldown = 5.f;
	Voice->BarkKind = BarkKind;

	// Its own body once the art drops it in (the Unpaid's skeleton and rig), else the Unpaid's. Its tints: the model's own
	// is A, and B and C go on the body's slot; on the Unpaid's body all three do, never the Unpaid's own clothes.
	USkeletalMeshComponent* Body = GetMesh();
	static USkeletalMesh* const MinerModel = FindIfMade<USkeletalMesh>(ModelPath);
	if (MinerModel)
	{
		Body->SetSkeletalMeshAsset(MinerModel);
	}
	static UMaterialInterface* const TintModels[] = {
		FindIfMade<UMaterialInterface>(ClothesPaths[0]),
		FindIfMade<UMaterialInterface>(ClothesPaths[1]),
		FindIfMade<UMaterialInterface>(ClothesPaths[2]),
	};
	TArray<TObjectPtr<UMaterialInterface>> Tints;
	for (int32 Index = MinerModel ? 1 : 0; Index < static_cast<int32>(UE_ARRAY_COUNT(TintModels)); ++Index)
	{
		if (UMaterialInterface* Tint = TintModels[Index])
		{
			Tints.Add(Tint);
		}
	}
	if (Tints.Num() > 0)
	{
		OtherClothes = Tints;
		bMinerTints = MinerModel == nullptr;
	}
	else if (MinerModel)
	{
		OtherClothes.Reset();
	}

	// The miner's hat in place of the Unpaid's (the Unpaid's stays until it's made).
	static UStaticMesh* const MinerHat = FindIfMade<UStaticMesh>(HatPath);
	if (MinerHat)
	{
		GetHat()->SetStaticMesh(MinerHat);
	}

	const USkeletalMesh* Model = Body->GetSkeletalMeshAsset();
	static UStaticMesh* const PickModel = FindIfMade<UStaticMesh>(PickPath);
	static UStaticMesh* const PailModel = FindIfMade<UStaticMesh>(PailPath);
	static UStaticMesh* const LampModel = FindIfMade<UStaticMesh>(LampPath);
	Pick = MakePart(*this, TEXT("Pick"), Body, Rig.RightArm.Hand, PickModel);
	Pail = MakePart(*this, TEXT("Pail"), Body, Rig.LeftArm.Hand, PailModel);
	Pail->SetVisibility(false);

	// The lamp on the hat bone, as the model rests, so what reads the defaults (the Ledger's stand) sees it on its brow;
	// play puts it on the hat's socket when the hat has one (PlaceLamp).
	Lamp = MakePart(*this, TEXT("Lamp"), Body, Rig.Hat, LampModel);
	FQuat HatTurn;
	if (RestTurnBack(Model, Rig.Hat, HatTurn))
	{
		Lamp->SetRelativeLocationAndRotation(HatTurn.RotateVector(LampOnHat), HatTurn);
	}
	else
	{
		Lamp->SetupAttachment(Body);
		Lamp->SetRelativeLocation(LampPointWithoutRig);
	}

	// Its light: a lamp's, small and warm, never a shadow, and only for the few nearest the player (the budget turns it on).
	LampLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LampLight"));
	LampLight->SetupAttachment(Lamp);
	LampLight->SetRelativeLocation(LensOnLamp);
	LampLight->SetMobility(EComponentMobility::Movable);
	LampLight->SetCastShadows(false);
	LampLight->SetIntensityUnits(ELightUnits::Candelas);
	LampLight->SetIntensity(Tuning.LampCandela);
	LampLight->SetAttenuationRadius(Tuning.LampRadius);
	LampLight->SetLightColor(DeadMinerRules::LampColor());
	LampLight->SetMaxDrawDistance(Tuning.LampLightRange);
	LampLight->SetVisibility(false);

	// Its glow at the lens: three crossed cards of the gunfire's glow, so it reads from every side (one instanced draw).
	static UStaticMesh* const Plane = FindIfMade<UStaticMesh>(PlanePath);
	static UMaterialInterface* const Glow = FindIfMade<UMaterialInterface>(GlowPath);
	LampGlow = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LampGlow"));
	LampGlow->SetupAttachment(Lamp);
	LampGlow->SetRelativeLocation(LensOnLamp);
	LampGlow->SetStaticMesh(Plane);
	LampGlow->SetMaterial(0, Glow);
	LampGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LampGlow->SetGenerateOverlapEvents(false);
	LampGlow->SetCanEverAffectNavigation(false);
	LampGlow->SetCastShadow(false);
	LampGlow->bReceivesDecals = false;
	LampGlow->SetNumCustomDataFloats(GlowFloats);

	// The lamp's weak spot: a small ball round the lens, seen by bullets (and sight) only, as the body's hit zones are.
	LampHitZone = CreateDefaultSubobject<USphereComponent>(TEXT("LampHitZone"));
	LampHitZone->SetupAttachment(Lamp);
	LampHitZone->SetRelativeLocation(LensOnLamp);
	LampHitZone->InitSphereRadius(Tuning.LampHitRadius);
	LampHitZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LampHitZone->SetCollisionObjectType(ECC_WorldDynamic);
	LampHitZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	LampHitZone->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	LampHitZone->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	LampHitZone->SetGenerateOverlapEvents(false);
	LampHitZone->SetCanEverAffectNavigation(false);
}

void ADeadMinerCreature::BeginPlay()
{
	// Its things wear the body's look (its dissolve, its rank's ember), as the Bell-Ringer's bell does: listed before the
	// Unpaid's start applies it.
	LookParts.AddUnique(Pick);
	LookParts.AddUnique(Lamp);
	LookParts.AddUnique(Pail);
	if (bMinerTints && Clothes < 0 && OtherClothes.Num() > 0)
	{
		Clothes = FMath::RandRange(1, OtherClothes.Num());
	}
	// Its rank (the swing's timing: OnRankChanged), level, size, clothes and rig first.
	Super::BeginPlay();

	// On the fists the configured rig names (DefaultGame.ini may rename them), as the Unpaid puts its hat on.
	Pick->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Rig.RightArm.Hand);
	Pail->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Rig.LeftArm.Hand);
	PlaceLamp();
	BuildLampGlow();
	SetupArmLayer();
	if (bRigReady)
	{
		UpdatePick();
	}
	ApplyLampHitZone();
	ApplyLamp(0.f);
	RefreshLampLight();
	// Out of step with its neighbours: each looks for its wall a moment after the others.
	NextWorkLook = FMath::FRandRange(0.f, 0.5f);
	LampBudgetClock = FMath::FRandRange(0.f, 0.4f);
}

void ADeadMinerCreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetAct(EAct::None);
	Super::EndPlay(EndPlayReason);
}

void ADeadMinerCreature::SetAct(EAct NewAct)
{
	if (Act == EAct::Pail || NewAct == EAct::Pail)
	{
		Pail->SetVisibility(NewAct == EAct::Pail && Pail->GetStaticMesh() != nullptr);
	}
	Act = NewAct;
	ActTime = 0.f;
}

// ---------------------------------------------------------------------------
// The frame
// ---------------------------------------------------------------------------

void ADeadMinerCreature::Tick(float DeltaSeconds)
{
	// The Unpaid's frame first: its brain (the chase, the swing's wind-up and blow), its phase-step, and its body, with
	// the miner's arms laid over it (LayerPose).
	Super::Tick(DeltaSeconds);

	ActTime += DeltaSeconds;
	if (!IsDead())
	{
		TickReel(DeltaSeconds);
		TickWork(DeltaSeconds);
	}
	// The pick in its fist as this frame posed it.
	if (bRigReady && !IsPoseFrozen())
	{
		UpdatePick();
	}
	// The lamp flares through the wind-up and snaps back at the blow.
	const float WantedFlare = bLampLit ? WantedLampFlare() : 0.f;
	LampFlare = WantedFlare >= LampFlare ? WantedFlare : FMath::FInterpTo(LampFlare, WantedFlare, DeltaSeconds, 10.f);
	TickLampBudget(DeltaSeconds);
	ApplyLamp(DeltaSeconds);
	ApplyLampHitZone();
}

// ---------------------------------------------------------------------------
// Hits, death and coming back
// ---------------------------------------------------------------------------

bool ADeadMinerCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	// The lamp is its weak spot, lit or out: any shot on it is critical, besides the Unpaid's coal.
	if (bLampIsWeakSpot && Hit.GetComponent() && Hit.GetComponent() == LampHitZone.Get())
	{
		return true;
	}
	return Super::IsCriticalSpot(Hit);
}

void ADeadMinerCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	// The Unpaid's flinch first (its arms flung, its coal flaring on a crit); then the lamp, if that's where it was hit.
	Super::OnHurt(bCritical, HitLocation);
	if (!IsDead() && bLampIsWeakSpot && IsLampHit(HitLocation))
	{
		LampStruck(HitLocation);
	}
}

void ADeadMinerCreature::OnDied()
{
	Super::OnDied();
	SetAct(EAct::None);
	bSwingBroken = false;
	// Its pick falls with it, clattering on the stone; its light goes out as it falls (the glow fades with the body).
	LooterSound::PlayAt(this, LooterSoundCue::DeadMiner::Drop, GetFeet(), 0.8f, Voice->GetPitch());
	RefreshLampLight();
}

void ADeadMinerCreature::OnRespawned()
{
	Super::OnRespawned();
	// A new life: its lamp lit again, back at work.
	SetAct(EAct::None);
	bSwingBroken = false;
	bLampLit = true;
	LampFlare = 0.f;
	WorkTime = 0.f;
	WorkBlows = 0;
	NextWorkLook = 0.f;
	bArmsStarted = false;
	LayerWeight = 1.f;
	NowBow = NowKneel = NowHeadPitch = 0.f;
	bLampZoneWanted = true;
	ShownGlow = -1.f;
	ShownLight = -1.f;
	ApplyLampHitZone();
	ApplyLamp(0.f);
	RefreshLampLight();
}

void ADeadMinerCreature::OnRankChanged()
{
	// The Unpaid's: its coal's color, and a Restless one's quicker wind-up (a share of the class's).
	Super::OnRankChanged();
	// Its swing's own timing at its rank, which a reel (or the Foreman's other moves) borrows and gives back.
	SwingWindup = AttackWindup;
	if (!bSwingTimingCaptured)
	{
		bSwingTimingCaptured = true;
		SwingRecovery = AttackRecovery;
	}
}

void ADeadMinerCreature::SetHitVolumesEnabled(bool bEnabled)
{
	Super::SetHitVolumesEnabled(bEnabled);
	bLampZoneWanted = bEnabled;
	ApplyLampHitZone();
}

// ---------------------------------------------------------------------------
// Where things are
// ---------------------------------------------------------------------------

FVector ADeadMinerCreature::GetFeet() const
{
	return GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

FVector ADeadMinerCreature::GroundUnder(const FVector& Where) const
{
	// Its own level of ground: never the floor of a gallery under it or the roof over it.
	const float Scale = GetSizeScale();
	FVector Ground;
	return FindGround(Where, 150.f * Scale, 300.f * Scale, Ground) ? Ground : Where;
}

void ADeadMinerCreature::KickDirt(const FVector& Where, float Strength) const
{
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnDirt(Where, FVector::UpVector, Strength);
	}
}
