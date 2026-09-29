#include "Environment/EnvironmentPalette.h"
#include "Environment/StylizedSurface.h"

namespace
{
	FEnvironmentPaletteEntry MakeProcedural(FName Id, const TCHAR* Name, EStylizedPropShape Shape, const FLinearColor& Primary,
		const FLinearColor& Secondary)
	{
		FEnvironmentPaletteEntry Entry;
		Entry.Id = Id;
		Entry.DisplayName = FText::FromString(Name);
		Entry.Shape = Shape;
		Entry.PrimaryColor = Primary;
		Entry.SecondaryColor = Secondary;
		return Entry;
	}

	FEnvironmentPaletteEntry MakeActor(FName Id, const TCHAR* Name, const TCHAR* ClassPath)
	{
		FEnvironmentPaletteEntry Entry;
		Entry.Id = Id;
		Entry.DisplayName = FText::FromString(Name);
		Entry.ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath));
		return Entry;
	}
}

UEnvironmentPalette::UEnvironmentPalette()
{
	// "Skyreach" art direction: sunny floating-island frontier. Soft sky blues in the distance, warm meadow
	// greens and poppy reds up close, pale weathered stone, and a few glowing sci-fantasy accents.
	using StylizedColors::Hex;
	using Shape = EStylizedPropShape;

	const FLinearColor Grass = Hex(0x5b8a3a);
	const FLinearColor Meadow = Hex(0x6fa044);
	const FLinearColor Cliff = Hex(0xa89886);
	const FLinearColor Stone = Hex(0xb3a795);
	const FLinearColor PaleStone = Hex(0xb5a891);
	const FLinearColor Moss = Hex(0x86a24c);
	const FLinearColor Bark = Hex(0x6b4a35);
	const FLinearColor Glow = Hex(0x5ce1f0);
	const FLinearColor Sun = Hex(0xffc866);

	// Terrain
	Entries.Add(MakeProcedural(TEXT("IslandTerrain"), TEXT("Sky Island (200m)"), Shape::IslandTerrain, Cliff, Grass));
	Entries.Add(MakeProcedural(TEXT("MeadowTerrain"), TEXT("Meadow Ground (200m)"), Shape::Terrain, Hex(0x8a6a4a), Grass));
	Entries.Add(MakeProcedural(TEXT("Hill"), TEXT("Grassy Knoll"), Shape::Hill, Hex(0x8a6a4a), Grass));
	Entries.Add(MakeProcedural(TEXT("SandDune"), TEXT("Sand Dune"), Shape::Hill, Hex(0xd99a5b), Hex(0xe8b377)));
	Entries.Add(MakeProcedural(TEXT("Cliff"), TEXT("Cliff Outcrop"), Shape::Cliff, Cliff, Grass));

	// Rocks
	Entries.Add(MakeProcedural(TEXT("Rock"), TEXT("Rock"), Shape::Rock, Stone, Moss));
	Entries.Add(MakeProcedural(TEXT("Boulder"), TEXT("Boulder Pile"), Shape::Boulder, Hex(0xa2968a), Moss));
	Entries.Add(MakeProcedural(TEXT("RockPillar"), TEXT("Stone Pillars"), Shape::RockPillar, Hex(0x9d93a0), Moss));
	Entries.Add(MakeProcedural(TEXT("Crystal"), TEXT("Glow Crystals"), Shape::Crystal, Hex(0x8f8a9a), Glow));
	Entries.Add(MakeProcedural(TEXT("SteppingStones"), TEXT("Stepping Stones"), Shape::SteppingStones, PaleStone, Moss));

	// Plants
	Entries.Add(MakeProcedural(TEXT("GrassPatch"), TEXT("Grass Tufts"), Shape::GrassPatch, Meadow, Meadow));
	Entries.Add(MakeProcedural(TEXT("TallGrass"), TEXT("Tall Grass"), Shape::TallGrass, Hex(0x5cae3a), Hex(0xff8fc8)));
	Entries.Add(MakeProcedural(TEXT("WildGrass"), TEXT("Golden Wild Grass"), Shape::TallGrass, Hex(0xb9b25a), Hex(0xfff4d6)));
	Entries.Add(MakeProcedural(TEXT("PoppyField"), TEXT("Poppy Field"), Shape::FlowerPatch, Meadow, Hex(0xff5a2e)));
	Entries.Add(MakeProcedural(TEXT("Marigolds"), TEXT("Marigolds"), Shape::FlowerPatch, Meadow, Hex(0xffb42e)));
	Entries.Add(MakeProcedural(TEXT("CanopyTree"), TEXT("Round Tree"), Shape::CanopyTree, Bark, Hex(0x7fae44)));
	Entries.Add(MakeProcedural(TEXT("BlossomTree"), TEXT("Coral Blossom"), Shape::BlossomTree, Hex(0x5a3d33), Hex(0xff8a5c)));
	Entries.Add(MakeProcedural(TEXT("PineTree"), TEXT("Teal Pine"), Shape::PineTree, Bark, Hex(0x3d7a5e)));
	Entries.Add(MakeProcedural(TEXT("Bush"), TEXT("Bush"), Shape::Bush, Hex(0x5f9a45), Hex(0xfff2d6)));

	// Props
	Entries.Add(MakeProcedural(TEXT("Crate"), TEXT("Supply Crate"), Shape::Crate, Hex(0x3f7a5a), Hex(0xe0b04a)));
	Entries.Add(MakeProcedural(TEXT("Barrel"), TEXT("Teal Barrel"), Shape::Barrel, Hex(0x2f9a9a), Hex(0x3a3f47)));
	Entries.Add(MakeProcedural(TEXT("StoneWall"), TEXT("Ruined Wall"), Shape::StoneWall, PaleStone, Moss));
	Entries.Add(MakeProcedural(TEXT("RuinPillar"), TEXT("Ruined Column"), Shape::RuinPillar, PaleStone, Moss));
	Entries.Add(MakeActor(TEXT("TargetDummy"), TEXT("Target Dummy"), TEXT("/Game/Combat/Blueprints/BP_TargetDummy.BP_TargetDummy_C")));

	// Creatures wander around the spot they were placed and respawn there.
	Entries.Add(MakeActor(TEXT("BrownSpider"), TEXT("Brown Spider"), TEXT("/Script/AI_Looter_Shooter.SpiderCreature")));

	// Sky
	Entries.Add(MakeProcedural(TEXT("FloatingIsland"), TEXT("Floating Island"), Shape::FloatingIsland, Cliff, Grass));
	Entries.Add(MakeProcedural(TEXT("FloatingDebris"), TEXT("Floating Rocks"), Shape::FloatingDebris, Cliff, Grass));
	Entries.Add(MakeProcedural(TEXT("Cloud"), TEXT("Cloud Bank"), Shape::Cloud, Hex(0xffffff), Hex(0xffffff)));
	Entries.Add(MakeProcedural(TEXT("Beacon"), TEXT("Sky Beacon"), Shape::Beacon, Stone, Sun));
}

const FEnvironmentPaletteEntry* UEnvironmentPalette::FindEntry(FName Id) const
{
	return Entries.FindByPredicate([Id](const FEnvironmentPaletteEntry& Entry) { return Entry.Id == Id; });
}
