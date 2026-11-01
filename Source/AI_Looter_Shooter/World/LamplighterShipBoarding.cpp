// ALamplighterShip: the fight on her deck: Vane on the horn, the tender's run in and its grapples, her lurch to a dead
// stop, the boarders' two waves over the gangplank, the deck clear, the cuts, and breaking free.

#include "World/LamplighterShip.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DredgerCreature.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Player/ViewKick.h"
#include "Story/VaneBroadcast.h"
#include "World/CompanyTender.h"
#include "World/LamplighterEffects.h"
#include "World/LandfallGrapple.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The tender is spawned astern this long after Vane breaks in (his first words first). */
	constexpr float TenderLeadSeconds = 2.f;
	/** The grapples wait for Vane to finish, but never longer than this after the call. */
	constexpr float LongestVaneWait = 30.f;
	/** She must be this close to the grapple stop (cm along her course) for the tender to fire: over open air. */
	constexpr float StopTolerance = 400.f;
	/** Each grapple bites her rail here (her frame: fore and aft of midships, on the tender's side, at the rail's top). */
	constexpr float BiteFore = 600.f;
	constexpr float BiteAft = -600.f;
	constexpr float BiteHeight = 140.f;

	/** The fight's beats from the grapples' bite (seconds): the tender's riflemen up, the plank down, the first wave over. */
	constexpr float RiflemenAt = 1.f;
	constexpr float PlankAt = 2.2f;
	constexpr float FirstWaveAt = 3.f;
	/** The second wave comes this long after the first is down, or once the first is down to one this long into it. */
	constexpr float WaveGap = 1.5f;
	constexpr float WaveTimeout = 25.f;
	/** Boarders cross the plank one after another, this far apart. */
	constexpr float BoarderSpacing = 0.7f;
	/** A boarder fallen this far below her deck (over the side, through the floor) is gone. */
	constexpr float LostBelow = 600.f;

	/** Where boarders come aboard (her frame, on the tender's side by the plank; Y is flipped to its side). */
	const FVector BoarderSpots[] = { FVector(60.0, 220.0, 0.0), FVector(-120.0, 200.0, 0.0), FVector(220.0, 180.0, 0.0), FVector(-300.0, 180.0, 0.0) };

	/** The two waves (the contract: 3 riflemen and a hose-man, 2 then 2). */
	struct FWaveMember
	{
		EDredgerRole Role;
		int32 Spot;
	};
	const FWaveMember FirstWave[] = { { EDredgerRole::Rifleman, 0 }, { EDredgerRole::Rifleman, 1 } };
	const FWaveMember SecondWave[] = { { EDredgerRole::HoseMan, 2 }, { EDredgerRole::Rifleman, 3 } };

	APawn* FindPlayer(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

ACompanyTender* ALamplighterShip::GetTender() const
{
	return Tender;
}

void ALamplighterShip::CallTender()
{
	if (bTenderCalled || Flight != EShipFlight::Sailing)
	{
		return;
	}
	bTenderCalled = true;
	SinceCalled = 0.f;
	// Vane breaks in on her horn as the player reaches the Lantern: the radio-in, his words, the hiss.
	UVaneBroadcast::Say(this, LandfallRules::LoadLines(LandfallRules::VaneHornLines()));
	UE_LOG(LogLooter, Log, TEXT("Landfall: the player is at the prow; Vane on the horn, and the Company tender comes."));
}

void ALamplighterShip::AdvanceTender(float DeltaSeconds)
{
	if (!bTenderCalled)
	{
		return;
	}
	SinceCalled += DeltaSeconds;
	if (!Tender && SinceCalled >= TenderLeadSeconds && Flight == EShipFlight::Sailing)
	{
		Tender = ACompanyTender::SpawnFor(*this);
	}
	if (!Tender || bGrapplesFired || Flight != EShipFlight::Sailing)
	{
		return;
	}
	const bool bAtStop = Distance >= GetGrappleStopDistance() - StopTolerance || Distance >= Path.GetLength() - 1.f;
	const bool bVaneDone = !UVaneBroadcast::IsSpeaking(this) || SinceCalled >= LongestVaneWait;
	if (Tender->IsAlongside() && bAtStop && bVaneDone)
	{
		FireGrapples();
	}
}

void ALamplighterShip::AdvancePieces(float DeltaSeconds)
{
	if (IsValid(Tender))
	{
		Tender->Advance(DeltaSeconds);
	}
	else
	{
		Tender = nullptr;
	}
	// A copy: a grapple's flight can end in its bite, and the bite in her being pinned.
	const TArray<TObjectPtr<ALandfallGrapple>> Moving = Grapples;
	for (ALandfallGrapple* Grapple : Moving)
	{
		if (IsValid(Grapple))
		{
			Grapple->Advance(DeltaSeconds);
		}
	}
	Grapples.RemoveAll([](const TObjectPtr<ALandfallGrapple>& Grapple) { return !IsValid(Grapple); });
}

void ALamplighterShip::FireGrapples()
{
	if (bGrapplesFired || !Tender)
	{
		return;
	}
	bGrapplesFired = true;
	const float Side = TenderSide >= 0.f ? 1.f : -1.f;
	const FVector Bites[] = { FVector(BiteFore, Side * RailOut, DeckTop + BiteHeight), FVector(BiteAft, Side * RailOut, DeckTop + BiteHeight) };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Bites); ++Index)
	{
		if (ALandfallGrapple* Grapple = ALandfallGrapple::Fire(*this, *Tender, Index, Bites[Index]))
		{
			Grapples.Add(Grapple);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Landfall: the tender fires its grapples."));
}

void ALamplighterShip::GrappleNow()
{
	if (Flight != EShipFlight::Sailing)
	{
		return;
	}
	bTenderCalled = true;
	if (!Tender)
	{
		Tender = ACompanyTender::SpawnFor(*this);
	}
	if (!Tender)
	{
		return;
	}
	Tender->SetAlongsideNow();
	FireGrapples();
	const TArray<TObjectPtr<ALandfallGrapple>> Fired = Grapples;
	for (ALandfallGrapple* Grapple : Fired)
	{
		if (IsValid(Grapple))
		{
			Grapple->Bite();
		}
	}
}

int32 ALamplighterShip::CountGrapples(bool bBitingOnly) const
{
	int32 Count = 0;
	for (const ALandfallGrapple* Grapple : Grapples)
	{
		Count += IsValid(Grapple) && (!bBitingOnly || Grapple->IsBiting()) ? 1 : 0;
	}
	return Count;
}

void ALamplighterShip::NotifyGrappleBit(ALandfallGrapple& Grapple)
{
	if (Flight == EShipFlight::Sailing && CountGrapples(true) >= 2)
	{
		PinnedByGrapples();
	}
}

void ALamplighterShip::PinnedByGrapples()
{
	LurchFrom = GetActorTransform();
	GrappleSpot = CalmTransform;
	GrappleSpot.SetScale3D(GetActorScale3D());
	Speed = 0.f;
	GrappledClock = 0.f;
	WaveClock = 0.f;
	EnterFlight(EShipFlight::Grappled);
	// Snubbed by the lines: the whole ship jerks, the crew grab hold, the timbers groan.
	LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, GetActorLocation());
	LamplighterEffects::Shake(this, 0.8f, 1.f);
	FViewKick Jolt;
	Jolt.Pitch = -2.5f;
	Jolt.Roll = (TenderSide >= 0.f ? 1.f : -1.f) * 1.5f;
	Jolt.Frequency = 6.f;
	Jolt.Damping = 0.4f;
	LamplighterEffects::Kick(this, Jolt);
	BraceCrew(2.f);
	LandfallRules::Say(this, LandfallRules::BoarderLines());
	UE_LOG(LogLooter, Log, TEXT("Landfall: grappled; she's pinned dead still, and the boarders are coming."));
}

void ALamplighterShip::AdvanceBoarding(float DeltaSeconds)
{
	WaveClock = GrappledClock;
	if (!bRiflemenOut && GrappledClock >= RiflemenAt)
	{
		bRiflemenOut = true;
		if (bSpawnBoarders && Tender)
		{
			Tender->SpawnRiflemen();
		}
	}
	if (!bPlankDown && GrappledClock >= PlankAt)
	{
		bPlankDown = true;
		if (Tender)
		{
			Tender->LowerGangplank();
		}
	}
	if (WavesSpawned == 0 && GrappledClock >= FirstWaveAt)
	{
		SpawnWave(0);
	}
	// Each boarder over the plank in its turn.
	for (int32 Index = PendingBoarders.Num() - 1; Index >= 0; --Index)
	{
		if (GrappledClock >= PendingBoarders[Index].At)
		{
			const FPendingBoarder Coming = PendingBoarders[Index];
			PendingBoarders.RemoveAt(Index);
			SpawnBoarder(Coming.Role, Coming.Spot);
		}
	}
	if (WavesSpawned == 1 && PendingBoarders.IsEmpty())
	{
		const int32 Left = GetBoardersLeft();
		if (NextWaveAt < 0.f && (Left == 0 || (Left <= 1 && GrappledClock >= FirstWaveAt + WaveTimeout)))
		{
			NextWaveAt = GrappledClock + WaveGap;
		}
		if (NextWaveAt >= 0.f && GrappledClock >= NextWaveAt)
		{
			SpawnWave(1);
		}
	}
	if (WavesSpawned >= 2 && !bDeckClear)
	{
		CheckDeckClear();
	}
}

void ALamplighterShip::SpawnWave(int32 Wave)
{
	WavesSpawned = FMath::Max(WavesSpawned, Wave + 1);
	if (!bSpawnBoarders)
	{
		return;
	}
	const TArrayView<const FWaveMember> Members = Wave == 0 ? MakeArrayView(FirstWave) : MakeArrayView(SecondWave);
	float At = GrappledClock;
	for (const FWaveMember& Member : Members)
	{
		FPendingBoarder Coming;
		Coming.Role = static_cast<uint8>(Member.Role);
		Coming.Spot = Member.Spot;
		Coming.At = At;
		PendingBoarders.Add(Coming);
		At += BoarderSpacing;
	}
	UE_LOG(LogLooter, Log, TEXT("Landfall: boarders, wave %d of 2."), Wave + 1);
}

void ALamplighterShip::SpawnBoarder(uint8 BoarderRole, int32 Spot)
{
	UWorld* World = GetWorld();
	const TSubclassOf<ACreatureBase> Class = ADredgerCreature::ClassFor(static_cast<EDredgerRole>(BoarderRole));
	if (!World || !Class)
	{
		return;
	}
	const float Side = TenderSide >= 0.f ? 1.f : -1.f;
	FVector Local = BoarderSpots[FMath::Clamp(Spot, 0, static_cast<int32>(UE_ARRAY_COUNT(BoarderSpots)) - 1)];
	Local.Y *= Side;
	const FVector Feet = DeckPoint(Local);
	// Facing in off the plank, toward her middle.
	const float Yaw = GetActorRotation().Yaw - Side * 90.f;
	ACreatureBase::FRuntimeSpawn Spawn;
	Spawn.bMayBeTouched = false;
	ACreatureBase* Boarder = ACreatureBase::SpawnAtRuntime(World, Class, Feet, Yaw, Spawn);
	if (!Boarder)
	{
		UE_LOG(LogLooter, Warning, TEXT("Landfall: a boarder couldn't be spawned on deck."));
		return;
	}
	if (APawn* Player = FindPlayer(World))
	{
		Boarder->AlertTo(Player);
	}
	// Boots on her planks off the gangplank.
	LooterSound::PlayAt(this, LooterSoundCue::ImpactWood, Feet);
	Boarders.Add(Boarder);
}

int32 ALamplighterShip::GetBoardersLeft() const
{
	int32 Left = 0;
	const float FloorZ = DeckPoint(FVector::ZeroVector).Z;
	for (const TWeakObjectPtr<ACreatureBase>& Boarder : Boarders)
	{
		const ACreatureBase* Standing = Boarder.Get();
		// Over the side or through her floor counts as gone: a boarder lost below can't hold the deck.
		Left += Standing && !Standing->IsDead() && Standing->GetActorLocation().Z > FloorZ - LostBelow ? 1 : 0;
	}
	return Left + PendingBoarders.Num();
}

void ALamplighterShip::CheckDeckClear()
{
	if (bDeckClear || WavesSpawned < 2 || GetBoardersLeft() > 0)
	{
		return;
	}
	bDeckClear = true;
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(LandfallRules::DeckClearEvent(), this));
	}
	LandfallRules::Say(this, LandfallRules::GrappleLines());
	UE_LOG(LogLooter, Log, TEXT("Landfall: the deck is clear (%s); the grapple lines can be cut."), *LandfallRules::DeckClearEvent().ToString());
}

void ALamplighterShip::ClearDeck()
{
	PendingBoarders.Reset();
	for (const TWeakObjectPtr<ACreatureBase>& Boarder : Boarders)
	{
		if (ACreatureBase* Gone = Boarder.Get())
		{
			Gone->Destroy();
		}
	}
	Boarders.Reset();
	if (Flight == EShipFlight::Grappled)
	{
		WavesSpawned = 2;
		CheckDeckClear();
	}
}

bool ALamplighterShip::CanCutGrapples() const
{
	return Flight == EShipFlight::Grappled && bDeckClear;
}

void ALamplighterShip::NotifyGrappleCut(ALandfallGrapple& Grapple)
{
	if (Flight == EShipFlight::Grappled && CountGrapples(true) == 0)
	{
		BreakFree();
	}
}

void ALamplighterShip::BreakFree()
{
	BreakFrom = GetActorTransform();
	BreakClock = 0.f;
	LastLanding = 0.f;
	GasWait = 0.f;
	SparkWait = 0.f;
	bTouched = false;
	EnterFlight(EShipFlight::Breaking);
	if (Tender)
	{
		Tender->FallAway();
	}
	// Free, and torn: the last line whips through her bag as it parts. A ripping hiss, a lurch up and over, the crew thrown.
	const FVector Bag = GetActorTransform().TransformPosition(FVector(0.0, 0.0, 1425.0));
	LooterSound::PlayAt(this, LooterSoundCue::ShipBagTear, Bag);
	LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, GetActorLocation());
	LamplighterEffects::Shake(this, 0.9f, 1.2f);
	FViewKick Jolt;
	Jolt.Pitch = 3.f;
	Jolt.Roll = -(TenderSide >= 0.f ? 1.f : -1.f) * 2.f;
	Jolt.Frequency = 5.f;
	Jolt.Damping = 0.35f;
	LamplighterEffects::Kick(this, Jolt);
	BraceCrew(3.f);
	LandfallRules::Say(this, LandfallRules::LandingLines(), /*bInterrupt*/ true);
	UE_LOG(LogLooter, Log, TEXT("Landfall: both lines cut; she breaks free with her bag torn."));
}
