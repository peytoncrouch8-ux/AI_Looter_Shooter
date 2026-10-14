#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"

// The guns drawn bigger than modeled (the user, 2026-10-14: rifle and shotgun 10%, six-gun 20%), as one: every part at
// the scale, every point the model gives (grip, foregrip, aim, muzzle) and its reload part's travel at it too, while
// the component that holds them stays unscaled (whoever places the gun places it by those points).

namespace
{
	struct FModelScaleTestCase
	{
		const TCHAR* Path;
		EWeaponKind Kind;
		float Scale;
	};

	/** A point of the model's, in its body part's own (modeled, unscaled) space. */
	FVector ModelScaleTestInBody(const UWeaponModelComponent& Model, const FVector& InModel)
	{
		const UStaticMeshComponent* Body = Model.GetParts()[0];
		return Body->GetComponentTransform().InverseTransformPosition(Model.GetComponentTransform().TransformPosition(InModel));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponModelScaleTest, "Looter.Weapons.ModelScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponModelScaleTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("A rifle is drawn 10% bigger"), UWeaponModelComponent::ScaleFor(EWeaponKind::Rifle), 1.1f);
	TestEqual(TEXT("A shotgun 10%"), UWeaponModelComponent::ScaleFor(EWeaponKind::Shotgun), 1.1f);
	TestEqual(TEXT("A six-gun 20%"), UWeaponModelComponent::ScaleFor(EWeaponKind::Revolver), 1.2f);
	TestEqual(TEXT("Anything else as modeled"), UWeaponModelComponent::ScaleFor(EWeaponKind::None), 1.f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AActor* Holder = TestLevel.GetTestWorld()->SpawnActor<AActor>();
	UWeaponModelComponent* Model = Holder ? NewObject<UWeaponModelComponent>(Holder) : nullptr;
	if (!TestNotNull(TEXT("A gun's model"), Model))
	{
		return false;
	}
	Holder->SetRootComponent(Model);
	Model->RegisterComponent();

	const FModelScaleTestCase Cases[] = {
		{ TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"), EWeaponKind::Rifle, 1.1f },
		{ TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"), EWeaponKind::Shotgun, 1.1f },
		{ TEXT("/Game/Weapons/Data/DA_Revolver.DA_Revolver"), EWeaponKind::Revolver, 1.2f },
	};
	for (const FModelScaleTestCase& Case : Cases)
	{
		FWeaponInstanceData Instance;
		Instance.Definition = LoadObject<UWeaponDefinition>(nullptr, Case.Path);
		Instance.Rarity = EWeaponRarity::Rare;
		Instance.Seed = 11;
		if (!TestNotNull(Case.Path, Instance.Definition.Get()) || !TestTrue(FString::Printf(TEXT("%s assembles"), Case.Path), Model->Assemble(Instance)))
		{
			continue;
		}
		const TCHAR* Name = Case.Path;
		TestTrue(FString::Printf(TEXT("%s: its kind"), Name), Instance.Definition->Kind == Case.Kind);
		TestEqual(FString::Printf(TEXT("%s: built at its kind's scale"), Name), Model->GetModelScale(), Case.Scale);
		TestTrue(FString::Printf(TEXT("%s: the holder's component isn't scaled"), Name), Model->GetComponentScale().Equals(FVector::OneVector, 1.e-4));
		TestTrue(FString::Printf(TEXT("%s: its body is"), Name), Model->GetParts()[0]->GetRelativeScale3D().Equals(FVector(Case.Scale), 1.e-4));

		// The points it gives sit at the scale: each span between them is the modeled span (measured in the body's own
		// space, which is the model's units) times it.
		const FVector Points[] = { Model->GetGrip(), Model->GetForegrip(), Model->GetAimPoint(), Model->GetMuzzle() };
		bool bScaled = true;
		for (int32 First = 0; First < UE_ARRAY_COUNT(Points); ++First)
		{
			for (int32 Second = First + 1; Second < UE_ARRAY_COUNT(Points); ++Second)
			{
				const double Drawn = FVector::Dist(Points[First], Points[Second]);
				const double Modeled = FVector::Dist(ModelScaleTestInBody(*Model, Points[First]), ModelScaleTestInBody(*Model, Points[Second]));
				bScaled = bScaled && FMath::IsNearlyEqual(Drawn, Modeled * Case.Scale, 0.01);
			}
		}
		TestTrue(FString::Printf(TEXT("%s: grip, foregrip, aim and muzzle at the scale"), Name), bScaled);

		// The reload part travels at it too: a 10 cm pull moves it 10 cm of the modeled gun (a cylinder swings instead).
		if (Model->GetReloadPart() == EWeaponReloadPart::Magazine || Model->GetReloadPart() == EWeaponReloadPart::Pump)
		{
			// The reload slot's part (the model builds one part per slot, in the slots' order).
			const int32 Slot = Instance.Definition->Parts.IndexOfByPredicate([&Instance](const FWeaponPartSlot& Each)
			{
				return Each.Name == Instance.Definition->ReloadSlot;
			});
			const UStaticMeshComponent* Moving = Model->GetParts().IsValidIndex(Slot) ? Model->GetParts()[Slot].Get() : nullptr;
			if (TestNotNull(FString::Printf(TEXT("%s: its reload part"), Name), Moving))
			{
				const FVector Rest = Moving->GetComponentLocation();
				Model->SetReloadTravel(10.f, true);
				const double Moved = FVector::Dist(Moving->GetComponentLocation(), Rest);
				Model->SetReloadTravel(0.f, true);
				TestTrue(FString::Printf(TEXT("%s: a 10 cm pull goes %.2f cm"), Name, Moved), FMath::IsNearlyEqual(Moved, 10.0 * Case.Scale, 0.01));
			}
		}
	}
	Holder->Destroy();
	return true;
}

#endif
