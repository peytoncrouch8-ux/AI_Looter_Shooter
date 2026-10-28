#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Interaction/InteractionComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Tests/InteractionTestWorld.h"
#include "UI/Cards/GunCardRules.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/Cards/LooterSegmentBar.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Style/WeaponText.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

// The gun card over the gun on the ground and the gun in hand's on the screen's right (the user's call, 2026-10-28;
// Docs/Work/2026-10-28-UICharacter/CONTRACT.md): Looter.UI.WorldCard.*.

using namespace InteractionTestWorld;

namespace
{
	const TCHAR* const RiflePath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
	const TCHAR* const ShotgunPath = TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun");

	/** On the floor in front of the stand-in, a little below its eyes: in reach and well inside the cone. */
	const FVector LootSpot(120.0, 0.0, 30.0);
	/** Behind the stand-in, out of its cone: a gun lying here is picked up by hand (TryPickup), never looked at. */
	const FVector BehindSpot(-400.0, 0.0, 30.0);

	AWeaponBase* Loot(UWorld* World, UWeaponDefinition* Definition, EWeaponRarity Rarity, const FVector& Where)
	{
		AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, UWeaponRollLibrary::RollWeaponWithRarity(Definition, Rarity, 4), FTransform(Where));
		if (Weapon)
		{
			Weapon->OnDropped();
		}
		return Weapon;
	}

	bool ShowsGun(const UGunCardWidget* Card, const AWeaponBase* Gun)
	{
		return Card && Gun && Card->GetNameText().ToString().Equals(LooterWeaponText::Name(Gun->GetInstance()), ESearchCase::IgnoreCase);
	}

	/** The card's first bar has the compared gun's tick (none: nothing compared). */
	bool Compares(const UGunCardWidget* Card)
	{
		const ULooterSegmentBar* Bar = Card ? Card->GetBar(0) : nullptr;
		return Bar && Bar->GetTick() >= 0.f;
	}

	bool HasHoldCap(const UGunCardWidget* Card)
	{
		return Card && Card->GetKeyCaps().ContainsByPredicate([](const FString& Cap) { return Cap.Contains(TEXT("HOLD")); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldGunCardOverTheGunTest, "Looter.UI.WorldCard.OverTheGun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWorldGunCardOverTheGunTest::RunTest(const FString& Parameters)
{
	// A gun on the ground, looked at: its label shows the gun card (World) in place of its name, compared with the gun in
	// hand and with the take keys from the weapon manager, the HOLD cap filling with the interact hold. Not looked at: the
	// name alone, no card.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, RiflePath);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, ShotgunPath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	if (!TestTrue(TEXT("Stand-in placed"), Player && Interaction))
	{
		return false;
	}
	UWeaponManagerComponent* Manager = NewObject<UWeaponManagerComponent>(Player);
	Manager->RegisterComponent();
	// One slot: once a gun is in hand the slots are full, and a hold takes the loot in hand.
	Manager->MaxWeapons = 1;

	AWeaponBase* Ground = Loot(World, Shotgun, EWeaponRarity::Epic, LootSpot);
	Interaction->UpdateInteraction(0.f);
	if (!TestTrue(TEXT("The shotgun in front is looked at"), Ground && Interaction->GetFocusedActor() == Ground))
	{
		return false;
	}

	UWeaponLabelWidget* Label = CreateWidget<UWeaponLabelWidget>(World, UWeaponLabelWidget::StaticClass());
	if (!TestNotNull(TEXT("A loot label"), Label))
	{
		return false;
	}
	Label->SetWeapon(Ground);
	Label->TakeWidget();
	TestFalse(TEXT("Not looked at: no card"), Label->IsCardShown());
	TestEqual(TEXT("...its name as ever"), Label->GetNameText().ToString(), LooterWeaponText::Name(Ground->GetInstance()).ToUpper());
	TestFalse(TEXT("...and no maker word (the card carries it)"), Label->IsMakerShown());

	// Looked at, unarmed: its card, nothing to compare, a tap takes it.
	Label->SetFocused(true);
	Label->UpdateCard(Manager, Interaction);
	UGunCardWidget* Card = Label->GetCard();
	TestTrue(TEXT("Looked at: the card"), Label->IsCardShown() && Card);
	TestTrue(TEXT("...a world card"), Card && Card->GetMode() == EGunCardMode::World);
	TestTrue(TEXT("...of this gun"), ShowsGun(Card, Ground));
	TestFalse(TEXT("...the dot never shows over it"), Label->IsDotShown());
	TestFalse(TEXT("Unarmed: nothing compared"), Compares(Card));
	TestTrue(TEXT("...a key to take it"), Card && !Card->GetKeyCaps().IsEmpty());
	TestFalse(TEXT("...a free slot: no hold to equip"), HasHoldCap(Card));
	TestEqual(TEXT("The maker shown is the gun's"), Label->IsMakerShown(), !Label->GetMakerText().IsEmpty());

	// A gun in hand: the card compares with it; the one slot full, a hold equips the loot.
	AWeaponBase* Held = Loot(World, Rifle, EWeaponRarity::Common, BehindSpot);
	if (!TestTrue(TEXT("A rifle taken in hand"), Held && Manager->TryPickup(Held) && Manager->GetActiveWeapon() == Held))
	{
		return false;
	}
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The shotgun still looked at"), Interaction->GetFocusedActor() == Ground);
	Label->UpdateCard(Manager, Interaction);
	TestTrue(TEXT("Armed: compared with the gun in hand"), Compares(Card));
	TestTrue(TEXT("...still this gun's card"), ShowsGun(Card, Ground));
	TestTrue(TEXT("Slots full: a hold to equip"), HasHoldCap(Card));
	TestEqual(TEXT("...not held yet"), Card ? Card->GetHoldFill() : -1.f, 0.f);

	// The interact key held on it: the HOLD cap fills.
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.1f);
	Label->UpdateCard(Manager, Interaction);
	TestTrue(TEXT("Held: the HOLD cap fills"), Card && Card->GetHoldFill() > 0.f);

	// Looked away: the name again, no card.
	Label->SetFocused(false);
	TestFalse(TEXT("Looked away: no card"), Label->IsCardShown());
	TestFalse(TEXT("...no maker word"), Label->IsMakerShown());
	TestFalse(TEXT("...not folded"), Label->IsDotShown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldGunCardInHandTest, "Looter.UI.WorldCard.InHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWorldGunCardInHandTest::RunTest(const FString& Parameters)
{
	// The HUD's right card: the gun in hand (InHand, nothing compared, no keys) while loot is looked at; nothing unarmed, and
	// nothing while no loot is looked at.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, RiflePath);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, ShotgunPath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	if (!TestTrue(TEXT("Stand-in placed"), Player && Interaction))
	{
		return false;
	}
	UWeaponManagerComponent* Manager = NewObject<UWeaponManagerComponent>(Player);
	Manager->RegisterComponent();
	Manager->MaxWeapons = 2;

	UPlayerHUDWidget* Hud = CreateWidget<UPlayerHUDWidget>(World, UPlayerHUDWidget::StaticClass());
	if (!TestNotNull(TEXT("The HUD"), Hud))
	{
		return false;
	}
	Hud->TakeWidget();
	if (!TestNotNull(TEXT("Its in-hand card made"), Hud->GetInHandCard()))
	{
		return false;
	}

	// Unarmed, looking at loot: nothing to hold it against.
	AWeaponBase* Ground = Loot(World, Shotgun, EWeaponRarity::Rare, LootSpot);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The shotgun in front is looked at"), Ground && Interaction->GetFocusedActor() == Ground);
	Hud->UpdatePickupCard(Manager, Interaction, 0.f);
	TestFalse(TEXT("Unarmed: no in-hand card"), Hud->IsInHandCardShown());

	// Armed, looking at loot: the gun in hand's card, comparing nothing, with no keys (they're on the card over the loot).
	AWeaponBase* Held = Loot(World, Rifle, EWeaponRarity::Common, BehindSpot);
	if (!TestTrue(TEXT("A rifle taken in hand"), Held && Manager->TryPickup(Held) && Manager->GetActiveWeapon() == Held))
	{
		return false;
	}
	Interaction->UpdateInteraction(0.f);
	Hud->UpdatePickupCard(Manager, Interaction, 0.f);
	const UGunCardWidget* Card = Hud->GetInHandCard();
	TestTrue(TEXT("Armed, looking at loot: the in-hand card"), Hud->IsInHandCardShown());
	TestTrue(TEXT("...an in-hand card"), Card->GetMode() == EGunCardMode::InHand);
	TestTrue(TEXT("...of the gun in hand"), ShowsGun(Card, Held));
	TestFalse(TEXT("...comparing nothing"), Compares(Card));
	TestTrue(TEXT("...with no keys"), Card->GetKeyCaps().IsEmpty());

	// Looking away from the loot: gone.
	Face(Player, 180.0);
	Interaction->UpdateInteraction(0.f);
	TestNull(TEXT("Nothing looked at"), Interaction->GetFocusedActor());
	Hud->UpdatePickupCard(Manager, Interaction, 0.f);
	TestFalse(TEXT("Nothing looked at: no in-hand card"), Hud->IsInHandCardShown());

	// Back to the loot: it shows again.
	Face(Player, 0.0);
	Interaction->UpdateInteraction(0.f);
	Hud->UpdatePickupCard(Manager, Interaction, 0.f);
	TestTrue(TEXT("Looking at loot again: the in-hand card again"), Hud->IsInHandCardShown() && ShowsGun(Card, Held));
	return true;
}

#endif
