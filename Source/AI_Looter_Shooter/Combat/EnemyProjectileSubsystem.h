#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyProjectileSubsystem.generated.h"

class AActor;
class ACharacter;
class AController;
class UInstancedStaticMeshComponent;

/** One enemy pellet as it leaves the shooter: everything it needs to fly and to hurt, copied (the shooter may be gone). */
struct FEnemyShot
{
	FVector Start = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	/** cm/s: enemy pellets are slow enough to see and step out of. */
	float Speed = 1000.f;
	/** How far it flies (cm) before it fades out. */
	float Range = 4000.f;
	/** Its radius (cm): how close it must pass a player's capsule to hit, and how big it's drawn. */
	float Radius = 12.f;
	/** Damage before the per-hit roll (LooterCombat::DamageVariance, as a bite rolls). */
	float Damage = 5.f;
	FLinearColor Color = FLinearColor(0.55f, 1.f, 0.2f);
	/** Who fired it (never hurt by it) and who gets the blame for its damage. */
	TWeakObjectPtr<AActor> Shooter;
	TWeakObjectPtr<AController> Instigator;
};

/** A pellet hit a player (its damage has been dealt): who, whose pellet, how much. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEnemyShotHit, AActor* /*Victim*/, AActor* /*Shooter*/, float /*Damage*/);

/**
 * Pellets that creatures fire (a boss's spectral buckshot): slow, glowing balls that fly straight, stop on the world and
 * hurt the player they touch. They live here like bullets in UBulletSubsystem, as plain data, so a volley costs no actors.
 *
 * How a pellet finds the player: each update its short path is swept as a sphere against the capsule of every living
 * character that isn't a creature, in plain geometry (no collision channel). Pellets then never hit the shooter or its
 * adds, whatever their collision says (the rule is CanHurt), need no change to the project's collision profiles, and hit
 * the same in a test level as in the game. The world stops them with one line trace per pellet against world-static
 * geometry (terrain, rocks, buildings), passing through volumes, the playable area's walls and a boss's fog wall.
 *
 * They're drawn as emissive spheres on one instanced mesh per color (M_StylizedSurface's glow, opaque: no translucency),
 * stretched a little along their flight. A volley's tell is a glow swelling at the shooter's muzzle (ShowCharge), and a
 * pellet that hits the world swells and is gone.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEnemyProjectileSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The most pellets in flight at once; past it the oldest goes, so a runaway spawner can't flood the frame. */
	static constexpr int32 MaxShots = 256;

	void Fire(const FEnemyShot& Shot);

	/** Count pellets like Pellet in a cone SpreadDegrees wide about Aim (VolleyDirections). */
	void FireVolley(const FEnemyShot& Pellet, const FVector& Aim, int32 Count, float SpreadDegrees);

	/**
	 * A harmless glow that swells at Shooter's LocalOffset (its own frame, so it follows it) over Seconds, up to Radius: the
	 * tell before a volley.
	 */
	void ShowCharge(AActor* Shooter, const FVector& LocalOffset, float Seconds, float Radius, const FLinearColor& Color);

	/** Puts out every pellet and tell a shooter has going (a boss fight's reset). Safe to call from a hit's handlers. */
	void ClearShotsFrom(const AActor* Shooter);

	int32 NumShotsInFlight() const;
	int32 NumShotsFrom(const AActor* Shooter) const;

	/**
	 * Whether a pellet fired by Shooter may hurt Victim: a living character that isn't a creature (the player), never the
	 * shooter itself. Creatures are never hurt by pellets, so a boss's shots pass through its own adds.
	 */
	static bool CanHurt(const AActor* Shooter, const AActor* Victim);

	/**
	 * Whether a pellet of Radius moving From -> To touches an upright capsule; OutTime is how far along the move (0-1) it
	 * comes closest.
	 */
	static bool SweepHitsCapsule(const FVector& From, const FVector& To, float Radius, const FVector& CapsuleCenter, float CapsuleRadius,
		float CapsuleHalfHeight, float& OutTime);

	/**
	 * The directions of a volley's Count pellets in a cone SpreadDegrees wide about Aim: the first straight down the middle,
	 * the rest evenly round the cone's edge, so every volley has the same readable shape.
	 */
	static TArray<FVector> VolleyDirections(const FVector& Aim, int32 Count, float SpreadDegrees);

	/** A pellet hit a player. */
	FOnEnemyShotHit OnShotHit;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UEnemyProjectileSubsystem, STATGROUP_Tickables); }
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FPellet
	{
		FEnemyShot Shot;
		FVector Position = FVector::ZeroVector;
		float Traveled = 0.f;
		/** Hit something, flew its range or was put out: removed at the next update. */
		bool bSpent = false;
	};

	struct FCharge
	{
		TWeakObjectPtr<AActor> Shooter;
		FVector Offset = FVector::ZeroVector;
		float Age = 0.f;
		float Life = 0.5f;
		float Radius = 12.f;
		FLinearColor Color = FLinearColor::White;
	};

	/** A pellet that hits the world swells for this long (seconds) where it stopped, then it's gone. */
	static constexpr float PopSeconds = 0.12f;

	/** A pellet that hit the world, swelling for a moment where it stopped. */
	struct FPop
	{
		FVector Location = FVector::ZeroVector;
		float Age = 0.f;
		float Radius = 12.f;
		FLinearColor Color = FLinearColor::White;
	};

	/** Moves pellet Index on by DeltaSeconds; hurts the player it reaches (last, after the pellet's own bookkeeping). */
	void Advance(int32 Index, float DeltaSeconds, const TArray<TWeakObjectPtr<ACharacter>>& Victims, const FCollisionQueryParams& WorldQuery);
	/** Everyone a pellet could hurt this update: living characters that aren't creatures. */
	TArray<TWeakObjectPtr<ACharacter>> GatherVictims() const;
	/** The world stops pellets, but not volumes (their brushes), the playable area's walls or the scatter they hold back. */
	FCollisionQueryParams MakeWorldQuery() const;

	/** Puts every pellet, tell and pop on screen (game worlds only: a test level draws nothing). EnemyProjectileSubsystemDraw.cpp. */
	void Draw();
	UInstancedStaticMeshComponent* InstancesFor(const FLinearColor& Color);

	TArray<FPellet> Pellets;
	TArray<FCharge> Charges;
	TArray<FPop> Pops;

	/** A transient actor holding the instanced meshes, one per color (keyed by the color as bytes). */
	TWeakObjectPtr<AActor> DrawActor;
	TMap<uint32, TWeakObjectPtr<UInstancedStaticMeshComponent>> Instances;
	/** Colors drawn last update, so a color with nothing left is cleared exactly once. */
	TSet<uint32> DrawnColors;
};
