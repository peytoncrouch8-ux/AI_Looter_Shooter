#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Audio/LooterSoundCues.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/ViewKick.h"
#include "Tests/PlayerAnimTestKit.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponSounds.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The Drover, the revolver family (DA_Revolver, Art/Models/Weapons/Drover.py and .parts.csv): its definition against the
// rifle's and the shotgun's, its parts and their keys, its cylinder (the swing out on the crane, the turn a chamber per
// shot, the capacity it shows), its reload's choreography and sounds, its kick, its nicknames and the arms holding it out.
// The rules every family shares (picks, saved keys, caps) are Looter.Weapons.Parts.*, which take DA_Revolver too.

using namespace PlayerAnimTestKit;

namespace
{
	const TCHAR* const RevolverPath = TEXT("/Game/Weapons/Data/DA_Revolver.DA_Revolver");
	const TCHAR* const RevolverRiflePath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
	const TCHAR* const RevolverShotgunPath = TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun");
	const EWeaponRarity RevolverRarities[] = { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic,
		EWeaponRarity::Legendary };

	UWeaponDefinition* LoadRevolver()
	{
		return LoadObject<UWeaponDefinition>(nullptr, RevolverPath);
	}

	int32 RevolverSlot(const UWeaponDefinition& Definition, FName Slot)
	{
		return Definition.Parts.IndexOfByPredicate([Slot](const FWeaponPartSlot& Part) { return Part.Name == Slot; });
	}

	bool HasSocket(const UStaticMesh* Mesh, const TCHAR* Socket)
	{
		return Mesh && Mesh->FindSocket(FName(Socket)) != nullptr;
	}

	float RevolverCustomData(const UPrimitiveComponent* Part, int32 Index)
	{
		const TArray<float>& Data = Part->GetCustomPrimitiveData().Data;
		return Data.IsValidIndex(Index) ? Data[Index] : 0.f;
	}

	/** Every part range inside its stat's cap, so no part alone is worth more than the whole gun may have. */
	bool WithinCap(const FWeaponStatRange& Range, const WeaponParts::FCap& Cap)
	{
		return Range.Min >= Cap.Min && Range.Max <= Cap.Max;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRevolverDefinitionTest, "Looter.Weapons.Revolver.Definition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRevolverDefinitionTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Revolver = LoadRevolver();
	const UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, RevolverRiflePath);
	const UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, RevolverShotgunPath);
	if (!TestNotNull(TEXT("DA_Revolver (Tools/Unreal/create_revolver.py, then setup_gun_parts.py)"), Revolver) || !TestNotNull(TEXT("The rifle"), Rifle)
		|| !TestNotNull(TEXT("The shotgun"), Shotgun))
	{
		return false;
	}

	// What it is: a six-gun on pistol ammo, one heavy shot per pull, its cylinder the part a reload works.
	TestEqual(TEXT("A revolver"), Revolver->Kind, EWeaponKind::Revolver);
	TestEqual(TEXT("On pistol ammo"), Revolver->AmmoType, EAmmoType::Pistol);
	TestEqual(TEXT("Semi-automatic"), Revolver->FireMode, EWeaponFireMode::SemiAuto);
	TestEqual(TEXT("One bullet a shot"), Revolver->BaseStats.PelletsPerShot, 1);
	TestEqual(TEXT("Its reload works the cylinder"), Revolver->ReloadPart, EWeaponReloadPart::Cylinder);
	TestTrue(TEXT("Its cylinder is one of its slots"), RevolverSlot(*Revolver, Revolver->ReloadSlot) != INDEX_NONE
		&& Revolver->ReloadSlot == FName(TEXT("Cylinder")));

	// Against the other two: nearly three rifle bullets a shot at a fraction of the rifle's rate and twice the shotgun's,
	// the tightest cone, the quickest hands, held out where they tuck in.
	TestTrue(TEXT("A shot hits harder than two rifle bullets"), Revolver->BaseStats.Damage > Rifle->BaseStats.Damage * 2.f);
	TestTrue(TEXT("...but less than a shotgun blast"), Revolver->BaseStats.Damage < Shotgun->BaseStats.Damage * Shotgun->BaseStats.PelletsPerShot);
	TestTrue(TEXT("Slower than the rifle, quicker than the pump"), Revolver->BaseStats.FireRate < Rifle->BaseStats.FireRate
		&& Revolver->BaseStats.FireRate > Shotgun->BaseStats.FireRate);
	TestTrue(TEXT("The tightest cone of the three"), Revolver->BaseStats.Spread < Rifle->BaseStats.Spread && Revolver->BaseStats.Spread < Shotgun->BaseStats.Spread);
	TestTrue(TEXT("Its range between the shotgun's and the rifle's"), Revolver->BaseStats.Range > Shotgun->BaseStats.Range
		&& Revolver->BaseStats.Range < Rifle->BaseStats.Range);
	TestTrue(TEXT("Quicker in the hand than a long gun"), Revolver->BaseStats.Handling > Rifle->BaseStats.Handling
		&& Revolver->BaseStats.Handling > Shotgun->BaseStats.Handling);
	TestTrue(TEXT("Held out; the long guns aren't"), Revolver->HoldReach > 0.f && Rifle->HoldReach == 0.f && Shotgun->HoldReach == 0.f);
	// A sharp flip that settles fast: more than the rifle's, less than the shotgun's, on a quicker spring than either.
	TestTrue(TEXT("Its muzzle flips between the rifle's and the shotgun's"), Revolver->Recoil.MuzzleFlip > Rifle->Recoil.MuzzleFlip
		&& Revolver->Recoil.MuzzleFlip < Shotgun->Recoil.MuzzleFlip);
	TestTrue(TEXT("...and settles quicker than either"), Revolver->Recoil.Snappiness > Rifle->Recoil.Snappiness
		&& Revolver->Recoil.AimRecovery > Rifle->Recoil.AimRecovery && Revolver->Recoil.AimRecovery > Shotgun->Recoil.AimRecovery);

	// A six-gun holds what its cylinder shows: rarity never changes the count, and every rolled gun holds its cylinder's.
	for (const EWeaponRarity Rarity : RevolverRarities)
	{
		TestEqual(FString::Printf(TEXT("%s: rarity keeps the capacity"), *UEnum::GetValueAsString(Rarity)), Revolver->GetRarityInfo(Rarity).MagazineMultiplier, 1.f);
	}
	const int32 CylinderIndex = RevolverSlot(*Revolver, TEXT("Cylinder"));
	for (int32 Seed = 0; Seed < 200 && CylinderIndex != INDEX_NONE; ++Seed)
	{
		for (const EWeaponRarity Rarity : RevolverRarities)
		{
			const FWeaponLook Look = WeaponParts::Pick(*Revolver, Seed, Rarity);
			const FWeaponPartOption* Cylinder = Look.Parts.IsValidIndex(CylinderIndex) ? Look.Parts[CylinderIndex] : nullptr;
			const FWeaponStats Stats = UWeaponRollLibrary::ComputeStatsWithParts(Revolver, Rarity, 1, Seed, WeaponParts::PartKeys(Look));
			if (!Cylinder || Stats.MagazineSize != Cylinder->Stats.Magazine || Stats.MagazineSize < 5 || Stats.MagazineSize > 8)
			{
				AddError(FString::Printf(TEXT("Seed %d: holds %d, its cylinder %d"), Seed, Stats.MagazineSize, Cylinder ? Cylinder->Stats.Magazine : -1));
			}
		}
	}

	// Its parts: each with its own key, never a key another family's same slot uses (a part's key names it in saves); every
	// range inside its cap; the sockets the game reads; a tally row on every grip.
	for (const FWeaponPartSlot& Slot : Revolver->Parts)
	{
		for (const FWeaponPartOption& Option : Slot.Options)
		{
			const FString Name = FString::Printf(TEXT("%s %s"), *Slot.Name.ToString(), *Option.Key.ToString());
			for (const UWeaponDefinition* Other : { Rifle, Shotgun })
			{
				const int32 Same = RevolverSlot(*Other, Slot.Name);
				TestFalse(FString::Printf(TEXT("%s isn't %s's key too"), *Name, *Other->GetName()), Same != INDEX_NONE
					&& Other->Parts[Same].Options.ContainsByPredicate([&Option](const FWeaponPartOption& Theirs) { return Theirs.Key == Option.Key; }));
			}
			const FWeaponPartStats& Stats = Option.Stats;
			TestTrue(FString::Printf(TEXT("%s: every range inside its cap"), *Name), WithinCap(Stats.Damage, WeaponParts::DamageCap)
				&& WithinCap(Stats.Accuracy, WeaponParts::AccuracyCap) && WithinCap(Stats.Range, WeaponParts::RangeCap)
				&& WithinCap(Stats.FireRate, WeaponParts::FireRateCap) && WithinCap(Stats.Reload, WeaponParts::ReloadCap)
				&& WithinCap(Stats.Recoil, WeaponParts::RecoilCap) && WithinCap(Stats.Handling, WeaponParts::HandlingCap));
			const UStaticMesh* Mesh = Option.Mesh.Get();
			if (!TestNotNull(FString::Printf(TEXT("%s has a mesh"), *Name), Mesh))
			{
				continue;
			}
			if (Slot.Name == TEXT("Body"))
			{
				TestTrue(FString::Printf(TEXT("%s has the frame's sockets"), *Name), HasSocket(Mesh, TEXT("Barrel")) && HasSocket(Mesh, TEXT("Cylinder"))
					&& HasSocket(Mesh, TEXT("Sight")) && HasSocket(Mesh, TEXT("GripMount")));
			}
			else if (Slot.Name == TEXT("Barrel"))
			{
				TestTrue(FString::Printf(TEXT("%s has a muzzle"), *Name), HasSocket(Mesh, TEXT("Muzzle")));
			}
			else if (Slot.Name == TEXT("Cylinder"))
			{
				TestTrue(FString::Printf(TEXT("%s swings on a crane"), *Name), HasSocket(Mesh, TEXT("Crane")));
				TestTrue(FString::Printf(TEXT("%s sets the capacity"), *Name), Stats.Magazine >= 5 && Stats.Magazine <= 8);
			}
			else if (Slot.Name == TEXT("Sight"))
			{
				TestTrue(FString::Printf(TEXT("%s can be aimed through"), *Name), HasSocket(Mesh, TEXT("Aim")) && Stats.Zoom >= 1.f);
			}
			else if (Slot.Name == TEXT("Grip"))
			{
				TestTrue(FString::Printf(TEXT("%s has both hands"), *Name), HasSocket(Mesh, TEXT("Grip")) && HasSocket(Mesh, TEXT("Foregrip")));
				TestTrue(FString::Printf(TEXT("%s has a tally row"), *Name), UWeaponModelComponent::HasTallyRow(Mesh));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRevolverCylinderTest, "Looter.Weapons.Revolver.Cylinder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRevolverCylinderTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Revolver = LoadRevolver();
	FTestWorldWrapper WorldWrapper;
	if (!TestNotNull(TEXT("DA_Revolver"), Revolver) || !TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const int32 CylinderIndex = RevolverSlot(*Revolver, TEXT("Cylinder"));
	const int32 GripIndex = RevolverSlot(*Revolver, TEXT("Grip"));
	if (!TestTrue(TEXT("Cylinder and grip slots"), CylinderIndex != INDEX_NONE && GripIndex != INDEX_NONE))
	{
		return false;
	}

	// Every cylinder: it shows its own chambers, swings out to the left clear of the frame and home again, and turns a
	// chamber per shot about its own axis, swung out or not.
	for (const FWeaponPartOption& Option : Revolver->Parts[CylinderIndex].Options)
	{
		FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Revolver, EWeaponRarity::Legendary, 1);
		Gun.Parts[CylinderIndex] = Option.Key;
		AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, Gun, FTransform::Identity);
		UWeaponModelComponent* Model = Weapon ? Weapon->FindComponentByClass<UWeaponModelComponent>() : nullptr;
		const FString Name = Option.Key.ToString();
		if (!TestNotNull(*FString::Printf(TEXT("%s: a gun"), *Name), Model) || !TestTrue(FString::Printf(TEXT("%s: built"), *Name), Model->IsAssembled()))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s: its chambers"), *Name), Model->GetCylinderChambers(), Option.Stats.Magazine);
		TestEqual(FString::Printf(TEXT("%s: a reload works the cylinder"), *Name), Model->GetReloadPart(), EWeaponReloadPart::Cylinder);
		TestEqual(FString::Printf(TEXT("%s: as many rounds as chambers"), *Name), Weapon->GetStats().MagazineSize, Option.Stats.Magazine);
		UStaticMeshComponent* Cylinder = Model->GetParts()[CylinderIndex];
		const FTransform Rest = Cylinder->GetRelativeTransform();

		// Swung out, its center (its origin, on the frame's socket at the gun's middle) is far enough left that its body
		// clears the frame's side (1.3 cm out).
		Model->SetReloadTravel(LooterReload::CylinderOpenDegrees, true);
		const FVector Out = Cylinder->GetRelativeLocation();
		const float CylRadius = Option.Mesh ? static_cast<float>(Option.Mesh->GetBoundingBox().Max.Y) : 2.f;
		TestTrue(FString::Printf(TEXT("%s swings out to the left (%s)"), *Name, *Out.ToCompactString()), Out.Y - Rest.GetLocation().Y < -2.0);
		TestTrue(FString::Printf(TEXT("%s clears the frame's side (its near edge at %.2f cm)"), *Name, Out.Y + CylRadius), Out.Y + CylRadius < -1.3);
		Model->SetReloadTravel(0.f, true);
		TestTrue(FString::Printf(TEXT("%s back home"), *Name), Cylinder->GetRelativeTransform().Equals(Rest, 0.01));

		// A shot: it waits for the kick, then turns one chamber and stops on it.
		Model->TurnCylinder(1);
		TestTrue(TEXT("Turning after a shot"), Model->IsCylinderTurning());
		Model->UpdateCylinder(UWeaponModelComponent::CylinderTurnDelay * 0.5f);
		TestTrue(TEXT("...not before the kick"), FMath::IsNearlyZero(Model->GetCylinderTurn(), 0.001f));
		for (int32 Frame = 0; Frame < 30; ++Frame)
		{
			Model->UpdateCylinder(1.f / 60.f);
		}
		TestFalse(TEXT("...then still"), Model->IsCylinderTurning());
		const float Chamber = 360.f / static_cast<float>(Option.Stats.Magazine);
		const FQuat Turned = Cylinder->GetRelativeTransform().GetRotation();
		TestTrue(FString::Printf(TEXT("%s turned one chamber (%.1f degrees)"), *Name, FMath::RadiansToDegrees(Turned.GetAngle())),
			FMath::IsNearlyEqual(FMath::RadiansToDegrees(Turned.GetAngle()), Chamber, 0.5f) && FMath::Abs(Turned.GetRotationAxis().X) > 0.99f);
		TestTrue(FString::Printf(TEXT("%s turns about its own axis"), *Name), Cylinder->GetRelativeLocation().Equals(Rest.GetLocation(), 0.01));
		// Swung out mid-reload, the turn stays with it; a whole turn of the cylinder is where it started.
		Model->SetReloadTravel(LooterReload::CylinderOpenDegrees, true);
		TestTrue(TEXT("The swing doesn't depend on the turn"), Cylinder->GetRelativeLocation().Equals(Out, 0.01));
		Model->SetReloadTravel(0.f, true);
		Model->TurnCylinder(Option.Stats.Magazine - 1);
		Model->UpdateCylinder(1.f);
		TestTrue(TEXT("A whole turn comes round"), Cylinder->GetRelativeTransform().Equals(Rest, 0.01));
		Weapon->Destroy();
	}

	// The gun as the hands and eye find it, and its notches cut into whichever grip it has.
	for (const FWeaponPartOption& Option : Revolver->Parts[GripIndex].Options)
	{
		FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Revolver, EWeaponRarity::Legendary, 1);
		Gun.Parts[GripIndex] = Option.Key;
		Gun.Kills = 37;
		AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, Gun, FTransform::Identity);
		const UWeaponModelComponent* Model = Weapon ? Weapon->FindComponentByClass<UWeaponModelComponent>() : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s: a gun"), *Option.Key.ToString()), Model))
		{
			continue;
		}
		const FVector Grip = Model->GetGrip();
		const FVector Foregrip = Model->GetForegrip();
		const FVector Aim = Model->GetAimPoint();
		TestTrue(FString::Printf(TEXT("%s: muzzle ahead (%s)"), *Option.Key.ToString(), *Model->GetMuzzle().ToCompactString()), Model->GetMuzzle().X > 10.0);
		TestTrue(FString::Printf(TEXT("%s: the hand under and behind the frame (%s)"), *Option.Key.ToString(), *Grip.ToCompactString()), Grip.Z < -3.0 && Grip.X < 2.0);
		TestTrue(FString::Printf(TEXT("%s: the support hand wrapped round it from the left"), *Option.Key.ToString()), Foregrip.Y < Grip.Y - 2.0
			&& FVector::Dist(Foregrip, Grip) < 6.0);
		TestTrue(FString::Printf(TEXT("%s: aims over the frame on its middle (%s)"), *Option.Key.ToString(), *Aim.ToCompactString()), Aim.Z > 1.5
			&& FMath::Abs(Aim.Y) < 0.5 && Aim.X < Model->GetMuzzle().X);
		const UStaticMeshComponent* Tallied = Model->GetNotchPart();
		TestTrue(FString::Printf(TEXT("The tally is cut in %s"), *Option.Key.ToString()), Tallied && Tallied->GetStaticMesh() == Option.Mesh.Get());
		if (Tallied)
		{
			TestEqual(TEXT("37 kills: seven marks"), RevolverCustomData(Tallied, UWeaponModelComponent::NotchMarksDataIndex), 7.f);
		}
		Weapon->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRevolverReloadTest, "Looter.Weapons.Revolver.Reload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRevolverReloadTest::RunTest(const FString& Parameters)
{
	// Its sounds land on its motion, in order: the cylinder out, the empties punched out, the fresh rounds in, the cylinder
	// home; each has its own sound.
	const TConstArrayView<FReloadStepAt> Steps = LooterReload::Steps(EWeaponReloadPart::Cylinder);
	if (!TestEqual(TEXT("Four steps"), Steps.Num(), 4))
	{
		return false;
	}
	TestTrue(TEXT("Out, eject, rounds in, home"), Steps[0].Step == EReloadStep::CylinderOut && Steps[1].Step == EReloadStep::Eject
		&& Steps[2].Step == EReloadStep::RoundsIn && Steps[3].Step == EReloadStep::CylinderIn);
	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		TestTrue(TEXT("Each step inside the reload, after the one before"), Steps[Index].Progress > 0.f && Steps[Index].Progress < 1.f
			&& (Index == 0 || Steps[Index].Progress > Steps[Index - 1].Progress));
		TestFalse(TEXT("Each step has a sound"), WeaponSounds::ReloadStep(Steps[Index].Step).IsNone());
	}
	TestEqual(TEXT("The rod's punch is the revolver's"), WeaponSounds::ReloadStep(EReloadStep::Eject), FName(LooterSoundCue::Revolver::Eject));
	TestEqual(TEXT("The long guns' steps keep theirs"), WeaponSounds::ReloadStep(EReloadStep::Pump), FName(LooterSoundCue::ShotgunPump));

	// The swing: closed at both ends, all the way out (and no further than a touch past) while the empties go and the
	// rounds come in, home once it has latched; never a jump.
	TestEqual(TEXT("Closed at the start"), LooterReload::CylinderSwing(0.f), 0.f);
	TestEqual(TEXT("Closed at the end"), LooterReload::CylinderSwing(1.f), 0.f);
	TestTrue(TEXT("Out when the rod is punched"), LooterReload::CylinderSwing(Steps[1].Progress) >= LooterReload::CylinderOpenDegrees * 0.99f);
	TestTrue(TEXT("Out when the rounds go in"), LooterReload::CylinderSwing(Steps[2].Progress) >= LooterReload::CylinderOpenDegrees * 0.99f);
	TestTrue(TEXT("Home once latched"), LooterReload::CylinderSwing(Steps[3].Progress + 0.01f) == 0.f);
	float Most = 0.f;
	float BiggestStep = 0.f;
	for (int32 Step = 0; Step < 400; ++Step)
	{
		const float Swing = LooterReload::CylinderSwing(Step / 400.f);
		Most = FMath::Max(Most, Swing);
		BiggestStep = FMath::Max(BiggestStep, FMath::Abs(LooterReload::CylinderSwing((Step + 1) / 400.f) - Swing));
	}
	TestTrue(FString::Printf(TEXT("Swings fully out, a touch past at most (%.1f degrees)"), Most), Most >= LooterReload::CylinderOpenDegrees
		&& Most <= LooterReload::CylinderOpenDegrees * 1.1f);
	TestTrue(FString::Printf(TEXT("Never a jump (%.1f degrees in a 400th of the reload)"), BiggestStep), BiggestStep < 12.f);

	// The first-person gun: in its hold pose at both ends, worked in front of the chest between, still when not reloading.
	FVector Offset;
	FRotator Rotation;
	LooterReload::ViewModelPose(EWeaponReloadPart::Cylinder, 0.f, Offset, Rotation);
	TestTrue(TEXT("Starts in the hold pose"), Offset.IsNearlyZero(0.01) && Rotation.IsNearlyZero(0.01f));
	LooterReload::ViewModelPose(EWeaponReloadPart::Cylinder, 1.f, Offset, Rotation);
	TestTrue(TEXT("Ends in the hold pose"), Offset.IsNearlyZero(0.01) && Rotation.IsNearlyZero(0.01f));
	LooterReload::ViewModelPose(EWeaponReloadPart::Cylinder, 0.5f, Offset, Rotation);
	TestTrue(TEXT("Brings the gun in"), !Rotation.IsNearlyZero(5.f));
	LooterReload::ViewModelPose(EWeaponReloadPart::Cylinder, Steps[1].Progress, Offset, Rotation);
	TestTrue(FString::Printf(TEXT("Muzzle up to punch the rod (%.1f degrees)"), Rotation.Pitch), Rotation.Pitch > 30.f);
	LooterReload::ViewModelPose(EWeaponReloadPart::Cylinder, -1.f, Offset, Rotation);
	TestTrue(TEXT("No motion when not reloading"), Offset.IsZero() && Rotation.IsZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRevolverFeelTest, "Looter.Weapons.Revolver.Feel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRevolverFeelTest::RunTest(const FString& Parameters)
{
	// Its shot's kick on the view: harder than the rifle's tick, softer than the shotgun's shove, and quicker to settle.
	FRandomStream Random(11);
	const FViewKick Revolver = ViewKicks::ForShot(EWeaponKind::Revolver, 1.f, 0.f, Random);
	const FViewKick Rifle = ViewKicks::ForShot(EWeaponKind::Rifle, 1.f, 0.f, Random);
	const FViewKick Shotgun = ViewKicks::ForShot(EWeaponKind::Shotgun, 1.f, 0.f, Random);
	TestTrue(TEXT("A sharper snap than the rifle's"), Revolver.Pitch > Rifle.Pitch * 2.f);
	TestTrue(TEXT("Less than the shotgun's"), Revolver.Pitch < Shotgun.Pitch && Revolver.FieldOfView < Shotgun.FieldOfView);
	TestTrue(TEXT("Settles faster than the shotgun's"), Revolver.Frequency * Revolver.Damping > Shotgun.Frequency * Shotgun.Damping);

	// Its own sounds, and the others' unchanged.
	TestEqual(TEXT("Its shot"), WeaponSounds::Fire(EWeaponKind::Revolver), FName(LooterSoundCue::Revolver::Fire));
	TestEqual(TEXT("The rifle's"), WeaponSounds::Fire(EWeaponKind::Rifle), FName(LooterSoundCue::RifleFire));
	TestEqual(TEXT("The shotgun's"), WeaponSounds::Fire(EWeaponKind::Shotgun), FName(LooterSoundCue::ShotgunFire));
	TestEqual(TEXT("A gun of no kind sounds like the rifle"), WeaponSounds::Fire(EWeaponKind::None), FName(LooterSoundCue::RifleFire));
	TestEqual(TEXT("Its click on a spent chamber"), WeaponSounds::DryFire(EWeaponKind::Revolver), FName(LooterSoundCue::Revolver::DryFire));
	TestEqual(TEXT("The long guns' click"), WeaponSounds::DryFire(EWeaponKind::Shotgun), FName(LooterSoundCue::DryFire));
	TestEqual(TEXT("Drawn from a holster"), WeaponSounds::Equip(EWeaponKind::Revolver), FName(LooterSoundCue::Revolver::Equip));
	TestEqual(TEXT("Long guns swing off the back"), WeaponSounds::Equip(EWeaponKind::Rifle), FName(LooterSoundCue::Equip));

	// Its nicknames: a dozen or more, every one its own and none another kind's.
	const TConstArrayView<const TCHAR*> Names = WeaponNotches::Nicknames(EWeaponKind::Revolver);
	TestTrue(TEXT("A dozen revolver nicknames"), Names.Num() >= 12);
	TSet<FString> Others;
	for (const EWeaponKind Kind : { EWeaponKind::Rifle, EWeaponKind::Shotgun })
	{
		for (const TCHAR* Name : WeaponNotches::Nicknames(Kind))
		{
			Others.Add(Name);
		}
	}
	TSet<FString> Seen;
	for (const TCHAR* Name : Names)
	{
		TestFalse(FString::Printf(TEXT("%s is the revolver's alone"), Name), FString(Name).IsEmpty() || Seen.Contains(Name) || Others.Contains(Name));
		Seen.Add(Name);
	}

	// Held out: the stance layer pushes the right hand (the gun in it) forward along the barrel; a sprint or a reload
	// takes the reach away.
	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	FLooterStanceInput Held;
	Held.bHandOnForegrip = true;
	Held.HoldBone = TEXT("hand_r");
	Held.WeaponRotation = FRotator(0.f, 90.f, 0.f).Quaternion();   // the mannequin faces +Y in its component space
	FCompactPose Tucked = Rig.RefPose();
	LooterStancePose::Apply(Tucked, Held);
	Held.HoldReach = 14.f;
	FCompactPose Reached = Rig.RefPose();
	LooterStancePose::Apply(Reached, Held);
	TestTrue(TEXT("Held out: sound"), FPoseRig::IsSound(Reached));
	const float Forward = static_cast<float>(Rig.Where(Reached, TEXT("hand_r")).Y - Rig.Where(Tucked, TEXT("hand_r")).Y);
	TestTrue(FString::Printf(TEXT("The right hand reaches out (%.1f cm)"), Forward), Forward > 8.f);
	Held.SprintAlpha = 1.f;
	FCompactPose Sprinting = Rig.RefPose();
	LooterStancePose::Apply(Sprinting, Held);
	TestTrue(TEXT("No reach in a sprint"), Rig.Where(Sprinting, TEXT("hand_r")).Equals(Rig.Where(Tucked, TEXT("hand_r")), 0.5));
	return true;
}

#endif
