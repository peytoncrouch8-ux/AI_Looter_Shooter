// ASetPieceSpawner's arena (FIGHTS lane 6, 2026-10-28: Saint Ada's Yard, the chapel yard as a set piece): fog walls across the
// arena's openings (ABossSeal, Gate shape) that rise once the player is inside and it's fighting, and drop when it's won, taken
// away or off, or the player dies. Springing, beats and its end are in SetPieceSpawner.cpp; the rules (when they rise and
// drop, where a wall faces) are SetPieceRules.

#include "Creatures/SetPieceSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossSeal.h"
#include "Combat/HealthComponent.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/SetPieceRules.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A wall shorter than this (cm) closes nothing: left out. */
	constexpr double ShortestGate = 100.0;
}

ABossSeal* ASetPieceSpawner::GetGateSeal(int32 Index) const
{
	return GateSeals.IsValidIndex(Index) ? GateSeals[Index].Get() : nullptr;
}

bool ASetPieceSpawner::IsInsideArena(const FVector& Point, float Margin) const
{
	// The spawner is inside its arena, so each wall faces it.
	for (const FSetPieceGate& Gate : ArenaGates)
	{
		if (!SetPieceRules::IsAheadOfGate(SetPieceRules::GateFrame(Gate.Start, Gate.End, GetActorLocation()), Point, Margin))
		{
			return false;
		}
	}
	return true;
}

void ASetPieceSpawner::UpdateArena()
{
	if (ArenaGates.IsEmpty())
	{
		return;
	}
	const bool bArenaOn = FSetPieceTuning::Current().bArena;
	const UEncounterSubsystem* Encounters = GetEncounters();
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	const bool bPlayerAlive = Player && !(Health && Health->IsDead());
	if (SetPieceRules::ShouldDropGates(bGatesUp, bArenaOn, GetState(), bDone, bPlayerAlive))
	{
		DropGates();
	}
	else if (Player && SetPieceRules::ShouldRaiseGates(bGatesUp, bArenaOn, GetState(), GetWavesStarted(), bDone, bPlayerAlive,
		IsInsideArena(Player->GetActorLocation(), ArenaMargin)))
	{
		RaiseGates(*Player);
	}
}

void ASetPieceSpawner::RaiseGates(const APawn& Player)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	GateSeals.SetNum(ArenaGates.Num());
	int32 Raised = 0;
	for (int32 Index = 0; Index < ArenaGates.Num(); ++Index)
	{
		const SetPieceRules::FGateFrame Frame = SetPieceRules::GateFrame(ArenaGates[Index].Start, ArenaGates[Index].End, GetActorLocation());
		if (Frame.Length < ShortestGate)
		{
			UE_LOG(LogLooter, Warning, TEXT("Set piece %s: its gate %d is shorter than %.0f cm; left out."), *GetSpawnerId().ToString(), Index, ShortestGate);
			continue;
		}
		ABossSeal* Seal = GateSeals[Index].Get();
		if (!Seal)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags |= RF_Transient;
			Seal = World->SpawnActor<ABossSeal>(ABossSeal::StaticClass(),
				FTransform(FRotator(0.0, Frame.Yaw, 0.0), Frame.Origin), Params);
			GateSeals[Index] = Seal;
		}
		if (!Seal)
		{
			continue;
		}
		// Turned so its +Y runs along the wall and its +X (the arena's side, ABossSeal::IsInside) faces the spawner; the far
		// end at its own height.
		Seal->SetActorLocationAndRotation(Frame.Origin, FRotator(0.0, Frame.Yaw, 0.0));
		Seal->Shape = EBossSealShape::Gate;
		Seal->GateEnd = FVector(0.0, Frame.Length, Frame.Rise);
		Seal->Raise();
		++Raised;
	}
	if (Raised == 0)
	{
		return;
	}
	bGatesUp = true;
	// The player's death opens them at once (not at the next look).
	if (UHealthComponent* Health = Player.FindComponentByClass<UHealthComponent>())
	{
		Health->OnDeath.AddUniqueDynamic(this, &ASetPieceSpawner::HandleArenaPlayerDeath);
		WatchedPlayerHealth = Health;
	}
	UE_LOG(LogLooter, Log, TEXT("Set piece %s: its %d fog gate(s) close."), *GetSpawnerId().ToString(), Raised);
}

void ASetPieceSpawner::DropGates()
{
	if (UHealthComponent* Health = WatchedPlayerHealth.Get())
	{
		Health->OnDeath.RemoveDynamic(this, &ASetPieceSpawner::HandleArenaPlayerDeath);
	}
	WatchedPlayerHealth.Reset();
	if (!bGatesUp)
	{
		return;
	}
	bGatesUp = false;
	for (const TWeakObjectPtr<ABossSeal>& Each : GateSeals)
	{
		if (ABossSeal* Seal = Each.Get())
		{
			Seal->Drop();
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Set piece %s: its fog gates open."), *GetSpawnerId().ToString());
}

void ASetPieceSpawner::DestroyGates()
{
	DropGates();
	for (const TWeakObjectPtr<ABossSeal>& Each : GateSeals)
	{
		if (ABossSeal* Seal = Each.Get())
		{
			Seal->Destroy();
		}
	}
	GateSeals.Reset();
}

void ASetPieceSpawner::HandleArenaPlayerDeath(AController* Killer)
{
	// The fight goes on (or is taken away as the player is far at their grave); the wall must not keep anyone in.
	DropGates();
}
