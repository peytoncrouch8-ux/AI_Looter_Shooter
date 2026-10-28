#include "Weapons/WeaponModelComponent.h"
#include "Affixes/WeaponMakers.h"
#include "Weapons/WeaponParts.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HAL/IConsoleManager.h"
#include "Math/RandomStream.h"

// The gun's field paint (the user's call, 2026-10-27; Docs/Work/2026-10-27-GunPatterns/CONTRACT.md): simple patterns that
// break up the monotony of a gun's parts, rarer guns fancier, subtle and worn-in like real field paint. C++ only rolls the
// pattern (no save change: from the seed, rarity and maker, like WeaponParts::Wear) and writes it into every part's custom
// primitive data; the gun master draws it in the part's own space (Tools/Unreal/build_world_materials.py, M_Gun PATTERN_*),
// so every gun family, Tripo or scripted, takes it with no texture or material of its own.

namespace
{
	TAutoConsoleVariable<bool> CVarGunPattern(TEXT("Looter.Weapons.Pattern"), true,
		TEXT("Guns wear a field-paint pattern rolled from their seed, rarity and maker; 0 shows them plain. Read as a gun is built."));
	TAutoConsoleVariable<int32> CVarGunPatternForce(TEXT("Looter.Weapons.Pattern.Force"), -1,
		TEXT("For checking: every gun built shows this pattern kind (0 none, 1 grime, 2 worn edges, 3 stripes, 4 chevrons, 5 camo, ")
		TEXT("6 stencil, 7 clean bold); -1 rolls it."));

	/** The pattern's own stream of the gun's seed: no other roll (parts, paint, wear, maker) moves when it's added to. */
	constexpr uint32 PatternSalt = 0x9A77E5u;

	/** Each part's seed moved on by its index (the golden ratio spreads them evenly over 0-1). */
	constexpr float PartSeedStep = 0.6180339887f;

	/** The kinds a rarity rolls among, each with its weight (Common to Legendary; the legendary maker design is added in code). */
	struct FPatternChoice
	{
		float Kind;
		float Weight;
	};
	const FPatternChoice CommonKinds[] = { { 1.f, 1.f }, { 2.f, 1.f } };
	const FPatternChoice UncommonKinds[] = { { 1.f, 1.f }, { 2.f, 1.f }, { 3.f, 1.f } };
	const FPatternChoice RareKinds[] = { { 3.f, 1.f }, { 4.f, 1.f }, { 5.f, 1.f }, { 6.f, 1.f } };
	const FPatternChoice EpicKinds[] = { { 4.f, 1.f }, { 5.f, 1.f }, { 6.f, 1.f }, { 7.f, 1.f } };

	/** How strongly each rarity's pattern shows (min, max): subtle on commons, bold on legendaries. */
	const FVector2f AmountByRarity[] = { { 0.3f, 0.5f }, { 0.3f, 0.55f }, { 0.35f, 0.6f }, { 0.4f, 0.6f }, { 0.6f, 0.8f } };

	/** Each kind's size (cm per repeat, min and max), 0 none to 7 clean bold; inside the contract's 4-30. */
	const FVector2f ScaleByKind[] = { { 0.f, 0.f }, { 14.f, 24.f }, { 6.f, 12.f }, { 10.f, 22.f }, { 6.f, 12.f }, { 12.f, 28.f },
		{ 5.f, 10.f }, { 14.f, 30.f } };

	/** A forced kind shows at least this much, so a check can see it. */
	constexpr float ForcedLeastAmount = 0.5f;

	/**
	 * Which palette tint each kind leans to (weights over the palette: darker, lighter, accent, black, bone). Grime is dark,
	 * worn edges show the lighter undercoat, the painted designs take the contrasting colours.
	 */
	const float TintWeightsByKind[][5] = {
		{ 0.f, 0.f, 0.f, 1.f, 0.f },   // none (unused)
		{ 2.f, 0.f, 0.f, 3.f, 0.f },   // grime streaks
		{ 1.f, 2.f, 0.f, 0.f, 1.f },   // worn edges and chips
		{ 1.f, 1.f, 3.f, 2.f, 2.f },   // stripes
		{ 0.f, 0.f, 2.f, 2.f, 3.f },   // chevrons
		{ 3.f, 2.f, 1.f, 1.f, 1.f },   // camo blotches
		{ 0.f, 1.f, 1.f, 2.f, 3.f },   // stencil
		{ 2.f, 1.f, 2.f, 2.f, 1.f },   // clean bold
	};

	/** The palette's fixed colours (linear): black that still reads as paint, and bone. */
	const FLinearColor PatternBlack(0.018f, 0.018f, 0.02f);
	const FLinearColor PatternBone(0.768f, 0.687f, 0.521f);   // sRGB e3d8bf
	/** Without a shell paint or a trim: a neutral grey shell and brass. */
	const FLinearColor FallbackShell(0.2f, 0.2f, 0.2f);
	const FLinearColor FallbackTrim(0.693f, 0.371f, 0.053f);  // sRGB d9a441, the kit's brass

	/** The gun kit's painted shell and trim slots (Tools/Blender/looter_guns.py; the maker repaints them: WeaponMakers.cpp). */
	const FName PatternShellSlot(TEXT("GunPolymerSand"));
	const FName PatternTrimSlot(TEXT("GunBrass"));

	/** The optics' reticle: an emissive slot without "Glow" in its name. */
	const FName PatternReticleSlot(TEXT("GunReticle"));

	/** A tint this close in brightness to the shell would vanish on it. */
	constexpr float LeastTintContrast = 0.02f;

	/** Picks among Choices by weight at Roll (0-1); PatternNone when none weighs anything. */
	float PickPatternKind(TConstArrayView<FPatternChoice> Choices, float Roll)
	{
		float Total = 0.f;
		for (const FPatternChoice& Choice : Choices)
		{
			Total += FMath::Max(Choice.Weight, 0.f);
		}
		if (Total <= 0.f)
		{
			return UWeaponModelComponent::PatternNone;
		}
		float Remaining = Roll * Total;
		float Last = UWeaponModelComponent::PatternNone;
		for (const FPatternChoice& Choice : Choices)
		{
			if (Choice.Weight <= 0.f)
			{
				continue;
			}
			Last = Choice.Kind;
			Remaining -= Choice.Weight;
			if (Remaining < 0.f)
			{
				return Choice.Kind;
			}
		}
		return Last;
	}

	/** The palette entry Roll (0-1) lands on by Weights (one per palette colour); 0 when none weighs anything. */
	int32 PickTintIndex(const float (&Weights)[5], float Roll)
	{
		float Total = 0.f;
		for (const float Weight : Weights)
		{
			Total += Weight;
		}
		if (Total <= 0.f)
		{
			return 0;
		}
		float Remaining = Roll * Total;
		int32 Last = 0;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			if (Weights[Index] <= 0.f)
			{
				continue;
			}
			Last = Index;
			Remaining -= Weights[Index];
			if (Remaining < 0.f)
			{
				return Index;
			}
		}
		return Last;
	}

	FLinearColor ShellOf(const FWeaponLook& Look)
	{
		if (const FLinearColor* Shell = Look.Colors.Find(PatternShellSlot))
		{
			return *Shell;
		}
		return FallbackShell;
	}
}

float UWeaponModelComponent::MakerPatternKind(FName MakerKey)
{
	// Each maker's legendary signature, in its fiction: the railroad's hazard chevrons, the mission's gilt bands, the
	// salvagers' patchwork, the instrument makers' survey stencils. Keys as WeaponMakers.cpp (saved with guns, never renamed).
	if (MakerKey == FName(TEXT("Roundhouse")))
	{
		return PatternChevrons;
	}
	if (MakerKey == FName(TEXT("SaintMaud")))
	{
		return PatternStripes;
	}
	if (MakerKey == FName(TEXT("Windfall")))
	{
		return PatternCamo;
	}
	if (MakerKey == FName(TEXT("Meridian")))
	{
		return PatternStencil;
	}
	return PatternBold;
}

TArray<FLinearColor> UWeaponModelComponent::PatternPalette(const FWeaponLook& Look)
{
	// The look already wears its maker's paints (WeaponMakers::PaintLook), so its shell and trim are the maker's own.
	const FLinearColor Shell = ShellOf(Look);
	const FLinearColor* Trim = Look.Colors.Find(PatternTrimSlot);
	TArray<FLinearColor> Palette;
	Palette.Add(Shell * 0.45f);
	Palette.Add(Shell + (FLinearColor::White - Shell) * 0.4f);
	Palette.Add(Trim ? *Trim : FallbackTrim);
	Palette.Add(PatternBlack);
	Palette.Add(PatternBone);
	for (FLinearColor& Color : Palette)
	{
		Color.A = 1.f;
	}
	return Palette;
}

FWeaponPattern UWeaponModelComponent::PickPattern(const FWeaponInstanceData& Instance, const FWeaponLook& Look, int32 ForceKind)
{
	// A named gun may later carry its own pattern (UNamedWeaponDefinition): return it here, before the roll. None does yet,
	// so a named gun rolls like any other of its rarity (without a maker: the legendary's clean bold).

	// Every draw is taken whatever it's used for, so the kind never moves the scale's, the seed's, the tint's or the amount's.
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Instance.Seed), PatternSalt)));
	const float KindRoll = Random.FRand();
	const float ScaleRoll = Random.FRand();
	const float SeedRoll = Random.FRand();
	const float TintRoll = Random.FRand();
	const float AmountRoll = Random.FRand();

	const int32 Rarity = FMath::Clamp(static_cast<int32>(Instance.Rarity), 0, static_cast<int32>(UE_ARRAY_COUNT(AmountByRarity)) - 1);
	float Kind = PatternNone;
	switch (Instance.Rarity)
	{
	case EWeaponRarity::Common:
		Kind = PickPatternKind(MakeArrayView(CommonKinds), KindRoll);
		break;
	case EWeaponRarity::Uncommon:
		Kind = PickPatternKind(MakeArrayView(UncommonKinds), KindRoll);
		break;
	case EWeaponRarity::Rare:
		Kind = PickPatternKind(MakeArrayView(RareKinds), KindRoll);
		break;
	case EWeaponRarity::Epic:
		Kind = PickPatternKind(MakeArrayView(EpicKinds), KindRoll);
		break;
	case EWeaponRarity::Legendary:
	default:
	{
		// Clean bold or the maker's own design, evenly; a gun without a maker (a named one) always clean bold.
		const FWeaponMaker* Maker = WeaponMakers::Of(Instance);
		const FPatternChoice LegendaryKinds[] = { { PatternBold, 1.f }, { MakerPatternKind(Maker ? Maker->Key : NAME_None), 1.f } };
		Kind = PickPatternKind(MakeArrayView(LegendaryKinds), KindRoll);
		break;
	}
	}
	float Amount = FMath::Lerp(AmountByRarity[Rarity].X, AmountByRarity[Rarity].Y, AmountRoll);
	if (ForceKind >= 0)
	{
		Kind = static_cast<float>(FMath::Clamp(ForceKind, 0, PatternKindCount - 1));
		Amount = FMath::Max(Amount, ForcedLeastAmount);
	}

	FWeaponPattern Pattern;
	const int32 KindIndex = FMath::Clamp(FMath::RoundToInt(Kind), 0, PatternKindCount - 1);
	if (KindIndex == 0)
	{
		return Pattern;
	}
	Pattern.Kind = static_cast<float>(KindIndex);
	Pattern.Scale = FMath::Clamp(FMath::Lerp(ScaleByKind[KindIndex].X, ScaleByKind[KindIndex].Y, ScaleRoll), 4.f, 30.f);
	Pattern.Seed = SeedRoll;
	Pattern.Amount = FMath::Clamp(Amount, 0.f, 1.f);

	// The tint from the maker's palette by the kind's leaning; one that would vanish on the shell turns to black on a light
	// shell, bone on a dark one.
	const TArray<FLinearColor> Palette = PatternPalette(Look);
	const FLinearColor Shell = ShellOf(Look);
	FLinearColor Tint = Palette[PickTintIndex(TintWeightsByKind[KindIndex], TintRoll)];
	if (FMath::Abs(Tint.GetLuminance() - Shell.GetLuminance()) < LeastTintContrast)
	{
		Tint = Shell.GetLuminance() > 0.15f ? PatternBlack : PatternBone;
	}
	Pattern.Tint = Tint;
	return Pattern;
}

FWeaponPattern UWeaponModelComponent::PatternFor(const FWeaponInstanceData& Instance, const FWeaponLook& Look)
{
	if (!CVarGunPattern.GetValueOnGameThread())
	{
		return FWeaponPattern();
	}
	return PickPattern(Instance, Look, CVarGunPatternForce.GetValueOnGameThread());
}

void UWeaponModelComponent::ShowPattern(UPrimitiveComponent& Part, const FWeaponPattern& Pattern, int32 PartIndex)
{
	const bool bShown = Pattern.Kind > 0.f && Pattern.Amount > 0.f;
	const float Seed = bShown ? FMath::Frac(Pattern.Seed + static_cast<float>(FMath::Max(PartIndex, 0)) * PartSeedStep) : 0.f;
	Part.SetCustomPrimitiveDataFloat(PatternKindDataIndex, bShown ? Pattern.Kind : 0.f);
	Part.SetCustomPrimitiveDataFloat(PatternScaleDataIndex, bShown ? Pattern.Scale : 0.f);
	Part.SetCustomPrimitiveDataFloat(PatternSeedDataIndex, Seed);
	Part.SetCustomPrimitiveDataVector3(PatternTintDataIndex, bShown ? FVector(Pattern.Tint.R, Pattern.Tint.G, Pattern.Tint.B) : FVector::ZeroVector);
	Part.SetCustomPrimitiveDataFloat(PatternAmountDataIndex, bShown ? Pattern.Amount : 0.f);
}

bool UWeaponModelComponent::IsGlowSlot(FName Slot, FName RarityGlowSlot)
{
	if (Slot.IsNone())
	{
		return false;
	}
	if (Slot == RarityGlowSlot || Slot == PatternReticleSlot)
	{
		return true;
	}
	const FString Name = Slot.ToString();
	// A part's own copy of the rarity accent ("<slot>_<part>", as the paints' copies), and every Gun<Thing>Glow.
	if (!RarityGlowSlot.IsNone() && Name.StartsWith(RarityGlowSlot.ToString() + TEXT("_")))
	{
		return true;
	}
	return Name.StartsWith(TEXT("Gun")) && Name.Contains(TEXT("Glow"));
}

bool UWeaponModelComponent::IsWhollyGlow(const UStaticMeshComponent& Part, FName RarityGlowSlot)
{
	const TArray<FName> Slots = Part.GetMaterialSlotNames();
	if (Slots.IsEmpty())
	{
		return false;
	}
	for (const FName Slot : Slots)
	{
		if (!IsGlowSlot(Slot, RarityGlowSlot))
		{
			return false;
		}
	}
	return true;
}
