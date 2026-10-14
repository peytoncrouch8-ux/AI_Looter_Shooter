#pragma once

#include "CoreMinimal.h"

/**
 * Rings on the water (the water concept's point 5): a shot into a creek or pond, a foot coming down in it, and a walk
 * through it leave marks on the surface. A ring is a few soft dashes laid flat on the water round a centre, spreading from a
 * few centimetres to 0.6-1.2 m and fading out over 0.8-1.2 s (a shot leaves two, the second smaller and a beat later; a
 * wading step one). A wake is a small V: two short arms trailing back from where a foot passed, widening and fading in about
 * a second; a walk leaves one every 75 cm.
 *
 * FWeaponFX owns one and draws it with its smoke instances (Draw adds the dashes to that frame's smoke transforms): no
 * assets of its own, no per-frame allocation, one extra instanced draw call shared with the dust. The rules (growth, fade,
 * counts, spacing) are static functions so the tests read them (Looter.FX.WaterRings.*).
 *
 * Capped: at most MaxRings rings and MaxWakes wakes live (the oldest goes first), and a ring costs RingCost from a budget
 * that refills (BudgetCap at most, BudgetRate a second), so a long burst into the water thins its rings instead of piling
 * them up. A ring's dash count is fixed at birth, so the instance count only changes when a ring is born or gone.
 */
class AI_LOOTER_SHOOTER_API FWaterRings
{
public:
	// ---- Rules ----

	/** The ring's largest radius (cm) for a hit of this Size (FWeaponFX::SpawnSurfaceImpact's: a rifle shot 1, a wading step about 0.5): 60 to 120 cm. */
	static float MaxRadiusFor(float Size);
	/** How long a ring of this largest radius lives (seconds): 0.8 at the smallest, 1.2 at the largest. */
	static float LifeFor(float MaxRadius);
	/** Rings a hit of this Size leaves: a shot (Size 0.8 and over) two, concentric; a pellet or a wading step one. */
	static int32 RingsFor(float Size);
	/** The radius T (0 born, 1 gone) of the way through the ring's life: quick at first, slowing as it spreads. */
	static float RadiusAt(float T, float StartRadius, float MaxRadius);
	/** The ring's strength at T: in over the first 8%, then thinning out to nothing. */
	static float OpacityAt(float T);
	/** Dashes in a ring that grows to this radius (10 to 18). */
	static int32 SegmentsFor(float MaxRadius);
	/** Water that lies flat enough to hold rings (a waterfall's face doesn't). */
	static bool LiesFlat(const FVector& Up) { return Up.Z >= FlatUp; }
	/** Whether a walk that has covered Travelled cm since the last wake (at Speed cm/s, a body of Scale) is due another. */
	static bool WakeDue(float Travelled, float Speed, float Scale);
	/** A wake's strength for the speed it was made at (0.45 creeping, 1 sprinting). */
	static float WakeOpacityFor(float Speed);
	/** The foam's colour before the light (linear; the unlit smoke material shows what it's given, so the caller multiplies the light in). */
	static FLinearColor FoamAlbedo() { return FLinearColor(0.4f, 0.44f, 0.44f); }

	static constexpr int32 MaxRings = 24;
	static constexpr int32 MaxWakes = 12;
	static constexpr float BudgetCap = 12.f;
	static constexpr float BudgetRate = 16.f;
	static constexpr float RingCost = 1.f;
	/** The surface normal's Z at the least for rings. */
	static constexpr float FlatUp = 0.7f;
	/** The ring radii (cm) for the smallest and largest hit, and the lives that go with them. */
	static constexpr float RadiusSmall = 60.f;
	static constexpr float RadiusLarge = 120.f;
	static constexpr float LifeShort = 0.8f;
	static constexpr float LifeLong = 1.2f;
	/** Walking slower than this leaves no wake and no step rings (cm/s; the creep is 150, the walk about 460). */
	static constexpr float WakeMinSpeed = 120.f;
	/** Ground covered between one wake and the next (cm, at the player's size). */
	static constexpr float WakeSpacing = 75.f;
	static constexpr float WakeLife = 0.9f;
	/** Rings and dashes further than this from the camera aren't drawn (cm). */
	static constexpr float DrawRange = 5000.f;

	// ---- Pool ----

	/**
	 * Count concentric rings centred on Center, lying on the surface whose normal is Up, for a hit of Size; Color is the foam
	 * in the light there. Returns the rings made (the budget can pay for fewer; none on a surface that isn't flat).
	 */
	int32 AddRings(const FVector& Center, const FVector& Up, float Size, int32 Count, const FLinearColor& Color);

	/**
	 * One wake (a V) at Origin on the surface with normal Up: its arms trail back from Direction (the way the body goes,
	 * any length) at Speed cm/s, for a body of Scale. False if refused (the surface isn't flat, or no direction).
	 */
	bool AddWake(const FVector& Origin, const FVector& Up, const FVector& Direction, float Speed, float Scale, const FLinearColor& Color);

	/** Time gone by: everything ages, the spent go, the budget refills. */
	void Tick(float DeltaSeconds);

	/**
	 * Adds this frame's dashes to Transforms (flat quads for the engine plane, 100 x 100 facing +Z) and, for each, four
	 * floats to SmokeData (R, G, B, opacity) as FWeaponFX's smoke instances read them. Camera is for the draw range.
	 */
	void Draw(const FVector& Camera, TArray<FTransform>& Transforms, TArray<float>& SmokeData) const;

	void Reset();
	bool IsEmpty() const { return Rings.IsEmpty() && Wakes.IsEmpty(); }
	int32 NumRings() const { return Rings.Num(); }
	int32 NumWakes() const { return Wakes.Num(); }
	float GetBudget() const { return Budget; }

private:
	struct FRing
	{
		FVector Center = FVector::ZeroVector;
		FVector Up = FVector::UpVector;
		FLinearColor Color = FLinearColor::White;
		/** Seconds since birth; below zero while it waits its turn (the second ring of a shot). */
		float Age = 0.f;
		float Life = 1.f;
		float StartRadius = 5.f;
		float MaxRadius = 90.f;
		/** The dash's width (cm), before it widens. */
		float Width = 8.f;
		float Opacity = 0.5f;
		int32 Segments = 12;
		uint32 Seed = 0;
	};

	struct FWake
	{
		FVector Origin = FVector::ZeroVector;
		FVector Up = FVector::UpVector;
		/** Along the surface: back from the way the body went, and across it. */
		FVector Back = FVector::BackwardVector;
		FVector Side = FVector::RightVector;
		FLinearColor Color = FLinearColor::White;
		float Age = 0.f;
		float ArmLength = 80.f;
		float Width = 7.f;
		float Opacity = 0.5f;
	};

	/** 0 to 1, the same for the same pair. */
	static float Hash01(uint32 Seed, uint32 Index);

	TArray<FRing> Rings;
	TArray<FWake> Wakes;
	float Budget = BudgetCap;
	uint32 NextSeed = 1;
};
