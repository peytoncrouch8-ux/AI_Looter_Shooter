#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LevelUpBurstSubsystem.generated.h"

class APawn;
class FWeaponFX;

/**
 * A level-up seen in the world, not only on the HUD (concept F, "level-up in the world"): as the level-up banner pops in
 * with its fanfare (UHudLevelUpBannerWidget::Show calls Play), a ring of pale cyan light bursts out along the ground
 * round the player's feet, following the ground's rise and fall, and embers in the banner's orange rise round them,
 * some left behind by the ring as it passes. Readable, not blinding: thin and soft, the embers dimmed near the eye, so
 * it reads from first person (the ring racing out over the ground ahead, embers rising past the view) and third alike.
 *
 * Drawn as glowing streaks through the bullet subsystem's pooled effects (FWeaponFX::AddStreak: no draws of its own);
 * the ground under the ring is looked up once as it starts. One burst at a time (a new level restarts it); it ticks
 * only while one plays. Once per level: the same level-up told twice (a banner shown again for it) bursts once.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULevelUpBurstSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static ULevelUpBurstSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * Bursts round Pawn's feet for NewLevel. False (and nothing) for no pawn, or for the level just burst again within
	 * RepeatSeconds (the same level-up told twice).
	 */
	bool Play(const APawn* Pawn, int32 NewLevel);

	/** Moves the burst on and draws it (the tick calls it; a test level, which doesn't tick, calls it directly). */
	void Advance(float DeltaSeconds);

	bool IsPlaying() const { return Time >= 0.f; }

	/** Bursts played this level, and the level of the last (tests). */
	int32 GetBurstCount() const { return BurstCount; }
	int32 GetLastLevel() const { return LastLevel; }

	/** The ring's radius now (cm; 0 once it has faded), embers glowing now, and streaks drawn last frame (tests). */
	float GetRingRadius() const;
	int32 NumEmbersGlowing() const { return EmbersGlowing; }
	int32 GetStreaksDrawn() const { return StreaksDrawn; }

	/** The whole show (s); the ring runs out over its first RingSeconds, to RingRadius (cm). */
	static constexpr float Seconds = 2.2f;
	static constexpr float RingSeconds = 0.8f;
	static constexpr float RingRadius = 360.f;
	/** The ring's pieces (and the spokes its ground is looked up along). */
	static constexpr int32 Segments = 36;
	/** Embers in all: some at the feet at once, the rest left by the ring as it passes. */
	static constexpr int32 Embers = 30;
	static constexpr int32 FeetEmbers = 10;
	/** The same level burst again within this long is the same level-up told twice (s). */
	static constexpr float RepeatSeconds = 5.f;

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FEmber
	{
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		/** Seconds into the show it appears, and how long it glows. */
		float Born = 0.f;
		float Life = 1.f;
		float Phase = 0.f;
		float Width = 2.f;
		float Glow = 1.f;
		bool bLaunched = false;
	};

	/** The ground's height over the feet along each spoke, halfway out and at the ring's end (cm), looked up once. */
	void FindGround(const APawn& Pawn);
	/** The ground's height over the feet along Spoke, Radius out. */
	float GroundAt(int32 Spoke, float Radius) const;
	/** A spot on the ring: Radius out along Spoke, just over the ground. */
	FVector RingPoint(int32 Spoke, float Radius) const;
	void DrawRing(FWeaponFX& Effects);
	void DrawEmbers(FWeaponFX& Effects, float DeltaSeconds, const FVector& Camera);

	FVector Center = FVector::ZeroVector;
	float Scale = 1.f;
	TArray<float> HalfwayHeight;
	TArray<float> EdgeHeight;
	TArray<FEmber> EmberList;
	/** Seconds into the show; below zero with none playing. */
	float Time = -1.f;
	int32 LastLevel = 0;
	double LastPlayed = -1.0e9;
	int32 BurstCount = 0;
	int32 EmbersGlowing = 0;
	int32 StreaksDrawn = 0;
	FRandomStream Random{ 0x1e7e1 };
};
