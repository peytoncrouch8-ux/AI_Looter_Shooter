#include "UI/World/DamageNumberSubsystem.h"
#include "UI/World/DamageNumberActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

// The damage numbers' pool and merge (DamageNumberSubsystem.h). Numbers before 2026-11-03 were one spawned actor and widget per
// bullet or pellet, destroyed after a second: a shotgun blast was eight actors and eight numbers on one chest.

namespace
{
	const DamageNumberMerge::FTuning DamageNumberDefaults;

	TAutoConsoleVariable<float> CVarDamageNumbersMergeSeconds(TEXT("Looter.UI.DamageNumbers.MergeSeconds"), DamageNumberDefaults.MergeSeconds,
		TEXT("Hits on one target this soon (s) after its number's first are summed into it (a shotgun blast: one number). 0: every hit its own."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarDamageNumbersMergeGrow(TEXT("Looter.UI.DamageNumbers.MergeGrow"), DamageNumberDefaults.Grow,
		TEXT("A merged number grows by this share of its size per hit after the first..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDamageNumbersMergeGrowMax(TEXT("Looter.UI.DamageNumbers.MergeGrowMax"), DamageNumberDefaults.GrowMax,
		TEXT("...up to this many times its size."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDamageNumbersPoolSize(TEXT("Looter.UI.DamageNumbers.PoolSize"), DamageNumberDefaults.PoolSize,
		TEXT("Damage numbers made at play start and used again (4-64); a burst past it takes over the oldest hit's."), ECVF_Default);

	bool IsUsable(const ADamageNumberActor* Number)
	{
		return IsValid(Number) && !Number->IsActorBeingDestroyed();
	}
}

DamageNumberMerge::FTuning DamageNumberMerge::FTuning::Live()
{
	FTuning Tuning;
	Tuning.MergeSeconds = FMath::Max(CVarDamageNumbersMergeSeconds.GetValueOnGameThread(), 0.f);
	Tuning.Grow = FMath::Max(CVarDamageNumbersMergeGrow.GetValueOnGameThread(), 0.f);
	Tuning.GrowMax = FMath::Max(CVarDamageNumbersMergeGrowMax.GetValueOnGameThread(), 1.f);
	Tuning.PoolSize = FMath::Clamp(CVarDamageNumbersPoolSize.GetValueOnGameThread(), 4, 64);
	return Tuning;
}

bool DamageNumberMerge::Joins(double GroupStart, double Now, float MergeSeconds)
{
	// The window runs from the number's first hit, not its last: a held trigger's stream of bullets never makes one endless number.
	return MergeSeconds > 0.f && Now >= GroupStart && Now - GroupStart <= MergeSeconds;
}

bool DamageNumberMerge::SameLook(ELooterElement ShownElement, bool bShownStatusTint, const FElementNumberLook& Next, bool bNextCritical)
{
	// A sum must show in one colour: a smite's burst stays its own number beside the gun's hits that landed with it.
	const bool bNextStatusTint = Next.bTint && !bNextCritical;
	return Next.Element == ShownElement && bNextStatusTint == bShownStatusTint;
}

FElementNumberLook DamageNumberMerge::Merged(const FElementNumberLook& Shown, const FElementNumberLook& Next)
{
	FElementNumberLook Look = Shown;
	if (Look.Element == ELooterElement::None)
	{
		Look.Element = Next.Element;
	}
	// An element gun's crit pellet tints the sum's crit colour toward its element.
	Look.bTint = Shown.bTint || Next.bTint;
	// The fight's first weak hit called WEAK; the pellets after it say nothing, and the sum keeps the tag.
	if (Look.Callout == EElementAffinity::Neutral)
	{
		Look.Callout = Next.Callout;
	}
	Look.SizeScale = FMath::Max(Shown.SizeScale, Next.SizeScale);
	return Look;
}

float DamageNumberMerge::GrowFor(int32 Hits, const FTuning& Tuning)
{
	return FMath::Clamp(1.f + Tuning.Grow * FMath::Max(Hits - 1, 0), 1.f, FMath::Max(Tuning.GrowMax, 1.f));
}

UDamageNumberSubsystem* UDamageNumberSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UDamageNumberSubsystem>() : nullptr;
}

bool UDamageNumberSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UDamageNumberSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.IsGameWorld())
	{
		// All made now, while the level loads, so the first shots of a fight spawn nothing.
		Prewarm();
	}
}

void UDamageNumberSubsystem::Deinitialize()
{
	Pool.Reset();
	Super::Deinitialize();
}

void UDamageNumberSubsystem::Prewarm()
{
	const int32 PoolSize = DamageNumberMerge::FTuning::Live().PoolSize;
	Pool.RemoveAll([](const TObjectPtr<ADamageNumberActor>& Number) { return !IsUsable(Number); });
	while (Pool.Num() < PoolSize)
	{
		if (!Make(ADamageNumberActor::StaticClass()))
		{
			break;
		}
	}
}

int32 UDamageNumberSubsystem::NumPooled() const
{
	int32 Count = 0;
	for (const TObjectPtr<ADamageNumberActor>& Number : Pool)
	{
		Count += IsUsable(Number) ? 1 : 0;
	}
	return Count;
}

int32 UDamageNumberSubsystem::NumShowing() const
{
	int32 Count = 0;
	for (const TObjectPtr<ADamageNumberActor>& Number : Pool)
	{
		Count += IsUsable(Number) && Number->IsShowing() ? 1 : 0;
	}
	return Count;
}

ADamageNumberActor* UDamageNumberSubsystem::FindStream(const AActor* Creature, ELooterElement Element) const
{
	for (const TObjectPtr<ADamageNumberActor>& Number : Pool)
	{
		if (IsUsable(Number) && Number->IsStreamOf(Creature, Element))
		{
			return Number;
		}
	}
	return nullptr;
}

int32 UDamageNumberSubsystem::CountStreams(const AActor* Creature) const
{
	int32 Count = 0;
	for (const TObjectPtr<ADamageNumberActor>& Number : Pool)
	{
		Count += IsUsable(Number) && Number->IsStreamOn(Creature) ? 1 : 0;
	}
	return Count;
}

ADamageNumberActor* UDamageNumberSubsystem::Make(UClass* Class)
{
	UWorld* World = GetWorld();
	if (!World || !Class)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	ADamageNumberActor* Number = World->SpawnActor<ADamageNumberActor>(Class, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (Number)
	{
		Number->SetPooled();
		Pool.Add(Number);
		++Made;
	}
	return Number;
}

ADamageNumberActor* UDamageNumberSubsystem::Take(UClass* Class, int32 PoolSize)
{
	Pool.RemoveAll([](const TObjectPtr<ADamageNumberActor>& Number) { return !IsUsable(Number); });
	ADamageNumberActor* Oldest = nullptr;
	// Counted per class: a creature naming its own number class gets a pool of its own beside the plain one.
	int32 OfClass = 0;
	for (const TObjectPtr<ADamageNumberActor>& Number : Pool)
	{
		if (Number->GetClass() != Class)
		{
			continue;
		}
		++OfClass;
		if (!Number->IsShowing())
		{
			return Number;
		}
		// A stream holds a creature's running total for as long as its status burns: a burst takes over a hit's number instead.
		if (Number->GetKind() == EDamageNumberKind::Hit && (!Oldest || Number->GetAge() > Oldest->GetAge()))
		{
			Oldest = Number;
		}
	}
	if (OfClass < PoolSize)
	{
		if (ADamageNumberActor* Fresh = Make(Class))
		{
			return Fresh;
		}
	}
	// All busy: the one up longest (nearly gone anyway).
	return Oldest;
}

ADamageNumberActor* UDamageNumberSubsystem::FindMerge(const FDamageNumberHit& Hit, double Now, float MergeSeconds) const
{
	ADamageNumberActor* Found = nullptr;
	for (const TObjectPtr<ADamageNumberActor>& Number : Pool)
	{
		if (!IsUsable(Number) || !Number->IsShowing() || Number->GetKind() != EDamageNumberKind::Hit || Number->GetTarget() != Hit.Target
			|| !DamageNumberMerge::Joins(Number->GetGroupStart(), Now, MergeSeconds)
			|| !DamageNumberMerge::SameLook(Number->GetLook().Element, Number->IsStatusTinted(), Hit.Look, Hit.bCritical))
		{
			continue;
		}
		// The latest of its numbers (another look's hit may have started one beside it).
		if (!Found || Number->GetGroupStart() > Found->GetGroupStart())
		{
			Found = Number;
		}
	}
	return Found;
}

ADamageNumberActor* UDamageNumberSubsystem::ShowHit(const FDamageNumberHit& Hit)
{
	const UWorld* World = GetWorld();
	return ShowHitAt(Hit, World ? World->GetTimeSeconds() : 0.0);
}

ADamageNumberActor* UDamageNumberSubsystem::ShowHitAt(const FDamageNumberHit& Hit, double Now)
{
	UClass* Class = Hit.Class ? Hit.Class : ADamageNumberActor::StaticClass();
	const DamageNumberMerge::FTuning Tuning = DamageNumberMerge::FTuning::Live();

	// A status's piece on a creature: into its stream (one rolling number per element), never merged with the hits.
	const ELooterElement Streamed = DamageNumberStream::StreamElementOf(Hit.bCritical, Hit.bStatusPiece, Hit.Look.Element);
	if (Streamed != ELooterElement::None && Hit.Target)
	{
		if (ADamageNumberActor* Stream = FindStream(Hit.Target, Streamed))
		{
			Stream->AddToStream(Hit.Damage, Hit.Look);
			return Stream;
		}
		// Its place up the screen over the creature's streams already showing, counted before this one is one.
		const int32 Slot = FMath::Min(CountStreams(Hit.Target), DamageNumberStream::MaxSlots - 1);
		ADamageNumberActor* Number = Take(Class, Tuning.PoolSize);
		if (Number)
		{
			Number->ShowStream(Hit.Damage, Hit.Look, Hit.Where, Hit.Target, Streamed, Slot);
		}
		return Number;
	}

	// Another hit on a target whose number is still young: summed into it.
	if (Hit.Target)
	{
		if (ADamageNumberActor* Shown = FindMerge(Hit, Now, Tuning.MergeSeconds))
		{
			Shown->MergeHit(Hit.Damage, Hit.bCritical, Hit.Look, DamageNumberMerge::GrowFor(Shown->GetMergedHits() + 1, Tuning));
			return Shown;
		}
	}
	ADamageNumberActor* Number = Take(Class, Tuning.PoolSize);
	if (Number)
	{
		Number->ShowHit(Hit.Damage, Hit.bCritical, Hit.Look, Hit.Where, Hit.Target, Now);
	}
	return Number;
}
