#include "Audio/LooterSoundBank.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

const TCHAR* ULooterSoundBank::AssetPath = TEXT("/Game/Audio/DA_SoundBank.DA_SoundBank");

const FLooterSoundCueEntry* ULooterSoundBank::FindCue(FName Cue) const
{
	return Cues.FindByPredicate([Cue](const FLooterSoundCueEntry& Entry) { return Entry.Cue == Cue; });
}

ULooterSoundBank* ULooterSoundBank::Load()
{
	// Asked first: loading a package that isn't there logs an engine warning, and a checkout without the bank built has none.
	const FString Package = FPackageName::ObjectPathToPackageName(FString(AssetPath));
	if (!FPackageName::DoesPackageExist(Package))
	{
		return nullptr;
	}
	return LoadObject<ULooterSoundBank>(nullptr, AssetPath);
}
