#include "UI/World/DamageNumberActor.h"
#include "UI/World/DamageNumberSubsystem.h"
#include "UI/World/DamageNumberWidget.h"
#include "Components/WidgetComponent.h"

// ADamageNumberActor's hit numbers and its life in the pool (the stream and the callout are DamageNumberActorStream.cpp's).

namespace
{
	/** A normal hit's number ticks in from this size over PopSeconds. */
	constexpr float NormalPop = 1.25f;
	constexpr float NormalPopSeconds = 0.1f;
	/** A crit's slam: down past its size by SlamSeconds, back up to it by SettleSeconds. */
	constexpr float CritUndershoot = 0.9f;
	constexpr float SlamSeconds = 0.08f;
	constexpr float SettleSeconds = 0.2f;
	/** White-hot for this long, cooled into the crit colour by HeatSeconds. */
	constexpr float HotSeconds = 0.06f;
	constexpr float HeatSeconds = 0.2f;
	/** The tilt the way it flies. */
	constexpr float CritTilt = 7.f;
}

DamageNumberMotion::FFrame DamageNumberMotion::At(float Age, bool bCritical, float Side)
{
	FFrame Frame;
	const float T = FMath::Max(Age, 0.f);
	if (!bCritical)
	{
		Frame.Scale = FMath::Lerp(NormalPop, 1.f, FMath::Clamp(T / NormalPopSeconds, 0.f, 1.f));
		return Frame;
	}
	const float Way = Side < 0.f ? -1.f : 1.f;
	// The slam: huge, shrinking past its size, then a little bounce back up to it.
	Frame.Scale = T < SlamSeconds
		? FMath::Lerp(CritSlamScale, CritUndershoot, FMath::InterpEaseIn(0.f, 1.f, T / SlamSeconds, 2.f))
		: FMath::Lerp(CritUndershoot, 1.f, FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp((T - SlamSeconds) / (SettleSeconds - SlamSeconds), 0.f, 1.f), 2.f));
	// The lob: up and over to its side, the pull bringing it back down a little by the end.
	Frame.Offset = FVector2D(Way * CritLobSide * (1.f - FMath::Exp(-4.f * T)), -(CritLobSpeed * T - 0.5f * CritLobPull * T * T));
	Frame.Angle = Way * CritTilt * (1.f - FMath::Exp(-6.f * T));
	Frame.Heat = 1.f - FMath::Clamp((T - HotSeconds) / (HeatSeconds - HotSeconds), 0.f, 1.f);
	return Frame;
}

float DamageNumberMotion::TagScaleAt(float Age)
{
	return FMath::Lerp(TagPopScale, 1.f, FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Age / TagPopSeconds, 0.f, 1.f), 2.f));
}

ADamageNumberActor::ADamageNumberActor()
{
	// It ticks only while it shows: a pooled one waits hidden and still.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	// Screen space: the same size on the screen however far the hit was.
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetPivot(FVector2D(0.5f, 0.5f));
	Widget->SetWidgetClass(UDamageNumberWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	Widget->SetVisibility(false);
	SetRootComponent(Widget);
}

float ADamageNumberActor::GetLife() const
{
	return FMath::Max(Lifetime + (bCritical ? CriticalExtraLife : 0.f), 0.01f);
}

UDamageNumberWidget* ADamageNumberActor::GetNumberWidget() const
{
	return Widget ? Cast<UDamageNumberWidget>(Widget->GetUserWidgetObject()) : nullptr;
}

FElementNumberLook ADamageNumberActor::ShownLook() const
{
	FElementNumberLook Shown = Look;
	Shown.SizeScale *= Grow;
	return Shown;
}

void ADamageNumberActor::Begin(const FVector& Where)
{
	bShowing = true;
	SetActorLocation(Where);
	if (!Widget->GetUserWidgetObject())
	{
		// Made at BeginPlay in a played level; a test level never begins play, so it's made on first show.
		Widget->InitWidget();
	}
	Widget->SetVisibility(true);
	SetActorTickEnabled(true);
}

void ADamageNumberActor::Finish()
{
	bShowing = false;
	if (!bPooled)
	{
		Destroy();
		return;
	}
	Kind = EDamageNumberKind::Hit;
	Target.Reset();
	StreamElement = ELooterElement::None;
	MergedHits = 0;
	Widget->SetVisibility(false);
	SetActorTickEnabled(false);
}

void ADamageNumberActor::ShowHit(float Damage, bool bInCritical, const FElementNumberLook& InLook, const FVector& Where, AActor* InTarget,
	double Now)
{
	Kind = EDamageNumberKind::Hit;
	Target = InTarget;
	GroupStart = Now;
	Total = FMath::Max(Damage, 0.f);
	MergedHits = 1;
	Grow = 1.f;
	bCritical = bInCritical;
	Look = InLook;
	// A status's own colour (a smite's burst) is a blow of its own: only a hit like it joins its number.
	bStatusTinted = InLook.bTint && !bInCritical;
	bTagged = Look.Callout != EElementAffinity::Neutral;
	TagAge = 0.f;
	Age = 0.f;
	Side = FMath::RandBool() ? 1.f : -1.f;

	// A crit's own lob does its travelling on the screen; it only rises a little in the world.
	const FVector2D Drift = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * MaxDriftSpeed * (bCritical ? 0.3f : 1.f);
	Velocity = FVector(Drift.X, Drift.Y, RiseSpeed * (bCritical ? 0.6f : 1.f));

	Begin(Where);
	if (UDamageNumberWidget* NumberWidget = GetNumberWidget())
	{
		NumberWidget->SetDamage(Total, bCritical, ShownLook());
	}
	// Its first frame's look now, not the last use's for a frame.
	TickHit(0.f);
}

void ADamageNumberActor::MergeHit(float Damage, bool bInCritical, const FElementNumberLook& InLook, float InGrow)
{
	if (!bShowing || Kind != EDamageNumberKind::Hit)
	{
		return;
	}
	Total += FMath::Max(Damage, 0.f);
	++MergedHits;
	Grow = FMath::Max(InGrow, 0.1f);
	// A weakness called out by a later pellet still pops its tag beside the sum.
	if (!bTagged && InLook.Callout != EElementAffinity::Neutral)
	{
		bTagged = true;
		TagAge = 0.f;
	}
	Look = DamageNumberMerge::Merged(Look, InLook);
	if (bInCritical && !bCritical)
	{
		// A crit pellet in the blast: the whole number is a crit's now, slamming in where it is. Its place and its rise go on.
		bCritical = true;
		Age = 0.f;
	}
	if (UDamageNumberWidget* NumberWidget = GetNumberWidget())
	{
		NumberWidget->SetTotal(Total, bCritical, ShownLook());
	}
	TickHit(0.f);
}

void ADamageNumberActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ADamageNumberActor::Advance(float DeltaSeconds)
{
	if (!bShowing)
	{
		return;
	}
	switch (Kind)
	{
	case EDamageNumberKind::Stream: TickStream(DeltaSeconds); break;
	case EDamageNumberKind::Callout: TickCallout(DeltaSeconds); break;
	case EDamageNumberKind::Hit:
	default: TickHit(DeltaSeconds); break;
	}
}

void ADamageNumberActor::TickHit(float DeltaSeconds)
{
	const float Step = FMath::Max(DeltaSeconds, 0.f);
	Age += Step;
	TagAge += Step;
	const float Life = GetLife();
	if (Age >= Life)
	{
		Finish();
		return;
	}
	AddActorWorldOffset(Velocity * Step);
	Velocity *= FMath::Max(0.f, 1.f - 2.f * Step); // ease out

	UDamageNumberWidget* NumberWidget = GetNumberWidget();
	if (!NumberWidget)
	{
		return;
	}
	// Fade over the last 40% of the lifetime.
	const float Alpha = FMath::Clamp(Age / Life, 0.f, 1.f);
	NumberWidget->SetRenderOpacity(Alpha < 0.6f ? 1.f : 1.f - (Alpha - 0.6f) / 0.4f);

	const DamageNumberMotion::FFrame Frame = DamageNumberMotion::At(Age, bCritical, Side);
	FWidgetTransform Transform;
	Transform.Translation = Frame.Offset;
	Transform.Scale = FVector2D(Frame.Scale);
	Transform.Angle = Frame.Angle;
	NumberWidget->SetRenderTransform(Transform);
	if (bCritical)
	{
		NumberWidget->SetHeat(Frame.Heat);
	}
	if (bTagged && TagAge < DamageNumberMotion::TagPopSeconds + 0.1f)
	{
		NumberWidget->SetTagScale(DamageNumberMotion::TagScaleAt(TagAge));
	}
}
