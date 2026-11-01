// AWardenCrewMember's table: who each of the Lamplighter's crew is (name, tag, the path their own model will have, the
// stand-in worn until then and its colours) and their station (where their hands are and what keeps them busy). The one
// place to change any of it; the stand-ins are the heroes' bodies (Docs/Pipeline/Heroes.md), each worn at the person's
// height in the player's 0.85 world (TRIPO_MODELS.md heights).

#include "Story/WardenCrewMember.h"

namespace
{
	FWardenStation MakeWheel()
	{
		// Captain Holloway at the wheel: both hands on its rim (its middle about 46 cm ahead, 110 up), the left high and the
		// right low as the wheel is held over, working it a little either way; eyes on the sky ahead. Talking, she keeps her
		// right hand on it and turns only so far: somebody has to fly the ship.
		FWardenStation Station;
		Station.Hands[0] = FVector(46.0, -22.0, 118.0);
		Station.Hands[1] = FVector(46.0, 21.0, 102.0);
		Station.Elbows[0] = FVector(5.0, -48.0, 95.0);
		Station.Elbows[1] = FVector(5.0, 48.0, 88.0);
		Station.Focus = FVector(3000.0, 0.0, 170.0);
		Station.FocusWeight = 0.55f;
		Station.Lean = 4.f;
		Station.Work[0] = FVector(0.0, 0.0, 6.0);
		Station.Work[1] = FVector(0.0, 0.0, -6.0);
		Station.WorkSeconds = 7.f;
		Station.bKeepsRightHand = true;
		Station.MaxTalkTurn = 35.f;
		return Station;
	}

	FWardenStation MakeLantern()
	{
		// Brother Finch at the Great Lantern (1.5 m tall on its mount ahead of him): both hands up to its cage, the right
		// trimming the wick; his eyes on the flame, leaning back a little to see it.
		FWardenStation Station;
		Station.Hands[0] = FVector(36.0, -15.0, 168.0);
		Station.Hands[1] = FVector(38.0, 15.0, 176.0);
		Station.Elbows[0] = FVector(0.0, -42.0, 128.0);
		Station.Elbows[1] = FVector(0.0, 42.0, 132.0);
		Station.Focus = FVector(55.0, 0.0, 178.0);
		Station.FocusWeight = 0.8f;
		Station.Lean = -4.f;
		Station.Work[0] = FVector::ZeroVector;
		Station.Work[1] = FVector(3.0, 0.0, 4.0);
		Station.WorkSeconds = 3.2f;
		return Station;
	}

	FWardenStation MakeEngine()
	{
		// Junie at the engine: leaning well in, knees bent, her left hand braced on the housing and the wrench in her right
		// at the works, turning it in bursts; eyes on what she's fixing.
		FWardenStation Station;
		Station.Hands[0] = FVector(50.0, -26.0, 100.0);
		Station.Hands[1] = FVector(56.0, 12.0, 86.0);
		Station.Elbows[0] = FVector(10.0, -50.0, 100.0);
		Station.Elbows[1] = FVector(12.0, 46.0, 70.0);
		Station.Focus = FVector(62.0, 6.0, 84.0);
		Station.FocusWeight = 0.85f;
		Station.Lean = 22.f;
		Station.Crouch = 9.f;
		Station.Work[0] = FVector::ZeroVector;
		Station.Work[1] = FVector(0.0, 3.0, 3.0);
		Station.WorkSeconds = 0.9f;
		Station.BurstSeconds = 2.5f;
		return Station;
	}
}

FText AWardenCrewMember::NameFor(EWardenCrew InCrew)
{
	switch (InCrew)
	{
	case EWardenCrew::Keeper:
		return NSLOCTEXT("LooterCrew", "Finch", "Brother Finch");
	case EWardenCrew::Engineer:
		return NSLOCTEXT("LooterCrew", "Junie", "Junie Calloway");
	default:
		return NSLOCTEXT("LooterCrew", "Holloway", "Captain Holloway");
	}
}

FName AWardenCrewMember::SpeakerTagFor(EWardenCrew InCrew)
{
	switch (InCrew)
	{
	case EWardenCrew::Keeper:
		return TEXT("Speaker_Finch");
	case EWardenCrew::Engineer:
		return TEXT("Speaker_Junie");
	default:
		return TEXT("Speaker_Holloway");
	}
}

FWardenLook AWardenCrewMember::LookFor(EWardenCrew InCrew)
{
	// Linear multipliers over the stand-in's own colours (the character masters' Tint).
	FWardenLook Look;
	switch (InCrew)
	{
	case EWardenCrew::Keeper:
		// Tripo 3: a thin older man, 1.8 m, a plain hooded robe. Crane's long frame (about 1.95 m) at 0.78; his coat to a
		// plain brown, his dead man's grey-green warmed toward a living face.
		Look.Model = FSoftObjectPath(TEXT("/Game/Art/Characters/SK_Finch.SK_Finch"));
		Look.StandInHero = TEXT("Crane");
		Look.StandInScale = 0.78f;
		Look.Cloth = FLinearColor(0.62f, 0.55f, 0.45f);
		Look.Skin = FLinearColor(1.25f, 1.f, 0.88f);
		Look.Glow = 0.08f;
		break;
	case EWardenCrew::Engineer:
		// Tripo 4: a young woman, 1.65 m, work overalls. Pike's body (about 1.78 m) at 0.79, in overall blue.
		Look.Model = FSoftObjectPath(TEXT("/Game/Art/Characters/SK_Junie.SK_Junie"));
		Look.StandInHero = TEXT("Pike");
		Look.StandInScale = 0.79f;
		Look.Cloth = FLinearColor(0.42f, 0.5f, 0.68f);
		break;
	default:
		// Tripo 2: a woman in her forties, 1.75 m, a long weathered coat. Odessa's body (1.72 m) at 0.86, her crimson to a
		// weathered dark teal-brown.
		Look.Model = FSoftObjectPath(TEXT("/Game/Art/Characters/SK_Holloway.SK_Holloway"));
		Look.StandInHero = TEXT("Odessa");
		Look.StandInScale = 0.86f;
		Look.Cloth = FLinearColor(0.38f, 0.5f, 0.5f);
		Look.Hair = FLinearColor(0.9f, 0.85f, 0.8f);
		break;
	}
	return Look;
}

const FWardenStation& AWardenCrewMember::StationFor(EWardenCrew InCrew)
{
	static const FWardenStation Wheel = MakeWheel();
	static const FWardenStation Lantern = MakeLantern();
	static const FWardenStation Engine = MakeEngine();
	switch (InCrew)
	{
	case EWardenCrew::Keeper:
		return Lantern;
	case EWardenCrew::Engineer:
		return Engine;
	default:
		return Wheel;
	}
}
