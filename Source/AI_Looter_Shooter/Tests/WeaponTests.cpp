#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/WeaponText.h"
#include "Weapons/BulletSubsystem.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponRecoil.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponDamageTextTest, "Looter.Weapons.DamageText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponDamageTextTest::RunTest(const FString& Parameters)
{
	// A weapon shows the one damage number it rolled with; the +/-10% only happens when a hit lands.
	FWeaponStats Rifle;
	Rifle.Damage = 19.6f;
	Rifle.PelletsPerShot = 1;
	TestEqual(TEXT("Rifle shows its damage"), LooterWeaponText::DamageString(Rifle), FString(TEXT("20")));
	TestFalse(TEXT("No damage range on the card"), LooterWeaponText::DamageString(Rifle).Contains(TEXT("-")));

	FWeaponStats Shotgun;
	Shotgun.Damage = 7.2f;
	Shotgun.PelletsPerShot = 9;
	TestEqual(TEXT("Shotgun shows damage per pellet"), LooterWeaponText::DamageString(Shotgun), FString(TEXT("7 x9")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponReloadMotionTest, "Looter.Weapons.ReloadMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponReloadMotionTest::RunTest(const FString& Parameters)
{
	// The first-person gun starts and ends every reload exactly in its hold pose, and moves in between.
	for (const EWeaponReloadPart Part : { EWeaponReloadPart::Magazine, EWeaponReloadPart::Pump })
	{
		const TCHAR* Name = Part == EWeaponReloadPart::Magazine ? TEXT("Magazine") : TEXT("Pump");
		FVector Offset;
		FRotator Rotation;
		LooterReload::ViewModelPose(Part, 0.f, Offset, Rotation);
		TestTrue(FString::Printf(TEXT("%s reload starts in the hold pose"), Name), Offset.IsNearlyZero(0.01) && Rotation.IsNearlyZero(0.01f));
		LooterReload::ViewModelPose(Part, 1.f, Offset, Rotation);
		TestTrue(FString::Printf(TEXT("%s reload ends in the hold pose"), Name), Offset.IsNearlyZero(0.01) && Rotation.IsNearlyZero(0.01f));
		LooterReload::ViewModelPose(Part, 0.5f, Offset, Rotation);
		TestTrue(FString::Printf(TEXT("%s reload brings the gun in"), Name), !Rotation.IsNearlyZero(5.f));
		LooterReload::ViewModelPose(Part, -1.f, Offset, Rotation);
		TestTrue(FString::Printf(TEXT("%s: no motion when not reloading"), Name), Offset.IsZero() && Rotation.IsZero());
	}

	// The magazine comes out, drops out of sight, and a fresh one ends up seated and visible.
	bool bVisible = false;
	float MostTravel = 0.f;
	bool bEverHidden = false;
	for (int32 Step = 0; Step <= 100; ++Step)
	{
		MostTravel = FMath::Max(MostTravel, LooterReload::MagazineTravel(Step / 100.f, bVisible));
		bEverHidden |= !bVisible;
	}
	TestTrue(TEXT("Old magazine comes all the way out"), MostTravel > 20.f);
	TestTrue(TEXT("Old magazine drops out of sight before the new one"), bEverHidden);
	TestEqual(TEXT("Magazine seated at the start"), LooterReload::MagazineTravel(0.f, bVisible), 0.f);
	TestEqual(TEXT("Magazine seated at the end"), LooterReload::MagazineTravel(1.f, bVisible), 0.f);
	TestTrue(TEXT("Magazine visible at the end"), bVisible);

	// The pump is racked once, near the end, and returns home.
	float MostRack = 0.f;
	for (int32 Step = 0; Step <= 100; ++Step)
	{
		MostRack = FMath::Max(MostRack, LooterReload::PumpTravel(Step / 100.f));
	}
	TestTrue(TEXT("Pump is racked"), MostRack > 5.f);
	TestEqual(TEXT("Pump home at the start"), LooterReload::PumpTravel(0.f), 0.f);
	TestEqual(TEXT("Pump home at the end"), LooterReload::PumpTravel(1.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponRecoilTest, "Looter.Weapons.Recoil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponRecoilTest::RunTest(const FString& Parameters)
{
	FWeaponRecoilProfile Profile;
	Profile.KickBack = 3.f;
	Profile.MuzzleFlip = 4.f;
	Profile.MuzzleTwist = 0.f;
	Profile.Roll = 0.f;
	Profile.AimKick = 0.5f;
	Profile.AimKickSide = 0.f;
	FRandomStream Random(7);

	// A shot kicks the gun back and its muzzle up within a few frames, about as far as the profile says, and the aim up.
	FWeaponRecoil Recoil;
	Recoil.AddShot(Profile, Random);
	float PeakBack = 0.f;
	float PeakFlip = 0.f;
	float Aim = 0.f;
	float Time = 0.f;
	for (; Time < 0.06f; Time += 0.005f)
	{
		Aim += Recoil.Tick(0.005f).Pitch;
		PeakBack = FMath::Max(PeakBack, Recoil.GetKickBack());
		PeakFlip = FMath::Max(PeakFlip, Recoil.GetKickRotation().Pitch);
	}
	TestTrue(FString::Printf(TEXT("Gun kicks back about 3 cm (%.2f)"), PeakBack), FMath::IsNearlyEqual(PeakBack, 3.f, 0.5f));
	TestTrue(FString::Printf(TEXT("Muzzle flips up about 4 degrees (%.2f)"), PeakFlip), FMath::IsNearlyEqual(PeakFlip, 4.f, 0.7f));
	TestTrue(FString::Printf(TEXT("Aim kicked up (%.2f)"), Aim), Aim > 0.3f && Aim < 0.6f);

	// Then it all settles back.
	for (; Time < 2.f; Time += 0.01f)
	{
		Aim += Recoil.Tick(0.01f).Pitch;
	}
	TestTrue(TEXT("Gun back in place"), FMath::Abs(Recoil.GetKickBack()) < 0.05f && FMath::Abs(Recoil.GetKickRotation().Pitch) < 0.05f);
	TestTrue(FString::Printf(TEXT("Aim back where it was (%.3f)"), Aim), FMath::Abs(Aim) < 0.02f);
	TestTrue(TEXT("Fully settled"), Recoil.IsSettled());

	// Sustained fire climbs, but levels off instead of running away.
	Recoil.Reset();
	Aim = 0.f;
	for (int32 Shot = 0; Shot < 30; ++Shot)
	{
		Recoil.AddShot(Profile, Random);
		for (int32 Step = 0; Step < 9; ++Step)
		{
			Aim += Recoil.Tick(0.01f).Pitch;
		}
	}
	TestTrue(FString::Printf(TEXT("Full auto climbs a few degrees (%.2f)"), Aim), Aim > 1.f && Aim < 6.f);

	// Pulling down against the kick uses up its recovery: the aim doesn't then sink below where the player put it.
	Recoil.Reset();
	Recoil.AddShot(Profile, Random);
	Aim = 0.f;
	for (Time = 0.f; Time < 0.06f; Time += 0.005f)
	{
		Aim += Recoil.Tick(0.005f).Pitch;
	}
	const float Kicked = Aim;
	Aim += Recoil.Tick(0.005f, -Kicked).Pitch;
	for (Time = 0.f; Time < 2.f; Time += 0.01f)
	{
		Aim += Recoil.Tick(0.01f).Pitch;
	}
	TestTrue(FString::Printf(TEXT("A compensated kick isn't pulled back down again (%.3f of %.3f)"), Aim, Kicked), FMath::IsNearlyEqual(Aim, Kicked, 0.05f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponBulletsTest, "Looter.Weapons.Bullets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponBulletsTest::RunTest(const FString& Parameters)
{
	// Bullets are real: they take time to get there, hit what's in their path, and stop at their range.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("Bullet system exists"), Bullets))
	{
		return false;
	}

	// A wall 30 m down range (its face at 29.8 m).
	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Box->SetBoxExtent(FVector(20.f, 300.f, 300.f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Wall->SetRootComponent(Box);
	Box->RegisterComponent();
	Wall->SetActorLocation(FVector(3000.f, 0.f, 0.f));

	int32 Hits = 0;
	FVector HitPoint = FVector::ZeroVector;
	Bullets->OnBulletHit.AddLambda([&Hits, &HitPoint](const FHitResult& Hit, float, bool)
	{
		++Hits;
		HitPoint = Hit.ImpactPoint;
	});

	FBulletShot Shot;
	Shot.Start = FVector::ZeroVector;
	Shot.VisualStart = FVector(20.f, 15.f, -12.f);
	Shot.Direction = FVector::ForwardVector;
	Shot.Speed = 10000.f; // 100 m/s: 0.298 s to the wall
	Shot.Range = 10000.f;
	Shot.Channel = ECC_Visibility;

	// The tracer leaves from the muzzle and has joined the aim line by the time it's a few meters out.
	TestTrue(TEXT("Tracer starts at the muzzle"), UBulletSubsystem::TracerPoint(Shot, 0.f).Equals(Shot.VisualStart, 0.01));
	TestTrue(TEXT("Tracer joins the aim line"), UBulletSubsystem::TracerPoint(Shot, UBulletSubsystem::ConvergeDistance).Equals(FVector(UBulletSubsystem::ConvergeDistance, 0.f, 0.f), 0.01));

	Bullets->Fire(Shot);
	float Time = 0.f;
	for (; Time < 0.28f; Time += 1.f / 60.f)
	{
		Bullets->Tick(1.f / 60.f);
	}
	TestEqual(TEXT("No hit before the bullet gets there"), Hits, 0);
	TestEqual(TEXT("One bullet in flight"), Bullets->NumBulletsInFlight(), 1);
	for (; Time < 0.4f; Time += 1.f / 60.f)
	{
		Bullets->Tick(1.f / 60.f);
	}
	TestEqual(TEXT("It hits once it gets there"), Hits, 1);
	TestTrue(FString::Printf(TEXT("On the wall's face (%s)"), *HitPoint.ToCompactString()), FMath::IsNearlyEqual(HitPoint.X, 2980.0, 1.0));
	TestEqual(TEXT("Nothing left in flight"), Bullets->NumBulletsInFlight(), 0);
	TestTrue(TEXT("The impact threw particles"), Bullets->GetEffects().NumParticles() > 0);

	// Instant (hitscan) shots land the moment they're fired.
	Shot.Speed = 0.f;
	Bullets->Fire(Shot);
	TestEqual(TEXT("Instant shot lands immediately"), Hits, 2);

	// A miss flies out to its range and is gone.
	Shot.Speed = 10000.f;
	Shot.Range = 1000.f;
	Shot.Direction = FVector(0.f, 1.f, 0.f);
	Bullets->Fire(Shot);
	for (Time = 0.f; Time < 0.2f; Time += 1.f / 60.f)
	{
		Bullets->Tick(1.f / 60.f);
	}
	TestEqual(TEXT("The miss hit nothing"), Hits, 2);
	TestEqual(TEXT("The miss ran out of range"), Bullets->NumBulletsInFlight(), 0);
	return true;
}

#endif
