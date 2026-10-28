#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/Chest.h"
#include "Loot/ChestShow.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Shields/ShieldPickup.h"
#include "Weapons/WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The loot chest tiers' show (the user's "escalating show", 2026-10-28; Loot/ChestShow.h): each tier's length (about 0.6 s
// for tier 1 up to 2.2 s for tier 5) and its order (the latches, the lock, the orb and trim, the crest and burst all before
// the lid; the loot after it; tier 5's fountain one piece at a time, as many as it paid), a chest the session kept open
// snapping to the show's end with nothing played, a save taken mid-fountain throwing the rest, and the stand-in models
// (or the tiers' own, once imported, with their sockets and glow slots).

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** The design: from the tap to the lid open (s), by tier. */
	constexpr float ShowLengths[FChestKindInfo::NumTiers] = { 0.6f, 0.75f, 1.2f, 1.7f, 2.2f };

	AChest* SpawnChest(UWorld* World, int32 Tier, const FVector& Where)
	{
		const FTransform At(FRotator(0.0, 180.0, 0.0), Where);
		AChest* Chest = World->SpawnActorDeferred<AChest>(AChest::StaticClass(), At);
		if (!Chest)
		{
			return nullptr;
		}
		Chest->Kind = FChestKindInfo::TierKind(Tier);
		Chest->FinishSpawning(At);
		Chest->DispatchBeginPlay();
		return Chest;
	}

	/** Its show's pieces at their end: latches out (1-2), lock spun and green (3), trim lit (4: 10x at its height, 7x at rest), crest out (5). */
	bool PiecesDone(int32 Tier, const FChestShowPose& Pose)
	{
		switch (Tier)
		{
		case 1:
		case 2: return FMath::IsNearlyEqual(Pose.LatchLeft, ChestShow::LatchOpenPitch, 0.5f) && FMath::IsNearlyEqual(Pose.LatchRight, ChestShow::LatchOpenPitch, 0.5f);
		case 3: return FMath::IsNearlyEqual(Pose.LockRoll, ChestShow::LockTurnDegrees, 0.5f) && Pose.LockGreen >= 0.999f;
		case 4: return Pose.Glow[static_cast<int32>(EChestGlow::Trim)] >= 6.9f;
		case 5: return FMath::IsNearlyEqual(Pose.CrestPitch, ChestShow::CrestOpenPitch, 0.5f);
		default: return false;
		}
	}

	/** The guns and wards it threw that are still in the world. */
	int32 GunsOut(const AChest* Chest)
	{
		int32 Count = 0;
		for (const AActor* Loot : Chest->GetDroppedLoot())
		{
			Count += Cast<AWeaponBase>(Loot) || Cast<AShieldPickup>(Loot) ? 1 : 0;
		}
		return Count;
	}

	bool DefaultTableHasGuns()
	{
		const ULootTable* Default = ULootLibrary::GetDefaultLootTable();
		return Default && !Default->Entries.IsEmpty();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestShowTimingTest, "Looter.Loot.ChestShow.Timing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChestShowTimingTest::RunTest(const FString& Parameters)
{
	// Each tier's length from the tap to the lid open, climbing; its moments; its pose shut and at the end.
	const uint32 Moments[FChestKindInfo::NumTiers] = {
		ChestShowMoment::LatchLeft | ChestShowMoment::LatchRight | ChestShowMoment::LightFlare,
		ChestShowMoment::LatchLeft | ChestShowMoment::LatchRight | ChestShowMoment::LightFlare,
		ChestShowMoment::LockSpin | ChestShowMoment::LockOpen,
		ChestShowMoment::OrbPulse | ChestShowMoment::TrimSweep,
		ChestShowMoment::CrestUnfold | ChestShowMoment::Burst };
	float Last = 0.f;
	for (int32 Tier = 1; Tier <= FChestKindInfo::NumTiers; ++Tier)
	{
		const FChestKindInfo Info = FChestKindInfo::Get(FChestKindInfo::TierKind(Tier));
		const float Length = Info.PreSeconds + Info.LidSeconds;
		TestTrue(FString::Printf(TEXT("Tier %d: a show before the lid"), Tier), Info.PreMotion == EChestPreMotion::Show && Info.PreSeconds > 0.f);
		TestTrue(FString::Printf(TEXT("Tier %d: %.2f s to open (%.2f)"), Tier, Length, ShowLengths[Tier - 1]),
			FMath::IsNearlyEqual(Length, ShowLengths[Tier - 1], 0.01f));
		TestTrue(FString::Printf(TEXT("Tier %d: longer than the tier under it"), Tier), Length > Last);
		Last = Length;
		TestEqual(*FString::Printf(TEXT("Tier %d: its moments"), Tier), static_cast<int32>(ChestShow::AllMoments(Tier)), static_cast<int32>(Moments[Tier - 1]));
		TestEqual(*FString::Printf(TEXT("Tier %d: fountains its loot: %s"), Tier, Tier == 5 ? TEXT("yes") : TEXT("no")), ChestShow::Fountains(Tier), Tier == 5);

		const FChestShowPose Shut = ChestShow::PoseAt(Tier, 0.f, 0.f);
		TestTrue(FString::Printf(TEXT("Tier %d: shut, nothing moved"), Tier), Shut.LatchLeft == 0.f && Shut.LatchRight == 0.f
			&& Shut.LockRoll == 0.f && Shut.LockGreen == 0.f && Shut.CrestPitch == 0.f);
		const FChestShowPose End = ChestShow::PoseAt(Tier, 1.f, 1.f);
		TestTrue(FString::Printf(TEXT("Tier %d: at the end, its pieces out"), Tier), PiecesDone(Tier, End));
		TestTrue(FString::Printf(TEXT("Tier %d: at the end, no orb halo or sweep left"), Tier), End.OrbHalo == 0.f && End.Sweep < 0.f);
	}
	TestTrue(TEXT("Tier 1 opens in about 0.6 s, tier 5 in about 2.2 s"), ShowLengths[0] <= 0.65f && ShowLengths[4] <= 2.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestShowOrderTest, "Looter.Loot.ChestShow.Order",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChestShowOrderTest::RunTest(const FString& Parameters)
{
	// Each tier opened with a tap and run a frame at a time: its moments all played; its pieces out before the lid moves; the
	// loot out after the lid starts; open at its length. Tier 5: its guns fountain out one at a time, as many as it paid.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	const bool bGuns = DefaultTableHasGuns();
	if (!bGuns)
	{
		AddWarning(TEXT("The default loot table lists no guns in this checkout: tier 5's fountain isn't counted."));
	}
	for (int32 Tier = 1; Tier <= FChestKindInfo::NumTiers; ++Tier)
	{
		const FString Name = FString::Printf(TEXT("Tier %d"), Tier);
		AChest* Chest = SpawnChest(World, Tier, FVector(300.0, 400.0 * Tier, 0.0));
		if (!TestNotNull(*Name, Chest) || !TestTrue(FString::Printf(TEXT("%s: opened"), *Name), Chest->Open(nullptr)))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: its show first"), *Name), Chest->GetState() == EChestState::Unlocking);
		TestTrue(FString::Printf(TEXT("%s: its first moment at the tap"), *Name), Chest->GetShowMomentsPlayed() >= 1);
		float Time = 0.f;
		float PiecesAt = -1.f;
		float LidAt = -1.f;
		float LootAt = -1.f;
		float OpenAt = -1.f;
		int32 MostThrownAtOnce = 0;
		int32 FountainPlanned = -1;
		while (Time < 5.f && (Chest->GetState() != EChestState::Open || Chest->GetFountainThrown() < Chest->GetFountainCount()))
		{
			const int32 ThrownBefore = Chest->GetFountainThrown();
			const bool bHadLoot = Chest->HasGivenLoot();
			Chest->Advance(Frame);
			Time += Frame;
			if (PiecesAt < 0.f && PiecesDone(Tier, Chest->GetShowPose()))
			{
				PiecesAt = Time;
			}
			if (LidAt < 0.f && Chest->GetLidShare() > 0.f)
			{
				LidAt = Time;
			}
			if (LootAt < 0.f && Chest->HasGivenLoot())
			{
				LootAt = Time;
				FountainPlanned = Chest->GetFountainCount();
			}
			if (OpenAt < 0.f && Chest->GetState() == EChestState::Open)
			{
				OpenAt = Time;
			}
			if (bHadLoot)
			{
				MostThrownAtOnce = FMath::Max(MostThrownAtOnce, Chest->GetFountainThrown() - ThrownBefore);
			}
		}
		const float Length = Chest->GetShowSeconds();
		TestEqual(*FString::Printf(TEXT("%s: every moment played"), *Name), Chest->GetShowMomentsPlayed(),
			static_cast<int32>(FMath::CountBits(static_cast<uint64>(ChestShow::AllMoments(Tier)))));
		TestTrue(FString::Printf(TEXT("%s: its pieces out (%.2f s) before the lid moves (%.2f s)"), *Name, PiecesAt, LidAt),
			PiecesAt > 0.f && LidAt > 0.f && PiecesAt <= LidAt + 0.5f * Frame);
		TestTrue(FString::Printf(TEXT("%s: the loot out (%.2f s) after the lid starts"), *Name, LootAt), LootAt > LidAt);
		TestTrue(FString::Printf(TEXT("%s: open at %.2f s (%.2f)"), *Name, OpenAt, Length), FMath::Abs(OpenAt - Length) <= 2.f * Frame);
		TestTrue(FString::Printf(TEXT("%s: never stops the clock: still ticking only while something moves"), *Name),
			!Chest->IsActorTickEnabled() || Chest->GetFountainThrown() < Chest->GetFountainCount());

		if (Tier < 5)
		{
			TestEqual(*FString::Printf(TEXT("%s: no fountain"), *Name), Chest->GetFountainCount(), 0);
			continue;
		}
		const FChestKindInfo Info = Chest->GetKindInfo();
		if (bGuns)
		{
			TestEqual(*FString::Printf(TEXT("%s: its fountain is its guns (%d)"), *Name, Info.Guns), FountainPlanned, Info.Guns);
			TestEqual(*FString::Printf(TEXT("%s: every piece thrown"), *Name), Chest->GetFountainThrown(), FountainPlanned);
			TestEqual(*FString::Printf(TEXT("%s: one piece at a time"), *Name), MostThrownAtOnce, 1);
			TestEqual(*FString::Printf(TEXT("%s: the guns out in the world match the payout"), *Name), GunsOut(Chest), FountainPlanned);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestShowSnapOpenTest, "Looter.Loot.ChestShow.SnapOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChestShowSnapOpenTest::RunTest(const FString& Parameters)
{
	// A chest the session kept open: straight to its show's end (lid open, pieces out), nothing played or thrown, not ticking.
	// And a save taken while tier 5's fountain is still throwing: the rest come out first, so the save holds them all.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	for (int32 Tier = 1; Tier <= FChestKindInfo::NumTiers; ++Tier)
	{
		const FString Name = FString::Printf(TEXT("Tier %d"), Tier);
		AChest* Chest = SpawnChest(World, Tier, FVector(300.0, 400.0 * Tier, 0.0));
		if (!TestNotNull(*Name, Chest))
		{
			continue;
		}
		Chest->RestoreOpened();
		TestTrue(FString::Printf(TEXT("%s: open, its loot given, its lid at its angle"), *Name), Chest->GetState() == EChestState::Open
			&& Chest->HasGivenLoot() && FMath::IsNearlyEqual(Chest->GetLidAngle(), Chest->GetKindInfo().OpenAngle, 0.01f));
		TestTrue(FString::Printf(TEXT("%s: its pieces at the show's end"), *Name), PiecesDone(Tier, Chest->GetShowPose()));
		TestEqual(*FString::Printf(TEXT("%s: no moment played"), *Name), Chest->GetShowMomentsPlayed(), 0);
		TestTrue(FString::Printf(TEXT("%s: nothing thrown, nothing to throw, not ticking"), *Name), Chest->GetDroppedLoot().IsEmpty()
			&& Chest->GetFountainCount() == 0 && !Chest->IsActorTickEnabled());
		TestFalse(*FString::Printf(TEXT("%s: never opened again"), *Name), Chest->Open(nullptr, /*bForce*/ true));
	}

	if (!DefaultTableHasGuns())
	{
		AddWarning(TEXT("The default loot table lists no guns in this checkout: the mid-fountain save isn't checked."));
		return true;
	}
	AChest* Five = SpawnChest(World, 5, FVector(-400.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A tier 5"), Five) || !TestTrue(TEXT("Tier 5 opened"), Five->Open(nullptr)))
	{
		return false;
	}
	for (float Time = 0.f; Time < 4.f && !Five->HasGivenLoot(); Time += Frame)
	{
		Five->Advance(Frame);
	}
	TestTrue(TEXT("Tier 5: its loot out, its fountain still to throw"), Five->HasGivenLoot() && Five->GetFountainThrown() < Five->GetFountainCount());
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(World, *Save);
	TestTrue(TEXT("Tier 5: the save threw the rest first"), Five->GetFountainCount() > 0 && Five->GetFountainThrown() == Five->GetFountainCount());
	TestEqual(TEXT("Tier 5: every gun of it in the world"), GunsOut(Five), Five->GetFountainCount());
	const FSavedMapWorld* Kept = Save->FindWorld(USessionSubsystem::MapOf(World));
	TestTrue(TEXT("Tier 5: kept open"), Kept && Kept->OpenedChests.Contains(Five->GetSaveKey()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestShowModelsTest, "Looter.Loot.ChestShow.Models",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChestShowModelsTest::RunTest(const FString& Parameters)
{
	// Without its own models a tier wears its stand-in's and its show pieces are hidden, empty (the show plays on: Order).
	// With them: each piece its model on its socket, shown on its tiers only, and the glow slots its show drives there.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	for (int32 Tier = 1; Tier <= FChestKindInfo::NumTiers; ++Tier)
	{
		const FString Name = FString::Printf(TEXT("Tier %d"), Tier);
		const EChestKind Kind = FChestKindInfo::TierKind(Tier);
		const FChestKindInfo Info = FChestKindInfo::Get(Kind);
		TestTrue(FString::Printf(TEXT("%s: its pieces' models named"), *Name),
			(Info.LatchLeftPath != nullptr) == (Tier <= 2) && (Info.LatchRightPath != nullptr) == (Tier <= 2)
			&& (Info.LockPath != nullptr) == (Tier == 3) && (Info.CrestPath != nullptr) == (Tier == 5));
		AChest* Chest = SpawnChest(World, Tier, FVector(300.0, 400.0 * Tier, 0.0));
		if (!TestNotNull(*Name, Chest))
		{
			continue;
		}
		UStaticMeshComponent* Pieces[] = { Chest->LatchLeft, Chest->LatchRight, Chest->Lock, Chest->Crest };
		if (FChestKindInfo::WearsStandIn(Kind))
		{
			for (const UStaticMeshComponent* Piece : Pieces)
			{
				TestTrue(FString::Printf(TEXT("%s (stand-in): %s empty and hidden"), *Name, *Piece->GetName()), !Piece->GetStaticMesh() && !Piece->IsVisible());
			}
			continue;
		}
		AddInfo(FString::Printf(TEXT("%s: its own models are imported; checking their sockets and glow slots."), *Name));
		const TCHAR* Paths[] = { Info.LatchLeftPath, Info.LatchRightPath, Info.LockPath, Info.CrestPath };
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Pieces)); ++Index)
		{
			const UStaticMeshComponent* Piece = Pieces[Index];
			const FString Has = Piece->GetStaticMesh() ? Piece->GetStaticMesh()->GetPathName() : FString();
			TestEqual(*FString::Printf(TEXT("%s: %s's model"), *Name, *Piece->GetName()), Has, FString(Paths[Index] ? Paths[Index] : TEXT("")));
			TestEqual(*FString::Printf(TEXT("%s: %s shown"), *Name, *Piece->GetName()), Piece->IsVisible(), Paths[Index] != nullptr);
		}
		for (const TCHAR* Socket : { TEXT("Lid"), TEXT("Loot"), TEXT("Interact") })
		{
			TestTrue(FString::Printf(TEXT("%s: body socket %s"), *Name, Socket), Chest->Body->DoesSocketExist(Socket));
		}
		if (Tier <= 2)
		{
			for (const TCHAR* Socket : { TEXT("Latch_L"), TEXT("Latch_R"), TEXT("Light") })
			{
				TestTrue(FString::Printf(TEXT("%s: body socket %s"), *Name, Socket), Chest->Body->DoesSocketExist(Socket));
			}
		}
		TestTrue(FString::Printf(TEXT("%s: its own sockets (Lock, Orb, Crest, Gem)"), *Name),
			(Tier != 3 || Chest->Body->DoesSocketExist(TEXT("Lock")))
			&& (Tier != 4 || Chest->Lid->DoesSocketExist(TEXT("Orb")) || Chest->Body->DoesSocketExist(TEXT("Orb")))
			&& (Tier != 5 || (Chest->Lid->DoesSocketExist(TEXT("Crest")) && Chest->Crest->DoesSocketExist(TEXT("Gem")))));
		for (int32 Glow = 0; Glow < static_cast<int32>(EChestGlow::Num); ++Glow)
		{
			const FName Slot = ChestShow::GlowSlot(Tier, static_cast<EChestGlow>(Glow));
			if (Slot.IsNone())
			{
				continue;
			}
			bool bFound = false;
			for (const UStaticMeshComponent* Part : { Chest->Body.Get(), Chest->Lid.Get(), Chest->LatchLeft.Get(), Chest->LatchRight.Get(), Chest->Lock.Get(), Chest->Crest.Get() })
			{
				bFound |= Part->GetStaticMesh() && Part->GetMaterialIndex(Slot) != INDEX_NONE;
			}
			TestTrue(FString::Printf(TEXT("%s: glow slot %s"), *Name, *Slot.ToString()), bFound);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestShowFountainClearTest, "Looter.Loot.ChestShow.FountainClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChestShowFountainClearTest::RunTest(const FString& Parameters)
{
	// Tier 5's pieces fly to the chest's sides in turn and never toward the opener, wherever they stand (playtest 2026-10-28:
	// an Epic landed at the feet of a player stood at the Widows' strongbox's front corner).
	TestEqual(TEXT("Nobody there: the first piece to the right"), ChestShow::FountainTurn(0, 0.f, {}), 60.f);
	TestEqual(TEXT("...the second to the left"), ChestShow::FountainTurn(1, 0.f, {}), -60.f);
	TestEqual(TEXT("...the third a little further round"), ChestShow::FountainTurn(2, 0.f, {}), 78.f);
	TestEqual(TEXT("Stood square in front: nothing changes"), ChestShow::FountainTurn(1, 0.f, TOptional<float>(0.f)), -60.f);
	for (float Bearing = -180.f; Bearing <= 180.f; Bearing += 15.f)
	{
		for (int32 Index = 0; Index < 8; ++Index)
		{
			for (const float Jitter : { -6.f, 0.f, 6.f })
			{
				const float Turn = ChestShow::FountainTurn(Index, Jitter, TOptional<float>(Bearing));
				const float Clear = FMath::Abs(FMath::FindDeltaAngleDegrees(Turn, Bearing));
				TestTrue(*FString::Printf(TEXT("Opener at %.0f deg, piece %d (jitter %.0f): flies %.0f deg, %.0f clear of them"), Bearing, Index,
					Jitter, Turn, Clear), Clear >= ChestShow::FountainClearOfOpener);
			}
		}
	}
	return true;
}

#endif
