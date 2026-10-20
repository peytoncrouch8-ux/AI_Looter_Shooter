#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The enemy tiers on the HUD (Looter.UI.Tier.*): the creature tag's tier (the top tiers' frame and their bigger, longer
// notice pop with the frame's flare; the trait line from Gravebound up, dressed from the creature itself) and the kill's
// "+N XP" in a top tier's colour. The pops are pure functions of the sting's age, so their numbers are checked alone.

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTierLook.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/CreatureTierLookComponent.h"
#include "Creatures/SpiderCreature.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/World/CreatureHealthBarWidget.h"
#include "UI/World/XpPopupActor.h"
#include "UI/World/XpPopupSubsystem.h"
#include "UI/World/XpPopupWidget.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTierTagLookTest, "Looter.UI.Tier.Tag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTierTagLookTest::RunTest(const FString& Parameters)
{
	using UBar = UCreatureHealthBarWidget;
	const FLinearColor Gold = LooterUI::Color::TierHallowed();

	// The top tiers' pop: longer and bigger than the rank sting's, in a white-hot of its own colour, never the accent.
	TestTrue(TEXT("A top tier's pop lasts longer"), UBar::TopTierFlashSeconds > UBar::RankFlashSeconds);
	TestTrue(TEXT("...and starts bigger"), UBar::TopTierFlashPeakScale > UBar::RankFlashPeakScale);
	TestNearlyEqual(TEXT("Its peak at the sting"), UBar::TierFlashScaleAt(0.f, true), UBar::TopTierFlashPeakScale, 0.001f);
	TestNearlyEqual(TEXT("Anyone else's is the rank sting's"), UBar::TierFlashScaleAt(0.2f, false), UBar::RankFlashScaleAt(0.2f), 0.0001f);
	TestTrue(TEXT("...colour too"), UBar::TierFlashColorAt(0.2f, Gold, false).Equals(UBar::RankFlashColorAt(0.2f, Gold), 0.0001f));
	const FLinearColor Lit = UBar::TierFlashColorAt(0.f, Gold, true);
	TestTrue(TEXT("Lit white-hot of its own colour"), Lit.G > Gold.G && Lit.B > Gold.B);
	TestFalse(TEXT("...not the accent's orange"), Lit.Equals(LooterUI::Color::Accent(), 0.01f));
	TestTrue(TEXT("Its own colour at the end"), UBar::TierFlashColorAt(UBar::TopTierFlashSeconds, Gold, true).Equals(Gold, 0.001f));
	TestNearlyEqual(TEXT("Its own size at the end"), UBar::TierFlashScaleAt(UBar::TopTierFlashSeconds, true), 1.f, 0.001f);
	TestTrue(TEXT("Still popping after the rank sting's would be over"), UBar::TierFlashScaleAt(UBar::RankFlashSeconds + 0.05f, true) > 1.f);
	float LastScale = UBar::TierFlashScaleAt(0.f, true);
	bool bSettles = true;
	for (int32 Step = 1; Step <= 12; ++Step)
	{
		const float Scale = UBar::TierFlashScaleAt(UBar::TopTierFlashSeconds * Step / 12.f, true);
		bSettles = bSettles && Scale <= LastScale + 0.0001f && Scale >= 1.f;
		LastScale = Scale;
	}
	TestTrue(TEXT("It only settles"), bSettles);

	// The frame's flare: full at the sting, gone by the pop's end, nothing outside it.
	TestNearlyEqual(TEXT("The frame flares at the sting"), UBar::FrameFlareAt(0.f), 1.f, 0.001f);
	TestNearlyEqual(TEXT("...and is out by the pop's end"), UBar::FrameFlareAt(UBar::TopTierFlashSeconds), 0.f, 0.001f);
	TestNearlyEqual(TEXT("...never before it"), UBar::FrameFlareAt(-1.f), 0.f, 0.001f);
	TestTrue(TEXT("The frame swells a little, not a lot"), UBar::FrameFlareScale > 1.f && UBar::FrameFlareScale < 1.25f);
	TestNearlyEqual(TEXT("A Regular tag's bar sits where it always did (3 px under the name)"), UBar::FramePad, 3.f, 0.001f);

	// On a tag.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	UBar* Tag = CreateWidget<UBar>(World, UBar::StaticClass());
	if (!TestNotNull(TEXT("A tag"), Tag))
	{
		return false;
	}
	// Its Slate widget held for the test, as a widget component holds it in play.
	const TSharedRef<SWidget> TagSlate = Tag->TakeWidget();
	TestFalse(TEXT("A new tag: no frame"), Tag->HasThickFrame());
	TestTrue(TEXT("...no trait line"), Tag->GetTraitLine().IsEmpty());
	Tag->SetTier(true, Gold, true);
	Tag->SetTraitLine(FText::FromString(TEXT("FLEET · WARDED")));
	TestTrue(TEXT("Framed for a top tier"), Tag->HasThickFrame());
	TestEqual(TEXT("...its traits under its name"), Tag->GetTraitLine().ToString(), FString(TEXT("FLEET · WARDED")));
	Tag->SetRankStingAge(0.f);
	Tag->SetRankStingAge(UBar::RankFlashSeconds + 0.1f);
	Tag->SetTier(false, FLinearColor::White, false);
	Tag->SetTraitLine(FText::GetEmpty());
	TestFalse(TEXT("Back to Regular: no frame"), Tag->HasThickFrame());
	TestTrue(TEXT("...no trait line"), Tag->GetTraitLine().IsEmpty());

	// A Warded creature's ward bar: up while its ward stands, a hit snapping the fill down with the chip trailing and
	// draining after it, the break flashing white-hot before it goes; never shown for a creature that isn't Warded.
	TestFalse(TEXT("A new tag: no ward bar"), Tag->IsWardShown());
	Tag->SetWard(false, 0.f);
	TestFalse(TEXT("Not Warded: no ward bar"), Tag->IsWardShown() || Tag->IsWardBreaking());
	Tag->SetWard(true, 1.f);
	TestTrue(TEXT("Warded: its ward bar, full"), Tag->IsWardShown() && FMath::IsNearlyEqual(Tag->GetWardFill(), 1.f));
	Tag->SetWard(true, 0.4f);
	TestNearlyEqual(TEXT("A hit on the ward: the fill down to what's left"), Tag->GetWardFill(), 0.4f, 0.0001f);
	TestNearlyEqual(TEXT("...the chip of what it took still there"), Tag->GetWardChip(), 1.f, 0.0001f);
	Tag->AdvanceWard(UBar::WardChipHold * 0.5f);
	TestNearlyEqual(TEXT("...holding a moment"), Tag->GetWardChip(), 1.f, 0.0001f);
	Tag->AdvanceWard(UBar::WardChipHold + 1.f);
	TestNearlyEqual(TEXT("...then drained down to the fill"), Tag->GetWardChip(), 0.4f, 0.0001f);
	Tag->SetWard(true, 0.f);
	TestTrue(TEXT("Broken: it flashes"), Tag->IsWardBreaking() && !Tag->IsWardShown());
	TestNearlyEqual(TEXT("...white-hot at once"), UBar::WardBreakFlashAt(0.f), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("...out by the break's end"), UBar::WardBreakFlashAt(UBar::WardBreakSeconds), 0.f, 0.0001f);
	Tag->AdvanceWard(UBar::WardBreakSeconds);
	TestFalse(TEXT("...and goes"), Tag->IsWardBreaking() || Tag->IsWardShown());
	Tag->SetWard(true, 0.f);
	TestFalse(TEXT("A ward already broken stays hidden (no second flash)"), Tag->IsWardShown() || Tag->IsWardBreaking());
	Tag->SetWard(true, 1.f);
	TestTrue(TEXT("Whole again (a new life): back, full"), Tag->IsWardShown() && FMath::IsNearlyEqual(Tag->GetWardFill(), 1.f));
	Tag->SetWard(false, 0.f);
	TestFalse(TEXT("Its traits taken: gone at once, no flash"), Tag->IsWardShown() || Tag->IsWardBreaking());
	TestTrue(TEXT("The ward's fill is the kit's cyan"), UBar::WardFillColor().Equals(LooterUI::Color::CyanText(), 0.0001f));

	// Dressed from a creature: a Regular spider plainly, a Hallowed one framed with its traits under its name.
	ASpiderCreature* Spider = Cast<ASpiderCreature>(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector::ZeroVector));
	if (!TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	BossTestWorld::NoLoot(Spider);
	UCreatureTierLookComponent::DressTag(*Spider, *Tag);
	TestFalse(TEXT("A Regular spider's tag: no frame"), Tag->HasThickFrame());
	TestTrue(TEXT("...no traits"), Tag->GetTraitLine().IsEmpty());
	Spider->SetRank(ECreatureRank::Hallowed);
	UCreatureTierLookComponent::DressTag(*Spider, *Tag);
	TestTrue(TEXT("A Hallowed spider's tag is framed"), Tag->HasThickFrame());
	UCreatureTierLookComponent* Look = UCreatureTierLookComponent::Find(Spider);
	TestNotNull(TEXT("...and it has its tier look"), Look);
	if (Look)
	{
		// Its line is its traits' (two at Hallowed, from the rank settings; none if a checkout's settings give it none).
		TestEqual(TEXT("...its traits under its name"), Tag->GetTraitLine().ToString(), Look->TraitLineNow().ToString());
	}
	Spider->SetRank(ECreatureRank::Rare);
	UCreatureTierLookComponent::DressTag(*Spider, *Tag);
	TestFalse(TEXT("Restless: no frame"), Tag->HasThickFrame());
	TestTrue(TEXT("...and no trait line"), Tag->GetTraitLine().IsEmpty());

	// Its ward, dressed from its traits: shown Warded, drained by a hit, gone once it isn't Warded.
	UCreatureTraitComponent* Traits = Spider->GetTraitComponent();
	const UHealthComponent* Health = Spider->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("Its traits"), Traits) && TestNotNull(TEXT("...and health"), Health))
	{
		TestFalse(TEXT("A Restless spider without Warded: no ward bar"), Tag->IsWardShown());
		Traits->SetTraits(TArray<ECreatureTrait>({ ECreatureTrait::Warded }));
		UCreatureTierLookComponent::DressTag(*Spider, *Tag);
		TestTrue(TEXT("Warded: its tag shows its ward, full"), Tag->IsWardShown() && FMath::IsNearlyEqual(Tag->GetWardFill(), 1.f));
		BossTestWorld::Hurt(Spider, Health->GetMaxHealth() * 0.1f);
		UCreatureTierLookComponent::DressTag(*Spider, *Tag);
		TestTrue(FString::Printf(TEXT("...drained by a hit (%.2f), its health untouched"), Tag->GetWardFill()),
			Tag->GetWardFill() > 0.f && Tag->GetWardFill() < 1.f && FMath::IsNearlyEqual(Health->GetHealth(), Health->GetMaxHealth()));
		TestNearlyEqual(TEXT("...the bar showing the ward's own share"), Tag->GetWardFill(), Traits->GetWardFraction(), 0.0001f);
		Traits->SetTraits(TArray<ECreatureTrait>());
		UCreatureTierLookComponent::DressTag(*Spider, *Tag);
		TestFalse(TEXT("...and none once it isn't Warded"), Tag->IsWardShown() || Tag->IsWardBreaking());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTierXpPopupTest, "Looter.UI.Tier.XpPopup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTierXpPopupTest::RunTest(const FString& Parameters)
{
	// A top tier's kill: popping harder, rising farther, a little longer; a kill's own look untouched.
	const AXpPopupActor::FLook Kill = AXpPopupActor::LookAt(0.f, false);
	const AXpPopupActor::FLook Tier = AXpPopupActor::LookAt(0.f, false, true);
	TestTrue(TEXT("A tier kill's pops harder"), Tier.Scale > Kill.Scale);
	TestTrue(TEXT("...stays a little longer"), AXpPopupActor::SecondsFor(false, true) > AXpPopupActor::SecondsFor(false, false));
	TestTrue(TEXT("...rises farther"), AXpPopupActor::LookAt(1.f, false, true).Offset.Y < AXpPopupActor::LookAt(1.f, false).Offset.Y);
	TestTrue(TEXT("...beside the damage number as a kill's"), FMath::IsNearlyEqual(static_cast<float>(AXpPopupActor::LookAt(0.5f, false, true).Offset.X),
		AXpPopupActor::SmallBeside));
	TestNearlyEqual(TEXT("A turn-in's is never a tier's"), AXpPopupActor::SecondsFor(true, true), AXpPopupActor::BigSeconds, 0.0001f);
	TestTrue(TEXT("A tier kill's type is a size up"), UXpPopupWidget::TierSize > UXpPopupWidget::SmallSize && UXpPopupWidget::TierSize < UXpPopupWidget::BigSize);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	UXpPopupSubsystem* Popups = UXpPopupSubsystem::Get(World);
	if (!TestNotNull(TEXT("The popups"), Popups))
	{
		return false;
	}
	ASpiderCreature* Spider = Cast<ASpiderCreature>(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector::ZeroVector));
	if (!TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	BossTestWorld::NoLoot(Spider);
	const AXpPopupActor* Plain = Popups->ShowKill(40, Spider);
	TestTrue(TEXT("A Regular kill's: the experience's cyan"), Plain && !Plain->IsTier());
	Spider->SetRank(ECreatureRank::Ethereal);
	const AXpPopupActor* Red = Popups->ShowKill(360, Spider);
	TestTrue(TEXT("An Ethereal kill's: in its tier's colour"), Red && Red->IsTier());
	Spider->SetRank(ECreatureRank::Epic);
	const AXpPopupActor* Purple = Popups->ShowKill(140, Spider);
	TestTrue(TEXT("A Gravebound kill's: the experience's cyan still"), Purple && !Purple->IsTier());
	const AXpPopupActor* TurnIn = Popups->ShowMissionAt(1240, FVector(0.0, 0.0, 150.0), 0.f);
	TestTrue(TEXT("A turn-in's: never a tier's"), TurnIn && TurnIn->IsBig() && !TurnIn->IsTier());
	return true;
}

#endif
