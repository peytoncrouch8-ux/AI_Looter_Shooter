#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/LooterSoundBank.h"
#include "Audio/LooterSoundCues.h"
#include "UI/Bestiary/BestiaryStage.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UObjectHash.h"

// The inventory pauses the game like the pause menu (the user's playtest, 2026-10-27), and everything in it still works
// over the pause: its stands turn and render, its sounds play.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryPauseTest, "Looter.UI.Inventory.Pause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInventoryPauseTest::RunTest(const FString& Parameters)
{
	// Which menus pause: the pause menu and the inventory (any page, the map's too); nothing open, nothing paused. The
	// world's screens (vendor, bench, boards) never ask.
	TestFalse(TEXT("Nothing open: the game runs"), ALooterHUD::PausesGame(false, false));
	TestTrue(TEXT("The pause menu pauses"), ALooterHUD::PausesGame(true, false));
	TestTrue(TEXT("The inventory pauses"), ALooterHUD::PausesGame(false, true));
	TestTrue(TEXT("...and both together"), ALooterHUD::PausesGame(true, true));

	// The loadout's showcase gun and the bestiary's creature tick over the pause (their turn, sway and picture), and the
	// creature's model animates.
	const ALoadoutGunStage* GunStage = GetDefault<ALoadoutGunStage>();
	TestTrue(TEXT("The showcase gun's stand ticks while paused"), GunStage && GunStage->PrimaryActorTick.bTickEvenWhenPaused);
	const ABestiaryStage* CreatureStage = GetDefault<ABestiaryStage>();
	TestTrue(TEXT("The bestiary's stand ticks while paused"), CreatureStage && CreatureStage->PrimaryActorTick.bTickEvenWhenPaused);
	TArray<UObject*> Parts;
	GetObjectsWithOuter(CreatureStage, Parts, /*bIncludeNestedObjects*/ false);
	int32 Models = 0;
	for (const UObject* Part : Parts)
	{
		if (const USkeletalMeshComponent* Model = Cast<USkeletalMeshComponent>(Part))
		{
			++Models;
			TestTrue(TEXT("...its creature animates while paused"), Model->PrimaryComponentTick.bTickEvenWhenPaused);
		}
	}
	TestEqual(TEXT("...it has its one animated model"), Models, 1);

	// The pages' sounds are the menus' (they go on over a paused game). Without the bank yet (a fresh checkout), nothing
	// to check.
	if (const ULooterSoundBank* Bank = ULooterSoundBank::Load())
	{
		const TCHAR* PageCues[] = { LooterSoundCue::Open, LooterSoundCue::Close, LooterSoundCue::Tab, LooterSoundCue::Click,
			LooterSoundCue::Hover, LooterSoundCue::Back, LooterSoundCue::Denied, LooterSoundCue::Inventory::Equip,
			LooterSoundCue::Inventory::Stow, LooterSoundCue::Inventory::Drop, LooterSoundCue::Inventory::Inspect };
		for (const TCHAR* Cue : PageCues)
		{
			const FLooterSoundCueEntry* Entry = Bank->FindCue(FName(Cue));
			TestTrue(FString::Printf(TEXT("%s plays over the pause"), Cue), Entry && Entry->bPlaysWhilePaused);
		}
	}
	return true;
}

#endif
