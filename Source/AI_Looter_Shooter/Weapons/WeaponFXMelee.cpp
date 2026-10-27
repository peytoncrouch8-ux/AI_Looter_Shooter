#include "Weapons/WeaponFXMelee.h"
#include "Weapons/WeaponFX.h"
#include "HAL/IConsoleManager.h"

// MeleeFX: the heroes' melee tools' own cues and impact effects (the user's round 4 picks, 2026-10-27), on FWeaponFX's
// code-drawn effects.

namespace
{
	TAutoConsoleVariable<float> CVarMeleeFX(TEXT("Looter.Melee.FX"), 1.f,
		TEXT("The melee tools' own impact effects' strength (sparks, glints, steam; 0: only the shared blow's dust and chips)."));
	TAutoConsoleVariable<float> CVarSmearSeconds(TEXT("Looter.Melee.Smear.Seconds"), 0.12f,
		TEXT("Seconds the melee swing's smear behind the tool's head lasts as it fades."));
	TAutoConsoleVariable<float> CVarSmearGlow(TEXT("Looter.Melee.Smear.Glow"), 6.f,
		TEXT("The melee swing smear's brightness (0: none)."));
	TAutoConsoleVariable<float> CVarSmearWidth(TEXT("Looter.Melee.Smear.Width"), 7.f,
		TEXT("The melee swing smear's width at its tip (cm, as the tool is drawn)."));
	TAutoConsoleVariable<float> CVarSparks(TEXT("Looter.Melee.Sparks"), 1.f,
		TEXT("How many sprays of sparks iron throws off stone or metal (1: two hard-target sprays for the spade and pick)."));
	TAutoConsoleVariable<float> CVarSteam(TEXT("Looter.Melee.Steam"), 1.f,
		TEXT("How much steam Gauge's piston fist blows on a hit (a share of a full burst)."));

	/** The tools' colours: the spade's ember crack (Ellis's cyan), card edges' gold, a struck iron spark, steam, Pike's teal. */
	const FLinearColor EmberCyan(0.34f, 0.86f, 1.f);
	const FLinearColor EdgeGold(1.f, 0.86f, 0.55f);
	const FLinearColor SparkHot(1.f, 0.72f, 0.35f);
	const FLinearColor SteamWhite(0.92f, 0.94f, 0.96f);
	const FLinearColor PickTeal(0.25f, 0.8f, 0.72f);

	/**
	 * Iron off a hard surface: a full spray of short, bright sparks (a hard target's bullet sparks, twice over, more for a
	 * bigger Scale) and a hot flare where it struck. (Round 4 had a ricochet's one long streak: faint, DEMO said.)
	 */
	void SparkOff(FWeaponFX& Effects, const FVector& At, const FVector& Along, const FVector& Normal, float Scale)
	{
		const int32 Sprays = FMath::Clamp(FMath::RoundToInt(2.f * Scale * CVarSparks.GetValueOnGameThread()), 1, 6);
		for (int32 Index = 0; Index < Sprays; ++Index)
		{
			Effects.SpawnImpact(At, Normal, Along, EImpactSurface::Target, false);
		}
		Effects.SpawnStarburst(At + Normal * 1.5, 22.f * Scale, SparkHot, 30.f, 0.09f, 8);
	}

	/** Stone chips knocked off, thrown off the surface and back along the blow. */
	void Chips(FWeaponFX& Effects, const FVector& At, const FVector& Normal, const FVector& Along, int32 Count, FRandomStream& Random)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Out = (Normal - Along * 0.4 + Random.GetUnitVector() * 0.6).GetSafeNormal();
			Effects.SpawnGrit(At, Out * Random.FRandRange(180.f, 380.f), Random.FRandRange(0.5f, 0.9f), Random.FRandRange(0.4f, 0.7f));
		}
	}
}

namespace MeleeFX
{
	FName Spade() { static const FName Name(TEXT("Spade")); return Name; }
	FName Fan() { static const FName Name(TEXT("Fan")); return Name; }
	FName Gaff() { static const FName Name(TEXT("Gaff")); return Name; }
	FName Piston() { static const FName Name(TEXT("Piston")); return Name; }
	FName Pick() { static const FName Name(TEXT("Pick")); return Name; }

	FName SwingCue(FName Kind)
	{
		return Kind.IsNone() ? NAME_None : FName(*FString::Printf(TEXT("Player.Melee.%s.Swing"), *Kind.ToString()));
	}

	FName HitCue(FName Kind)
	{
		return Kind.IsNone() ? NAME_None : FName(*FString::Printf(TEXT("Player.Melee.%s.Hit"), *Kind.ToString()));
	}

	bool Sparks(FName Kind, EImpactMaterial Material)
	{
		const bool bIron = Material == EImpactMaterial::Metal || Material == EImpactMaterial::SheetMetal;
		const bool bRock = Material == EImpactMaterial::Stone || Material == EImpactMaterial::Gravel;
		if (Kind == Spade() || Kind == Pick() || Kind == Gaff())
		{
			return bIron || bRock;
		}
		// The fan's steel edges spark on iron alone; the piston's plate meets it with steam, not sparks.
		return Kind == Fan() && bIron;
	}

	void Spawn(FWeaponFX& Effects, FName Kind, const FVector& At, const FVector& Normal, const FVector& Along, EMeleeStruck Struck,
		EImpactMaterial Material, FRandomStream& Random)
	{
		const float Strength = FMath::Clamp(CVarMeleeFX.GetValueOnGameThread(), 0.f, 3.f);
		if (Kind.IsNone() || Strength <= 0.f)
		{
			return;
		}
		const bool bBody = Struck == EMeleeStruck::Body;
		if (!bBody && Sparks(Kind, Material))
		{
			SparkOff(Effects, At, Along, Normal, (Kind == Fan() ? 0.4f : 1.f) * Strength);
		}
		if (Kind == Spade())
		{
			// The ember in the blade's crack flares as it lands on a body; on rock and iron the sparks say it.
			if (bBody)
			{
				Effects.SpawnStarburst(At + Normal * 2.0, 24.f * Strength, EmberCyan, 16.f, 0.12f, 6);
			}
		}
		else if (Kind == Fan())
		{
			// Card-edge glints along the cut: a few small four-point flashes, gold and white, a hair apart in time and place.
			const FVector Across = FVector::CrossProduct(Along, Normal).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const FVector Spot = At + Normal * 2.0 + Across * Random.FRandRange(-9.f, 9.f) + Along * Random.FRandRange(-3.f, 3.f);
				Effects.SpawnStarburst(Spot, Random.FRandRange(9.f, 15.f) * Strength, Index % 2 ? EdgeGold : FLinearColor::White, 18.f,
					Random.FRandRange(0.08f, 0.16f), 4);
			}
		}
		else if (Kind == Gaff())
		{
			// The hook bites and yanks: grit and splinters dragged back toward him; a pale glint of the iron on a body.
			Chips(Effects, At, (Normal - Along).GetSafeNormal(UE_SMALL_NUMBER, Normal), Along, bBody ? 2 : 5, Random);
			if (bBody)
			{
				Effects.SpawnStarburst(At + Normal * 2.0, 16.f * Strength, FLinearColor(0.8f, 0.85f, 0.9f), 10.f, 0.1f, 5);
			}
		}
		else if (Kind == Piston())
		{
			// The knuckle plate's kick: a ring of it, and steam blown off the plate.
			Effects.SpawnShockRing(At + Normal * 3.0, Along, 6.f, 38.f * Strength, 2.5f, SteamWhite, 10.f, 0.14f, true);
			SpawnSteam(Effects, At + Normal * 4.0, (Normal + Random.GetUnitVector() * 0.3).GetSafeNormal(), 0.7f, Random);
		}
		else if (Kind == Pick())
		{
			// Stone chips off rock (the pick's whole job); a teal glint off its band on a body.
			if (!bBody && (Material == EImpactMaterial::Stone || Material == EImpactMaterial::Gravel))
			{
				Chips(Effects, At, Normal, Along, 6, Random);
			}
			if (bBody)
			{
				Effects.SpawnStarburst(At + Normal * 2.0, 14.f * Strength, PickTeal, 14.f, 0.1f, 4);
			}
		}
	}

	FLinearColor SmearColor(FName Kind)
	{
		if (Kind == Spade())
		{
			return FLinearColor(0.55f, 0.9f, 1.f);
		}
		if (Kind == Fan())
		{
			return FLinearColor(1.f, 0.85f, 0.5f);
		}
		if (Kind == Gaff())
		{
			return FLinearColor(0.75f, 0.8f, 0.85f);
		}
		if (Kind == Piston())
		{
			return FLinearColor(0.95f, 0.92f, 0.85f);
		}
		if (Kind == Pick())
		{
			return FLinearColor(0.45f, 0.9f, 0.82f);
		}
		return FLinearColor(0.9f, 0.9f, 0.9f);
	}

	float SmearSeconds()
	{
		return FMath::Clamp(CVarSmearSeconds.GetValueOnGameThread(), 0.02f, 0.4f);
	}

	float SmearShare(float Age)
	{
		// Bright as it's laid down, then gone quickly (squared: most of it fades in the first half of its life).
		const float Left = 1.f - FMath::Clamp(Age / SmearSeconds(), 0.f, 1.f);
		return Left * Left;
	}

	void DrawSmear(FWeaponFX& Effects, const TArray<FSmearSample>& Samples, double Now, const FLinearColor& Color)
	{
		const float Glow = FMath::Max(CVarSmearGlow.GetValueOnGameThread(), 0.f);
		const float Width = FMath::Max(CVarSmearWidth.GetValueOnGameThread(), 0.f);
		if (Glow <= 0.f || Width <= 0.f)
		{
			return;
		}
		for (int32 Index = 1; Index < Samples.Num(); ++Index)
		{
			const FSmearSample& From = Samples[Index - 1];
			const FSmearSample& To = Samples[Index];
			const float Share = SmearShare(static_cast<float>(Now - From.Time));
			if (Share <= 0.f || From.Tip.Equals(To.Tip, 0.05))
			{
				continue;
			}
			const float Scale = FMath::Max(To.Scale, 0.01f);
			// The tip's arc the brightest and widest; the inner edge fainter; a fill between them so it reads as a band.
			Effects.AddStreak(From.Tip, To.Tip, Width * Scale * (0.4f + 0.6f * Share), Color, Glow * Share);
			Effects.AddStreak(From.Inner, To.Inner, Width * 0.6f * Scale * (0.4f + 0.6f * Share), Color, Glow * 0.4f * Share);
			const FVector FillFrom = FMath::Lerp(From.Inner, From.Tip, 0.5f);
			const FVector FillTo = FMath::Lerp(To.Inner, To.Tip, 0.5f);
			Effects.AddStreak(FillFrom, FillTo, Width * 1.6f * Scale * Share, Color, Glow * 0.25f * Share);
		}
	}

	void SpawnSteam(FWeaponFX& Effects, const FVector& Vent, const FVector& Out, float Strength, FRandomStream& Random)
	{
		const float Amount = FMath::Clamp(Strength * CVarSteam.GetValueOnGameThread(), 0.f, 3.f);
		const int32 Puffs = FMath::RoundToInt(6.f * Amount);
		const FVector Way = Out.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		for (int32 Index = 0; Index < Puffs; ++Index)
		{
			// Blown out hard and rising as it slows, spreading and thinning (the muzzle's wisps, faster and whiter).
			const FVector Velocity = (Way + Random.GetUnitVector() * 0.35).GetSafeNormal() * Random.FRandRange(120.f, 260.f) + FVector(0.0, 0.0, 30.0);
			Effects.SpawnWisp(Vent, Velocity, Random.FRandRange(0.35f, 0.55f), Random.FRandRange(4.f, 7.f), Random.FRandRange(22.f, 36.f),
				Random.FRandRange(0.5f, 0.9f));
		}
	}
}
