#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Combat/RecoverySettings.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/SoulMotePickup.h"
#include "Player/FirstPersonHandsComponent.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Player/PlayerFlaskComponent.h"
#include "Player/PlayerFlaskMotion.h"
#include "Player/PlayerFlaskRules.h"
#include "Player/PlayerFlaskSave.h"
#include "Player/PlayerViewComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "Tests/RecoveryTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The soul flask in a test level (its rules alone are PlayerFlaskTests.cpp): the drink heals 35% of the most over its
// window and a hit doesn't stop it, the slow, the refusals and what they tell, the soul kills and motes pour in, the
// session's save, and the real character's first-person hand on the flask with the gun dipped and held.

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** A holder as the game has one for the flask: a plain character with health and the flask, started as play starts it. */
	struct FFlaskHolder
	{
		ACharacter* Body = nullptr;
		UHealthComponent* Health = nullptr;
		UPlayerFlaskComponent* Flask = nullptr;
	};

	FFlaskHolder SpawnHolder(UWorld* World, const FVector& Where)
	{
		FFlaskHolder Holder;
		Holder.Body = World->SpawnActor<ACharacter>(Where, FRotator::ZeroRotator);
		if (!Holder.Body)
		{
			return Holder;
		}
		Holder.Health = NewObject<UHealthComponent>(Holder.Body, TEXT("Health"));
		Holder.Health->bShowDamageNumbers = false;
		Holder.Body->AddInstanceComponent(Holder.Health);
		Holder.Health->RegisterComponent();
		Holder.Flask = NewObject<UPlayerFlaskComponent>(Holder.Body, TEXT("Flask"));
		Holder.Body->AddInstanceComponent(Holder.Flask);
		Holder.Flask->RegisterComponent();
		Holder.Body->DispatchBeginPlay();
		return Holder;
	}

	/** The flask at Charges and Soul, as a session would put it back. */
	void SetFlask(UPlayerFlaskComponent& Flask, int32 Charges, float Soul = 0.f)
	{
		FFlaskSave Save;
		Save.Charges = Charges;
		Save.Soul = Soul;
		Save.bSaved = true;
		Flask.RestoreFlask(Save);
	}

	/** Steps a drink to its end (the heal's too), up to Seconds. */
	void RunOut(UPlayerFlaskComponent& Flask, float Seconds = 2.f)
	{
		for (float Time = 0.f; Time < Seconds && (Flask.IsDrinking() || Flask.IsHealing()); Time += Frame)
		{
			Flask.Advance(Frame);
		}
		// The heal's last slice runs past the hands' part.
		Flask.Advance(Frame);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskDrinkTest, "Looter.Flask.Drink",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskDrinkTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	const FFlaskHolder Holder = SpawnHolder(World, FVector::ZeroVector);
	if (!TestTrue(TEXT("A holder with health and the flask"), Holder.Body && Holder.Health && Holder.Flask))
	{
		return false;
	}
	UPlayerFlaskComponent& Flask = *Holder.Flask;
	UHealthComponent& Health = *Holder.Health;
	TestEqual(TEXT("A new body's flask is full"), Flask.GetCharges(), FFlaskRules::MaxCharges);
	TestEqual(TEXT("...with no soul toward a next"), Flask.GetSoulShare(), 0.f);

	int32 Drunk = 0;
	const FDelegateHandle Listening = Flask.OnFlaskChanged.AddLambda([&Drunk](int32, int32 Delta, EFlaskChange Why)
	{
		Drunk += Why == EFlaskChange::Drunk && Delta == -1 ? 1 : 0;
	});
	ON_SCOPE_EXIT
	{
		Holder.Flask->OnFlaskChanged.Remove(Listening);
	};

	// Down to 30 of 100, then a drink: a charge goes at the press, the walk slows, nothing heals before the sip.
	RecoveryTestWorld::Hurt(Holder.Body, 70.f);
	TestEqual(TEXT("Hurt to 30"), Health.GetHealth(), 30.f, 0.01f);
	TestTrue(TEXT("The press starts a drink"), Flask.TryDrink());
	TestTrue(TEXT("...drinking"), Flask.IsDrinking());
	TestEqual(TEXT("...one charge gone at once"), Flask.GetCharges(), FFlaskRules::MaxCharges - 1);
	TestEqual(TEXT("...told as drunk"), Drunk, 1);
	const UMovementSlowComponent* Slow = Holder.Body->FindComponentByClass<UMovementSlowComponent>();
	TestTrue(TEXT("...walking at 0.75 through the drink"), Slow && Slow->IsSlowed() && FMath::IsNearlyEqual(Slow->GetMultiplier(), FFlaskRules::MoveScale)
		&& FMath::IsNearlyEqual(Slow->GetTimeLeft(), FFlaskRules::DrinkSeconds, 0.01f));
	TestTrue(TEXT("...a second press is refused while it runs"), Flask.GetBlock() == EFlaskBlock::Busy && !Flask.TryDrink());

	float Time = 0.f;
	bool bEarlyHeal = false;
	bool bHealingSeen = false;
	bool bHit = false;
	bool bOnThroughTheHit = false;
	bool bDrinkingAtEnd = false;
	float Lowest = Health.GetHealth();
	while (Time < 1.5f)
	{
		Flask.Advance(Frame);
		Time += Frame;
		if (Time < FFlaskRules::SipAt - Frame)
		{
			bEarlyHeal |= Health.GetHealth() > 30.01f;
		}
		bHealingSeen |= Flask.IsHealing();
		// A hit mid-heal: the drink goes on.
		if (!bHit && Time > 0.6f)
		{
			bHit = true;
			RecoveryTestWorld::Hurt(Holder.Body, 5.f);
			Lowest = FMath::Min(Lowest, Health.GetHealth());
			Flask.Advance(Frame);
			Time += Frame;
			bOnThroughTheHit = Flask.IsDrinking();
		}
		if (Time > FFlaskRules::DrinkSeconds - 2.f * Frame && Time < FFlaskRules::DrinkSeconds - Frame)
		{
			bDrinkingAtEnd = Flask.IsDrinking();
		}
	}
	TestFalse(TEXT("No heal before the sip"), bEarlyHeal);
	TestTrue(TEXT("The heal flows after it"), bHealingSeen);
	TestTrue(TEXT("A hit doesn't stop the drink"), bOnThroughTheHit);
	TestTrue(TEXT("Still drinking just before 1.1 s"), bDrinkingAtEnd);
	TestFalse(TEXT("Over after"), Flask.IsDrinking() || Flask.IsHealing());
	TestEqual(TEXT("35 of 100 healed in all (30 - 5 + 35)"), Health.GetHealth(), 60.f, 0.05f);
	TestEqual(TEXT("Two charges left"), Flask.GetCharges(), FFlaskRules::MaxCharges - 1);
	TestEqual(TEXT("One drink counted"), Flask.GetDrinkCount(), 1);

	// The next press waits 0.4 s after the drink's end (the test level's clock moved by hand).
	TestTrue(TEXT("Just after: still busy"), Flask.GetBlock() == EFlaskBlock::Busy);
	World->TimeSeconds += FFlaskRules::RepeatSeconds + 0.05f;
	TestTrue(TEXT("0.45 s on: ready"), Flask.GetBlock() == EFlaskBlock::None);

	// Near full: the heal stops at the most, never past it.
	Health.SetHealth(Health.GetMaxHealth() - 10.f);
	TestTrue(TEXT("A drink at 90"), Flask.TryDrink());
	RunOut(Flask);
	TestEqual(TEXT("...heals to 100, no further"), Health.GetHealth(), Health.GetMaxHealth(), 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskBlocksTest, "Looter.Flask.Blocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskBlocksTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	const FFlaskHolder Holder = SpawnHolder(World, FVector::ZeroVector);
	if (!TestTrue(TEXT("A holder with health and the flask"), Holder.Body && Holder.Health && Holder.Flask))
	{
		return false;
	}
	UPlayerFlaskComponent& Flask = *Holder.Flask;
	EFlaskChange LastWhy = EFlaskChange::Restored;
	int32 Told = 0;
	const FDelegateHandle Listening = Flask.OnFlaskChanged.AddLambda([&LastWhy, &Told](int32, int32, EFlaskChange Why) { LastWhy = Why; ++Told; });
	ON_SCOPE_EXIT
	{
		Holder.Flask->OnFlaskChanged.Remove(Listening);
	};

	// Full health: refused, said ("Health full"), no charge used.
	TestTrue(TEXT("At full health: refused"), Flask.GetBlock() == EFlaskBlock::FullHealth && !Flask.TryDrink());
	TestTrue(TEXT("...told as full"), LastWhy == EFlaskChange::DeniedFull);
	TestEqual(TEXT("...no charge used"), Flask.GetCharges(), FFlaskRules::MaxCharges);
	// Within half a point of the most counts as full, as for the motes.
	RecoveryTestWorld::Hurt(Holder.Body, 0.3f);
	TestTrue(TEXT("A scratch: still full"), Flask.GetBlock() == EFlaskBlock::FullHealth);

	// Empty: refused, said (the pips flash), nothing changes.
	RecoveryTestWorld::Hurt(Holder.Body, 40.f);
	SetFlask(Flask, 0);
	Told = 0;
	TestTrue(TEXT("Hurt and empty: refused"), Flask.GetBlock() == EFlaskBlock::Empty && !Flask.TryDrink());
	TestTrue(TEXT("...told as empty, once"), LastWhy == EFlaskChange::DeniedEmpty && Told == 1);
	TestFalse(TEXT("...no drink"), Flask.IsDrinking());

	// Dead: refused, quietly.
	SetFlask(Flask, 2);
	RecoveryTestWorld::Hurt(Holder.Body, 1000.f);
	Told = 0;
	TestTrue(TEXT("Dead: refused"), Holder.Health->IsDead() && Flask.GetBlock() == EFlaskBlock::Dead && !Flask.TryDrink());
	TestEqual(TEXT("...and nothing said"), Told, 0);
	TestEqual(TEXT("...two charges still"), Flask.GetCharges(), 2);

	// The respawn's refill (Main's hook in UPlayerVitalsSubsystem): all three back, told as refilled.
	Flask.RefillAll();
	TestTrue(TEXT("Refilled: all three, said so"), Flask.GetCharges() == FFlaskRules::MaxCharges && LastWhy == EFlaskChange::Refilled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskSoulTest, "Looter.Flask.Soul",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskSoulTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	const FFlaskHolder Holder = SpawnHolder(World, FVector::ZeroVector);
	ASpiderCreature* Basic = RecoveryTestWorld::SpawnSpider(World, FVector(3000.0, 0.0, 0.0), ECreatureRank::Basic);
	ASpiderCreature* Gravebound = RecoveryTestWorld::SpawnSpider(World, FVector(3000.0, 600.0, 0.0), ECreatureRank::Epic);
	ASpiderCreature* Ethereal = RecoveryTestWorld::SpawnSpider(World, FVector(3000.0, 1200.0, 0.0), ECreatureRank::Ethereal);
	ASpiderCreature* Boss = RecoveryTestWorld::SpawnSpider(World, FVector(3000.0, -600.0, 0.0), ECreatureRank::Boss);
	if (!TestTrue(TEXT("A holder and four spiders"), Holder.Flask && Basic && Gravebound && Ethereal && Boss))
	{
		return false;
	}
	UPlayerFlaskComponent& Flask = *Holder.Flask;
	int32 Filled = 0;
	const FDelegateHandle Listening = Flask.OnFlaskChanged.AddLambda([&Filled](int32, int32 Delta, EFlaskChange Why)
	{
		Filled += Why == EFlaskChange::Filled ? Delta : 0;
	});
	ON_SCOPE_EXIT
	{
		Holder.Flask->OnFlaskChanged.Remove(Listening);
	};

	SetFlask(Flask, 0);
	TestEqual(TEXT("A Basic kill: 15"), Flask.GiveKillSoul(Basic), 15.f);
	TestEqual(TEXT("...15% toward the first charge"), Flask.GetSoulShare(), 0.15f, 0.001f);
	TestEqual(TEXT("A Gravebound kill: 45"), Flask.GiveKillSoul(Gravebound), 45.f);
	TestEqual(TEXT("An Ethereal kill: 60, over the line"), Flask.GiveKillSoul(Ethereal), 60.f);
	TestTrue(TEXT("...one charge, 20 carried on"), Flask.GetCharges() == 1 && FMath::IsNearlyEqual(Flask.GetSoul(), 20.f));
	TestEqual(TEXT("...told as one filled"), Filled, 1);
	TestEqual(TEXT("A boss's kill: the whole flask"), Flask.GiveKillSoul(Boss), 300.f);
	TestTrue(TEXT("...all three, no soul kept"), Flask.IsFull() && Flask.GetSoul() == 0.f && Flask.GetSoulShare() == 0.f);
	TestEqual(TEXT("...each charge told"), Filled, 3);
	Flask.AddSoul(50.f);
	TestTrue(TEXT("Full: soul isn't kept"), Flask.IsFull() && Flask.GetSoul() == 0.f);
	TestEqual(TEXT("...nothing more told filled"), Filled, 3);

	// The session's save: what it held comes back; a save from before the flask, full.
	SetFlask(Flask, 1, 40.f);
	FFlaskSave Saved;
	Flask.SaveFlask(Saved);
	TestTrue(TEXT("Saved: one charge, 40 soul, marked saved"), Saved.bSaved && Saved.Charges == 1 && FMath::IsNearlyEqual(Saved.Soul, 40.f));
	Flask.RestoreFlask(FFlaskSave());
	TestTrue(TEXT("An older save: full"), Flask.IsFull() && Flask.GetSoul() == 0.f);
	Flask.RestoreFlask(Saved);
	TestTrue(TEXT("Back as saved"), Flask.GetCharges() == 1 && FMath::IsNearlyEqual(Flask.GetSoul(), 40.f));
	FFlaskSave Edited;
	Edited.Charges = 9;
	Edited.Soul = 500.f;
	Edited.bSaved = true;
	Flask.RestoreFlask(Edited);
	TestTrue(TEXT("An edited save is held to three, no soul"), Flask.IsFull() && Flask.GetSoul() == 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskMoteTest, "Looter.Flask.Mote",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskMoteTest::RunTest(const FString& Parameters)
{
	// A soul-mote at full health: drifts in and pours 34 soul into a flask with room; ignores the player when it's full; and
	// a hurt player is healed, the flask untouched.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	const FFlaskHolder Holder = SpawnHolder(World, FVector::ZeroVector);
	if (!TestTrue(TEXT("A holder with health and the flask"), Holder.Body && Holder.Health && Holder.Flask))
	{
		return false;
	}
	UPlayerFlaskComponent& Flask = *Holder.Flask;
	const FVector Near(300.0, 0.0, 60.0);
	auto Offer = [&](int32 Steps) -> bool
	{
		ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, Near, FVector::ZeroVector);
		bool bTaken = false;
		for (int32 Step = 0; Mote && Step < Steps && !bTaken; ++Step)
		{
			bTaken = Mote->Advance(0.05f, Holder.Body);
		}
		if (!bTaken && RecoveryTestWorld::IsHere(Mote))
		{
			Mote->Destroy();
		}
		return bTaken;
	};

	SetFlask(Flask, 1);
	TestTrue(TEXT("Full health, room in the flask: it's wanted"), ASoulMotePickup::IsWanted(*Holder.Body, *Holder.Health));
	TestTrue(TEXT("...drifts in and is taken"), Offer(200));
	TestEqual(TEXT("...34 soul into the flask"), Flask.GetSoul(), FFlaskRules::MoteSoul, 0.01f);
	TestEqual(TEXT("...health as it was"), Holder.Health->GetHealth(), Holder.Health->GetMaxHealth(), 0.01f);

	SetFlask(Flask, FFlaskRules::MaxCharges);
	TestFalse(TEXT("Full health, full flask: no use to them"), ASoulMotePickup::IsWanted(*Holder.Body, *Holder.Health));
	TestFalse(TEXT("...not taken"), Offer(100));

	SetFlask(Flask, 1);
	RecoveryTestWorld::Hurt(Holder.Body, 50.f);
	TestTrue(TEXT("Hurt: taken"), Offer(200));
	TestEqual(TEXT("...it healed (15 of 100)"), Holder.Health->GetHealth(), 50.f + FRecoverySettings().MoteHeal(Holder.Health->GetMaxHealth()), 0.01f);
	TestEqual(TEXT("...the flask untouched"), Flask.GetSoul(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskHandsTest, "Looter.Flask.Hands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskHandsTest::RunTest(const FString& Parameters)
{
	// The real character (BP_LooterCharacter), first person, a rifle in hand: through the drink the gun is held (its swing
	// state) and dipped, the left hand is round the flask on its clock, and after it the gun is free and the hand back.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("A floor"), LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0)))
		|| !TestNotNull(TEXT("The rifle"), Rifle))
	{
		return false;
	}
	UPlayerLocomotionComponent* Locomotion = LocomotionTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	ACharacter* Body = Locomotion ? LocomotionTestWorld::BodyOf(Locomotion) : nullptr;
	UPlayerViewComponent* View = Body ? Body->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	UFirstPersonHandsComponent* Hands = Body ? Body->FindComponentByClass<UFirstPersonHandsComponent>() : nullptr;
	UPlayerFlaskComponent* Flask = UPlayerFlaskComponent::Find(Body);
	UWeaponManagerComponent* Weapons = Body ? Body->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	UHealthComponent* Health = Body ? Body->FindComponentByClass<UHealthComponent>() : nullptr;
	const UCameraComponent* Eye = UPlayerViewComponent::FindFirstPersonCamera(Body);
	if (!TestTrue(TEXT("The player carries the flask, with its view, hands, weapons, health and eye"), Body && View && Hands && Flask && Weapons && Health && Eye))
	{
		return false;
	}
	Weapons->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	AWeaponBase* Gun = Weapons->GetActiveWeapon();
	if (!TestNotNull(TEXT("A rifle in hand"), Gun))
	{
		return false;
	}
	View->SetViewMode(EPlayerViewMode::FirstPerson);
	World->TimeSeconds += 10.0;
	auto Step = [&]()
	{
		LocomotionTestWorld::PlayFrame(Locomotion);
		static_cast<UActorComponent*>(View)->TickComponent(Frame, LEVELTICK_All, nullptr);
		Flask->Advance(Frame);
		Hands->UpdateHands(Frame);
	};
	Step();
	TestFalse(TEXT("At rest: the gun free"), Gun->IsMeleeSwinging());

	Health->Drain(Health->GetMaxHealth() * 0.5f);
	const EFlaskBlock Block = Flask->GetBlock();
	if (!TestTrue(FString::Printf(TEXT("Hurt: the drink starts (block %d)"), static_cast<int32>(Block)), Flask->TryDrink()))
	{
		return false;
	}
	TestTrue(TEXT("The gun held back (its swing state)"), Gun->IsMeleeSwinging());
	bool bHandOnFlask = false;
	bool bJobSeen = false;
	bool bHeldThrough = true;
	for (float Time = 0.f; Time < FFlaskRules::DrinkSeconds + 0.2f; Time += Frame)
	{
		Step();
		if (!Flask->IsDrinking())
		{
			continue;
		}
		bHeldThrough &= Gun->IsMeleeSwinging();
		const float DrinkTime = Flask->GetDrinkTime();
		if (DrinkTime > FlaskMotion::RaiseSeconds && DrinkTime < FlaskMotion::TipEndSeconds)
		{
			bJobSeen |= Hands->GetSupportJob() == EHandsSupportJob::Drink;
			const FVector Wanted = Eye->GetComponentTransform().TransformPosition(FirstPersonHandsMotion::OnFlask(DrinkTime).Hold);
			bHandOnFlask |= Hands->IsShowingHands() && FVector::Dist(Hands->GetTargetHoldPoint(EFirstPersonHand::Left), Wanted) < 1.f;
		}
	}
	TestTrue(TEXT("The gun held through the drink"), bHeldThrough);
	TestTrue(TEXT("The left hand's job was the drink"), bJobSeen);
	TestTrue(TEXT("...round the flask, where FlaskMotion has it"), bHandOnFlask);
	TestFalse(TEXT("After: the gun free again"), Gun->IsMeleeSwinging());
	TestTrue(TEXT("...and the hand back on the foregrip"), Hands->GetSupportJob() == EHandsSupportJob::Foregrip);
	return true;
}

#endif
