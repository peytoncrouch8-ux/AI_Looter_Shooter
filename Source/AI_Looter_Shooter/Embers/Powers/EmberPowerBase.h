#pragma once

#include "CoreMinimal.h"
#include "Embers/EmberPower.h"
#include "EmberPowerBase.generated.h"

class AActor;
class ACharacter;
class ACreatureBase;
class AController;
class APlayerController;
class FWeaponFX;
class UWorld;
struct FViewKick;

/**
 * What the four new heroes' twelve powers share (Embers/Powers/Ember<Power>; Odessa's, Crane's, Gauge's and Pike's):
 *  - The use: BeginUse keeps the body, its controller, the definition's colour and the level's scale (Ellis's powers keep
 *    the same, each in its own way).
 *  - Damage the contract's way (Hurt): UEmberDamageType, the player's controller as instigator and NO causer, so a kill
 *    gives its experience and notches no gun; a point hit, so the number shows where the power struck. Never critical.
 *  - Where the player aims (GetAim), where a power leaves the body as the player sees it (HandPoint), the living creature
 *    in their sights (FindInSights), whether a point sees another past the world's solid shapes (CanSee), the feet.
 *  - What lingers after End (cards still flying, a ring fading, a wire's last twang) runs on a next-frame timer chain on
 *    the player's own clock (StartLinger, Linger), as Dust Devil's and Borrowed Time's pumps do; the component ticks a
 *    power only while it lasts. A test level never ticks: the powers' step functions are public for the tests.
 */
UCLASS(Abstract)
class AI_LOOTER_SHOOTER_API UEmberPowerBase : public UEmberPower
{
	GENERATED_BODY()

public:
	/** Where the player's shots start and the way they go (UPlayerViewComponent::GetAimViewPoint; the eyes without one). Unit direction. */
	static void GetAim(const ACharacter& Character, FVector& OutStart, FVector& OutDirection);

	/**
	 * Where something a power throws leaves the body, as the player sees it: in first person a little right of and under
	 * the eye and out in front (Right, Down, Forward cm, in the view's frame); seen from outside, the chest and out in front.
	 */
	static FVector HandPoint(const ACharacter& Character, float Right = 18.f, float Down = 18.f, float Forward = 45.f);

	/** The bottom of the body's capsule. */
	static FVector FeetOf(const ACharacter& Character);

	/**
	 * The living creature nearest the crosshair within Range (cm) of the aim's start: within ConeDegrees of the aim or a
	 * body's width of the line, in sight of the eye (CanSee). The nearer to the line wins, then the nearer to the player.
	 * Null when there's none. Bosses count (each power says what it does to one).
	 */
	static ACreatureBase* FindInSights(const ACharacter& Character, float Range, float ConeDegrees);

	/** Nothing of the world's solid shapes between From and To (terrain, rocks, walls; never a creature, a volume, grass). */
	static bool CanSee(const UWorld& World, const FVector& From, const FVector& To, const AActor* Ignored = nullptr);

	/** The ground under Point within Above over and Below under it (world-static); false over nothing. */
	static bool FindGround(const UWorld& World, const FVector& Point, float Above, float Below, FVector& OutGround, FVector* OutNormal = nullptr,
		const AActor* Ignored = nullptr);

	/** A boss: Boss rank, or a creature a boss fight is built round (its fight's rules: never dragged, tripped or slowed). */
	static bool IsBoss(const ACreatureBase& Creature);

	/**
	 * How far the line from A to B passes from the creature's body (its capsule: the distance to its surface, 0 or less
	 * touching), and the point of the line nearest it (OutOnLine) and of its axis (OutOnBody).
	 */
	static float DistanceToBody(const ACreatureBase& Creature, const FVector& A, const FVector& B, FVector& OutOnLine, FVector& OutOnBody);

	/** The creature's body's middle, and its capsule's radius and half height (0s without a capsule). */
	static void BodyOf(const ACreatureBase& Creature, FVector& OutMiddle, float& OutRadius, float& OutHalfHeight);

	/** The power's damage this use (the definition's Damage, or Fallback, times the level's scale). */
	static float ScaledDamage(const FEmberContext& Context, float Fallback);

	/** The world's effects (the bullets' FWeaponFX), ready to draw; null without a world. */
	static FWeaponFX* EffectsIn(UWorld* World);

	/** The last hits it dealt, for the tests (a test level has no game mode, so the damage itself never lands there). */
	struct FDealt
	{
		TWeakObjectPtr<AActor> Target;
		float Damage = 0.f;
	};
	const TArray<FDealt>& GetDealt() const { return Dealt; }

protected:
	/** Keeps what the whole use works with: the body, its controller, the colour, the level's scale. False without a body. */
	bool BeginUse(const FEmberContext& Context);

	/** Deals Damage to Target where it struck (Where, from the way Along), the contract's way. Returns what it asked for. */
	float Hurt(AActor& Target, float Damage, const FVector& Where, const FVector& Along);

	UWorld* GetBodyWorld() const;
	APlayerController* GetPlayer() const;
	FWeaponFX* Effects() const { return EffectsIn(GetBodyWorld()); }
	/** A jolt of the player's view (nothing without a player). */
	void Kick(const FViewKick& ViewKick) const;
	/** The body's own seconds for this frame (the world's, at the player's clock). */
	float PlayerDeltaSeconds() const;

	/** Starts the next-frame chain (once; it runs while Linger says so). */
	void StartLinger();
	/** One frame of what lingers, DeltaSeconds on the player's clock; true to keep going. */
	virtual bool Linger(float DeltaSeconds) { return false; }

	TWeakObjectPtr<ACharacter> Body;
	TWeakObjectPtr<AController> Instigator;
	FLinearColor Color = FLinearColor::White;
	float LevelScale = 1.f;
	/** The latest use's hits (cleared by BeginUse). */
	TArray<FDealt> Dealt;

private:
	void Pump();
	bool bPumping = false;
};
