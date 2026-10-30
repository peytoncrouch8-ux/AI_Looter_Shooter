// AForemanCreature's shift and the dark. The shift (at 66% and 33%): three dead miners rise out of the fall's rubble (an actor
// tagged Foreman_Rubble in the stope, or the wall he digs at), on spots an encounter's rules choose (his own level of floor,
// standable and roomy, clear of the player), kept to the level's caps, fading up through the rock as the Unpaid rise; his
// boss counts them as its adds (gone with a fight that starts over, crumbling with him). The dark (at 50%): the stope's
// lanterns (actors tagged StopeLantern: their lights and their parts tagged LanternGlow) blow out for 12 s, and the view's
// exposure lifts a little so the lamps and coals left read in a dark that isn't black; then they catch again, flickering.

#include "Creatures/ForemanCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterGroundProbe.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/PlayableArea.h"
#include "Components/CapsuleComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A shift's miner never rises nearer the player than this (cm): out of the rubble, not under their feet. */
	constexpr float ShiftClearOfPlayer = 300.f;

	/** They rise at least this far apart (cm), on floor at most this far above or below the rubble's (cm). */
	constexpr float ShiftSpacing = 200.f;
	constexpr float ShiftMaxStep = 200.f;

	/** Without rubble, the shift rises this far back from the wall he digs at toward his spot (cm). */
	constexpr float OffTheWall = 150.f;

	/** The dark reaches the lanterns within this of his spot (cm): the stope and the drift's end. */
	constexpr float DarkReach = 3500.f;

	/** The lanterns catching again: a flicker over this long (s), and the exposure's ease (1/s). */
	constexpr float RelightSeconds = 0.6f;
	constexpr float ExposureEase = 1.2f;

	/** The exposure's lift stands over every other volume (the level's lighting state's). */
	constexpr float LiftPriority = 10000.f;
}

// ---------------------------------------------------------------------------
// The shift
// ---------------------------------------------------------------------------

void AForemanCreature::PruneShift()
{
	Shift.RemoveAll([](const TWeakObjectPtr<ADeadMinerCreature>& Each) { return !Each.IsValid() || Each->IsDead(); });
}

TArray<ADeadMinerCreature*> AForemanCreature::GetShift() const
{
	TArray<ADeadMinerCreature*> Alive;
	for (const TWeakObjectPtr<ADeadMinerCreature>& Each : Shift)
	{
		ADeadMinerCreature* Miner = Each.Get();
		if (Miner && !Miner->IsDead())
		{
			Alive.Add(Miner);
		}
	}
	return Alive;
}

int32 AForemanCreature::CallShift()
{
	UWorld* World = GetWorld();
	if (!World || IsDead())
	{
		return 0;
	}
	PruneShift();
	++ShiftsCalled;

	// A call into a crowded fight brings fewer: the level's caps hold for his shift too (the Unpaid's 12, and 16 near the player).
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	int32 Wanted = Foreman::ShiftSize;
	if (Encounters)
	{
		const UClass* Counted = nullptr;
		const int32 Cap = Settings.FindClassCap(ADeadMinerCreature::StaticClass(), &Counted);
		if (Cap > 0)
		{
			Wanted = FMath::Min(Wanted, EncounterRules::Room(Encounters->CountAlive(Counted), Cap));
		}
		if (Player)
		{
			Wanted = FMath::Min(Wanted, EncounterRules::Room(Encounters->CountAliveNear(Player->GetActorLocation(), Settings.NearPlayerRadius),
				Settings.MaxCreaturesNearPlayer));
		}
	}
	if (Wanted <= 0)
	{
		UE_LOG(LogLooter, Log, TEXT("%s calls his shift, but the fight is full: none come."), *GetName());
		return 0;
	}

	// The fall's rubble: the tagged actor in his stope nearest him, or the wall he digs at, or his own spot.
	const FVector HomeSpot = GetHome().GetLocation();
	FVector Rubble = HomeSpot;
	double Nearest = TNumericLimits<double>::Max();
	bool bFoundRubble = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(Foreman::RubbleTag))
		{
			const double Distance = FVector::Dist2D(It->GetActorLocation(), HomeSpot);
			if (Distance <= ArenaRadius && Distance < Nearest)
			{
				Nearest = Distance;
				Rubble = It->GetActorLocation();
				bFoundRubble = true;
			}
		}
	}
	FVector Wall;
	FVector WallNormal;
	if (!bFoundRubble && FindWorkWall(Wall, WallNormal))
	{
		Rubble = Wall + WallNormal.GetSafeNormal2D() * OffTheWall;
	}

	// Spots round the rubble on his own level of floor, as a spawner chooses them (standable, roomy, out of safe zones, inside
	// the playable area and his ground, clear of the player and apart); with no ground at all (a test level), level with him.
	const FEncounterBody MinerBody = EncounterRules::CreatureBody(ADeadMinerCreature::StaticClass(), 0.f,
		UCreatureRankSettings::Get(ECreatureRank::Basic).Size);
	FEncounterGroundProbe Probe(World, this, TEXT("ForemanShift"), MinerBody, ShiftMaxStep);
	const FVector Feet = GetFeet();
	Probe.FindHome(FVector(Rubble.X, Rubble.Y, Feet.Z), 150.0, 300.0);
	const double HomeZ = Probe.GetHomeZ();
	const TArray<FVector> Candidates = EncounterRules::CandidateSpots(FVector(Rubble.X, Rubble.Y, HomeZ), ShiftRadius, TArray<FVector>(),
		Wanted, ShiftSpacing, FMath::FRandRange(0.f, 360.f));
	TArray<FVector> Occupied = { GetActorLocation() };
	for (const ADeadMinerCreature* Miner : GetShift())
	{
		Occupied.Add(Miner->GetActorLocation());
	}
	auto GroundAt = [&Probe](const FVector& Spot)
	{
		return Probe.Look(Spot);
	};
	const APlayableArea* Playable = APlayableArea::Find(World);
	const APawn* Victim = GetTarget();
	const APawn* Watched = Victim ? Victim : Player;
	const double ClearSquared = FMath::Square(static_cast<double>(ShiftClearOfPlayer));
	auto IsBlocked = [this, Encounters, Playable, Watched, ClearSquared, &HomeSpot](const FVector& Spot)
	{
		return (Encounters && Encounters->IsInSafeZone(Spot))
			|| (Playable && !Playable->Contains(Spot))
			|| !HuntingGround.ContainsSpot(Spot, HomeSpot)
			|| (Watched && FVector::DistSquared2D(Spot, Watched->GetActorLocation()) < ClearSquared);
	};
	const TArray<FVector> Spots = EncounterRules::ChooseSpots(Candidates, Wanted, HomeZ, ShiftMaxStep, ShiftSpacing, Occupied, GroundAt, IsBlocked);

	int32 Rose = 0;
	for (const FVector& Spot : Spots)
	{
		const FVector Facing = Victim ? Victim->GetActorLocation() - Spot : Spot - Rubble;
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = ECreatureRank::Basic;
		// Their own level from where they stand (the Providence's), never his Soulfed one.
		Spawn.Level = 0;
		// His fight is built by hand: his shift is never Touched.
		Spawn.bMayBeTouched = false;
		ADeadMinerCreature* Miner = Cast<ADeadMinerCreature>(ACreatureBase::SpawnAtRuntime(World, ADeadMinerCreature::StaticClass(), Spot,
			static_cast<float>(Facing.Rotation().Yaw), Spawn));
		if (!Miner)
		{
			continue;
		}
		// In a level that isn't playing yet (a test level), it starts as play would start it.
		if (!Miner->HasActorBegunPlay())
		{
			Miner->DispatchBeginPlay();
		}
		// They fight on his ground and go for whoever he's after; up through the rock, fading in.
		Miner->HuntingGround = HuntingGround;
		Miner->RiseIn();
		if (APawn* Hunted = GetTarget())
		{
			Miner->AlertTo(Hunted);
		}
		Shift.Add(Miner);
		if (Boss)
		{
			Boss->AdoptAdd(Miner);
		}
		KickDirt(Spot, 0.85f);
		++Rose;
	}
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::ShiftRise, Rubble);
	UE_LOG(LogLooter, Log, TEXT("%s calls his shift (%d of %d): %d of %d dead miners rise out of the rubble."), *GetName(), ShiftsCalled,
		Foreman::NumShifts, Rose, Foreman::ShiftSize);
	return Rose;
}

// ---------------------------------------------------------------------------
// The dark
// ---------------------------------------------------------------------------

void AForemanCreature::BeginDark()
{
	UWorld* World = GetWorld();
	if (!World || IsDark())
	{
		return;
	}
	// Any flicker of the last relight ends lit first, so what goes out now is everything that burns.
	if (RelightLeft > 0.f)
	{
		RelightLeft = 0.f;
		SetLanterns(true);
	}
	DarkenedLights.Reset();
	DarkenedGlows.Reset();
	const FVector Center = Boss && Boss->IsFighting() ? Boss->GetSpot() : GetHome().GetLocation();
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Lantern = *It;
		if (!Lantern->ActorHasTag(Foreman::LanternTag) || FVector::Dist2D(Lantern->GetActorLocation(), Center) > DarkReach)
		{
			continue;
		}
		TArray<ULightComponent*> Lights;
		Lantern->GetComponents<ULightComponent>(Lights);
		for (ULightComponent* Light : Lights)
		{
			if (Light && Light->IsVisible())
			{
				DarkenedLights.Add(Light);
			}
		}
		TArray<UPrimitiveComponent*> Parts;
		Lantern->GetComponents<UPrimitiveComponent>(Parts);
		for (UPrimitiveComponent* Part : Parts)
		{
			if (Part && Part->ComponentHasTag(Foreman::LanternGlowTag) && Part->IsVisible())
			{
				DarkenedGlows.Add(Part);
			}
		}
		// A breath of smoke off each wick as it's snuffed.
		if (Bullets)
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			Effects.SpawnDustPuff(Lantern->GetActorLocation() + FVector(0.0, 0.0, 20.0), FVector(0.0, 0.0, 40.0), FLinearColor(0.12f, 0.12f, 0.12f),
				0.4f, 6.f, 40.f, 1.8f);
		}
	}
	SetLanterns(false);
	DarkLeft = DarkSeconds;
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::LanternsOut, Center + FVector(0.0, 0.0, 300.0));
	Say(ForemanFight::DarkLine());
	UE_LOG(LogLooter, Log, TEXT("%s's stope goes dark: %d lights out for %.0f s."), *GetName(), DarkenedLights.Num(), DarkSeconds);
}

void AForemanCreature::EndDark(bool bAtOnce)
{
	const bool bWasDark = DarkLeft > 0.f || DarkenedLights.Num() > 0 || DarkenedGlows.Num() > 0;
	DarkLeft = 0.f;
	if (bAtOnce)
	{
		RelightLeft = 0.f;
		SetLanterns(true);
		DarkenedLights.Reset();
		DarkenedGlows.Reset();
		SetExposureLift(0.f);
		return;
	}
	if (bWasDark && RelightLeft <= 0.f)
	{
		// They catch again with a flicker, glass ticking as it warms.
		RelightLeft = RelightSeconds;
		const FVector Center = Boss && Boss->IsFighting() ? Boss->GetSpot() : GetHome().GetLocation();
		LooterSound::PlayAt(this, LooterSoundCue::Foreman::LanternsBack, Center + FVector(0.0, 0.0, 300.0));
	}
}

void AForemanCreature::TickDark(float DeltaSeconds)
{
	if (DarkLeft > 0.f)
	{
		DarkLeft -= DeltaSeconds;
		// The view's lift only where lights went out: a stope with none to put out stays as it is.
		if (DarkenedLights.Num() > 0)
		{
			ExposureShare = FMath::Min(ExposureShare + ExposureEase * DeltaSeconds, 1.f);
			SetExposureLift(ExposureShare);
		}
		if (DarkLeft <= 0.f)
		{
			EndDark(/*bAtOnce*/ false);
		}
		return;
	}
	if (RelightLeft > 0.f)
	{
		// On, off, and on to stay: a flame catching.
		RelightLeft -= DeltaSeconds;
		const float Share = RelightLeft / RelightSeconds;
		SetLanterns(RelightLeft <= 0.f || Share > 0.75f || Share < 0.45f);
		if (RelightLeft <= 0.f)
		{
			RelightLeft = 0.f;
			DarkenedLights.Reset();
			DarkenedGlows.Reset();
		}
	}
	if (ExposureShare > 0.f)
	{
		ExposureShare = FMath::Max(ExposureShare - ExposureEase * DeltaSeconds, 0.f);
		SetExposureLift(ExposureShare);
	}
}

void AForemanCreature::SetLanterns(bool bOn)
{
	for (const TWeakObjectPtr<ULightComponent>& Each : DarkenedLights)
	{
		if (ULightComponent* Light = Each.Get(); Light && Light->IsVisible() != bOn)
		{
			Light->SetVisibility(bOn);
		}
	}
	for (const TWeakObjectPtr<UPrimitiveComponent>& Each : DarkenedGlows)
	{
		if (UPrimitiveComponent* Part = Each.Get(); Part && Part->IsVisible() != bOn)
		{
			Part->SetVisibility(bOn);
		}
	}
}

void AForemanCreature::SetExposureLift(float Share)
{
	APostProcessVolume* Volume = DarkVolume.Get();
	if (Share <= 0.f || DarkExposureLift <= 0.f)
	{
		if (Volume)
		{
			Volume->Destroy();
		}
		DarkVolume.Reset();
		return;
	}
	UWorld* World = GetWorld();
	if (!Volume && World)
	{
		// Lifted from the level's own exposure (its lighting state's unbound volume), over everything else for the dark.
		float Base = 0.f;
		float BestPriority = -TNumericLimits<float>::Max();
		for (TActorIterator<APostProcessVolume> It(World); It; ++It)
		{
			if (It->bUnbound && It->bEnabled && It->Settings.bOverride_AutoExposureBias && It->Priority > BestPriority)
			{
				Base = It->Settings.AutoExposureBias;
				BestPriority = It->Priority;
			}
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Volume = World->SpawnActor<APostProcessVolume>(GetActorLocation(), FRotator::ZeroRotator, Params);
		if (!Volume)
		{
			return;
		}
		Volume->bUnbound = true;
		Volume->Priority = LiftPriority;
		Volume->Settings.bOverride_AutoExposureBias = true;
		Volume->Settings.AutoExposureBias = Base + DarkExposureLift;
		DarkVolume = Volume;
	}
	if (Volume)
	{
		Volume->BlendWeight = FMath::Clamp(Share, 0.f, 1.f);
	}
}
