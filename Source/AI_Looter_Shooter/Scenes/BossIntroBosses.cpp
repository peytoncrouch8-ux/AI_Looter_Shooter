// The boss intros' hooks and each boss's own moments: the Gravemother's dive, her line of bursts under the Sink's floor and
// her burst out of it; the Foreman's and Abel's entrances are their own (the scene only lets them go), Abel's first line
// said on camera.

#include "Scenes/BossIntro.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherStone.h"
#include "Creatures/GravemotherTuning.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	// --- The Gravemother's burrow (each a feel number: the dive and the burst are the scene's two big beats) ---

	/** Her dive: the dirt thrown up (0-1) and the view's jolt. */
	constexpr float DiveDirt = 1.f;
	constexpr float DiveShake = 0.3f;
	constexpr float DiveShakeSeconds = 0.6f;
	/** Each burst of the line toward the player: smaller dirt (a share of her size), a faint jolt, the mound's stir quieter. */
	constexpr float TrailSize = 0.6f;
	constexpr float TrailDirt = 0.55f;
	constexpr float TrailShake = 0.12f;
	constexpr float TrailShakeSeconds = 0.4f;
	constexpr float TrailVolume = 0.8f;
	/** Her burst out: more dirt than her size alone throws, and a hard jolt. */
	constexpr float BurstSize = 1.3f;
	constexpr float BurstDirt = 1.f;
	constexpr float BurstShake = 0.7f;
	constexpr float BurstShakeSeconds = 0.9f;
	/** The ground under a spot is looked for this far above it and below it (cm). */
	constexpr float GroundAbove = 500.f;
	constexpr float GroundBelow = 1000.f;

	/** The ground under Spot within reach of Level's height, on the world's static geometry. */
	bool FindGround(const UWorld& World, const FVector& Spot, float Level, FVector& OutGround)
	{
		FHitResult Hit;
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("BossIntroGround"));
		const FVector From(Spot.X, Spot.Y, Level + GroundAbove);
		const FVector To(Spot.X, Spot.Y, Level - GroundBelow);
		if (!World.LineTraceSingleByObjectType(Hit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			return false;
		}
		OutGround = Hit.ImpactPoint;
		return true;
	}

	/**
	 * Where she bursts out: BossIntro::BurstAlong's spot, on ground at her level with room over it for her standing (the
	 * den's roof); else where she stands (she goes under and comes up again in place).
	 */
	FVector FindBurst(const AGravemotherCreature& Her, const FVector& HerFeet, const APawn* Player)
	{
		const UWorld* World = Her.GetWorld();
		if (!World || !Player)
		{
			return HerFeet;
		}
		const float PlayerHalf = Player->GetRootComponent() ? static_cast<float>(Player->GetRootComponent()->Bounds.BoxExtent.Z) : 90.f;
		const FVector PlayerFeet = Player->GetActorLocation() - FVector(0.0, 0.0, PlayerHalf);
		const FVector Wanted = BossIntro::BurstAlong(HerFeet, PlayerFeet, Her.GetHome().GetLocation());
		FVector Ground;
		if (!FindGround(*World, Wanted, static_cast<float>(HerFeet.Z), Ground) || FMath::Abs(Ground.Z - HerFeet.Z) > BossIntro::BurstMaxRise)
		{
			return HerFeet;
		}
		const float Height = Her.GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f;
		FHitResult Roof;
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("BossIntroRoom"));
		if (World->LineTraceSingleByObjectType(Roof, Ground + FVector(0.0, 0.0, 20.0), Ground + FVector(0.0, 0.0, Height),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			return HerFeet;
		}
		return Ground;
	}

	void ThrowDirt(AGravemotherCreature& Her, const FVector& Where, float Size, float Strength)
	{
		if (UCreatureReactComponent* Reacts = Her.GetReact())
		{
			Reacts->ThrowGraveDirt(Where, Her.GetSizeScale() * Size, Strength);
		}
	}

	FBossIntroActs GravemotherActs(AGravemotherCreature& Her)
	{
		const float Half = Her.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Feet = Her.GetActorLocation() - FVector(0.0, 0.0, Half);
		const UBossComponent* Fight = Her.GetBoss();
		const APawn* Player = Fight ? Fight->GetFightPlayer() : nullptr;
		FBossIntroActs Acts;
		Acts.BurstFeet = FindBurst(Her, Feet, Player);
		Acts.bHasBurst = true;
		const TWeakObjectPtr<AGravemotherCreature> Weak = &Her;
		const FVector Burst = Acts.BurstFeet;
		const TWeakObjectPtr<const APawn> WeakPlayer = Player;

		// Into the Sink's floor: the earth thrown up, a jolt, and she's gone under it.
		Acts.Dive = [Weak, Feet](bool bSkipping)
		{
			AGravemotherCreature* She = Weak.Get();
			if (!She)
			{
				return;
			}
			if (!bSkipping)
			{
				ThrowDirt(*She, Feet, 1.f, DiveDirt);
				LooterSound::PlayAt(She, FName(BossIntroCue::Burrow), Feet);
				BossCameraShake::Kick(She, Feet, DiveShake, DiveShakeSeconds);
			}
			She->SetActorHiddenInGame(true);
		};

		// The ground bursting in a line toward the player as she comes under it.
		Acts.Trail = [Weak](int32 Puff, const FVector& Where, bool bSkipping)
		{
			AGravemotherCreature* She = Weak.Get();
			const UWorld* World = She ? She->GetWorld() : nullptr;
			if (!She || !World || bSkipping)
			{
				return;
			}
			FVector Ground = Where;
			FindGround(*World, Where, static_cast<float>(Where.Z), Ground);
			ThrowDirt(*She, Ground, TrailSize, TrailDirt);
			LooterSound::PlayAt(She, FName(LooterSoundCue::GravemotherMoundBulge), Ground, TrailVolume);
			BossCameraShake::Kick(She, Ground, TrailShake, TrailShakeSeconds);
		};

		// Out of it at the burst, facing the player: her own climb out of the dirt (her brain held through it), then her
		// scream (her fight's first move). Played or skipped: the fight needs her there and up.
		Acts.Live = [Weak, Burst, WeakPlayer](bool bSkipping)
		{
			AGravemotherCreature* She = Weak.Get();
			if (!She)
			{
				return;
			}
			const float HalfHeight = She->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			float Yaw = static_cast<float>(She->GetActorRotation().Yaw);
			if (const APawn* Facing = WeakPlayer.Get())
			{
				Yaw = static_cast<float>((Facing->GetActorLocation() - Burst).Rotation().Yaw);
			}
			She->SetActorLocationAndRotation(Burst + FVector(0.0, 0.0, HalfHeight), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			She->SetActorHiddenInGame(false);
			She->DevPlayEntrance();
			ThrowDirt(*She, Burst, BurstSize, BurstDirt);
			LooterSound::PlayAt(She, FName(BossIntroCue::Emerge), Burst);
			if (!bSkipping)
			{
				BossCameraShake::Kick(She, Burst, BurstShake, BurstShakeSeconds);
			}
			UE_LOG(LogLooter, Log, TEXT("Boss intro: the Gravemother bursts out at %s."), *Burst.ToCompactString());
		};

		// Her headstone lights up as the target again as the fight comes back to the player (its glow at the fight's start
		// ran out under the scene).
		Acts.Ended = [Weak](bool)
		{
			AGravemotherCreature* She = Weak.Get();
			if (She && She->IsHidden())
			{
				She->SetActorHiddenInGame(false);
			}
			AGravemotherStone* Slab = She ? She->GetStone() : nullptr;
			if (Slab && She->IsStoneOn() && GravemotherTuning::StoneGuards())
			{
				Slab->Mark(GravemotherTuning::MarkAtFightSeconds());
			}
		};
		return Acts;
	}

	FBossIntroActs ForemanActs()
	{
		// His entrance is his own (turned from the fall, the pick handle up and slammed, his line): the scene only lets him go.
		return FBossIntroActs();
	}

	FBossIntroActs AbelActs()
	{
		// His fight's first line, said as he raises his lantern on camera (the one his first phase queued was cleared).
		FBossIntroActs Acts;
		Acts.OwnLine = AbelRules::PhaseLine(0);
		return Acts;
	}

	/** Plays Kind's intro on Body now when due (or whatever, with bEvenIfSeen) in a game world, and records it seen. */
	bool Begin(EBossIntro Kind, ACreatureBase& Body, UBossComponent* Fight, TFunctionRef<FBossIntroActs()> MakeActs, bool bEvenIfSeen)
	{
		UWorld* World = Body.GetWorld();
		// Game worlds only: a test level's boss fights go on as they always did (BossIntro::Play is the tests' own way in).
		if (!World || !World->IsGameWorld() || !Fight || Body.IsDead())
		{
			return false;
		}
		FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(World);
		if (!bEvenIfSeen && !BossIntro::IsDue(Campaign, Kind, Fight->IsRematch()))
		{
			return false;
		}
		const USceneSubsystem* Scenes = USceneSubsystem::Get(World);
		if (!Scenes || Scenes->IsPlaying() || !USceneSubsystem::AreScenesOn(World))
		{
			return false;
		}
		if (!BossIntro::Play(Kind, Body, Fight, MakeActs()))
		{
			return false;
		}
		// Kept with the session's record: it goes into the next save.
		if (Campaign)
		{
			Campaign->SetFlag(BossIntro::SeenFlag(Kind));
		}
		return true;
	}
}

bool BossIntro::PlayIfDue(AGravemotherCreature& Boss)
{
	return Begin(EBossIntro::Gravemother, Boss, Boss.GetBoss(), [&Boss]() { return GravemotherActs(Boss); }, false);
}

bool BossIntro::PlayIfDue(AForemanCreature& Boss)
{
	return Begin(EBossIntro::Foreman, Boss, Boss.GetBoss(), []() { return ForemanActs(); }, false);
}

bool BossIntro::PlayIfDue(AAbelKeeper& Boss)
{
	return Begin(EBossIntro::Abel, Boss, Boss.GetBoss(), []() { return AbelActs(); }, false);
}

bool BossIntro::PlayNow(AGravemotherCreature& Boss)
{
	return Begin(EBossIntro::Gravemother, Boss, Boss.GetBoss(), [&Boss]() { return GravemotherActs(Boss); }, true);
}

bool BossIntro::PlayNow(AForemanCreature& Boss)
{
	return Begin(EBossIntro::Foreman, Boss, Boss.GetBoss(), []() { return ForemanActs(); }, true);
}

bool BossIntro::PlayNow(AAbelKeeper& Boss)
{
	return Begin(EBossIntro::Abel, Boss, Boss.GetBoss(), []() { return AbelActs(); }, true);
}
