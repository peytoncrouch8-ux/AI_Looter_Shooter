// The breakable props' kinds (their models in /Game/Art/Props: Containers.py's intact ones, Lootables.py's pieces and
// stumps) and the rules of a break: the pieces' throw and the loot's rolls.

#include "World/BreakableKinds.h"
#include "Audio/LooterSoundCues.h"
#include "Math/RandomStream.h"

FBreakableKindInfo FBreakableKindInfo::Get(EBreakableKind Kind)
{
	FBreakableKindInfo Info;
	switch (Kind)
	{
	case EBreakableKind::PackingCrate:
		// Containers.py crate_b(): 1.0 x 0.62 m, 0.55 m to its rim, the lid askew on top. Straw inside: a little more often
		// something packed in it, and chaff in its dust.
		Info.BodyPath = TEXT("/Game/Art/Props/SM_Crate_B.SM_Crate_B");
		Info.StumpPath = TEXT("/Game/Art/Props/SM_Break_CrateB_Stump.SM_Break_CrateB_Stump");
		Info.PiecePrefix = TEXT("/Game/Art/Props/SM_Break_CrateB_");
		Info.PieceCount = 7;
		Info.Health = 50.f;
		Info.Height = 60.f;
		Info.AmmoChance = 0.4f;
		Info.MoteChance = 0.06f;
		Info.BreakCue = LooterSoundCue::Lootables::BreakCrate;
		Info.DustColor = FLinearColor(0.34f, 0.29f, 0.17f);
		break;
	case EBreakableKind::Barrel:
		// Containers.py barrel_a(): 0.92 m, 0.66 m across its belly; four iron hoops ring as the staves fly.
		Info.BodyPath = TEXT("/Game/Art/Props/SM_Barrel_A.SM_Barrel_A");
		Info.StumpPath = TEXT("/Game/Art/Props/SM_Break_BarrelA_Stump.SM_Break_BarrelA_Stump");
		Info.PiecePrefix = TEXT("/Game/Art/Props/SM_Break_BarrelA_");
		Info.PieceCount = 7;
		Info.Health = 52.f;
		Info.Height = 92.f;
		Info.AmmoChance = 0.35f;
		Info.MoteChance = 0.06f;
		Info.BreakCue = LooterSoundCue::Lootables::BreakBarrel;
		Info.DustColor = FLinearColor(0.28f, 0.23f, 0.17f);
		break;
	case EBreakableKind::SlattedCrate:
	default:
		// Containers.py crate_a(): 0.9 x 0.65 m, 0.6 m tall, slats with gaps on four corner posts.
		Info.BodyPath = TEXT("/Game/Art/Props/SM_Crate_A.SM_Crate_A");
		Info.StumpPath = TEXT("/Game/Art/Props/SM_Break_CrateA_Stump.SM_Break_CrateA_Stump");
		Info.PiecePrefix = TEXT("/Game/Art/Props/SM_Break_CrateA_");
		Info.PieceCount = 7;
		Info.Health = 50.f;
		Info.Height = 62.f;
		Info.AmmoChance = 0.35f;
		Info.MoteChance = 0.06f;
		Info.BreakCue = LooterSoundCue::Lootables::BreakCrate;
		Info.DustColor = FLinearColor(0.3f, 0.25f, 0.18f);
		break;
	}
	return Info;
}

FString FBreakableKindInfo::PiecePath(int32 Index) const
{
	// "/Game/Art/Props/SM_Break_CrateA_" + "3" + ".SM_Break_CrateA_3"
	const FString Package = FString::Printf(TEXT("%s%d"), PiecePrefix ? PiecePrefix : TEXT(""), Index + 1);
	FString Name;
	Package.Split(TEXT("/"), nullptr, &Name, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	return Package + TEXT(".") + Name;
}

bool LooterBreakables::RollMote(const FBreakableKindInfo& Info, FRandomStream& Random)
{
	return Random.FRand() < Info.MoteChance;
}

FVector LooterBreakables::PieceVelocity(const FVector& Out, const FVector& Blow, FRandomStream& Random)
{
	// Out through the piece's own side, a little off it so no two go the same way; the blow carries them all a little.
	FVector Away = Out.GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f).Vector();
	}
	Away = Away.RotateAngleAxis(Random.FRandRange(-25.f, 25.f), FVector::UpVector);
	const FVector Along = Blow.GetSafeNormal2D() * BlowSpeed * Random.FRandRange(0.6f, 1.2f);
	return Away * Random.FRandRange(OutSpeedMin, OutSpeedMax) + Along + FVector::UpVector * Random.FRandRange(UpSpeedMin, UpSpeedMax);
}

FVector LooterBreakables::PieceSpin(FRandomStream& Random)
{
	return Random.GetUnitVector() * Random.FRandRange(SpinMin, SpinMax);
}
