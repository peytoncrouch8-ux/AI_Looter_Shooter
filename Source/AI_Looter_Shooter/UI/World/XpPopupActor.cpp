#include "UI/World/XpPopupActor.h"
#include "UI/World/XpPopupWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** A kill's: pops in from this size over this long, rises this far (px) over its life, fades over its last share. */
	constexpr float XpSmallPop = 1.35f;
	constexpr float XpSmallPopSeconds = 0.12f;
	constexpr float XpSmallRise = 46.f;
	constexpr float XpSmallFadeShare = 0.35f;
	/** A turn-in's: slams in big, past its size and back, rises a little, fades over its last share. */
	constexpr float XpBigPop = 1.6f;
	constexpr float XpBigUnder = 0.94f;
	constexpr float XpBigSlamSeconds = 0.14f;
	constexpr float XpBigSettleSeconds = 0.32f;
	constexpr float XpBigRise = 34.f;
	constexpr float XpBigFadeShare = 0.3f;
	/** Both fade in over this long. */
	constexpr float XpFadeIn = 0.06f;

	float XpEaseOut(float Alpha)
	{
		const float Left = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Left * Left;
	}
}

AXpPopupActor::FLook AXpPopupActor::LookAt(float Shown, bool bInBig)
{
	FLook Look;
	const float T = FMath::Max(Shown, 0.f);
	const float Life = bInBig ? BigSeconds : SmallSeconds;
	const float Share = FMath::Clamp(T / Life, 0.f, 1.f);
	const float FadeShare = bInBig ? XpBigFadeShare : XpSmallFadeShare;
	Look.Opacity = FMath::Clamp(T / XpFadeIn, 0.f, 1.f) * FMath::Clamp((1.f - Share) / FadeShare, 0.f, 1.f);
	if (bInBig)
	{
		Look.Scale = T < XpBigSlamSeconds
			? FMath::Lerp(XpBigPop, XpBigUnder, XpEaseOut(T / XpBigSlamSeconds))
			: FMath::Lerp(XpBigUnder, 1.f, XpEaseOut((T - XpBigSlamSeconds) / (XpBigSettleSeconds - XpBigSlamSeconds)));
		Look.Offset = FVector2D(0.f, -XpBigRise * XpEaseOut(Share));
	}
	else
	{
		Look.Scale = FMath::Lerp(XpSmallPop, 1.f, XpEaseOut(T / XpSmallPopSeconds));
		Look.Offset = FVector2D(SmallBeside, -XpSmallRise * XpEaseOut(Share));
	}
	return Look;
}

AXpPopupActor::AXpPopupActor()
{
	// It ticks only while it shows.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	// By its left middle, so a kill's "+XP" starts just right of its spot whatever its length; a turn-in's is centred
	// (its offset is nothing sideways, and its pivot is set as it shows).
	Widget->SetPivot(FVector2D(0.f, 0.5f));
	Widget->SetWidgetClass(UXpPopupWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	Widget->SetVisibility(false);
	SetRootComponent(Widget);
}

void AXpPopupActor::Show(int64 InAmount, bool bInBig, const FVector& Where, float InDelay)
{
	Amount = InAmount;
	bBig = bInBig;
	Words = UXpPopupWidget::Words(Amount);
	Age = 0.f;
	Delay = FMath::Max(InDelay, 0.f);
	bShowing = true;
	bFollowView = false;
	SetActorLocation(Where);
	Widget->SetPivot(bBig ? FVector2D(0.5f, 0.5f) : FVector2D(0.f, 0.5f));
	if (!Widget->GetUserWidgetObject())
	{
		// Made on first show; its words go on once it exists.
		Widget->InitWidget();
	}
	if (UXpPopupWidget* Look = Cast<UXpPopupWidget>(Widget->GetUserWidgetObject()))
	{
		Look->SetAmount(Amount, bBig);
	}
	SetActorTickEnabled(true);
	Advance(0.f);
}

void AXpPopupActor::FollowView(const FVector& InViewOffset)
{
	ViewOffset = InViewOffset;
	bFollowView = true;
	PlaceBeforeView();
}

void AXpPopupActor::PlaceBeforeView()
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* Camera = Controller ? Controller->PlayerCameraManager.Get() : nullptr;
	if (!Camera)
	{
		return;
	}
	const FRotationMatrix View(Camera->GetCameraRotation());
	SetActorLocation(Camera->GetCameraLocation() + View.GetUnitAxis(EAxis::X) * ViewOffset.X + View.GetUnitAxis(EAxis::Y) * ViewOffset.Y
		+ View.GetUnitAxis(EAxis::Z) * ViewOffset.Z);
}

void AXpPopupActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AXpPopupActor::Advance(float DeltaSeconds)
{
	if (!bShowing)
	{
		return;
	}
	Age += FMath::Max(DeltaSeconds, 0.f);
	const float Shown = Age - Delay;
	if (Shown >= (bBig ? BigSeconds : SmallSeconds))
	{
		Hide();
		return;
	}
	if (bFollowView)
	{
		PlaceBeforeView();
	}
	// Waiting out its delay: not on screen yet.
	const bool bVisible = Shown >= 0.f;
	if (Widget->IsVisible() != bVisible)
	{
		Widget->SetVisibility(bVisible);
	}
	UUserWidget* Look = Widget->GetUserWidgetObject();
	if (!bVisible || !Look)
	{
		return;
	}
	const FLook Frame = LookAt(Shown, bBig);
	FWidgetTransform Transform;
	Transform.Translation = Frame.Offset;
	Transform.Scale = FVector2D(Frame.Scale);
	Look->SetRenderTransform(Transform);
	Look->SetRenderOpacity(Frame.Opacity);
}

void AXpPopupActor::Hide()
{
	bShowing = false;
	bFollowView = false;
	Widget->SetVisibility(false);
	SetActorTickEnabled(false);
}
