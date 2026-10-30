#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Elements matter on the HUD (Looter.UI.StatusIcons, 2026-10-29): the status icons under a creature's tag (which show, in
// what order, their stack counts and pop), a typed ward's colours on the tag and on the hit marker, the status pieces'
// rolling stream number and the first-time callout's word and motion. No world: the rules are plain functions and the tag
// is checked unbuilt (its state, not its pixels; Looter.UI.Tier.Tag builds one).

#include "Elements/ElementRules.h"
#include "Elements/ElementStatusIcons.h"
#include "UI/HUD/HudHitMarker.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/World/CreatureHealthBarWidget.h"
#include "UI/World/DamageNumberActor.h"
#include "UObject/Package.h"

namespace
{
	FCreatureStatusIcon Icon(ELooterElement Element, float TimeShare, int32 Stacks = 1)
	{
		FCreatureStatusIcon Made;
		Made.Element = Element;
		Made.TimeShare = TimeShare;
		Made.Stacks = Stacks;
		return Made;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStatusIconsTest, "Looter.UI.StatusIcons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStatusIconsTest::RunTest(const FString& Parameters)
{
	using UBar = UCreatureHealthBarWidget;

	// Which icons show, and where: one an element, in the elements' order, spent and empty ones left out.
	TArray<FCreatureStatusIcon> In = { Icon(ELooterElement::Rot, 0.5f), Icon(ELooterElement::None, 1.f), Icon(ELooterElement::Ember, 0.9f, 3),
		Icon(ELooterElement::GraveSalt, 0.f), Icon(ELooterElement::Ember, 0.95f, 2), Icon(ELooterElement::Thunder, 1.4f, 0) };
	TArray<FCreatureStatusIcon> Out;
	UBar::ArrangeStatuses(In, Out);
	if (TestEqual(TEXT("Burn, jolt and rot show; the spent salt and None don't"), Out.Num(), 3))
	{
		TestTrue(TEXT("In the elements' order"), Out[0].Element == ELooterElement::Ember && Out[1].Element == ELooterElement::Thunder
			&& Out[2].Element == ELooterElement::Rot);
		TestNearlyEqual(TEXT("Two burns reported apart: one icon, the longer time"), Out[0].TimeShare, 0.95f, 0.0001f);
		TestEqual(TEXT("...and the most stacks"), Out[0].Stacks, 3);
		TestNearlyEqual(TEXT("A time past whole is whole"), Out[1].TimeShare, 1.f, 0.0001f);
		TestEqual(TEXT("A count of none is one"), Out[1].Stacks, 1);
	}
	TestTrue(TEXT("One stack: no count"), UBar::StacksLabel(1).IsEmpty());
	TestEqual(TEXT("Three: \"3\""), UBar::StacksLabel(3).ToString(), FString(TEXT("3")));
	TestNearlyEqual(TEXT("A new icon pops in big"), UBar::StatusPopScaleAt(0.f), UBar::StatusPopScale, 0.001f);
	TestNearlyEqual(TEXT("...settled by the pop's end"), UBar::StatusPopScaleAt(UBar::StatusPopSeconds), 1.f, 0.001f);
	TestTrue(TEXT("...shrinking between"), UBar::StatusPopScaleAt(UBar::StatusPopSeconds * 0.5f) < UBar::StatusPopScale
		&& UBar::StatusPopScaleAt(UBar::StatusPopSeconds * 0.5f) > 1.f);
	TestTrue(TEXT("Five icons fit under the bar"),
		UBar::MaxStatusIcons * UBar::StatusIconSize + (UBar::MaxStatusIcons - 1) * UBar::StatusIconGap <= UBar::BarWidth);

	// On a tag (unbuilt: its state).
	UBar* Tag = NewObject<UBar>(GetTransientPackage());
	if (!TestNotNull(TEXT("A tag"), Tag))
	{
		return false;
	}
	TestEqual(TEXT("A new tag: no statuses"), Tag->GetStatusCount(), 0);
	TArray<FCreatureStatusIcon> Burning = { Icon(ELooterElement::Ember, 1.f) };
	Tag->SetStatuses(Burning);
	TestEqual(TEXT("Burning: one icon"), Tag->GetStatusCount(), 1);
	TestNearlyEqual(TEXT("...popping in"), Tag->GetStatusScale(0), UBar::StatusPopScale, 0.001f);
	Tag->AdvanceStatuses(UBar::StatusPopSeconds);
	TestNearlyEqual(TEXT("...then settled"), Tag->GetStatusScale(0), 1.f, 0.001f);
	TArray<FCreatureStatusIcon> BurningRotting = { Icon(ELooterElement::Rot, 1.f), Icon(ELooterElement::Ember, 0.6f, 2) };
	Tag->SetStatuses(BurningRotting);
	if (TestEqual(TEXT("Rot joins: two icons"), Tag->GetStatusCount(), 2))
	{
		TestTrue(TEXT("...the burn kept its place first"), Tag->GetStatuses()[0].Element == ELooterElement::Ember);
		TestNearlyEqual(TEXT("...its ring drained to its time left"), Tag->GetStatuses()[0].TimeShare, 0.6f, 0.0001f);
		TestNearlyEqual(TEXT("...not popping again"), Tag->GetStatusScale(0), 1.f, 0.001f);
		TestTrue(TEXT("...the rot popping in"), Tag->GetStatusScale(1) > 1.f);
	}
	Tag->SetStatuses(TConstArrayView<FCreatureStatusIcon>());
	TestEqual(TEXT("All run out: none"), Tag->GetStatusCount(), 0);

	// A typed ward: the tag's bar in the ward's colour, its chip paler; no element is the kit's cyan.
	Tag->SetWard(true, 1.f, ElementRules::UiColor(ELooterElement::Rot));
	TestTrue(TEXT("A Rot ward's bar is Rot's green"), Tag->GetWardFillColor().Equals(LooterUI::Color::ElementRot(), 0.0001f));
	TestFalse(TEXT("...its chip a different (paler) green"), Tag->GetWardChipColor().Equals(Tag->GetWardFillColor(), 0.01f));
	TestTrue(TEXT("No element: the cyan"), UBar::DefaultWardColor().Equals(LooterUI::Color::CyanText(), 0.0001f));

	// ...and on the hit marker: a ward's tick in its colour, its break a touch whiter; other marks as they were.
	TestTrue(TEXT("A Thunder ward's tick is Thunder's blue"),
		HudHitMarker::Tint(EHudHitMark::Ward, ELooterElement::Thunder).Equals(LooterUI::Color::ElementThunder(), 0.0001f));
	const FLinearColor Break = HudHitMarker::Tint(EHudHitMark::WardBreak, ELooterElement::Thunder);
	TestTrue(TEXT("...its break whiter"), Break.R > LooterUI::Color::ElementThunder().R && Break.G > LooterUI::Color::ElementThunder().G);
	TestTrue(TEXT("A ward with no element: the cyan as before"),
		HudHitMarker::Tint(EHudHitMark::Ward, ELooterElement::None).Equals(HudHitMarker::Tint(EHudHitMark::Ward), 0.0001f));
	TestTrue(TEXT("A crit is a crit whatever the ward"),
		HudHitMarker::Tint(EHudHitMark::Crit, ELooterElement::Thunder).Equals(HudHitMarker::Tint(EHudHitMark::Crit), 0.0001f));

	// The stream: only a status's pieces join one; it counts up to its total, never past it, holds between pieces, then fades.
	using namespace DamageNumberStream;
	TestTrue(TEXT("A burn's piece streams"), StreamElementOf(false, true, ELooterElement::Ember) == ELooterElement::Ember);
	TestTrue(TEXT("A hit doesn't"), StreamElementOf(false, false, ELooterElement::Ember) == ELooterElement::None);
	TestTrue(TEXT("Nor a crit"), StreamElementOf(true, true, ELooterElement::Ember) == ELooterElement::None);
	float Shown = 10.f;
	Shown = RollToward(Shown, 40.f, 0.05f);
	TestTrue(FString::Printf(TEXT("It rolls up (%.1f)"), Shown), Shown > 10.f && Shown < 40.f);
	for (int32 Step = 0; Step < 60; ++Step)
	{
		Shown = RollToward(Shown, 40.f, 0.05f);
	}
	TestNearlyEqual(TEXT("...reaching its total"), Shown, 40.f, 0.0001f);
	TestNearlyEqual(TEXT("...never past it"), RollToward(45.f, 40.f, 0.1f), 40.f, 0.0001f);
	TestTrue(TEXT("It holds whole longer than a status's tick"), HoldSeconds > 0.5f && OpacityAt(0.5f) == 1.f);
	TestTrue(TEXT("...then fades"), OpacityAt(HoldSeconds + FadeSeconds * 0.5f) < 1.f && OpacityAt(HoldSeconds + FadeSeconds) <= 0.f);
	TestTrue(TEXT("...and is spent"), IsSpent(HoldSeconds + FadeSeconds) && !IsSpent(HoldSeconds));
	TestTrue(TEXT("A piece pulses it a little"), PulseAt(0.f) > 1.f && PulseAt(0.f) < 1.3f && PulseAt(PulseSeconds) == 1.f);
	TestTrue(TEXT("It rises a little and stops"), RiseAt(0.f) == 0.f && RiseAt(5.f) <= RiseHeight && RiseAt(5.f) > RiseHeight * 0.9f);

	// The first-time callout: each element's word, slamming in and fading late.
	TestEqual(TEXT("Cinder burns"), ElementRules::StatusWord(ELooterElement::Ember), FString(TEXT("BURNING")));
	for (const ELooterElement Element : { ELooterElement::SoulLight, ELooterElement::GraveSalt, ELooterElement::Thunder, ELooterElement::Rot })
	{
		TestFalse(TEXT("Every element has its word"), ElementRules::StatusWord(Element).IsEmpty());
	}
	TestTrue(TEXT("No word for None"), ElementRules::StatusWord(ELooterElement::None).IsEmpty());
	TestNearlyEqual(TEXT("The callout slams in"), DamageNumberCallout::ScaleAt(0.f), DamageNumberCallout::PopScale, 0.001f);
	TestNearlyEqual(TEXT("...settles"), DamageNumberCallout::ScaleAt(DamageNumberCallout::PopSeconds), 1.f, 0.001f);
	TestTrue(TEXT("...stays whole most of its life"), DamageNumberCallout::OpacityAt(DamageNumberCallout::Seconds * 0.5f) == 1.f);
	TestTrue(TEXT("...and is gone by its end"), DamageNumberCallout::OpacityAt(DamageNumberCallout::Seconds) <= 0.f);
	return true;
}

#endif
