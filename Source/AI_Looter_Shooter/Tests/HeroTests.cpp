#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "Embers/HeroDefinition.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "UI/Menus/HeroSelectWidget.h"
#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/UserWidget.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Retargeter/IKRetargeter.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The hero pick (2026-10-19): the five definitions complete, which heroes are playable (all five since Odessa's body
// landed, 2026-10-20: a hero in the works is made for the tests that need one, FHeroInTheWorks), the session keeping its
// hero (an old save reads as Ellis), the hero select built and driven by keys (the choice, a hero in the works refused,
// the confirm's beat starting the chosen hero), the main menu's way in and out, and another hero's powers on a body
// (owned, with their code).

namespace
{
	const FName EllisId(TEXT("Ellis"));
	const FName OdessaId(TEXT("Odessa"));
	const FName CraneId(TEXT("Crane"));
	const FName PikeId(TEXT("Pike"));

	/** Looter.Hero.Unlock set for a test and put back however it ends. */
	struct FHeroUnlock
	{
		explicit FHeroUnlock(bool bOn)
		{
			Var = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Hero.Unlock"));
			Before = Var ? Var->GetInt() : 0;
			Set(bOn);
		}
		~FHeroUnlock()
		{
			if (Var)
			{
				Var->Set(Before, ECVF_SetByCode);
			}
		}
		void Set(bool bOn)
		{
			if (Var)
			{
				Var->Set(bOn ? 1 : 0, ECVF_SetByCode);
			}
		}
		IConsoleVariable* Var = nullptr;
		int32 Before = 0;
	};

	/**
	 * A hero in the works for a test: every hero is playable since Odessa's body landed (2026-10-20), so her loaded
	 * definition is made one (its retargeter pointed at a package that isn't in the game, in memory only, never marked
	 * dirty or saved) and put back however the test ends. The hero select and a new game read the same loaded object.
	 */
	struct FHeroInTheWorks
	{
		FHeroInTheWorks()
		{
			Hero = UHeroDefinition::Load(OdessaId);
			if (Hero)
			{
				Before = Hero->Retargeter;
				Hero->Retargeter = TSoftObjectPtr<UIKRetargeter>(FSoftObjectPath(TEXT("/Game/Tests/RTG_InTheWorks.RTG_InTheWorks")));
			}
		}
		~FHeroInTheWorks()
		{
			if (Hero)
			{
				Hero->Retargeter = Before;
			}
		}
		UHeroDefinition* Hero = nullptr;
		TSoftObjectPtr<UIKRetargeter> Before;
	};

	/** The menus back under the mouse when a test ends, as the game starts. */
	struct FMouseAfter
	{
		FMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
		~FMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
	};

	/** A key pressed and let go, as a player taps it. */
	template <typename TScreen>
	bool Tap(TScreen& Screen, const FKey& Key)
	{
		const bool bTaken = Screen.HandleNavKey(Key);
		Screen.HandleNavKeyUp(Key);
		return bTaken;
	}

	/** Runs the screen's clock for Seconds, a frame at a time. */
	void RunScreen(UHeroSelectWidget& Screen, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Time = 0.f; Time < Seconds; Time += Frame)
		{
			Screen.AdvanceNav(Frame);
		}
	}

	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	ULooterSessionSave* NewSave(int32 Version)
	{
		ULooterSessionSave* Save = NewObject<ULooterSessionSave>(GetTransientPackage());
		Save->Version = Version;
		Save->Map = TEXT("/Game/Maps/Lvl_RansomsRest");
		return Save;
	}
}

// ---------------------------------------------------------------------------
// The five
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroDefinitionsTest, "Looter.Heroes.Definitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroDefinitionsTest::RunTest(const FString& Parameters)
{
	// The contract's colours (sRGB), by hero.
	const TMap<FName, FLinearColor> Colours = {
		{ EllisId, LooterUI::Hex(0x5A, 0xD8, 0xFF) }, { OdessaId, LooterUI::Hex(0xD0, 0x4A, 0x68) }, { CraneId, LooterUI::Hex(0xB9, 0xD9, 0x8C) },
		{ FName(TEXT("Gauge")), LooterUI::Hex(0xFF, 0xB3, 0x47) }, { PikeId, LooterUI::Hex(0x3F, 0xBF, 0xAA) } };

	const TArray<FName>& Roster = UHeroDefinition::RosterIds();
	TestEqual(TEXT("Five on the roster"), Roster.Num(), 5);
	TestEqual(TEXT("Ellis first, the default"), UHeroDefinition::DefaultId(), EllisId);
	TSet<FName> HeroIds;
	TSet<FName> PowerIds;
	for (const FName Id : Roster)
	{
		const UHeroDefinition* Hero = UHeroDefinition::Load(Id);
		if (!TestNotNull(*FString::Printf(TEXT("DA_Hero_%s (create_ember_assets.py)"), *Id.ToString()), Hero))
		{
			continue;
		}
		const FString Who = Id.ToString();
		TestEqual(*FString::Printf(TEXT("%s: its id"), *Who), Hero->Id, Id);
		TestFalse(*FString::Printf(TEXT("%s: unique"), *Who), HeroIds.Contains(Hero->Id));
		HeroIds.Add(Hero->Id);
		TestFalse(*FString::Printf(TEXT("%s: a name"), *Who), Hero->DisplayName.IsEmpty());
		TestTrue(*FString::Printf(TEXT("%s: a role (\"the ...\")"), *Who), Hero->Role.ToString().StartsWith(TEXT("the ")));
		TestTrue(*FString::Printf(TEXT("%s: a story"), *Who), Hero->Story.ToString().Len() > 40);
		const FLinearColor* Wanted = Colours.Find(Id);
		TestTrue(*FString::Printf(TEXT("%s: the contract's colour"), *Who), Wanted && Hero->Color.Equals(*Wanted, 0.002f));

		// The portrait imported as a UI texture; the body and retargeter named by the convention, landed or not.
		const UTexture2D* Portrait = Hero->LoadPortrait();
		if (TestNotNull(*FString::Printf(TEXT("%s: the portrait loads (import_hero_portraits.py)"), *Who), Portrait))
		{
			TestEqual(*FString::Printf(TEXT("%s: a UI texture"), *Who), static_cast<int32>(Portrait->LODGroup.GetValue()), static_cast<int32>(TEXTUREGROUP_UI));
		}
		TestEqual(*FString::Printf(TEXT("%s: the portrait's path"), *Who), Hero->GetPortraitPath().GetAssetPathString(),
			FString::Printf(TEXT("/Game/UI/Portraits/T_Portrait_%s.T_Portrait_%s"), *Who, *Who));
		TestEqual(*FString::Printf(TEXT("%s: the body's path"), *Who), Hero->GetFigureMeshPath().GetAssetPathString(),
			FString::Printf(TEXT("/Game/Art/Characters/SK_%s.SK_%s"), *Who, *Who));
		TestEqual(*FString::Printf(TEXT("%s: the retargeter's path"), *Who), Hero->GetRetargeterPath().GetAssetPathString(),
			FString::Printf(TEXT("/Game/Player/Retarget/RTG_MannyTo%s.RTG_MannyTo%s"), *Who, *Who));
		// A field is set only once its asset is in the game (no soft pointer to a missing package).
		TestTrue(*FString::Printf(TEXT("%s: the body's field set only once it's there"), *Who), Hero->FigureMesh.IsNull() || Hero->LoadFigureMesh());
		TestTrue(*FString::Printf(TEXT("%s: the retargeter's too"), *Who), Hero->Retargeter.IsNull() || Hero->LoadRetargeter());

		// Three powers, each with its words, colour and clock; no power is two heroes'.
		if (!TestEqual(*FString::Printf(TEXT("%s: three powers"), *Who), Hero->Powers.Num(), 3))
		{
			continue;
		}
		for (const UEmberDefinition* Power : Hero->Powers)
		{
			if (!TestNotNull(*FString::Printf(TEXT("%s: a power"), *Who), Power))
			{
				continue;
			}
			const FString What = FString::Printf(TEXT("%s's %s"), *Who, *Power->Id.ToString());
			TestFalse(*FString::Printf(TEXT("%s: one hero's only"), *What), PowerIds.Contains(Power->Id));
			PowerIds.Add(Power->Id);
			TestTrue(*FString::Printf(TEXT("%s: words"), *What), !Power->DisplayName.IsEmpty() && !Power->Nature.IsEmpty() && Power->Description.ToString().Len() > 30);
			TestTrue(*FString::Printf(TEXT("%s: a colour of its own"), *What), !Power->Color.Equals(FLinearColor::White) && Power->Color.A == 1.f);
			TestTrue(*FString::Printf(TEXT("%s: a cooldown"), *What), Power->CooldownSeconds > 0.f);
		}
	}
	TestEqual(TEXT("Fifteen powers"), PowerIds.Num(), 15);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroPlayableTest, "Looter.Heroes.Playable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroPlayableTest::RunTest(const FString& Parameters)
{
	FHeroUnlock Unlock(false);
	if (!TestNotNull(TEXT("Looter.Hero.Unlock exists"), Unlock.Var))
	{
		return false;
	}
	for (const FName Id : UHeroDefinition::RosterIds())
	{
		const UHeroDefinition* Hero = UHeroDefinition::Load(Id);
		if (!TestNotNull(*FString::Printf(TEXT("DA_Hero_%s"), *Id.ToString()), Hero))
		{
			continue;
		}
		// Playable exactly when the body, its retargeter and every power's code are in the game.
		const bool bParts = Hero->HasAllPowerCode() && Hero->LoadFigureMesh() && Hero->LoadRetargeter();
		TestEqual(*FString::Printf(TEXT("%s: playable by its parts"), *Id.ToString()), Hero->IsPlayable(), bParts);
		TestEqual(*FString::Printf(TEXT("%s: picked only when playable"), *Id.ToString()), Hero->CanBePicked(), Hero->IsPlayable());
		// Every hero's three powers have their code (Embers/Powers, 2026-10-20): what's left is a body and its retargeter.
		TestTrue(*FString::Printf(TEXT("%s: every power has its code"), *Id.ToString()), Hero->HasAllPowerCode());
		// All five are in the game (Odessa's body last, 2026-10-20): each body with its retargeter.
		TestTrue(*FString::Printf(TEXT("%s is playable (SK_%s, RTG_MannyTo%s, three powers with code)"), *Id.ToString(), *Id.ToString(),
			*Id.ToString()), Hero->IsPlayable());
	}

	// The developers' unlock lets a hero in the works be picked (one made for the test: FHeroInTheWorks).
	FHeroInTheWorks InTheWorks;
	const UHeroDefinition* Odessa = InTheWorks.Hero;
	if (TestNotNull(TEXT("Odessa"), Odessa))
	{
		TestFalse(TEXT("In the works: not playable (no retargeter)"), Odessa->IsPlayable());
		TestFalse(TEXT("...not picked"), Odessa->CanBePicked());
		Unlock.Set(true);
		TestTrue(TEXT("Unlocked: Odessa can be picked"), UHeroDefinition::AreLockedHeroesUnlocked() && Odessa->CanBePicked());
		Unlock.Set(false);
		TestEqual(TEXT("Locked again: picked only once playable"), Odessa->CanBePicked(), Odessa->IsPlayable());
	}
	TestNull(TEXT("No hero by an unknown id"), UHeroDefinition::Load(TEXT("Nobody")));
	TestNull(TEXT("...or none"), UHeroDefinition::Load(NAME_None));
	return true;
}

// ---------------------------------------------------------------------------
// The session keeps its hero
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroSaveTest, "Looter.Heroes.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroSaveTest::RunTest(const FString& Parameters)
{
	FHeroUnlock Unlock(false);

	// A hero kept through the save format.
	ULooterSessionSave* Picked = NewSave(ULooterSessionSave::CurrentVersion);
	Picked->HeroId = PikeId;
	const ULooterSessionSave* PickedBack = WriteAndRead(Picked);
	if (TestNotNull(TEXT("A save with a hero reads back"), PickedBack))
	{
		TestEqual(TEXT("...its hero kept"), PickedBack->HeroId, PikeId);
		TestEqual(TEXT("...and played"), PickedBack->GetHeroId(), PikeId);
	}

	// A save from before the pick (version 3 without the field, and an older version 2) reads as Ellis.
	const ULooterSessionSave* OldBack = WriteAndRead(NewSave(ULooterSessionSave::CurrentVersion));
	if (TestNotNull(TEXT("A save from before the pick reads"), OldBack))
	{
		TestTrue(TEXT("...no hero in it"), OldBack->HeroId.IsNone());
		TestEqual(TEXT("...played as Ellis"), OldBack->GetHeroId(), EllisId);
	}
	const ULooterSessionSave* OlderBack = WriteAndRead(NewSave(2));
	if (TestNotNull(TEXT("A version 2 save reads"), OlderBack))
	{
		TestEqual(TEXT("...brought up to date as Ellis's"), OlderBack->GetHeroId(), EllisId);
	}

	// A new game takes only a hero that can be picked (Odessa in the works for the test).
	FHeroInTheWorks InTheWorks;
	ULooterSessionSave* NewGame = NewSave(ULooterSessionSave::CurrentVersion);
	TestTrue(TEXT("A new game as Ellis"), USessionSubsystem::ApplyNewGameHero(*NewGame, EllisId) && NewGame->HeroId == EllisId);
	ULooterSessionSave* Refused = NewSave(ULooterSessionSave::CurrentVersion);
	TestFalse(TEXT("...not as Odessa, in the works"), USessionSubsystem::ApplyNewGameHero(*Refused, OdessaId));
	TestTrue(TEXT("...the save untouched"), Refused->HeroId.IsNone());
	TestFalse(TEXT("...nor as nobody"), USessionSubsystem::ApplyNewGameHero(*Refused, TEXT("Nobody")));
	Unlock.Set(true);
	TestTrue(TEXT("Unlocked: Odessa's new game"), USessionSubsystem::ApplyNewGameHero(*Refused, OdessaId) && Refused->HeroId == OdessaId);
	Unlock.Set(false);

	// A play with no session plays Ellis unless Looter.Hero named another; a world with no sessions has no say.
	const FName Before = USessionSubsystem::GetNoSessionHeroId();
	USessionSubsystem::SetNoSessionHeroId(NAME_None);
	TestEqual(TEXT("No session: Ellis"), USessionSubsystem::GetNoSessionHeroId(), EllisId);
	USessionSubsystem::SetNoSessionHeroId(OdessaId);
	TestEqual(TEXT("...or Looter.Hero's"), USessionSubsystem::GetNoSessionHeroId(), OdessaId);
	USessionSubsystem::SetNoSessionHeroId(Before == EllisId ? NAME_None : Before);
	return true;
}

// ---------------------------------------------------------------------------
// The hero select
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroSelectScreenTest, "Looter.Heroes.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroSelectScreenTest::RunTest(const FString& Parameters)
{
	FMouseAfter Mouse;
	FHeroUnlock Unlock(false);
	// Odessa in the works for the test (every hero is playable now): the screen reads her as such when it opens.
	FHeroInTheWorks InTheWorks;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UHeroSelectWidget* Screen = CreateWidget<UHeroSelectWidget>(TestLevel.GetTestWorld(), UHeroSelectWidget::StaticClass());
	if (!TestNotNull(TEXT("The hero select"), Screen))
	{
		return false;
	}
	const TSharedRef<SWidget> ScreenSlate = Screen->TakeWidget();
	int32 Confirms = 0;
	int32 ConfirmedSlot = INDEX_NONE;
	FName ConfirmedHero;
	int32 Backs = 0;
	Screen->OnConfirmed.BindLambda([&](int32 Slot, FName HeroId) { ++Confirms; ConfirmedSlot = Slot; ConfirmedHero = HeroId; });
	Screen->OnBack.BindLambda([&Backs]() { ++Backs; });
	Screen->Open(1);

	// The roster: the five, Ellis chosen and the focus on their card; Play reads their name.
	if (!TestEqual(TEXT("Five cards"), Screen->NumHeroes(), 5) || !TestNotNull(TEXT("Their buttons"), Screen->GetCard(4)))
	{
		return false;
	}
	TestEqual(TEXT("For session 2"), Screen->GetSlot(), 1);
	TestEqual(TEXT("Ellis chosen"), Screen->GetSelectedId(), EllisId);
	TestTrue(TEXT("...the focus on their card"), Screen->GetFocusedWidget() == Screen->GetCard(0));
	TestTrue(TEXT("Ellis can be confirmed"), Screen->CanConfirm(0) && Screen->GetConfirmButton() && Screen->GetConfirmButton()->GetIsEnabled());

	// Keys: the first press shows the focus; then right moves along the roster and the choice follows.
	Tap(*Screen, EKeys::Gamepad_DPad_Right);
	TestEqual(TEXT("The first press only shows the focus"), Screen->GetSelectedId(), EllisId);
	TestTrue(TEXT("Right is the screen's"), Tap(*Screen, EKeys::Gamepad_DPad_Right));
	TestEqual(TEXT("Right: Odessa"), Screen->GetSelectedId(), OdessaId);
	TestTrue(TEXT("...the focus with her"), Screen->GetFocusedWidget() == Screen->GetCard(1));
	TestTrue(TEXT("...the gamepad's hints"), MenuNav::PromptsText(Screen->GetPrompts()) == TEXT("A Play  D-pad Hero  B Back"));

	// A hero in the works: shown, not confirmed (A on the card says why; Play is disabled).
	TestFalse(TEXT("Odessa is in the works"), Screen->CanConfirm(1));
	TestFalse(TEXT("...Play disabled"), Screen->GetConfirmButton()->GetIsEnabled());
	Tap(*Screen, EKeys::Gamepad_FaceButton_Bottom);
	TestFalse(TEXT("A: refused"), Screen->IsConfirming());
	TestTrue(TEXT("...and said"), Screen->GetStatus().Contains(TEXT("in the works")));
	RunScreen(*Screen, UHeroSelectWidget::ConfirmSeconds + 0.2f);
	TestEqual(TEXT("...nothing started"), Confirms, 0);

	// 1 to 5 and the bumpers choose straight away.
	Tap(*Screen, EKeys::Five);
	TestEqual(TEXT("5: Pike"), Screen->GetSelectedId(), PikeId);
	Tap(*Screen, EKeys::Gamepad_RightShoulder);
	TestEqual(TEXT("RB: round to Ellis"), Screen->GetSelectedId(), EllisId);
	Tap(*Screen, EKeys::Gamepad_LeftShoulder);
	Tap(*Screen, EKeys::One);
	TestEqual(TEXT("LB then 1: Ellis"), Screen->GetSelectedId(), EllisId);
	TestTrue(TEXT("...the focus on their card"), Screen->GetFocusedWidget() == Screen->GetCard(0));

	// A on Ellis's card: the beat, then the session starts as Ellis, once; keys meanwhile do nothing.
	Tap(*Screen, EKeys::Gamepad_FaceButton_Bottom);
	TestTrue(TEXT("A: confirming"), Screen->IsConfirming());
	TestEqual(TEXT("...not yet started (the beat)"), Confirms, 0);
	Tap(*Screen, EKeys::Gamepad_DPad_Right);
	TestEqual(TEXT("...the choice held meanwhile"), Screen->GetSelectedId(), EllisId);
	RunScreen(*Screen, UHeroSelectWidget::ConfirmSeconds + 0.2f);
	TestEqual(TEXT("Started once"), Confirms, 1);
	TestTrue(TEXT("...session 2 as Ellis"), ConfirmedSlot == 1 && ConfirmedHero == EllisId);
	RunScreen(*Screen, 0.5f);
	TestEqual(TEXT("...and only once"), Confirms, 1);

	// The start failed after all: the screen takes keys again and says so; B goes back.
	Screen->CancelConfirm(TEXT("Session 2 couldn't be started."));
	TestFalse(TEXT("Cancelled: choosing again"), Screen->IsConfirming());
	Tap(*Screen, EKeys::Gamepad_FaceButton_Right);
	TestEqual(TEXT("B: back to the picker"), Backs, 1);

	// Unlocked, a hero in the works can be confirmed (opening reads it again).
	Unlock.Set(true);
	Screen->Open(0);
	TestTrue(TEXT("Unlocked: Odessa can be confirmed"), Screen->CanConfirm(1));
	Screen->Select(1);
	TestTrue(TEXT("...Play enabled"), Screen->GetConfirmButton()->GetIsEnabled());
	TestTrue(TEXT("...confirmed"), Screen->Confirm());
	RunScreen(*Screen, UHeroSelectWidget::ConfirmSeconds + 0.2f);
	TestTrue(TEXT("...session 1 as Odessa"), Confirms == 2 && ConfirmedSlot == 0 && ConfirmedHero == OdessaId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroSelectMenuTest, "Looter.Heroes.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroSelectMenuTest::RunTest(const FString& Parameters)
{
	FMouseAfter Mouse;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UMainMenuWidget* Menu = CreateWidget<UMainMenuWidget>(TestLevel.GetTestWorld(), UMainMenuWidget::StaticClass());
	if (!TestNotNull(TEXT("The main menu"), Menu))
	{
		return false;
	}
	const TSharedRef<SWidget> MenuSlate = Menu->TakeWidget();
	if (!TestNotNull(TEXT("...holding the hero select"), Menu->GetHeroSelect()))
	{
		return false;
	}
	TestFalse(TEXT("Hidden at first"), Menu->IsHeroSelectShown());

	// An empty slot's New Game opens it (here directly: a test level has no sessions to read).
	Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Bottom);
	Menu->HandleNavKeyUp(EKeys::Gamepad_FaceButton_Bottom);
	Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Bottom);
	Menu->HandleNavKeyUp(EKeys::Gamepad_FaceButton_Bottom);
	TestTrue(TEXT("Single Player: the picker"), Menu->IsPickerShown());
	Menu->OpenHeroSelect(2);
	TestTrue(TEXT("The hero select shows"), Menu->IsHeroSelectShown());
	TestEqual(TEXT("...for session 3"), Menu->GetHeroSelect()->GetSlot(), 2);
	TestTrue(TEXT("...its keys"), Menu->GetFocusedWidget() == Menu->GetHeroSelect()->GetCard(0));
	TestTrue(TEXT("...its hints"), MenuNav::PromptsText(Menu->GetPrompts()) == MenuNav::PromptsText(Menu->GetHeroSelect()->GetPrompts()));

	// The menu hands it every key: right chooses Odessa there; B comes back to the picker.
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Right);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Right);
	TestEqual(TEXT("Right reaches it: Odessa"), Menu->GetHeroSelect()->GetSelectedId(), OdessaId);
	Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Right);
	TestFalse(TEXT("B: closed"), Menu->IsHeroSelectShown());
	TestTrue(TEXT("...back on the picker"), Menu->IsPickerShown());
	return true;
}

// ---------------------------------------------------------------------------
// On the character
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroCharacterTest, "Looter.Heroes.Character",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroCharacterTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHeroDefinition* Crane = UHeroDefinition::Load(CraneId);
	if (!TestNotNull(TEXT("Crane"), Crane))
	{
		return false;
	}
	TestNull(TEXT("A world with no sessions names no hero (the caller keeps its own)"), USessionSubsystem::FindPlayedHero(World));

	// Crane's powers on a bare body: his three owned (the powers page lists them), each with its code; a bare actor is no
	// body to throw a chain from, so Toll Chain's own say refuses the press (no longer NoPower: the code is in).
	AActor* Body = World->SpawnActor<AActor>();
	UEmberComponent* Embers = NewObject<UEmberComponent>(Body);
	Embers->HeroDefinition = Crane;
	Embers->RegisterComponent();
	Body->DispatchBeginPlay();
	TestTrue(TEXT("Crane bound"), Embers->GetHero() == Crane);
	TestEqual(TEXT("...three owned"), Embers->NumOwned(), 3);
	TestEqual(TEXT("...the first equipped"), Embers->GetEquippedId(), FName(TEXT("TollChain")));
	TestEqual(TEXT("...its code there (no body to throw from: refused, not NoPower)"), Embers->TryUse(), EEmberDenied::Refused);
	Embers->RebindHero();
	TestTrue(TEXT("Rebound: the same hero, ready"), Embers->GetHero() == Crane && Embers->IsReady());

	// Ellis's body and its fallbacks: a holstered gun is any slot ending in HolsterGun.
	const UHeroDefinition* Ellis = UHeroDefinition::Load(EllisId);
	const USkeletalMesh* EllisMesh = Ellis ? Ellis->LoadFigureMesh() : nullptr;
	TestTrue(TEXT("Ellis's body is SK_Ellis"), EllisMesh && EllisMesh->GetName() == TEXT("SK_Ellis"));
	TestTrue(TEXT("...the class's default body"), EllisMesh && GetDefault<UBodyFigureComponent>()->GetSkeletalMeshAsset() == EllisMesh);
	TestTrue(TEXT("Ellis's holster"), UBodyFigureComponent::IsHolsterGunSlot(TEXT("EllisHolsterGun")));
	TestTrue(TEXT("Another hero's"), UBodyFigureComponent::IsHolsterGunSlot(TEXT("PikeHolsterGun")));
	TestFalse(TEXT("Not a coat"), UBodyFigureComponent::IsHolsterGunSlot(TEXT("EllisCoat")));
	return true;
}

#endif
