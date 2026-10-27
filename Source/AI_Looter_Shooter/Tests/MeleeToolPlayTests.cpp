#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/FirstPersonHandsComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerMeleeRules.h"
#include "Player/PlayerViewComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The melee strike on the real character (BP_LooterCharacter) in a test level, first person, a rifle in hand: through
// the swing the gun is out of sight and the hero's tool and the arms holding it are shown, in front of the eye; at rest
// the tool is gone and the gun and the hands are back (the 2026-10-27 play-check: mid-swing showed nothing at all).

using namespace LocomotionTestWorld;

namespace
{
	/** The first-person tool's parts (MeleeTool, or the stand-in's MeleeToolHandle/Head; not the third-person one). */
	TArray<UStaticMeshComponent*> FirstPersonToolParts(const AActor& Body)
	{
		TArray<UStaticMeshComponent*> Parts;
		TInlineComponentArray<UStaticMeshComponent*> Meshes(&Body);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			const FString Name = Mesh ? Mesh->GetName() : FString();
			if (Name.StartsWith(TEXT("MeleeTool")) && !Name.StartsWith(TEXT("MeleeToolThirdPerson")))
			{
				Parts.Add(Mesh);
			}
		}
		return Parts;
	}

	bool AnyShown(const TArray<UStaticMeshComponent*>& Parts)
	{
		return Parts.ContainsByPredicate([](const UStaticMeshComponent* Part) { return Part->IsVisible(); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeToolShownTest, "Looter.Melee.Tool.Shown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeToolShownTest::RunTest(const FString& Parameters)
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
	UPlayerLocomotionComponent* Locomotion = SpawnPlayer(World, FVector::ZeroVector);
	ACharacter* Body = Locomotion ? BodyOf(Locomotion) : nullptr;
	UPlayerViewComponent* View = Body ? Body->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	UFirstPersonHandsComponent* Hands = Body ? Body->FindComponentByClass<UFirstPersonHandsComponent>() : nullptr;
	UPlayerMeleeComponent* Melee = UPlayerMeleeComponent::Find(Body);
	UWeaponManagerComponent* Weapons = Body ? Body->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	const UCameraComponent* Eye = UPlayerViewComponent::FindFirstPersonCamera(Body);
	if (!TestTrue(TEXT("The player, its view, hands, strike, weapons and eye"), Body && View && Hands && Melee && Weapons && Eye))
	{
		return false;
	}
	Weapons->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 1));
	AWeaponBase* Gun = Weapons->GetActiveWeapon();
	if (!TestNotNull(TEXT("A rifle in hand"), Gun))
	{
		return false;
	}
	View->SetViewMode(EPlayerViewMode::FirstPerson);
	World->TimeSeconds += 10.0;

	// One frame as the game runs it: the gun placed, the view, the strike (its tool and the gun's pose), then the hands.
	auto Step = [&]()
	{
		PlayFrame(Locomotion);
		static_cast<UActorComponent*>(View)->TickComponent(Frame, LEVELTICK_All, nullptr);
		if (Melee->IsSwinging())
		{
			static_cast<UActorComponent*>(Melee)->TickComponent(Frame, LEVELTICK_All, nullptr);
		}
		Hands->UpdateHands(Frame);
	};

	Step();
	TestTrue(TEXT("At rest: the hands on the gun"), Hands->IsShowingHands() && !Gun->IsHidden());
	TestFalse(TEXT("At rest: no tool"), AnyShown(FirstPersonToolParts(*Body)));

	Melee->BeginSwingNow();
	TestTrue(TEXT("Swinging"), Melee->IsSwinging());
	// Through the swing from the wind-up's top to the follow-through: the gun away, the tool and the arms on show, the
	// tool in front of the eye and within the view.
	int32 Frames = 0;
	int32 ToolFrames = 0;
	int32 ArmsFrames = 0;
	int32 InViewFrames = 0;
	int32 GunAwayFrames = 0;
	while (Melee->IsSwinging() && Frames < 120)
	{
		Step();
		++Frames;
		const float Time = Melee->GetSwingTime();
		if (Time < 0.1f || Time > 0.3f)
		{
			continue;
		}
		const TArray<UStaticMeshComponent*> Parts = FirstPersonToolParts(*Body);
		ToolFrames += AnyShown(Parts) ? 1 : 0;
		ArmsFrames += Hands->IsShowingHands() && Hands->GetVisibleFlag() ? 1 : 0;
		GunAwayFrames += Gun->IsHidden() ? 1 : 0;
		FMeleeToolHold Hold;
		if (Melee->GetToolHold(Hold))
		{
			// The grip in the camera's space: ahead, and inside a 90 degree view's frame.
			const FVector At = Hold.Grip.GetLocation();
			const bool bAhead = At.X > 5.0;
			const bool bInside = bAhead && FMath::Abs(At.Y / At.X) < 1.0 && FMath::Abs(At.Z / At.X) < 0.6;
			InViewFrames += bInside && !Hold.Grip.ContainsNaN() ? 1 : 0;
		}
	}
	const int32 Mid = FMath::RoundToInt(0.2f / Frame);
	TestTrue(FString::Printf(TEXT("The tool shown mid-swing (%d of %d frames)"), ToolFrames, Mid), ToolFrames >= Mid - 1);
	TestTrue(FString::Printf(TEXT("The arms shown mid-swing (%d of %d frames)"), ArmsFrames, Mid), ArmsFrames >= Mid - 1);
	TestTrue(FString::Printf(TEXT("The hand and tool in view mid-swing (%d of %d frames)"), InViewFrames, Mid), InViewFrames >= Mid - 1);
	TestTrue(FString::Printf(TEXT("The gun out of sight mid-swing (%d of %d frames)"), GunAwayFrames, Mid), GunAwayFrames >= Mid - 1);
	TestFalse(TEXT("The swing ends"), Melee->IsSwinging());

	Step();
	TestFalse(TEXT("At rest again: no tool"), AnyShown(FirstPersonToolParts(*Body)));
	TestTrue(TEXT("...the gun back, the hands on it"), !Gun->IsHidden() && Hands->IsShowingHands());
	return true;
}

#endif
