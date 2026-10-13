#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/GrenadePickup.h"
#include "Loot/LostLootStore.h"
#include "Loot/LostLootSubsystem.h"
#include "Player/PlayerThrowComponent.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

namespace
{
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	/** The grenade tins lying in a level that are still there (not one being removed). */
	TArray<AGrenadePickup*> TinsOnGround(UWorld* World)
	{
		TArray<AGrenadePickup*> Tins;
		for (TActorIterator<AGrenadePickup> It(World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed())
			{
				Tins.Add(*It);
			}
		}
		return Tins;
	}

	/** The tin holding Amount grenades, or null. */
	AGrenadePickup* TinOf(const TArray<AGrenadePickup*>& Tins, int32 Amount)
	{
		AGrenadePickup* const* Found = Tins.FindByPredicate([Amount](const AGrenadePickup* Tin) { return Tin->GetAmount() == Amount; });
		return Found ? *Found : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootGrenadesSavedTest, "Looter.Loot.Grenades.Saved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootGrenadesSavedTest::RunTest(const FString& Parameters)
{
	// Grave-salt grenade tins lying about are kept with the map's world as ammo and silver are (the user's yes, 2026-10-13):
	// where they lay, what they held and the seconds each had left, so a Save & Quit then Continue gives no new life, and a
	// tin whose seconds run out is gone. They are not Lost Property's (only guns go there). A save from before reads as none.
	FTestWorldWrapper Level;
	if (!TestTrue(TEXT("Test level made"), Level.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Level.GetTestWorld();
	const FVector PairAt(300.0, 0.0, 50.0);
	const FVector SingleAt(-300.0, 100.0, 50.0);
	AGrenadePickup* Pair = AGrenadePickup::SpawnGrenades(World, 2, PairAt);
	AGrenadePickup* Single = AGrenadePickup::SpawnGrenades(World, 1, SingleAt);
	AGrenadePickup* Spent = AGrenadePickup::SpawnGrenades(World, 1, FVector(0.0, 400.0, 50.0));
	if (!TestNotNull(TEXT("A pair of grenades lying there"), Pair) || !TestNotNull(TEXT("A single one"), Single)
		|| !TestNotNull(TEXT("And one about to be picked up"), Spent))
	{
		return false;
	}
	// Four minutes and twenty seconds on for the pair (forty seconds left of five minutes); the single is newer.
	Pair->SetLifeSpan(40.f);
	Single->SetLifeSpan(200.f);
	// A tin already collected is not kept: it's gone, whatever it still says it held.
	UPlayerThrowComponent* Hand = NewObject<UPlayerThrowComponent>();
	Spent->CollectInto(*Hand);

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(World, *Save);
	ULooterSessionSave* Loaded = WriteAndRead(Save);
	const FString Map = USessionSubsystem::MapOf(World);
	const FSavedMapWorld* Here = Loaded ? Loaded->FindWorld(Map) : nullptr;
	if (!TestNotNull(TEXT("Saved and read back"), Here) || !TestEqual(TEXT("Two tins kept, not the collected one"), Here->GrenadePickups.Num(), 2))
	{
		return false;
	}
	const FSavedGrenadePickup* SavedPair = Here->GrenadePickups.FindByPredicate([](const FSavedGrenadePickup& Tin) { return Tin.Amount == 2; });
	const FSavedGrenadePickup* SavedSingle = Here->GrenadePickups.FindByPredicate([](const FSavedGrenadePickup& Tin) { return Tin.Amount == 1; });
	if (!TestNotNull(TEXT("The pair kept with its two"), SavedPair) || !TestNotNull(TEXT("The single kept with its one"), SavedSingle))
	{
		return false;
	}
	TestTrue(TEXT("Each is kept where it lay"), SavedPair->Location.Equals(PairAt, 0.1) && SavedSingle->Location.Equals(SingleAt, 0.1));
	TestNearlyEqual(TEXT("The pair's time left is kept"), SavedPair->LifeLeft, 40.f, 0.01f);
	TestNearlyEqual(TEXT("The single's time left is kept"), SavedSingle->LifeLeft, 200.f, 0.01f);

	// Continue: the level starts again and the session puts back what was lying there, and only that.
	USessionSubsystem::RestoreWorld(World, *Loaded);
	TArray<AGrenadePickup*> Back = TinsOnGround(World);
	AGrenadePickup* BackPair = TinOf(Back, 2);
	AGrenadePickup* BackSingle = TinOf(Back, 1);
	if (!TestEqual(TEXT("Two tins on the ground, the old ones replaced by the save's"), Back.Num(), 2) || !TestNotNull(TEXT("The pair is back"), BackPair)
		|| !TestNotNull(TEXT("The single is back"), BackSingle))
	{
		return false;
	}
	TestTrue(TEXT("They lie about where they did"), FVector::Dist(BackPair->GetActorLocation(), PairAt) < 50.0
		&& FVector::Dist(BackSingle->GetActorLocation(), SingleAt) < 50.0);
	TestNearlyEqual(TEXT("The pair goes on with the forty seconds it had"), BackPair->GetLifeSpan(), 40.f, 0.01f);
	TestNearlyEqual(TEXT("The single with its two hundred"), BackSingle->GetLifeSpan(), 200.f, 0.01f);
	const ULostLootSubsystem* Shelves = ULostLootSubsystem::Get(World);
	TestTrue(TEXT("No tin went to Lost Property"), Shelves && Shelves->GetStore().Num() == 0);

	// When its seconds run out it's gone, as on the ground: the lifespan's timer is what ends it.
	TWeakObjectPtr<AGrenadePickup> PairGone(BackPair);
	BackPair->LifeSpanExpired();
	TestFalse(TEXT("The pair vanishes when its time is up"), PairGone.IsValid());

	// A save that kept no time left (none was saved) leaves the tin its whole life; one claiming more than a life gets no more.
	FSavedMapWorld& Kept = Loaded->FindOrAddWorld(Map);
	for (FSavedGrenadePickup& Tin : Kept.GrenadePickups)
	{
		Tin.LifeLeft = Tin.Amount == 2 ? 0.f : 100000.f;
	}
	USessionSubsystem::RestoreWorld(World, *Loaded);
	Back = TinsOnGround(World);
	BackPair = TinOf(Back, 2);
	BackSingle = TinOf(Back, 1);
	if (TestEqual(TEXT("Two tins again"), Back.Num(), 2) && TestNotNull(TEXT("The pair"), BackPair) && TestNotNull(TEXT("The single"), BackSingle))
	{
		TestNearlyEqual(TEXT("No time kept: a whole life"), BackPair->GetLifeSpan(), BackPair->LifeSeconds, 0.01f);
		TestNearlyEqual(TEXT("More than a life kept: no more than a whole one"), BackSingle->GetLifeSpan(), BackSingle->LifeSeconds, 0.01f);
	}

	// A save from before the tins were kept has none: the world it puts back has none lying about.
	TestTrue(TEXT("Before the tins: none kept"), FSavedMapWorld().GrenadePickups.IsEmpty());
	ULooterSessionSave* Old = NewObject<ULooterSessionSave>();
	Old->Version = ULooterSessionSave::CurrentVersion;
	Old->FindOrAddWorld(Map);
	USessionSubsystem::RestoreWorld(World, *Old);
	TestEqual(TEXT("An older save puts back no tins"), TinsOnGround(World).Num(), 0);
	return true;
}

#endif
