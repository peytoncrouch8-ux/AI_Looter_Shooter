// AGravemotherCreature: construction (her size, rank, crits and pale hide), her life in the level, and her brood's calls as
// she's hurt. GravemotherCreatureCharge.cpp has the charge, GravemotherCreatureBrood.cpp her brood and the spiderlings.

#include "Creatures/GravemotherCreature.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GroundCrack.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Her hide: the spider's texture set made pale (made by the main session; see the class's notes). */
	const TCHAR* PaleHidePath = TEXT("/Game/Art/Materials/MI_SpiderBody_Pale.MI_SpiderBody_Pale");

	/**
	 * An asset made after this code, found only once it exists: until then she wears the spider's own hide, and her tests
	 * run without it.
	 */
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
	// 1.8 times a spider as seen, her rank's own growth (x1.4) included.
	BodyScale = Gravemother::Size / Gravemother::RankSize;
	// The head as on every spider, and the swollen abdomen she drags about: their hit hulls, no new bones.
	CriticalSpotBones = { TEXT("head"), TEXT("abdomen") };

	// Heavy and unhurried until she means it (the charge is her speed), keeping close to her den while nobody's about.
	WalkSpeed = 140.f;
	ChaseSpeed = 480.f;
	WanderRadius = 400.f;
	// Her body lies a while longer: she's the fight's end.
	CorpseTime = 10.f;

	// Pale where a spider is brown (one slot: the spider's MI_SpiderBody). Set on the defaults, so the Ledger's stand
	// shows her as she looks out there.
	static UMaterialInterface* const PaleHide = FindIfMade<UMaterialInterface>(PaleHidePath);
	if (PaleHide)
	{
		GetMesh()->SetMaterial(0, PaleHide);
	}
}

void AGravemotherCreature::BeginPlay()
{
	Super::BeginPlay();
	// Her bite's timing as her class (or the level) gave it: the charge borrows the attack's and gives it back.
	BiteWindup = AttackWindup;
	BiteRecovery = AttackRecovery;
	// She sizes up whoever she first sees before she charges them.
	ChargeCooldownLeft = ChargeFirstDelay;
}

void AGravemotherCreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Taken away alive (her lair's player went far off) or removed: her brood goes with her, as she goes. Killed, she
	// leaves them fighting, and her body goes later.
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
	if (AGroundCrack* Open = Crack.Get())
	{
		Open->Destroy();
	}
	Crack = nullptr;
	Super::EndPlay(EndPlayReason);
}

void AGravemotherCreature::Tick(float DeltaSeconds)
{
	// The brain first (its attack runs the charge's wind-up and strike) and the spider's body; then the charge's own
	// frame: the crack opening, the dash.
	Super::Tick(DeltaSeconds);
	ChargeClock += DeltaSeconds;
	TickCharge(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Hurt, dying, coming back
// ---------------------------------------------------------------------------

void AGravemotherCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	Super::OnHurt(bCritical, HitLocation);
	// Her brood comes at two thirds and at one third of her health, each once a life; a hit past both lines brings both.
	// A killing blow brings none.
	if (IsDead() || !Health || Health->IsDead() || Health->GetHealth() <= 0.f)
	{
		return;
	}
	const float Share = Health->GetHealthPercent();
	while (BroodCallsMade < Gravemother::NumBroodCalls && Share <= Gravemother::BroodCallShare(BroodCallsMade))
	{
		++BroodCallsMade;
		CallBrood();
	}
}

void AGravemotherCreature::OnDied()
{
	Super::OnDied();
	if (ChargeState != EGravemotherCharge::None)
	{
		FinishCharge();
	}
}

void AGravemotherCreature::OnRespawned()
{
	Super::OnRespawned();
	// Back at her spot (a fight starting over): a new life's calls, and no charge under way.
	if (ChargeState != EGravemotherCharge::None)
	{
		FinishCharge();
	}
	BroodCallsMade = 0;
	ChargeCooldownLeft = ChargeFirstDelay;
}
