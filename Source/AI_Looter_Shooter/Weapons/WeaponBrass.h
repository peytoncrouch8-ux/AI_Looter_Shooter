#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponTypes.h"

class AActor;
class UInstancedStaticMeshComponent;
class UWorld;

/** Which spent case a gun throws. */
enum class ECasingKind : uint8
{
	/** The rifle's brass case, flicked out to the right on every shot. */
	Rifle,
	/** The shotgun's oxblood paper hull on its brass head, thrown out as the gun is pumped. */
	Hull,
	/** The six-gun's straight case: none on firing, the empties all tumble out on a reload's eject. */
	Pistol,
	// Gun identity (2026-10-28, Looter.Gunfeel.Identity 1; appended): the SMG, sniper and LMG stop throwing the rifle's case.
	/** The SMG's small, fast case. */
	Small,
	/** The sniper's fat magnum case, drawn out lazily with the bolt. */
	Magnum,
	/** The LMG's case, thrown down and to the right with its belt link. */
	Belt,
	/** The LMG's belt link: a dark brass tab that falls with each round's case (drawn on its own instances, the last MaxLinks kept). */
	Link
};

/** One spent case leaving a gun. */
struct FCasingLaunch
{
	ECasingKind Kind = ECasingKind::Rifle;
	FVector Location = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	/** The case's length runs along the rotation's Z. */
	FQuat Rotation = FQuat::Identity;
	/** Tumble: a world axis times its speed (radians a second). */
	FVector Spin = FVector::ZeroVector;
	/** What it lands on is traced on this channel (the gun's own: what stops bullets stops brass). */
	ECollisionChannel Channel = ECC_Visibility;
	/** Never landed on: whoever threw it, and the gun. */
	TWeakObjectPtr<const AActor> Holder;
	TWeakObjectPtr<const AActor> Gun;
};

/**
 * Spent brass (UBulletSubsystem keeps it): cases flicked from a gun's ejection port that tumble on a simple ballistic arc,
 * bounce on what they land on (at most MaxBounces times, each landing found by tracing the arc ahead once: two or three
 * short chords a leg, never a trace a frame), tinkle by the surface (a clink on stone, a knock on wood, a dull tick in the
 * dirt, a ring on metal), then lie there RestSeconds before they sink away. The last MaxCasings are kept: past it the
 * oldest goes.
 *
 * Cases are code-drawn on two instanced meshes (the engine's cylinder in the ammo pickups' MI_AmmoCaseBrass and
 * MI_AmmoShellHull: one draw each), redrawn only on frames something moved, faded or went: brass lying still costs no
 * updates.
 */
class AI_LOOTER_SHOOTER_API FWeaponBrass
{
public:
	static constexpr int32 MaxCasings = 60;
	/** The belt links kept, apart from the cases (an LMG throws one with every round). */
	static constexpr int32 MaxLinks = 40;
	/** Bounces after the first landing. */
	static constexpr int32 MaxBounces = 2;
	/** How long a case lies before it fades, and how long it takes to sink away. */
	static constexpr float RestSeconds = 20.f;
	static constexpr float FadeSeconds = 1.2f;
	/** How far a case may fall before it's given up (off a cliff, into nothing). */
	static constexpr float MaxFall = 800.f;

	/** Makes the transient actor that draws the cases. Safe to call again (does nothing). */
	void Initialize(UWorld* World);
	void Shutdown();

	/** A case leaves a gun: it's traced to where it lands now. Past the cap, the oldest goes. */
	void Eject(const FCasingLaunch& Launch);

	/** Moves the flying cases on, lands and bounces them, ages the resting ones, and redraws what changed. */
	void Tick(float DeltaSeconds, const FVector& CameraLocation);

	/** No cases at all and nothing drawn: ticking can stop. */
	bool IsIdle() const { return Casings.IsEmpty() && !bDrawnAnything; }

	int32 Num() const { return Casings.Num(); }
	int32 NumFlying() const;
	int32 NumOfKind(ECasingKind Kind) const;
	/** Whether a case of Kind is drawn as an engine cube on the link instances (otherwise a cylinder). */
	static bool IsLink(ECasingKind Kind) { return Kind == ECasingKind::Link; }
	/** Where each case is now (tests). */
	TArray<FVector> GetLocations() const;

	/** The last case launched, and how many have been (tests). */
	const FCasingLaunch& GetLastLaunch() const { return LastLaunch; }
	int32 GetLaunches() const { return Launches; }

	/** The case a gun of Kind throws (by Looter.Gunfeel.Identity: with it off, the rifle's case for the SMG, LMG and sniper, as before). */
	static ECasingKind KindFor(EWeaponKind Kind);
	/** The same with the identity switch given: on, the SMG throws a Small case, the LMG a Belt one, the sniper a Magnum one. */
	static ECasingKind KindFor(EWeaponKind Kind, bool bIdentity);

	/** The case's length and diameter (cm); a link's length and width (it is 0.25 thick). */
	static FVector2D SizeOf(ECasingKind Kind);

	/** How far a resting case's middle sits off the ground it lies on (cm): half its diameter, half a link's thickness. */
	static float HalfThickness(ECasingKind Kind);

	/** The sound of a case landing on Surface (a hull's plastic knock on anything hard). */
	static FName CueFor(ECasingKind Kind, EImpactMaterial Surface);

	/** Soft ground (dirt, grass, sand, hay, cloth): a case barely bounces and the sound is a dull tick. */
	static bool IsSoft(EImpactMaterial Surface);

private:
	struct FCasing
	{
		ECasingKind Kind = ECasingKind::Rifle;
		/** Its arc now: from LegStart at LegVelocity, LegTime seconds in. */
		FVector LegStart = FVector::ZeroVector;
		FVector LegVelocity = FVector::ZeroVector;
		float LegTime = 0.f;
		/** Seconds into the leg it lands (-1: nothing under it within MaxFall) and how long the leg may last. */
		float LegLanding = -1.f;
		float LegHorizon = 1.f;
		FVector LandPoint = FVector::ZeroVector;
		FVector LandNormal = FVector::UpVector;
		EImpactMaterial LandSurface = EImpactMaterial::Unknown;
		FVector Location = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FVector Spin = FVector::ZeroVector;
		/** Landings so far; lying still since RestAge seconds. */
		int32 Contacts = 0;
		bool bResting = false;
		float RestAge = 0.f;
		ECollisionChannel Channel = ECC_Visibility;
		TWeakObjectPtr<const AActor> Holder;
		TWeakObjectPtr<const AActor> Gun;
	};

	/** Starts a leg of its arc from From at Velocity, and traces it ahead to where it lands. */
	void StartLeg(FCasing& Casing, const FVector& From, const FVector& Velocity);
	/** It reached its landing: a sound, then a bounce (a new leg) or it lies still. False: it's gone (into water). */
	bool Land(FCasing& Casing);
	void PlayLanding(const FCasing& Casing, const FVector& Where);
	void Redraw();

	TWeakObjectPtr<UWorld> WorldRef;
	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<UInstancedStaticMeshComponent> BrassInstances;
	TWeakObjectPtr<UInstancedStaticMeshComponent> HullInstances;
	TWeakObjectPtr<UInstancedStaticMeshComponent> LinkInstances;

	TArray<FCasing> Casings;
	FCasingLaunch LastLaunch;
	int32 Launches = 0;
	FVector Camera = FVector::ZeroVector;
	/** Something moved, faded, came or went: redraw. */
	bool bDirty = false;
	bool bDrawnAnything = false;
	double LastSoundTime = -1000.0;
	FRandomStream Random{ 0xb4a55 };
};
