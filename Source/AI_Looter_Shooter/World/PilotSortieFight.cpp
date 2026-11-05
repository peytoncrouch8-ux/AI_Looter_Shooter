// APilotSortie: the fight on the way and the pieces at the barge: the Company's two tenders (their gas-bags the weak spot,
// their boarders on her deck, Pilot.TendersDown), Junie's two launchers, the plank across to the barge and the deck's
// marker. (The jump, the catch and the fall: PilotSortieJump.cpp.)

#include "World/PilotSortie.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DredgerCreature.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "World/BargeBoarding.h"
#include "World/CompanyTender.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterLauncher.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"
#include "World/LandfallMarker.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// --- The tenders (her calm frame, cm; +Y her starboard) ---

	/** The port tender comes up from astern and stands off her port beam, a little high, its riflemen firing across. */
	const FVector PortFrom(-6000.0, -3600.0, 2000.0);
	const FVector PortStation(300.0, -2300.0, 420.0);
	constexpr float PortRunSeconds = 10.f;
	/** The boarder runs in alongside her starboard rail (Landfall's station: its plank reaches her rail from there). */
	const FVector BoardFrom(-6500.0, 3400.0, 1800.0);
	const FVector BoardStation(0.0, ACompanyTender::StationOut, ACompanyTender::StationUp);
	constexpr float BoardRunSeconds = 9.f;
	/** Its men across, it pulls off to stand off her starboard quarter, its riflemen on her. */
	const FVector BoardStandOff(-400.0, 2400.0, 420.0);
	constexpr float PullOffSeconds = 5.f;
	/** Its beats once alongside (s): its riflemen up, the plank down, the boarders over one after another, then it pulls off. */
	constexpr float RiflemenAt = 1.f;
	constexpr float PlankAt = 1.5f;
	constexpr float FirstBoarderAt = 2.3f;
	constexpr float BoarderSpacing = 0.7f;
	constexpr float PullOffAfter = 4.f;
	/** Where the boarders come aboard (her frame, on her starboard side by the plank), and who: two riflemen and a hose-man. */
	const FVector BoarderSpots[] = { FVector(60.0, 220.0, 0.0), FVector(-120.0, 200.0, 0.0), FVector(220.0, 180.0, 0.0) };
	const EDredgerRole BoarderRoles[] = { EDredgerRole::Rifleman, EDredgerRole::Rifleman, EDredgerRole::HoseMan };
	/** A boarder this far under her deck is gone (over the side). */
	constexpr float LostBelow = 600.f;

	// --- Her launchers and the plank (her frame, cm, Z from the deck's top) ---

	/** Fore and aft of the plank's gap on her starboard rail (x 150..450 round its hinge). */
	const FVector LauncherSpots[] = { FVector(520.0, ALamplighterShip::RailOut - 40.0, 0.0), FVector(-560.0, ALamplighterShip::RailOut - 40.0, 0.0) };
	/** The plank's hinge in the gap (fore of her locker bench; the barge's berth lines it up with its rail's gap), its swing
	 *  down (s), its width and its own length (SM_Skiff_Gangplank: 3 m). */
	const FVector PlankHinge(ALamplighterShip::PlankHingeX, ALamplighterShip::RailOut, 0.0);
	constexpr float PlankSwingSeconds = 0.8f;
	constexpr float PlankWidthScale = 1.3f;
	constexpr float PlankOwnLength = 300.f;
	constexpr float PlankHalfWidth = 55.f;

	/** The deck's marker for step 7's Reach: over her main deck's middle. */
	const FVector DeckMarkerLocal(0.0, 0.0, 90.0);

	APawn* SortiePlayer(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

// ---------------------------------------------------------------------------
// The tenders and their boarders
// ---------------------------------------------------------------------------

ACompanyTender* APilotSortie::GetTender(int32 Index) const
{
	return Tenders.IsValidIndex(Index) && IsValid(Tenders[Index]) ? Tenders[Index].Get() : nullptr;
}

void APilotSortie::CallTenders()
{
	if (bTendersCalled)
	{
		return;
	}
	bTendersCalled = true;
	ALamplighterShip* Boat = FindShip();
	LandfallRules::Say(this, PilotLight::TendersLines());
	if (!bSpawnTenders || !Boat)
	{
		// (Tests, or a level without the fight: the step's over at once.)
		CheckTendersDown();
		return;
	}
	const float Bag = BagHealth > 0.f ? BagHealth : PilotLight::BagHealthAt(PilotLight::FightLevel);
	Tenders.Reset();
	TenderSeenDown.Reset();
	Tenders.Add(ACompanyTender::SpawnEscort(*Boat, PortFrom, PortStation, -1.f, PortRunSeconds, Bag));
	Tenders.Add(ACompanyTender::SpawnEscort(*Boat, BoardFrom, BoardStation, 1.f, BoardRunSeconds, Bag));
	TenderSeenDown.Init(false, Tenders.Num());
	BoardClock = -1.f;
	bBoardRiflemen = bPortRiflemen = bBoardPlank = bBoarded = bBoardPulledOff = false;
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: two Company tenders come up from astern (their bags good for %.0f each)."), Bag);
}

void APilotSortie::AdvanceTenders(float DeltaSeconds)
{
	for (int32 Index = 0; Index < Tenders.Num(); ++Index)
	{
		ACompanyTender* Tender = GetTender(Index);
		const bool bDown = !Tender || Tender->IsDown();
		if (TenderSeenDown.IsValidIndex(Index) && !TenderSeenDown[Index] && bDown)
		{
			// One cooked: Junie's crow, then Holloway's word for the second.
			TenderSeenDown[Index] = true;
			++TendersFallen;
			LandfallRules::Say(this, PilotLight::TenderDownLines(TendersFallen));
		}
		if (!Tender)
		{
			continue;
		}
		Tender->Advance(DeltaSeconds);
		if (bDown)
		{
			continue;
		}
		if (Index == 0)
		{
			// The port tender on station: its riflemen up and firing across.
			if (!bPortRiflemen && Tender->IsAlongside())
			{
				bPortRiflemen = true;
				if (bSpawnBoarders)
				{
					Tender->SpawnRiflemen(PilotLight::FightLevel);
				}
			}
			continue;
		}
		// The boarder: alongside, its riflemen, its plank, its men over one after another, then it pulls off.
		if (BoardClock < 0.f && Tender->IsAlongside())
		{
			BoardClock = 0.f;
		}
		if (BoardClock < 0.f)
		{
			continue;
		}
		BoardClock += DeltaSeconds;
		if (!bBoardRiflemen && BoardClock >= RiflemenAt)
		{
			bBoardRiflemen = true;
			if (bSpawnBoarders)
			{
				Tender->SpawnRiflemen(PilotLight::FightLevel);
			}
		}
		if (!bBoardPlank && BoardClock >= PlankAt)
		{
			bBoardPlank = true;
			Tender->LowerGangplank();
		}
		if (!bBoarded && BoardClock >= FirstBoarderAt)
		{
			bBoarded = true;
			if (bSpawnBoarders)
			{
				for (int32 Man = 0; Man < UE_ARRAY_COUNT(BoarderRoles); ++Man)
				{
					FPendingBoarder Coming;
					Coming.Role = static_cast<uint8>(BoarderRoles[Man]);
					Coming.Spot = Man;
					Coming.At = FirstBoarderAt + BoarderSpacing * Man;
					PendingBoarders.Add(Coming);
				}
			}
			LandfallRules::Say(this, PilotLight::BoardersLines());
		}
		for (int32 Pending = PendingBoarders.Num() - 1; Pending >= 0; --Pending)
		{
			if (BoardClock >= PendingBoarders[Pending].At)
			{
				const FPendingBoarder Coming = PendingBoarders[Pending];
				PendingBoarders.RemoveAt(Pending);
				SpawnBoarder(Coming.Role, Coming.Spot);
			}
		}
		if (bBoarded && !bBoardPulledOff && PendingBoarders.IsEmpty()
			&& BoardClock >= FirstBoarderAt + BoarderSpacing * UE_ARRAY_COUNT(BoarderRoles) + PullOffAfter)
		{
			bBoardPulledOff = true;
			Tender->RaiseGangplank();
			Tender->MoveToStation(BoardStandOff, PullOffSeconds);
		}
	}
	// A boarder's tender gone before its men crossed: they went down with it.
	if (const ACompanyTender* Boarder = GetTender(1); (!Boarder || Boarder->IsDown()) && !PendingBoarders.IsEmpty())
	{
		PendingBoarders.Reset();
	}
}

void APilotSortie::SpawnBoarder(uint8 BoarderRole, int32 Spot)
{
	ALamplighterShip* Boat = FindShip();
	UWorld* World = GetWorld();
	const TSubclassOf<ACreatureBase> Class = ADredgerCreature::ClassFor(static_cast<EDredgerRole>(BoarderRole));
	if (!Boat || !World || !Class)
	{
		return;
	}
	const FVector Feet = Boat->DeckPoint(BoarderSpots[FMath::Clamp(Spot, 0, static_cast<int32>(UE_ARRAY_COUNT(BoarderSpots)) - 1)]);
	// Facing in off the plank, toward her middle (from her starboard: her -Y).
	const float Yaw = static_cast<float>(Boat->GetActorRotation().Yaw) - 90.f;
	ACreatureBase::FRuntimeSpawn Spawn;
	Spawn.bMayBeTouched = false;
	Spawn.Level = PilotLight::FightLevel;
	ACreatureBase* Boarder = ACreatureBase::SpawnAtRuntime(World, Class, Feet, Yaw, Spawn);
	if (!Boarder)
	{
		UE_LOG(LogLooter, Warning, TEXT("The Pilot Light: a boarder couldn't be spawned on her deck."));
		return;
	}
	if (APawn* Player = SortiePlayer(World))
	{
		Boarder->AlertTo(Player);
	}
	// Boots on her planks off the tender's plank.
	LooterSound::PlayAt(this, LooterSoundCue::ImpactWood, Feet);
	Boarders.Add(Boarder);
}

int32 APilotSortie::GetBoardersLeft() const
{
	const ALamplighterShip* Boat = Ship ? Ship.Get() : ShipNow.Get();
	const double FloorZ = Boat ? Boat->DeckPoint(FVector::ZeroVector).Z : -1.0e9;
	int32 Left = 0;
	for (const TWeakObjectPtr<ACreatureBase>& Boarder : Boarders)
	{
		const ACreatureBase* Standing = Boarder.Get();
		Left += Standing && !Standing->IsDead() && Standing->GetActorLocation().Z > FloorZ - LostBelow ? 1 : 0;
	}
	return Left + PendingBoarders.Num();
}

void APilotSortie::CheckTendersDown()
{
	if (bTendersDown || !bTendersCalled)
	{
		return;
	}
	for (int32 Index = 0; Index < Tenders.Num(); ++Index)
	{
		if (const ACompanyTender* Tender = GetTender(Index); Tender && !Tender->IsDown())
		{
			return;
		}
	}
	if (GetBoardersLeft() > 0)
	{
		return;
	}
	bTendersDown = true;
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(PilotLight::TendersDownEvent(), this));
	}
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: both tenders down and her deck clear (%s); on to the barge."),
		*PilotLight::TendersDownEvent().ToString());
}

void APilotSortie::TendersDownNow()
{
	bTendersCalled = true;
	DestroyTenders();
	CheckTendersDown();
}

bool APilotSortie::BurstTenderBag(int32 Index)
{
	ACompanyTender* Tender = GetTender(Index);
	return Tender && Tender->BurstBag(nullptr);
}

void APilotSortie::DestroyTenders()
{
	for (ACompanyTender* Tender : Tenders)
	{
		if (IsValid(Tender))
		{
			Tender->Destroy();
		}
	}
	Tenders.Reset();
	TenderSeenDown.Reset();
	PendingBoarders.Reset();
	for (const TWeakObjectPtr<ACreatureBase>& Boarder : Boarders)
	{
		if (ACreatureBase* Gone = Boarder.Get())
		{
			Gone->Destroy();
		}
	}
	Boarders.Reset();
	BoardClock = -1.f;
}

void APilotSortie::DestroyFight()
{
	DestroyTenders();
	for (ALamplighterLauncher* Launcher : Launchers)
	{
		if (IsValid(Launcher))
		{
			Launcher->Destroy();
		}
	}
	Launchers.Reset();
	if (IsValid(DeckMarker))
	{
		DeckMarker->Destroy();
	}
	DeckMarker = nullptr;
	bTendersCalled = false;
	bTendersDown = false;
	TendersFallen = 0;
	bPulled = false;
}

// ---------------------------------------------------------------------------
// Junie's launchers, the plank, the deck's marker
// ---------------------------------------------------------------------------

void APilotSortie::SpawnLaunchers()
{
	ALamplighterShip* Boat = FindShip();
	if (!Boat)
	{
		return;
	}
	Launchers.RemoveAll([](const TObjectPtr<ALamplighterLauncher>& Launcher) { return !IsValid(Launcher); });
	if (Launchers.Num() >= UE_ARRAY_COUNT(LauncherSpots))
	{
		return;
	}
	for (const FVector& Spot : LauncherSpots)
	{
		if (ALamplighterLauncher* Launcher = ALamplighterLauncher::SpawnOn(*Boat, Spot))
		{
			Launchers.Add(Launcher);
		}
	}
}

ALamplighterLauncher* APilotSortie::GetLauncher(int32 Index) const
{
	return Launchers.IsValidIndex(Index) && IsValid(Launchers[Index]) ? Launchers[Index].Get() : nullptr;
}

int32 APilotSortie::NumLaunchersBiting() const
{
	int32 Biting = 0;
	for (const ALamplighterLauncher* Launcher : Launchers)
	{
		Biting += IsValid(Launcher) && Launcher->IsBiting() ? 1 : 0;
	}
	return Biting;
}

int32 APilotSortie::FireLaunchers()
{
	int32 Fired = 0;
	for (int32 Index = 0; Index < Launchers.Num(); ++Index)
	{
		if (ALamplighterLauncher* Launcher = GetLauncher(Index))
		{
			Launcher->SetArmed(true);
			Fired += Launcher->Fire(GetGrappleAnchor(Index)) ? 1 : 0;
		}
	}
	return Fired;
}

void APilotSortie::AdvanceLaunchers(float DeltaSeconds)
{
	const bool bArmed = Phase == EPilotPhase::Alongside;
	const bool bStraining = Phase == EPilotPhase::Pulling;
	for (int32 Index = 0; Index < Launchers.Num(); ++Index)
	{
		if (ALamplighterLauncher* Launcher = GetLauncher(Index))
		{
			Launcher->SetArmed(bArmed);
			Launcher->Advance(DeltaSeconds, GetGrappleAnchor(Index));
			Launcher->SetStraining(bStraining);
		}
	}
}

void APilotSortie::CutLaunchers()
{
	int32 Cut = 0;
	for (ALamplighterLauncher* Launcher : Launchers)
	{
		Cut += IsValid(Launcher) && Launcher->Cut() ? 1 : 0;
	}
	if (Cut > 0)
	{
		// Holloway's axe: the chains part and she drops away from the barge's side.
		LamplighterEffects::Shake(this, 0.4f, 0.6f);
		if (ALamplighterShip* Boat = FindShip())
		{
			Boat->BraceCrew(1.5f);
			LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, Boat->GetActorLocation(), 0.8f, 1.1f);
		}
	}
}

void APilotSortie::ShowPlank(bool bShown, bool bAtOnce)
{
	bPlankWanted = bShown;
	if (bAtOnce)
	{
		PlankAlpha = bShown ? 1.f : 0.f;
	}
	PlacePlank(PlankAlpha);
}

void APilotSortie::AdvancePlank(float DeltaSeconds)
{
	const float Before = PlankAlpha;
	const float Goal = bPlankWanted ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(PlankAlpha, Goal))
	{
		return;
	}
	PlankAlpha = FMath::FInterpConstantTo(PlankAlpha, Goal, DeltaSeconds, 1.f / PlankSwingSeconds);
	if (Before < 1.f && PlankAlpha >= 1.f)
	{
		// Down on the barge's deck with a thud: the way over.
		if (const ABargeBoarding* Here = FindBarge())
		{
			LooterSound::PlayAt(this, LooterSoundCue::ImpactWood, Here->GetBoardingPoint());
			LamplighterEffects::DustSkirt(this, Here->GetBoardingPoint(), 120.f, 4);
		}
		LamplighterEffects::Shake(this, 0.15f, 0.3f);
	}
	PlacePlank(PlankAlpha);
}

void APilotSortie::PlacePlank(float Alpha)
{
	const ALamplighterShip* Boat = Ship ? Ship.Get() : ShipNow.Get();
	if (!Boat || Alpha <= KINDA_SMALL_NUMBER)
	{
		Plank->SetVisibility(false);
		PlankFloor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}
	const FVector Hinge = Boat->DeckPoint(PlankHinge);
	const ABargeBoarding* Here = FindBarge();
	const FVector End = Here ? Here->GetBoardingPoint() : Hinge + Boat->GetActorRightVector() * 500.0;
	const FVector Span = End - Hinge;
	const double Length = FMath::Max(Span.Size(), 50.0);
	const FVector Up = Boat->GetActorUpVector();
	const FVector Ahead = Boat->GetActorForwardVector();
	// Swung down from upright on its hinge to lie on the barge's deck.
	const FQuat Swing = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Up, Span / Length), FMath::SmoothStep(0.f, 1.f, Alpha));
	const FVector Along = Swing.RotateVector(Up);
	// SM_Skiff_Gangplank reaches along its own -Y from its hinge.
	Plank->SetWorldLocationAndRotation(Hinge, FRotationMatrix::MakeFromYX(-Along, Ahead).Rotator());
	Plank->SetWorldScale3D(FVector(PlankWidthScale, Length / PlankOwnLength, PlankWidthScale));
	Plank->SetVisibility(true);
	PlankFloor->SetBoxExtent(FVector(PlankHalfWidth, Length * 0.5, 5.0), false);
	PlankFloor->SetWorldLocationAndRotation(Hinge + Along * (Length * 0.5) - Up * 5.0, FRotationMatrix::MakeFromYX(Along, Ahead).Rotator());
	PlankFloor->SetCollisionEnabled(Alpha >= 1.f - KINDA_SMALL_NUMBER ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void APilotSortie::SpawnDeckMarker()
{
	ALamplighterShip* Boat = FindShip();
	if (IsValid(DeckMarker) || !Boat || !GetWorld())
	{
		return;
	}
	DeckMarker = ALandfallMarker::SpawnTagged(GetWorld(), PilotLight::DeckMarkerTag(), Boat->DeckPoint(DeckMarkerLocal), Boat->GetDeck());
}

// (The jump, the catch and the fall: PilotSortieJump.cpp.)
