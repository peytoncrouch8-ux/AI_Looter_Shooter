// Main 6's rules apart from the world: the names the missions, the barge and the ship share, where a level begins her,
// and her moves as math of time (the lift, the standoff, the glides, the settle, the jump's catch).

#include "World/PilotSortieRules.h"
#include "Progression/LevelRules.h"
#include "Progression/ProgressionSettings.h"

namespace
{
	/** Her bag's lazy pull (cm/s/s): she speeds up and brakes no harder than this. */
	constexpr float Pull = 120.f;

	struct FPhaseName
	{
		EPilotPhase Phase;
		const TCHAR* Name;
	};
	const FPhaseName PhaseNames[] = {
		{ EPilotPhase::Idle, TEXT("Moored") },
		{ EPilotPhase::WaitCastOff, TEXT("WaitCastOff") },
		{ EPilotPhase::LiftOff, TEXT("LiftOff") },
		{ EPilotPhase::Cruise, TEXT("Flying") },
		{ EPilotPhase::Approach, TEXT("Approach") },
		{ EPilotPhase::Alongside, TEXT("Alongside") },
		{ EPilotPhase::Pulling, TEXT("Pulling") },
		{ EPilotPhase::Boarded, TEXT("Grappled") },
		{ EPilotPhase::Jump, TEXT("Jump") },
		{ EPilotPhase::Falling, TEXT("Falling") },
		{ EPilotPhase::Home, TEXT("Home") },
		{ EPilotPhase::Settling, TEXT("Settling") },
	};
}

namespace PilotLight
{
	FName MissionId() { return FName(Mission); }
	FName TendersDownEvent() { return FName(TEXT("Pilot.TendersDown")); }
	FName DrumJammedFlag() { return FName(TEXT("Pilot.DrumJammed")); }
	FName BargeFellFlag() { return FName(TEXT("Pilot.BargeFell")); }
	FName LauncherTag() { return FName(TEXT("Lamplighter_GrappleLauncher")); }
	FName DeckMarkerTag() { return FName(TEXT("Lamplighter_Deck")); }
	FName BargeTag() { return FName(TEXT("Barge_Pilot")); }

	FName CastOffLines() { return FName(TEXT("DA_Lines_Main6CastOff")); }
	FName TendersLines() { return FName(TEXT("DA_Lines_Main6Tenders")); }
	FName TenderDownLines(int32 Fallen) { return Fallen <= 1 ? FName(TEXT("DA_Lines_Main6TenderDown1")) : FName(TEXT("DA_Lines_Main6TenderDown2")); }
	FName BoardersLines() { return FName(TEXT("DA_Lines_Main6Boarders")); }
	FName LaunchersLines() { return FName(TEXT("DA_Lines_Main6Launchers")); }
	FName FlickerLines() { return FName(TEXT("DA_Lines_Main6Flicker")); }
	FName PlankLines() { return FName(TEXT("DA_Lines_Main6Plank")); }
	FName JumpLines() { return FName(TEXT("DA_Lines_Main6Jump")); }

	const TCHAR* PhaseName(EPilotPhase Phase)
	{
		for (const FPhaseName& Each : PhaseNames)
		{
			if (Each.Phase == Phase)
			{
				return Each.Name;
			}
		}
		return TEXT("?");
	}

	bool ParsePhase(const FString& Words, EPilotPhase& OutPhase)
	{
		for (const FPhaseName& Each : PhaseNames)
		{
			if (Words.Equals(Each.Name, ESearchCase::IgnoreCase))
			{
				OutPhase = Each.Phase;
				return true;
			}
		}
		// The console's other words for them.
		if (Words.Equals(TEXT("Tenders"), ESearchCase::IgnoreCase))
		{
			OutPhase = EPilotPhase::Cruise;
			return true;
		}
		if (Words.Equals(TEXT("Boarded"), ESearchCase::IgnoreCase))
		{
			OutPhase = EPilotPhase::Boarded;
			return true;
		}
		return false;
	}

	EPilotPhase PhaseForStory(bool bRunning, int32 Step, bool bReady)
	{
		if (!bRunning || bReady || Step >= ReadyStep || Step < 0)
		{
			return EPilotPhase::Idle;
		}
		if (Step == CastOffStep)
		{
			return EPilotPhase::WaitCastOff;
		}
		if (Step <= GrappleStep)
		{
			// The flight again from the lift (a save mid-air would wake the player in the sky).
			return EPilotPhase::LiftOff;
		}
		if (Step < EscapeStep)
		{
			return EPilotPhase::Boarded;
		}
		return EPilotPhase::Jump;
	}

	bool IsFlying(EPilotPhase Phase)
	{
		switch (Phase)
		{
		case EPilotPhase::LiftOff:
		case EPilotPhase::Cruise:
		case EPilotPhase::Approach:
		case EPilotPhase::Alongside:
		case EPilotPhase::Pulling:
		case EPilotPhase::Jump:
		case EPilotPhase::Falling:
		case EPilotPhase::Home:
		case EPilotPhase::Settling:
			return true;
		default:
			return false;
		}
	}

	bool KeepsPlayerAboard(EPilotPhase Phase)
	{
		// Over on the barge (the berth) or on their way down to her (the jump) they're the barge's (W3) or the catch's.
		return IsFlying(Phase) && Phase != EPilotPhase::Jump;
	}

	float SpeedFor(float Remaining, float Wanted)
	{
		// v squared = 2as: never more than she can brake to nothing in what's left.
		const float Stoppable = FMath::Sqrt(2.f * Pull * FMath::Max(Remaining, 0.f));
		return FMath::Clamp(FMath::Min(Wanted, Stoppable), 0.f, Wanted);
	}

	float BagHealthAt(int32 Level)
	{
		const FLevelRules Rules = GetDefault<UProgressionSettings>()->GetLevelRules();
		return BagBaseHealth * Rules.EnemyScale(FMath::Max(Level, 1));
	}

	FTransform Glide(const FTransform& From, const FTransform& To, float Alpha)
	{
		const float Eased = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
		return FTransform(FQuat::Slerp(From.GetRotation(), To.GetRotation(), Eased),
			FMath::Lerp(From.GetLocation(), To.GetLocation(), static_cast<double>(Eased)), From.GetScale3D());
	}

	FTransform LiftPose(const FTransform& Moored, const FTransform& Start, float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		// Up first, so her keel clears the ledge and the cliff before she moves an inch over; then out and round. (Up by
		// half the lift: she's two thirds of the way up when Out starts at 0.3; 0.55 had her barely past half.)
		const float Up = FMath::SmoothStep(0.f, 0.5f, A);
		const float Out = FMath::SmoothStep(0.3f, 1.f, A);
		const float Round = FMath::SmoothStep(0.2f, 1.f, A);
		const FVector Raised = Moored.GetLocation() + FVector(0.0, 0.0, LiftRise * Up);
		FVector Where = FMath::Lerp(Raised, Start.GetLocation(), static_cast<double>(Out));
		// Never under the raised height until she's well out (the course's start may sit lower over the drop).
		Where.Z = FMath::Max(Where.Z, FMath::Lerp(Raised.Z, Start.GetLocation().Z, static_cast<double>(Out)));
		const FQuat Turn = FQuat::Slerp(Moored.GetRotation(), Start.GetRotation(), Round);
		// The bag takes her weight: a little nose-up and a list that dies away as she gets her way on.
		const float Settle = FMath::Sin(UE_PI * A);
		const FQuat Heave = FQuat(FVector::YAxisVector, FMath::DegreesToRadians(-2.5f * Settle))
			* FQuat(FVector::XAxisVector, FMath::DegreesToRadians(1.8f * Settle * FMath::Sin(UE_TWO_PI * 0.6f * A * LiftSeconds)));
		return FTransform(Turn * Heave, Where, Moored.GetScale3D());
	}

	FTransform StandoffFor(const FTransform& Berth)
	{
		// Her port is -Y in her own frame (her starboard is to the barge).
		const FVector Off = Berth.GetRotation().RotateVector(FVector(0.0, -StandoffOut, 0.0)) + FVector(0.0, 0.0, StandoffUp);
		return FTransform(Berth.GetRotation(), Berth.GetLocation() + Off, Berth.GetScale3D());
	}

	FTransform SettlePose(const FTransform& From, const FTransform& Moored, float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		// Across early, down late and slow: Holloway feels for the ledge with the last metre.
		const float Across = FMath::SmoothStep(0.f, 0.7f, A);
		const float Down = FMath::InterpEaseInOut(0.f, 1.f, FMath::SmoothStep(0.2f, 1.f, A), 2.2f);
		FVector Where = FMath::Lerp(From.GetLocation(), Moored.GetLocation(), static_cast<double>(Across));
		Where.Z = FMath::Lerp(From.GetLocation().Z, Moored.GetLocation().Z, static_cast<double>(Down));
		return FTransform(FQuat::Slerp(From.GetRotation(), Moored.GetRotation(), FMath::SmoothStep(0.f, 0.85f, A)), Where, From.GetScale3D());
	}

	bool IsCaughtBelow(const FVector& Local, bool bFalling, float DeckTop, float RailOut)
	{
		// Over the barge's side (beyond her starboard rail by more than a long step) they're still in its hull or on its deck.
		const bool bBesideHer = Local.Y < RailOut + 300.f;
		return bFalling && bBesideHer && Local.Z < DeckTop - CatchBelow;
	}
}
