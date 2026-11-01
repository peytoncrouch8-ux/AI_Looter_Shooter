// ACompanyTender: Vane's boarding tender: its run in, its station alongside the Lamplighter, its gangplank, its riflemen
// and its burning fall into the canyon.

#include "World/CompanyTender.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DredgerCreature.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterShip.h"
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
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The user's Tripo company tender (TRIPO_MODELS.md 14; lanes/IMPORT.md), real size; the stand-in was SM_Skiff_A_Gang.
	const TCHAR* HullPath = TEXT("/Game/Art/Tripo/SM_CompanyTender.SM_CompanyTender");
	const TCHAR* PlankPath = TEXT("/Game/Art/Vehicles/SM_Skiff_Gangplank.SM_Skiff_Gangplank");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* IronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");

	/** SM_CompanyTender at its real 12 m (4.4 m in the beam, 4.66 m tall); its riflemen stand on the iron hull's top walk
	 *  (its collision box's top), fore and aft of the bag. */
	const FVector HullScale(1.0, 1.0, 1.0);
	constexpr float DeckTop = 298.f;
	/** Its side's plank hinge out from its middle (the top walk's edge), and the plank's own length (cm). */
	constexpr float HingeOut = 120.f;
	constexpr float PlankLength = 300.f;

	/** Alongside: this far out from her middle and this much higher, so its riflemen look down onto her deck (cm). */
	constexpr float AlongsideOut = 850.f;
	constexpr float AlongsideUp = 110.f;
	/** Where its run in starts in her frame: astern, out and above, angled in (cm, degrees). */
	const FVector ApproachFrom(-6500.0, 3200.0, 1800.0);
	constexpr float ApproachYaw = 18.f;

	/** Its gentle heave while it runs in (cm, Hz): a lighter bag than hers, a quicker bob. */
	constexpr float HeaveCm = 10.f;
	constexpr float HeaveHz = 0.31f;

	/** The plank swings down in this long; the fall's gravity is eased by what's left in its bag. */
	constexpr float PlankSeconds = 0.6f;
	constexpr float FallGravity = 520.f;
	constexpr float FallAwaySpeed = 600.f;
	constexpr float FallRoll = 40.f;
	constexpr float FallPitch = 25.f;
	constexpr float SmokeEvery = 0.06f;
	/** Its bag's middle above its keel (the Tripo tender's bag, 2.3-3.9 m up). */
	constexpr float BagHeight = 388.f;

	/** Its two launchers on its ship-facing rail (tender-local, X along it; Y is flipped by the side it's on). */
	const FVector LauncherSpots[] = { FVector(330.0, 125.0, 330.0), FVector(-450.0, 110.0, 330.0) };
	/** Where its two riflemen stand on its deck (tender-local). */
	const FVector RiflemanDeck[] = { FVector(260.0, 0.0, DeckTop), FVector(-450.0, 0.0, DeckTop) };

	APawn* FindPlayer(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

ACompanyTender::ACompanyTender()
{
	// The Lamplighter moves it on right after herself (ALamplighterShip::AdvancePieces): it keeps station without a frame's lag.
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> HullMesh(HullPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlankMesh(PlankPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(IronPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Its own walkable deck and sides (the skiff's simple hulls): its riflemen stand on it, world-static so their ground
	// traces find it.
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	Hull->SetStaticMesh(HullMesh.Object);
	Hull->SetRelativeScale3D(HullScale);
	Hull->SetMobility(EComponentMobility::Movable);
	Hull->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Hull->SetCollisionObjectType(ECC_WorldStatic);
	Hull->SetCanEverAffectNavigation(false);

	Gangplank = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gangplank"));
	Gangplank->SetupAttachment(Root);
	Gangplank->SetStaticMesh(PlankMesh.Object);
	Gangplank->SetMobility(EComponentMobility::Movable);
	Gangplank->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Gangplank->SetCanEverAffectNavigation(false);

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(LauncherSpots); ++Index)
	{
		UStaticMeshComponent* Launcher = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Launcher%d"), Index));
		Launcher->SetupAttachment(Root);
		Launcher->SetStaticMesh(Cylinder.Object);
		Launcher->SetMaterial(0, Iron.Object);
		Launcher->SetMobility(EComponentMobility::Movable);
		Launcher->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Launcher->SetCanEverAffectNavigation(false);
		// A short iron barrel, 1.1 m long and 22 cm across, lying across the rail.
		Launcher->SetRelativeScale3D(FVector(0.22, 0.22, 1.1));
		Launchers.Add(Launcher);
	}
}

ACompanyTender* ACompanyTender::SpawnFor(ALamplighterShip& ForShip)
{
	UWorld* World = ForShip.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	ACompanyTender* Tender = World->SpawnActorDeferred<ACompanyTender>(ACompanyTender::StaticClass(), ForShip.GetCalmTransform(), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Tender)
	{
		return nullptr;
	}
	Tender->Ship = &ForShip;
	Tender->Side = ForShip.TenderSide >= 0.f ? 1.f : -1.f;
	Tender->FinishSpawning(Tender->StationAt(0.f) * ForShip.GetCalmTransform());
	// Its pieces on the side facing her: the launchers aimed across, the plank's hinge at its opening, raised.
	for (int32 Index = 0; Index < Tender->Launchers.Num(); ++Index)
	{
		FVector Spot = LauncherSpots[Index];
		Spot.Y *= -Tender->Side;
		Tender->Launchers[Index]->SetRelativeLocationAndRotation(Spot, FRotationMatrix::MakeFromZ(FVector(0.0, -Tender->Side, 0.0)).Rotator());
	}
	Tender->PlankAlpha = 0.f;
	Tender->Advance(0.f);
	return Tender;
}

void ACompanyTender::BeginPlay()
{
	Super::BeginPlay();
	// Heard before it's seen, coming up astern: the same engine as hers, smaller and faster.
	EngineSound = LooterSound::Start(this, LooterSoundCue::ShipEngine, Root, NAME_None, 0.85f, 1.35f);
}

void ACompanyTender::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LooterSound::Stop(EngineSound.Get(), 0.3f);
	EngineSound.Reset();
	for (const TWeakObjectPtr<ACreatureBase>& Rifleman : Riflemen)
	{
		if (ACreatureBase* Gone = Rifleman.Get())
		{
			Gone->Destroy();
		}
	}
	Riflemen.Reset();
	Super::EndPlay(EndPlayReason);
}

FTransform ACompanyTender::StationAt(float Alpha) const
{
	const float A = FMath::Clamp(Alpha, 0.f, 1.f);
	// Catching her up astern first (an ease out along her length), sliding in to her side late, dropping as it comes.
	const float Ahead = 1.f - FMath::Pow(1.f - A, 3.f);
	const float In = FMath::SmoothStep(0.3f, 1.f, A);
	const float Down = FMath::SmoothStep(0.f, 0.85f, A);
	const FVector Local(
		FMath::Lerp(ApproachFrom.X, 0.0, static_cast<double>(Ahead)),
		Side * FMath::Lerp(ApproachFrom.Y, static_cast<double>(AlongsideOut), static_cast<double>(In)),
		FMath::Lerp(ApproachFrom.Z, static_cast<double>(AlongsideUp), static_cast<double>(Down)));
	const float Yaw = Side * ApproachYaw * (1.f - In);
	return FTransform(FRotator(0.0, Yaw, 0.0), Local, FVector::OneVector);
}

void ACompanyTender::SetAlongsideNow()
{
	Clock = ApproachSeconds;
	bAlongside = true;
	Advance(0.f);
}

void ACompanyTender::Advance(float DeltaSeconds)
{
	const ALamplighterShip* Boat = Ship.Get();
	if (bFalling)
	{
		FallClock += DeltaSeconds;
		// Pushed off her side as the lines part, sinking faster as its bag empties, rolling away and nosing down.
		FallVelocity.Z -= FallGravity * DeltaSeconds;
		FTransform Now = GetActorTransform();
		Now.AddToTranslation(FallVelocity * DeltaSeconds);
		const float Over = FMath::SmoothStep(0.f, 4.f, FallClock);
		// Its far side dips (a turn about X lifts +Y), so it rolls away from her.
		const FQuat Tumble = FQuat(FVector::XAxisVector, FMath::DegreesToRadians(-Side * FallRoll * Over))
			* FQuat(FVector::YAxisVector, FMath::DegreesToRadians(FallPitch * Over));
		Now.SetRotation(FallFrom.GetRotation() * Tumble);
		SetActorTransform(Now);
		if (!bBurst && FallClock >= 0.2f)
		{
			// Its bag goes up as it drops: a ball of fire, then the smoke.
			bBurst = true;
			LamplighterEffects::Fireball(this, Now.TransformPosition(FVector(0.0, 0.0, BagHeight)), FallVelocity * 0.5f);
		}
		SmokeWait -= DeltaSeconds;
		if (SmokeWait <= 0.f)
		{
			SmokeWait = SmokeEvery;
			LamplighterEffects::SmokePuff(this, Now.TransformPosition(FVector(FMath::FRandRange(-300.f, 300.f), 0.0, BagHeight)),
				FVector(0.0, 0.0, 120.0) - FallVelocity * 0.15f);
		}
		// Its riflemen go down with it, held where they stood.
		for (int32 Index = 0; Index < Riflemen.Num(); ++Index)
		{
			ACreatureBase* Rifleman = Riflemen[Index].Get();
			if (Rifleman && RiflemanSpots.IsValidIndex(Index))
			{
				const float Half = Rifleman->GetCapsuleComponent() ? Rifleman->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
				Rifleman->SetActorLocation(Now.TransformPosition(RiflemanSpots[Index]) + Now.GetRotation().GetUpVector() * Half,
					/*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		if (FallClock >= FallSeconds)
		{
			Destroy();
		}
		return;
	}
	if (!Boat)
	{
		return;
	}
	Clock += DeltaSeconds;
	const float Alpha = Clock / ApproachSeconds;
	if (!bAlongside && Alpha >= 1.f)
	{
		bAlongside = true;
		UE_LOG(LogLooter, Log, TEXT("Landfall: the Company tender is alongside."));
	}
	// Bobbing while it runs in and keeps station on a sailing ship; dead still beside a grappled one.
	const bool bStill = Boat->GetFlight() != EShipFlight::Sailing;
	const float Heave = bStill ? 0.f : HeaveCm * FMath::Sin(UE_TWO_PI * HeaveHz * Clock);
	FTransform Station = StationAt(Alpha);
	Station.AddToTranslation(FVector(0.0, 0.0, Heave));
	Station.SetScale3D(FVector::OneVector);
	SetActorTransform(Station * Boat->GetCalmTransform());

	if (bPlankLowering && PlankAlpha < 1.f)
	{
		PlankAlpha = FMath::Min(PlankAlpha + DeltaSeconds / PlankSeconds, 1.f);
		if (PlankAlpha >= 1.f)
		{
			const FVector End = Gangplank->GetComponentTransform().TransformPosition(FVector(0.0, -PlankLength, 0.0));
			LooterSound::PlayAt(this, LooterSoundCue::ImpactWood, End);
		}
	}
	// The plank on its hinge: it reaches along its own -Y from the hinge, so on the port side it's turned about.
	const float Gap = AlongsideOut - HingeOut - ALamplighterShip::RailOut;
	const float Drop = AlongsideUp + DeckTop - ALamplighterShip::DeckTop;
	const float Reach = FMath::Sqrt(Gap * Gap + Drop * Drop);
	const float Lowered = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Drop / Reach, 0.f, 1.f)));
	const float Tilt = FMath::Lerp(-90.f, Lowered, FMath::SmoothStep(0.f, 1.f, PlankAlpha));
	const FQuat Facing(FVector::UpVector, Side > 0.f ? 0.0 : UE_PI);
	Gangplank->SetRelativeLocationAndRotation(FVector(0.0, -Side * HingeOut, DeckTop + 2.0),
		Facing * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Tilt)));
	const float PlankScale = Reach / PlankLength;
	Gangplank->SetRelativeScale3D(FVector(PlankScale, PlankScale, PlankScale));
}

FVector ACompanyTender::GetLauncher(int32 Index) const
{
	if (!Launchers.IsValidIndex(Index) || !Launchers[Index])
	{
		return GetActorLocation();
	}
	// The barrel's ship-side end.
	FVector Spot = LauncherSpots[Index];
	Spot.Y = -Side * (Spot.Y + 55.0);
	return GetActorTransform().TransformPosition(Spot);
}

void ACompanyTender::LowerGangplank()
{
	bPlankLowering = true;
}

void ACompanyTender::SpawnRiflemen()
{
	UWorld* World = GetWorld();
	const TSubclassOf<ACreatureBase> Class = ADredgerCreature::ClassFor(EDredgerRole::Rifleman);
	if (!World || !Class || !Riflemen.IsEmpty())
	{
		return;
	}
	APawn* Player = FindPlayer(World);
	// Facing across to her deck.
	const float Yaw = GetActorRotation().Yaw - Side * 90.f;
	for (const FVector& Spot : RiflemanDeck)
	{
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.bMayBeTouched = false;
		ACreatureBase* Rifleman = ACreatureBase::SpawnAtRuntime(World, Class, GetActorTransform().TransformPosition(Spot), Yaw, Spawn);
		if (!Rifleman)
		{
			continue;
		}
		// They stay on its deck and fire across: no cover to look for, no walking off toward the player.
		if (ADredgerCreature* Dredger = Cast<ADredgerCreature>(Rifleman))
		{
			Dredger->SetHoldsGround(true);
		}
		if (Player)
		{
			Rifleman->AlertTo(Player);
		}
		Riflemen.Add(Rifleman);
		RiflemanSpots.Add(Spot);
	}
	UE_LOG(LogLooter, Log, TEXT("Landfall: %d riflemen on the tender's deck."), Riflemen.Num());
}

int32 ACompanyTender::GetRiflemenAlive() const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<ACreatureBase>& Rifleman : Riflemen)
	{
		Alive += Rifleman.IsValid() && !Rifleman->IsDead() ? 1 : 0;
	}
	return Alive;
}

void ACompanyTender::FallAway()
{
	if (bFalling)
	{
		return;
	}
	bFalling = true;
	FallClock = 0.f;
	FallFrom = GetActorTransform();
	FallVelocity = GetActorRightVector() * (Side * FallAwaySpeed) + FVector(0.0, 0.0, -150.0);
	SmokeWait = 0.f;
	// Its riflemen go down with it, frozen where they stood (as a boss intro holds actors still): no more shots, no loot.
	for (const TWeakObjectPtr<ACreatureBase>& Rifleman : Riflemen)
	{
		if (ACreatureBase* Held = Rifleman.Get())
		{
			Held->CustomTimeDilation = 0.001f;
			Held->SetActorEnableCollision(false);
		}
	}
	LooterSound::PlayAttached(LooterSoundCue::TenderFall, Root);
	LooterSound::Stop(EngineSound.Get(), 1.5f);
	EngineSound.Reset();
	UE_LOG(LogLooter, Log, TEXT("Landfall: the tender falls away into the canyon."));
}
