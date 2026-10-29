#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HUD/HudCrosshairRules.h"
#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Weapons/WeaponTypes.h"

// The crosshair per gun kind (the user's pick, 2026-10-29, Docs/Work/2026-10-28-Core/crosshair): every kind's shape, its
// kick, and the colour setting, checked as plain data (HudCrosshairRules) with no world.

namespace
{
	/** Two shapes drawn alike: the same lines, rings and dot. */
	bool SameShape(const HudCrosshair::FShape& A, const HudCrosshair::FShape& B)
	{
		if (A.Strokes.Num() != B.Strokes.Num() || A.Rings.Num() != B.Rings.Num() || !FMath::IsNearlyEqual(A.Dot.Radius, B.Dot.Radius))
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Strokes.Num(); ++Index)
		{
			if (A.Strokes[Index].Points != B.Strokes[Index].Points)
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudCrosshairShapesTest, "Looter.UI.Crosshair.Shapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudCrosshairShapesTest::RunTest(const FString& Parameters)
{
	using namespace HudCrosshair;
	const FShape& Plain = ShapeFor(EWeaponKind::None);
	TestEqual(TEXT("No gun: the old cross's four ticks"), Plain.Strokes.Num(), 4);
	TestTrue(TEXT("...with its dot from the hip"), Plain.bHipDot);
	TestTrue(TEXT("...at rest exactly the old one: 3 px off the middle"), FMath::IsNearlyEqual(Plain.Strokes[0].Points[1].Size() * UnitScale, 3.f, 0.01f));

	TArray<const FShape*> Seen;
	for (const EWeaponKind Kind : AllGunKinds)
	{
		const FString Name = FString::Printf(TEXT("Kind %d"), static_cast<int32>(Kind));
		const FShape& Shape = ShapeFor(Kind);
		TestTrue(Name + TEXT(" has lines or a ring"), Shape.Strokes.Num() + Shape.Rings.Num() > 0);
		TestFalse(Name + TEXT(" has its own shape, not the old cross"), SameShape(Shape, Plain));
		for (const FShape* Other : Seen)
		{
			TestFalse(Name + TEXT(" differs from every other kind"), SameShape(Shape, *Other));
		}
		Seen.Add(&Shape);
		// Every kind keeps an aim point for a third-person aim, where the lines have gone.
		TestTrue(Name + TEXT(" has a centre dot"), Shape.Dot.Radius > 0.f);
		for (const FStroke& Stroke : Shape.Strokes)
		{
			TestTrue(Name + TEXT(": every line has two points or more and a width"), Stroke.Points.Num() >= 2 && Stroke.Width > 0.f);
		}
		// However wide the spread, the shape stays clear of the prompt under it and inside its square.
		const float Limit = OpenLimitFor(Shape);
		TestTrue(Name + TEXT(" stays short of the interaction prompt"), ReachOf(Shape, Limit) <= MaxReach + 0.01f
			&& MaxReach < UHudInteractPromptWidget::Gap);
		TestTrue(Name + TEXT(" fits its square"), ReachOf(Shape, Limit) < CanvasSize * 0.5f);
		// The launcher's one rocket has no spread to show: its diamond and drop marks stay put.
		TestTrue(Name + TEXT(" opens outward with the spread"), Kind == EWeaponKind::Launcher || ReachOf(Shape, Limit) > ReachOf(Shape, 0.f));
	}
	TestTrue(TEXT("The rifle's cross has no dot from the hip (as drawn)"), !ShapeFor(EWeaponKind::Rifle).bHipDot);
	TestEqual(TEXT("The shotgun's is a ring"), ShapeFor(EWeaponKind::Shotgun).Rings.Num(), 1);
	TestTrue(TEXT("The revolver's dot sits under its chevron"), ShapeFor(EWeaponKind::Revolver).Dot.At.Y > 0.f);

	// The gap follows the spread as the old cross's did, and a shape with no dot at the hip still shows one aimed in third person.
	TestEqual(TEXT("No spread: shut"), OpenOf(GapBoxFor(0.f)), 0.f);
	TestTrue(TEXT("More spread: wider"), OpenOf(GapBoxFor(4.f)) > OpenOf(GapBoxFor(1.f)));
	TestEqual(TEXT("A huge spread stops at the most"), OpenOf(GapBoxFor(100.f)), MaxOpen);
	TestEqual(TEXT("No hip dot: none beside the lines"), UPlayerHUDWidget::AimDotOpacity(0.f, 0.f, false, true, false), 0.f);
	TestTrue(TEXT("...but the aim point through a third-person aim"), UPlayerHUDWidget::AimDotOpacity(1.f, 0.f, false, false, false) > 0.5f);

	// Another kind grows in from small and faint to whole.
	TestTrue(TEXT("Switch: starts faint and small"), SwitchFadeAt(0.f) < 0.01f && SwitchScaleAt(0.f) < 1.f);
	TestTrue(TEXT("...whole by its end"), FMath::IsNearlyEqual(SwitchFadeAt(SwitchSeconds), 1.f) && FMath::IsNearlyEqual(SwitchScaleAt(SwitchSeconds), 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudCrosshairKickTest, "Looter.UI.Crosshair.Kick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudCrosshairKickTest::RunTest(const FString& Parameters)
{
	using namespace HudCrosshair;
	for (const EWeaponKind Kind : AllGunKinds)
	{
		const FString Name = FString::Printf(TEXT("Kind %d"), static_cast<int32>(Kind));
		const FKick Kick = KickFor(Kind);
		TestTrue(Name + TEXT(" kicks out, not in, and not wild"), Kick.Scale > 1.f && Kick.Scale <= 1.7f);
		TestTrue(Name + TEXT(" settles within a quarter second"), Kick.Seconds > 0.f && Kick.Seconds <= 0.25f);
		TestTrue(Name + TEXT(": the frame of the shot shows the whole kick"), FMath::IsNearlyEqual(KickScaleAt(Kick, 0.f), Kick.Scale));
		TestTrue(Name + TEXT(": settled by its time"), FMath::IsNearlyEqual(KickScaleAt(Kick, Kick.Seconds), 1.f));
	}
	// SMG small and quick, sniper and launcher big and slow (the mockup's words; numbers from GUNS.md F).
	const FKick Smg = KickFor(EWeaponKind::SMG);
	for (const EWeaponKind Heavy : { EWeaponKind::Sniper, EWeaponKind::Launcher })
	{
		const FKick Big = KickFor(Heavy);
		TestTrue(FString::Printf(TEXT("SMG kicks less than kind %d"), static_cast<int32>(Heavy)), Smg.Scale < Big.Scale);
		TestTrue(FString::Printf(TEXT("SMG settles faster than kind %d"), static_cast<int32>(Heavy)), Smg.Seconds < Big.Seconds);
	}
	TestTrue(TEXT("The rifle's is the plan's 1.3x over 0.12 s"), FMath::IsNearlyEqual(KickFor(EWeaponKind::Rifle).Scale, 1.3f)
		&& FMath::IsNearlyEqual(KickFor(EWeaponKind::Rifle).Seconds, 0.12f));
	TestTrue(TEXT("No gun keeps the old cross's 1.45x over 0.16 s"), FMath::IsNearlyEqual(KickFor(EWeaponKind::None).Scale, 1.45f)
		&& FMath::IsNearlyEqual(KickFor(EWeaponKind::None).Seconds, 0.16f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudCrosshairColourTest, "Looter.UI.Crosshair.Colour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudCrosshairColourTest::RunTest(const FString& Parameters)
{
	using namespace LooterUI;
	using Graphics = UGraphicsSettingsSubsystem;

	// White with a dark edge by default (the user's call, 2026-10-29), for a fresh install and a save from before the option.
	TestTrue(TEXT("A fresh save: white"), GetDefault<ULooterGraphicsSave>()->CrosshairColour == ECrosshairColour::White);
	TestTrue(TEXT("No save: white"), Graphics::CrosshairColourOf(nullptr) == ECrosshairColour::White);
	TestTrue(TEXT("White is the first choice on the row"), AllCrosshairColours[0] == ECrosshairColour::White);
	TestTrue(TEXT("White is the HUD's white"), HudCrosshair::ColourOf(ECrosshairColour::White).Equals(Color::SlateText()));

	// Each choice is a palette colour, and the edge stays dark under every one.
	TestTrue(TEXT("Salt"), HudCrosshair::ColourOf(ECrosshairColour::Salt).Equals(Color::SaltLine()));
	TestTrue(TEXT("Gold"), HudCrosshair::ColourOf(ECrosshairColour::Gold).Equals(Color::SoulGold()));
	TestTrue(TEXT("Cyan"), HudCrosshair::ColourOf(ECrosshairColour::Cyan).Equals(Color::CyanText()));
	TestTrue(TEXT("Green"), HudCrosshair::ColourOf(ECrosshairColour::Green).Equals(Color::Better()));
	TestTrue(TEXT("The dark edge is the HUD's outline"), HudCrosshair::EdgeColour().Equals(Color::Outline()));

	TSet<FString> Names;
	for (const ECrosshairColour Colour : AllCrosshairColours)
	{
		const FString Name = Graphics::CrosshairColourName(Colour);
		TestFalse(TEXT("Every colour has a name"), Name.IsEmpty());
		TestFalse(FString::Printf(TEXT("%s is named once"), *Name), Names.Contains(Name));
		Names.Add(Name);
		TestTrue(FString::Printf(TEXT("%s reads lighter than its edge"), *Name),
			HudCrosshair::ColourOf(Colour).GetLuminance() > HudCrosshair::EdgeColour().GetLuminance() + 0.2f);
	}
	TestEqual(TEXT("Five choices"), Names.Num(), 5);

	// A hand-edited save with no such colour falls back to white.
	ULooterGraphicsSave* Odd = NewObject<ULooterGraphicsSave>();
	Odd->CrosshairColour = static_cast<ECrosshairColour>(200);
	TestTrue(TEXT("An unknown colour: white"), Graphics::CrosshairColourOf(Odd) == ECrosshairColour::White);
	Odd->CrosshairColour = ECrosshairColour::Gold;
	TestTrue(TEXT("A saved colour is kept"), Graphics::CrosshairColourOf(Odd) == ECrosshairColour::Gold);
	return true;
}

#endif
