#include "UI/DamageNumberActor.h"
#include "UI/DamageNumberWidget.h"
#include "Components/WidgetComponent.h"

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

	const FVector2D Drift = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * MaxDriftSpeed;
	Velocity = FVector(Drift.X, Drift.Y, RiseSpeed * (bCritical ? 1.3f : 1.f));

	if (UDamageNumberWidget* NumberWidget = Cast<UDamageNumberWidget>(Widget->GetUserWidgetObject()))
	{
		NumberWidget->SetDamage(Damage, bCritical);
	}

	SetLifeSpan(Lifetime + 0.1f);
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

	const float Alpha = FMath::Clamp(Age / FMath::Max(Lifetime, 0.01f), 0.f, 1.f);

	// Fade over the last 40% of the lifetime.
	UserWidget->SetRenderOpacity(Alpha < 0.6f ? 1.f : 1.f - (Alpha - 0.6f) / 0.4f);

	// Crits "pop": start oversized and settle.
	const float PopScale = bCritical ? FMath::Lerp(1.6f, 1.f, FMath::Clamp(Age / 0.15f, 0.f, 1.f)) : 1.f;
	UserWidget->SetRenderScale(FVector2D(PopScale));
}
