#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// How the enemy tiers look (Saved/Tiers/DESIGN.md, "Readable and rewarding"; Looter.Creatures.TierLook.*): the rules on
// their own (CreatureTierLook.h), the tier shell on creatures in a test level (made on a tier above Regular, shared looks,
// never under a hit flash, taken off as the creature goes back to Regular, fading as it dies), and a top tier's death show
// (pillars lit and given back, the loot's pop, the crowd's cap). A test level never ticks: the tests move them on by hand.

#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureTierFXSubsystem.h"
#include "Creatures/CreatureTierLook.h"
#include "Creatures/CreatureTierLookComponent.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Elements/ElementRules.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float TierLookFrame = 1.f / 60.f;

	/** Whether M_CreatureTier is in the game yet (Tools/Unreal/build_tier_material.py): without it no shell is worn. */
	bool TierLookHasShellMaterial()
	{
		return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(UCreatureTierFXSubsystem::ShellMaterialPath)));
	}

	/** Its shell's flare and fade moved on by Seconds, a frame at a time. */
	void TierLookRun(UCreatureTierLookComponent& Look, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += TierLookFrame)
		{
			Look.Advance(TierLookFrame);
		}
	}

	void TierShowRun(UCreatureTierFXSubsystem& Effects, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += TierLookFrame)
		{
			Effects.Advance(TierLookFrame);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTierLookRulesTest, "Looter.Creatures.TierLook.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTierLookRulesTest::RunTest(const FString& Parameters)
{
	using namespace CreatureTierLook;
	const ECreatureRank Regular = ECreatureRank::Basic;
	const ECreatureRank Restless = ECreatureRank::Rare;
	const ECreatureRank Gravebound = ECreatureRank::Epic;
	const ECreatureRank Hallowed = ECreatureRank::Hallowed;
	const ECreatureRank Ethereal = ECreatureRank::Ethereal;

	// Which tiers show what: a glow from Restless, traits from Gravebound, the top tiers' moments, the Ethereal's rim.
	TestEqual(TEXT("Regular is tier 0"), TierOf(Regular), 0);
	TestEqual(TEXT("Ethereal is tier 4"), TierOf(Ethereal), 4);
	TestEqual(TEXT("A Legendary monster has no tier"), TierOf(ECreatureRank::Legendary), static_cast<int32>(INDEX_NONE));
	TestFalse(TEXT("Regular: no glow"), HasBodyGlow(Regular));
	TestTrue(TEXT("Restless: a glow"), HasBodyGlow(Restless));
	TestFalse(TEXT("A boss: none of it"), HasBodyGlow(ECreatureRank::Boss) || IsTopTier(ECreatureRank::Boss));
	TestFalse(TEXT("A Legendary monster: none of it"), HasBodyGlow(ECreatureRank::Legendary) || IsTopTier(ECreatureRank::Legendary));
	TestFalse(TEXT("Restless: no trait line"), ShowsTraitLine(Restless));
	TestTrue(TEXT("Gravebound, Hallowed, Ethereal: the trait line"), ShowsTraitLine(Gravebound) && ShowsTraitLine(Hallowed) && ShowsTraitLine(Ethereal));
	TestTrue(TEXT("Hallowed and Ethereal are the top tiers"), IsTopTier(Hallowed) && IsTopTier(Ethereal) && !IsTopTier(Gravebound));
	TestTrue(TEXT("Only the Ethereal has the ghost-rim"), HasGhostRim(Ethereal) && !HasGhostRim(Hallowed) && !HasGhostRim(Regular));

	// Colours from the rank settings: the body's glow and the tag's word.
	for (const ECreatureRank Rank : LooterRanks::Tiers)
	{
		const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Rank);
		TestTrue(FString::Printf(TEXT("%s: its glow is its settings'"), *UEnum::GetValueAsString(Rank)), GlowColor(Rank).Equals(Info.GlowColor));
		TestTrue(FString::Printf(TEXT("%s: its tag's colour is its settings'"), *UEnum::GetValueAsString(Rank)), TagColor(Rank).Equals(Info.Color));
	}
	TestTrue(TEXT("The top tiers frame their tags in their colour"), TagFor(Hallowed).bThickFrame && TagFor(Ethereal).bThickFrame
		&& TagFor(Ethereal).FrameColor.Equals(TagColor(Ethereal)) && TagFor(Hallowed).bTopTierFlash);
	TestFalse(TEXT("Gravebound's tag has no frame"), TagFor(Gravebound).bThickFrame || TagFor(Gravebound).bTopTierFlash);

	// The shells by kind: eyes on a spider, the core on a slime, the rim on an Ethereal anything; none on a Regular one.
	TestFalse(TEXT("A Regular spider wears no shell"), ShellFor(ECreatureTierBody::Spider, Regular).bWorn);
	const FCreatureTierShell RestlessSpider = ShellFor(ECreatureTierBody::Spider, Restless);
	TestTrue(TEXT("A Restless spider's eyes glow"), RestlessSpider.bWorn && RestlessSpider.bEyes && !RestlessSpider.bRim && !RestlessSpider.bCore);
	const FCreatureTierShell EtherealSpider = ShellFor(ECreatureTierBody::Spider, Ethereal);
	TestTrue(TEXT("An Ethereal spider's eyes glow, and its ghost-rim shows"), EtherealSpider.bEyes && EtherealSpider.bRim);
	TestTrue(TEXT("Each tier's eyes burn hotter"), ShellFor(ECreatureTierBody::Spider, Restless).Glow < ShellFor(ECreatureTierBody::Spider, Gravebound).Glow
		&& ShellFor(ECreatureTierBody::Spider, Gravebound).Glow < ShellFor(ECreatureTierBody::Spider, Hallowed).Glow
		&& ShellFor(ECreatureTierBody::Spider, Hallowed).Glow < EtherealSpider.Glow);
	const FCreatureTierShell HallowedSlime = ShellFor(ECreatureTierBody::Slime, Hallowed);
	TestTrue(TEXT("A Hallowed slime's core glows"), HallowedSlime.bWorn && HallowedSlime.bCore && !HallowedSlime.bEyes && HallowedSlime.CoreRadius > 0.f);
	TestFalse(TEXT("A Restless Unpaid wears no shell (its coal burns in its tier's colour already)"), ShellFor(ECreatureTierBody::Unpaid, Restless).bWorn);
	const FCreatureTierShell EtherealUnpaid = ShellFor(ECreatureTierBody::Unpaid, Ethereal);
	TestTrue(TEXT("An Ethereal Unpaid wears the ghost-rim alone"), EtherealUnpaid.bWorn && EtherealUnpaid.bRim && !EtherealUnpaid.bEyes && !EtherealUnpaid.bCore);
	TestTrue(TEXT("...fading with its shroud (vertex alpha), which a spider's baked occlusion never does"), EtherealUnpaid.bShroudFade
		&& !EtherealSpider.bShroudFade && !HallowedSlime.bShroudFade);
	TestTrue(TEXT("Shared looks are named by kind and tier"), RestlessSpider.Key != EtherealSpider.Key && EtherealSpider.Key != EtherealUnpaid.Key
		&& !RestlessSpider.Key.IsNone());

	// The eye patch: inside Spider.py's 'eye' patch as Unreal reads it (V flipped), off its neighbours.
	FVector2f EyeMin;
	FVector2f EyeMax;
	SpiderEyePatch(EyeMin, EyeMax);
	TestTrue(TEXT("The eye patch is a real rectangle"), EyeMin.X < EyeMax.X && EyeMin.Y < EyeMax.Y);
	TestTrue(TEXT("...inside u 0.75-0.8125"), EyeMin.X > 0.75f && EyeMax.X < 0.8125f);
	TestTrue(TEXT("...inside V 0.875-1 (Blender's v 0-0.125, flipped)"), EyeMin.Y > 0.875f && EyeMax.Y < 1.f);

	// The coal and the lamp.
	TestTrue(TEXT("The top tiers' coals burn harder"), CoalGlow(Hallowed, 1.f) > 1.f && CoalGlow(Ethereal, 1.f) > CoalGlow(Hallowed, 1.f));
	TestNearlyEqual(TEXT("...the rest at their kind's own"), CoalGlow(Gravebound, 1.8f), 1.8f, 0.0001f);
	const FLinearColor Warm(1.f, 0.82f, 0.55f);
	TestTrue(TEXT("A Regular miner's lamp is its own warm flame"), LampColor(Regular, Warm).Equals(Warm));
	TestFalse(TEXT("A Restless miner's lamp is its tier's"), LampColor(Restless, Warm).Equals(Warm));

	// The sounds: the top tiers' alone, by the contract's names.
	TestTrue(TEXT("No tier cues below Hallowed"), NoticeCue(Gravebound).IsNone() && DeathCue(Gravebound).IsNone() && PopCue(Regular).IsNone());
	TestTrue(TEXT("Hallowed's notice"), NoticeCue(Hallowed) == FName(LooterSoundCue::Tier::NoticeHallowed));
	TestTrue(TEXT("Ethereal's death"), DeathCue(Ethereal) == FName(LooterSoundCue::Tier::DeathEthereal));
	TestTrue(TEXT("Ethereal's loot pop"), PopCue(Ethereal) == FName(LooterSoundCue::Tier::LootPopEthereal));
	TestFalse(TEXT("A Hallowed's sting never gives way to a Restless one's"), StingGivesWayTo(Hallowed, Restless));
	TestTrue(TEXT("...but two top tiers turning together sting once"), StingGivesWayTo(Hallowed, Ethereal));
	TestTrue(TEXT("Two Restless sting once"), StingGivesWayTo(Restless, Restless) && StingGivesWayTo(Restless, Hallowed));

	// The trait line.
	const TArray<ECreatureTrait> None;
	TestTrue(TEXT("No traits: no line"), TraitLine(None).IsEmpty());
	const TArray<ECreatureTrait> Two = { ECreatureTrait::Fleet, ECreatureTrait::Warded };
	TestEqual(TEXT("Two traits, upper case, a dot between"), TraitLine(Two).ToString(), FString(TEXT("FLEET · WARDED")));
	for (const ECreatureTrait Trait : CreatureTraits::All)
	{
		TestFalse(FString::Printf(TEXT("%s has a word"), *UEnum::GetValueAsString(Trait)), TraitWord(Trait).IsEmpty());
	}
	TestEqual(TEXT("Grave-burst's word"), TraitWord(ECreatureTrait::GraveBurst), FString(TEXT("Grave-burst")));

	// The death shows: the top tiers', the Ethereal's the bigger.
	TestFalse(TEXT("Gravebound has no death show"), DeathShowFor(Gravebound).bValid);
	const FCreatureTierDeathShow HallowedShow = DeathShowFor(Hallowed);
	const FCreatureTierDeathShow EtherealShow = DeathShowFor(Ethereal);
	TestTrue(TEXT("Hallowed and Ethereal have one"), HallowedShow.bValid && EtherealShow.bValid);
	TestTrue(TEXT("The Ethereal's is taller, longer, wider and flickers"), EtherealShow.PillarHeight > HallowedShow.PillarHeight
		&& EtherealShow.Seconds > HallowedShow.Seconds && EtherealShow.RingRadius > HallowedShow.RingRadius && EtherealShow.bFlicker
		&& !HallowedShow.bFlicker);
	TestTrue(TEXT("The burst is bigger than the body's own"), HallowedShow.BurstScale > 1.f && EtherealShow.BurstScale >= HallowedShow.BurstScale);
	TestTrue(TEXT("The loot pops a beat after the death, inside the show"), HallowedShow.PopDelay > 0.f && HallowedShow.PopDelay < 0.5f
		&& HallowedShow.Glints > 0);
	TestTrue(TEXT("A short pillar: two seconds or so"), EtherealShow.Seconds <= 2.5f);

	// The shell's flare and fade.
	TestNearlyEqual(TEXT("No sting: no flare"), NoticeFlareAt(TNumericLimits<float>::Max(), true), 0.f, 0.0001f);
	TestTrue(TEXT("A top tier's notice flares harder"), NoticeFlareAt(0.1f, true) > NoticeFlareAt(0.1f, false) && NoticeFlareAt(0.1f, false) > 0.f);
	TestNearlyEqual(TEXT("...and is over by its end"), NoticeFlareAt(NoticeFlareSeconds, true), 0.f, 0.0001f);
	TestNearlyEqual(TEXT("Alive: shown whole"), DeathFadeAt(-1.f), 1.f, 0.0001f);
	TestTrue(TEXT("Dying, it flares as its soul goes"), DeathFlareAt(0.f) > 1.f);
	TestNearlyEqual(TEXT("...and is gone by the fade's end"), DeathFadeAt(DeathFadeSeconds), 0.f, 0.0001f);
	float Last = 1.f;
	bool bOnlyDown = true;
	for (float Since = 0.f; Since <= DeathFadeSeconds; Since += 0.05f)
	{
		bOnlyDown = bOnlyDown && DeathFadeAt(Since) <= Last + 0.0001f;
		Last = DeathFadeAt(Since);
	}
	TestTrue(TEXT("...only ever fading"), bOnlyDown);

	TestTrue(TEXT("Only the top tiers colour a kill's XP"), TintsKillXP(Hallowed) && TintsKillXP(Ethereal) && !TintsKillXP(Gravebound));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTierLookShellTest, "Looter.Creatures.TierLook.Shell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTierLookShellTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Spider = Cast<ASpiderCreature>(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector::ZeroVector));
	ASpiderCreature* Other = Cast<ASpiderCreature>(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(600.0, 0.0, 0.0)));
	AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), FVector(-600.0, 0.0, 0.0)));
	if (!TestNotNull(TEXT("A spider"), Spider) || !TestNotNull(TEXT("Another"), Other) || !TestNotNull(TEXT("An Unpaid"), Unpaid))
	{
		return false;
	}
	BossTestWorld::NoLoot(Spider);
	BossTestWorld::NoLoot(Other);
	BossTestWorld::NoLoot(Unpaid);

	// Regular: nothing of the tiers at all.
	TestNull(TEXT("A Regular creature has no tier look"), UCreatureTierLookComponent::Find(Spider));

	// Restless: the look is made; with the model and M_CreatureTier, the shell follows the body's pose.
	Spider->SetRank(ECreatureRank::Rare);
	UCreatureTierLookComponent* Look = UCreatureTierLookComponent::Find(Spider);
	if (!TestNotNull(TEXT("Promoted to Restless: a tier look"), Look))
	{
		return false;
	}
	TestTrue(TEXT("...at its rank"), Look->GetShownRank() == ECreatureRank::Rare);
	TestTrue(TEXT("...its eyes to glow"), Look->GetShellSpec().bWorn && Look->GetShellSpec().bEyes);
	const bool bCanWear = Spider->GetMesh() && Spider->GetMesh()->GetSkeletalMeshAsset() && TierLookHasShellMaterial();
	if (!bCanWear)
	{
		AddWarning(TEXT("No SK_Spider or no M_CreatureTier (run Tools/Unreal/build_tier_material.py): the shell itself isn't checked."));
	}
	else
	{
		USkeletalMeshComponent* Shell = Look->GetShell();
		if (TestNotNull(TEXT("It wears a shell"), Shell))
		{
			TestTrue(TEXT("...tagged as one"), UCreatureTierLookComponent::IsTierShell(Shell));
			TestTrue(TEXT("...that never collides"), Shell->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestFalse(TEXT("...and casts no shadow"), Shell->CastShadow);
			TestTrue(TEXT("...on the body's pose"), Shell->LeaderPoseComponent.Get() == Spider->GetMesh());
			TestTrue(TEXT("...in the tier's look"), Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0)) != nullptr);
			TestNearlyEqual(TEXT("...shown whole"), Look->GetFade(), 1.f, 0.0001f);
		}

		// Two Ethereal spiders share one look; the rim is on.
		Spider->SetRank(ECreatureRank::Ethereal);
		Other->SetRank(ECreatureRank::Ethereal);
		UCreatureTierLookComponent* OtherLook = UCreatureTierLookComponent::Find(Other);
		TestTrue(TEXT("Ethereal: the ghost-rim too"), Look->GetShellSpec().bRim);
		TestTrue(TEXT("Two Ethereal spiders wear one shared look"), OtherLook && Look->GetShell() && OtherLook->GetShell()
			&& Look->GetShell()->GetMaterial(0) == OtherLook->GetShell()->GetMaterial(0));

		// A hit flash goes on the body, never on its shell.
		Spider->GetReact()->FlashHit(false);
		TestNull(TEXT("The hit flash passes the shell by"), Look->GetShell() ? Look->GetShell()->GetOverlayMaterial() : nullptr);

		// Turning on the player (its rank sting) flares the glow; it dies and the glow goes out.
		TierLookRun(*Look, 0.1f);
		const float RestingFlare = Look->GetFlare();
		Look->SetHeldFlare(1.f);
		TierLookRun(*Look, 0.05f);
		TestTrue(TEXT("A trait's held flare brightens it"), Look->GetFlare() > RestingFlare + 0.5f);
		Look->SetHeldFlare(0.f);
		BossTestWorld::Kill(Spider);
		TestTrue(TEXT("Killed: its death show waits for its next frame (a top tier)"), Look->IsDeathShowWaiting());
		Look->Advance(TierLookFrame);
		const bool bSwelling = Spider->GetTraitComponent() && Spider->GetTraitComponent()->IsSwelling();
		TestTrue(TEXT("...then plays (or waits out a Grave-burst's swell)"), Look->HasShownDeath() || bSwelling);
		if (!bSwelling)
		{
			TierLookRun(*Look, CreatureTierLook::DeathFadeSeconds + 0.1f);
			TestTrue(TEXT("...and its glow goes out"), Look->GetFade() < 0.02f && Look->GetShell() && !Look->GetShell()->IsVisible());
		}
	}

	// Back to Regular (a promotion's life over): the shell comes off.
	Other->SetRank(ECreatureRank::Basic);
	UCreatureTierLookComponent* OtherLook = UCreatureTierLookComponent::Find(Other);
	TestTrue(TEXT("Regular again: no shell"), OtherLook && OtherLook->GetShell() == nullptr && !OtherLook->GetShellSpec().bWorn);

	// An Unpaid: no shell below Ethereal (its coal burns in the tier's colour), the rim at Ethereal.
	Unpaid->SetRank(ECreatureRank::Hallowed);
	UCreatureTierLookComponent* GhostLook = UCreatureTierLookComponent::Find(Unpaid);
	TestTrue(TEXT("A Hallowed Unpaid has a tier look (its death show) but no shell"), GhostLook && !GhostLook->GetShellSpec().bWorn);
	TestTrue(TEXT("...its coal in the tier's glow"), Unpaid->GetCoalColor().Equals(CreatureTierLook::GlowColor(ECreatureRank::Hallowed)));
	Unpaid->SetRank(ECreatureRank::Ethereal);
	TestTrue(TEXT("An Ethereal Unpaid wears the ghost-rim"), GhostLook && GhostLook->GetShellSpec().bRim);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTierLookDeathShowTest, "Looter.Creatures.TierLook.DeathShow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTierLookDeathShowTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UCreatureTierFXSubsystem* Effects = UCreatureTierFXSubsystem::Get(World);
	if (!TestNotNull(TEXT("The tiers' effects"), Effects))
	{
		return false;
	}
	const FVector Center(0.0, 0.0, 60.0);
	const FVector Feet = FVector::ZeroVector;
	TestFalse(TEXT("A Gravebound death has no show"), Effects->PlayDeathAt(ECreatureRank::Epic, Center, Feet, 100.f, Center, true));
	TestEqual(TEXT("...nothing playing"), Effects->NumShows(), 0);
	TestFalse(TEXT("Nothing to tick"), Effects->IsTickable());

	// A Hallowed death: the pillar shoots up, the ring and motes are drawn, the loot pops a beat later.
	TestTrue(TEXT("A Hallowed death plays"), Effects->PlayDeathAt(ECreatureRank::Hallowed, Center, Feet, 120.f, Center, true));
	TestTrue(TEXT("...and ticks"), Effects->IsTickable());
	TestEqual(TEXT("...its pillar and its core lit"), Effects->NumPillarsLit(), 2);
	TestEqual(TEXT("No pop yet"), Effects->GetPopsPlayed(), 0);
	const FCreatureTierDeathShow Look = CreatureTierLook::DeathShowFor(ECreatureRank::Hallowed);
	TierShowRun(*Effects, Look.PopDelay + 0.05f);
	TestEqual(TEXT("The loot popped"), Effects->GetPopsPlayed(), 1);
	TestTrue(TEXT("...with its ring, motes and glints drawn"), Effects->GetStreaksDrawn() > UCreatureTierFXSubsystem::RingSegments * 2);
	TierShowRun(*Effects, Look.Seconds);
	TestEqual(TEXT("Over: nothing playing"), Effects->NumShows(), 0);
	TestEqual(TEXT("...its pillars given back"), Effects->NumPillarsLit(), 0);

	// A death that drops nothing (its loot off): no pop.
	Effects->PlayDeathAt(ECreatureRank::Ethereal, Center, Feet, 120.f, Center, false);
	TierShowRun(*Effects, CreatureTierLook::DeathShowFor(ECreatureRank::Ethereal).Seconds + 0.1f);
	TestEqual(TEXT("No drops: no pop"), Effects->GetPopsPlayed(), 1);

	// A horde of Ethereals dying together: never more than MaxShows at once, every pillar back afterwards.
	for (int32 Index = 0; Index < UCreatureTierFXSubsystem::MaxShows + 3; ++Index)
	{
		Effects->PlayDeathAt(ECreatureRank::Ethereal, Center + FVector(200.0 * Index, 0.0, 0.0), Feet + FVector(200.0 * Index, 0.0, 0.0), 130.f,
			Center, true);
		Effects->Advance(TierLookFrame);
	}
	TestEqual(TEXT("At most MaxShows at once"), Effects->NumShows(), UCreatureTierFXSubsystem::MaxShows);
	TestTrue(TEXT("...two pillars each, at most"), Effects->NumPillarsLit() <= UCreatureTierFXSubsystem::MaxShows * 2);
	TierShowRun(*Effects, CreatureTierLook::DeathShowFor(ECreatureRank::Ethereal).Seconds + 0.1f);
	TestEqual(TEXT("All over"), Effects->NumShows(), 0);
	TestEqual(TEXT("...every pillar back"), Effects->NumPillarsLit(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTierLookTouchedTest, "Looter.Creatures.TierLook.Touched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTierLookTouchedTest::RunTest(const FString& Parameters)
{
	using namespace CreatureTierLook;
	const ECreatureRank Regular = ECreatureRank::Basic;
	const ECreatureRank Restless = ECreatureRank::Rare;
	const ELooterElement Cinder = ELooterElement::Ember;
	const ELooterElement Thunder = ELooterElement::Thunder;

	// Who wears a shell: a Touched Basic one now too; never a boss or a Legendary monster, and a plain Basic one still not.
	TestFalse(TEXT("A plain Regular creature wears no shell"), WearsShell(Regular, ELooterElement::None));
	TestTrue(TEXT("A Touched Regular one does"), WearsShell(Regular, Cinder));
	TestTrue(TEXT("A Restless one does as ever"), WearsShell(Restless, ELooterElement::None) && WearsShell(Restless, Cinder));
	TestFalse(TEXT("A Touched boss or Legendary monster never"), WearsShell(ECreatureRank::Boss, Cinder) || WearsShell(ECreatureRank::Legendary, Cinder));

	// Its shell: the Restless glow in its element's colour, a key of its own per element, its tier's tag untouched.
	const FCreatureTierShell Plain = ShellFor(ECreatureTierBody::Spider, Regular);
	const FCreatureTierShell Touched = ShellFor(ECreatureTierBody::Spider, Regular, Cinder);
	TestFalse(TEXT("A plain Regular spider: no shell"), Plain.bWorn);
	TestTrue(TEXT("A Touched Regular spider's eyes glow"), Touched.bWorn && Touched.bEyes && Touched.Touched == Cinder);
	TestNearlyEqual(TEXT("...at the Restless glow"), Touched.Glow, ShellFor(ECreatureTierBody::Spider, Restless).Glow, 0.0001f);
	TestTrue(TEXT("...in its element's colour"), ShellColor(Touched, Regular).Equals(ElementRules::FxColor(Cinder)));
	TestTrue(TEXT("...a plain shell in its tier's"), ShellColor(ShellFor(ECreatureTierBody::Spider, Restless), Restless).Equals(GlowColor(Restless)));
	const FCreatureTierShell Storm = ShellFor(ECreatureTierBody::Spider, Regular, Thunder);
	TestTrue(TEXT("Each element has its own shared look"), Touched.Key != Storm.Key && Touched.Key != ShellFor(ECreatureTierBody::Spider, Restless).Key
		&& !Touched.Key.IsNone());
	const FCreatureTierShell TouchedRestless = ShellFor(ECreatureTierBody::Spider, Restless, Cinder);
	TestTrue(TEXT("A Touched Restless one burns in the element's colour too, under another key than a plain Restless"),
		TouchedRestless.Touched == Cinder && TouchedRestless.Key != ShellFor(ECreatureTierBody::Spider, Restless).Key);
	TestTrue(TEXT("The slime's core lights in it as well"), ShellFor(ECreatureTierBody::Slime, Regular, Thunder).bCore);
	const FCreatureTierShell Miner = ShellFor(ECreatureTierBody::Unpaid, Regular, ELooterElement::Rot);
	TestTrue(TEXT("A Touched Unpaid, with no eyes or core to light, shows the rim"), Miner.bWorn && Miner.bRim && !Miner.bEyes && !Miner.bCore && Miner.bShroudFade);
	TestFalse(TEXT("Its frame stays its rank's: a Touched Regular one's tag has none"), TagFor(Regular).bThickFrame || TagFor(Regular).bTopTierFlash);

	// The tag's name gains the word in front of the kind's name, after the rank's word.
	const FText Base = FText::FromString(TEXT("Brown Spider"));
	TestEqual(TEXT("A plain creature's name is as it was"), TagName(Base, ELooterElement::None).ToString(), Base.ToString());
	const FString Named = TagName(Base, Cinder).ToString();
	TestTrue(FString::Printf(TEXT("A Touched one's name leads with the word and keeps the kind (%s)"), *Named),
		Named.EndsWith(TEXT("Brown Spider")) && Named.Contains(TEXT("touched")) && Named.Len() > Base.ToString().Len() + 8);
	return true;
}

#endif
