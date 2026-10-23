#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Missions/MissionRewards.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Tests/AutomationCommon.h"

// A mission's reward gun at rest (MissionRewards::LayFlat): set down on its side where its toss stopped, still, as a gift
// is laid down. The 2026-10-23 play-check: the Old Fare's revolver stood upright on its grip on Fenn's coffin, turning.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionRewardLaysFlatTest, "Looter.Missions.RewardDrop.LaysFlat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionRewardLaysFlatTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!Rifle)
	{
		AddWarning(TEXT("DA_AssaultRifle isn't in this checkout: skipped."));
		return true;
	}
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	const FWeaponInstanceData Rolled = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 3);
	const FVector Rest(200.0, 0.0, 115.0);
	AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, Rolled, FTransform(FRotator(0.0, 35.0, 0.0), Rest));
	if (!TestNotNull(TEXT("A reward gun"), Gun))
	{
		return false;
	}
	UWeaponModelComponent* Model = Gun->FindComponentByClass<UWeaponModelComponent>();
	URotatingMovementComponent* Spinner = Gun->FindComponentByClass<URotatingMovementComponent>();
	TArray<USceneComponent*> Parts;
	if (Model)
	{
		Model->GetChildrenComponents(/*bIncludeAllDescendants*/ true, Parts);
	}
	if (!Model || Parts.IsEmpty())
	{
		AddWarning(TEXT("The rifle isn't built from parts in this checkout: skipped."));
		return true;
	}
	TestFalse(TEXT("Held by nobody, not yet loot: never laid down"), MissionRewards::LayFlat(*Gun, Rest.Z - 15.0));

	// Loot, come to rest on a coffin lid 15 cm under its middle (its loot sphere's radius).
	Gun->Toss(FVector::ZeroVector);
	const double Lid = Rest.Z - 15.0;
	TestTrue(TEXT("Loot at rest: laid down"), MissionRewards::LayFlat(*Gun, Lid));
	TestTrue(TEXT("...on its side"), FMath::IsNearlyEqual(FMath::Abs(Model->GetRelativeRotation().Roll), 90.0, 0.5)
		&& FMath::IsNearlyZero(Model->GetRelativeRotation().Pitch, 0.5));
	TestTrue(TEXT("...still: its loot turn stopped"), Spinner && !Spinner->IsActive());
	FBox Lying(ForceInit);
	for (const USceneComponent* Child : Parts)
	{
		const UPrimitiveComponent* Part = Cast<UPrimitiveComponent>(Child);
		if (Part && Part->IsRegistered() && Part->IsVisible())
		{
			Lying += Part->Bounds.GetBox();
		}
	}
	TestTrue(*FString::Printf(TEXT("...lying on the lid, not over it or in it (lowest point %.1f cm over it)"), Lying.Min.Z - Lid),
		Lying.Min.Z >= Lid - 0.1 && Lying.Min.Z <= Lid + 1.0);
	TestTrue(TEXT("...flatter than it stood (lower than its length)"), Lying.GetSize().Z < FMath::Max(Lying.GetSize().X, Lying.GetSize().Y) * 0.5);
	TestTrue(TEXT("...where it came to rest"), FVector2D(Lying.GetCenter().X - Rest.X, Lying.GetCenter().Y - Rest.Y).Size() < 1.0);
	return true;
}

#endif
