#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/AmmoPickup.h"
#include "Loot/Chest.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootOdds.h"
#include "Loot/LootTable.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Shields/ShieldPickup.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "UObject/Package.h"
#include "Tests/AutomationCommon.h"

// The five loot chest tiers (the user, 2026-10-27: "the better the chest the better the loot"; EChestKind::ChestTier1-5,
// FChestKindInfo's tier numbers in ChestKinds.cpp): what each pays over many openings (counts, rarity floors, tier 5's
// legendary odds kept early, the steady climb), the stand-in models until SM_ChestTier<N> is imported, the plain hinged
// opening after its show, and the old kinds (Supply Crate, Strongbox) still read and kept by id as they were.

// The kinds are saved in levels and sessions by their number: the old ones never move, the tiers come after them.
static_assert(static_cast<uint8>(EChestKind::SupplyCrate) == 0 && static_cast<uint8>(EChestKind::Strongbox) == 1
	&& static_cast<uint8>(EChestKind::Coffin) == 2 && static_cast<uint8>(EChestKind::Grave) == 3
	&& static_cast<uint8>(EChestKind::Mailbox) == 4 && static_cast<uint8>(EChestKind::Footlocker) == 5,
	"EChestKind's old values are saved: only append new kinds.");
static_assert(static_cast<uint8>(EChestKind::ChestTier1) == 6 && static_cast<uint8>(EChestKind::ChestTier5) == 10,
	"The tiers follow the old kinds, in order.");

namespace
{
	constexpr int32 Openings = 20000;
	/** Ransom's Rest's first level (the early legendary cap on), and the Lily's first (full odds). */
	constexpr int32 EarlyLevel = 1;
	constexpr int32 LateLevel = 9;

	/** The design, tier by tier: guns from Min to Max at a chance, the floor, ammo pickups, legendary odds kept early. */
	struct FTierDesign
	{
		int32 Tier;
		int32 MinGuns;
		int32 MaxGuns;
		float GunChance;
		EWeaponRarity Floor;
		int32 Ammo;
		bool bKeepsOdds;
	};
	const FTierDesign Designs[] = {
		{ 1, 1, 1, 0.35f, EWeaponRarity::Common, 2, false },
		{ 2, 1, 1, 1.f, EWeaponRarity::Common, 2, false },
		{ 3, 1, 2, 1.f, EWeaponRarity::Uncommon, 2, false },
		{ 4, 2, 2, 1.f, EWeaponRarity::Rare, 2, false },
		{ 5, 3, 3, 1.f, EWeaponRarity::Epic, 3, true },
	};

	/** A tier's table as an opening builds it, its guns one stock gun (rarity weights 60/25/10/4/1), so the odds are the design's
	 *  whatever guns this checkout's default table lists. */
	ULootTable* StockTierTable(int32 Tier)
	{
		ULootTable* Table = AChest::MakeLootTableFor(FChestKindInfo::Get(FChestKindInfo::TierKind(Tier)));
		Table->Entries.Reset();
		FLootTableEntry Entry;
		Entry.Weapon = NewObject<UWeaponDefinition>();
		Table->Entries.Add(Entry);
		Table->bDropsShields = false;
		return Table;
	}

	/** Many openings of a table, counted. */
	struct FOpenings
	{
		int32 Opened = 0;
		int32 WithGuns = 0;
		int32 FewestGuns = MAX_int32;
		int32 MostGuns = 0;
		int32 Guns = 0;
		int32 UnderFloor = 0;
		int32 Legendaries = 0;
		int32 WithLegendary = 0;
		int32 AmmoWrong = 0;
	};

	/** Count openings of Table from Seed at Level, drawn as a chest draws them (ammo, then the guns: ULootLibrary::RollLoot). */
	FOpenings OpenMany(const ULootTable* Table, int32 Count, int32 Seed, int32 Level, int32 Ammo)
	{
		FOpenings Result;
		FRandomStream Random(Seed);
		for (int32 Each = 0; Each < Count; ++Each)
		{
			const TArray<FAmmoDrop> Drops = ULootLibrary::RollAmmo(Table, Random);
			const TArray<FLootWeaponPick> Picks = ULootLibrary::RollWeaponPicks(Table, 0.f, Random, Level);
			++Result.Opened;
			bool bAmmoWrong = Drops.Num() != Ammo;
			for (const FAmmoDrop& Drop : Drops)
			{
				bAmmoWrong |= Drop.Amount != LooterLoot::AmmoAmountFor(Drop.Type, LooterLoot::ChestAmmoAmount);
			}
			Result.AmmoWrong += bAmmoWrong ? 1 : 0;
			if (!Picks.IsEmpty())
			{
				++Result.WithGuns;
				Result.FewestGuns = FMath::Min(Result.FewestGuns, Picks.Num());
				Result.MostGuns = FMath::Max(Result.MostGuns, Picks.Num());
			}
			bool bLegendary = false;
			for (const FLootWeaponPick& Pick : Picks)
			{
				++Result.Guns;
				Result.UnderFloor += Pick.Rarity < Table->MinWeaponRarity ? 1 : 0;
				Result.Legendaries += Pick.Rarity == EWeaponRarity::Legendary ? 1 : 0;
				bLegendary |= Pick.Rarity == EWeaponRarity::Legendary;
			}
			Result.WithLegendary += bLegendary ? 1 : 0;
		}
		return Result;
	}

	/** Five standard errors of a share P over N tries (a little slack for the smallest). */
	double Spread(double P, int32 N)
	{
		return 5.0 * FMath::Sqrt(FMath::Max(P * (1.0 - P), 1e-6) / FMath::Max(N, 1)) + 1e-4;
	}

	/** A chest of a kind at Where, built and begun as a level's is. */
	AChest* SpawnChest(UWorld* World, EChestKind ChestKind, const FVector& Where, FName Id = NAME_None)
	{
		const FTransform At(FRotator(0.0, 180.0, 0.0), Where);
		AChest* Chest = World->SpawnActorDeferred<AChest>(AChest::StaticClass(), At);
		if (!Chest)
		{
			return nullptr;
		}
		Chest->Kind = ChestKind;
		Chest->ChestId = Id;
		Chest->FinishSpawning(At);
		Chest->DispatchBeginPlay();
		return Chest;
	}

	/** Moves its opening on a frame at a time (a test level never ticks). */
	void Run(AChest* Chest, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			Chest->Advance(Frame);
		}
	}

	/** The lid's pitch about its hinge, read from the lid itself (degrees). */
	double LidPitch(const AChest* Chest)
	{
		const FVector Front = Chest->Lid->GetRelativeRotation().Vector();
		return FMath::RadiansToDegrees(FMath::Atan2(Front.Z, Front.X));
	}

	FString MeshPath(const UStaticMeshComponent* Part)
	{
		return Part && Part->GetStaticMesh() ? Part->GetStaticMesh()->GetPathName() : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestTiersPayoutTest, "Looter.Loot.ChestTiers.Payout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChestTiersPayoutTest::RunTest(const FString& Parameters)
{
	// Each tier's numbers, and 20000 openings of each from a fixed seed, early (Ransom's Rest) and late (the Lily on): the
	// gun counts in range, never a gun under the floor, the ammo a chest's, tier 1's gun at its chance; tiers 1-4 under the
	// early legendary cap, tier 5 keeping its odds (a legendary in about 70% of openings); and the climb from tier to tier.
	double LastGuns = -1.0;
	double LastRarity = -1.0;
	for (const FTierDesign& Design : Designs)
	{
		const FString Name = FString::Printf(TEXT("Tier %d"), Design.Tier);
		const EChestKind Kind = FChestKindInfo::TierKind(Design.Tier);
		const FChestKindInfo Info = FChestKindInfo::Get(Kind);
		TestEqual(*FString::Printf(TEXT("%s: its kind's tier"), *Name), Info.Tier, Design.Tier);
		TestTrue(FString::Printf(TEXT("%s: %d-%d gun(s) at %.0f%%, %s or better, %d ammo pickups"), *Name, Design.MinGuns, Design.MaxGuns,
			Design.GunChance * 100.f, *UEnum::GetDisplayValueAsText(Design.Floor).ToString(), Design.Ammo),
			Info.Guns == Design.MinGuns && FMath::Max(Info.MaxGuns, Info.Guns) == Design.MaxGuns
			&& FMath::IsNearlyEqual(Info.GunChance, Design.GunChance) && Info.MinRarity == Design.Floor && Info.AmmoPickups == Design.Ammo);
		TestEqual(*FString::Printf(TEXT("%s: keeps its legendary odds early: %s"), *Name, Design.bKeepsOdds ? TEXT("yes") : TEXT("no")),
			Info.bKeepsLegendaryOdds, Design.bKeepsOdds);
		TestTrue(FString::Printf(TEXT("%s: a tap, its show, a hinged lid, on the map"), *Name), Info.HoldSeconds <= 0.f
			&& Info.LidMotion == EChestLidMotion::Hinge && Info.PreMotion == EChestPreMotion::Show && Info.bOnMap);

		const ULootTable* Table = StockTierTable(Design.Tier);
		TestTrue(FString::Printf(TEXT("%s: its table carries the floor, the gun counts and a chest's 1.5 boxes of ammo"), *Name),
			Table->MinWeaponRarity == Design.Floor && Table->MinWeaponDrops == Design.MinGuns && Table->MaxWeaponDrops == Design.MaxGuns
			&& Table->bKeepsLegendaryOdds == Design.bKeepsOdds && Table->AmmoAmountMin == LooterLoot::ChestAmmoAmount
			&& Table->AmmoAmountMax == LooterLoot::ChestAmmoAmount);

		for (const int32 Level : { EarlyLevel, LateLevel })
		{
			const FString At = FString::Printf(TEXT("%s at level %d"), *Name, Level);
			const FOpenings Rolled = OpenMany(Table, Openings, 20261027 + Design.Tier * 31 + Level, Level, Design.Ammo);
			TestEqual(*FString::Printf(TEXT("%s: never a gun under the floor"), *At), Rolled.UnderFloor, 0);
			TestEqual(*FString::Printf(TEXT("%s: its ammo pickups, each a chest's 1.5 boxes"), *At), Rolled.AmmoWrong, 0);
			TestTrue(FString::Printf(TEXT("%s: %d to %d guns an opening that gives any (%d-%d)"), *At, Rolled.FewestGuns, Rolled.MostGuns,
				Design.MinGuns, Design.MaxGuns), Rolled.WithGuns == 0 || (Rolled.FewestGuns >= Design.MinGuns && Rolled.MostGuns <= Design.MaxGuns));
			if (Design.MinGuns != Design.MaxGuns)
			{
				TestTrue(FString::Printf(TEXT("%s: both counts come"), *At), Rolled.FewestGuns == Design.MinGuns && Rolled.MostGuns == Design.MaxGuns);
			}
			const double GunShare = static_cast<double>(Rolled.WithGuns) / Rolled.Opened;
			TestTrue(FString::Printf(TEXT("%s: guns in %.1f%% of openings (%.0f%%)"), *At, GunShare * 100.0, Design.GunChance * 100.f),
				FMath::Abs(GunShare - Design.GunChance) <= Spread(Design.GunChance, Rolled.Opened));

			const LootOdds::FExpected Expected = LootOdds::Expected(Table, 0.f, Level);
			const double WithLegendary = static_cast<double>(Rolled.WithLegendary) / Rolled.Opened;
			TestTrue(FString::Printf(TEXT("%s: a legendary in %.2f%% of openings (table %.2f%%)"), *At, WithLegendary * 100.0,
				Expected.LegendaryPerKill * 100.0), FMath::Abs(WithLegendary - Expected.LegendaryPerKill) <= Spread(Expected.LegendaryPerKill, Rolled.Opened));
			if (Level == EarlyLevel)
			{
				if (Design.bKeepsOdds)
				{
					TestEqual(*FString::Printf(TEXT("%s: never capped"), *At), ULootLibrary::LegendaryCapFor(Table, Level), 1.f);
					TestTrue(FString::Printf(TEXT("%s: high legendary odds (%.1f%% of openings, over half)"), *At, Expected.LegendaryPerKill * 100.0),
						Expected.LegendaryPerKill > 0.5 && Expected.LegendaryPerKill < 0.9);
				}
				else
				{
					const float Cap = ULootLibrary::LegendaryCapFor(Table, Level);
					const double PerGun = Rolled.Guns > 0 ? static_cast<double>(Rolled.Legendaries) / Rolled.Guns : 0.0;
					TestTrue(FString::Printf(TEXT("%s: the early cap holds over the floor (%.3f%% a gun, cap %.3f%%)"), *At, PerGun * 100.0, Cap * 100.f),
						Cap < 1.f && PerGun <= Cap + Spread(Cap, Rolled.Guns));
					TestTrue(FString::Printf(TEXT("%s: a legendary in under 1%% of openings"), *At), Expected.LegendaryPerKill < 0.01);
				}
			}
			else
			{
				// The steady climb, with every tier's full odds: more guns, and better ones, each tier up.
				double Rarity = 0.0;
				for (int32 Tier = 0; Tier < LootOdds::NumRarities; ++Tier)
				{
					Rarity += Expected.RarityShares[Tier] * Tier;
				}
				TestTrue(FString::Printf(TEXT("%s: more guns than the tier under it (%.2f)"), *At, Expected.WeaponsPerKill), Expected.WeaponsPerKill > LastGuns);
				TestTrue(FString::Printf(TEXT("%s: better guns than the tier under it (mean rarity %.2f)"), *At, Rarity), Rarity > LastRarity);
				LastGuns = Expected.WeaponsPerKill;
				LastRarity = Rarity;
				AddInfo(FString::Printf(TEXT("%s: %.2f guns an opening (%s), a legendary in %.1f%%"), *At, Expected.WeaponsPerKill,
					*LootOdds::DescribeShares(Expected.RarityShares), Expected.LegendaryPerKill * 100.0));
			}
		}

		// The game's own guns (the default table), where this checkout lists them: the floor holds on them too.
		const ULootTable* Game = AChest::MakeLootTableFor(Info);
		if (!Game->Entries.IsEmpty())
		{
			const FOpenings Rolled = OpenMany(Game, Openings / 4, 1027 + Design.Tier, EarlyLevel, Design.Ammo);
			TestEqual(*FString::Printf(TEXT("%s with the game's guns: never one under the floor"), *Name), Rolled.UnderFloor, 0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestTiersStandInTest, "Looter.Loot.ChestTiers.StandIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChestTiersStandInTest::RunTest(const FString& Parameters)
{
	// Each tier built in a level: its own model when SM_ChestTier<N> is imported, else the stand-in's (tiers 1-2 the Supply
	// Crate's, 3-5 the Strongbox's, its wheel shown but never turned); then opened with a tap: no wheel or pry first, the lid
	// swung up on its hinge to its angle, and out of it its guns (in range, none under the floor) and its ammo pickups.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	const ULootTable* Default = ULootLibrary::GetDefaultLootTable();
	const bool bGuns = Default && !Default->Entries.IsEmpty();
	if (!bGuns)
	{
		AddWarning(TEXT("The default loot table (DA_LootTable_Default) lists no guns in this checkout: the tiers' guns aren't counted."));
	}
	for (int32 Tier = 1; Tier <= FChestKindInfo::NumTiers; ++Tier)
	{
		const FString Name = FString::Printf(TEXT("Tier %d"), Tier);
		const EChestKind Kind = FChestKindInfo::TierKind(Tier);
		const FChestKindInfo Info = FChestKindInfo::Get(Kind);
		const EChestKind Expected = Tier <= 2 ? EChestKind::SupplyCrate : EChestKind::Strongbox;
		TestTrue(FString::Printf(TEXT("%s: stands in as the %s"), *Name, *AChest::KindName(Expected).ToString()),
			Info.StandIn.IsSet() && Info.StandIn.GetValue() == Expected);
		TestTrue(FString::Printf(TEXT("%s: its own model is SM_ChestTier%d"), *Name, Tier),
			FString(Info.BodyPath).Contains(FString::Printf(TEXT("SM_ChestTier%d."), Tier))
			&& FString(Info.LidPath).Contains(FString::Printf(TEXT("SM_ChestTier%d_Lid."), Tier)));

		AChest* Chest = SpawnChest(World, Kind, FVector(300.0, 400.0 * Tier, 0.0));
		if (!TestNotNull(*FString::Printf(TEXT("A %s chest"), *Name), Chest))
		{
			continue;
		}
		const FChestKindInfo Worn = FChestKindInfo::GetWorn(Kind);
		if (FChestKindInfo::WearsStandIn(Kind))
		{
			const FChestKindInfo StandIn = FChestKindInfo::Get(Expected);
			TestTrue(FString::Printf(TEXT("%s: without its own model, it wears the stand-in's"), *Name),
				FCString::Strcmp(Worn.BodyPath, StandIn.BodyPath) == 0 && FCString::Strcmp(Worn.LidPath, StandIn.LidPath) == 0);
			if (Chest->Body->GetStaticMesh())
			{
				TestEqual(*FString::Printf(TEXT("%s: the stand-in's body"), *Name), MeshPath(Chest->Body), FString(StandIn.BodyPath));
				TestEqual(*FString::Printf(TEXT("%s: the stand-in's lid"), *Name), MeshPath(Chest->Lid), FString(StandIn.LidPath));
			}
			else
			{
				AddWarning(FString::Printf(TEXT("%s: the stand-in's model isn't imported either (Art/Models/Loot/Chests.py)."), *Name));
			}
			TestEqual(*FString::Printf(TEXT("%s: the Strongbox's wheel shows on tiers 3-5 only"), *Name), Chest->Wheel->IsVisible(), Tier >= 3);
		}
		else
		{
			TestEqual(*FString::Printf(TEXT("%s: its own body"), *Name), MeshPath(Chest->Body), FString(Info.BodyPath));
			TestEqual(*FString::Printf(TEXT("%s: its own lid"), *Name), MeshPath(Chest->Lid), FString(Info.LidPath));
			TestFalse(*FString::Printf(TEXT("%s: no wheel"), *Name), Chest->Wheel->IsVisible());
		}

		// Opened: the lid at once, about its hinge, to its angle.
		if (!TestTrue(FString::Printf(TEXT("%s: opened with a tap"), *Name), Chest->Open(nullptr)))
		{
			continue;
		}
		// Its show first (ChestShowTests.cpp has its order), then the lid.
		TestTrue(FString::Printf(TEXT("%s: its show before the lid"), *Name), Chest->GetState() == EChestState::Unlocking);
		Run(Chest, Chest->GetPreSeconds());
		while (Chest->GetState() == EChestState::Unlocking)
		{
			Chest->Advance(0.001f);
		}
		const float LidSeconds = Chest->GetLidSecondsFor();
		Run(Chest, LidSeconds * 0.5f);
		TestTrue(FString::Printf(TEXT("%s: halfway, the lid partway up (%.0f degrees)"), *Name, Chest->GetLidAngle()),
			Chest->GetLidAngle() > Info.OpenAngle * 0.3f && Chest->GetLidAngle() < Info.OpenAngle * 0.7f);
		Run(Chest, LidSeconds * 0.5f + 0.05f);
		TestTrue(FString::Printf(TEXT("%s: open at its angle (%.1f of %.0f)"), *Name, Chest->GetLidAngle(), Info.OpenAngle),
			Chest->GetState() == EChestState::Open && FMath::IsNearlyEqual(Chest->GetLidAngle(), Info.OpenAngle, 0.01f)
			&& FMath::IsNearlyEqual(LidPitch(Chest), static_cast<double>(Info.OpenAngle), 0.5));
		TestTrue(FString::Printf(TEXT("%s: its loot given"), *Name), Chest->HasGivenLoot());

		int32 Guns = 0;
		int32 Ammo = 0;
		for (const AActor* Loot : Chest->GetDroppedLoot())
		{
			if (const AWeaponBase* Gun = Cast<AWeaponBase>(Loot))
			{
				++Guns;
				TestTrue(FString::Printf(TEXT("%s: %s no rarer than its floor allows"), *Name, *Gun->GetName()), Gun->GetInstance().Rarity >= Info.MinRarity);
			}
			else if (const AShieldPickup* Ward = Cast<AShieldPickup>(Loot))
			{
				// A gun pick may come as a ward of the same rarity (the default table drops wards).
				++Guns;
				TestTrue(FString::Printf(TEXT("%s: a ward no rarer than its floor allows"), *Name), Ward->GetWard().Rarity >= Info.MinRarity);
			}
			else if (Cast<AAmmoPickup>(Loot))
			{
				++Ammo;
			}
		}
		TestEqual(*FString::Printf(TEXT("%s: its ammo pickups"), *Name), Ammo, Info.AmmoPickups);
		if (bGuns)
		{
			const int32 Least = Info.GunChance >= 1.f ? Info.Guns : 0;
			TestTrue(FString::Printf(TEXT("%s: %d gun(s) (%d-%d)"), *Name, Guns, Least, FMath::Max(Info.MaxGuns, Info.Guns)),
				Guns >= Least && Guns <= FMath::Max(Info.MaxGuns, Info.Guns));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChestTiersOldSavesTest, "Looter.Loot.ChestTiers.OldSaves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChestTiersOldSavesTest::RunTest(const FString& Parameters)
{
	// The old kinds keep their names, numbers and loot (a level or a save naming them reads as before), and a chest the session
	// kept open by its id stays open when the level's chest of that id has become a tier (GangStrongbox, a Strongbox, now tier 3).
	const UEnum* Kinds = StaticEnum<EChestKind>();
	if (TestNotNull(TEXT("EChestKind is reflected"), Kinds))
	{
		const TCHAR* Old[] = { TEXT("SupplyCrate"), TEXT("Strongbox"), TEXT("Coffin"), TEXT("Grave"), TEXT("Mailbox"), TEXT("Footlocker") };
		for (int32 Value = 0; Value < static_cast<int32>(UE_ARRAY_COUNT(Old)); ++Value)
		{
			TestEqual(*FString::Printf(TEXT("Kind %d is still %s"), Value, Old[Value]), Kinds->GetNameStringByValue(Value), FString(Old[Value]));
		}
		TestEqual(TEXT("The tiers come after them"), Kinds->GetNameStringByValue(static_cast<int64>(EChestKind::ChestTier1)), FString(TEXT("ChestTier1")));
	}
	TestEqual(TEXT("Every kind is listed"), FChestKindInfo::All().Num(), 11);
	for (int32 Tier = 1; Tier <= FChestKindInfo::NumTiers; ++Tier)
	{
		TestEqual(*FString::Printf(TEXT("Tier %d's kind"), Tier), FChestKindInfo::Get(FChestKindInfo::TierKind(Tier)).Tier, Tier);
	}

	// The old crate and strongbox: their numbers as before, no tier, no floor, the early cap.
	const FChestKindInfo Crate = FChestKindInfo::Get(EChestKind::SupplyCrate);
	const FChestKindInfo Box = FChestKindInfo::Get(EChestKind::Strongbox);
	TestTrue(TEXT("The Supply Crate: one gun at Luck 0.5, no tier"), Crate.Tier == 0 && Crate.Guns == 1 && FMath::IsNearlyEqual(Crate.Luck, 0.5f)
		&& Crate.MinRarity == EWeaponRarity::Common && !Crate.bKeepsLegendaryOdds && !Crate.StandIn.IsSet());
	TestTrue(TEXT("The Strongbox: two guns at Luck 1.0, its wheel, no tier"), Box.Tier == 0 && Box.Guns == 2 && FMath::IsNearlyEqual(Box.Luck, 1.f)
		&& Box.PreMotion == EChestPreMotion::Wheel && Box.MinRarity == EWeaponRarity::Common && !Box.bKeepsLegendaryOdds);
	const ULootTable* BoxTable = AChest::MakeLootTableFor(Box);
	TestTrue(TEXT("...its table as before: no floor, capped early"), BoxTable->MinWeaponRarity == EWeaponRarity::Common
		&& !BoxTable->bKeepsLegendaryOdds && BoxTable->MinWeaponDrops == 2 && BoxTable->MaxWeaponDrops == 2);

	// Kept by id across the change.
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper AgainLevel;
	if (!TestTrue(TEXT("Test levels made"), PlayedLevel.CreateTestWorld(EWorldType::EditorPreview) && AgainLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Played = PlayedLevel.GetTestWorld();
	UWorld* Again = AgainLevel.GetTestWorld();
	const FName Id(TEXT("GangStrongbox"));
	AChest* Old = SpawnChest(Played, EChestKind::Strongbox, FVector(300.0, 0.0, 0.0), Id);
	if (!TestNotNull(TEXT("The old Strongbox"), Old))
	{
		return false;
	}
	Old->Open(nullptr);
	Run(Old, Old->WheelSeconds + Old->LidSeconds + 0.1f);
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Played, *Save);
	TArray<uint8> Bytes;
	ULooterSessionSave* Back = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	const FSavedMapWorld* Read = Back ? Back->FindWorld(USessionSubsystem::MapOf(Played)) : nullptr;
	if (!TestTrue(TEXT("Kept open by its id, through the save file"), Read && Read->OpenedChests.Contains(Id)))
	{
		return false;
	}
	AChest* Tiered = SpawnChest(Again, FChestKindInfo::TierKind(3), FVector(300.0, 0.0, 0.0), Id);
	if (!TestNotNull(TEXT("The same chest, now tier 3"), Tiered))
	{
		return false;
	}
	const FSavedMapWorld PlayedWorld = *Read;
	Back->FindOrAddWorld(USessionSubsystem::MapOf(Again)) = PlayedWorld;
	USessionSubsystem::RestoreWorld(Again, *Back);
	TestTrue(TEXT("Open at once, its loot given, its lid at its tier's angle"), Tiered->GetState() == EChestState::Open && Tiered->HasGivenLoot()
		&& FMath::IsNearlyEqual(Tiered->GetLidAngle(), Tiered->GetKindInfo().OpenAngle, 0.01f));
	TestFalse(TEXT("...and it gives nothing more"), Tiered->Open(nullptr, /*bForce*/ true));
	return true;
}

#endif
