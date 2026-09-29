#include "Environment/EnvironmentPalette.h"
#include "Environment/StylizedSurface.h"

namespace
{
	FEnvironmentPaletteEntry MakeProcedural(FName Id, const TCHAR* Name, FName Category, EStylizedPropShape Shape,
		const FLinearColor& Primary, const FLinearColor& Secondary, float MinScale, float MaxScale, float Footprint, bool bAlign = false)
	{
		FEnvironmentPaletteEntry Entry;
		Entry.Id = Id;
		Entry.DisplayName = FText::FromString(Name);
		Entry.Category = Category;
		Entry.Shape = Shape;
		Entry.PrimaryColor = Primary;
		Entry.SecondaryColor = Secondary;
		Entry.MinScale = MinScale;
		Entry.MaxScale = MaxScale;
		Entry.Footprint = Footprint;
		Entry.bAlignToSurface = bAlign;
		return Entry;
	}

	FEnvironmentPaletteEntry InAir(FEnvironmentPaletteEntry Entry, float Distance)
	{
		Entry.bPlaceInAir = true;
		Entry.AirDistance = Distance;
		return Entry;
	}
}

UEnvironmentPalette::UEnvironmentPalette()
{
	// "Skyreach" art direction: sunny floating-island frontier. Soft sky blues in the distance, warm meadow
	// greens and poppy reds up close, pale weathered stone, and a few glowing sci-fantasy accents.
	using StylizedColors::Hex;
	using Shape = EStylizedPropShape;

	const FName Terrain(TEXT("Terrain"));
	const FName Rocks(TEXT("Rocks"));
	const FName Plants(TEXT("Plants"));
	const FName Props(TEXT("Props"));
	const FName Sky(TEXT("Sky"));

	const FLinearColor Grass = Hex(0x5b8a3a);
	const FLinearColor Meadow = Hex(0x6fa044);
	const FLinearColor Cliff = Hex(0xa89886);
	const FLinearColor Stone = Hex(0xb3a795);
	const FLinearColor PaleStone = Hex(0xb5a891);
	const FLinearColor Moss = Hex(0x86a24c);
	const FLinearColor Bark = Hex(0x6b4a35);
	const FLinearColor Glow = Hex(0x5ce1f0);
	const FLinearColor Sun = Hex(0xffc866);

	Entries.Add(MakeProcedural(TEXT("IslandTerrain"), TEXT("Sky Island (200m)"), Terrain, Shape::IslandTerrain, Cliff, Grass, 1.f, 1.f, 20000.f));
	Entries.Add(MakeProcedural(TEXT("MeadowTerrain"), TEXT("Meadow Ground (200m)"), Terrain, Shape::Terrain, Hex(0x8a6a4a), Grass, 1.f, 1.f, 20000.f));
	Entries.Add(MakeProcedural(TEXT("Hill"), TEXT("Grassy Knoll"), Terrain, Shape::Hill, Hex(0x8a6a4a), Grass, 0.7f, 1.4f, 3000.f));
	Entries.Add(MakeProcedural(TEXT("SandDune"), TEXT("Sand Dune"), Terrain, Shape::Hill, Hex(0xd99a5b), Hex(0xe8b377), 0.6f, 1.2f, 3000.f));
	Entries.Add(MakeProcedural(TEXT("Cliff"), TEXT("Cliff Outcrop"), Terrain, Shape::Cliff, Cliff, Grass, 0.7f, 1.3f, 1200.f));

	Entries.Add(MakeProcedural(TEXT("Rock"), TEXT("Rock"), Rocks, Shape::Rock, Stone, Moss, 0.6f, 1.6f, 200.f, true));
	Entries.Add(MakeProcedural(TEXT("Boulder"), TEXT("Boulder Pile"), Rocks, Shape::Boulder, Hex(0xa2968a), Moss, 0.7f, 1.3f, 700.f, true));
	Entries.Add(MakeProcedural(TEXT("RockPillar"), TEXT("Stone Pillars"), Rocks, Shape::RockPillar, Hex(0x9d93a0), Moss, 0.7f, 1.3f, 500.f));
	Entries.Add(MakeProcedural(TEXT("Crystal"), TEXT("Glow Crystals"), Rocks, Shape::Crystal, Hex(0x8f8a9a), Glow, 0.7f, 1.4f, 300.f));
	Entries.Add(MakeProcedural(TEXT("SteppingStones"), TEXT("Stepping Stones"), Rocks, Shape::SteppingStones, PaleStone, Moss, 0.9f, 1.2f, 600.f, true));

	Entries.Add(MakeProcedural(TEXT("GrassPatch"), TEXT("Grass Tufts"), Plants, Shape::GrassPatch, Meadow, Meadow, 0.8f, 1.3f, 500.f, true));
	Entries.Add(MakeProcedural(TEXT("TallGrass"), TEXT("Tall Grass"), Plants, Shape::TallGrass, Hex(0x5cae3a), Hex(0xff8fc8), 0.85f, 1.3f, 520.f, true));
	Entries.Add(MakeProcedural(TEXT("WildGrass"), TEXT("Golden Wild Grass"), Plants, Shape::TallGrass, Hex(0xb9b25a), Hex(0xfff4d6), 0.85f, 1.3f, 520.f, true));
	Entries.Add(MakeProcedural(TEXT("PoppyField"), TEXT("Poppy Field"), Plants, Shape::FlowerPatch, Meadow, Hex(0xff5a2e), 0.8f, 1.3f, 600.f, true));
	Entries.Add(MakeProcedural(TEXT("Marigolds"), TEXT("Marigolds"), Plants, Shape::FlowerPatch, Meadow, Hex(0xffb42e), 0.8f, 1.3f, 600.f, true));
	Entries.Add(MakeProcedural(TEXT("CanopyTree"), TEXT("Round Tree"), Plants, Shape::CanopyTree, Bark, Hex(0x7fae44), 0.8f, 1.3f, 700.f));
	Entries.Add(MakeProcedural(TEXT("BlossomTree"), TEXT("Coral Blossom"), Plants, Shape::BlossomTree, Hex(0x5a3d33), Hex(0xff8a5c), 0.8f, 1.3f, 800.f));
	Entries.Add(MakeProcedural(TEXT("PineTree"), TEXT("Teal Pine"), Plants, Shape::PineTree, Bark, Hex(0x3d7a5e), 0.8f, 1.35f, 500.f));
	Entries.Add(MakeProcedural(TEXT("Bush"), TEXT("Bush"), Plants, Shape::Bush, Hex(0x5f9a45), Hex(0xfff2d6), 0.7f, 1.4f, 250.f));

	Entries.Add(MakeProcedural(TEXT("Crate"), TEXT("Supply Crate"), Props, Shape::Crate, Hex(0x3f7a5a), Hex(0xe0b04a), 0.9f, 1.1f, 180.f));
	Entries.Add(MakeProcedural(TEXT("Barrel"), TEXT("Teal Barrel"), Props, Shape::Barrel, Hex(0x2f9a9a), Hex(0x3a3f47), 1.f, 1.f, 120.f));
	Entries.Add(MakeProcedural(TEXT("StoneWall"), TEXT("Ruined Wall"), Props, Shape::StoneWall, PaleStone, Moss, 0.9f, 1.2f, 700.f));
	Entries.Add(MakeProcedural(TEXT("RuinPillar"), TEXT("Ruined Column"), Props, Shape::RuinPillar, PaleStone, Moss, 0.9f, 1.3f, 350.f));

	FEnvironmentPaletteEntry Dummy;
	Dummy.Id = TEXT("TargetDummy");
	Dummy.DisplayName = FText::FromString(TEXT("Target Dummy"));
	Dummy.Category = Props;
	Dummy.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Combat/Blueprints/BP_TargetDummy.BP_TargetDummy_C")));
	Dummy.MinScale = 1.f;
	Dummy.MaxScale = 1.f;
	Dummy.Footprint = 150.f;
	Entries.Add(Dummy);

	// Creatures are placed like props; they wander around the spot they were placed and respawn there.
	const FName Creatures(TEXT("Creatures"));
	FEnvironmentPaletteEntry Spider;
	Spider.Id = TEXT("BrownSpider");
	Spider.DisplayName = FText::FromString(TEXT("Brown Spider"));
	Spider.Category = Creatures;
	Spider.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Script/AI_Looter_Shooter.SpiderCreature")));
	Spider.MinScale = 1.f;
	Spider.MaxScale = 1.f;
	Spider.Footprint = 600.f;
	Entries.Add(Spider);
	Entries.Add(InAir(MakeProcedural(TEXT("FloatingIsland"), TEXT("Floating Island"), Sky, Shape::FloatingIsland, Cliff, Grass, 0.6f, 2.5f, 4000.f), 12000.f));
	Entries.Add(InAir(MakeProcedural(TEXT("FloatingDebris"), TEXT("Floating Rocks"), Sky, Shape::FloatingDebris, Cliff, Grass, 0.7f, 1.8f, 3000.f), 6000.f));
	Entries.Add(InAir(MakeProcedural(TEXT("Cloud"), TEXT("Cloud Bank"), Sky, Shape::Cloud, Hex(0xffffff), Hex(0xffffff), 0.8f, 2.5f, 3000.f), 10000.f));
	Entries.Add(MakeProcedural(TEXT("Beacon"), TEXT("Sky Beacon"), Sky, Shape::Beacon, Stone, Sun, 1.f, 1.f, 800.f));
}

const FEnvironmentPaletteEntry* UEnvironmentPalette::FindEntry(FName Id) const
{
	return Entries.FindByPredicate([Id](const FEnvironmentPaletteEntry& Entry) { return Entry.Id == Id; });
}

TArray<FName> UEnvironmentPalette::GetCategories() const
{
	TArray<FName> Categories;
	for (const FEnvironmentPaletteEntry& Entry : Entries)
	{
		Categories.AddUnique(Entry.Category);
	}
	return Categories;
}
