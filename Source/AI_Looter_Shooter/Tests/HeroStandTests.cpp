#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/HeroDefinition.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "UI/Menus/HeroSelectStage.h"
#include "UI/Menus/HeroSelectWidget.h"
#include "UI/Style/LooterUIFocus.h"
#include "Blueprint/UserWidget.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Retargeter/IKRetargeter.h"
#include "Tests/AutomationCommon.h"

// The hero select's live stand (2026-10-20): it builds for each of the five (their own body worn over the idling
// mannequin, through their own retargeter), turns when told, stops when the screen closes, and the screen puts the
// chosen hero on it (or their painting, for a hero whose body isn't in the game).

namespace
{
	/** A hero whose body isn't in the game, for one test: Gauge's retargeter pointed nowhere (in memory only). */
	struct FBodyMissing
	{
		FBodyMissing()
		{
			Hero = UHeroDefinition::Load(TEXT("Gauge"));
			if (Hero)
			{
				Before = Hero->Retargeter;
				Hero->Retargeter = TSoftObjectPtr<UIKRetargeter>(FSoftObjectPath(TEXT("/Game/Tests/RTG_NotMade.RTG_NotMade")));
			}
		}
		~FBodyMissing()
		{
			if (Hero)
			{
				Hero->Retargeter = Before;
			}
		}
		UHeroDefinition* Hero = nullptr;
		TSoftObjectPtr<UIKRetargeter> Before;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroStandTest, "Looter.Heroes.Stand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroStandTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();

	// The stand on its own: its picture, then each hero on it, their own body through their own retargeter.
	AHeroSelectStage* Stand = World->SpawnActor<AHeroSelectStage>();
	if (!TestNotNull(TEXT("The stand"), Stand))
	{
		return false;
	}
	Stand->DispatchBeginPlay();
	const UTextureRenderTarget2D* Picture = Stand->GetRenderTarget();
	TestTrue(TEXT("Its picture"), Picture && Picture->SizeX == AHeroSelectStage::ImageWidth && Picture->SizeY == AHeroSelectStage::ImageHeight);
	for (const FName Id : UHeroDefinition::RosterIds())
	{
		const UHeroDefinition* Hero = UHeroDefinition::Load(Id);
		if (!TestNotNull(*FString::Printf(TEXT("DA_Hero_%s"), *Id.ToString()), Hero))
		{
			continue;
		}
		const FString Who = Id.ToString();
		TestTrue(*FString::Printf(TEXT("%s stands on it"), *Who), Stand->ShowHero(Hero, true));
		TestTrue(*FString::Printf(TEXT("%s: their own body worn"), *Who), Stand->IsHeroWorn() && Stand->GetHero() == Hero);
		const UBodyFigureComponent* Figure = Stand->GetFigure();
		TestTrue(*FString::Printf(TEXT("%s: SK_%s"), *Who, *Who), Figure && Figure->GetSkeletalMeshAsset() == Hero->LoadFigureMesh());
		TestTrue(*FString::Printf(TEXT("%s: through RTG_MannyTo%s"), *Who, *Who), Figure && Figure->GetRetargeter() == Hero->LoadRetargeter());
		TestTrue(*FString::Printf(TEXT("%s: turning in from the side"), *Who), Stand->GetTurn() > 30.f);
		// A pale colour's rim is held down (it lit near white and blew Crane's face out); a saturated one's is whole.
		TestEqual(*FString::Printf(TEXT("%s: the rim's share by its colour"), *Who), Stand->GetRimShare(), AHeroSelectStage::RimShareFor(Hero->Color));
	}
	TestTrue(TEXT("Crane's sage rim is held down"), AHeroSelectStage::RimShareFor(FLinearColor(FColor(0xB9, 0xD9, 0x8C))) < 0.8f);
	TestEqual(TEXT("Odessa's crimson rim is whole"), AHeroSelectStage::RimShareFor(FLinearColor(FColor(0xD0, 0x4A, 0x68))), 1.f);

	// Turning: the player's turn takes over from the turn-in at once; running and stopped.
	Stand->AddTurn(-20.f);
	TestEqual(TEXT("Turned by the player"), Stand->GetTurn(), -20.f, 0.01f);
	Stand->SetActive(true);
	TestTrue(TEXT("Running"), Stand->IsActive() && Stand->IsActorTickEnabled());
	Stand->SetActive(false);
	TestTrue(TEXT("Stopped: no tick, nothing rendered"), !Stand->IsActive() && !Stand->IsActorTickEnabled());

	// A hero whose body isn't in the game: nothing on the stand (the screen shows their painting).
	{
		FBodyMissing Missing;
		TestFalse(TEXT("No body: not on the stand"), Missing.Hero && Stand->ShowHero(Missing.Hero, false));
	}

	// The screen: the chosen hero on its stand, a switch puts the next one there, closing stops it.
	UHeroSelectWidget* Screen = CreateWidget<UHeroSelectWidget>(World, UHeroSelectWidget::StaticClass());
	if (!TestNotNull(TEXT("The hero select"), Screen))
	{
		return false;
	}
	const TSharedRef<SWidget> ScreenSlate = Screen->TakeWidget();
	Screen->Open(0);
	AHeroSelectStage* ScreenStand = Screen->GetStand();
	if (!TestNotNull(TEXT("The screen's stand"), ScreenStand))
	{
		return false;
	}
	TestTrue(TEXT("Ellis on it, running"), Screen->IsStandShown() && ScreenStand->IsActive() && ScreenStand->GetHero()
		&& ScreenStand->GetHero()->Id == UHeroDefinition::DefaultId());
	Screen->Select(2);
	TestTrue(TEXT("Chose Crane: Crane on it"), Screen->IsStandShown() && ScreenStand->GetHero() && ScreenStand->GetHero()->Id == FName(TEXT("Crane"))
		&& ScreenStand->IsHeroWorn());
	Screen->HandleTurnStick(1.f);
	const float TurnBefore = ScreenStand->GetTurn();
	Screen->AdvanceNav(0.25f);
	TestTrue(TEXT("The right stick turns them"), !FMath::IsNearlyEqual(ScreenStand->GetTurn(), TurnBefore, 1.f));
	Screen->HandleTurnStick(0.f);
	{
		FBodyMissing Missing;
		Screen->Open(0);
		Screen->Select(3);
		TestFalse(TEXT("Gauge with no body: their painting instead"), Screen->IsStandShown());
	}
	Screen->Close();
	TestFalse(TEXT("Closed: the stand stops"), ScreenStand->IsActive());
	return true;
}

#endif
