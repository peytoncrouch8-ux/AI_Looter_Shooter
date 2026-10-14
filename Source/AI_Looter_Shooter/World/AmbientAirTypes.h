#pragma once

#include "CoreMinimal.h"
#include "Weapons/ImpactSurface.h"

class ULightComponent;

/** What drifts in the air round the player (UAmbientAirSubsystem). */
enum class EAirKind : uint8
{
	/** Dust motes catching the sun: by day, in the sun, more in a doorway's or porch's shafts. Tiny glints (M_FX_Glow). */
	Mote,
	/** Pollen over the meadows and fields: by day, over grass and hay under the open sky. Pale specks (M_FX_Smoke). */
	Pollen,
	/** Seed fluff drifting over the same ground, by day and fewer at dusk. Soft white tufts (M_FX_Smoke). */
	Fluff,
	/** Leaves drifting down under the broadleaf trees (oaks, birches, the orchard's apples). Lit flakes. */
	Leaf,
	/** Moths circling the lit lamps (lights tagged Flicker) at dusk. Lamp-lit flecks (M_FX_Glow). */
	Moth,
	Count
};

/** One thing in the air (in play only). Kept in a fixed pool: a dead one is a free slot. */
struct FAirParticle
{
	EAirKind Kind = EAirKind::Mote;
	bool bLive = false;
	FVector Location = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Age = 0.f;
	float Life = 10.f;
	float Size = 1.f;
	/** Going early (its kind no longer wanted, its lamp out, the view gone in): seconds left of its fade, 0 while it stays. */
	float Leaving = 0.f;
	/** The sun reaches it (motes, pollen, fluff; a mote's is looked at again now and then). */
	bool bInSun = false;
	/** Its own phases for the wander, the bob, the sway, the flutter. */
	FVector Phase = FVector::ZeroVector;
	/** Where its wander is heading (cm/s), and until it picks another (s). */
	FVector Wander = FVector::ZeroVector;
	float Timer = 0.f;
	/** A leaf's tumble: its turn, and its spin (axis times radians a second). */
	FQuat Rotation = FQuat::Identity;
	FVector Spin = FVector::ZeroVector;
	/** A leaf's sway across its fall (level, unit) and the ground it lands on (z); lying there for Landed seconds (below 0: falling). */
	FVector Sway = FVector::ForwardVector;
	float Floor = 0.f;
	float Landed = -1.f;
	/** The tree a leaf fell from or the lamp a moth circles (an index into the subsystem's lists). */
	int32 Anchor = INDEX_NONE;
	/** A moth's circling: how far out, how high against the lamp, where round it and how fast (radians a second). */
	float Orbit = 0.f;
	float Height = 0.f;
	float OrbitAngle = 0.f;
	float OrbitSpeed = 0.f;
};

/** What a point in the air is over, as the probe found it. */
struct FAirPlace
{
	/** Ground was found under it, at GroundZ. */
	bool bGround = false;
	float GroundZ = 0.f;
	/** The ground is open to the sky there: walkable terrain (tagged Ground) was the first thing met coming down from above. */
	bool bOpenSky = false;
	/** What that ground is (the terrain's surface map: grass, dirt, gravel...). */
	EImpactMaterial Surface = EImpactMaterial::Unknown;
	/** The sun reaches the point. */
	bool bInSun = false;
};

/** A broadleaf tree whose leaves fall (an oak, a birch, an orchard apple): its foot and its crown's reach and heights (cm). */
struct FAirTree
{
	FVector Base = FVector::ZeroVector;
	float CrownRadius = 300.f;
	float CrownBottom = 300.f;
	float CrownTop = 800.f;
};

/** A lamp moths come to (a light tagged Flicker): where it is, and its light (tests give none and say bLit). */
struct FAirLamp
{
	FVector Location = FVector::ZeroVector;
	TWeakObjectPtr<const ULightComponent> Light;
	bool bLit = true;
};

/** The view the air is kept round, and what's known of it this frame. */
struct FAirView
{
	bool bValid = false;
	FVector Location = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float FieldOfView = 90.f;
	/** The level's lighting state (Day, Dusk; None in a level without states: day). */
	FName Lighting;
	/** In the mine or an interior's rooms (tags Underground, AInteriorVisibility inside): nothing out. */
	bool bSheltered = false;
	/** A roof over the view (a porch, a doorway, a room): motes keep near, in the sun's shafts. */
	bool bUnderCover = false;
	/** The prevailing breeze's way, and the gust now: its way and push (0 to 1). */
	FVector BreezeDirection = FVector::ForwardVector;
	FVector GustDirection = FVector::ForwardVector;
	float GustStrength = 0.f;
};

/** The place round the view as the probes have found it lately (shares 0 to 1), and what's near. */
struct FAirSurroundings
{
	/** Of the points looked at, the share over open grass or hay, and the share in the sun. */
	float GrassShare = 0.5f;
	float SunShare = 0.5f;
	/** Broadleaf trees and lit lamps within reach. */
	int32 NearTrees = 0;
	int32 NearLamps = 0;
};
