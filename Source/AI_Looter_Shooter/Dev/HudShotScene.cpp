// The scene of Looter.HudShots: how each state the gameplay HUD is photographed in is made (not in shipping builds).
// HudShotDevCommands.cpp runs the steps and takes the pictures; HudShotScene.h says what a step is.

#include "Dev/HudShotScene.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossSeal.h"
#include "Bosses/BossTestSpider.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Progression/XPCurve.h"
#include "Tutorial/TutorialDirector.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

namespace
{
	using HudShotScene::FStep;

	/** After a request the screenshot is written at the end of the frame; this long before the next step changes the picture. */
	constexpr float DefaultGapSeconds = 0.6f;
	/** After the last picture, before the game quits: the file is written from another thread. */
	constexpr float LastGapSeconds = 1.5f;
	/** The low-health beat counts as at its peak above this (0 to 1). */
	constexpr float LowBeatPeak = 0.95f;

	/** Fixed seeds, so the same two guns (and the same names) come up every run and the pictures compare. */
	constexpr int32 RifleSeed = 4711;
	constexpr int32 ShotgunSeed = 1138;
	constexpr EWeaponRarity RifleRarity = EWeaponRarity::Rare;
	constexpr EWeaponRarity ShotgunRarity = EWeaponRarity::Epic;
	/** How much of each magazine is left, and the rounds carried for it. */
	constexpr float RifleMagazineShare = 0.6f;
	constexpr float ShotgunMagazineShare = 0.5f;
	constexpr int32 RifleReserve = 90;
	constexpr int32 ShotgunReserve = 24;
	/** Shares of the maximum health: the damage of the hit, where low health leaves it, where the heal brings it. */
	constexpr float HitShare = 0.3f;
	constexpr float LowShare = 0.25f;
	constexpr float HealedShare = 0.75f;
	/** The experience gained without a level-up, as a share of the level; and how far past the level the level-up goes, as a share of the next. */
	constexpr float GainShare = 0.35f;
	constexpr float LevelUpPastShare = 0.3f;

	/** Everything the steps act on, found afresh each time (a respawn or a gun swap changes it). */
	struct FHudShotPlayer
	{
		APlayerController* Controller = nullptr;
		APawn* Pawn = nullptr;
		UWeaponManagerComponent* Weapons = nullptr;
		UHealthComponent* Health = nullptr;
		UPlayerProgressionSubsystem* Progression = nullptr;
		ALooterHUD* Hud = nullptr;

		bool IsReady() const { return Weapons && Health && Progression && Hud; }
	};

	FHudShotPlayer HudShotFindPlayer(UWorld& World)
	{
		FHudShotPlayer Player;
		Player.Controller = World.GetFirstPlayerController();
		Player.Pawn = Player.Controller ? Player.Controller->GetPawn() : nullptr;
		if (Player.Pawn)
		{
			Player.Weapons = Player.Pawn->FindComponentByClass<UWeaponManagerComponent>();
			Player.Health = Player.Pawn->FindComponentByClass<UHealthComponent>();
		}
		const ULocalPlayer* LocalPlayer = Player.Controller ? Player.Controller->GetLocalPlayer() : nullptr;
		Player.Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
		Player.Hud = ALooterHUD::FindFor(Player.Controller);
		return Player;
	}

	ATutorialDirector* HudShotFindDirector(UWorld& World)
	{
		TActorIterator<ATutorialDirector> It(&World);
		return It ? *It : nullptr;
	}

	/** Runs an existing console command as typed in the game. */
	void HudShotConsole(UWorld& World, const FString& Command)
	{
		UE_LOG(LogLooter, Display, TEXT("Looter.HudShots: > %s"), *Command);
		if (APlayerController* Controller = World.GetFirstPlayerController())
		{
			Controller->ConsoleCommand(Command);
		}
		else if (GEngine)
		{
			GEngine->Exec(&World, *Command);
		}
	}

	// --- Guns ---

	/** The weapon definition called ExactName, or else the first whose name contains NameContains. */
	UWeaponDefinition* HudShotFindDefinition(const TCHAR* ExactName, const TCHAR* NameContains)
	{
		TArray<FAssetData> Assets;
		IAssetRegistry::GetChecked().GetAssetsByClass(UWeaponDefinition::StaticClass()->GetClassPathName(), Assets, true);
		const FAssetData* Found = Assets.FindByPredicate([ExactName](const FAssetData& Asset)
			{
				return Asset.AssetName.ToString().Equals(ExactName, ESearchCase::IgnoreCase);
			});
		if (!Found)
		{
			Found = Assets.FindByPredicate([NameContains](const FAssetData& Asset)
				{
					return Asset.AssetName.ToString().Contains(NameContains);
				});
		}
		return Found ? Cast<UWeaponDefinition>(Found->GetAsset()) : nullptr;
	}

	/** A gun at a fixed seed with MagazineShare of its magazine left (1 or more: full). */
	FWeaponInstanceData HudShotRollGun(UWeaponDefinition& Definition, EWeaponRarity Rarity, int32 Seed, float MagazineShare)
	{
		FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(&Definition, Rarity, 1);
		// No saved parts: the seed picks them, and the stats are worked out from the same seed.
		Gun.Seed = Seed;
		Gun.Parts.Reset();
		Gun.Stats = UWeaponRollLibrary::ComputeStatsWithParts(&Definition, Rarity, Gun.Level, Seed, Gun.Parts);
		Gun.SavedMagazine = MagazineShare >= 1.f ? -1 : FMath::RoundToInt32(Gun.Stats.MagazineSize * MagazineShare);
		return Gun;
	}

	/** Takes everything the player carries and gives back the rifle, in hand, with MagazineShare of its magazine and rounds in reserve. */
	void HudShotGiveRifle(const FHudShotPlayer& Player, float MagazineShare)
	{
		UWeaponDefinition* Rifle = HudShotFindDefinition(TEXT("DA_AssaultRifle"), TEXT("Rifle"));
		if (!Rifle)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: no rifle definition (DA_AssaultRifle) found."));
			return;
		}
		Player.Weapons->ClearInventory();
		Player.Weapons->GiveWeapon(HudShotRollGun(*Rifle, RifleRarity, RifleSeed, MagazineShare));
		Player.Weapons->AddAmmo(Rifle->AmmoType, RifleReserve);
	}

	void HudShotHurt(const FHudShotPlayer& Player, float Amount)
	{
		if (Amount > 0.f)
		{
			// No instigator and no causer: no one's hit marker or damage number, only the player's own HUD reacts.
			UGameplayStatics::ApplyDamage(Player.Pawn, Amount, nullptr, nullptr, UDamageType::StaticClass());
		}
	}

	// --- The steps' beginnings, moments and endings ---

	void HudShotBeginHit(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (Player.IsReady())
		{
			HudShotHurt(Player, Player.Health->GetMaxHealth() * HitShare);
		}
	}

	void HudShotBeginLow(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (Player.IsReady())
		{
			HudShotHurt(Player, Player.Health->GetHealth() - Player.Health->GetMaxHealth() * LowShare);
		}
	}

	/** The beat is brightest about now; it comes round every LowBeatSeconds. */
	bool HudShotLowBeatPeaks(UWorld& World)
	{
		return UHudPlayerFrameWidget::LowBeat(&World) >= LowBeatPeak;
	}

	void HudShotBeginHeal(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (Player.IsReady())
		{
			Player.Health->SetHealth(Player.Health->GetMaxHealth() * HealedShare);
		}
	}

	void HudShotBeginXP(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (!Player.IsReady())
		{
			return;
		}
		const int64 Needed = Player.Progression->GetXPToNextLevel();
		const int64 Room = Needed - Player.Progression->GetXP();
		// A good bite of the bar, but always short of the level.
		const int64 Gain = FMath::Clamp(static_cast<int64>(Needed * GainShare), static_cast<int64>(1), FMath::Max(Room - 1, static_cast<int64>(1)));
		Player.Progression->AddXP(Gain, EXPSource::Kill);
	}

	void HudShotBeginLevelUp(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (!Player.IsReady())
		{
			return;
		}
		// Past the level by a share of the next one, so the bar crosses into it soon after the gain and the banner shows.
		const int64 Room = Player.Progression->GetXPToNextLevel() - Player.Progression->GetXP();
		const int64 Past = static_cast<int64>(UPlayerProgressionSubsystem::GetCurve().XPToNextLevel(Player.Progression->GetLevel() + 1) * LevelUpPastShare);
		if (Player.Progression->AddXP(Room + FMath::Max(Past, static_cast<int64>(1)), EXPSource::Kill) == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: the player is at the top level, so there is no level-up to show."));
		}
	}

	void HudShotBeginReload(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (!Player.IsReady())
		{
			return;
		}
		Player.Weapons->Reload();
		const AWeaponBase* Gun = Player.Weapons->GetActiveWeapon();
		if (!Gun || !Gun->IsReloading())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: the gun in hand can't reload (magazine full or no rounds in reserve)."));
		}
	}

	bool HudShotReloadHalfway(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		const AWeaponBase* Gun = Player.Weapons ? Player.Weapons->GetActiveWeapon() : nullptr;
		return Gun && Gun->IsReloading() && Gun->GetReloadProgress() >= 0.5f;
	}

	void HudShotBeginEmpty(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (Player.IsReady())
		{
			HudShotGiveRifle(Player, 0.f);
		}
	}

	void HudShotBeginTwoGuns(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		UWeaponDefinition* Shotgun = Player.IsReady() ? HudShotFindDefinition(TEXT("DA_PumpShotgun"), TEXT("Shotgun")) : nullptr;
		if (!Shotgun)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: no shotgun definition (DA_PumpShotgun) found, so there is no second gun."));
			return;
		}
		AWeaponBase* Gun = Player.Weapons->GiveWeapon(HudShotRollGun(*Shotgun, ShotgunRarity, ShotgunSeed, ShotgunMagazineShare));
		Player.Weapons->AddAmmo(Shotgun->AmmoType, ShotgunReserve);
		// It joins in the next free slot; that slot takes it in hand.
		const int32 Slot = Gun ? Player.Weapons->GetWeapons().Find(Gun) : INDEX_NONE;
		if (Slot != INDEX_NONE)
		{
			Player.Weapons->EquipSlot(Slot);
		}
	}

	/** The boss Looter.Boss.Test spawned (it carries the test boss's tag), or none. */
	ACreatureBase* HudShotFindTestBoss(UWorld& World)
	{
		for (TActorIterator<ACreatureBase> It(&World); It; ++It)
		{
			if (It->Tags.Contains(BossTestSpider::Tag()))
			{
				return *It;
			}
		}
		return nullptr;
	}

	void HudShotBeginBoss(UWorld& World)
	{
		// The test boss may reach the player in the seconds this takes: the health holds.
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (Player.IsReady())
		{
			Player.Health->bInvulnerable = true;
		}
		HudShotConsole(World, TEXT("Looter.Boss.Test"));

		// The test boss's fog wall is a 15 m ring round it with the player inside, so its curtain of grave-fog stands all round
		// the view. No real fight looks like that (Abel's wall is the gate behind the player, the Gravemother has none), and
		// these pictures are of the HUD, so the picture shows the bar without it: the ring drops in the frame it rose, before
		// its curtain draws, and with no radius none closes again.
		const ACreatureBase* TestBoss = HudShotFindTestBoss(World);
		if (UBossComponent* Boss = TestBoss ? TestBoss->FindComponentByClass<UBossComponent>() : nullptr)
		{
			Boss->SealRadius = 0.f;
			if (ABossSeal* Wall = Boss->GetActiveSeal())
			{
				Wall->Drop();
			}
		}
	}

	bool HudShotBossBarShown(UWorld& World)
	{
		for (TObjectIterator<UHudBossBarWidget> It; It; ++It)
		{
			if (It->GetWorld() == &World && It->IsShown())
			{
				return true;
			}
		}
		return false;
	}

	void HudShotEndBoss(UWorld& World)
	{
		// The test boss goes, its fight, wall and bar with it (as a second Looter.Boss.Test replaces the first), so it doesn't
		// spoil the pictures after it. Looter.Boss.Reset didn't do: the player stands 7 m from it, inside its 12 m engage radius,
		// so the fight started again the next frame, its bar and wall back up and its bite on the player once the health let go.
		if (ACreatureBase* TestBoss = HudShotFindTestBoss(World))
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.HudShots: the test boss removed."));
			TestBoss->Destroy();
		}
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (Player.IsReady())
		{
			Player.Health->bInvulnerable = false;
		}
	}

	void HudShotBeginObjectiveDone(UWorld& World)
	{
		// The tutorial's current step (the board's, its last) is finished as a console command finishes it: the tracker ticks it
		// and holds it, then the first goal ends and the board's postings go up, the main one sliding in.
		const ATutorialDirector* Director = HudShotFindDirector(World);
		const FString Id = Director ? Director->MissionId.ToString() : FString(TEXT("Tutorial"));
		HudShotConsole(World, FString::Printf(TEXT("Looter.Mission.Complete %s"), *Id));
	}

	// The gaps after the heal, experience, level-up and reload steps let their floating numbers, flashes, the banner and the
	// reload run out, so they don't show in the pictures after (two gains close together even add up into one "+N XP").
	const FStep HudShotSteps[] =
	{
		// Name            Seconds  Begin                         Ready                       After                  Gap
		{ TEXT("calm"),        0.5f, nullptr,                      nullptr,                    nullptr,               DefaultGapSeconds },
		{ TEXT("hit"),         0.1f, &HudShotBeginHit,             nullptr,                    nullptr,               DefaultGapSeconds },
		{ TEXT("low"),         2.2f, &HudShotBeginLow,             &HudShotLowBeatPeaks,       nullptr,               DefaultGapSeconds },
		{ TEXT("heal"),        0.25f, &HudShotBeginHeal,           nullptr,                    nullptr,               1.3f },
		{ TEXT("xp"),          0.3f, &HudShotBeginXP,              nullptr,                    nullptr,               2.4f },
		{ TEXT("levelup"),     1.0f, &HudShotBeginLevelUp,         nullptr,                    nullptr,               2.6f },
		{ TEXT("reload"),      0.6f, &HudShotBeginReload,          &HudShotReloadHalfway,      nullptr,               1.4f },
		{ TEXT("empty"),       1.5f, &HudShotBeginEmpty,           nullptr,                    nullptr,               DefaultGapSeconds },
		{ TEXT("two_guns"),    1.5f, &HudShotBeginTwoGuns,         nullptr,                    nullptr,               DefaultGapSeconds },
		{ TEXT("boss"),        2.0f, &HudShotBeginBoss,            &HudShotBossBarShown,       &HudShotEndBoss,       DefaultGapSeconds },
		// The tick, at 0.35 s: the done objective's tick and its step's section cyan. The tracker holds it for DoneHoldSeconds (1.4 s)
		// from the completion; the next step's picture, 1.6 s after the completion, catches what follows sliding in (the board's
		// main posting, tracked in its place, or the tracker fading out if none is).
		{ TEXT("done"),        0.35f, &HudShotBeginObjectiveDone,  nullptr,                    nullptr,               0.1f },
		{ TEXT("next"),        1.15f, nullptr,                     nullptr,                    nullptr,               LastGapSeconds },
	};
}

namespace HudShotScene
{
	TConstArrayView<FStep> Steps()
	{
		return MakeArrayView(HudShotSteps);
	}

	UWorld* FindGameWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld())
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}

	bool IsPlayerReady(UWorld& World)
	{
		return HudShotFindPlayer(World).IsReady();
	}

	void Prepare(UWorld& World)
	{
		const FHudShotPlayer Player = HudShotFindPlayer(World);
		if (!Player.IsReady())
		{
			return;
		}
		Player.Progression->SetLevel(1);
		Player.Health->bInvulnerable = false;
		Player.Health->ResetHealth();
		HudShotGiveRifle(Player, RifleMagazineShare);

		if (ATutorialDirector* Director = HudShotFindDirector(World))
		{
			// A tutorial done or skipped starts again; then it goes on from the notice board step (the rifle is in hand already,
			// so "Find a gun in town" is behind it), as a saved session would.
			if (Director->GetCurrentStep() == INDEX_NONE)
			{
				Director->Restart();
			}
			Director->ResumeAtStep(ATutorialDirector::BoardStep);
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.HudShots: this level has no tutorial director, so the mission tracker may show nothing."));
		}
	}
}

#endif
