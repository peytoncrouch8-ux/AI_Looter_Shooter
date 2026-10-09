#include "UI/World/DamageNumberActor.h"
#include "UI/World/DamageNumberWidget.h"
#include "Components/WidgetComponent.h"

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

ADamageNumberActor::ADamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	Widget->SetPivot(FVector2D(0.5f, 0.5f));
	Widget->SetWidgetClass(UDamageNumberWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Widget);
}

void ADamageNumberActor::Show(float Damage, bool bInCritical)
{
	bCritical = bInCritical;
	Age = 0.f;
	Side = FMath::RandBool() ? 1.f : -1.f;

	// A crit's own lob does its travelling on the screen; it only rises a little in the world.
	const FVector2D Drift = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * MaxDriftSpeed * (bCritical ? 0.3f : 1.f);
	Velocity = FVector(Drift.X, Drift.Y, RiseSpeed * (bCritical ? 0.6f : 1.f));

	if (UDamageNumberWidget* NumberWidget = Cast<UDamageNumberWidget>(Widget->GetUserWidgetObject()))
	{
		NumberWidget->SetDamage(Damage, bCritical);
	}

	SetLifeSpan(Lifetime + (bCritical ? CriticalExtraLife : 0.f) + 0.1f);
}

void ADamageNumberActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	AddActorWorldOffset(Velocity * DeltaSeconds);
	Velocity *= FMath::Max(0.f, 1.f - 2.f * DeltaSeconds); // ease out

	UUserWidget* UserWidget = Widget->GetUserWidgetObject();
	if (!UserWidget)
	{
		return;
	}

	const float Life = FMath::Max(Lifetime + (bCritical ? CriticalExtraLife : 0.f), 0.01f);
	const float Alpha = FMath::Clamp(Age / Life, 0.f, 1.f);

	// Fade over the last 40% of the lifetime.
	UserWidget->SetRenderOpacity(Alpha < 0.6f ? 1.f : 1.f - (Alpha - 0.6f) / 0.4f);

	const DamageNumberMotion::FFrame Frame = DamageNumberMotion::At(Age, bCritical, Side);
	FWidgetTransform Transform;
	Transform.Translation = Frame.Offset;
	Transform.Scale = FVector2D(Frame.Scale);
	Transform.Angle = Frame.Angle;
	UserWidget->SetRenderTransform(Transform);
	if (bCritical)
	{
		if (UDamageNumberWidget* NumberWidget = Cast<UDamageNumberWidget>(UserWidget))
		{
			NumberWidget->SetHeat(Frame.Heat);
		}
	}
}
