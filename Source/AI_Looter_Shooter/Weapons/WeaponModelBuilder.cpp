#include "Weapons/WeaponModelBuilder.h"
#include "Environment/StylizedMeshKit.h"
#include "Environment/StylizedSurface.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "UDynamicMesh.h"

using namespace StylizedMesh;
using StylizedColors::Hex;

namespace
{
	// Material slots.
	namespace GunSlot
	{
		constexpr int32 Body = 0;    // dark metal
		constexpr int32 Paint = 1;   // colored polymer panels
		constexpr int32 Accent = 2;  // glowing rarity strips
		constexpr int32 Grip = 3;    // rubber / wood
	}

	FLinearColor PickPaint(FRandomStream& Random)
	{
		static const uint32 Paints[] = { 0xe4dccb, 0xd9a766, 0x4f9e98, 0xe0795c, 0x8fb7d9, 0xc9d36a };
		return Hex(Paints[Random.RandRange(0, UE_ARRAY_COUNT(Paints) - 1)]);
	}

	void BuildRifle(UDynamicMesh* Mesh, UDynamicMesh* Part, FRandomStream& Random, EWeaponRarity Rarity, WeaponModels::FPoints& OutPoints)
	{
		// The magazine sits tilted in its well; a reload slides it out along its own long axis.
		const FRotator MagazineTilt(10.f, 0.f, 0.f);

		const float BarrelLength = Random.FRandRange(10.f, 18.f);

		Box(Mesh, GunSlot::Body, FVector(14.f, 0.f, 0.f), FVector(30.f, 6.f, 9.f));                 // receiver
		Box(Mesh, GunSlot::Paint, FVector(14.f, 0.f, 5.4f), FVector(28.f, 5.4f, 2.6f));             // top cover
		Box(Mesh, GunSlot::Paint, FVector(38.f, 0.f, -0.5f), FVector(20.f, 6.6f, 8.f));             // handguard
		for (const float Side : { -3.4f, 3.4f })
		{
			Box(Mesh, GunSlot::Accent, FVector(38.f, Side, 0.5f), FVector(14.f, 0.6f, 1.4f));       // glow vents
			Box(Mesh, GunSlot::Accent, FVector(12.f, Side, -2.f), FVector(8.f, 0.6f, 1.f));
		}
		Cylinder(Mesh, GunSlot::Body, FTransform(AlongX(), FVector(48.f, 0.f, 0.5f)), 1.4f, BarrelLength, 8);
		Cylinder(Mesh, GunSlot::Body, FTransform(AlongX(), FVector(48.f + BarrelLength, 0.f, 0.5f)), 2.3f, 5.f, 6);
		Box(Part, GunSlot::Body, FVector(20.f, 0.f, -9.f), FVector(5.f, 4.f, 12.f), MagazineTilt);    // magazine
		Box(Mesh, GunSlot::Grip, FVector(5.f, 0.f, -8.f), FVector(4.2f, 3.8f, 10.f), FRotator(-15.f, 0.f, 0.f)); // pistol grip
		Box(Mesh, GunSlot::Body, FVector(11.f, 0.f, -5.6f), FVector(6.f, 1.2f, 1.f));               // trigger guard
		Box(Mesh, GunSlot::Paint, FVector(-9.f, 0.f, -1.f), FVector(18.f, 4.6f, 7.f));              // stock
		Box(Mesh, GunSlot::Grip, FVector(-18.5f, 0.f, -1.5f), FVector(2.f, 5.f, 9.f));              // butt pad

		switch (Random.RandRange(0, 2))
		{
		case 0: // iron sights
			Box(Mesh, GunSlot::Body, FVector(40.f, 0.f, 4.8f), FVector(1.5f, 1.2f, 3.f));
			Box(Mesh, GunSlot::Body, FVector(4.f, 0.f, 7.5f), FVector(2.f, 3.f, 2.f));
			break;
		case 1: // red dot
			Box(Mesh, GunSlot::Body, FVector(16.f, 0.f, 8.6f), FVector(6.f, 3.2f, 3.6f));
			Box(Mesh, GunSlot::Accent, FVector(12.9f, 0.f, 9.f), FVector(0.4f, 2.2f, 2.2f));
			break;
		default: // scope
			Box(Mesh, GunSlot::Body, FVector(10.f, 0.f, 7.8f), FVector(3.f, 2.f, 2.f));
			Box(Mesh, GunSlot::Body, FVector(20.f, 0.f, 7.8f), FVector(3.f, 2.f, 2.f));
			Cylinder(Mesh, GunSlot::Body, FTransform(AlongX(), FVector(6.f, 0.f, 10.f)), 2.3f, 18.f, 8);
			Cylinder(Mesh, GunSlot::Accent, FTransform(AlongX(), FVector(23.9f, 0.f, 10.f)), 1.9f, 0.3f, 8);
			break;
		}

		if (Rarity >= EWeaponRarity::Legendary)
		{
			// Legendaries get a pair of glowing fins along the handguard.
			for (const float Side : { -2.2f, 2.2f })
			{
				Box(Mesh, GunSlot::Accent, FVector(39.f, Side, 4.6f), FVector(16.f, 0.8f, 2.2f));
			}
		}

		OutPoints.Muzzle = FVector(53.f + BarrelLength, 0.f, 0.5f);
		OutPoints.Grip = FVector(5.f, 0.f, -5.f);
		OutPoints.Foregrip = FVector(38.f, 0.f, -4.f);
		OutPoints.ReloadPart = EWeaponReloadPart::Magazine;
		OutPoints.ReloadPartAxis = MagazineTilt.RotateVector(FVector(0.f, 0.f, -1.f));
	}

	void BuildShotgun(UDynamicMesh* Mesh, UDynamicMesh* Part, FRandomStream& Random, EWeaponRarity Rarity, WeaponModels::FPoints& OutPoints)
	{
		const float BarrelLength = Random.FRandRange(30.f, 38.f);

		Box(Mesh, GunSlot::Body, FVector(10.f, 0.f, 0.f), FVector(22.f, 6.6f, 10.f));               // receiver
		for (const float Side : { -3.4f, 3.4f })
		{
			Box(Mesh, GunSlot::Paint, FVector(10.f, Side, -1.5f), FVector(18.f, 0.5f, 5.f));        // side plates
			Box(Mesh, GunSlot::Accent, FVector(10.f, Side * 1.05f, 2.3f), FVector(14.f, 0.5f, 1.3f)); // glow strips
		}
		Cylinder(Mesh, GunSlot::Body, FTransform(AlongX(), FVector(21.f, 0.f, 2.f)), 2.2f, BarrelLength, 8);
		Cylinder(Mesh, GunSlot::Body, FTransform(AlongX(), FVector(21.f, 0.f, -2.6f)), 1.8f, BarrelLength - 6.f, 8);
		Box(Part, GunSlot::Grip, FVector(21.f + BarrelLength * 0.45f, 0.f, -2.4f), FVector(14.f, 6.2f, 5.6f));  // pump
		Box(Mesh, GunSlot::Paint, FVector(21.f + BarrelLength * 0.5f, 0.f, 3.8f), FVector(BarrelLength, 1.1f, 0.9f)); // vent rib
		Box(Mesh, GunSlot::Accent, FVector(20.f + BarrelLength, 0.f, 4.6f), FVector(1.f, 1.f, 1.f));  // bead
		Box(Mesh, GunSlot::Grip, FVector(1.f, 0.f, -8.f), FVector(4.4f, 3.8f, 10.f), FRotator(-18.f, 0.f, 0.f)); // grip
		Box(Mesh, GunSlot::Body, FVector(7.f, 0.f, -5.8f), FVector(6.f, 1.2f, 1.f));                // trigger guard
		Box(Mesh, GunSlot::Grip, FVector(-11.f, 0.f, -2.5f), FVector(20.f, 5.f, 7.6f), FRotator(-5.f, 0.f, 0.f)); // stock
		Box(Mesh, GunSlot::Body, FVector(-21.5f, 0.f, -3.5f), FVector(2.f, 5.4f, 9.f));             // butt pad

		if (Rarity >= EWeaponRarity::Epic)
		{
			// Heat shroud with glowing slots on the higher tiers.
			Box(Mesh, GunSlot::Paint, FVector(21.f + BarrelLength * 0.8f, 0.f, 2.f), FVector(10.f, 5.6f, 5.6f));
			for (const float Side : { -2.9f, 2.9f })
			{
				Box(Mesh, GunSlot::Accent, FVector(21.f + BarrelLength * 0.8f, Side, 2.f), FVector(7.f, 0.4f, 1.2f));
			}
		}

		OutPoints.Muzzle = FVector(21.f + BarrelLength, 0.f, 2.f);
		OutPoints.Grip = FVector(1.f, 0.f, -5.f);
		OutPoints.Foregrip = FVector(21.f + BarrelLength * 0.45f, 0.f, -4.f);
		OutPoints.ReloadPart = EWeaponReloadPart::Pump;
		OutPoints.ReloadPartAxis = FVector(-1.f, 0.f, 0.f);
	}
}

void WeaponModels::Build(EWeaponModel Model, int32 Seed, EWeaponRarity Rarity, const FLinearColor& RarityColor, UDynamicMesh* Mesh,
	UDynamicMesh* PartMesh, TArray<FStylizedSurface>& OutSurfaces, FPoints& OutPoints)
{
	FRandomStream Random(Seed);
	const FLinearColor PanelPaint = PickPaint(Random);
	const bool bWood = Model == EWeaponModel::Shotgun && Random.FRand() < 0.6f;

	// Without a part mesh the moving part is simply built into the gun.
	UDynamicMesh* Part = PartMesh ? PartMesh : Mesh;
	switch (Model)
	{
	case EWeaponModel::Rifle:   BuildRifle(Mesh, Part, Random, Rarity, OutPoints); break;
	case EWeaponModel::Shotgun: BuildShotgun(Mesh, Part, Random, Rarity, OutPoints); break;
	default: OutPoints = FPoints(); break;
	}

	// Soften the hard boxes a touch so it reads hand-made rather than CAD. The noise is sampled by position, so the
	// part and the gun get the same field and still fit together.
	const float NoiseSeed = static_cast<float>(FMath::Abs(Seed) % 97);
	for (UDynamicMesh* Target : { Mesh, PartMesh })
	{
		if (Target)
		{
			Displace(Target, 0, 0.25f, 1.f / 6.f, NoiseSeed);
			FinishNormals(Target, 0.f);
		}
	}

	OutSurfaces.Reset();
	OutSurfaces.Add(FStylizedSurface::Solid(Hex(0x2e343d), 0.05f));
	OutSurfaces.Add(FStylizedSurface::Solid(PanelPaint, 0.06f));
	FStylizedSurface Glow = FStylizedSurface::Solid(RarityColor, 0.f);
	Glow.Glow = Rarity == EWeaponRarity::Common ? 0.6f : 2.5f;
	OutSurfaces.Add(Glow);
	OutSurfaces.Add(FStylizedSurface::Solid(bWood ? Hex(0x7a4f32) : Hex(0x23272e), 0.06f));
}

bool WeaponModels::BuildInto(const FWeaponInstanceData& Instance, UDynamicMeshComponent* Mesh, UDynamicMeshComponent* PartMesh, FPoints& OutPoints)
{
	const UWeaponDefinition* Definition = Instance.Definition;
	if (!Definition || Definition->ProceduralModel == EWeaponModel::None || !Mesh || !PartMesh)
	{
		return false;
	}

	UDynamicMesh* Scratch = NewObject<UDynamicMesh>(Mesh, NAME_None, RF_Transient);
	UDynamicMesh* PartScratch = NewObject<UDynamicMesh>(Mesh, NAME_None, RF_Transient);
	TArray<FStylizedSurface> Surfaces;
	Build(Definition->ProceduralModel, Instance.Seed, Instance.Rarity, UWeaponRollLibrary::GetRarityColor(Definition, Instance.Rarity),
		Scratch, PartScratch, Surfaces, OutPoints);

	auto CopyMesh = [](UDynamicMesh* From, UDynamicMeshComponent* To)
	{
		UE::Geometry::FDynamicMesh3 Built;
		From->ProcessMesh([&Built](const UE::Geometry::FDynamicMesh3& Source) { Built = Source; });
		To->SetMesh(MoveTemp(Built));
	};
	CopyMesh(Scratch, Mesh);
	CopyMesh(PartScratch, PartMesh);

	// One set of materials for the gun and its moving part.
	const TArray<UMaterialInterface*> Materials = StylizedSurfaces::CreateMaterials(Mesh, Surfaces);
	Mesh->ConfigureMaterialSet(Materials);
	PartMesh->ConfigureMaterialSet(Materials);
	return true;
}
