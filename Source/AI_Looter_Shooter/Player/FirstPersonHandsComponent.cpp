#include "Player/FirstPersonHandsComponent.h"
#include "AI_Looter_Shooter.h"
#include "Embers/HeroDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/FirstPersonHandsFraming.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"

// UFirstPersonHandsComponent: the arms' mesh and where it sits, when the hands show, and each frame the gun read off as
// it is, the hands placed on it (FirstPersonHandsGrips, FirstPersonHandsMotion) and the arms posed to them.

namespace
{
	const FName MagazineSocket(TEXT("Magazine"));
	const FName RocketSocket(TEXT("Rocket"));

	TAutoConsoleVariable<int32> CVarHandsStandIn(
		TEXT("Looter.Hands.StandIn"),
		0,
		TEXT("1 shows the first-person hands on the template body's arms while SK_FP_Arms isn't imported (testing only)."));

	TAutoConsoleVariable<float> CVarHandsScale(
		TEXT("Looter.Hands.Scale"),
		0.f,
		TEXT("The first-person arms' size against the guns while tuning (0: the component's ArmsScale)."));

	TAutoConsoleVariable<int32> CVarGripDebug(
		TEXT("Looter.Hands.Grip.Debug"),
		0,
		TEXT("1 draws the handles the first-person hands close on (boxes), the trigger spot and every finger joint (development)."));

	/** A mesh the component names, if its package is in this build (SK_FP_Arms waits for its import; no warning meanwhile). */
	USkeletalMesh* LoadIfMade(const TSoftObjectPtr<USkeletalMesh>& Asset)
	{
		if (Asset.IsNull())
		{
			return nullptr;
		}
		if (USkeletalMesh* Loaded = Asset.Get())
		{
			return Loaded;
		}
		return FPackageName::DoesPackageExist(Asset.ToSoftObjectPath().GetLongPackageName()) ? Asset.LoadSynchronous() : nullptr;
	}

	/** A socket on any of the model's parts (the latest that has it), in the model's own space. */
	bool FindPartSocket(const UWeaponModelComponent& Model, FName Socket, FVector& OutLocation)
	{
		const TArray<TObjectPtr<UStaticMeshComponent>>& Parts = Model.GetParts();
		for (int32 Index = Parts.Num() - 1; Index >= 0; --Index)
		{
			const UStaticMeshComponent* Part = Parts[Index];
			if (Part && Part->DoesSocketExist(Socket))
			{
				OutLocation = Model.GetComponentTransform().InverseTransformPosition(Part->GetSocketLocation(Socket));
				return true;
			}
		}
		return false;
	}
}

UFirstPersonHandsComponent::UFirstPersonHandsComponent()
{
	// After everything that places the gun this frame: the locomotion's view model (TG_PrePhysics), a strike's or a throw's
	// pose on it (TG_DuringPhysics) and the gun's own reload parts (its actor, TG_PrePhysics).
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	// The bones change only when the hands are posed (which marks them dirty), never by themselves.
	VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	// Arms floating at the eye would throw a floating shadow on the world; the body's own shadow is the player's.
	SetCastShadow(false);
	bReceivesDecals = false;

	ArmsMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Art/Characters/SK_FP_Arms.SK_FP_Arms")));
	StandInMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
}

void UFirstPersonHandsComponent::BeginPlay()
{
	Super::BeginPlay();
	SetVisibility(false);
	bShowing = false;

	AActor* Owner = GetOwner();
	UCameraComponent* Eye = UPlayerViewComponent::FindFirstPersonCamera(Owner);
	if (!Eye)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: no first-person camera, so no first-person hands."), *GetName());
		SetComponentTickEnabled(false);
		return;
	}
	Camera = Eye;
	AttachToComponent(Eye, FAttachmentTransformRules::KeepRelativeTransform);
	if (!LoadArms())
	{
		UE_LOG(LogLooter, Log, TEXT("%s: no arms mesh with both arms' bones, so no first-person hands."), *GetName());
		SetComponentTickEnabled(false);
		return;
	}

	// Drawn like the gun in hand: the view model's own field of view and scale, never clipping a wall, only for its owner.
	SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	SetOnlyOwnerSee(true);

	View = Owner->FindComponentByClass<UPlayerViewComponent>();
	Weapons = Owner->FindComponentByClass<UWeaponManagerComponent>();
	Throw = UPlayerThrowComponent::Find(Owner);
	// Its tick group already follows the locomotion's; said here too, so a move of either keeps the order.
	if (UPlayerLocomotionComponent* Locomotion = Owner->FindComponentByClass<UPlayerLocomotionComponent>())
	{
		AddTickPrerequisiteComponent(Locomotion);
	}
}

bool UFirstPersonHandsComponent::LoadArms()
{
	// The played hero's own arms (the session's pick, or Looter.Hero's), at their size; Ellis's while theirs aren't in the
	// game (as UBodyFigureComponent::PickBody does for bodies), or where no hero is played (a test level's character).
	USkeletalMesh* Arms = nullptr;
	HeroScale = 0.f;
	if (const UHeroDefinition* Hero = USessionSubsystem::FindPlayedHero(this))
	{
		Arms = Hero->LoadFirstPersonArms();
		HeroScale = Arms ? Hero->FirstPersonArmsScale : 0.f;
		if (!Arms && Hero->Id != UHeroDefinition::DefaultId())
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s's first-person arms aren't in the game yet (%s); Ellis's stand in."), *GetName(),
				*Hero->Id.ToString(), *Hero->GetFirstPersonArmsPath().GetAssetName());
		}
	}
	if (!Arms)
	{
		Arms = LoadIfMade(ArmsMesh);
	}
	bStandIn = Arms == nullptr;
	if (!Arms)
	{
		Arms = LoadIfMade(StandInMesh);
	}
	if (!Arms || !Rig.Bind(Arms, bStandIn))
	{
		return false;
	}
	SetSkinnedAssetAndUpdate(Arms);
	// The arms touch nothing (the mesh's physics asset would give them bodies): they're drawn, nothing more.
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceOnCamera(GetWantedScale());
	return true;
}

void UFirstPersonHandsComponent::PlaceOnCamera(float Scale)
{
	// SK_FP_Arms is made with its root at the eye, facing ahead: sized about the eye. The stand-in, a whole body facing its
	// own +Y, is turned to face the camera's ahead and set so its shoulders sit under and just behind the eye.
	FTransform Placement(FQuat::Identity, FVector::ZeroVector, FVector(Scale));
	if (bStandIn)
	{
		const FQuat Facing = FRotationMatrix::MakeFromXZ(Rig.GetForward(), Rig.GetUp()).ToQuat().Inverse();
		Placement.SetRotation(Facing);
		Placement.SetTranslation(StandInShoulders - Facing.RotateVector(Rig.GetShoulderMiddle()) * Scale);
	}
	SetRelativeTransform(Placement);
}

float UFirstPersonHandsComponent::GetWantedScale() const
{
	const float Tuning = CVarHandsScale.GetValueOnGameThread();
	return FMath::Clamp(Tuning > 0.f ? Tuning : (HeroScale > 0.f ? HeroScale : ArmsScale), 0.5f, 1.5f);
}

void UFirstPersonHandsComponent::WearHero()
{
	// Only once begun on a camera (BeginPlay found one and attached the arms to it).
	if (!Camera.IsValid() || !HasBegunPlay())
	{
		return;
	}
	const USkeletalMesh* Before = GetArmsMesh();
	if (!LoadArms())
	{
		UE_LOG(LogLooter, Log, TEXT("%s: no arms mesh with both arms' bones for the hero played; the hands stay hidden."), *GetName());
		Show(false);
		SetComponentTickEnabled(false);
		return;
	}
	SetComponentTickEnabled(true);
	// Posed afresh on the next tick (the new mesh's bones), never shown in its rest pose meanwhile.
	bShowing = false;
	SetVisibility(false);
	if (Before != GetArmsMesh())
	{
		UE_LOG(LogLooter, Log, TEXT("%s: wearing %s at %.2f."), *GetName(), *GetNameSafe(GetArmsMesh()), GetWantedScale());
	}
}

const USkeletalMesh* UFirstPersonHandsComponent::GetArmsMesh() const
{
	return Cast<USkeletalMesh>(GetSkinnedAsset());
}

void UFirstPersonHandsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Posed first, so the mesh's own tick refreshes the bones from this frame's pose (once: it does whenever they're dirty).
	PoseHands(DeltaTime);
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UFirstPersonHandsComponent::UpdateHands(float DeltaTime)
{
	if (PoseHands(DeltaTime))
	{
		RefreshBoneTransforms();
	}
}

bool UFirstPersonHandsComponent::PoseHands(float DeltaTime)
{
	const float Dt = FMath::Clamp(DeltaTime, 0.f, 0.1f);
	FHandsGun Gun;
	if (!Rig.IsBound() || !FindGun(Dt, Gun))
	{
		Show(false);
		bHaveSupport = false;
		CatchUp = 0.f;
		SupportJob = EHandsSupportJob::Foregrip;
		return false;
	}

	const int32 Left = FirstPersonHands::Index(EFirstPersonHand::Left);
	const int32 Right = FirstPersonHands::Index(EFirstPersonHand::Right);
	const UWeaponManagerComponent* Manager = Weapons.Get();
	if (const AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr)
	{
		Gun.TriggerReady = NextTriggerReady(*Weapon, Dt);
	}
	Targets[Right] = FirstPersonHandsGrips::OnGrip(Gun);
	LastGun = Gun;
	Targets[Left] = ParkSupportHand(Gun, PlaceSupportHand(Gun, Dt), Dt);

	// The arms' size (a smaller hand holds the same spots: they're scaled into the mesh's space with it), then into the
	// mesh's own space (the stand-in is turned and set back on the camera) and posed.
	const float Scale = GetWantedScale();
	if (!FMath::IsNearlyEqual(static_cast<float>(GetRelativeScale3D().X), Scale, 1.e-3f))
	{
		PlaceOnCamera(Scale);
	}
	// The forearms framed last, from where the hands ended up (a reload's or a throw's spot too): low on a slant, elbows
	// out of the frame.
	FrameForearms(Scale);
	const FTransform CameraToMesh = GetRelativeTransform().Inverse();
	FHandsRigTargets RigTargets;
	RigTargets.Hands[Left] = Targets[Left].Moved(CameraToMesh);
	RigTargets.Hands[Right] = Targets[Right].Moved(CameraToMesh);
	RigTargets.DefaultForearm[Left] = CameraToMesh.TransformVectorNoScale(LeftForearm).GetSafeNormal();
	RigTargets.DefaultForearm[Right] = CameraToMesh.TransformVectorNoScale(RightForearm).GetSafeNormal();
	Rig.Pose(RigTargets, BoneSpaceTransforms, PosedSpace);
	MarkRefreshTransformDirty();
#if ENABLE_DRAW_DEBUG
	if (CVarGripDebug.GetValueOnGameThread() != 0)
	{
		DrawGripDebug();
	}
#endif
	// Shown only once posed, so no frame ever shows the mesh's rest pose.
	Show(true);
	return true;
}

bool UFirstPersonHandsComponent::FindGun(float DeltaTime, FHandsGun& OutGun)
{
	const APawn* Owner = Cast<APawn>(GetOwner());
	const UPlayerViewComponent* PlayerView = View.Get();
	const UWeaponManagerComponent* Manager = Weapons.Get();
	const UCameraComponent* Eye = Camera.Get();
	const AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
	// The hands are the first-person gun's: none in third person (the body's own hands hold it there) or without a gun
	// drawn on the camera.
	if (!Owner || !PlayerView || !Eye || !Weapon || !PlayerView->IsFirstPerson() || Manager->IsThirdPersonHold() || Weapon->IsHidden()
		|| Weapon->GetAttachParentActor() != Owner)
	{
		MenuOpenSeconds = 0.f;
		return false;
	}
	// A menu lowers the gun out of view (PlayerLocomotionViewModel.cpp): the hands go down with it, then go.
	const APlayerController* Player = Cast<APlayerController>(Owner->GetController());
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	MenuOpenSeconds = Hud && Hud->IsMenuOpen() ? MenuOpenSeconds + DeltaTime : 0.f;
	if (MenuOpenSeconds > MenuHideSeconds)
	{
		return false;
	}
	DescribeGun(*Weapon, Eye->GetComponentTransform(), OutGun);
	return true;
}

void UFirstPersonHandsComponent::DescribeGun(const AWeaponBase& Weapon, const FTransform& CameraWorld, FHandsGun& OutGun) const
{
	// The visible model, not the gun's root: a strike or a throw turns the model off its hold, and the hands go with it.
	const UWeaponModelComponent* Model = Weapon.FindComponentByClass<UWeaponModelComponent>();
	const bool bModel = Model && Model->IsAssembled();
	const FTransform GunWorld = bModel ? Model->GetComponentTransform() : Weapon.GetActorTransform();
	OutGun.ToCamera = GunWorld.GetRelativeTransform(CameraWorld);
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition.Get();
	OutGun.Kind = Definition ? Definition->Kind : EWeaponKind::None;
	// A gun not built from parts has no grip socket: the weapon manager lines such a gun's hold up on AttachGrip.
	const UWeaponManagerComponent* Manager = Weapons.Get();
	OutGun.Grip = bModel && !Model->GetGrip().IsZero() ? Model->GetGrip() : (Manager ? Manager->AttachGrip : FVector::ZeroVector);
	OutGun.Foregrip = bModel ? Model->GetForegrip() : Weapon.GetForegripPoint();
	// Drawn bigger than modeled (UWeaponModelComponent::ScaleFor): the sockets already are, the hands' own numbers off the
	// gun follow it.
	OutGun.Scale = Weapon.GetModelScale();
	OutGun.ReloadPart = Weapon.GetReloadPart();
	OutGun.ReloadProgress = Weapon.GetReloadProgress();
	OutGun.ReloadPlan = Weapon.GetReloadPlan();
	OutGun.PumpCycle = Weapon.GetPumpCycle();
	// The sniper's bolt, worked by the trigger hand between shots and in its reload (FirstPersonHandsGrips::OnGrip).
	OutGun.BoltOpen = Weapon.GetBoltOpen();
	OutGun.BoltHand = Weapon.GetBoltHand();
	if (OutGun.ReloadProgress < 0.f || !bModel)
	{
		return;
	}
	if (IsMagazineLike(OutGun.ReloadPart))
	{
		OutGun.bHasMagazineSeat = FindPartSocket(*Model, MagazineSocket, OutGun.MagazineSeat);
	}
	else if (OutGun.ReloadPart == EWeaponReloadPart::Rocket)
	{
		// The launcher's rocket seats in its barrel's mouth: the reload's hand works there.
		OutGun.bHasMagazineSeat = FindPartSocket(*Model, RocketSocket, OutGun.MagazineSeat);
	}
	else if (OutGun.ReloadPart == EWeaponReloadPart::Cylinder)
	{
		// The cylinder as the gun's model has swung it this frame (the gun ticks before the hands): its chambers' backs
		// ring its axis at its rear face.
		TArray<FVector> Rears;
		FVector Back;
		if (Model->GetChamberRears(Rears, Back) && !Rears.IsEmpty())
		{
			FVector Middle = FVector::ZeroVector;
			for (const FVector& Rear : Rears)
			{
				Middle += Rear;
			}
			OutGun.CylinderRear = GunWorld.InverseTransformPosition(Middle / Rears.Num());
			OutGun.CylinderAxis = -GunWorld.InverseTransformVectorNoScale(Back).GetSafeNormal(UE_SMALL_NUMBER, -FVector::ForwardVector);
			OutGun.bHasCylinder = true;
		}
	}
}

FHandPlacement UFirstPersonHandsComponent::PlaceSupportHand(const FHandsGun& Gun, float DeltaTime)
{
	// Not reloading, SupportInReload is the foregrip: riding the pump as it's racked between shots.
	const FHandPlacement OnGun = Gun.ReloadProgress < 0.f ? FirstPersonHandsMotion::SupportInReload(Gun) : FirstPersonHandsGrips::OnForegrip(Gun);
	const UPlayerThrowComponent* Thrower = Throw.Get();
	EHandsSupportJob Job = EHandsSupportJob::Foregrip;
	FHandPlacement Wanted = OnGun;
	if (Thrower && Thrower->IsThrowing())
	{
		Job = EHandsSupportJob::Throw;
		Wanted = FirstPersonHandsMotion::SupportInThrow(OnGun, Thrower->GetThrowTime());
	}
	else if (Gun.ReloadProgress >= 0.f)
	{
		Job = EHandsSupportJob::Reload;
		Wanted = FirstPersonHandsMotion::SupportInReload(Gun);
	}

	// Every job starts from the foregrip and ends on it, but one can be cut short: a strike or a throw stops a reload, a
	// swap ends one. The hand eases over from where it was on the gun instead of jumping.
	if (bHaveSupport && Job != SupportJob && SupportJob != EHandsSupportJob::Foregrip)
	{
		CatchUpFrom = SupportOnGun;
		CatchUp = 1.f;
	}
	SupportJob = Job;
	if (CatchUp > 0.f)
	{
		CatchUp = FMath::Max(CatchUp - DeltaTime / JobCatchUpSeconds, 0.f);
		Wanted = FHandPlacement::Blend(Wanted, CatchUpFrom.Moved(Gun.ToCamera), FMath::SmoothStep(0.f, 1.f, CatchUp));
	}
	SupportOnGun = Wanted.Moved(Gun.ToCamera.Inverse());
	bHaveSupport = true;
	return Wanted;
}

FHandPlacement UFirstPersonHandsComponent::ParkSupportHand(const FHandsGun& Gun, const FHandPlacement& OnGun, float DeltaTime)
{
	// Borderlands holds a pistol in the right hand alone: the left leaves the gun while it only holds, and comes back for
	// the work only it does (a reload, the jar), on PlaceSupportHand's job this frame.
	const float Wanted = SupportJob == EHandsSupportJob::Foregrip ? FPHandsFraming::OneHandShare(Gun.Kind) : 0.f;
	OneHandBlend = FMath::FInterpConstantTo(OneHandBlend, Wanted, DeltaTime, 1.f / FPHandsFraming::OneHandSeconds());
	if (OneHandBlend <= 0.f)
	{
		return OnGun;
	}
	return FHandPlacement::Blend(OnGun, FPHandsFraming::ParkedSupport(), FMath::SmoothStep(0.f, 1.f, OneHandBlend));
}

void UFirstPersonHandsComponent::FrameForearms(float Scale)
{
	const UCameraComponent* Eye = Camera.Get();
	if (!Eye || !FPHandsFraming::IsEnabled())
	{
		return;
	}
	// The frame the arms are drawn in: the view model's own field of view (UPlayerViewComponent::UpdateFieldOfView sets it).
	const float Degrees = Eye->bEnableFirstPersonFieldOfView ? Eye->FirstPersonFieldOfView : Eye->FieldOfView;
	const FPHandsFraming::FFrameView Frame = FPHandsFraming::FFrameView::FromFieldOfView(Degrees, FPHandsFraming::FrameAspect());
	const EFirstPersonHand Hands[] = { EFirstPersonHand::Left, EFirstPersonHand::Right };
	for (const EFirstPersonHand Hand : Hands)
	{
		FHandPlacement& Target = Targets[FirstPersonHands::Index(Hand)];
		const FVector TableWay = Target.Forearm.IsNearlyZero() ? (Hand == EFirstPersonHand::Left ? LeftForearm : RightForearm) : Target.Forearm;
		const float Length = Rig.GetForearmLength(Hand) * Scale;
		Target.Forearm = FPHandsFraming::FrameForearm(Hand, FPHandsFraming::WristOf(Target, Scale), TableWay, Length, Frame,
			FPHandsFraming::FForearmRules::For(Hand));
	}
}

void UFirstPersonHandsComponent::Show(bool bShow)
{
	// The template body's arms stand in only for testing: until SK_FP_Arms is approved and imported, the game shows no
	// placeholder art unless Looter.Hands.StandIn asks. Hidden in game, so the hands still pose and IsVisible still says
	// whether they would show.
	SetHiddenInGame(bStandIn && CVarHandsStandIn.GetValueOnGameThread() == 0);
	if (bShowing != bShow)
	{
		bShowing = bShow;
		SetVisibility(bShow);
	}
}

FVector UFirstPersonHandsComponent::GetHoldPoint(EFirstPersonHand Hand) const
{
	const int32 Bone = Rig.GetHandBone(Hand);
	return Rig.IsBound() && Bone != INDEX_NONE ? GetBoneTransform(Bone).TransformPosition(Rig.GetHoldLocal(Hand)) : FVector::ZeroVector;
}

FVector UFirstPersonHandsComponent::GetSeatedHoldPoint(EFirstPersonHand Hand) const
{
	// The seat works in the arms mesh's space (the hand's own size), as the rig does.
	const UCameraComponent* Eye = Camera.Get();
	if (!Eye)
	{
		return FVector::ZeroVector;
	}
	const FTransform MeshToCamera = GetRelativeTransform();
	const FHandPlacement Seated = Rig.Seated(Targets[FirstPersonHands::Index(Hand)].Moved(MeshToCamera.Inverse()));
	return Eye->GetComponentTransform().TransformPosition(MeshToCamera.TransformPosition(Seated.Hold));
}

FVector UFirstPersonHandsComponent::GetTargetHoldPoint(EFirstPersonHand Hand) const
{
	const UCameraComponent* Eye = Camera.Get();
	return Eye ? Eye->GetComponentTransform().TransformPosition(Targets[FirstPersonHands::Index(Hand)].Hold) : FVector::ZeroVector;
}
