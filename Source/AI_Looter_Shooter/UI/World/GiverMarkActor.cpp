#include "UI/World/GiverMarkActor.h"
#include "UI/World/GiverMarkSubsystem.h"
#include "UI/World/GiverMarkWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Whole up to here (cm), fading out to the subsystem's range. */
	constexpr float FullUntil = 3400.f;
	/** Whole size up to here (cm); at the range's end, this much of it. */
	constexpr float FullSizeUntil = 800.f;
	constexpr float FarScale = 0.7f;
	/** The bob: a few pixels, slow. */
	constexpr float BobPixels = 3.f;
	constexpr float BobRate = 2.2f;
	constexpr float GlowRate = 2.6f;
	/** The pop as it appears. */
	constexpr float PopSeconds = 0.4f;
	constexpr float PopPeak = 1.22f;

	float EaseOut(float Alpha)
	{
		const float Left = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Left * Left * Left;
	}
}

AGiverMarkActor::AGiverMarkActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	// Bottom-middle on the spot over the giver's head: the glyph stands on it.
	Widget->SetPivot(FVector2D(0.5f, 1.f));
	Widget->SetWidgetClass(UGiverMarkWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	Widget->SetVisibility(false);
	SetRootComponent(Widget);

	Phase = FMath::FRandRange(0.f, UE_TWO_PI);
}

float AGiverMarkActor::AlphaAt(float Distance)
{
	return 1.f - FMath::SmoothStep(FullUntil, UGiverMarkSubsystem::ShowRange, Distance);
}

float AGiverMarkActor::ScaleAt(float Distance)
{
	const float Far = FMath::Clamp((Distance - FullSizeUntil) / (UGiverMarkSubsystem::ShowRange - FullSizeUntil), 0.f, 1.f);
	return FMath::Lerp(1.f, FarScale, Far);
}

float AGiverMarkActor::PopAt(float InAge)
{
	const float Alpha = InAge / PopSeconds;
	if (Alpha >= 1.f)
	{
		return 1.f;
	}
	// Up from nothing past its size, then settling back: a mark arriving, not just appearing.
	constexpr float Rise = 0.6f;
	return Alpha < Rise ? FMath::Lerp(0.f, PopPeak, EaseOut(Alpha / Rise)) : FMath::Lerp(PopPeak, 1.f, EaseOut((Alpha - Rise) / (1.f - Rise)));
}

void AGiverMarkActor::Show(const FGiverMark& InMark)
{
	AActor* NewGiver = InMark.Giver.Get();
	if (!NewGiver)
	{
		Retire();
		return;
	}
	const bool bNewGiver = NewGiver != Giver.Get() || IsFree();
	if (bNewGiver || InMark.Mark != Mark || InMark.bMain != bMain)
	{
		Age = 0.f;
	}
	if (bNewGiver)
	{
		Presence = 0.f;
	}
	Giver = NewGiver;
	Offset = InMark.Location - NewGiver->GetActorLocation();
	Mark = InMark.Mark;
	bMain = InMark.bMain;
	bRetired = false;

	if (!Widget->GetUserWidgetObject())
	{
		// Made on first show; its mark goes on once it exists.
		Widget->InitWidget();
	}
	if (UGiverMarkWidget* Look = Cast<UGiverMarkWidget>(Widget->GetUserWidgetObject()))
	{
		Look->SetMark(Mark, bMain);
	}
	SetActorLocation(InMark.Location);
	SetActorTickEnabled(true);
	Advance(0.f);
}

void AGiverMarkActor::Retire()
{
	bRetired = true;
}

void AGiverMarkActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AGiverMarkActor::Advance(float DeltaSeconds)
{
	Age += DeltaSeconds;
	Presence = FMath::FInterpConstantTo(Presence, bRetired ? 0.f : 1.f, DeltaSeconds, 1.f / FadeSeconds);
	const AActor* GiverActor = Giver.Get();
	if (GiverActor && !bRetired)
	{
		SetActorLocation(GiverActor->GetActorLocation() + Offset);
	}
	if (bRetired && Presence <= 0.f)
	{
		// Gone: hidden and free until another giver needs it.
		Giver.Reset();
		Widget->SetVisibility(false);
		SetActorTickEnabled(false);
		return;
	}

	// Eased with the player's distance: whole near, smaller and fainter far off, gone past the range.
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	const float Distance = Player ? static_cast<float>(FVector::Dist(Player->GetActorLocation(), GetActorLocation())) : 0.f;
	// The giver it hangs over (the actor's own Owner is never set: checking that hid every mark, play-test 2026-10-22).
	const AActor* Shown = Giver.Get();
	const bool bGiverShown = Shown && !Shown->IsHidden();
	const float Alpha = bGiverShown ? AlphaAt(Distance) * Presence : 0.f;
	const float Time = World ? static_cast<float>(World->GetTimeSeconds()) : 0.f;
	const float Scale = ScaleAt(Distance) * (bRetired ? 1.f : PopAt(Age));

	Widget->SetVisibility(Alpha > 0.01f);
	if (UGiverMarkWidget* Look = Cast<UGiverMarkWidget>(Widget->GetUserWidgetObject()))
	{
		Look->SetLook(Alpha, Scale, BobPixels * FMath::Sin(Time * BobRate + Phase), 0.5f + 0.5f * FMath::Sin(Time * GlowRate + Phase));
	}
}
