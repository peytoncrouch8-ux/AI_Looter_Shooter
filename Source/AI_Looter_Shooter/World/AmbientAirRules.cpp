#include "World/AmbientAirRules.h"
#include <initializer_list>

namespace
{
	/** Each kind's look, by EAirKind. Sizes are a speck's: a 1 cm mote is a pixel or two across at a few meters. */
	const FAirKindLook Looks[] = {
		// Mote: tiny, long-lived, slow; at eye height, kept near (they're too small to see past 7 m).
		{ 0.6f, 1.1f, 8.f, 18.f, 700.f, 500.f, -140.f, 220.f, 5.f, 6.f, 250.f },
		// Pollen: pale specks a little bigger, a little quicker, low over the grass.
		{ 0.8f, 1.4f, 6.f, 14.f, 900.f, 900.f, 30.f, 220.f, 10.f, 25.f, 350.f },
		// Fluff: soft tufts riding the breeze higher up, seen further off.
		{ 2.f, 3.5f, 10.f, 20.f, 1400.f, 1400.f, 60.f, 450.f, 6.f, 35.f, 500.f },
		// Leaf: a leaf's length; it lives as long as its fall (and a moment on the ground); from trees within reach.
		{ 3.5f, 6.f, 20.f, 30.f, 2500.f, 2500.f, 0.f, 0.f, 0.f, 30.f, 450.f },
		// Moth: a wingspan; round lamps within reach; the wind hardly moves them off their light.
		{ 1.6f, 2.6f, 8.f, 16.f, 2500.f, 2500.f, -25.f, 25.f, 0.f, 0.f, 60.f },
	};
	static_assert(UE_ARRAY_COUNT(Looks) == static_cast<int32>(EAirKind::Count), "A look for every kind of air");

	/** Each kind's share of the budget: they add up to AmbientAirRules::Budget. */
	constexpr int32 Caps[] = { 90, 60, 36, 40, 24 };
	static_assert(UE_ARRAY_COUNT(Caps) == static_cast<int32>(EAirKind::Count), "A cap for every kind of air");
	static_assert(Caps[0] + Caps[1] + Caps[2] + Caps[3] + Caps[4] == AmbientAirRules::Budget, "The caps make the budget");

	/** Motes out in the open, against under a roof by a doorway's shafts (shares of their cap at full sun). */
	constexpr float MotesInTheOpen = 0.6f;
	/** Fluff at dusk, against by day. */
	constexpr float FluffAtDusk = 0.35f;
	/** Of an eye this near (cm), it's gone; past twice it, whole. */
	constexpr float EyeNear = 20.f;
}

int32 AmbientAirRules::Cap(EAirKind Kind)
{
	const int32 Index = static_cast<int32>(Kind);
	return Index >= 0 && Index < static_cast<int32>(UE_ARRAY_COUNT(Caps)) ? Caps[Index] : 0;
}

const FAirKindLook& AmbientAirRules::Look(EAirKind Kind)
{
	const int32 Index = FMath::Clamp(static_cast<int32>(Kind), 0, static_cast<int32>(EAirKind::Count) - 1);
	return Looks[Index];
}

float AmbientAirRules::Reach(EAirKind Kind, bool bUnderCover)
{
	const FAirKindLook& Its = Look(Kind);
	return bUnderCover ? Its.ReachUnderCover : Its.Reach;
}

bool AmbientAirRules::IsDusk(FName Lighting)
{
	static const FName Dusk(TEXT("Dusk"));
	return Lighting == Dusk;
}

bool AmbientAirRules::IsMeadow(EImpactMaterial Surface)
{
	return Surface == EImpactMaterial::Grass || Surface == EImpactMaterial::Hay;
}

bool AmbientAirRules::IsBroadleaf(const FString& MeshName)
{
	// Far cards (SM_FarBroadleaf) stand past the boundary; dead trees have no leaves; pines and junipers keep theirs.
	for (const TCHAR* Not : { TEXT("Far"), TEXT("Dead"), TEXT("Pine"), TEXT("Juniper"), TEXT("Stump"), TEXT("Log") })
	{
		if (MeshName.Contains(Not, ESearchCase::CaseSensitive))
		{
			return false;
		}
	}
	for (const TCHAR* Leafy : { TEXT("Oak"), TEXT("Birch"), TEXT("Apple"), TEXT("Cottonwood"), TEXT("Willow"), TEXT("Aspen"),
		TEXT("Poplar"), TEXT("Elm"), TEXT("Maple"), TEXT("Broadleaf") })
	{
		if (MeshName.Contains(Leafy, ESearchCase::CaseSensitive))
		{
			return true;
		}
	}
	return false;
}

bool AmbientAirRules::IsPlacedByProbe(EAirKind Kind)
{
	return Kind == EAirKind::Mote || Kind == EAirKind::Pollen || Kind == EAirKind::Fluff;
}

bool AmbientAirRules::Belongs(EAirKind Kind, const FAirPlace& Place, const FAirView& View)
{
	if (View.bSheltered)
	{
		return false;
	}
	const bool bDusk = IsDusk(View.Lighting);
	switch (Kind)
	{
	case EAirKind::Mote:
		// Only the sun shows them: in its light, a roof over them or not (a doorway's shaft is the best place for them).
		return !bDusk && Place.bInSun;
	case EAirKind::Pollen:
		return !bDusk && Place.bGround && Place.bOpenSky && IsMeadow(Place.Surface);
	case EAirKind::Fluff:
		return Place.bGround && Place.bOpenSky && IsMeadow(Place.Surface);
	default:
		// Leaves and moths come from their trees and lamps, never from a point.
		return false;
	}
}

int32 AmbientAirRules::Target(EAirKind Kind, const FAirView& View, const FAirSurroundings& Around, float Density)
{
	if (!View.bValid || View.bSheltered)
	{
		return 0;
	}
	const bool bDusk = IsDusk(View.Lighting);
	const float CapNow = static_cast<float>(Cap(Kind)) * FMath::Clamp(Density, 0.f, 1.f);
	float Wanted = 0.f;
	switch (Kind)
	{
	case EAirKind::Mote:
		Wanted = bDusk ? 0.f : CapNow * FMath::Clamp(Around.SunShare, 0.f, 1.f) * (View.bUnderCover ? 1.f : MotesInTheOpen);
		break;
	case EAirKind::Pollen:
		Wanted = bDusk ? 0.f : CapNow * FMath::Clamp(Around.GrassShare, 0.f, 1.f);
		break;
	case EAirKind::Fluff:
		Wanted = CapNow * FMath::Clamp(Around.GrassShare, 0.f, 1.f) * (bDusk ? FluffAtDusk : 1.f);
		break;
	case EAirKind::Leaf:
		Wanted = CapNow * FMath::Clamp(static_cast<float>(Around.NearTrees) / TreesForFullLeaves, 0.f, 1.f);
		break;
	case EAirKind::Moth:
		Wanted = bDusk ? FMath::Min(CapNow, static_cast<float>(Around.NearLamps * MothsPerLamp)) : 0.f;
		break;
	default:
		break;
	}
	return FMath::Clamp(FMath::RoundToInt(Wanted), 0, Cap(Kind));
}

float AmbientAirRules::DistanceFade(float Level, float InReach, float ToEye)
{
	const float Far = 1.f - FMath::SmoothStep(InReach * 0.7f, InReach, Level);
	const float Near = FMath::SmoothStep(EyeNear, EyeNear * 2.f, ToEye);
	return Far * Near;
}

float AmbientAirRules::AgeFade(float Age, float Life)
{
	const float In = FMath::Clamp(Age / 1.f, 0.f, 1.f);
	const float Out = FMath::Clamp((Life - Age) / 1.5f, 0.f, 1.f);
	return FMath::SmoothStep(0.f, 1.f, FMath::Min(In, Out));
}

FLinearColor AmbientAirRules::Albedo(EAirKind Kind)
{
	switch (Kind)
	{
	case EAirKind::Mote: return FLinearColor(1.f, 0.88f, 0.7f);
	case EAirKind::Pollen: return FLinearColor(0.92f, 0.8f, 0.42f);
	case EAirKind::Fluff: return FLinearColor(0.9f, 0.88f, 0.82f);
	// Late summer's olive going gold (the oaks' and the orchard's own swaps).
	case EAirKind::Leaf: return FLinearColor(0.3f, 0.24f, 0.07f);
	case EAirKind::Moth: return FLinearColor(1.f, 0.8f, 0.55f);
	default: return FLinearColor::White;
	}
}

const TCHAR* AmbientAirRules::Name(EAirKind Kind)
{
	switch (Kind)
	{
	case EAirKind::Mote: return TEXT("motes");
	case EAirKind::Pollen: return TEXT("pollen");
	case EAirKind::Fluff: return TEXT("fluff");
	case EAirKind::Leaf: return TEXT("leaves");
	case EAirKind::Moth: return TEXT("moths");
	default: return TEXT("?");
	}
}
