// AGravemotherCreature: construction (her size, rank, crits, her body or the pale hide, her boss fight), her life in the
// level (the stone, her mounds and her clod coming and going with her) and her brood's calls as she's hurt. The moves are
// in their topic files (GravemotherCreature.h lists them).

#include "Creatures/GravemotherCreature.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherClod.h"
#include "Creatures/GravemotherFurrow.h"
#include "Creatures/GravemotherMound.h"
#include "Creatures/GravemotherStone.h"
#include "Creatures/GroundCrack.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Player/PlayerSize.h"
#include "UObject/ConstructorHelpers.h"

const FName AGravemotherCreature::AbdomenBone(TEXT("abdomen"));

namespace
{
	/** Her hide on the spider's own model, until hers is imported: the spider's texture set made pale. */
	const TCHAR* PaleHidePath = TEXT("/Game/Art/Materials/MI_SpiderBody_Pale.MI_SpiderBody_Pale");

	/** An asset made after this code, found only once it exists (constructor-time). */
	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

AGravemotherCreature::AGravemotherCreature()
{
	// Her name stands for her rank's word on her tag: "The Gravemother" in orange.
	DisplayName = FText::FromString(TEXT("The Gravemother"));
	bNameIsRankWord = true;
	StartingRank = ECreatureRank::Legendary;
	// The spider's numbers before they were softened (2026-10-22): her fight was tuned on them.
	Health->MaxHealth = 300.f;
	AttackDamage = 12.f;
	// 1.8 times a spider as seen, her rank's own growth (x1.4) included.
	BodyScale = Gravemother::Size / Gravemother::RankSize;
	// Her head, and her abdomen (IsCriticalSpot narrows it to the soft rear while the stone stands).
	CriticalSpotBones = { TEXT("head"), AbdomenBone };

	// Heavy and unhurried until she means it (the plough is her speed). Her chase was tuned against the full-size player and
	// scales with the player's speed and pace, as a spider's does.
	WalkSpeed = 140.f;
	ChaseSpeed = 480.f * LooterPlayerSize::MoveScale;
	WanderRadius = 400.f;
	// Her body lies a while longer: she's the fight's end.
	CorpseTime = 10.f;

	// Her own body (Spider.py's mesh and rig in her mud-grey hide, the stone's roots and the grave mud on her legs) once it's
	// imported; until then the spider's, pale. Set on the defaults, so the Ledger's stand shows her as she looks out there.
	static USkeletalMesh* const OwnBody = FindIfMade<USkeletalMesh>(GravemotherAssets::Body);
	static UMaterialInterface* const PaleHide = OwnBody ? nullptr : FindIfMade<UMaterialInterface>(PaleHidePath);
	if (OwnBody)
	{
		GetMesh()->SetSkeletalMeshAsset(OwnBody);
	}
	else if (PaleHide)
	{
		GetMesh()->SetMaterial(0, PaleHide);
	}

	// Her boss fight. She lives in her den as any spider does until she turns on a player; then her bar sweeps in as she
	// rears and screams. Her lair's hunting ground keeps her fight in the den and on the Sink's floor; a player gone off it a
	// while, and her fight stands down. Her brood are her adds and die with her; her loot bursts out of her.
	Boss = CreateDefaultSubobject<UBossComponent>(TEXT("Boss"));
	Boss->BossName = DisplayName;
	Boss->Phases = GravemotherFight::MakePhases();
	Boss->bWaitsPassive = false;
	// A hit turns her on the shooter (a creature's own rule), which starts the fight; a shot from off her ground (the Sink's
	// rim) doesn't, so a sniper up there doesn't flash her bar on and off.
	Boss->bStartWhenHunting = true;
	Boss->bStartWhenHurt = false;
	Boss->EngageRadius = 0.f;
	Boss->LeashRadius = 0.f;
	Boss->SealRadius = 0.f;
	Boss->StandDownSeconds = GravemotherFight::StandDownSeconds;
	Boss->bAddsDieWithBoss = true;
	Boss->Show = GravemotherFight::MakeShow();
	Boss->Stagger = GravemotherFight::MakeStagger();
	Boss->LootShower = GravemotherFight::MakeLootShower();
}

void AGravemotherCreature::BeginPlay()
{
	Super::BeginPlay();
	// Her bite's timing as her class (or the level) gave it: her moves borrow the attack's and give it back.
	BiteWindup = AttackWindup;
	BiteRecovery = AttackRecovery;
	if (!bOwnPaceCaptured)
	{
		bOwnPaceCaptured = true;
		OwnFurrowCooldown = Furrows.Cooldown;
		OwnChaseSpeed = ChaseSpeed;
	}
	ApplyPace(0);
	// She sizes up whoever she first sees before she ploughs at them.
	FurrowCooldownLeft = Furrows.FirstDelay;

	Boss->OnFightStarted.AddUObject(this, &AGravemotherCreature::HandleFightStarted);
	Boss->OnFightReset.AddUObject(this, &AGravemotherCreature::HandleFightReset);
	Boss->OnPhaseChanged.AddUObject(this, &AGravemotherCreature::HandlePhaseChanged);
	Boss->OnStaggered.AddUObject(this, &AGravemotherCreature::HandleStaggered);
	Boss->CanStagger.BindWeakLambda(this, [this]() { return !IsDead(); });

	// Her headstone on her back, its health hers to scale (after her rank and level are applied).
	ReadAbdomenZones();
	ResetStone();
}

void AGravemotherCreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Taken away (her lair's player went far off) or removed: what's hers goes with her. Her furrow's berms and a clod's
	// rubble stand their time on their own.
	if (AGroundCrack* Open = QuakeCrack.Get())
	{
		Open->Destroy();
	}
	QuakeCrack = nullptr;
	if (Boss)
	{
		Boss->CanStagger.Unbind();
	}
	if (AGravemotherClod* Clod = HeldClod.Get())
	{
		Clod->Destroy();
	}
	HeldClod = nullptr;
	if (AGravemotherFurrow* Ground = Furrow.Get(); Ground && FurrowState == EGravemotherFurrow::Heave)
	{
		Ground->Cancel();
	}
	Furrow = nullptr;
	CollapseMounds();
	if (EndPlayReason == EEndPlayReason::Destroyed && !IsDead())
	{
		for (const TWeakObjectPtr<ASpiderCreature>& Each : Brood)
		{
			ASpiderCreature* Spiderling = Each.Get();
			if (Spiderling && !Spiderling->IsDead())
			{
				Spiderling->Destroy();
			}
		}
	}
	Brood.Reset();
	if (AGravemotherStone* Slab = Stone.Get())
	{
		Slab->Destroy();
	}
	Stone = nullptr;
	for (UAudioComponent* Sound : { HeaveSound.Get(), PloughSound.Get(), DigSound.Get() })
	{
		LooterSound::Stop(Sound);
	}
	Super::EndPlay(EndPlayReason);
}

void AGravemotherCreature::Tick(float DeltaSeconds)
{
	// The brain first (its attack runs her moves' wind-ups and strikes) and the spider's body (her poses over it); then her
	// moves' own frames: the heave and the plough, the clod in her grip, her clocks, the reel's sink and her stars.
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	TickFurrow(DeltaSeconds);
	TickClod(DeltaSeconds);
	TickUnearth(DeltaSeconds);
	TickMoves(DeltaSeconds);
}

void AGravemotherCreature::SetActorHiddenInGame(bool bNewHidden)
{
	// Her stone rides her body: hidden with it (her corpse gone) and shown again (her coming back).
	Super::SetActorHiddenInGame(bNewHidden);
	if (AGravemotherStone* Slab = Stone.Get())
	{
		Slab->SetActorHiddenInGame(bNewHidden);
	}
}

void AGravemotherCreature::SetHitVolumesEnabled(bool bEnabled)
{
	Super::SetHitVolumesEnabled(bEnabled);
	if (AGravemotherStone* Slab = Stone.Get())
	{
		Slab->SetShootable(bEnabled);
	}
}

// ---------------------------------------------------------------------------
// Hurt, dying, coming back
// ---------------------------------------------------------------------------

void AGravemotherCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	Super::OnHurt(bCritical, HitLocation);
	// A killing blow brings nothing.
	if (IsDead() || !Health || Health->IsDead() || Health->GetHealth() <= 0.f)
	{
		return;
	}
	// Her brood comes at two thirds and at one third of her health, each once a life (a hit past both lines: both): her next
	// attack is the roar whose slam raises the mounds.
	const float Share = Health->GetHealthPercent();
	while (BroodCallsMade < Gravemother::NumBroodCalls && Share <= Gravemother::BroodCallShare(BroodCallsMade))
	{
		++BroodCallsMade;
		++PendingUnearths;
		PendingFor = 0.f;
		bRoarWanted = true;
	}
	// Her last third: the stone cracks off by itself if it still stands.
	AGravemotherStone* Slab = Stone.Get();
	if (Slab && Slab->IsOn() && Share <= Gravemother::BroodCallShare(1))
	{
		Slab->Break(true);
	}
}

void AGravemotherCreature::OnDied()
{
	Super::OnDied();
	if (FurrowState != EGravemotherFurrow::None)
	{
		FinishFurrow();
	}
	DropClod();
	if (AGroundCrack* Open = QuakeCrack.Get())
	{
		Open->Close(0.f);
	}
	QuakeCrack = nullptr;
	bReeling = false;
	bStunned = false;
	OwnReelLeft = 0.f;
	// Her brood dies with her: the mounds still bulging cave in, and no call is left to raise.
	PendingUnearths = 0;
	CollapseMounds();
	if (AGravemotherStone* Slab = Stone.Get())
	{
		Slab->SetShootable(false);
	}
	// She crashes down: the den's dirt thrown up all round her (her boss's slow beat, shake and cry come with it).
	KickDirtRing(GetFeet(), 120.f * GetSizeScale(), 7, 0.9f);
}

void AGravemotherCreature::OnRespawned()
{
	Super::OnRespawned();
	// Back at her spot (a fight starting over, a rematch): a new life's calls, no move under way, her stone whole again.
	if (FurrowState != EGravemotherFurrow::None)
	{
		FinishFurrow();
	}
	if (AGravemotherClod* Clod = HeldClod.Get())
	{
		Clod->Destroy();
	}
	HeldClod = nullptr;
	BroodCallsMade = 0;
	PendingUnearths = 0;
	CollapseMounds();
	FurrowCooldownLeft = Furrows.FirstDelay;
	if (AGroundCrack* Open = QuakeCrack.Get())
	{
		Open->Close(0.f);
	}
	QuakeCrack = nullptr;
	Move = EGravemotherMove::Bite;
	bRoarWanted = false;
	bReeling = false;
	bStunned = false;
	OwnReelLeft = 0.f;
	QuakeCooldownLeft = 0.f;
	ClodCooldownLeft = 0.f;
	ApplyPace(0);
	ResetStone();
}
