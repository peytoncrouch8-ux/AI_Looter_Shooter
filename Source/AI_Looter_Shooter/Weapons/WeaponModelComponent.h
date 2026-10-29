#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Elements/LooterElement.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponModelComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
struct FHandMarks;

/**
 * How an element part looks on the gun master (M_Gun's element finish, custom primitive data
 * UWeaponModelComponent::ElementFinishDataIndex on): the same mesh recoloured, no models of its own (the user's call,
 * 2026-10-27). Kind picks the pattern (UWeaponModelComponent::FinishScorched / FinishGilt / FinishFrosted, 0 none): Ember
 * scorched (dark, soot blotches, glowing seams), Soul-light gilt (gold, metallic, a faint gold-white rim glow), Grave-salt
 * frosted (pale, salt-crystal speckle lit cyan); the story elements reuse the patterns in their own tints (2026-10-29):
 * Thunder "Charged" frosted in cold blue steel, its speckle lit electric blue; Rot "Rotten" scorched in mossy green, its
 * seams glowing dim green. Tint is the finish's colour (linear) the part's texture is pulled toward
 * by Amount (0-1), keeping its light and dark; Glow the emissive hue times GlowStrength.
 */
struct FWeaponElementFinish
{
	float Kind = 0.f;
	FLinearColor Tint = FLinearColor::White;
	float Amount = 0.f;
	FLinearColor Glow = FLinearColor::Black;
	float GlowStrength = 0.f;
};

/**
 * A gun's field paint on the gun master (M_Gun's pattern, custom primitive data UWeaponModelComponent::PatternKindDataIndex
 * on; Docs/Work/2026-10-27-GunPatterns/CONTRACT.md): Kind (UWeaponModelComponent::PatternGrime .. PatternBold, 0 none),
 * Scale (cm per repeat, 4-30), Seed (0-1, the material's offsets and turn), Tint (linear) and Amount (0-1). Rolled from the
 * gun's seed, rarity and maker (rarer is fancier); all zeros = the part as modeled.
 */
struct FWeaponPattern
{
	float Kind = 0.f;
	float Scale = 0.f;
	float Seed = 0.f;
	FLinearColor Tint = FLinearColor::Black;
	float Amount = 0.f;
};

struct FWeaponLook;

/**
 * A rolled gun's model: its definition's parts (static meshes made in Blender) as the roll's seed picks them, in the
 * gun's paint and its rarity's glow. This component is the gun's own space (origin at the back of the receiver, +X
 * toward the muzzle): the parts hang in it, or from sockets on earlier parts. It knows where the muzzle and the hands
 * go, and moves the part a reload works on.
 *
 * Its notches show on it (WeaponModelNotches.cpp): the gun master (M_Gun) cuts the tally into one part from that part's
 * custom primitive data, and lights a soul-forged gun's soul-light the same way, so no gun needs meshes or materials of
 * its own.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UWeaponModelComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Builds the gun, replacing the one it showed. False (and empty) when its definition has no parts. */
	bool Assemble(const FWeaponInstanceData& Instance);

	/** Removes the parts. */
	void Clear();

	bool IsAssembled() const { return !Parts.IsEmpty(); }

	/**
	 * How much bigger than modeled a gun kind is drawn (the user's call, 2026-10-14: the rifle and the shotgun 10%, the
	 * six-gun 20%), everywhere its model shows. The parts hang at this scale inside the component, so every point it gives
	 * in its space (grip, foregrip, aim, muzzle, ejection port) and a reload part's travel come out at it too; whoever holds
	 * the component never scales it.
	 */
	static float ScaleFor(EWeaponKind Kind);

	/** The scale this gun was built at (ScaleFor its kind; 1 before it's built). */
	float GetModelScale() const { return ModelScale; }

	/** Key points in the gun's space. */
	FVector GetMuzzle() const { return Muzzle; }
	FVector GetGrip() const { return Grip; }
	FVector GetForegrip() const { return Foregrip; }
	/** Where the eye lines up when aiming down the sights: just over the sight (the gun's top without one), along its middle. */
	FVector GetAimPoint() const { return AimPoint; }

	/** The middle of the gun's parts, in its space. */
	FVector GetCenter() const;

	/**
	 * The hands' grip spots its parts mark (SOCKET_<Hold|Rack|Reload>_<L|R>_<Pose>, SOCKET_Trigger: Weapons/WeaponHandMarks.h;
	 * WeaponModelHandMarks.cpp), read from the parts as they are now, in this component's space as GetGrip's point is (its own
	 * scale divided out, the parts' ScaleFor in it). The latest part in assembly order wins each role and side; a mark on the
	 * reload part is where that part rests. False (and empty) when no part marks any.
	 */
	bool GetHandMarks(FHandMarks& Out) const;

	/** The ejection port its spent cases leave from, in its space, when a part marks it (SOCKET_Eject); false if none does. */
	bool GetEject(FVector& OutLocation) const
	{
		OutLocation = Eject;
		return bHasEject;
	}

	/**
	 * A revolver's chambers where they open at the back of its cylinder, in the world as the cylinder is now (swung out,
	 * turned), and the way out of them (OutBack: the cylinder's back): where its empties tumble from. False without one.
	 */
	bool GetChamberRears(TArray<FVector>& OutLocations, FVector& OutBack) const;

	EWeaponReloadPart GetReloadPart() const { return ReloadPartMesh ? ReloadPart : EWeaponReloadPart::None; }

	/**
	 * Moves the reload's part this far out of place (cm, along its way out) and shows or hides it. A revolver's cylinder
	 * swings instead: Travel is how far it has swung out on its crane (degrees, to the gun's left).
	 */
	void SetReloadTravel(float Travel, bool bShow);

	/**
	 * A revolver's cylinder turns on by Chambers (the next round under the hammer), eased over CylinderTurnSeconds after a
	 * short wait (the recoil first); nothing for a gun without one. UpdateCylinder plays it out.
	 */
	void TurnCylinder(int32 Chambers);

	/** Advances the cylinder's turn. True while it's still turning (the gun keeps ticking for it). */
	bool UpdateCylinder(float DeltaSeconds);

	bool IsCylinderTurning() const { return CylinderTurnAge < CylinderTurnDelay + CylinderTurnSeconds; }

	/** The chambers in this gun's cylinder (its part's capacity), 0 without one. */
	int32 GetCylinderChambers() const { return ReloadPart == EWeaponReloadPart::Cylinder && ReloadPartMesh ? CylinderChambers : 0; }

	/** How far the cylinder has turned since the gun was built, in chambers (fractions mid-turn). */
	float GetCylinderTurn() const { return CylinderTurnShown; }

	/** The cylinder's turn after a shot: the wait (the hammer's fall and the kick) and the turn itself (seconds). */
	static constexpr float CylinderTurnDelay = 0.04f;
	static constexpr float CylinderTurnSeconds = 0.12f;

	// --- The frontier-tech guns' moving and glowing parts (Phase 12.3; WeaponModelKinds.cpp) ---

	/** The sniper's bolt (its Bolt slot's part): Open 0 home, 1 turned up and drawn fully back (LooterReload::BoltPose). */
	void SetBoltOpen(float Open);
	bool HasBolt() const { return BoltMesh != nullptr; }

	/**
	 * The launcher's rocket: the next one (the definition's ProjectileMesh) Travel cm in front of its seat in the mouth,
	 * shown while it's in the hand (bIncomingShown); the loaded rocket's nose (the magazine slot's part) shown while one is
	 * in (bLoaded).
	 */
	void SetRocketLoad(float Travel, bool bIncomingShown, bool bLoaded);

	/**
	 * The glows the game drives (material slots GunAmmoGlow, the SMG helix's rounds left; GunHeatGlow, the LMG barrel's
	 * heat): lit from the part's feed or breech end (its UV0.x 0) up to Fill (0-1), at Strength. On the gauge master
	 * (M_GunGauge, Tools/Unreal/build_gun_gauge_material.py) the lit length follows Fill; without it the whole slot
	 * glows at Strength times Fill's share, so it still dims and brightens.
	 */
	void SetAmmoGauge(float Fill, float Strength);
	void SetHeatGauge(float Fill, float Strength);
	bool HasAmmoGauge() const { return AmmoGauge != nullptr; }
	/** The round-count glow's color now (the element's hue on a gun with an element part); black without the gauge. */
	FLinearColor GetAmmoGaugeColor() const;
	bool HasHeatGauge() const { return HeatGauge != nullptr; }

	/** A launcher's exhaust mouth (SOCKET_Backblast) and the way it faces, in the gun's space; false without one. */
	bool GetBackblast(FVector& OutLocation, FVector& OutDirection) const
	{
		OutLocation = Backblast;
		OutDirection = BackblastWay;
		return bHasBackblast;
	}

	/** The material slots the game lights part of the way, and the gauge master that can (null until it's built). */
	static const FName AmmoGlowSlot;
	static const FName HeatGlowSlot;
	static UMaterialInterface* FindGaugeMaster();

	// --- The gun's element on its looks (WeaponModelElements.cpp; the user's call 2026-10-27, "Glow parts take element") ---

	/**
	 * Whether a material slot is one of the glows that take the gun's element's colour: every emissive accent of the gun
	 * kit (a "Gun...Glow" slot: the ember vents and barrel slots, the optics' and the irons' cyan dots, the ammo and heat
	 * gauges, the element pieces' own glows; the Tripo parts' glows are the same slots) and the optics' GunReticle while
	 * Looter.Weapons.ElementLook.Reticle is on; never the rarity accent (RarityGlowSlot, or a part's own copy of it).
	 */
	static bool IsElementGlowSlot(FName Slot, FName RarityGlowSlot);

	/** Whether the glows take the element at all (Looter.Weapons.ElementLook.Glows; read as a gun is built). */
	static bool ElementGlowsOn();

	/** The finish an element part wears (Kind 0 for None, or while Looter.Weapons.ElementLook.Finish is off). */
	static FWeaponElementFinish ElementFinishFor(ELooterElement Element);

	/**
	 * Writes Element's finish into a part's custom primitive data for the gun master (zeros = the part as modeled). The gun's
	 * other parts wear it too as light stains (the user's call, 2026-10-27): AmountScale (ElementSpread) scales its amount and
	 * its glow; the element part takes it whole (1).
	 */
	static void ShowElementFinish(UPrimitiveComponent& Part, ELooterElement Element, float AmountScale = 1.f);

	/** How much of the element part's finish the gun's other parts wear (Looter.Weapons.ElementLook.Spread, 0.25; 0-1). */
	static float ElementSpread();

	/** The part whose element the gun fires (its first part with one), wearing its finish; null without one. */
	UStaticMeshComponent* GetElementPart() const { return ElementPart; }

	/**
	 * The gun master's element finish in custom primitive data (after the maker's mark, 12-19): the kind (20), the tint and
	 * its amount (21-24, RGBA), the glow's hue and strength (25-28, RGBA). Kinds as FWeaponElementFinish::Kind.
	 */
	static constexpr int32 ElementFinishDataIndex = 20;
	static constexpr int32 ElementTintDataIndex = 21;
	static constexpr int32 ElementGlowDataIndex = 25;
	static constexpr float FinishScorched = 1.f;
	static constexpr float FinishGilt = 2.f;
	static constexpr float FinishFrosted = 3.f;

	// --- The gun's field paint (WeaponModelPattern.cpp; the user's call 2026-10-27: rarer is fancier, subtle and worn-in) ---

	/**
	 * The gun master's pattern in custom primitive data (after the element finish, 20-28): the kind (29), the scale (30), the
	 * seed (31), the tint (32-34, RGB linear), the amount (35).
	 */
	static constexpr int32 PatternKindDataIndex = 29;
	static constexpr int32 PatternScaleDataIndex = 30;
	static constexpr int32 PatternSeedDataIndex = 31;
	static constexpr int32 PatternTintDataIndex = 32;
	static constexpr int32 PatternAmountDataIndex = 35;
	static constexpr float PatternNone = 0.f;
	static constexpr float PatternGrime = 1.f;
	static constexpr float PatternWorn = 2.f;
	static constexpr float PatternStripes = 3.f;
	static constexpr float PatternChevrons = 4.f;
	static constexpr float PatternCamo = 5.f;
	static constexpr float PatternStencil = 6.f;
	static constexpr float PatternBold = 7.f;
	static constexpr int32 PatternKindCount = 8;

	/**
	 * The pattern a gun rolls (pure, no cvars): from its seed (a stream of its own, like WeaponParts::Wear, so no other roll
	 * moves), its rarity (Common grime or worn edges, Uncommon those or stripes, Rare stripes/chevrons/camo/stencil, Epic
	 * chevrons/camo/stencil/clean bold, Legendary clean bold or its maker's own design, MakerPatternKind) and its maker's
	 * palette (PatternPalette). ForceKind 0-7 keeps the roll but shows that kind (Looter.Weapons.Pattern.Force); -1 rolls it.
	 */
	static FWeaponPattern PickPattern(const FWeaponInstanceData& Instance, const FWeaponLook& Look, int32 ForceKind = -1);

	/** PickPattern under the cvars: zeros while Looter.Weapons.Pattern is 0, the kind Looter.Weapons.Pattern.Force names. */
	static FWeaponPattern PatternFor(const FWeaponInstanceData& Instance, const FWeaponLook& Look);

	/** A legendary's maker-specific bold design (Roundhouse chevrons, Saint Maud stripes, Windfall camo, Meridian stencil); else PatternBold. */
	static float MakerPatternKind(FName MakerKey);

	/**
	 * The tints a gun's pattern picks from (linear): a darker and a lighter shade of its shell's paint (the maker's), its trim
	 * (the maker's brass), black and bone.
	 */
	static TArray<FLinearColor> PatternPalette(const FWeaponLook& Look);

	/**
	 * Writes the pattern into a part's custom primitive data for the gun master. Each part's seed is moved on by its index, so
	 * the parts (separate meshes) don't repeat one another.
	 */
	static void ShowPattern(UPrimitiveComponent& Part, const FWeaponPattern& Pattern, int32 PartIndex = 0);

	/** A glow of the gun (its rarity accent, any Gun*Glow, the reticle): never takes a pattern or stains. */
	static bool IsGlowSlot(FName Slot, FName RarityGlowSlot);

	/** A part that is nothing but glow (every material slot IsGlowSlot): takes no pattern and no element stains. */
	static bool IsWhollyGlow(const UStaticMeshComponent& Part, FName RarityGlowSlot);

	/** Draws the parts like first-person arms (own field of view, no clipping) or like the world. */
	void SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType Type);
	EFirstPersonPrimitiveType GetFirstPersonPrimitiveType() const { return FirstPersonType; }

	/** Every part's mesh, for rendering settings. */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetParts() const { return Parts; }

	/**
	 * Shows the gun's notches: one tally mark per WeaponNotches::KillsPerMark kills cut into its stock (into the body's
	 * butt on a gun whose stock is only a pad, the bullpup), and a faint soul-light in its rarity's color once it's
	 * soul-forged. Assemble shows them; call again when its kills change.
	 */
	void ShowNotches(const FWeaponInstanceData& Instance);

	/** The part the tally is cut into (null when there's none to cut). */
	UStaticMeshComponent* GetNotchPart() const { return NotchPart; }

	/** Whether a part has a tally row laid out for it (WeaponModelNotches.cpp); any other gets one fitted to its size. */
	static bool HasTallyRow(const UStaticMesh* Mesh);

	/**
	 * The gun master's custom primitive data (WeaponParts::WearDataIndex, 0, is the wear): the marks cut (0 to 25); the
	 * row they run along, from (start X, start Z) to (end X, end Z) in the part's own space (cm); how tall a cut is and how
	 * far from the part's middle (|Y|, cm) a surface must be to take one, so only the outside of its sides is cut; the
	 * soul-light's color (RGB) and strength.
	 */
	static constexpr int32 NotchMarksDataIndex = 1;
	static constexpr int32 NotchRowDataIndex = 2;
	static constexpr int32 NotchHeightDataIndex = 6;
	static constexpr int32 NotchDepthDataIndex = 7;
	static constexpr int32 SoulLightDataIndex = 8;

	/**
	 * How bright a soul-forged gun's soul-light is (M_Gun's rim glow, times its rarity's color). Faint, as the spec says:
	 * at 1.6 the whole gun in hand glowed blue-white (2026-10-08).
	 */
	static constexpr float SoulLightGlow = 0.3f;

protected:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	/** A socket of any part, in the gun's space; false if no part has it. */
	bool FindSocket(FName Socket, FVector& OutLocation) const;
	void FindAimPoint();

	/** Picks the part the tally goes on and where on it: the stock's or the body's row (WeaponModelNotches.cpp). */
	void ChooseNotchPart(UStaticMeshComponent* Stock, UStaticMeshComponent* Body);

	/** Puts the cylinder where its swing and its turn have it: turned about its own axis, swung about its crane. */
	void PoseCylinder();

	/**
	 * A gauge slot's material for this gun (WeaponModelKinds.cpp): the gauge master's instance in Source's color, else Source's own.
	 * A gun with an element part fitted lights its round count in that element's hue, so the orange count does not drown a
	 * Soul-light or Grave-salt helix; the heat rings too while the glows take the element (ElementGlowsOn).
	 */
	UMaterialInstanceDynamic* MakeGauge(FName Slot, UMaterialInterface* Source, ELooterElement Element = ELooterElement::None);

	/** The launcher's next rocket for its reload, beside the loaded one (WeaponModelKinds.cpp; none without a projectile mesh). */
	void MakeRocketLoad(const UWeaponDefinition& Definition, UStaticMeshComponent& Loaded);

	/** The back-blast socket's place and facing, from whichever part has it (the launcher's body). */
	void FindBackblast();

	/** The slot (or the socket it hangs from) whose part is the sight. */
	const FName SightSocket = TEXT("Sight");
	/** On a sight: the point the eye lines up with when aiming (the dot, an optic's center, the irons' notch). */
	const FName AimSocket = TEXT("Aim");
	/** The slots whose parts can take the tally: the stock's (a revolver's grip, where gunfighters cut theirs), else the body's. */
	const FName StockSlot = TEXT("Stock");
	const FName GripSlot = TEXT("Grip");
	const FName BodySlot = TEXT("Body");
	/** On a revolver's cylinder: the crane's hinge it swings out on, in the cylinder's own space. */
	const FName CraneSocket = TEXT("Crane");
	/** The sniper's bolt slot. */
	const FName BoltSlot = TEXT("Bolt");

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	/** The sniper's bolt, and the launcher's next rocket (not one of Parts: it only shows through a reload). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BoltMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> RocketLoadMesh;

	/** This gun's instances of its gauge slots (null for a gun without them). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AmmoGauge;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeatGauge;

	FVector Backblast = FVector::ZeroVector;
	FVector BackblastWay = -FVector::ForwardVector;
	bool bHasBackblast = false;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ReloadPartMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SightPart;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> NotchPart;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ElementPart;

	/** The tally's row on NotchPart (start X, start Z, end X, end Z; cm), and its cuts' height and least depth. */
	FVector4f NotchRow = FVector4f(0.f, 0.f, 0.f, 0.f);
	FVector2f NotchCut = FVector2f(0.f, 0.f);

	/** This gun's colors of the parts' shared materials. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Paints;

	EWeaponReloadPart ReloadPart = EWeaponReloadPart::None;

	/** The scale the parts were hung at (ScaleFor the definition's kind). */
	float ModelScale = 1.f;

	/** A revolver's cylinder: its chambers, its crane's hinge (its own space), its swing (degrees) and its turn (chambers). */
	int32 CylinderChambers = 0;
	FVector CranePivot = FVector::ZeroVector;
	float CylinderSwing = 0.f;
	float CylinderTurnShown = 0.f;
	float CylinderTurnFrom = 0.f;
	float CylinderTurnTo = 0.f;
	/** Seconds since the last shot's turn began (past its wait and turn: still). */
	float CylinderTurnAge = 1000.f;

	FVector Muzzle = FVector::ZeroVector;
	FVector Grip = FVector::ZeroVector;
	FVector Foregrip = FVector::ZeroVector;
	FVector AimPoint = FVector::ZeroVector;
	FVector Eject = FVector::ZeroVector;
	bool bHasEject = false;
	EFirstPersonPrimitiveType FirstPersonType = EFirstPersonPrimitiveType::None;
};
