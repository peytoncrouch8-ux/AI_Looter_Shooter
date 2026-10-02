#include "Bosses/BossTestSpider.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Engine/World.h"

FName BossTestSpider::Tag()
{
	static const FName Name(TEXT("Boss_TestSpider"));
	return Name;
}

FBossAddWave BossTestSpider::Brood()
{
	FBossAddWave Wave;
	Wave.CreatureClass = ASpiderCreature::StaticClass();
	Wave.Rank = ECreatureRank::Basic;
	Wave.Count = 3;
	Wave.BodyScale = 0.55f;
	Wave.Radius = 650.f;
	return Wave;
}

FBossVolley BossTestSpider::VenomVolley()
{
	FBossVolley Volley;
	Volley.Pellets = 5;
	Volley.SpreadDegrees = 14.f;
	Volley.Speed = 1000.f;
	Volley.DamageShare = 0.35f;
	Volley.Radius = 12.f;
	Volley.Range = 4000.f;
	Volley.WindupSeconds = 0.6f;
	// Its fangs, a little up: the spider's head at size 1 (it scales with the boss).
	Volley.Muzzle = FVector(95.0, 0.0, 15.0);
	Volley.Color = FLinearColor(0.55f, 1.f, 0.15f);
	return Volley;
}

TArray<FBossPhase> BossTestSpider::MakePhases(int32 PhaseCount)
{
	const int32 Count = FMath::Clamp(PhaseCount, MinPhases, MaxPhases);

	FBossPhase Stirs;
	Stirs.Name = FText::FromString(TEXT("The Brood Stirs"));
	Stirs.HealthShare = 1.f;

	FBossPhase Hunger;
	Hunger.Name = FText::FromString(TEXT("Old Hunger"));
	Hunger.HealthShare = 0.75f;

	// At half health she wraps herself in silk and sends her brood; she can be hurt again once they're dead (or 45 s pass).
	FBossPhase Silk;
	Silk.Name = FText::FromString(TEXT("Under Silk"));
	Silk.HealthShare = 0.5f;
	FBossUntargetable Spell;
	Spell.Seconds = 45.f;
	Spell.bUntilAddsDie = true;
	Spell.bWithdraw = true;
	Spell.Hint = FText::FromString(TEXT("Kill her brood"));
	Silk.Events.Add(FBossPhaseEvent::MakeUntargetable(Spell));
	Silk.Events.Add(FBossPhaseEvent::MakeWave(Brood()));

	// From a quarter she spits venom: a five-pellet cone every 3 s, after a glowing tell.
	FBossPhase Venom;
	Venom.Name = FText::FromString(TEXT("Venom Rain"));
	Venom.HealthShare = 0.25f;
	Venom.Events.Add(FBossPhaseEvent::MakeVolley(VenomVolley(), 1.5f, 3.f));

	FBossPhase Legs;
	Legs.Name = FText::FromString(TEXT("Last Legs"));
	Legs.HealthShare = 0.1f;
	Legs.Events.Add(FBossPhaseEvent::MakeVolley(VenomVolley(), 0.5f, 2.f));

	TArray<FBossPhase> Result;
	Result.Add(Stirs);
	if (Count >= 4)
	{
		Result.Add(Hunger);
	}
	if (Count >= 2)
	{
		Result.Add(Silk);
	}
	if (Count >= 3)
	{
		Result.Add(Venom);
	}
	if (Count >= 5)
	{
		Result.Add(Legs);
	}
	return Result;
}

void BossTestSpider::Configure(UBossComponent& Boss, int32 PhaseCount)
{
	Boss.BossName = FText::FromString(TEXT("Brood Mother (test)"));
	// A test fight: never recorded in the campaign.
	Boss.BossId = NAME_None;
	Boss.Phases = MakePhases(PhaseCount);
	Boss.SealRadius = SealRadius;
	Boss.LeashRadius = SealRadius + 1000.f;
	// Walking back up to it starts the fight again (the wall closes once the player is well inside its ring).
	Boss.EngageRadius = SealRadius - 300.f;
	Boss.MaxAliveAdds = 10;
	Boss.bShowBar = true;
}

UBossComponent* BossTestSpider::Spawn(UWorld* World, const FVector& Feet, float Yaw, int32 PhaseCount)
{
	ACreatureBase::FRuntimeSpawn Setup;
	Setup.Rank = ECreatureRank::Boss;
	Setup.BodyScale = Size;
	Setup.HealthScale = HealthScale;
	ACreatureBase* Spider = ACreatureBase::SpawnAtRuntime(World, ASpiderCreature::StaticClass(), Feet, Yaw, Setup);
	if (!Spider)
	{
		return nullptr;
	}
	Spider->Tags.Add(Tag());

	UBossComponent* Boss = NewObject<UBossComponent>(Spider, TEXT("Boss"));
	Configure(*Boss, PhaseCount);
	Spider->AddInstanceComponent(Boss);
	// In a game the spider is playing already, so the component begins play as it registers.
	Boss->RegisterComponent();
	// A level that isn't playing (a test level) never begins anything itself: the spider starts as play would start it,
	// its boss component with it.
	if (!Spider->HasActorBegunPlay())
	{
		Spider->DispatchBeginPlay();
	}
	return Boss;
}
