// ADamageNumberActor's stream and callout (Elements matter, 2026-10-29). A status's pieces of damage over time (a burn's, the
// salt's, the rot's) used to float one number each, a ladder of small numbers every half second; now they roll into one
// number per creature per element that counts up while they come and fades once they stop, so the player reads what the
// status is doing in total. The first time a status is set on a creature its word ("BURNING") pops over it in its colour
// (UStatusCalloutSubsystem spawns it). The hit's own number is DamageNumberActor.cpp's.

#include "UI/World/DamageNumberActor.h"
#include "UI/World/DamageNumberWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// ---------------------------------------------------------------------------
// The rules, as plain math
// ---------------------------------------------------------------------------

ELooterElement DamageNumberStream::StreamElementOf(bool bCritical, bool bDealingPiece, ELooterElement Element)
{
	// Only a status's piece streams: a smite's or a jolt's burst is a blow of its own and keeps its own number, as a hit does.
	return !bCritical && bDealingPiece ? Element : ELooterElement::None;
}

float DamageNumberStream::RollToward(float Shown, float Total, float DeltaSeconds)
{
	if (Shown >= Total)
	{
		return Total;
	}
	// Counting up, not jumping: quick when far behind (a big piece), never slower than RollMinRate, never past the total.
	const float Rate = FMath::Max(RollMinRate, (Total - Shown) * RollCatchUp);
	return FMath::Min(Total, Shown + Rate * FMath::Max(DeltaSeconds, 0.f));
}

float DamageNumberStream::OpacityAt(float SincePiece)
{
	if (SincePiece <= HoldSeconds)
	{
		return 1.f;
	}
	if (IsSpent(SincePiece))
	{
		return 0.f;
	}
	return 1.f - FMath::Clamp((SincePiece - HoldSeconds) / FadeSeconds, 0.f, 1.f);
}

float DamageNumberStream::PulseAt(float SincePiece)
{
	if (SincePiece < 0.f || SincePiece >= PulseSeconds)
	{
		return 1.f;
	}
	const float Left = 1.f - SincePiece / PulseSeconds;
	return FMath::Lerp(1.f, PulseScale, Left * Left);
}

float DamageNumberStream::RiseAt(float Age)
{
	return RiseHeight * (1.f - FMath::Exp(-4.f * FMath::Max(Age, 0.f)));
}

float DamageNumberCallout::ScaleAt(float Age)
{
	if (Age < 0.f || Age >= PopSeconds)
	{
		return 1.f;
	}
	// Slammed in big and settled fast, like a crit's number: it reads as news, not as another tick.
	const float Left = 1.f - Age / PopSeconds;
	return FMath::Lerp(1.f, PopScale, Left * Left);
}

float DamageNumberCallout::OpacityAt(float Age)
{
	if (Age >= Seconds)
	{
		return 0.f;
	}
	const float Share = FMath::Max(Age, 0.f) / Seconds;
	const float FadeFrom = 1.f - FadeShare;
	if (Share <= FadeFrom)
	{
		return 1.f;
	}
	return 1.f - FMath::Clamp((Share - FadeFrom) / FadeShare, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// Finding a creature's streams
// ---------------------------------------------------------------------------

ADamageNumberActor* ADamageNumberActor::FindStream(const UWorld* World, const AActor* Creature, ELooterElement Element)
{
	if (!World || !Creature || Element == ELooterElement::None)
	{
		return nullptr;
	}
	// A handful of numbers are up at most: a walk over them is cheaper than keeping a registry right through deaths and levels.
	for (TActorIterator<ADamageNumberActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		ADamageNumberActor* Number = *It;
		if (Number->Kind == EDamageNumberKind::Stream && Number->StreamElement == Element && Number->StreamCreature.Get() == Creature
			&& !DamageNumberStream::IsSpent(Number->SincePiece))
		{
			return Number;
		}
	}
	return nullptr;
}

int32 ADamageNumberActor::CountStreams(const UWorld* World, const AActor* Creature)
{
	int32 Count = 0;
	if (!World || !Creature)
	{
		return Count;
	}
	for (TActorIterator<ADamageNumberActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const ADamageNumberActor* Number = *It;
		Count += Number->Kind == EDamageNumberKind::Stream && Number->StreamCreature.Get() == Creature
			&& !DamageNumberStream::IsSpent(Number->SincePiece) ? 1 : 0;
	}
	return Count;
}

// ---------------------------------------------------------------------------
// The stream
// ---------------------------------------------------------------------------

void ADamageNumberActor::StartStream(float Damage, const FElementNumberLook& Look, AActor* Creature, ELooterElement Element)
{
	// Its place up the screen: over the streams already showing on this creature (counted before this one is a stream).
	Slot = FMath::Min(CountStreams(GetWorld(), Creature), DamageNumberStream::MaxSlots - 1);
	Kind = EDamageNumberKind::Stream;
	bCritical = false;
	StreamCreature = Creature;
	StreamElement = Element;
	// Where the first piece landed on the body, kept as it moves (a burning creature running).
	StreamAnchor = GetActorLocation() - Creature->GetActorLocation();
	StreamTotal = StreamShown = FMath::Max(Damage, 0.f);
	SincePiece = 0.f;
	Age = 0.f;
	bTagged = Look.Callout != EElementAffinity::Neutral;
	TagAge = 0.f;
	if (UDamageNumberWidget* NumberWidget = Cast<UDamageNumberWidget>(Widget->GetUserWidgetObject()))
	{
		NumberWidget->SetDamage(StreamShown, false, Look);
	}
	StreamShownWhole = FMath::Max(1, FMath::RoundToInt(StreamShown));
	// It lives as long as the pieces keep coming (TickStream lets it go once spent).
	SetLifeSpan(0.f);
}

void ADamageNumberActor::AddToStream(float Damage, const FElementNumberLook& Look)
{
	StreamTotal += FMath::Max(Damage, 0.f);
	SincePiece = 0.f;
	// A weakness called out at a later piece (a new fight on the same creature) still pops its tag beside the stream.
	if (!bTagged && Look.Callout != EElementAffinity::Neutral)
	{
		bTagged = true;
		TagAge = 0.f;
		if (UDamageNumberWidget* NumberWidget = Cast<UDamageNumberWidget>(Widget->GetUserWidgetObject()))
		{
			NumberWidget->SetDamage(StreamShown, false, Look);
		}
		StreamShownWhole = -1;
	}
}

void ADamageNumberActor::TickStream(float DeltaSeconds)
{
	Age += DeltaSeconds;
	SincePiece += DeltaSeconds;
	TagAge += DeltaSeconds;
	if (DamageNumberStream::IsSpent(SincePiece))
	{
		Destroy();
		return;
	}
	StreamShown = DamageNumberStream::RollToward(StreamShown, StreamTotal, DeltaSeconds);
	// Over its creature as it moves, risen a little; where it was last once the creature is gone.
	if (const AActor* Creature = StreamCreature.Get())
	{
		SetActorLocation(Creature->GetActorLocation() + StreamAnchor + FVector(0.0, 0.0, DamageNumberStream::RiseAt(Age)));
	}

	UDamageNumberWidget* NumberWidget = Cast<UDamageNumberWidget>(Widget->GetUserWidgetObject());
	if (!NumberWidget)
	{
		return;
	}
	NumberWidget->SetRenderOpacity(DamageNumberStream::OpacityAt(SincePiece));
	FWidgetTransform Transform;
	Transform.Translation = FVector2D(0.0, -DamageNumberStream::SlotStep * Slot);
	Transform.Scale = FVector2D(DamageNumberStream::PulseAt(SincePiece));
	NumberWidget->SetRenderTransform(Transform);
	const int32 Whole = FMath::Max(1, FMath::RoundToInt(StreamShown));
	if (Whole != StreamShownWhole)
	{
		StreamShownWhole = Whole;
		NumberWidget->SetValue(StreamShown);
	}
	if (bTagged && TagAge < DamageNumberMotion::TagPopSeconds + 0.1f)
	{
		NumberWidget->SetTagScale(DamageNumberMotion::TagScaleAt(TagAge));
	}
}

// ---------------------------------------------------------------------------
// The callout
// ---------------------------------------------------------------------------

void ADamageNumberActor::ShowCallout(ELooterElement Element, int32 InSlot)
{
	Kind = EDamageNumberKind::Callout;
	Age = 0.f;
	Slot = FMath::Clamp(InSlot, 0, DamageNumberStream::MaxSlots - 1);
	// Straight up and slower than a hit's number, so it stays readable over the creature a moment.
	Velocity = FVector(0.0, 0.0, DamageNumberCallout::RiseSpeed);
	if (UDamageNumberWidget* NumberWidget = Cast<UDamageNumberWidget>(Widget->GetUserWidgetObject()))
	{
		NumberWidget->SetCallout(Element);
	}
	SetLifeSpan(DamageNumberCallout::Seconds + 0.1f);
}

void ADamageNumberActor::TickCallout(float DeltaSeconds)
{
	Age += DeltaSeconds;
	AddActorWorldOffset(Velocity * DeltaSeconds);
	Velocity *= FMath::Max(0.f, 1.f - 1.5f * DeltaSeconds);
	UUserWidget* UserWidget = Widget->GetUserWidgetObject();
	if (!UserWidget)
	{
		return;
	}
	UserWidget->SetRenderOpacity(DamageNumberCallout::OpacityAt(Age));
	FWidgetTransform Transform;
	Transform.Translation = FVector2D(0.0, -(DamageNumberCallout::Lift + DamageNumberCallout::SlotStep * Slot));
	Transform.Scale = FVector2D(DamageNumberCallout::ScaleAt(Age));
	UserWidget->SetRenderTransform(Transform);
}
