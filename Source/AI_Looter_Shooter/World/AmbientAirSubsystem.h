#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/Function.h"
#include "World/AmbientAirTypes.h"
#include "AmbientAirSubsystem.generated.h"

class AActor;
class AInteriorVisibility;
class UDirectionalLightComponent;
class UInstancedStaticMeshComponent;
class UMaterialParameterCollection;

/**
 * Things in the air round the player (board C item 5: "dust motes in sunbeams, pollen over the meadows, leaves drifting
 * under the cottonwoods, moths round lamps at dusk"): at most AmbientAirRules::Budget (250) at once, kept in a volume round
 * the camera, each kind only where it belongs (AmbientAirRules::Belongs, Target):
 *   - motes catching the sun by day, glinting brightest against it, gathered near under a roof (a doorway's, a porch's
 *     shafts);
 *   - pollen by day and seed fluff (fewer at dusk) over open grass and hay (the terrain's surface map, ImpactSurface);
 *   - leaves drifting down under the broadleaf trees near the camera (the scatter's, the town's and the orchard's, known
 *     by their Tree tag and their model: oaks, birches, apples);
 *   - moths circling the lit lamps at dusk (light components tagged Flicker, the flicker subsystem's lamps).
 * None in the mine or an interior's rooms (tag Underground, AInteriorVisibility inside). Gusts (UGustSubsystem) carry them
 * along, motes drawn as streaks while they race.
 *
 * Cheap by design: the level's trees, lamps and underground spaces are looked for once; a few traces a frame (a point
 * looked at as one is placed, a survey of the place now and then, a mote's sun again); particles recycled as they leave
 * their kind's reach of the view or end their life, fading in and out. Drawn as three instanced components on one
 * transient actor, one per material: motes and moths as glows (M_FX_Glow), pollen and fluff as soft puffs (M_FX_Smoke),
 * leaves as lit flakes (M_StylizedSurface); each keeps its pool's count of instances, so the frame's moves never rebuild
 * them. Unlit ones are lit in code as FSlideDust's dust is (the sun where it reaches, the sky's in shade, the lighting
 * state's tint).
 *
 * Looter.Air 0/1 switches it (and the gusts) off and on; Looter.Air.Density (0 to 1) thins it; Looter.Air.Glow brightens
 * the motes and moths; Looter.Air.Stats says what's out and what it costs (World/AmbientAirDevCommands.cpp).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UAmbientAirSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UAmbientAirSubsystem* Get(const UObject* WorldContextObject);

	/** Looter.Air: the air (and the gusts) on. */
	static bool IsEnabled();

	/** Looter.Air.Density: the share of each kind's cap that may be out (0 to 1). */
	static float GetDensity();

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** Moves the air on by DeltaSeconds round View (the tick does, with the player's camera; tests call it with their own). */
	void Step(float DeltaSeconds, const FAirView& View);

	/** Everything out goes at once (Looter.Air 0; tests). */
	void Clear();

	int32 CountOf(EAirKind Kind) const;
	int32 NumLive() const;
	const TArray<FAirParticle>& GetParticles() const { return Particles; }
	const FAirSurroundings& GetSurroundings() const { return Around; }
	const FAirView& GetLastView() const { return LastView; }

	/** The view is in the mine or an interior's rooms: nothing's out, and the gusts aren't heard. */
	bool IsSheltered() const { return LastView.bSheltered; }

	/** The sun reaches Point (one trace toward it; false with no sun up). */
	bool IsInSun(const FVector& Point) const;

	/**
	 * The light on something unlit (as FSlideDust lights its dust): the sky's in shade, the sun's in SunShare of its light
	 * (lower as it sinks), all times the lighting state's tint, so dust and pollen in shade or at dusk stay dim.
	 */
	FLinearColor LightFor(float SunShare) const;

	/** For tests: what a point is over, in place of tracing the world (the trees and lamps likewise, in place of the level's). */
	void SetPlaceProbe(TFunction<bool(const FVector&, FAirPlace&)> Probe);
	void SetTrees(const TArray<FAirTree>& InTrees);
	void SetLamps(const TArray<FAirLamp>& InLamps);
	const TArray<FAirTree>& GetTrees() const { return Trees; }
	const TArray<FAirLamp>& GetLamps() const { return Lamps; }

	/** Looks for the level's trees, lamps and underground spaces again (the first step does it; Looter.Air 1 too). */
	void GatherLevel();

	/** What it costs: the game thread's time a frame (ms, averaged) and traces a frame (averaged). */
	float GetAverageMilliseconds() const { return AverageMilliseconds; }
	float GetAverageTraces() const { return AverageTraces; }

	/** The most traces one frame may take. */
	static constexpr int32 MaxTracesPerFrame = 6;
	/** The most placements tried in a frame. */
	static constexpr int32 MaxPlacementsPerFrame = 3;

	/** How many kinds of air there are (EAirKind::Count). */
	static constexpr int32 KindCount = static_cast<int32>(EAirKind::Count);

private:
	/** The local player's camera, the lighting, the gust, and (every so often) the cover and shelter over it. */
	FAirView GatherView(float DeltaSeconds);
	bool IsShelteredAt(const FVector& Location) const;

	// --- Placing (AmbientAirSpawn.cpp) ---
	/** The trees and lit lamps near the view, and the place's shares, kept fresh. */
	void RefreshSurroundings(const FAirView& View, float DeltaSeconds);
	/** New ones where their kind is short, a few tries a frame. */
	void Place(const FAirView& View);
	bool TryPlace(EAirKind Kind, const FAirView& View);
	bool PlaceFromProbe(FAirParticle& Particle, EAirKind Kind, const FAirView& View);
	bool PlaceLeaf(FAirParticle& Particle, const FAirView& View);
	bool PlaceMoth(FAirParticle& Particle, const FAirView& View);
	/** What Point is over: the test probe's answer, or the world's (a trace down from above it, and one toward the sun). */
	bool Probe(const FVector& Point, bool bGround, bool bSun, FAirPlace& OutPlace);
	/** A point round the view at Kind's reach, mostly in front of it. */
	FVector PointAround(const FAirView& View, float MinReach, float MaxReach, float Z);
	/** Ones over their kind's wanted count leave (fading). */
	void Thin(const FAirView& View);
	int32 FreeSlot();
	void Begin(FAirParticle& Particle, EAirKind Kind);
	bool IsLampLit(int32 Lamp) const;

	// --- Moving (AmbientAirMotion.cpp) ---
	void Move(const FAirView& View, float DeltaSeconds);
	void MoveOne(FAirParticle& Particle, const FAirView& View, float DeltaSeconds);
	/** Out of its reach, its life done, its fade over: back to the pool. */
	bool IsDone(const FAirParticle& Particle, const FAirView& View) const;
	void Retire(FAirParticle& Particle);
	void RecheckSun();

	// --- Drawing and the light (AmbientAirDraw.cpp) ---
	void Draw(const FAirView& View);
	void MakeComponents();
	void RefreshLight();

	TArray<FAirParticle> Particles;
	int32 Counts[KindCount] = {};
	/** After a miss (no place for it near: a dark room, a road), a kind waits a little before it's tried again, longer each miss. */
	float KindWait[KindCount] = {};
	int32 KindMisses[KindCount] = {};
	int32 SlotCursor = 0;
	int32 SunCursor = 0;
	FAirSurroundings Around;
	FAirView LastView;
	FRandomStream Random{ 0xA112 };

	// The level, looked for once.
	bool bGathered = false;
	bool bTreesGiven = false;
	bool bLampsGiven = false;
	TArray<FAirTree> Trees;
	TMap<FIntPoint, TArray<int32>> TreeCells;
	TArray<FAirLamp> Lamps;
	TArray<FBox> UndergroundZones;
	TArray<TWeakObjectPtr<AInteriorVisibility>> Interiors;
	TFunction<bool(const FVector&, FAirPlace&)> PlaceProbe;
	FCollisionQueryParams GroundParams;
	FCollisionQueryParams SunParams;

	// Near the view, looked at again a few times a second.
	TArray<int32> NearTrees;
	TArray<int32> NearLamps;
	FVector NearFrom = FVector(TNumericLimits<float>::Max());
	float NearIn = 0.f;
	float SurveyIn = 0.f;
	float ShelterIn = 0.f;
	bool bUnderCover = false;
	bool bSheltered = false;
	FName LastLighting;
	bool bLightingKnown = false;

	// The light, for the unlit ones.
	TWeakObjectPtr<const UDirectionalLightComponent> Sun;
	FVector ToSun = FVector::UpVector;
	FLinearColor SunHue = FLinearColor::White;
	float SunHeight = 1.f;
	FLinearColor StateTint = FLinearColor::White;
	float LightIn = 0.f;
	/** MPC_Lighting (its BackdropTint is the lighting state's tint), looked for once. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> LightingCollection;
	bool bLightingCollectionLooked = false;

	// Drawing.
	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Glows;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Puffs;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Leaves;
	bool bDrawn = false;
	bool bComponentsMade = false;

	// Its cost.
	int32 TracesLeft = 0;
	int32 TracesThisFrame = 0;
	float AverageMilliseconds = 0.f;
	float AverageTraces = 0.f;
	bool bWasEnabled = true;
};
