#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/ImpactFlashSubsystem.h"
#include "Weapons/VfxLightRules.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponBase.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The gun lights the hands and the ground (Phase 3, lane MUZZLE, 2026-11-03; the audit's change 8): the shot light's per-kind
// numbers, how long it shows and where it hangs, the aimed flash's size, the flash a hit throws on stone, metal and wood (its
// surfaces, the eye's distance rule, the round's heft) and the four-light pool's cap, all as plain data with the cvars'
// defaults (a default-made FVfxTuning), never what the user has set. What the light looks like is judged in play. (Split from
// VfxTests.cpp, which was at its 500 lines.)

namespace
{
	const EWeaponKind LightKinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Sniper,
		EWeaponKind::LMG, EWeaponKind::Launcher };

	FString KindName(EWeaponKind Kind)
	{
		return FString::Printf(TEXT("Kind %d"), static_cast<int32>(Kind));
	}

	bool IsLitSurface(EImpactMaterial Material)
	{
		return Material == EImpactMaterial::Stone || Material == EImpactMaterial::Metal || Material == EImpactMaterial::SheetMetal
			|| Material == EImpactMaterial::Wood;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxMuzzleLightRulesTest, "Looter.Vfx.Light.Muzzle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxMuzzleLightRulesTest::RunTest(const FString& Parameters)
{
	const FVfxTuning Tuning;
	// The audit's numbers: 24 cd / 3.2 m (invisible in daylight) become ~400 cd / 5 m for ~40 ms.
	const FVfxLightShape Rifle = VfxRules::MuzzleLightShape(EWeaponKind::Rifle, false, Tuning);
	TestEqual(TEXT("A rifle's shot light peaks at 400 cd"), Rifle.Candelas, 400.f);
	TestEqual(TEXT("...and reaches 5 m"), Rifle.RadiusCm, 500.f);
	TestEqual(TEXT("...for 40 ms"), Tuning.MuzzleLightSeconds, 0.04f);
	TestTrue(TEXT("...far past the 24 cd / 3.2 m it was"), Rifle.Candelas >= 24.f * 16.f && Rifle.RadiusCm > 320.f);
	TestEqual(TEXT("A gun with no kind is a rifle's"), VfxRules::MuzzleLightShape(EWeaponKind::None, false, Tuning).Candelas, Rifle.Candelas);

	// By kind: the shotgun's and the launcher's bigger, the SMG's smaller; every kind a light, its glow edge alike.
	for (const EWeaponKind Kind : LightKinds)
	{
		const FVfxLightShape Shape = VfxRules::MuzzleLightShape(Kind, false, Tuning);
		TestTrue(KindName(Kind) + TEXT(": a light with a strength and a reach"), Shape.Candelas > 0.f && Shape.RadiusCm > 0.f);
		const float Edge = Shape.Candelas / FMath::Square(Shape.RadiusCm);
		TestEqual(KindName(Kind) + TEXT(": the same strength at its edge as the rifle's (reach by the root of the share)"), Edge,
			Rifle.Candelas / FMath::Square(Rifle.RadiusCm), 0.0001f);
	}
	const FVfxLightShape Shotgun = VfxRules::MuzzleLightShape(EWeaponKind::Shotgun, false, Tuning);
	const FVfxLightShape Launcher = VfxRules::MuzzleLightShape(EWeaponKind::Launcher, false, Tuning);
	const FVfxLightShape Smg = VfxRules::MuzzleLightShape(EWeaponKind::SMG, false, Tuning);
	TestTrue(TEXT("The shotgun's and the launcher's lights are stronger and reach farther than the rifle's"), Shotgun.Candelas > Rifle.Candelas
		&& Launcher.Candelas > Rifle.Candelas && Shotgun.RadiusCm > Rifle.RadiusCm && Launcher.RadiusCm > Rifle.RadiusCm);
	TestTrue(TEXT("The SMG's (a dozen shots a second) is the smallest"), Smg.Candelas < Rifle.Candelas && Smg.RadiusCm < Rifle.RadiusCm);
	for (const EWeaponKind Kind : LightKinds)
	{
		TestTrue(KindName(Kind) + TEXT(": none weaker than the SMG's"), VfxRules::MuzzleLightShape(Kind, false, Tuning).Candelas >= Smg.Candelas);
	}
	TestEqual(TEXT("The sniper's reach is the root of its share"), VfxRules::MuzzleLightShape(EWeaponKind::Sniper, false, Tuning).RadiusCm,
		Tuning.MuzzleLightRadius * FMath::Sqrt(Tuning.MuzzleLightSniper), 0.01f);

	// The switches: the light 0, one kind's share 0, the first-person share.
	FVfxTuning Off = Tuning;
	Off.MuzzleLight = 0.f;
	TestEqual(TEXT("Looter.Vfx.Muzzle.Light 0: no light"), VfxRules::MuzzleLightShape(EWeaponKind::Rifle, false, Off).Candelas, 0.f);
	FVfxTuning NoSmg = Tuning;
	NoSmg.MuzzleLightSMG = 0.f;
	TestEqual(TEXT("A kind's share 0: that kind has none"), VfxRules::MuzzleLightShape(EWeaponKind::SMG, false, NoSmg).Candelas, 0.f);
	TestEqual(TEXT("...the others keep theirs"), VfxRules::MuzzleLightShape(EWeaponKind::Rifle, false, NoSmg).Candelas, 400.f);
	FVfxTuning Hands = Tuning;
	Hands.MuzzleLight1P = 0.5f;
	TestEqual(TEXT("The player's own view takes its share (the hands are close)"), VfxRules::MuzzleLightShape(EWeaponKind::Rifle, true, Hands).Candelas, 200.f);
	TestEqual(TEXT("...a body seen from behind doesn't"), VfxRules::MuzzleLightShape(EWeaponKind::Rifle, false, Hands).Candelas, 400.f);
	TestEqual(TEXT("By default both views are alike"), VfxRules::MuzzleLightShape(EWeaponKind::Rifle, true, Tuning).Candelas, 400.f);

	// How long: 40 ms, never longer than the flash it rides (the gun ticks only while the flash shows).
	TestEqual(TEXT("A 40 ms flash: a 40 ms light"), VfxRules::MuzzleLightLife(0.04f, Tuning), 0.04f);
	TestEqual(TEXT("A longer flash (the launcher's 60 ms): still 40"), VfxRules::MuzzleLightLife(0.06f, Tuning), 0.04f);
	TestEqual(TEXT("A shorter one (the SMG's 30 ms) cuts it short"), VfxRules::MuzzleLightLife(0.03f, Tuning), 0.03f);
	TestEqual(TEXT("No flash, no light"), VfxRules::MuzzleLightLife(0.f, Tuning), 0.f);
	for (const EWeaponKind Kind : LightKinds)
	{
		const float Life = VfxRules::MuzzleLightLife(VfxRules::MuzzleLook(Kind).Duration * Tuning.MuzzleTime, Tuning);
		TestTrue(KindName(Kind) + FString::Printf(TEXT(": its light shows 30 to 40 ms (%.0f)"), Life * 1000.f), Life >= 0.0299f && Life <= 0.0401f);
	}
	TestEqual(TEXT("The light fades full at once..."), VfxRules::MuzzleFade(0.f, 0.04f, Tuning.MuzzleFadePower), 1.f);
	TestEqual(TEXT("...and is gone at its end"), VfxRules::MuzzleFade(0.04f, 0.04f, Tuning.MuzzleFadePower), 0.f);

	// Where: the muzzle pushed 10 cm out along the barrel, never inside the gun.
	const FVector Muzzle(100.0, 50.0, 20.0);
	TestEqual(TEXT("Pushed 10 cm out"), Tuning.MuzzleLightPush, 10.f);
	const FVector Along = VfxRules::MuzzleLightPlace(Muzzle, FVector(3.0, 0.0, 0.0), Tuning.MuzzleLightPush);
	TestTrue(TEXT("...along the barrel, whatever length the vector has"), Along.Equals(FVector(110.0, 50.0, 20.0), 0.001));
	const FVector Diagonal = VfxRules::MuzzleLightPlace(Muzzle, FVector(1.0, 1.0, 0.0), Tuning.MuzzleLightPush);
	TestEqual(TEXT("...exactly that far from the muzzle"), static_cast<float>(FVector::Dist(Diagonal, Muzzle)), 10.f, 0.001f);
	TestTrue(TEXT("...and never back toward the gun"), FVector::DotProduct(Diagonal - Muzzle, FVector(1.0, 1.0, 0.0)) > 0.0);
	TestTrue(TEXT("A barrel with no direction leaves it at the muzzle"), VfxRules::MuzzleLightPlace(Muzzle, FVector::ZeroVector, 10.f).Equals(Muzzle, 0.001));

	// The aimed flash: 0.55 -> 0.8 of the size (still shrunk off the crosshair by the clearance, not here).
	TestEqual(TEXT("Aimed, the flash is 0.8 of its size"), Tuning.MuzzleAim, 0.8f);
	TestEqual(TEXT("...in the view's size"), VfxRules::MuzzleView(true, 1.f, Tuning).Size, 0.8f, 0.0001f);
	TestEqual(TEXT("...from the hip a full-size one"), VfxRules::MuzzleView(true, 0.f, Tuning).Size, 1.f, 0.0001f);
	TestEqual(TEXT("...seen from behind, unchanged"), VfxRules::MuzzleView(false, 1.f, Tuning).Size, 1.f, 0.0001f);
	TestTrue(TEXT("The crosshair's clearance stays, tighter aimed"), Tuning.MuzzleAimClear > 0.f && Tuning.MuzzleAimClear < Tuning.MuzzleClear);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxMuzzleLightComponentTest, "Looter.Vfx.Light.MuzzleComponent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxMuzzleLightComponentTest::RunTest(const FString& Parameters)
{
	// The gun's own light as built: movable (each shot hangs it again), never shadowed, in candelas, resting at the 5 m reach.
	UObject* Gun = AWeaponBase::StaticClass()->GetDefaultObject();
	const UPointLightComponent* Light = Cast<UPointLightComponent>(Gun->GetDefaultSubobjectByName(TEXT("MuzzleLight")));
	if (!TestNotNull(TEXT("The gun has a muzzle light"), Light))
	{
		return false;
	}
	TestFalse(TEXT("It casts no shadows"), static_cast<bool>(Light->CastShadows));
	TestTrue(TEXT("It is movable"), Light->Mobility == EComponentMobility::Movable);
	TestTrue(TEXT("It is in candelas"), Light->IntensityUnits == ELightUnits::Candelas);
	TestEqual(TEXT("It rests at the 5 m reach"), Light->AttenuationRadius, 500.f);
	TestFalse(TEXT("It starts hidden"), Light->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxImpactLightRulesTest, "Looter.Vfx.Light.Impact", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxImpactLightRulesTest::RunTest(const FString& Parameters)
{
	const FVfxTuning Tuning;
	// The audit's numbers: ~150 cd for 60 ms, four pooled lights.
	TestEqual(TEXT("An impact flash peaks at 150 cd"), Tuning.ImpactLight, 150.f);
	TestEqual(TEXT("...for 60 ms"), Tuning.ImpactLightSeconds, 0.06f);
	TestEqual(TEXT("...from a pool of four lights"), Tuning.ImpactLightMax, 4);
	TestEqual(TEXT("...the pool's own size"), FImpactFlashPool::MaxSlots, 4);
	TestEqual(TEXT("...never nearer than a metre to the eye"), Tuning.ImpactLightEye, 100.f);

	// Surfaces: stone, metal and wood flash (sheet metal is metal); soft ground, water, cloth, hay, glass, bone, gravel and the unknown don't.
	for (uint8 Index = 0; Index < static_cast<uint8>(EImpactMaterial::Count); ++Index)
	{
		const EImpactMaterial Material = static_cast<EImpactMaterial>(Index);
		TestTrue(FString::Printf(TEXT("Surface %s: flashes only if stone, metal or wood"), ImpactSurface::Name(Material)),
			(VfxRules::ImpactLightSurface(Material) > 0.f) == IsLitSurface(Material));
	}
	TestTrue(TEXT("Stone and metal take it whole, wood the least"), VfxRules::ImpactLightSurface(EImpactMaterial::Stone) == 1.f
		&& VfxRules::ImpactLightSurface(EImpactMaterial::Metal) == 1.f
		&& VfxRules::ImpactLightSurface(EImpactMaterial::Wood) < VfxRules::ImpactLightSurface(EImpactMaterial::SheetMetal)
		&& VfxRules::ImpactLightSurface(EImpactMaterial::SheetMetal) < 1.f);
	for (const EImpactMaterial Material : { EImpactMaterial::Stone, EImpactMaterial::Metal, EImpactMaterial::SheetMetal, EImpactMaterial::Wood })
	{
		const FLinearColor Color = VfxRules::ImpactLightColor(Material);
		TestTrue(FString::Printf(TEXT("%s: a warm flash, red over green over blue"), ImpactSurface::Name(Material)),
			Color.R >= Color.G && Color.G > Color.B && Color.R == 1.f);
	}

	// A rifle round (weight 1.1) on stone, well away from the eye: the whole 150 cd, 3 m.
	const float Far = 500.f;
	const FVfxLightShape Rifle = VfxRules::ImpactLightShape(EImpactMaterial::Stone, VfxRules::ImpactLightWeightRef, Far, Tuning);
	TestEqual(TEXT("A rifle round on stone: 150 cd"), Rifle.Candelas, 150.f, 0.001f);
	TestEqual(TEXT("...3 m"), Rifle.RadiusCm, 300.f);
	TestEqual(TEXT("Wood flashes dimmer"), VfxRules::ImpactLightShape(EImpactMaterial::Wood, VfxRules::ImpactLightWeightRef, Far, Tuning).Candelas,
		150.f * 0.55f, 0.001f);
	TestEqual(TEXT("Dirt not at all"), VfxRules::ImpactLightShape(EImpactMaterial::Dirt, VfxRules::ImpactLightWeightRef, Far, Tuning).Candelas, 0.f);

	// The eye: nothing at a metre and nearer, easing in to full at two; an unknown eye (no camera yet) is no limit.
	const float Weight = VfxRules::ImpactLightWeightRef;
	TestEqual(TEXT("50 cm from the eye: no light"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, 50.f, Tuning).Candelas, 0.f);
	TestEqual(TEXT("99 cm: none"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, 99.f, Tuning).Candelas, 0.f);
	TestEqual(TEXT("A metre exactly: none"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, 100.f, Tuning).Candelas, 0.f);
	TestEqual(TEXT("1.5 m: half"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, 150.f, Tuning).Candelas, 75.f, 0.001f);
	TestEqual(TEXT("2 m: all of it"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, 200.f, Tuning).Candelas, 150.f, 0.001f);
	TestEqual(TEXT("No eye known yet: all of it"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, -1.f, Tuning).Candelas, 150.f, 0.001f);
	float Last = -1.f;
	bool bRises = true;
	for (float Distance = 0.f; Distance <= 400.f; Distance += 10.f)
	{
		const float Share = VfxRules::ImpactLightEyeShare(Distance, Tuning);
		bRises &= Share >= Last && Share >= 0.f && Share <= 1.f;
		Last = Share;
	}
	TestTrue(TEXT("The eye's share only rises with distance, 0 to 1"), bRises);
	FVfxTuning NoEyeRule = Tuning;
	NoEyeRule.ImpactLightEye = 0.f;
	TestEqual(TEXT("Looter.Vfx.Impact.LightEye 0: no rule"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, 10.f, NoEyeRule).Candelas, 150.f, 0.001f);

	// Far hits are a few pixels of wall: no light past Looter.Vfx.Impact.LightFar.
	TestTrue(TEXT("At the far edge: lit"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, Tuning.ImpactLightFar, Tuning).Candelas > 0.f);
	TestEqual(TEXT("Past it: none"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, Tuning.ImpactLightFar + 1.f, Tuning).Candelas, 0.f);

	// The round's heft: a sniper's more (to 1.5x), an SMG's and a pellet's less (to half).
	const float Sniper = VfxRules::ImpactLightShape(EImpactMaterial::Stone, 2.05f, Far, Tuning).Candelas;
	const float Pellet = VfxRules::ImpactLightShape(EImpactMaterial::Stone, 0.76f, Far, Tuning).Candelas;
	TestEqual(TEXT("A sniper's heft is capped at 1.5x"), Sniper, 225.f, 0.001f);
	TestTrue(TEXT("A pellet's is under the rifle's, over half of it"), Pellet < 150.f && Pellet > 75.f);
	TestEqual(TEXT("Nothing is lighter than half"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, 0.05f, Far, Tuning).Candelas, 75.f, 0.001f);

	// The switches.
	FVfxTuning Off = Tuning;
	Off.ImpactLight = 0.f;
	TestEqual(TEXT("Looter.Vfx.Impact.Light 0: none"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, Far, Off).Candelas, 0.f);
	FVfxTuning NoPool = Tuning;
	NoPool.ImpactLightMax = 0;
	TestEqual(TEXT("Looter.Vfx.Impact.LightMax 0: none"), VfxRules::ImpactLightShape(EImpactMaterial::Stone, Weight, Far, NoPool).Candelas, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxImpactLightPoolTest, "Looter.Vfx.Light.ImpactPool", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxImpactLightPoolTest::RunTest(const FString& Parameters)
{
	FVfxLightShape Bright;
	Bright.Candelas = 150.f;
	Bright.RadiusCm = 300.f;
	FVfxLightShape Brighter = Bright;
	Brighter.Candelas = 200.f;
	FVfxLightShape Dim = Bright;
	Dim.Candelas = 60.f;
	constexpr float Life = 0.06f;
	constexpr float Reach = 150.f;

	// Four distinct spots light four; the fifth is refused when it is no brighter, and takes over the weakest when it is.
	FImpactFlashPool Pool;
	TArray<int32> Taken;
	for (int32 Spot = 0; Spot < FImpactFlashPool::MaxSlots; ++Spot)
	{
		const int32 Slot = Pool.Start(FVector(Spot * 1000.0, 0.0, 0.0), Bright, Life, 4, Reach, 10);
		TestTrue(FString::Printf(TEXT("Spot %d takes a light"), Spot), Slot != INDEX_NONE && !Taken.Contains(Slot));
		Taken.Add(Slot);
	}
	TestEqual(TEXT("Four lit"), Pool.NumLive(), 4);
	TestEqual(TEXT("A fifth, no brighter, is refused"), Pool.Start(FVector(5000.0, 0.0, 0.0), Bright, Life, 4, Reach, 10), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("...a dimmer one too"), Pool.Start(FVector(5000.0, 0.0, 0.0), Dim, Life, 4, Reach, 10), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Still four"), Pool.NumLive(), 4);
	// Let one fade a while, then a new spot takes over the weakest (the oldest).
	Pool.Advance(0.02f, 11);
	Pool.Slots[2].Age = 0.04f;
	const int32 Replaced = Pool.Start(FVector(6000.0, 0.0, 0.0), Bright, Life, 4, Reach, 12);
	TestEqual(TEXT("A new spot takes the weakest light (the oldest flash)"), Replaced, 2);
	TestEqual(TEXT("...and the pool stays four"), Pool.NumLive(), 4);

	// Looter.Vfx.Impact.LightMax: fewer lit at once.
	FImpactFlashPool Two;
	for (int32 Spot = 0; Spot < 6; ++Spot)
	{
		Two.Start(FVector(Spot * 1000.0, 0.0, 0.0), Spot < 2 ? Bright : Brighter, Life, 2, Reach, 20);
		TestTrue(FString::Printf(TEXT("Max 2: %d lit after %d hits"), Two.NumLive(), Spot + 1), Two.NumLive() <= 2);
	}
	FImpactFlashPool None;
	TestEqual(TEXT("Max 0: none lit"), None.Start(FVector::ZeroVector, Bright, Life, 0, Reach, 1), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("No strength: none lit"), None.Start(FVector::ZeroVector, FVfxLightShape(), Life, 4, Reach, 1), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("No life: none lit"), None.Start(FVector::ZeroVector, Bright, 0.f, 4, Reach, 1), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("...and nothing in the pool"), None.NumLive(), 0);

	// A hit beside a live flash takes that light (a stream into one wall, a blast's pellets): one light, restarted full.
	FImpactFlashPool Merge;
	const int32 First = Merge.Start(FVector(0.0, 0.0, 0.0), Bright, Life, 4, Reach, 30);
	Merge.Advance(0.03f, 31);
	const float Aged = Merge.StrengthOf(First);
	TestTrue(TEXT("Half way through its life the flash is a quarter bright"), FMath::IsNearlyEqual(Aged, 150.f * 0.25f, 0.01f));
	TestEqual(TEXT("A hit 1 m off takes the same light"), Merge.Start(FVector(100.0, 0.0, 0.0), Bright, Life, 4, Reach, 32), First);
	TestEqual(TEXT("...one light, not two"), Merge.NumLive(), 1);
	TestEqual(TEXT("...restarted full"), Merge.StrengthOf(First), 150.f, 0.001f);
	TestTrue(TEXT("...moved to the new hit"), Merge.Slots[First].Place.Equals(FVector(100.0, 0.0, 0.0), 0.001));
	TestEqual(TEXT("A dimmer hit beside a brighter flash adds nothing"), Merge.Start(FVector(50.0, 0.0, 0.0), Dim, Life, 4, Reach, 33),
		static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("...and leaves it be"), Merge.StrengthOf(First), 150.f, 0.001f);
	const int32 Far = Merge.Start(FVector(400.0, 0.0, 0.0), Bright, Life, 4, Reach, 34);
	TestTrue(TEXT("A hit past the reach is its own light"), Far != INDEX_NONE && Far != First && Merge.NumLive() == 2);

	// Time: not aged on the frame it started (every flash is seen once at full), the curve (1 - share)^2, gone at its life.
	FImpactFlashPool Clock;
	const int32 Slot = Clock.Start(FVector::ZeroVector, Bright, Life, 4, Reach, 50);
	TestEqual(TEXT("Full when it starts"), Clock.StrengthOf(Slot), 150.f, 0.001f);
	TestEqual(TEXT("A tick on its own frame ages nothing"), static_cast<int32>(Clock.Advance(0.05f, 50)), 0);
	TestEqual(TEXT("...still full"), Clock.StrengthOf(Slot), 150.f, 0.001f);
	TestEqual(TEXT("A tick next frame, half its life on"), static_cast<int32>(Clock.Advance(0.03f, 51)), 0);
	TestEqual(TEXT("...a quarter"), Clock.StrengthOf(Slot), 37.5f, 0.001f);
	TestEqual(TEXT("It ends at its life: its bit is set"), static_cast<int32>(Clock.Advance(0.03f, 52)), 1 << Slot);
	TestEqual(TEXT("...nothing lit"), Clock.NumLive(), 0);
	TestEqual(TEXT("...and no strength"), Clock.StrengthOf(Slot), 0.f);
	TestEqual(TEXT("A freed pool ends nothing again"), static_cast<int32>(Clock.Advance(0.1f, 53)), 0);

	// Whatever the fire: never more than the pool lit, never more than Max (a stream of 500 hits over a wall).
	FImpactFlashPool Stream;
	FRandomStream Random(7);
	bool bCapped = true;
	for (int32 Hit = 0; Hit < 500; ++Hit)
	{
		FVfxLightShape Shape = Bright;
		Shape.Candelas = Random.FRandRange(40.f, 260.f);
		Stream.Start(FVector(Random.FRandRange(-2000.f, 2000.f), Random.FRandRange(-2000.f, 2000.f), 0.0), Shape, Life, 3, Reach, static_cast<uint64>(Hit));
		bCapped &= Stream.NumLive() <= 3;
		if (Hit % 3 == 0)
		{
			Stream.Advance(0.016f, static_cast<uint64>(Hit) + 1);
		}
	}
	TestTrue(TEXT("500 hits at 60 fps never light more than Max"), bCapped);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVfxImpactLightWorldTest, "Looter.Vfx.Light.ImpactWorld", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVfxImpactLightWorldTest::RunTest(const FString& Parameters)
{
	// A test level (an editor preview world) gets no lights: the subsystem is there, asleep, and a hit lights nothing.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UImpactFlashSubsystem* Flashes = UImpactFlashSubsystem::Get(World);
	if (!TestNotNull(TEXT("The impact flash system"), Flashes))
	{
		return false;
	}
	const FVfxTuning Tuning;
	const FVector Eye(-600.0, 0.0, 100.0);
	TestFalse(TEXT("A hit on stone in a test world lights nothing"),
		UImpactFlashSubsystem::Flash(World, Tuning, FVector(0.0, 0.0, 0.0), FVector::UpVector, EImpactMaterial::Stone, 1.1f, &Eye));
	TestEqual(TEXT("...nothing lit"), Flashes->NumLit(), 0);
	TestFalse(TEXT("...and the system sleeps"), Flashes->IsTickable());
	TestFalse(TEXT("No world, no light"), UImpactFlashSubsystem::Flash(nullptr, Tuning, FVector::ZeroVector, FVector::UpVector, EImpactMaterial::Stone, 1.1f, nullptr));
	return true;
}

#endif
