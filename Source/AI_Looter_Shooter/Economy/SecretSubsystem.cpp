// USecretSubsystem: the level's catalog of Sexton's secrets, its rows, buying one and the places the bought ones mark.

#include "Economy/SecretSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Economy/SecretCatalog.h"
#include "Economy/WalletSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/FindRules.h"
#include "World/FindSpot.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/PackageName.h"

USecretSubsystem* USecretSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<USecretSubsystem>() : nullptr;
}

bool USecretSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests (they give it a catalog: SetCatalogForTesting).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

// ---------------------------------------------------------------------------
// The catalog and the story
// ---------------------------------------------------------------------------

const USecretCatalog* USecretSubsystem::GetCatalog() const
{
	if (!bCatalogLooked)
	{
		// Asked once a level: the catalog named for the level, when this checkout has one (most levels sell nothing).
		USecretSubsystem* Self = const_cast<USecretSubsystem*>(this);
		Self->bCatalogLooked = true;
		const UWorld* World = GetWorld();
		const FString Path = World ? SecretRules::CatalogPathFor(World->GetMapName()) : FString();
		const FString Package = FPackageName::ObjectPathToPackageName(Path);
		if (!Path.IsEmpty() && FPackageName::DoesPackageExist(Package))
		{
			Self->Catalog = LoadObject<USecretCatalog>(nullptr, *Path);
			UE_LOG(LogLooter, Log, TEXT("Secrets: %s %s (%d secrets)."), *Path, Catalog ? TEXT("loaded") : TEXT("failed to load"),
				Catalog ? Catalog->Secrets.Num() : 0);
		}
	}
	return Catalog;
}

void USecretSubsystem::SetCatalogForTesting(USecretCatalog* InCatalog)
{
	Catalog = InCatalog;
	bCatalogLooked = InCatalog != nullptr;
	bTestCatalog = InCatalog != nullptr;
	Spots.Reset();
	Tagged.Reset();
	TaggedLookedAt.Reset();
	SpotsLookedAt = -1.0e9;
}

UMissionRunner* USecretSubsystem::GetRunner() const
{
	return UMissionRunner::Get(this);
}

const FCampaignRecord* USecretSubsystem::GetRecord() const
{
	const UMissionRunner* Runner = GetRunner();
	return Runner && Runner->IsActive() ? &Runner->GetCampaign() : nullptr;
}

bool USecretSubsystem::IsOpen() const
{
	const USecretCatalog* Here = GetCatalog();
	const FCampaignRecord* Record = GetRecord();
	return Here && Record && Here->IsOpen(*Record, GetRunner());
}

float USecretSubsystem::GetDiscount() const
{
	const USecretCatalog* Here = GetCatalog();
	const FCampaignRecord* Record = GetRecord();
	return Here && Record ? Here->DiscountFor(*Record) : 0.f;
}

FText USecretSubsystem::GetGreeting() const
{
	const USecretCatalog* Here = GetCatalog();
	const FCampaignRecord* Record = GetRecord();
	if (!Here || !Record)
	{
		return FText::GetEmpty();
	}
	const TArray<FSecretRow> Rows = GetRows();
	const bool bAnyForSale = Rows.ContainsByPredicate([](const FSecretRow& Row) { return Row.State == ESecretState::ForSale; });
	return bAnyForSale || Here->SoldOut.IsEmpty() ? Here->GreetingFor(*Record, GetRunner()) : Here->SoldOut;
}

// ---------------------------------------------------------------------------
// The list
// ---------------------------------------------------------------------------

TArray<FSecretRow> USecretSubsystem::GetRows() const
{
	const FCampaignRecord* Record = GetRecord();
	return Record ? GetRows(*Record, GetRunner(), UWalletSubsystem::PlayerLevelOf(this)) : TArray<FSecretRow>();
}

TArray<FSecretRow> USecretSubsystem::GetRows(const FCampaignRecord& Record, const UMissionRunner* Runner, int32 PlayerLevel) const
{
	TArray<FSecretRow> ForSale;
	TArray<FSecretRow> Sold;
	const USecretCatalog* Here = GetCatalog();
	if (!Here)
	{
		return ForSale;
	}
	const float Discount = Here->DiscountFor(Record);
	for (const FSecretOffer& Offer : Here->Secrets)
	{
		const ESecretState State = SecretRules::StateOf(Offer, Record, Runner);
		if (!SecretRules::IsListed(State))
		{
			continue;
		}
		// Nowhere to mark it in this level (its find isn't built here): nothing to sell.
		const int32 Places = CountPlaces(Offer, Record);
		if (Places == 0)
		{
			continue;
		}
		FSecretRow Row;
		Row.Id = Offer.Id;
		Row.Title = Offer.Title;
		Row.Teaser = Offer.Teaser;
		Row.Line = Offer.Line;
		Row.Region = Offer.Region;
		Row.Kind = Offer.Kind;
		Row.State = State;
		Row.Price = SecretRules::PriceFor(Offer, PlayerLevel, Discount);
		Row.Places = Places;
		(State == ESecretState::Sold ? Sold : ForSale).Add(MoveTemp(Row));
	}
	ForSale.Append(MoveTemp(Sold));
	return ForSale;
}

// ---------------------------------------------------------------------------
// Buying
// ---------------------------------------------------------------------------

ESecretBuy USecretSubsystem::Buy(FName Id, bool bFree)
{
	const USecretCatalog* Here = GetCatalog();
	UMissionRunner* Runner = GetRunner();
	const FCampaignRecord* Record = GetRecord();
	const FSecretOffer* Offer = Here ? Here->FindOffer(Id) : nullptr;
	if (!Here || !Runner || !Record)
	{
		return ESecretBuy::NoTrade;
	}
	if (!Offer)
	{
		return ESecretBuy::NotForSale;
	}
	const ESecretState State = SecretRules::StateOf(*Offer, *Record, Runner);
	if (State == ESecretState::Sold)
	{
		return ESecretBuy::AlreadySold;
	}
	if (State != ESecretState::ForSale || CountPlaces(*Offer, *Record) == 0)
	{
		return ESecretBuy::NotForSale;
	}
	const int32 Price = SecretRules::PriceFor(*Offer, UWalletSubsystem::PlayerLevelOf(this), Here->DiscountFor(*Record));
	if (!bFree)
	{
		UWalletSubsystem* Wallet = UWalletSubsystem::Get(this);
		if (!Wallet)
		{
			return ESecretBuy::NoTrade;
		}
		if (!Wallet->CanAfford(Price))
		{
			return ESecretBuy::NotEnoughSilver;
		}
		Wallet->Spend(Price);
	}
	// The flag is the purchase: saved soon after with the story (the wallet goes with the same save), so it stays bought.
	Runner->SetFlag(SecretRules::SoldFlag(Offer->Id));
	if (!Offer->SetsFlag.IsNone())
	{
		Runner->SetFlag(Offer->SetsFlag);
	}
	UE_LOG(LogLooter, Log, TEXT("Secrets: %s sold%s (%s)."), *Offer->Id.ToString(),
		bFree ? TEXT(" free") : *FString::Printf(TEXT(" for %d silver"), Price), *Offer->Title.ToString());
	OnSold.Broadcast(Offer->Id);
	return ESecretBuy::Bought;
}

bool USecretSubsystem::Forget(FName Id)
{
	UMissionRunner* Runner = GetRunner();
	if (!Runner || !Runner->IsActive())
	{
		return false;
	}
	const bool bWas = Runner->GetCampaign().ClearFlag(SecretRules::SoldFlag(Id));
	if (bWas)
	{
		OnSold.Broadcast(Id);
	}
	return bWas;
}

int32 USecretSubsystem::ForgetAll()
{
	int32 Forgotten = 0;
	if (const USecretCatalog* Here = GetCatalog())
	{
		for (const FSecretOffer& Offer : Here->Secrets)
		{
			Forgotten += Forget(Offer.Id) ? 1 : 0;
		}
	}
	return Forgotten;
}

// ---------------------------------------------------------------------------
// Marks
// ---------------------------------------------------------------------------

TArray<FSecretMark> USecretSubsystem::GetMarks() const
{
	const FCampaignRecord* Record = GetRecord();
	return Record ? GetMarks(*Record, GetRunner()) : TArray<FSecretMark>();
}

TArray<FSecretMark> USecretSubsystem::GetMarks(const FCampaignRecord& Record, const UMissionRunner* Runner) const
{
	TArray<FSecretMark> Marks;
	const USecretCatalog* Here = GetCatalog();
	if (!Here)
	{
		return Marks;
	}
	TArray<FVector> Places;
	for (const FSecretOffer& Offer : Here->Secrets)
	{
		if (SecretRules::StateOf(Offer, Record, Runner) != ESecretState::Sold)
		{
			continue;
		}
		Places.Reset();
		GatherPlaces(Offer, Record, Places);
		for (const FVector& Place : Places)
		{
			FSecretMark& Mark = Marks.AddDefaulted_GetRef();
			Mark.Id = Offer.Id;
			Mark.Location = Place;
			Mark.Title = Offer.Title;
			Mark.Line = Offer.Line;
			Mark.Kind = Offer.Kind;
		}
	}
	return Marks;
}

int32 USecretSubsystem::CountPlaces(const FSecretOffer& Offer, const FCampaignRecord& Record) const
{
	TArray<FVector> Places;
	GatherPlaces(Offer, Record, Places);
	return Places.Num();
}

void USecretSubsystem::GatherPlaces(const FSecretOffer& Offer, const FCampaignRecord& Record, TArray<FVector>& Out) const
{
	if (!Offer.FindIds.IsEmpty())
	{
		// Each find of it not found yet, where its spot stands (the Ledger pages: the ones still lying out there).
		for (const FName FindId : Offer.FindIds)
		{
			if (Record.HasFlag(FindRules::FlagFor(FindId)))
			{
				continue;
			}
			if (const AFindSpot* Spot = FindSpot(FindId))
			{
				Out.Add(Spot->GetActorLocation());
			}
		}
		return;
	}
	if (!Offer.MarkTag.IsNone())
	{
		TArray<AActor*> Actors;
		FindTagged(Offer.MarkTag, Actors);
		for (const AActor* Actor : Actors)
		{
			Out.Add(Actor->GetActorLocation());
		}
	}
}

AFindSpot* USecretSubsystem::FindSpot(FName FindId) const
{
	if (const TWeakObjectPtr<AFindSpot>* Known = Spots.Find(FindId))
	{
		if (AFindSpot* Spot = Known->Get())
		{
			return Spot;
		}
	}
	// Not known (or gone): the level's spots once more, no sooner than LookAgainSeconds since the last look (a test level's
	// clock stands still, so a test looks every time).
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (!World || (!bTestCatalog && Now - SpotsLookedAt < LookAgainSeconds))
	{
		return nullptr;
	}
	SpotsLookedAt = Now;
	Spots.Reset();
	for (TActorIterator<AFindSpot> It(World); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && !It->FindId.IsNone())
		{
			Spots.Add(It->FindId, *It);
		}
	}
	const TWeakObjectPtr<AFindSpot>* Found = Spots.Find(FindId);
	return Found ? Found->Get() : nullptr;
}

void USecretSubsystem::FindTagged(FName Tag, TArray<AActor*>& Out) const
{
	if (const TArray<TWeakObjectPtr<AActor>>* Known = Tagged.Find(Tag))
	{
		for (const TWeakObjectPtr<AActor>& Each : *Known)
		{
			if (AActor* Actor = Each.Get())
			{
				Out.Add(Actor);
			}
		}
		if (!Out.IsEmpty())
		{
			return;
		}
	}
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	const double* Last = TaggedLookedAt.Find(Tag);
	if (!World || (!bTestCatalog && Last && Now - *Last < LookAgainSeconds))
	{
		return;
	}
	// A story secret's place: every actor carrying the tag (one look over the level, kept).
	TaggedLookedAt.Add(Tag, Now);
	TArray<TWeakObjectPtr<AActor>>& Kept = Tagged.FindOrAdd(Tag);
	Kept.Reset();
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->ActorHasTag(Tag))
		{
			Kept.Add(*It);
			Out.Add(*It);
		}
	}
}
