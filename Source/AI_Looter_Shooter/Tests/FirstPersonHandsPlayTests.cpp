#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/FirstPersonHandsComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponTypes.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The first-person hands on the real character (BP_LooterCharacter) in a test level, each of the three guns in hand:
// both hands on the gun from the hip and through the sight, in the frame the gun was placed (no lag), and the hands gone
// in third person and with no gun.

using namespace LocomotionTestWorld;

namespace
{
	/** How near a hand's grip point must be to where it was sent (cm), and how far its spot may sit off the gun's socket. */
	constexpr float HandsPlayOnSpot = 1.f;
	constexpr float HandsPlayOffSocket = 3.5f;
	/** Frames for a sight to come all the way up (a slow gun takes under half a second). */
	constexpr int32 HandsPlayRaiseFrames = 40;

	struct FHandsPlayPlayer
	{
		UPlayerLocomotionComponent* Locomotion = nullptr;
		UPlayerViewComponent* View = nullptr;
		UFirstPersonHandsComponent* Hands = nullptr;
		AWeaponBase* Gun = nullptr;
	};

	FHandsPlayPlayer HandsPlaySpawn(UWorld* World, const FVector& Feet, UWeaponDefinition* Definition)
	{
		FHandsPlayPlayer Player;
		Player.Locomotion = SpawnPlayer(World, Feet);
		ACharacter* Body = Player.Locomotion ? BodyOf(Player.Locomotion) : nullptr;
		if (!Body)
		{
			return Player;
		}
		Player.View = Body->FindComponentByClass<UPlayerViewComponent>();
		Player.Hands = Body->FindComponentByClass<UFirstPersonHandsComponent>();
		UWeaponManagerComponent* Weapons = Body->FindComponentByClass<UWeaponManagerComponent>();
		if (Weapons && Definition)
		{
			Weapons->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Rare, 1));
			Player.Gun = Weapons->GetActiveWeapon();
		}
		return Player;
	}

	/** One frame as the game runs it: the gun placed (the locomotion), the sight raised (the view), then the hands. */
	void HandsPlayFrame(const FHandsPlayPlayer& Player)
	{
		PlayFrame(Player.Locomotion);
		static_cast<UActorComponent*>(Player.View)->TickComponent(Frame, LEVELTICK_All, nullptr);
		Player.Hands->UpdateHands(Frame);
	}

	/** Each hand on its spot, and each spot on the gun's grip or foregrip socket as the gun is this frame. */
	void HandsPlayCheck(FAutomationTestBase& Test, const FHandsPlayPlayer& Player, const FString& Name)
	{
		const UWeaponModelComponent* Model = Player.Gun->FindComponentByClass<UWeaponModelComponent>();
		if (!Test.TestTrue(FString::Printf(TEXT("%s: the gun is built from its parts"), *Name), Model && Model->IsAssembled()))
		{
			return;
		}
		const FTransform GunNow = Model->GetComponentTransform();
		struct FHandsPlaySide
		{
			EFirstPersonHand Hand;
			const TCHAR* Which;
			FVector Socket;
		};
		const FHandsPlaySide Sides[] = {
			{ EFirstPersonHand::Right, TEXT("right hand on the grip"), GunNow.TransformPosition(Model->GetGrip()) },
			{ EFirstPersonHand::Left, TEXT("left hand on the foregrip"), GunNow.TransformPosition(Model->GetForegrip()) },
		};
		for (const FHandsPlaySide& Side : Sides)
		{
			const FVector Target = Player.Hands->GetTargetHoldPoint(Side.Hand);
			const float OnSpot = static_cast<float>(FVector::Dist(Player.Hands->GetHoldPoint(Side.Hand), Target));
			const float OffSocket = static_cast<float>(FVector::Dist(Target, Side.Socket));
			Test.TestTrue(FString::Printf(TEXT("%s: %s (%.2f cm from its spot)"), *Name, Side.Which, OnSpot), OnSpot <= HandsPlayOnSpot);
			Test.TestTrue(FString::Printf(TEXT("%s: ...its spot on the gun as it is this frame (%.2f cm off the socket)"), *Name, OffSocket),
				OffSocket <= HandsPlayOffSocket);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsOnGunTest, "Looter.Player.Hands.OnGun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsOnGunTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	// As in the game, the actors' own events run (a level never readied for play drops them: AActor::ProcessEvent).
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	if (!TestNotNull(TEXT("A floor"), SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0))))
	{
		return false;
	}
	struct FHandsPlayGun
	{
		const TCHAR* Name;
		const TCHAR* Path;
	};
	const FHandsPlayGun Guns[] = {
		{ TEXT("Bullpup"), TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle") },
		{ TEXT("Ranchhand"), TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun") },
		{ TEXT("Drover"), TEXT("/Game/Weapons/Data/DA_Revolver.DA_Revolver") },
	};
	FHandsPlayPlayer Players[UE_ARRAY_COUNT(Guns)];
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Guns); ++Index)
	{
		UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, Guns[Index].Path);
		Players[Index] = HandsPlaySpawn(World, FVector(0.0, -2000.0 + 2000.0 * Index, 0.0), Definition);
		const FHandsPlayPlayer& Made = Players[Index];
		TestNotNull(FString::Printf(TEXT("%s: its definition"), Guns[Index].Name), Definition);
		TestNotNull(FString::Printf(TEXT("%s: the player (BP_LooterCharacter)"), Guns[Index].Name), Made.Locomotion);
		TestNotNull(FString::Printf(TEXT("%s: its first-person hands"), Guns[Index].Name), Made.Hands);
		TestNotNull(FString::Printf(TEXT("%s: the gun in hand"), Guns[Index].Name), Made.Gun);
		if (!Made.Locomotion || !Made.View || !Made.Hands || !Made.Gun)
		{
			return false;
		}
	}
	// The guns drawn a while ago (each comes up over its ready time).
	World->TimeSeconds += 10.0;

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Guns); ++Index)
	{
		const FHandsPlayPlayer& Player = Players[Index];
		for (int32 Frames = 0; Frames < 10; ++Frames)
		{
			HandsPlayFrame(Player);
		}
		TestTrue(FString::Printf(TEXT("%s: the hands show in first person with a gun"), Guns[Index].Name), Player.Hands->IsShowingHands());
		HandsPlayCheck(*this, Player, FString::Printf(TEXT("%s from the hip"), Guns[Index].Name));

		Player.View->HandleAimPressed();
		for (int32 Frames = 0; Frames < HandsPlayRaiseFrames; ++Frames)
		{
			HandsPlayFrame(Player);
		}
		TestTrue(FString::Printf(TEXT("%s: the sight came up"), Guns[Index].Name), Player.View->GetAimAlpha() >= 1.f);
		HandsPlayCheck(*this, Player, FString::Printf(TEXT("%s through the sight"), Guns[Index].Name));
		Player.View->HandleAimReleased();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsHiddenTest, "Looter.Player.Hands.Hidden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsHiddenTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("A floor"), SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0))) || !TestNotNull(TEXT("The rifle"), Rifle))
	{
		return false;
	}
	const FHandsPlayPlayer Armed = HandsPlaySpawn(World, FVector(0.0, -1500.0, 0.0), Rifle);
	const FHandsPlayPlayer Unarmed = HandsPlaySpawn(World, FVector(0.0, 1500.0, 0.0), nullptr);
	if (!TestTrue(TEXT("Both players, their views and hands"), Armed.Locomotion && Armed.View && Armed.Hands && Armed.Gun
		&& Unarmed.Locomotion && Unarmed.View && Unarmed.Hands))
	{
		return false;
	}
	World->TimeSeconds += 10.0;

	HandsPlayFrame(Unarmed);
	TestFalse(TEXT("No gun in hand: no hands"), Unarmed.Hands->IsShowingHands() || Unarmed.Hands->GetVisibleFlag());

	HandsPlayFrame(Armed);
	TestTrue(TEXT("A gun in hand in first person: the hands"), Armed.Hands->IsShowingHands() && Armed.Hands->GetVisibleFlag());
	Armed.View->SetViewMode(EPlayerViewMode::ThirdPerson);
	HandsPlayFrame(Armed);
	TestFalse(TEXT("Third person: the body's hands hold the gun, not these"), Armed.Hands->IsShowingHands() || Armed.Hands->GetVisibleFlag());
	Armed.View->SetViewMode(EPlayerViewMode::FirstPerson);
	HandsPlayFrame(Armed);
	TestTrue(TEXT("Back in first person: the hands again"), Armed.Hands->IsShowingHands() && Armed.Hands->GetVisibleFlag());
	// The template body's stand-in arms show only when asked (Looter.Hands.StandIn): the game shows no placeholder art.
	if (Armed.Hands->IsUsingStandIn())
	{
		TestTrue(TEXT("Stand-in arms stay hidden in game unless asked"), Armed.Hands->bHiddenInGame != 0);
	}
	return true;
}

#endif
