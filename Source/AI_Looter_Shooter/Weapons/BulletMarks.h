#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponTypes.h"

class AActor;
class UDecalComponent;
class UMaterialInterface;
class UWorld;
struct FHitResult;

/** How a surface's mark looks: its style in the marks' atlas, and its size for a rifle round. */
struct FBulletMarkLook
{
	/**
	 * The style: a row pair of the atlas (T_BulletMarks_*, Art/Textures/BulletMarks/make_bullet_marks.py STYLES, which
	 * M_BulletMark reads by the decal's colour). Below 0: no mark (water).
	 */
	int32 Style = INDEX_NONE;
	/** The mark's radius in cm for a rifle round. */
	float Radius = 6.f;
};

/** Where one mark's decal goes and how big it is. */
struct FBulletMarkPlacement
{
	FVector Location = FVector::ZeroVector;
	/** X into the surface (the decal projects along it), Z along the shot's way across the surface. */
	FRotator Rotation = FRotator::ZeroRotator;
	/** The decal's half extents: X its depth, Y across the shot, Z along it (a glancing shot's gouge is longer). */
	FVector Size = FVector::OneVector;
	/** 0 for a shot square into the surface, 1 for one that only grazed it. */
	float Grazing = 0.f;
};

/**
 * Bullet marks: a small deferred decal where a bullet struck the world (terrain, buildings, props; never creatures, the
 * player, water or anything that moves), in the look of what it struck (a scorched hole and pale splinters in wood, a
 * chip of fresh stone, a bright dent in metal, a dark puff of thrown soil in dirt, a star of cracks in glass), sized to
 * the gun, that holds for a while and then fades slowly away (Looter.BulletMarks.Hold and .Fade: 20 s and 15 s).
 *
 * Cheap on Medium: the decals go into the DBuffer (the wanted posters' path), so they draw on the Nanite fallback
 * without Lumen, every one is a few dozen pixels, and they fade out with distance (FadeScreenSize). They come from one
 * pool of decal components on a transient actor, at most Looter.BulletMarks.Max (64) at once: a fresh mark takes an
 * expired one, else the oldest; under a long burst the oldest still holding fade out quickly (in 1.2 s) as the pool
 * fills, so marks make room instead of popping. The fade runs on the GPU from each decal's start (M_BulletMark reads
 * DecalLifetimeOpacity); a single timer hides the expired ones, so nothing ticks and an idle pool costs nothing.
 * UBulletSubsystem owns it and caps a shotgun's pellets at MarksPerVolley marks a blast.
 */
class AI_LOOTER_SHOOTER_API FBulletMarks
{
public:
	/** The decal material (Tools/Unreal/build_world_materials.py M_BulletMark); without it there are no marks. */
	static const TCHAR* MaterialPath;

	/** The most marks one blast leaves (a shotgun's pellets each make one, up to this). */
	static constexpr int32 MarksPerVolley = 5;

	/**
	 * Leaves a mark for a bullet's hit, when it may have one (CanMark). SizeScale sizes it (GunScale()); Pick (0..1) picks its
	 * turn and variant. False when it left none.
	 */
	bool Add(UWorld& World, const FHitResult& Hit, const FVector& ShotDirection, EImpactMaterial Material, float SizeScale, float Pick);

	/** Hides every mark at once (Looter.BulletMarks.Clear). */
	void Clear();

	/** Lets go of the pool (its actor and decals go). */
	void Shutdown();

	/** Marks showing now (holding or fading). */
	int32 NumLive(double Now) const;

	/** Decal components in the pool. */
	int32 NumDecals() const { return Marks.Num(); }

	// --- The rules, apart from any world (the tests read them) ---

	/** Whether the hit may take a mark: a component that never moves (static or stationary) and takes decals, not a pawn's, not water. */
	static bool CanMark(const FHitResult& Hit, EImpactMaterial Material);

	/** The mark a surface takes. */
	static FBulletMarkLook LookOf(EImpactMaterial Material);

	/** How big a gun's marks are against a rifle's: a revolver's heavy slug bigger, a shotgun's pellets smaller. */
	static float GunScale(EWeaponKind Kind);

	/**
	 * Where a mark goes: on the surface at Point, facing out along Normal, turned by Turn (0..1 of a full turn; a glancing
	 * shot keeps its gouge along its way instead), its gouge drawn out along a glancing shot and shifted down range.
	 */
	static FBulletMarkPlacement Place(const FVector& Point, const FVector& Normal, const FVector& ShotDirection, float Radius, float Turn);

	/** The decal's colour, which tells M_BulletMark the style and the variant (R: (style + 0.5) / 16, G: the seed). */
	static FLinearColor Code(int32 Style, float Seed);

	/**
	 * The pool slot a new mark takes, from each mark's end and start: the first that has ended, else a new one while
	 * fewer than Max exist (returns Ends.Num()), else the oldest.
	 */
	static int32 PickSlot(TConstArrayView<double> Ends, TConstArrayView<double> Born, double Now, int32 Max);

	/**
	 * Which mark to fade out early once more than SoftCap are showing: the oldest still holding (FadeAt not reached, not
	 * hurried already), or INDEX_NONE.
	 */
	static int32 PickHurry(TConstArrayView<double> Born, TConstArrayView<double> FadeAt, TConstArrayView<double> Ends,
		TConstArrayView<bool> Hurried, double Now, int32 SoftCap);

private:
	struct FMark
	{
		TWeakObjectPtr<UDecalComponent> Decal;
		double Born = -1.0;
		double FadeAt = 0.0;
		double Ends = 0.0;
		bool bHurried = false;
		bool bShown = false;
	};

	bool Initialize(UWorld& World);
	UDecalComponent* MakeDecal();
	void ScheduleExpiry(UWorld& World);
	void HideExpired();

	TArray<FMark> Marks;
	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<UMaterialInterface> DecalMaterial;
	FTimerHandle ExpiryTimer;
	/** The material was looked for and isn't there (said once). */
	bool bMaterialMissing = false;
};
