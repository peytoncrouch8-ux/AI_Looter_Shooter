#include "Weapons/ImpactSurface.h"

// ImpactSurface's words: the names and tags that say what a surface is made of. ImpactSurface.cpp finds a hit's surface.

namespace
{
	struct FSurfaceWord
	{
		const TCHAR* Word;
		EImpactMaterial Material;
	};

	/**
	 * The words that name a surface, matched as PascalCase words (the word as written, not followed by a lower-case
	 * letter: "Tin" in MI_MailboxTin, not in MI_Tinted), the first in this list found anywhere in the name winning. So
	 * the specific come first: glass before metal (a lantern's glass), sheet metal before metal, "Sandstone" and
	 * "Flagstone" before sand and cloth's "Flag". The world's materials are instances named for what they show
	 * (/Game/Art/Materials: MI_WoodPlanks, MI_RockCliff_Ransom, MI_MetalWorn_Windmill, MI_WindowGlow, MI_Hay...).
	 */
	const FSurfaceWord ImpactSurfaceWords[] = {
		{ TEXT("Water"), EImpactMaterial::Water },
		{ TEXT("Waterfall"), EImpactMaterial::Water },
		{ TEXT("Pond"), EImpactMaterial::Water },
		{ TEXT("Creek"), EImpactMaterial::Water },
		{ TEXT("River"), EImpactMaterial::Water },
		{ TEXT("Puddle"), EImpactMaterial::Water },
		{ TEXT("Glass"), EImpactMaterial::Glass },
		{ TEXT("Window"), EImpactMaterial::Glass },
		{ TEXT("Pane"), EImpactMaterial::Glass },
		{ TEXT("Lens"), EImpactMaterial::Glass },
		{ TEXT("Bottle"), EImpactMaterial::Glass },
		{ TEXT("Jar"), EImpactMaterial::Glass },
		{ TEXT("Bone"), EImpactMaterial::Bone },
		{ TEXT("Skull"), EImpactMaterial::Bone },
		{ TEXT("Antler"), EImpactMaterial::Bone },
		{ TEXT("Ivory"), EImpactMaterial::Bone },
		{ TEXT("Tin"), EImpactMaterial::SheetMetal },
		{ TEXT("Corrugated"), EImpactMaterial::SheetMetal },
		{ TEXT("Sheet"), EImpactMaterial::SheetMetal },
		{ TEXT("Drum"), EImpactMaterial::SheetMetal },
		{ TEXT("Mailbox"), EImpactMaterial::SheetMetal },
		{ TEXT("Bucket"), EImpactMaterial::SheetMetal },
		{ TEXT("Pail"), EImpactMaterial::SheetMetal },
		{ TEXT("Screen"), EImpactMaterial::SheetMetal },
		{ TEXT("Metal"), EImpactMaterial::Metal },
		{ TEXT("Iron"), EImpactMaterial::Metal },
		{ TEXT("Steel"), EImpactMaterial::Metal },
		{ TEXT("Brass"), EImpactMaterial::Metal },
		{ TEXT("Copper"), EImpactMaterial::Metal },
		{ TEXT("Bronze"), EImpactMaterial::Metal },
		{ TEXT("Nickel"), EImpactMaterial::Metal },
		{ TEXT("Silver"), EImpactMaterial::Metal },
		{ TEXT("Rust"), EImpactMaterial::Metal },
		{ TEXT("Chain"), EImpactMaterial::Metal },
		{ TEXT("Anvil"), EImpactMaterial::Metal },
		{ TEXT("Bell"), EImpactMaterial::Metal },
		{ TEXT("Gun"), EImpactMaterial::Metal },
		{ TEXT("Sandstone"), EImpactMaterial::Stone },
		{ TEXT("Flagstone"), EImpactMaterial::Stone },
		{ TEXT("Stone"), EImpactMaterial::Stone },
		{ TEXT("Rock"), EImpactMaterial::Stone },
		{ TEXT("Granite"), EImpactMaterial::Stone },
		{ TEXT("Cliff"), EImpactMaterial::Stone },
		{ TEXT("Boulder"), EImpactMaterial::Stone },
		{ TEXT("Brick"), EImpactMaterial::Stone },
		{ TEXT("Cobble"), EImpactMaterial::Stone },
		{ TEXT("Concrete"), EImpactMaterial::Stone },
		{ TEXT("Marble"), EImpactMaterial::Stone },
		{ TEXT("Slate"), EImpactMaterial::Stone },
		{ TEXT("Masonry"), EImpactMaterial::Stone },
		{ TEXT("Plaster"), EImpactMaterial::Stone },
		{ TEXT("Mortar"), EImpactMaterial::Stone },
		{ TEXT("Chimney"), EImpactMaterial::Stone },
		{ TEXT("Gravel"), EImpactMaterial::Gravel },
		{ TEXT("Ballast"), EImpactMaterial::Gravel },
		{ TEXT("Scree"), EImpactMaterial::Gravel },
		{ TEXT("Pebble"), EImpactMaterial::Gravel },
		{ TEXT("Pebbles"), EImpactMaterial::Gravel },
		{ TEXT("Talus"), EImpactMaterial::Gravel },
		{ TEXT("Rubble"), EImpactMaterial::Gravel },
		{ TEXT("Sand"), EImpactMaterial::Sand },
		{ TEXT("Dust"), EImpactMaterial::Sand },
		{ TEXT("Dune"), EImpactMaterial::Sand },
		{ TEXT("Hay"), EImpactMaterial::Hay },
		{ TEXT("Straw"), EImpactMaterial::Hay },
		{ TEXT("Thatch"), EImpactMaterial::Hay },
		{ TEXT("Bale"), EImpactMaterial::Hay },
		{ TEXT("Cloth"), EImpactMaterial::Cloth },
		{ TEXT("Canvas"), EImpactMaterial::Cloth },
		{ TEXT("Linen"), EImpactMaterial::Cloth },
		{ TEXT("Wool"), EImpactMaterial::Cloth },
		{ TEXT("Crepe"), EImpactMaterial::Cloth },
		{ TEXT("Rope"), EImpactMaterial::Cloth },
		{ TEXT("Hemp"), EImpactMaterial::Cloth },
		{ TEXT("Leather"), EImpactMaterial::Cloth },
		{ TEXT("Tarp"), EImpactMaterial::Cloth },
		{ TEXT("Sack"), EImpactMaterial::Cloth },
		{ TEXT("Burlap"), EImpactMaterial::Cloth },
		{ TEXT("Felt"), EImpactMaterial::Cloth },
		{ TEXT("Silk"), EImpactMaterial::Cloth },
		{ TEXT("Web"), EImpactMaterial::Cloth },
		{ TEXT("Quilt"), EImpactMaterial::Cloth },
		{ TEXT("Flag"), EImpactMaterial::Cloth },
		// Leaves before the trees' woods (MI_LeavesOak, MI_NeedlesPine are leaves, MI_BarkOak is wood).
		{ TEXT("Leaves"), EImpactMaterial::Grass },
		{ TEXT("Needles"), EImpactMaterial::Grass },
		{ TEXT("Foliage"), EImpactMaterial::Grass },
		{ TEXT("Wood"), EImpactMaterial::Wood },
		{ TEXT("Plank"), EImpactMaterial::Wood },
		{ TEXT("Board"), EImpactMaterial::Wood },
		{ TEXT("Bark"), EImpactMaterial::Wood },
		{ TEXT("Log"), EImpactMaterial::Wood },
		{ TEXT("Timber"), EImpactMaterial::Wood },
		{ TEXT("Beam"), EImpactMaterial::Wood },
		{ TEXT("Deck"), EImpactMaterial::Wood },
		{ TEXT("Porch"), EImpactMaterial::Wood },
		{ TEXT("Crate"), EImpactMaterial::Wood },
		{ TEXT("Barrel"), EImpactMaterial::Wood },
		{ TEXT("Coffin"), EImpactMaterial::Wood },
		{ TEXT("Fence"), EImpactMaterial::Wood },
		{ TEXT("Shake"), EImpactMaterial::Wood },
		{ TEXT("Shingle"), EImpactMaterial::Wood },
		{ TEXT("HouseTrim"), EImpactMaterial::Wood },
		// The town's painted surfaces are painted boards.
		{ TEXT("Paint"), EImpactMaterial::Wood },
		{ TEXT("Walnut"), EImpactMaterial::Wood },
		{ TEXT("Oak"), EImpactMaterial::Wood },
		{ TEXT("Pine"), EImpactMaterial::Wood },
		{ TEXT("Birch"), EImpactMaterial::Wood },
		{ TEXT("Ebony"), EImpactMaterial::Wood },
		{ TEXT("Charcoal"), EImpactMaterial::Wood },
		{ TEXT("Stump"), EImpactMaterial::Wood },
		{ TEXT("Branch"), EImpactMaterial::Wood },
		{ TEXT("Twig"), EImpactMaterial::Wood },
		{ TEXT("Twigs"), EImpactMaterial::Wood },
		{ TEXT("Ladder"), EImpactMaterial::Wood },
		{ TEXT("Wagon"), EImpactMaterial::Wood },
		{ TEXT("Grass"), EImpactMaterial::Grass },
		{ TEXT("Meadow"), EImpactMaterial::Grass },
		{ TEXT("Moss"), EImpactMaterial::Grass },
		{ TEXT("Lawn"), EImpactMaterial::Grass },
		{ TEXT("Turf"), EImpactMaterial::Grass },
		{ TEXT("Clover"), EImpactMaterial::Grass },
		{ TEXT("Reed"), EImpactMaterial::Grass },
		{ TEXT("Fern"), EImpactMaterial::Grass },
		{ TEXT("Bush"), EImpactMaterial::Grass },
		{ TEXT("Shrub"), EImpactMaterial::Grass },
		{ TEXT("Dirt"), EImpactMaterial::Dirt },
		{ TEXT("Mud"), EImpactMaterial::Dirt },
		{ TEXT("Soil"), EImpactMaterial::Dirt },
		{ TEXT("Earth"), EImpactMaterial::Dirt },
		{ TEXT("Clay"), EImpactMaterial::Dirt },
		{ TEXT("Path"), EImpactMaterial::Dirt },
		{ TEXT("Road"), EImpactMaterial::Dirt },
		{ TEXT("DenFloor"), EImpactMaterial::Dirt },
		{ TEXT("Ground"), EImpactMaterial::Dirt },
		{ TEXT("Grave"), EImpactMaterial::Dirt },
		{ TEXT("Mound"), EImpactMaterial::Dirt },
	};

	/** EImpactMaterial's names, in its order: also the words a "Surface.<Name>" tag may use. */
	const TCHAR* const ImpactMaterialNames[] = {
		TEXT("Unknown"), TEXT("Dirt"), TEXT("Grass"), TEXT("Gravel"), TEXT("Sand"), TEXT("Stone"), TEXT("Wood"), TEXT("Metal"),
		TEXT("SheetMetal"), TEXT("Glass"), TEXT("Water"), TEXT("Cloth"), TEXT("Hay"), TEXT("Bone"),
	};
	static_assert(UE_ARRAY_COUNT(ImpactMaterialNames) == static_cast<int32>(EImpactMaterial::Count), "A name for every surface");

	const TCHAR* const ImpactTagPrefix = TEXT("Surface.");
}

bool ImpactSurface::HasWord(const FString& Name, const TCHAR* Word)
{
	const int32 Length = FCString::Strlen(Word);
	int32 From = 0;
	while (From < Name.Len())
	{
		const int32 At = Name.Find(Word, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
		if (At == INDEX_NONE)
		{
			return false;
		}
		const int32 After = At + Length;
		if (After >= Name.Len() || !FChar::IsLower(Name[After]))
		{
			return true;
		}
		From = At + 1;
	}
	return false;
}

bool ImpactSurface::FromName(const FString& Name, EImpactMaterial& OutMaterial)
{
	for (const FSurfaceWord& Entry : ImpactSurfaceWords)
	{
		if (HasWord(Name, Entry.Word))
		{
			OutMaterial = Entry.Material;
			return true;
		}
	}
	return false;
}

bool ImpactSurface::FromTag(FName Tag, EImpactMaterial& OutMaterial)
{
	const FString Text = Tag.ToString();
	if (!Text.StartsWith(ImpactTagPrefix, ESearchCase::IgnoreCase))
	{
		return false;
	}
	const FString Rest = Text.RightChop(FCString::Strlen(ImpactTagPrefix));
	// The surface's own name in any case (Surface.wood), then any word that names one (Surface.Tin, Surface.Ballast).
	for (int32 Index = 1; Index < static_cast<int32>(EImpactMaterial::Count); ++Index)
	{
		if (Rest.Equals(ImpactMaterialNames[Index], ESearchCase::IgnoreCase))
		{
			OutMaterial = static_cast<EImpactMaterial>(Index);
			return true;
		}
	}
	return FromName(Rest, OutMaterial);
}

const TCHAR* ImpactSurface::Name(EImpactMaterial Material)
{
	const int32 Index = static_cast<int32>(Material);
	return Index >= 0 && Index < UE_ARRAY_COUNT(ImpactMaterialNames) ? ImpactMaterialNames[Index] : TEXT("Unknown");
}
