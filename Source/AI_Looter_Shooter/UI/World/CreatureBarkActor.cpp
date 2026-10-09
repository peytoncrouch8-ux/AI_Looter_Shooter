#include "UI/World/CreatureBarkActor.h"
#include "UI/World/CreatureBarkWidget.h"
#include "Components/WidgetComponent.h"

namespace
{
	/** The component creatures float their tag on (ACreatureBase's HealthBar): the words hang there, just over the tag. */
	const FName TagComponentName(TEXT("HealthBar"));
}

ACreatureBarkActor::ACreatureBarkActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	// Bottom-centred on the anchor, as the tag is, so the words stand on top of it.
	Widget->SetPivot(FVector2D(0.5f, 1.f));
	Widget->SetWidgetClass(UCreatureBarkWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	Widget->SetVisibility(false);
	SetRootComponent(Widget);
}

float ACreatureBarkActor::AlphaAt(float InAge, float InSeconds)
{
	if (InAge < 0.f || InAge >= InSeconds)
	{
		return 0.f;
	}
	const float In = FMath::Clamp(InAge / FadeInSeconds, 0.f, 1.f);
	const float Out = FMath::Clamp((InSeconds - InAge) / FadeOutSeconds, 0.f, 1.f);
	return FMath::Min(In, Out);
}

void ACreatureBarkActor::Show(AActor* InSpeaker, const FText& InLine, float InSeconds, bool bInMuttered)
{
	Speaker = InSpeaker;
	Anchor.Reset();
	if (InSpeaker)
	{
		for (UActorComponent* Component : InSpeaker->GetComponents())
		{
			if (Component && Component->GetFName() == TagComponentName && Component->IsA<USceneComponent>())
			{
				Anchor = Cast<USceneComponent>(Component);
				break;
			}
		}
		LastSpot = InSpeaker->GetActorLocation() + FVector(0.0, 0.0, HeadRoom * InSpeaker->GetActorScale3D().Z);
	}
	Line = InLine;
	Seconds = FMath::Max(InSeconds, 0.2f);
	bMuttered = bInMuttered;
	Age = 0.f;
	bShowing = true;
	if (UCreatureBarkWidget* Words = Cast<UCreatureBarkWidget>(Widget->GetUserWidgetObject()))
	{
		Words->SetLine(Line, bMuttered);
	}
	else if (!Widget->GetUserWidgetObject())
	{
		// Made on first show; the words go on once it exists.
		Widget->InitWidget();
		if (UCreatureBarkWidget* Made = Cast<UCreatureBarkWidget>(Widget->GetUserWidgetObject()))
		{
			Made->SetLine(Line, bMuttered);
		}
	}
	SetActorLocation(AnchorNow());
	Widget->SetVisibility(true);
	SetActorTickEnabled(true);
	Advance(0.f);
}

void ACreatureBarkActor::Hide()
{
	bShowing = false;
	Speaker.Reset();
	Anchor.Reset();
	Widget->SetVisibility(false);
	SetActorTickEnabled(false);
}

void ACreatureBarkActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ACreatureBarkActor::Advance(float DeltaSeconds)
{
	if (!bShowing)
	{
		return;
	}
	Age += DeltaSeconds;
	if (Age >= Seconds)
	{
		Hide();
		return;
	}
	// Kept as it goes, so words whose speaker is gone mid-line stay where it was last.
	LastSpot = AnchorNow();
	SetActorLocation(LastSpot);
	if (UUserWidget* Words = Widget->GetUserWidgetObject())
	{
		Words->SetRenderOpacity(AlphaAt(Age, Seconds));
	}
}

FVector ACreatureBarkActor::AnchorNow() const
{
	if (const USceneComponent* Tag = Anchor.Get())
	{
		return Tag->GetComponentLocation();
	}
	if (const AActor* Body = Speaker.Get())
	{
		return Body->GetActorLocation() + FVector(0.0, 0.0, HeadRoom * Body->GetActorScale3D().Z);
	}
	return LastSpot;
}
