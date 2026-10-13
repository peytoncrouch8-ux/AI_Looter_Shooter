#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/SilverPickup.h"
#include "Loot/AmmoPickup.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Slate/SGameLayerManager.h"
#include "Tests/AutomationCommon.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	/** The engine's layer of a player's screen-space widget components (UWidgetComponent's default SharedLayerName). */
	const FName LabelsLayerName(TEXT("WidgetComponentScreenLayer"));

	/** Stands in for that layer: creature tags, loot labels and damage numbers all draw on it. */
	class FTestLabelsLayer : public IGameLayer
	{
	public:
		FTestLabelsLayer()
			: Widget(SNew(SBox))
		{
			// As the engine makes its own: showing, letting the clicks through.
			Widget->SetVisibility(EVisibility::SelfHitTestInvisible);
		}

		virtual TSharedRef<SWidget> AsWidget() override
		{
			return Widget;
		}

		TSharedRef<SBox> Widget;
	};

	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelReloadWorldLabelsTest, "Looter.LevelReload.WorldLabels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelReloadWorldLabelsTest::RunTest(const FString& Parameters)
{
	// The labels over the world sit on one layer the game's viewport keeps for the player through every level change. The
	// pause menu hides it, and Save & Quit leaves from there (a trip leaves from the map or a station board the same way).
	// The next level's HUD never hid it, and once remembered "shown" for itself: from then on no damage number, creature tag
	// or loot label was seen. A real layer manager stands in for the viewport's.
	const TSharedRef<SGameLayerManager> Layers = SNew(SGameLayerManager);
	// Local players live within the engine (as the game instance makes them); these two are never added to a viewport.
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	ULocalPlayer* Other = NewObject<ULocalPlayer>(GEngine);
	if (!TestNotNull(TEXT("A local player"), Player) || !TestNotNull(TEXT("Another"), Other))
	{
		return false;
	}
	TestFalse(TEXT("Nothing to show before the first label makes the layer"), ALooterHUD::ShowWorldLabels(*Layers, Player, true));

	const TSharedRef<FTestLabelsLayer> Labels = MakeShared<FTestLabelsLayer>();
	const TSharedRef<FTestLabelsLayer> OtherLabels = MakeShared<FTestLabelsLayer>();
	Layers->AddLayerForPlayer(Player, LabelsLayerName, Labels, 0);
	Layers->AddLayerForPlayer(Other, LabelsLayerName, OtherLabels, 0);

	// Level one: the pause menu opens over the game, the labels go under it, and Save & Quit goes to the main menu.
	TestTrue(TEXT("Level one's pause menu finds the layer"), ALooterHUD::ShowWorldLabels(*Layers, Player, false));
	TestTrue(TEXT("Hidden under the pause menu"), Labels->Widget->GetVisibility() == EVisibility::Collapsed);
	TestTrue(TEXT("Another player's labels are left alone"), OtherLabels->Widget->GetVisibility() == EVisibility::SelfHitTestInvisible);

	// Level two, after the main menu: the layer is the same one, still hidden. A new HUD shows it on its first frame.
	const IGameLayer* Kept = Layers->FindLayerForPlayer(Player, LabelsLayerName).Get();
	TestTrue(TEXT("The layer outlives the level"), Kept == &Labels.Get());
	TestTrue(TEXT("Level two's HUD finds it"), ALooterHUD::ShowWorldLabels(*Layers, Player, true));
	TestTrue(TEXT("Damage numbers, tags and labels show again in level two"), Labels->Widget->GetVisibility() == EVisibility::SelfHitTestInvisible);

	// Asked every frame, it keeps to what's asked; a menu in level two still hides it, and closing it shows it again.
	ALooterHUD::ShowWorldLabels(*Layers, Player, true);
	TestTrue(TEXT("Still showing"), Labels->Widget->GetVisibility() == EVisibility::SelfHitTestInvisible);
	ALooterHUD::ShowWorldLabels(*Layers, Player, false);
	TestTrue(TEXT("A menu in level two hides it"), Labels->Widget->GetVisibility() == EVisibility::Collapsed);
	ALooterHUD::ShowWorldLabels(*Layers, Player, true);
	TestTrue(TEXT("And closing it shows it"), Labels->Widget->GetVisibility() == EVisibility::SelfHitTestInvisible);
	TestFalse(TEXT("No player, no layer"), ALooterHUD::ShowWorldLabels(*Layers, nullptr, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelReloadLootLifeTest, "Looter.LevelReload.LootLife",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelReloadLootLifeTest::RunTest(const FString& Parameters)
{
	// Ammo and silver lying about go after five minutes. A Save & Quit then Continue once gave each a whole new life, so a
	// player saving every few minutes kept every drop lying round the fights for good (a session's log: 59, 122, 164, 213,
	// 260 ammo pickups over six loads), until the minimap's markers were all loot and no hostile showed. Put back, each goes
	// on with the seconds it had left; one from an older save, which kept none, keeps a whole life.
	FTestWorldWrapper Level;
	if (!TestTrue(TEXT("Test level made"), Level.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Level.GetTestWorld();
	AAmmoPickup* Shells = AAmmoPickup::SpawnAmmo(World, EAmmoType::Shotgun, 12, FVector(300.0, 0.0, 50.0));
	ASilverPickup* Coins = ASilverPickup::SpawnSilver(World, 25, FVector(-300.0, 0.0, 50.0));
	if (!TestNotNull(TEXT("Shells lying there"), Shells) || !TestNotNull(TEXT("Silver lying there"), Coins))
	{
		return false;
	}
	// Four minutes and twenty seconds on: forty seconds left of their five.
	Shells->SetLifeSpan(40.f);
	Coins->SetLifeSpan(40.f);

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(World, *Save);
	ULooterSessionSave* Loaded = WriteAndRead(Save);
	const FSavedMapWorld* Here = Loaded ? Loaded->FindWorld(USessionSubsystem::MapOf(World)) : nullptr;
	if (!TestNotNull(TEXT("Saved and read back"), Here) || !TestEqual(TEXT("One ammo pickup kept"), Here->AmmoPickups.Num(), 1)
		|| !TestEqual(TEXT("One silver pile kept"), Here->SilverPickups.Num(), 1))
	{
		return false;
	}
	TestNearlyEqual(TEXT("The shells' time left is kept"), Here->AmmoPickups[0].LifeLeft, 40.f, 0.01f);
	TestNearlyEqual(TEXT("The silver's time left is kept"), Here->SilverPickups[0].LifeLeft, 40.f, 0.01f);

	// Continue: the level starts again and the session puts back what was lying there.
	USessionSubsystem::RestoreWorld(World, *Loaded);
	TArray<float> AmmoLives;
	for (TActorIterator<AAmmoPickup> It(World); It; ++It)
	{
		AmmoLives.Add(It->GetLifeSpan());
	}
	TArray<float> SilverLives;
	for (TActorIterator<ASilverPickup> It(World); It; ++It)
	{
		SilverLives.Add(It->GetLifeSpan());
	}
	TestTrue(TEXT("The shells go on with the forty seconds they had"), AmmoLives.Num() == 1 && FMath::IsNearlyEqual(AmmoLives[0], 40.f, 0.01f));
	TestTrue(TEXT("So does the silver"), SilverLives.Num() == 1 && FMath::IsNearlyEqual(SilverLives[0], 40.f, 0.01f));

	// A save from before the time left was kept: what it puts back keeps the life it begins with.
	Loaded->FindOrAddWorld(USessionSubsystem::MapOf(World)).AmmoPickups[0].LifeLeft = 0.f;
	USessionSubsystem::RestoreWorld(World, *Loaded);
	TArray<float> OldLives;
	for (TActorIterator<AAmmoPickup> It(World); It; ++It)
	{
		if (!It->IsActorBeingDestroyed())
		{
			OldLives.Add(It->GetLifeSpan());
		}
	}
	TestTrue(TEXT("An older save's ammo isn't cut short"), OldLives.Num() == 1 && !FMath::IsNearlyEqual(OldLives[0], 40.f, 0.01f));
	return true;
}

#endif
