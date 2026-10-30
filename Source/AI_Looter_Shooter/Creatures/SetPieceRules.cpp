#include "Creatures/SetPieceRules.h"
#include "HAL/IConsoleManager.h"

// The Looter.Fight.SetPiece.* cvars (FIGHT-LAYOUT): their defaults are FSetPieceTuning's own field values, so the rules'
// tests (a default-made tuning) and a fresh game agree.

namespace
{
	const FSetPieceTuning SetPieceDefaults;

	TAutoConsoleVariable<float> CVarShakeScale(TEXT("Looter.Fight.SetPiece.ShakeScale"), SetPieceDefaults.ShakeScale,
		TEXT("Every set piece beat's camera shake, times (0: none)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarPushWhenLeft(TEXT("Looter.Fight.SetPiece.PushWhenLeft"), SetPieceDefaults.PushWhenLeft,
		TEXT("A set piece's next push comes once this many of the last are left standing; -1: each set piece's own (the layout's)."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarDustScale(TEXT("Looter.Fight.SetPiece.DustScale"), SetPieceDefaults.DustScale,
		TEXT("The dust a cave-in's beat sifts down from the roof, times."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarRewardGuns(TEXT("Looter.Fight.SetPiece.RewardGuns"), SetPieceDefaults.RewardGuns,
		TEXT("Guns in a set piece's loot shower at its end."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRewardLuck(TEXT("Looter.Fight.SetPiece.RewardLuck"), SetPieceDefaults.RewardLuck,
		TEXT("The luck of a set piece's reward guns (a Strongbox's is 1)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarRewardAmmo(TEXT("Looter.Fight.SetPiece.RewardAmmo"), SetPieceDefaults.RewardAmmo,
		TEXT("Ammo pickups in a set piece's loot shower (a chest's 1.5 boxes each)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarRewardBonusAmmo(TEXT("Looter.Fight.SetPiece.RewardBonusAmmo"), SetPieceDefaults.RewardBonusAmmo,
		TEXT("Extra chest-full ammo boxes on top of a set piece's shower."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarArena(TEXT("Looter.Fight.SetPiece.Arena"), SetPieceDefaults.bArena ? 1 : 0,
		TEXT("1: an arena set piece's fog gates close at its first push (the chapel yard's); 0: they never do."), ECVF_Default);
}

FSetPieceTuning FSetPieceTuning::Current()
{
	FSetPieceTuning Made;
	Made.ShakeScale = FMath::Max(CVarShakeScale.GetValueOnGameThread(), 0.f);
	Made.PushWhenLeft = CVarPushWhenLeft.GetValueOnGameThread();
	Made.DustScale = FMath::Max(CVarDustScale.GetValueOnGameThread(), 0.f);
	Made.RewardGuns = FMath::Max(CVarRewardGuns.GetValueOnGameThread(), 0);
	Made.RewardLuck = FMath::Max(CVarRewardLuck.GetValueOnGameThread(), 0.f);
	Made.RewardAmmo = FMath::Max(CVarRewardAmmo.GetValueOnGameThread(), 0);
	Made.RewardBonusAmmo = FMath::Clamp(CVarRewardBonusAmmo.GetValueOnGameThread(), 0, 10);
	Made.bArena = CVarArena.GetValueOnGameThread() != 0;
	return Made;
}

bool SetPieceRules::IsPushDue(EEncounterState State, int32 WavesStarted, int32 PlannedWaves, int32 Remaining, int32 PushWhenLeft)
{
	// The first wave is the spring's; later ones are pushes, each on top of the last one's stragglers.
	return State == EEncounterState::Engaged && WavesStarted >= 1 && WavesStarted < PlannedWaves
		&& Remaining <= FMath::Max(PushWhenLeft, 0);
}

int32 SetPieceRules::PushThreshold(int32 Own, int32 Override)
{
	return Override >= 0 ? Override : FMath::Max(Own, 0);
}

bool SetPieceRules::IsDone(EEncounterState State, int32 WavesStarted, int32 PlacedAlive, int32 PlacedOwed)
{
	return State == EEncounterState::Cleared && WavesStarted > 0 && PlacedAlive <= 0 && PlacedOwed <= 0;
}

bool SetPieceRules::KeepsHushed(EEncounterState State, int32 WavesStarted, bool bDone)
{
	return !bDone && WavesStarted > 0 && State != EEncounterState::Off;
}

bool SetPieceRules::IsHeld(TConstArrayView<EEncounterState> Waited)
{
	for (const EEncounterState Each : Waited)
	{
		if (Each == EEncounterState::Engaged)
		{
			return true;
		}
	}
	return false;
}

int32 SetPieceRules::FindBeat(TConstArrayView<int32> BeatWaves, int32 Wave)
{
	for (int32 Index = 0; Index < BeatWaves.Num(); ++Index)
	{
		if (BeatWaves[Index] == Wave)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

float SetPieceRules::ShakeStrength(float Strength, float Scale)
{
	return FMath::Clamp(Strength * FMath::Max(Scale, 0.f), 0.f, 1.f);
}

int32 SetPieceRules::DustPuffs(int32 Count, float Scale)
{
	return FMath::Clamp(FMath::RoundToInt32(static_cast<float>(FMath::Max(Count, 0)) * FMath::Max(Scale, 0.f)), 0, MaxDustPuffs);
}

int32 SetPieceRules::RewardLevel(int32 HighestCreatureLevel)
{
	return FMath::Max(HighestCreatureLevel, 1);
}

SetPieceRules::FGateFrame SetPieceRules::GateFrame(const FVector& Start, const FVector& End, const FVector& ArenaPoint)
{
	// The seal's +Y runs along the wall and its +X is the arena's side (ABossSeal::IsInside), a quarter turn clockwise of it.
	// Whichever way round the ends were given, the end that leaves the arena ahead is the origin.
	FVector From = Start;
	FVector To = End;
	auto AheadOf = [](const FVector& A, const FVector& B)
	{
		const FVector2D Along = FVector2D(B.X - A.X, B.Y - A.Y).GetSafeNormal();
		return FVector(Along.Y, -Along.X, 0.0);
	};
	if (FVector::DotProduct(FVector(ArenaPoint.X - From.X, ArenaPoint.Y - From.Y, 0.0), AheadOf(From, To)) < 0.0)
	{
		Swap(From, To);
	}
	FGateFrame Frame;
	Frame.Origin = From;
	Frame.Ahead = AheadOf(From, To);
	Frame.Yaw = FMath::RadiansToDegrees(FMath::Atan2(Frame.Ahead.Y, Frame.Ahead.X));
	Frame.Length = FVector2D::Distance(FVector2D(From.X, From.Y), FVector2D(To.X, To.Y));
	Frame.Rise = To.Z - From.Z;
	return Frame;
}

bool SetPieceRules::IsAheadOfGate(const FGateFrame& Frame, const FVector& Point, double Margin)
{
	return FVector::DotProduct(FVector(Point.X - Frame.Origin.X, Point.Y - Frame.Origin.Y, 0.0), Frame.Ahead) >= Margin;
}

bool SetPieceRules::ShouldRaiseGates(bool bUp, bool bArenaOn, EEncounterState State, int32 WavesStarted, bool bDone,
	bool bPlayerAlive, bool bPlayerInside)
{
	return !bUp && bArenaOn && State == EEncounterState::Engaged && WavesStarted >= 1 && !bDone && bPlayerAlive && bPlayerInside;
}

bool SetPieceRules::ShouldDropGates(bool bUp, bool bArenaOn, EEncounterState State, bool bDone, bool bPlayerAlive)
{
	return bUp && (!bArenaOn || bDone || !bPlayerAlive || State != EEncounterState::Engaged);
}
