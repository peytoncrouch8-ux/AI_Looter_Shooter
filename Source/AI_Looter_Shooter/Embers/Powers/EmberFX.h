#pragma once

#include "CoreMinimal.h"

class AActor;
class FWeaponFX;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UObject;
class UWorld;

/**
 * The twelve new powers' shared looks, drawn with the bullets' code-drawn effects (FWeaponFX: glowing streaks, dust and
 * smoke puffs, grit), so there are no particle assets: glowing sparks that fly and fall (FEmberSparks), rings and lines
 * laid on the ground (GroundRing, DrawLine), a playing card (DrawCard), and a creature's body tinted for a while through
 * the hit flash's overlay material (FEmberTint: a slag burn's orange, a spyglass mark's).
 */
namespace EmberFX
{
	/** Dust in the light where it's thrown (the puffs are unlit), as the creatures' and the slide's is. */
	AI_LOOTER_SHOOTER_API FLinearColor DustColor(const UObject* WorldContext);

	/**
	 * Segments points on a circle of Radius round Center, each laid on the ground under it (world-static, within Reach over
	 * and under Center) and lifted Lift; a point over nothing keeps Center's height. For rings that stay put (compute once).
	 */
	AI_LOOTER_SHOOTER_API TArray<FVector> GroundRing(const UWorld& World, const FVector& Center, float Radius, int32 Segments, float Reach,
		float Lift);

	/** A glowing polyline through Points this frame (Closed: back to the first), Width cm, at Glow. */
	AI_LOOTER_SHOOTER_API void DrawLine(FWeaponFX& Effects, const TArray<FVector>& Points, bool bClosed, float Width, const FLinearColor& Color,
		float Glow);

	/**
	 * A glowing ring this frame: Radius round Center, square to Axis, in Segments streaks. Square to the way something flies,
	 * it faces whoever looks along that way: a shock ring reads from behind where a line along the flight shows end-on.
	 */
	AI_LOOTER_SHOOTER_API void DrawRing(FWeaponFX& Effects, const FVector& Center, const FVector& Axis, float Radius, int32 Segments, float Width,
		const FLinearColor& Color, float Glow);

	/**
	 * A playing card seen this frame at Center, its long side along Along (any length), turned Spin radians round it:
	 * a pale face with a coloured edge, as two short wide streaks (the face and its glinting rim).
	 */
	AI_LOOTER_SHOOTER_API void DrawCard(FWeaponFX& Effects, const FVector& Center, const FVector& Along, float Spin, const FLinearColor& Edge,
		float Glow, float Size = 1.f);

	// --- Real cards (EmberFXCards.cpp; the user's note 6, 2026-10-27: "shoots tiny particles, needs ... bigger card like
	// projectiles") ---

	/** A playing card as modeled (cm): long side, short side. Drawn CardSize() times that. */
	inline constexpr float CardLong = 8.9f;
	inline constexpr float CardShort = 6.4f;

	/** Looter.Embers.Cards.Size: how much bigger than a real card a thrown one is drawn, so a fan reads at range. */
	AI_LOOTER_SHOOTER_API float CardSize();

	/**
	 * A thrown card's turn this frame (X along its long side, Z out of its face): flying along Velocity, its face tipped
	 * back toward the thrower by Looter.Embers.Cards.Tilt (a card flying flat is seen edge on from behind, a sliver) plus a
	 * flutter of Looter.Embers.Cards.Flutter at its Flutter phase (radians), rolled Roll radians about its way, spun Spin
	 * radians about its face.
	 */
	AI_LOOTER_SHOOTER_API FQuat FlyingCardTurn(const FVector& Velocity, float Spin, float Flutter, float Roll);

	/**
	 * Cards drawn as real cards: an ivory face with a red pip, the back in the power's colour (Odessa's red), each a pair of
	 * the engine's planes back to back on M_StylizedSurface (as her idle card, UBodyFigureComponent), lit like the world and
	 * glowing a little so they read in shade. One deck per power; Begin, Add each card, Flush once a frame. Its components sit
	 * on a transient actor of its own, made at the first Flush in a world; an empty Flush hides them.
	 */
	class AI_LOOTER_SHOOTER_API FCardDeck
	{
	public:
		/** A new frame: no cards yet. */
		void Begin() { Pending.Reset(); }

		/** One card this frame at Center, turned Turn (FlyingCardTurn), Size times a real card (CardSize() for a thrown one). */
		void Add(const FVector& Center, const FQuat& Turn, float Size);

		/** Draws this frame's cards in World with their backs in Back. */
		void Flush(UWorld* World, const FLinearColor& Back);

		int32 NumDrawn() const { return Drawn; }

	private:
		struct FCardDraw
		{
			FVector Center;
			FQuat Turn;
			float Size;
		};
		bool Make(UWorld& World, const FLinearColor& Back);

		TArray<FCardDraw> Pending;
		TWeakObjectPtr<AActor> Owner;
		TWeakObjectPtr<UInstancedStaticMeshComponent> Faces;
		TWeakObjectPtr<UInstancedStaticMeshComponent> Backs;
		TWeakObjectPtr<UInstancedStaticMeshComponent> Pips;
		int32 Drawn = 0;
	};

	/** Glowing sparks thrown out that fall, slow in the air and fade (slag's, a spike's, a lantern's motes). Drawn as streaks. */
	class AI_LOOTER_SHOOTER_API FEmberSparks
	{
	public:
		/** One spark from Location at Velocity, Life seconds, Width cm across, pulled down by Gravity (cm/s/s; 0 floats). */
		void Spawn(const FVector& Location, const FVector& Velocity, float Life, float Width, const FLinearColor& Color, float Glow,
			float Gravity = 980.f, float Drag = 1.5f);

		/** Moves every spark on by DeltaSeconds and draws it (a streak along its motion). */
		void Tick(float DeltaSeconds, FWeaponFX* Effects);

		bool IsEmpty() const { return Sparks.IsEmpty(); }
		int32 Num() const { return Sparks.Num(); }
		void Reset() { Sparks.Reset(); }

		/** At most this many at once: the oldest gives way. */
		static constexpr int32 MaxSparks = 256;

	private:
		struct FSpark
		{
			FVector Location = FVector::ZeroVector;
			FVector Velocity = FVector::ZeroVector;
			float Age = 0.f;
			float Life = 1.f;
			float Width = 2.f;
			FLinearColor Color = FLinearColor::White;
			float Glow = 10.f;
			float Gravity = 980.f;
			float Drag = 1.5f;
		};
		TArray<FSpark> Sparks;
	};

	/**
	 * A creature's body worn in one colour for a while (the hit flash's overlay material, /Game/Weapons/FX/M_FX_HitFlash,
	 * Color and Intensity): put on each skinned mesh that wears no other overlay (the hit flash keeps its own, and this
	 * waits for it), taken off only where it's still this one's. One per use; Remove on every actor before it goes.
	 * It holds its material weakly: the power keeps it in a UPROPERTY so it isn't collected (MakeMaterial).
	 */
	class AI_LOOTER_SHOOTER_API FEmberTint
	{
	public:
		/** The tint's material in Color for Outer (null when M_FX_HitFlash isn't in the game). Keep it in a UPROPERTY. */
		static UMaterialInstanceDynamic* MakeMaterial(UObject* Outer, const FLinearColor& Color);

		/** What it wears from now (the power's, from MakeMaterial). */
		void SetMaterial(UMaterialInstanceDynamic* InMaterial);

		/** Its strength now (all it's worn on). */
		void SetIntensity(float Intensity);

		/** Wears it on Actor's skinned meshes that have no overlay (call again to catch one a hit flash had). */
		void Wear(AActor& Actor);

		/** Takes it off Actor where it's still worn. */
		void Remove(AActor& Actor);

		/** Off every actor it was put on. */
		void RemoveAll();

		bool IsWornOn(const AActor& Actor) const;

	private:
		TWeakObjectPtr<UMaterialInstanceDynamic> Material;
		TArray<TWeakObjectPtr<AActor>> WornOn;
	};
}
