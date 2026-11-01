// ALamplighterShip: breaking free with her bag torn, the hard landing on the Mooring Ledge, lying moored there (groaning
// toward the drop until the anchor is down), and the anchor winch.

#include "World/LamplighterShip.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Audio/LooterSound.h"
#include "Player/ViewKick.h"
#include "Scenes/HardLanding.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "World/LamplighterEffects.h"
#include "World/LandfallWinch.h"
#include "World/WorldQueries.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	/** The torn bag's gas jets this often while she falls; the skid throws sparks this often (seconds). */
	constexpr float GasEvery = 0.05f;
	constexpr float SparkEvery = 0.06f;
	/** Where her bag is torn (her frame: the tender's side, high on the bag) and her keel's three points that strike. */
	const FVector TearLocal(150.0, 520.0, 1425.0);
	const FVector KeelPoints[] = { FVector(800.0, 0.0, 0.0), FVector(0.0, 0.0, 0.0), FVector(-800.0, 0.0, 45.0) };
	/** The crash's dust round her: this far across, this many puffs. */
	constexpr float DustRadius = 1500.f;
	constexpr int32 DustPuffs = 30;

	/** Moored, not anchored: she shifts toward the drop now and then with a groan (seconds apart), this far each time at most,
	 *  and never more than SlideMax in all (cm); the shift takes ShiftSeconds. */
	constexpr float GroanMin = 3.5f;
	constexpr float GroanMax = 7.f;
	constexpr float ShiftCm = 12.f;
	constexpr float SlideMax = 80.f;
	constexpr float ShiftSeconds = 0.9f;
	constexpr float ShiftRock = 0.6f;

	/** The anchor chain ties on under her rail toward the bow, on the gangway's (the ledge's) side (her frame). */
	const FVector AnchorTieLocal(700.0, 375.0, -30.0);
}

void ALamplighterShip::AdvanceBreaking(float DeltaSeconds)
{
	BreakClock += DeltaSeconds;
	ApplyLanding(FMath::Min(BreakClock, HardLanding::BreakSeconds));
	if (BreakClock >= HardLanding::BreakSeconds)
	{
		StartHardLanding();
	}
}

void ALamplighterShip::StartHardLanding()
{
	USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (Scenes && Scenes->IsPlaying())
	{
		// Another scene has the player: she hangs where she is and tries again next frame.
		return;
	}
	EnterFlight(EShipFlight::Landing);
	// With scenes off (a tour, a perf run) Play counts it played for the missions and plays nothing: she's simply down.
	if (!Scenes || !Scenes->Play(HardLanding::Make(*Scenes, *this)))
	{
		if (Scenes)
		{
			Scenes->MarkPlayed(HardLanding::SceneName());
		}
		FinishLanding();
	}
}

void ALamplighterShip::ApplyLanding(float SecondsFromBreak)
{
	const float TornSide = TenderSide >= 0.f ? 1.f : -1.f;
	SetActorTransform(HardLanding::PoseAt(BreakFrom, Moored, SecondsFromBreak, TornSide), /*bSweep*/ false, nullptr, ETeleportType::None);
	CalmTransform = FTransform(FRotator(0.0, GetActorRotation().Yaw, 0.0), GetActorLocation());
	const float Step = SecondsFromBreak - LastLanding;
	LastLanding = SecondsFromBreak;
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (Step <= 0.f || Step > 0.5f || (Scenes && Scenes->IsSkipping()))
	{
		return;
	}
	if (SecondsFromBreak < HardLanding::TouchAt)
	{
		// The gas jetting from the tear, pale breaths out of her bag's side.
		GasWait -= Step;
		if (GasWait <= 0.f)
		{
			GasWait = GasEvery;
			const FVector Tear(TearLocal.X, TearLocal.Y * TornSide, TearLocal.Z);
			LamplighterEffects::GasPuff(this, GetActorTransform().TransformPosition(Tear),
				GetActorTransform().TransformVectorNoScale(FVector(-0.3, TornSide, 0.25)));
		}
	}
	else if (SecondsFromBreak < HardLanding::TouchAt + HardLanding::SkidSeconds)
	{
		// The skid: her keel screaming on rock, sparks thrown out behind her and to the sides.
		SparkWait -= Step;
		if (SparkWait <= 0.f)
		{
			SparkWait = SparkEvery;
			const FVector& Keel = KeelPoints[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(KeelPoints)) - 1)];
			const FVector Side = GetActorRightVector() * (FMath::RandBool() ? 1.0 : -1.0);
			LamplighterEffects::Sparks(this, GetActorTransform().TransformPosition(Keel), Side - GetActorForwardVector() * 0.6, 3, 1.3f);
		}
	}
}

void ALamplighterShip::Touchdown(bool bSkipping)
{
	bTouched = true;
	if (bSkipping)
	{
		return;
	}
	// The crash: the whole hull slammed onto the ledge, dust blown out round her, sparks off her keel, the view thrown.
	const FVector Here = GetActorLocation();
	LooterSound::PlayAt(this, LooterSoundCue::ShipHardLanding, Here);
	LamplighterEffects::Shake(this, 1.25f, 1.6f);
	FViewKick Jolt;
	Jolt.Pitch = -5.f;
	Jolt.Roll = (TenderSide >= 0.f ? -1.f : 1.f) * 3.f;
	Jolt.FieldOfView = 0.04f;
	Jolt.Frequency = 7.f;
	Jolt.Damping = 0.45f;
	LamplighterEffects::Kick(this, Jolt);
	LamplighterEffects::DustSkirt(this, Here, DustRadius, DustPuffs);
	for (const FVector& Keel : KeelPoints)
	{
		LamplighterEffects::Sparks(this, GetActorTransform().TransformPosition(Keel), GetActorRightVector(), 5, 1.5f);
		LamplighterEffects::Sparks(this, GetActorTransform().TransformPosition(Keel), -GetActorRightVector(), 5, 1.5f);
	}
	BraceCrew(2.5f);
	UE_LOG(LogLooter, Log, TEXT("Landfall: she hits the Mooring Ledge."));
}

void ALamplighterShip::FinishLanding()
{
	if (Flight == EShipFlight::Moored)
	{
		return;
	}
	if (!bTouched)
	{
		bTouched = true;
	}
	const FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(GetWorld());
	bAnchored = Campaign && LandfallRules::IsAnchoredIn(*Campaign);
	EnterFlight(EShipFlight::Moored);
	NextGroan = FMath::FRandRange(GroanMin, GroanMax);
	FindOrSpawnWinch();
	// The anchor's lines (DA_Lines_LandfallAnchor) are Hob's arrival on the ledge as step 4 begins (story_texts
	// LANDFALL_PERCHES), not the ship's: said once, by one owner.
	bAnchorLinesSaid = true;
	UE_LOG(LogLooter, Log, TEXT("Landfall: down on the Mooring Ledge%s."), bAnchored ? TEXT(", anchored") : TEXT("; the anchor isn't down yet"));
}

void ALamplighterShip::AdvanceMoored(float DeltaSeconds)
{
	if (bAnchored)
	{
		return;
	}
	// Not held: she groans and shifts toward the drop now and then, a little each time (the reason to wind the winch).
	NextGroan -= DeltaSeconds;
	if (NextGroan <= 0.f && SlideClock <= 0.f)
	{
		NextGroan = FMath::FRandRange(GroanMin, GroanMax);
		SlideClock = ShiftSeconds;
		LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, GetActorLocation(), 0.45f, 0.8f);
		LamplighterEffects::Shake(this, 0.18f, 0.6f);
	}
	if (SlideClock <= 0.f)
	{
		return;
	}
	SlideClock = FMath::Max(SlideClock - DeltaSeconds, 0.f);
	const float Through = 1.f - SlideClock / ShiftSeconds;
	const FVector Toward = CanyonSide.GetSafeNormal2D();
	if (!Toward.IsNearlyZero())
	{
		Slid = FMath::Min(Slid + ShiftCm * DeltaSeconds / ShiftSeconds, SlideMax);
	}
	const float Rock = ShiftRock * FMath::Sin(UE_PI * Through) * FMath::Sin(UE_TWO_PI * 1.4f * Through);
	FTransform Pose = Moored;
	Pose.AddToTranslation(Toward * Slid);
	Pose.SetRotation(Moored.GetRotation() * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Rock)));
	SetActorTransform(Pose, /*bSweep*/ false, nullptr, ETeleportType::None);
}

void ALamplighterShip::SetAnchored(bool bPlayed)
{
	if (bAnchored)
	{
		return;
	}
	bAnchored = true;
	SlideClock = 0.f;
	// Settled where the last shift left her, level again.
	FTransform Pose = GetActorTransform();
	Pose.SetRotation(Moored.GetRotation());
	SetActorTransform(Pose, /*bSweep*/ false, nullptr, ETeleportType::None);
	if (FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(GetWorld()))
	{
		Campaign->SetFlag(LandfallRules::AnchoredFlag());
	}
	if (bPlayed)
	{
		// The chain snaps taut, she settles on it, and Junie cuts the engine.
		LamplighterEffects::Shake(this, 0.3f, 0.5f);
		BraceCrew(1.f);
	}
	UpdateSounds();
	// The winch shows its chain run out to her (wound by the player, or a jump past it).
	if (ALandfallWinch* Winch = FindOrSpawnWinch())
	{
		Winch->SetShip(this);
	}
	UE_LOG(LogLooter, Log, TEXT("Landfall: the anchor is down; she's held on the ledge."));
}

FVector ALamplighterShip::GetAnchorPoint() const
{
	const float SideSign = GangwaySide >= 0.f ? 1.f : -1.f;
	return DeckPoint(FVector(AnchorTieLocal.X, AnchorTieLocal.Y * SideSign, AnchorTieLocal.Z));
}

ALandfallWinch* ALamplighterShip::FindOrSpawnWinch()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ALandfallWinch> It(World); It; ++It)
	{
		It->SetShip(this);
		return *It;
	}
	// None placed on the ledge: one beside her gangway, stood on the ground there.
	FVector Spot = Moored.TransformPosition(WinchOffset);
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("LandfallWinchGround"), this);
	if (World->LineTraceSingleByObjectType(Hit, Spot + FVector(0.0, 0.0, 500.0), Spot - FVector(0.0, 0.0, 2000.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Spot = Hit.ImpactPoint;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALandfallWinch* Winch = World->SpawnActor<ALandfallWinch>(Spot, FRotator(0.0, Moored.Rotator().Yaw, 0.0), SpawnParams);
	if (Winch)
	{
		Winch->SetShip(this);
		UE_LOG(LogLooter, Log, TEXT("Landfall: no anchor winch placed on the ledge; one stands by her gangway."));
	}
	return Winch;
}
