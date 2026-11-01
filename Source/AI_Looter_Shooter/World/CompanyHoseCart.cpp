// ACompanyHoseCart: a Company pump cart drinking the dead: its parts, its story, its valve and its tank. The stop itself
// (the spill, the blast, the crew's next wave, Vane's complaint) is CompanyHoseCartSpill.cpp's.

#include "World/CompanyHoseCart.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "World/LamplighterEffects.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "CompanyHoseCart"

const FName ACompanyHoseCart::HoseCartTag(TEXT("HoseCart"));
const FName ACompanyHoseCart::StoppedEvent(TEXT("HoseCart.Stopped"));
const TCHAR* const ACompanyHoseCart::ModelPath = TEXT("/Game/Art/Tripo/SM_CompanyHoseCart.SM_CompanyHoseCart");

namespace
{
	// The stand-in's look, one path each.
	const TCHAR* CartCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* CartCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* CartSpillPath = TEXT("/Game/Art/Props/SM_SlagSplatA.SM_SlagSplatA");
	const TCHAR* CartIronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");
	const TCHAR* CartWoodPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");
	const TCHAR* CartBrassPath = TEXT("/Game/Art/Materials/MI_BrassWorn.MI_BrassWorn");
	const TCHAR* CartSpillPaintPath = TEXT("/Game/Art/Materials/MI_PaintBlack.MI_PaintBlack");
	/** WORLD's hose rubber (build_landfall_materials.py); the black paint until it's made. */
	const TCHAR* CartHosePath = TEXT("/Game/Art/Landfall/Materials/MI_Landfall_HoseRubber.MI_Landfall_HoseRubber");

	/** Where things are on the cart (cm, its frame: X its handles' way, Z up from the ground). */
	const FVector ReelAt(-100.0, 0.0, 118.0);
	const FVector ValveAt(40.0, 0.0, 180.0);
	const FVector TankAt(-2.0, 0.0, 104.0);
	/** The hose: 14 cm across, swelling this much at a pump's stroke, this many strokes a second. */
	constexpr float HoseWidth = 14.f;
	constexpr float HosePulse = 0.09f;
	constexpr float PumpStrokesPerSecond = 0.8f;
	/** It sags to lie this high over the ground between reel and end, once they're this far apart (cm). */
	constexpr float HoseSagHeight = 7.f;
	constexpr float HoseSagReach = 150.f;
	/** The spill's size (its splat's scale), and how long it takes to spread (s). */
	const FVector SpillScale(3.2, 3.2, 1.0);
	constexpr float SpillSpreadSeconds = 1.2f;
	/** Its pulse and its pump are heard and seen within these (cm). */
	constexpr double PulseReach = 6000.0;
	constexpr double PumpReach = 2500.0;
	/** The valve wheel spins this fast while held (degrees a second). */
	constexpr float ValveSpin = 300.f;

	/** Bullets' trace channel (DefaultEngine.ini's Weapon). */
	constexpr ECollisionChannel CartWeaponChannel = ECC_GameTraceChannel2;

	/** An asset found only once it's in this checkout (the Tripo cart may not be yet). In a constructor. */
	template <typename T>
	T* CartAssetIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	/** A solid part (cover), its look and its place. */
	UStaticMeshComponent* MakeCartPart(AActor& Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh,
		UMaterialInterface* Look, const FVector& At, const FRotator& Turn, const FVector& Scale, bool bSolid)
	{
		UStaticMeshComponent* Part = Owner.CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		Part->SetMaterial(0, Look);
		Part->SetRelativeLocationAndRotation(At, Turn);
		Part->SetRelativeScale3D(Scale);
		Part->SetCanEverAffectNavigation(false);
		if (bSolid)
		{
			Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		}
		else
		{
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Part;
	}

	/** A cylinder part stretched from A to B (cart frame), Width across. */
	void StretchBetween(UStaticMeshComponent* Part, const FVector& A, const FVector& B, float Width)
	{
		if (!Part)
		{
			return;
		}
		const FVector Span = B - A;
		const double Length = FMath::Max(Span.Size(), 1.0);
		Part->SetRelativeLocationAndRotation((A + B) * 0.5, FRotationMatrix::MakeFromZ(Span.GetSafeNormal()).Rotator());
		Part->SetRelativeScale3D(FVector(Width / 100.0, Width / 100.0, Length / 100.0));
	}
}

ACompanyHoseCart::ACompanyHoseCart()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Tags.Add(HoseCartTag);
	Tags.Add(FName(TEXT("Obstacle")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(CartCubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CartCylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(CartIronPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(CartWoodPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Brass(CartBrassPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Paint(CartSpillPaintPath);
	static UStaticMesh* const SpillMesh = CartAssetIfMade<UStaticMesh>(CartSpillPath);
	static UMaterialInterface* const HoseRubber = CartAssetIfMade<UMaterialInterface>(CartHosePath);
	static UStaticMesh* const TripoCart = CartAssetIfMade<UStaticMesh>(ModelPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	UMaterialInterface* Hose = HoseRubber ? HoseRubber : Paint.Object.Get();
	Bed = MakeCartPart(*this, Root, TEXT("Bed"), Cube.Object, Wood.Object, FVector(0.0, 0.0, 70.0), FRotator::ZeroRotator,
		FVector(1.6, 1.1, 0.1), true);
	// The wheels' axles across the cart (a cylinder's axis is its Z: rolled onto the cart's Y).
	WheelLeft = MakeCartPart(*this, Root, TEXT("WheelLeft"), Cylinder.Object, Iron.Object, FVector(0.0, -62.0, 45.0),
		FRotator(0.0, 0.0, 90.0), FVector(0.9, 0.9, 0.08), true);
	WheelRight = MakeCartPart(*this, Root, TEXT("WheelRight"), Cylinder.Object, Iron.Object, FVector(0.0, 62.0, 45.0),
		FRotator(0.0, 0.0, 90.0), FVector(0.9, 0.9, 0.08), true);
	// The tank lies along the cart.
	Tank = MakeCartPart(*this, Root, TEXT("Tank"), Cylinder.Object, Iron.Object, TankAt, FRotator(90.0, 0.0, 0.0),
		FVector(0.9, 0.9, 1.4), true);
	Valve = MakeCartPart(*this, Root, TEXT("Valve"), Cylinder.Object, Brass.Object, ValveAt, FRotator::ZeroRotator,
		FVector(0.36, 0.36, 0.04), false);
	Valve->SetMobility(EComponentMobility::Movable);
	Reel = MakeCartPart(*this, Root, TEXT("Reel"), Cylinder.Object, Hose, ReelAt, FRotator(0.0, 0.0, 90.0),
		FVector(0.6, 0.6, 0.5), false);
	HoseNear = MakeCartPart(*this, Root, TEXT("HoseNear"), Cylinder.Object, Hose, ReelAt, FRotator::ZeroRotator,
		FVector::OneVector, false);
	HoseFar = MakeCartPart(*this, Root, TEXT("HoseFar"), Cylinder.Object, Hose, ReelAt, FRotator::ZeroRotator,
		FVector::OneVector, false);
	HoseNear->SetMobility(EComponentMobility::Movable);
	HoseFar->SetMobility(EComponentMobility::Movable);

	Model = MakeCartPart(*this, Root, TEXT("Model"), TripoCart, nullptr, FVector::ZeroVector, FRotator::ZeroRotator,
		FVector::OneVector, TripoCart != nullptr);
	if (TripoCart)
	{
		// The Tripo cart is cart, tank, reel and valve in one: the stand-in's go (the valve's spot still is the hold's).
		Model->SetMaterial(0, nullptr);
		for (UStaticMeshComponent* Part : { Bed.Get(), WheelLeft.Get(), WheelRight.Get(), Tank.Get(), Valve.Get(), Reel.Get() })
		{
			Part->SetVisibility(false);
			Part->SetHiddenInGame(true);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		Model->SetVisibility(false);
		Model->SetHiddenInGame(true);
	}

	// Shots on the tank only (world dynamic, so ground traces pass it), a little bigger than the tank so its edge counts.
	TankHit = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TankHit"));
	TankHit->SetupAttachment(Root);
	TankHit->InitCapsuleSize(48.f, 100.f);
	TankHit->SetRelativeLocationAndRotation(TankAt, FRotator(90.0, 0.0, 0.0));
	TankHit->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TankHit->SetCollisionObjectType(ECC_WorldDynamic);
	TankHit->SetCollisionResponseToAllChannels(ECR_Ignore);
	TankHit->SetCollisionResponseToChannel(CartWeaponChannel, ECR_Block);
	TankHit->SetGenerateOverlapEvents(false);
	TankHit->SetCanEverAffectNavigation(false);
	TankHit->SetHiddenInGame(true);

	Spill = MakeCartPart(*this, Root, TEXT("Spill"), SpillMesh ? SpillMesh : Cube.Object.Get(), Paint.Object, FVector(20.0, 0.0, 2.0),
		FRotator::ZeroRotator, SpillMesh ? SpillScale : FVector(3.0, 3.0, 0.02), false);
	Spill->SetMobility(EComponentMobility::Movable);
	// The slag splat carries four slots (molten, white-hot, glowing edge): all black paint, or the spill glowed orange (the
	// play-through of 2026-11-01).
	for (int32 Slot = 1; Slot < Spill->GetNumMaterials(); ++Slot)
	{
		Spill->SetMaterial(Slot, Paint.Object);
	}
	Spill->SetCastShadow(false);
	Spill->SetVisibility(false);

	ValvePrompt = LOCTEXT("Valve", "Shut the valve");
}

void ACompanyHoseCart::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// In the editor it shows pumping, its hose in place.
	PlaceHose(1.f);
}

void ACompanyHoseCart::BeginPlay()
{
	Super::BeginPlay();
	OnTakePointDamage.AddUniqueDynamic(this, &ACompanyHoseCart::HandlePointDamage);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &ACompanyHoseCart::HandleRunnerChanged);
	}
	// What the story has it as, from the start (no spreading: it was spilled before anyone looked).
	State = EHoseCartState::Away;
	bStoppedHere = false;
	bWasBurst = false;
	TankDamage = 0.f;
	SpillSpread = 1.f;
	ShowState();
	RefreshStory();
}

void ACompanyHoseCart::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	LooterSound::Stop(PumpSound.Get(), 0.f);
	PumpSound.Reset();
	Super::EndPlay(EndPlayReason);
}

void ACompanyHoseCart::HandleRunnerChanged()
{
	RefreshStory();
}

void ACompanyHoseCart::RefreshStory()
{
	EHoseCartState Wanted = EHoseCartState::Away;
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const FStoryCondition& Shown = ShownWhen.IsEmpty() ? PumpingWhen : ShownWhen;
	if (bStoppedHere)
	{
		// Stopped by the player this visit: so it stays, spilled where it was, whatever the story says next.
		Wanted = EHoseCartState::Stopped;
	}
	else if (!Runner)
	{
		// No campaign (a test level, the console's own): a cart with no story pumps and can be stopped.
		Wanted = Shown.IsEmpty() && StoppedWhen.IsEmpty() ? EHoseCartState::Pumping : EHoseCartState::Away;
		bLive = PumpingWhen.IsEmpty();
	}
	else
	{
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		const bool bPast = StoppedWhen.ContainsByPredicate([&Campaign, Runner](const FStoryCondition& When)
		{
			return When.IsMet(Campaign, Runner);
		});
		Wanted = bPast ? EHoseCartState::Stopped : (Shown.IsMet(Campaign, Runner) ? EHoseCartState::Pumping : EHoseCartState::Away);
		bLive = PumpingWhen.IsMet(Campaign, Runner);
	}
	if (Wanted == State)
	{
		return;
	}
	State = Wanted;
	ShowState();
}

void ACompanyHoseCart::ShowState()
{
	const bool bThere = State != EHoseCartState::Away;
	SetActorHiddenInGame(!bThere);
	SetActorEnableCollision(bThere);
	if (Spill)
	{
		const bool bSpilled = State == EHoseCartState::Stopped;
		Spill->SetVisibility(bSpilled);
		Spill->SetRelativeScale3D((Spill->GetStaticMesh() && Spill->GetStaticMesh()->GetName() == TEXT("SM_SlagSplatA") ? SpillScale
			: FVector(3.0, 3.0, 0.02)) * FMath::Max(SpillSpread, 0.05f));
	}
	if (Tank)
	{
		// A burst tank is gone from the cart (torn open: its pieces went with the blast).
		const bool bTankThere = !bWasBurst && !(Model && Model->GetStaticMesh());
		Tank->SetVisibility(bTankThere);
		// Its collision goes with it: hidden, it still stopped bullets in mid-air (the play-through of 2026-11-01).
		Tank->SetCollisionEnabled(bTankThere ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	if (TankHit)
	{
		TankHit->SetCollisionEnabled(State == EHoseCartState::Pumping ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (State != EHoseCartState::Pumping)
	{
		LooterSound::Stop(PumpSound.Get(), 0.2f);
		PumpSound.Reset();
		PlaceHose(1.f);
	}
	// It ticks while it pumps (its pulse, its pump, the valve under a hold), while its spill spreads and while Vane's
	// complaint waits for the spill's words.
	SetActorTickEnabled(State == EHoseCartState::Pumping || (State == EHoseCartState::Stopped && (SpillSpread < 1.f || bComplainPending)));
}

void ACompanyHoseCart::PlaceHose(float Pulse)
{
	const FVector End = HoseEnd.IsNearlyZero() ? FVector(160.0, 0.0, HoseSagHeight) : HoseEnd;
	const FVector Flat(End.X - ReelAt.X, End.Y - ReelAt.Y, 0.0);
	FVector Middle = (ReelAt + End) * 0.5;
	if (Flat.Size() > HoseSagReach)
	{
		// It lies on the ground (the boards) between the reel and what it drinks.
		Middle.Z = HoseSagHeight;
	}
	else
	{
		Middle.Z = FMath::Min(ReelAt.Z, End.Z) - 20.0;
	}
	StretchBetween(HoseNear, ReelAt, Middle, HoseWidth * Pulse);
	StretchBetween(HoseFar, Middle, End, HoseWidth * Pulse);
}

bool ACompanyHoseCart::IsPlayerNear(double Within) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	return Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= Within * Within;
}

void ACompanyHoseCart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (State == EHoseCartState::Stopped)
	{
		// The spill spreads out from under the cart, easing to its size.
		SpillSpread = FMath::Min(SpillSpread + DeltaSeconds / SpillSpreadSeconds, 1.f);
		// Vane complains once the spill's words are said (or after a while).
		if (bComplainPending)
		{
			ComplainWaited += DeltaSeconds;
			const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
			if (!Captions || Captions->GetQueue().IsEmpty() || ComplainWaited >= ComplainWaitSeconds)
			{
				bComplainPending = false;
				Complain();
			}
		}
		ShowState();
		return;
	}
	if (State != EHoseCartState::Pumping || !IsPlayerNear(PulseReach))
	{
		LooterSound::Stop(PumpSound.Get(), 0.2f);
		PumpSound.Reset();
		return;
	}
	// The hose swells at each stroke of the pump: the dead going into the tank.
	PulseClock += DeltaSeconds;
	const float Stroke = FMath::Max(FMath::Sin(PulseClock * PumpStrokesPerSecond * static_cast<float>(UE_TWO_PI)), 0.f);
	PlaceHose(1.f + HosePulse * Stroke);
	if (IsPlayerNear(PumpReach))
	{
		if (!PumpSound.IsValid())
		{
			PumpSound = LooterSound::Start(this, LooterSoundCue::DredgerHoseDrain, Tank, NAME_None, 0.6f, 0.85f);
		}
	}
	else if (PumpSound.IsValid())
	{
		LooterSound::Stop(PumpSound.Get(), 0.3f);
		PumpSound.Reset();
	}
	// The valve wheel spins under the player's hold.
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (Valve && bLive && Interaction && Interaction->GetFocusedActor() == this && Interaction->IsHolding()
		&& Interaction->GetHoldProgress() > 0.f)
	{
		Valve->AddLocalRotation(FRotator(0.0, ValveSpin * DeltaSeconds, 0.0));
	}
}

void ACompanyHoseCart::HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	if (State != EHoseCartState::Pumping || HitComponent != TankHit || Damage <= 0.f)
	{
		return;
	}
	TankDamage += Damage;
	// Each hole spits a puff of the black it holds.
	LamplighterEffects::SmokePuff(this, HitLocation, -ShotFromDirection.GetSafeNormal() * 80.0 + FVector(0.0, 0.0, 40.0));
	// It bursts only once its story lets it be stopped (shot from the approach, it holds until the cart's step begins).
	if (TankDamage >= TankHealth && bLive)
	{
		StopPumping(InstigatedBy, /*bBurst*/ true);
	}
}

FVector ACompanyHoseCart::GetValveLocation() const
{
	return GetActorTransform().TransformPosition(ValveAt);
}

FVector ACompanyHoseCart::GetTankCenter() const
{
	return GetActorTransform().TransformPosition(TankAt);
}

FInteractionOptions ACompanyHoseCart::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!IsLive())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = ValveHoldSeconds;
	Options.HoldPrompt = ValvePrompt;
	return Options;
}

bool ACompanyHoseCart::Interact(UInteractionComponent& User, bool bHeld)
{
	if (!bHeld)
	{
		return false;
	}
	const APawn* Pawn = Cast<APawn>(User.GetOwner());
	return StopPumping(Pawn ? Pawn->GetController() : nullptr, /*bBurst*/ false);
}

TOptional<FVector> ACompanyHoseCart::GetInteractionLocation() const
{
	return TOptional<FVector>(GetValveLocation());
}

TOptional<FInteractionBox> ACompanyHoseCart::GetInteractionBox() const
{
	// The valve wheel and the top of the tank under it.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(ValveAt - FVector(10.0, 0.0, 20.0)),
		FVector(45.0, 45.0, 35.0), GetActorQuat());
}

#undef LOCTEXT_NAMESPACE
