#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BellRingerRing.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;

/** The Bell-Ringer's lights (ABellRingerRing). */
enum class EBellRingKind : uint8
{
	/** The toll's ring, running out over the ground from the ringer's feet. */
	Wave,
	/** The toll at the bell: a ring round it at its height, bursting out in a moment. */
	Burst,
	/** A fallen called up: a ring closing in over the ground on its grave as it rises. */
	Gather,
	/** The tell: dashes circling the bell, brighter as the tell builds. */
	Halo,
	/** A steadied creature's faint ring at its feet, pulsing while its quickening lasts. */
	Aura,
};

/**
 * The Bell-Ringer's lights (ABellRingerCreature), code-drawn as the shriek's ring is (AShriekRing): glowing dashes of the
 * engine's cube on one instanced mesh, opaque glow (M_StylizedSurface's), no translucency and no particles. A wave, a
 * burst and a gather play out and go by themselves; a halo and an aura stay on what they're attached to until Finish.
 * Its instances are laid in its own space, so an attached one follows the bell or the creature without being laid again.
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API ABellRingerRing : public AActor
{
	GENERATED_BODY()

public:
	ABellRingerRing();

	/** The toll's ring over the ground from Center (on the ground) out to InRadius, then thinning away. */
	static ABellRingerRing* SpawnWave(UWorld* World, const FVector& Center, float InRadius, const FLinearColor& InColor);

	/** The toll at the bell: a ring round Center at its height that bursts out to a meter and a half in a third of a second. */
	static ABellRingerRing* SpawnBurst(UWorld* World, const FVector& Center, const FLinearColor& InColor);

	/** A fallen called up: a ring over the ground closing in from InRadius on Center (its grave) as it rises. */
	static ABellRingerRing* SpawnGather(UWorld* World, const FVector& Center, float InRadius, const FLinearColor& InColor);

	/** The tell's halo, attached to Bell at Offset (its own space): dashes circling InRadius round it. SetStrength builds it. */
	static ABellRingerRing* SpawnHalo(USceneComponent* Bell, const FVector& Offset, float InRadius, const FLinearColor& InColor);

	/** A steadied creature's aura, attached to On FeetBelow under its middle: a faint ring InRadius round its feet. */
	static ABellRingerRing* SpawnAura(AActor* On, float FeetBelow, float InRadius, const FLinearColor& InColor);

	virtual void Tick(float DeltaSeconds) override;

	/** Moves it on by DeltaSeconds: it widens, closes, spins or pulses, fades and goes. Its tick calls it; the tests call it. */
	void Advance(float DeltaSeconds);

	/** A halo's strength (0 to 1): its dashes grow and brighten with it. */
	void SetStrength(float InStrength);

	/** A halo or an aura lets go: it fades and goes. The others go by themselves. */
	void Finish();

	EBellRingKind GetKind() const { return Kind; }
	float GetRadius() const { return Radius; }
	float GetStrength() const { return Strength; }
	bool IsFinishing() const { return bFinishing; }

private:
	static ABellRingerRing* SpawnRing(UWorld* World, const FTransform& Where, EBellRingKind InKind, const FLinearColor& InColor,
		float InRadius);

	/** Lays the dashes round the ring as it is now (in its own space). */
	void Draw();

	/** How much of it shows now (1 whole, 0 gone): its fade at the end. */
	float Showing() const;

	UPROPERTY(VisibleAnywhere, Category = "Bell-Ringer")
	TObjectPtr<UInstancedStaticMeshComponent> Dashes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Glow;

	EBellRingKind Kind = EBellRingKind::Wave;
	FLinearColor Color = FLinearColor::White;
	float MaxRadius = 100.f;
	float Radius = 0.f;
	float Age = 0.f;
	float Strength = 1.f;
	float FinishAge = 0.f;
	bool bFinishing = false;
	float ShownGlow = -1.f;
	float ShownSize = -1.f;
	float ShownRadius = -1.f;
};
