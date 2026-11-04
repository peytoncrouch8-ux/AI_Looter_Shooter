// UUnpaidPackSubsystem's one blink gate (2026-11-03, the audit's change 11; Docs/Work/2026-11-03-Phase3/UNPAID.md). The Unpaid's
// teleport was decided in seven places (the phase-step, the press step, the step to sight, the flank, the phase-strike, the
// fade-back and the Phasing trait, which asked no gate at all): up to ~50 blinks a minute at one player, flanks landing 3.3 m
// behind them at up to 180 degrees. Now each keeps its own reasons to go and asks this one rule whether it may: never two at
// one player within Looter.Unpaid.BlinkGap, never more than Looter.Unpaid.BlinksPerMinute in any minute across the pack, never
// a landing behind their back nearer than Looter.Unpaid.BlinkNearestBehind, a flank's within Looter.Unpaid.FlankArcMin-Max of
// their view (they see it come at the edge of the screen). The rules are plain (FUnpaidBlinkGate, UnpaidBlinkRules).

#include "Creatures/UnpaidPackSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
#if !UE_BUILD_SHIPPING
	/** Looter.Unpaid.Blinks: the blinks at each player in the last minute against the budget (the PIE check: "count blinks in a minute"). */
	FAutoConsoleCommandWithWorldAndArgs BlinksCommand(TEXT("Looter.Unpaid.Blinks"),
		TEXT("Prints the Unpaid's blinks at the first player in the last minute, against Looter.Unpaid.BlinksPerMinute."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(World);
			const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
			if (!Packs || !Pawn)
			{
				UE_LOG(LogLooter, Warning, TEXT("UNPAID BLINKS: no player or no Unpaid pack subsystem in this world (play first)."));
				return;
			}
			const FUnpaidFightTuning Tuning = FUnpaidFightTuning::Live();
			UE_LOG(LogLooter, Display, TEXT("UNPAID BLINKS: %d at %s in the last minute (budget %d, gap %.1f s)."), Packs->BlinksInMinute(*Pawn),
				*Pawn->GetName(), Tuning.BlinksPerMinute, Tuning.BlinkGap);
		}));
#endif
}

FUnpaidBlinkGate& UUnpaidPackSubsystem::GateFor(const APawn& Target)
{
	// Gates of players gone are dropped as a new one is made (a handful of entries at most).
	if (!Blinks.Contains(TWeakObjectPtr<const APawn>(&Target)))
	{
		for (auto It = Blinks.CreateIterator(); It; ++It)
		{
			if (!It->Key.IsValid())
			{
				It.RemoveCurrent();
			}
		}
	}
	return Blinks.FindOrAdd(TWeakObjectPtr<const APawn>(&Target));
}

bool UUnpaidPackSubsystem::MayBlink(const APawn& Target, uint32 Id, bool bOwed)
{
	const FUnpaidFightTuning Tuning = FUnpaidFightTuning::Live();
	return GateFor(Target).MayBlink(Id, bOwed, Now(), Tuning.BlinkGap, Tuning.BlinksPerMinute);
}

void UUnpaidPackSubsystem::NoteBlink(const APawn& Target, uint32 Id)
{
	GateFor(Target).Note(Id, Now());
	Count(EUnpaidTally::Blink);
}

int32 UUnpaidPackSubsystem::BlinksInMinute(const APawn& Target) const
{
	const FUnpaidBlinkGate* Gate = Blinks.Find(TWeakObjectPtr<const APawn>(&Target));
	return Gate ? Gate->CountInMinute(Now()) : 0;
}

bool UUnpaidPackSubsystem::MayLandAt(const APawn& Target, const FVector& Feet, bool bFlank)
{
	// The way they look now: a player's control rotation (a pawn with none, its own facing).
	return UnpaidBlinkRules::MayLand(Target.GetActorLocation(), static_cast<float>(Target.GetViewRotation().Yaw), Feet, bFlank,
		FUnpaidFightTuning::LiveLanding());
}
