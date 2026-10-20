#include "UI/World/EmberMarkActor.h"
#include "Creatures/CreatureBase.h"
#include "UI/World/EmberMarkWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The pop as it's marked. */
	constexpr float PopSeconds = 0.35f;
	constexpr float PopPeak = 1.35f;
	/** A flare's swell and how long it takes to settle. */
	constexpr float FlareSeconds = 0.4f;
	constexpr float FlarePeak = 1.3f;
	/** Whole size up to here (cm); at FarAt, this much of it (it never goes: a mark is meant to be seen far off). */
	constexpr float FullSizeUntil = 1000.f;
	constexpr float FarAt = 6000.f;
	constexpr float FarScale = 0.7f;
	/** The glow's breath. */
	constexpr float GlowRate = 3.2f;
	/** Over the top of its body and its tag (cm at size 1). */
	constexpr float OverHead = 62.f;

	float EaseOut(float Alpha)
	{
		const float Left = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Left * Left * Left;
	}
}

AEmberMarkActor::AEmberMarkActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetCanBeDamaged(false);

	Widget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	Widget->SetWidgetSpace(EWidgetSpace::Screen);
	Widget->SetDrawAtDesiredSize(true);
	// Bottom-middle on the spot over the creature: the glyph stands over it, the word at its foot.
	Widget->SetPivot(FVector2D(0.5f, 1.f));
	Widget->SetWidgetClass(UEmberMarkWidget::StaticClass());
	Widget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Widget->SetGenerateOverlapEvents(false);
	SetRootComponent(Widget);

	Phase = FMath::FRandRange(0.f, UE_TWO_PI);
}

AEmberMarkActor* AEmberMarkActor::Mark(AActor* InTarget, const FLinearColor& Color, FName IconName)
{
	UWorld* World = InTarget ? InTarget->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEmberMarkActor* Made = World->SpawnActor<AEmberMarkActor>(AEmberMarkActor::StaticClass(), MarkPoint(*InTarget), FRotator::ZeroRotator, Params);
	if (!Made)
	{
		return nullptr;
	}
	Made->Target = InTarget;
	Made->Tint = Color;
	Made->Icon = IconName;
	if (!Made->Widget->GetUserWidgetObject())
	{
		Made->Widget->InitWidget();
	}
	if (UEmberMarkWidget* Look = Cast<UEmberMarkWidget>(Made->Widget->GetUserWidgetObject()))
	{
		Look->SetGlyph(IconName, Color);
	}
	Made->Advance(0.f);
	return Made;
}

FVector AEmberMarkActor::MarkPoint(const AActor& InTarget)
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(&InTarget);
	const float Size = Creature ? FMath::Max(Creature->GetSizeScale(), 0.4f) : 1.f;
	const ACharacter* Character = Cast<ACharacter>(&InTarget);
	const float HalfHeight = Character && Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 60.f;
	return InTarget.GetActorLocation() + FVector(0.0, 0.0, HalfHeight + OverHead * Size);
}

float AEmberMarkActor::PopAt(float InAge)
{
	const float Alpha = InAge / PopSeconds;
	if (Alpha >= 1.f)
	{
		return 1.f;
	}
	// Up from nothing past its size, then settling: a mark landing on it, not just appearing.
	constexpr float Rise = 0.55f;
	return Alpha < Rise ? FMath::Lerp(0.f, PopPeak, EaseOut(Alpha / Rise)) : FMath::Lerp(PopPeak, 1.f, EaseOut((Alpha - Rise) / (1.f - Rise)));
}

FString AEmberMarkActor::DistanceWords(float Centimetres)
{
	return FString::Printf(TEXT("%d m"), FMath::Max(FMath::RoundToInt(Centimetres / 100.f), 0));
}

void AEmberMarkActor::Flare()
{
	FlareAge = 0.f;
}

void AEmberMarkActor::Retire()
{
	bRetired = true;
}

void AEmberMarkActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

FString AEmberMarkActor::WordsNow() const
{
	if (Countdown >= 0.f)
	{
		return FString::FromInt(FMath::Max(FMath::CeilToInt(Countdown), 1));
	}
	if (!bShowsDistance)
	{
		return FString();
	}
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	const AActor* Marked = Target.Get();
	return Player && Marked ? DistanceWords(static_cast<float>(FVector::Dist(Player->GetActorLocation(), Marked->GetActorLocation()))) : FString();
}

FString AEmberMarkActor::GetWords() const
{
	return WordsNow();
}

void AEmberMarkActor::Advance(float DeltaSeconds)
{
	Age += DeltaSeconds;
	FlareAge += DeltaSeconds;
	const AActor* Marked = Target.Get();
	if (!Marked || Marked->IsActorBeingDestroyed())
	{
		// Its creature is gone (a runtime spawn sunk away): nothing to mark.
		bRetired = true;
	}
	Presence = bRetired ? FMath::Max(Presence - DeltaSeconds / FadeSeconds, 0.f) : 1.f;
	if (bRetired && Presence <= 0.f)
	{
		SetActorTickEnabled(false);
		Destroy();
		return;
	}
	if (Marked)
	{
		SetActorLocation(MarkPoint(*Marked));
	}

	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	const float Distance = Player ? static_cast<float>(FVector::Dist(Player->GetActorLocation(), GetActorLocation())) : 0.f;
	const float Far = FMath::Clamp((Distance - FullSizeUntil) / (FarAt - FullSizeUntil), 0.f, 1.f);
	const float Flaring = FlareAge < FlareSeconds ? FMath::Lerp(FlarePeak, 1.f, EaseOut(FlareAge / FlareSeconds)) : 1.f;
	const float Scale = FMath::Lerp(1.f, FarScale, Far) * PopAt(Age) * Flaring;
	const float Time = World ? static_cast<float>(World->GetTimeSeconds()) : Age;
	const float Glow = FlareAge < FlareSeconds ? 1.f : 0.5f + 0.5f * FMath::Sin(Time * GlowRate + Phase);

	if (UEmberMarkWidget* Look = Cast<UEmberMarkWidget>(Widget->GetUserWidgetObject()))
	{
		Look->SetWords(WordsNow());
		Look->SetLook(Presence, Scale, Glow);
	}
}
