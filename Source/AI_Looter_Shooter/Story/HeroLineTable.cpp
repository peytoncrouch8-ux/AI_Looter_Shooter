// UHeroLineTable: where the asset lives, and telling HeroLines when its rows change in the editor.

#include "Story/HeroLineTable.h"
#include "Story/HeroLines.h"

const TCHAR* UHeroLineTable::AssetPath()
{
	return TEXT("/Game/Data/Story/DA_HeroLines.DA_HeroLines");
}

#if WITH_EDITOR
void UHeroLineTable::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// create_hero_lines.py writes the rows while the editor runs: the lookup is built again on its next use.
	HeroLines::ForgetTable();
}
#endif
