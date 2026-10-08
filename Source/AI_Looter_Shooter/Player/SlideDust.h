#pragma once

#include "CoreMinimal.h"

class ACharacter;
class AActor;
class UDirectionalLightComponent;
class UWorld;
class FWeaponFX;
struct FHitResult;

/** What the ground under a slide is, which decides how much it throws up. */
enum class ESlideGround : uint8
{
	/** Nothing under the feet (off a ledge): nothing thrown. */
	None,
	/** Nothing either: a slide through water throws no dust. */
	Water,
	/** A little fine dust, no grit. */
	Wood,
	/** Some dust and a little grit. */
	Grass,
	/** Pale dust and plenty of grit. */
	Stone,
	/** The most: also any ground the game can't name. */
	Dirt
};

/**
 * The dust and grit a slide kicks up from the feet: puffs that rise off the leading heel and trail behind as the slide
 * carries on past them, some pushed out ahead (what the first-person view sees at the bottom of the screen), and grains
 * thrown forward that arc and fall. It thins as the slide slows, comes in a burst as the body drops into it, and
 * depends on the ground: none on water, a little on wood. Drawn by FWeaponFX's pooled, instanced puffs and chips (the
 * bullet subsystem's), so it adds no draw calls, components or assets: a couple of traces a frame while it runs.
 *
 * Lit by the world: the grit is a lit surface, and the puffs (an unlit material) are given the color the light at the
 * slide shows: the sun's where the sun reaches it (a trace toward it a few times a second, eased across shadow edges),
 * the sky's in shade, both dimmed with the lighting state (MPC_Lighting's BackdropTint, as the unlit backdrop is), so
 * dust in shadow or at dusk doesn't glow. UPlayerLocomotionComponent runs it every frame.
 */
class AI_LOOTER_SHOOTER_API FSlideDust
{
public:
	/**
	 * This frame's dust. bSliding: a slide under way on the ground (false throws nothing; what's in the air settles by
	 * itself). bStarted: the slide's first frame (a burst). Speed along Direction (horizontal), TopSpeed the slide's most.
	 */
	void Update(const ACharacter& Character, bool bSliding, bool bStarted, const FVector& Direction, float Speed, float TopSpeed,
		float DeltaTime);

	/** The ground a name says (a material's, a mesh's): water, wood, grass, stone, dirt; None when it says nothing. */
	static ESlideGround GroundFromName(const FString& Name);
	/** The ground a trace hit: the material at the spot (then its parents), then the mesh's name; dirt if none says. */
	static ESlideGround GroundOf(const FHitResult& Hit);
	/** How much dust and grit each ground throws (shares of the most, at full speed). */
	static float DustAmount(ESlideGround Ground);
	static float GritAmount(ESlideGround Ground);
	/** The dust's own color on each ground, before the light. */
	static FLinearColor DustAlbedo(ESlideGround Ground);

	/** Puffs and grains thrown since the slide started, and the ground it was last on (for the tests). */
	int32 GetPuffsThrown() const { return PuffsThrown; }
	int32 GetGritThrown() const { return GritThrown; }
	ESlideGround GetGround() const { return Ground; }

private:
	/** The light at Where: the sun's share (traced, eased) and the sky's, times the lighting state's tint. */
	FLinearColor LightAt(const UWorld& World, const FVector& Where, const AActor& Ignore, float DeltaTime);
	/** The level's sun and the lighting state's tint, looked up as a slide starts. */
	void FindLight(const UWorld& World);
	void ThrowPuff(FWeaponFX& Effects, const FVector& Spot, const FVector& Along, const FVector& Side, float Speed, float Scale,
		const FLinearColor& Color, float Opacity, bool bAhead);
	void ThrowGrit(FWeaponFX& Effects, const FVector& Spot, const FVector& Along, const FVector& Side, float Speed, float Scale);

	FRandomStream Random{ 0x511de };
	/** Fractions of a puff or grain owed from earlier frames (the rates are per second). */
	float PuffsOwed = 0.f;
	float GritOwed = 0.f;
	int32 PuffsThrown = 0;
	int32 GritThrown = 0;
	ESlideGround Ground = ESlideGround::None;

	TWeakObjectPtr<const UDirectionalLightComponent> Sun;
	FLinearColor StateTint = FLinearColor::White;
	bool bLightFound = false;
	/** 0 in shade, 1 in the sun, eased; below 0 until the first trace. */
	float SunShare = -1.f;
	float SunTarget = 1.f;
	float SunCheckIn = 0.f;
};
