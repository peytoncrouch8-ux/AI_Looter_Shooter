#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Elements/ElementCallout.h"
#include "UI/World/DamageNumberActor.h"
#include "UI/World/DamageNumberSubsystem.h"
#include "UI/World/DamageNumberWidget.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"

// The damage numbers' pool and merge (change 2 of the 2026-11-03 audit; UDamageNumberSubsystem): the merge window and looks as
// plain rules (Looter.UI.DamageNumbers.Merge), the pool in a test level (.Pool: made at play start, used again, a shotgun
// blast one number summing its pellets, a crit pellet making it a crit, a merged number rising on, nothing spawned per hit),
// and a status's stream going through the pool as it did before (.Stream). The stream's own math is Looter.UI.StatusIcons'.

namespace
{
	constexpr float DamageNumberTestStep = 1.f / 30.f;

	/** Moves a number on Seconds in steps, as its tick would (a test level doesn't tick). */
	void DamageNumberRun(ADamageNumberActor& Number, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += DamageNumberTestStep)
		{
			Number.Advance(DamageNumberTestStep);
		}
	}

	/** A plain pellet of Damage on Target at Where. */
	FDamageNumberHit DamageNumberPellet(AActor* Target, float Damage, const FVector& Where = FVector(0.0, 0.0, 100.0))
	{
		FDamageNumberHit Hit;
		Hit.Target = Target;
		Hit.Damage = Damage;
		Hit.Where = Where;
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageNumberMergeRulesTest, "Looter.UI.DamageNumbers.Merge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDamageNumberMergeRulesTest::RunTest(const FString& Parameters)
{
	using namespace DamageNumberMerge;
	using EE = ELooterElement;
	const FTuning Tuning;

	// The window: from the number's first hit, 80 ms.
	TestNearlyEqual(TEXT("The window is 80 ms"), Tuning.MergeSeconds, 0.08f, 0.0001f);
	TestTrue(TEXT("A pellet landing with the blast's first joins it"), Joins(10.0, 10.0, Tuning.MergeSeconds));
	TestTrue(TEXT("...one 70 ms on still does"), Joins(10.0, 10.07, Tuning.MergeSeconds));
	TestFalse(TEXT("...past the window: a number of its own"), Joins(10.0, 10.1, Tuning.MergeSeconds));
	TestFalse(TEXT("A rifle's next round (92 ms at 650 rpm) is its own number"), Joins(10.0, 10.0 + 60.0 / 650.0, Tuning.MergeSeconds));
	TestFalse(TEXT("A clock that went back (a new world) joins nothing"), Joins(10.0, 9.9, Tuning.MergeSeconds));
	TestFalse(TEXT("0 turns merging off"), Joins(10.0, 10.0, 0.f));

	// Which looks share a number.
	FElementNumberLook Plain;
	FElementNumberLook EmberPlain;
	EmberPlain.Element = EE::Ember;
	FElementNumberLook EmberCrit = EmberPlain;
	EmberCrit.bTint = true;
	FElementNumberLook Smite;
	Smite.Element = EE::SoulLight;
	Smite.bTint = true;
	FElementNumberLook SaltPlain;
	SaltPlain.Element = EE::GraveSalt;
	TestTrue(TEXT("Plain pellets share a number"), SameLook(EE::None, false, Plain, false));
	TestTrue(TEXT("...a crit pellet joins them"), SameLook(EE::None, false, Plain, true));
	TestTrue(TEXT("An element gun's crit pellet joins its plain pellets"), SameLook(EE::Ember, false, EmberCrit, true));
	TestFalse(TEXT("A smite's burst keeps its own number beside a gun's hit"), SameLook(EE::None, false, Smite, false));
	TestFalse(TEXT("...even a Soul-light gun's"), SameLook(EE::SoulLight, false, Smite, false));
	TestTrue(TEXT("...and joins another of its own kind"), SameLook(EE::SoulLight, true, Smite, false));
	TestFalse(TEXT("Another element: its own number"), SameLook(EE::Ember, false, SaltPlain, false));

	// The sum's look.
	FElementNumberLook EmberWeak = EmberPlain;
	EmberWeak.Callout = EElementAffinity::Weak;
	EmberWeak.SizeScale = ElementCallout::WeakSize;
	const FElementNumberLook WeakFirst = Merged(EmberWeak, EmberPlain);
	TestTrue(TEXT("The fight's WEAK stays on the sum, a size up"), WeakFirst.Callout == EElementAffinity::Weak && WeakFirst.SizeScale == ElementCallout::WeakSize);
	const FElementNumberLook WeakLater = Merged(EmberPlain, EmberWeak);
	TestTrue(TEXT("...a later pellet's WEAK joins it"), WeakLater.Callout == EElementAffinity::Weak && WeakLater.SizeScale == ElementCallout::WeakSize);
	FElementNumberLook Resist = SaltPlain;
	Resist.Callout = EElementAffinity::Resist;
	TestTrue(TEXT("...and a RESIST"), Merged(SaltPlain, Resist).Callout == EElementAffinity::Resist && Merged(SaltPlain, Resist).SizeScale == 1.f);
	const FElementNumberLook CritTinted = Merged(EmberPlain, EmberCrit);
	TestTrue(TEXT("An element crit pellet tints the sum's crit toward its element"), CritTinted.bTint && CritTinted.Element == EE::Ember);
	const FElementNumberLook StillPlain = Merged(Plain, Plain);
	TestTrue(TEXT("Plain pellets stay a plain number"), !StillPlain.bTint && StillPlain.Element == EE::None
		&& StillPlain.Callout == EElementAffinity::Neutral && StillPlain.SizeScale == 1.f);

	// The sum's size: a blast is one big number, still under a crit's.
	TestEqual(TEXT("One hit: its own size"), GrowFor(1, Tuning), 1.f);
	TestNearlyEqual(TEXT("Three: a little bigger"), GrowFor(3, Tuning), 1.f + 2.f * Tuning.Grow, 0.0001f);
	TestNearlyEqual(TEXT("A twelve-pellet blast: capped"), GrowFor(12, Tuning), FMath::Min(1.f + 11.f * Tuning.Grow, Tuning.GrowMax), 0.0001f);
	TestTrue(TEXT("...a big number, under a single crit's"), GrowFor(12, Tuning) * UDamageNumberWidget::NormalSize < UDamageNumberWidget::CriticalSize);
	TestTrue(TEXT("The pool keeps 16-24 numbers"), Tuning.PoolSize >= 16 && Tuning.PoolSize <= 24);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageNumberPoolTest, "Looter.UI.DamageNumbers.Pool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDamageNumberPoolTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UDamageNumberSubsystem* Numbers = UDamageNumberSubsystem::Get(World);
	if (!TestNotNull(TEXT("The damage numbers"), Numbers))
	{
		return false;
	}
	const DamageNumberMerge::FTuning Tuning = DamageNumberMerge::FTuning::Live();
	Numbers->Prewarm();
	TestEqual(TEXT("Made at play start: the whole pool"), Numbers->NumPooled(), Tuning.PoolSize);
	TestEqual(TEXT("...none on show"), Numbers->NumShowing(), 0);
	const int32 MadeWarm = Numbers->NumMade();
	AActor* Spider = World->SpawnActor<AActor>();
	AActor* Other = World->SpawnActor<AActor>();
	if (!TestTrue(TEXT("Two bodies to hit"), Spider && Other))
	{
		return false;
	}

	// A shotgun blast: eight pellets of 12 on one body inside the window are one number, their sum, where the first landed.
	const FVector First(0.0, 0.0, 100.0);
	ADamageNumberActor* Blast = Numbers->ShowHitAt(DamageNumberPellet(Spider, 12.f, First), 5.0);
	if (!TestNotNull(TEXT("The blast's number"), Blast))
	{
		return false;
	}
	for (int32 Index = 1; Index < 8; ++Index)
	{
		TestTrue(TEXT("Each pellet joins the blast's number"),
			Numbers->ShowHitAt(DamageNumberPellet(Spider, 12.f, First + FVector(Index * 4.0, 0.0, 0.0)), 5.0 + Index * 0.005) == Blast);
	}
	TestEqual(TEXT("One number on show"), Numbers->NumShowing(), 1);
	TestEqual(TEXT("...summing the pellets"), Blast->GetShownDamage(), 96.f);
	TestEqual(TEXT("...all eight in it"), Blast->GetMergedHits(), 8);
	TestFalse(TEXT("...plain so far"), Blast->IsCritical());
	TestTrue(TEXT("...where the first pellet landed"), Blast->GetActorLocation().Equals(First));

	// It rises; a crit pellet still inside the window makes it a crit where it is, and it keeps rising.
	DamageNumberRun(*Blast, 0.03f);
	const FVector Risen = Blast->GetActorLocation();
	TestTrue(TEXT("It rises"), Risen.Z > First.Z);
	FDamageNumberHit CritPellet = DamageNumberPellet(Spider, 24.f, First + FVector(50.0, 0.0, 0.0));
	CritPellet.bCritical = true;
	TestTrue(TEXT("A crit pellet joins the blast"), Numbers->ShowHitAt(CritPellet, 5.06) == Blast);
	TestTrue(TEXT("...the number is a crit's now"), Blast->IsCritical() && Blast->GetShownDamage() == 120.f);
	TestTrue(TEXT("...it stays where it had risen to"), Blast->GetActorLocation().Equals(Risen));
	TestNearlyEqual(TEXT("...and lives a crit's life"), Blast->GetLife(), Blast->Lifetime + Blast->CriticalExtraLife, 0.001f);
	DamageNumberRun(*Blast, 0.05f);
	TestTrue(TEXT("...rising on"), Blast->GetActorLocation().Z > Risen.Z);
	FDamageNumberHit Plain = DamageNumberPellet(Spider, 12.f);
	TestTrue(TEXT("A plain pellet after it keeps the crit look"), Numbers->ShowHitAt(Plain, 5.07) == Blast && Blast->IsCritical());

	// Another body in the same moment has its own number; the same body's next blast, after the window, a new one.
	ADamageNumberActor* OtherNumber = Numbers->ShowHitAt(DamageNumberPellet(Other, 12.f), 5.06);
	TestTrue(TEXT("Another body hit in the same blast: its own number"), OtherNumber && OtherNumber != Blast && OtherNumber->GetShownDamage() == 12.f);
	ADamageNumberActor* NextBlast = Numbers->ShowHitAt(DamageNumberPellet(Spider, 12.f), 5.2);
	TestTrue(TEXT("The same body's next blast: a number of its own"), NextBlast && NextBlast != Blast && NextBlast->GetMergedHits() == 1);
	TestEqual(TEXT("No actor spawned for any of these hits"), Numbers->NumMade(), MadeWarm);

	// Done: hidden, kept, used again.
	for (ADamageNumberActor* Shown : { Blast, OtherNumber, NextBlast })
	{
		if (Shown)
		{
			DamageNumberRun(*Shown, Shown->GetLife() + 0.1f);
		}
	}
	TestFalse(TEXT("Done: off the screen"), Blast->IsShowing());
	TestTrue(TEXT("...kept in the pool, not destroyed"), IsValid(Blast) && Numbers->NumPooled() == Tuning.PoolSize && Numbers->NumShowing() == 0);
	ADamageNumberActor* Again = Numbers->ShowHitAt(DamageNumberPellet(Spider, 7.f), 9.0);
	TestTrue(TEXT("The next hit uses one again"), Again && Again->IsShowing() && Again->GetShownDamage() == 7.f && !Again->IsCritical()
		&& Numbers->NumMade() == MadeWarm);

	// A burst past the pool: never more made; the oldest hit's number is taken over.
	ADamageNumberActor* Latest = nullptr;
	for (int32 Index = 0; Index < Tuning.PoolSize + 6; ++Index)
	{
		AActor* Body = World->SpawnActor<AActor>();
		if (ADamageNumberActor* Shown = Numbers->ShowHitAt(DamageNumberPellet(Body, 10.f + Index), 10.0 + Index * 0.001))
		{
			Latest = Shown;
			DamageNumberRun(*Shown, 0.01f);
		}
	}
	TestEqual(TEXT("At most the pool's numbers"), Numbers->NumPooled(), Tuning.PoolSize);
	TestEqual(TEXT("...all on show"), Numbers->NumShowing(), Tuning.PoolSize);
	TestEqual(TEXT("...none spawned for the burst"), Numbers->NumMade(), MadeWarm);
	TestTrue(TEXT("...the latest hit's shown"), Latest && Latest->GetShownDamage() == 10.f + Tuning.PoolSize + 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageNumberStreamPoolTest, "Looter.UI.DamageNumbers.Stream",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDamageNumberStreamPoolTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UDamageNumberSubsystem* Numbers = UDamageNumberSubsystem::Get(World);
	AActor* Spider = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("The damage numbers"), Numbers) || !TestNotNull(TEXT("A burning body"), Spider))
	{
		return false;
	}
	Numbers->Prewarm();
	const int32 MadeWarm = Numbers->NumMade();

	// A burn's pieces on a creature: one stream rolling up their total.
	FDamageNumberHit Piece = DamageNumberPellet(Spider, 4.f);
	Piece.bStatusPiece = true;
	Piece.Look.Element = ELooterElement::Ember;
	Piece.Look.bTint = true;
	ADamageNumberActor* Stream = Numbers->ShowHitAt(Piece, 1.0);
	if (!TestNotNull(TEXT("The burn's stream"), Stream))
	{
		return false;
	}
	TestTrue(TEXT("It's a stream, found as the creature's Ember stream"), Stream->GetKind() == EDamageNumberKind::Stream
		&& Numbers->FindStream(Spider, ELooterElement::Ember) == Stream && Numbers->CountStreams(Spider) == 1);
	TestTrue(TEXT("The next piece joins it"), Numbers->ShowHitAt(Piece, 1.5) == Stream && Stream->GetStreamTotal() == 8.f);
	DamageNumberRun(*Stream, 0.5f);
	TestNearlyEqual(TEXT("...rolling up to its total"), Stream->GetStreamShown(), 8.f, 0.001f);

	// A gun's hit on the burning creature is a number of its own, and a piece in the same moment still the stream's.
	FDamageNumberHit Shot = DamageNumberPellet(Spider, 20.f);
	Shot.Look.Element = ELooterElement::Ember;
	ADamageNumberActor* ShotNumber = Numbers->ShowHitAt(Shot, 1.5);
	TestTrue(TEXT("A shot on it: its own number"), ShotNumber && ShotNumber != Stream && ShotNumber->GetKind() == EDamageNumberKind::Hit);
	TestTrue(TEXT("...a piece right after it: the stream's"), Numbers->ShowHitAt(Piece, 1.52) == Stream && Stream->GetStreamTotal() == 12.f
		&& ShotNumber && ShotNumber->GetShownDamage() == 20.f);

	// A second element on it is a stream of its own, a slot up.
	FDamageNumberHit Salt = Piece;
	Salt.Look.Element = ELooterElement::GraveSalt;
	ADamageNumberActor* SaltStream = Numbers->ShowHitAt(Salt, 1.6);
	TestTrue(TEXT("Salt on it: a second stream"), SaltStream && SaltStream != Stream && Numbers->CountStreams(Spider) == 2
		&& Numbers->FindStream(Spider, ELooterElement::GraveSalt) == SaltStream);

	// Spent once the pieces stop: back in the pool, found no more.
	DamageNumberRun(*Stream, DamageNumberStream::HoldSeconds + DamageNumberStream::FadeSeconds + 0.1f);
	TestFalse(TEXT("Spent: off the screen"), Stream->IsShowing());
	TestTrue(TEXT("...kept in the pool"), IsValid(Stream));
	TestNull(TEXT("...not the creature's stream any more"), Numbers->FindStream(Spider, ELooterElement::Ember));
	TestEqual(TEXT("...one stream left on it"), Numbers->CountStreams(Spider), 1);
	ADamageNumberActor* NewBurn = Numbers->ShowHitAt(Piece, 4.0);
	TestTrue(TEXT("A new burn starts a new stream"), NewBurn && NewBurn->GetKind() == EDamageNumberKind::Stream && NewBurn->GetStreamTotal() == 4.f);
	TestEqual(TEXT("No actor spawned for any piece"), Numbers->NumMade(), MadeWarm);
	return true;
}

#endif
