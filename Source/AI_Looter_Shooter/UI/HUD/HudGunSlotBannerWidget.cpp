// UHudGunSlotBannerWidget: "NEW GUN SLOT". When it shows and how it moves are here; its pictures and pieces are
// HudGunSlotBannerWidgetLayout.cpp.

#include "UI/HUD/HudGunSlotBannerWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Inventory/GunSlotRules.h"
#include "Scenes/SceneSubsystem.h"
#include "UI/HUD/HudFindBannerWidget.h"
#include "UI/HUD/HudGraveMarkedWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** The show, in seconds: as the level-up's, the slot opening where the level's number lands. */
	constexpr float FadeInEnd = 0.15f;
	constexpr float WipeStart = 0.02f;
	constexpr float WipeEnd = 0.38f;
	constexpr float ArrowStart = 0.06f;
	constexpr float ArrowEnd = 0.22f;
	constexpr float OpenAt = 0.42f;
	/** The padlock pops off (up and out, fading), the ring flashes out, the number pops in. */
	constexpr float LockOffSeconds = 0.28f;
	constexpr float LockOffScale = 1.7f;
	constexpr float LockOffRise = 14.f;
	constexpr float RingSeconds = 0.7f;
	constexpr float RingGrow = 0.55f;
	constexpr float PopSeconds = 0.26f;
	constexpr float PopScale = 1.25f;
	constexpr float GlowUp = 0.08f;
	constexpr float GlowDown = 0.6f;
	constexpr float BarStart = 0.62f;
	constexpr float BarEnd = 0.86f;
	constexpr float RewardStart = 0.7f;
	constexpr float RewardEnd = 0.92f;
	constexpr float CauseStart = 0.82f;
	constexpr float CauseEnd = 1.04f;
	constexpr float OutStart = UHudGunSlotBannerWidget::ShowSeconds - 0.4f;
	/** Words rise this far as they fade in; the shut slot's gunmetal stands this dim until it opens. */
	constexpr float WordRise = 8.f;
	constexpr float ShutMetal = 0.45f;

	/**
	 * The moment's own cue, named here until its sounds are made (then it joins Audio/LooterSoundCues.h and cues.json together,
	 * as every cue does: Looter.Audio.BankCoversCues wants each header cue in the bank): a heavy steel catch thrown open and a
	 * rack's latch dropping under a rising soul-gold shimmer that settles on one warm struck chord, about 1.6 s, its chord on the
	 * padlock's pop (0.42 s in). Until then a milestone's sting (UI.NotchMilestone) plays in its place.
	 */
	const FName GunSlotCue(TEXT("UI.GunSlotUnlocked"));
	const FName FallbackCue(LooterSoundCue::NotchMilestone);

	float Span(float Now, float From, float To)
	{
		return FMath::Clamp((Now - From) / (To - From), 0.f, 1.f);
	}

	float EaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, Alpha, 3.f);
	}
}

// ---------------------------------------------------------------------------
// What it announces, and taking turns with the other banners
// ---------------------------------------------------------------------------

void UHudGunSlotBannerWidget::SetOtherBanners(UHudLevelUpBannerWidget* InLevelUp, UHudMissionCompleteWidget* InMission,
	UHudGraveMarkedWidget* InGrave, UHudFindBannerWidget* InFind)
{
	LevelUpBanner = InLevelUp;
	MissionBanner = InMission;
	GraveBanner = InGrave;
	FindBanner = InFind;
}

void UHudGunSlotBannerWidget::Announce(int32 SlotIndex, EGunSlotLock Was)
{
	FShow Show;
	Show.Slot = SlotIndex;
	Show.Was = Was;
	Queue.Add(Show);
}

bool UHudGunSlotBannerWidget::IsLevelUpShowing() const
{
	// The level-up banner collapses its root while it waits for a level-up, so a root on screen is a banner showing.
	const UHudLevelUpBannerWidget* LevelUp = LevelUpBanner.Get();
	const UWidget* Root = LevelUp ? LevelUp->GetRootWidget() : nullptr;
	return Root && Root->GetVisibility() != ESlateVisibility::Collapsed;
}

bool UHudGunSlotBannerWidget::IsOtherBannerShowing() const
{
	if (IsLevelUpShowing())
	{
		return true;
	}
	// The mission-complete banner with one waiting too: a turn-in's fanfare goes first (it waits only while this shows).
	const UHudMissionCompleteWidget* Mission = MissionBanner.Get();
	if (Mission && Mission->IsBusy())
	{
		return true;
	}
	const UHudGraveMarkedWidget* Grave = GraveBanner.Get();
	const UHudFindBannerWidget* Find = FindBanner.Get();
	return (Grave && Grave->IsShowing()) || (Find && Find->IsShowing());
}

bool UHudGunSlotBannerWidget::IsCovered() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	if (LooterHUD && LooterHUD->IsMenuOpen())
	{
		return true;
	}
	if (USceneSubsystem::HidesGameplayHUD(this))
	{
		return true;
	}
	const UWorld* World = GetWorld();
	return World && World->IsPaused();
}

bool UHudGunSlotBannerWidget::MayStart(FShow& Next, bool bOthers, float DeltaSeconds)
{
	Next.Waited += DeltaSeconds;
	// A slot the level opened follows its LEVEL UP, which waits for the experience bar to get there: until that banner has
	// shown (or long enough that it won't), this waits.
	if (Next.Was == EGunSlotLock::Level && !Next.bLevelUpSeen)
	{
		Next.bLevelUpSeen = IsLevelUpShowing();
		if (!Next.bLevelUpSeen && Next.Waited < LevelUpWaitSeconds)
		{
			return false;
		}
	}
	// A gap between two banners (a turn-in's deferred level-up) isn't room for this one: the others quiet a moment first.
	Calm = bOthers ? 0.f : Calm + DeltaSeconds;
	return !bOthers && Calm >= CalmSeconds;
}

void UHudGunSlotBannerWidget::StartShow(const FShow& Show)
{
	ShownSlot = Show.Slot;
	Time = 0.f;
	if (SlotNumber)
	{
		SlotNumber->SetText(FText::AsNumber(Show.Slot + 1));
	}
	if (RewardText)
	{
		RewardText->SetText(GunSlotRules::CarryText(Show.Slot).ToUpper());
	}
	if (CauseText)
	{
		CauseText->SetText(GunSlotRules::OpenedByText(Show.Was).ToUpper());
	}
	// Its own sound once made (Art/Sounds), a milestone's sting until then.
	const ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(this);
	LooterSound::Play2D(this, Sounds && Sounds->HasSounds(GunSlotCue) ? GunSlotCue : FallbackCue);
}

void UHudGunSlotBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Advance(InDeltaTime);
}

void UHudGunSlotBannerWidget::Advance(float DeltaSeconds)
{
	const bool bCovered = IsCovered();
	// A show starts at its first moment: the time that passed while it waited (a long frame, a hitch, a test's big step) is
	// the wait's, never the show's, so the whole show is always seen (it once ended in the frame it began).
	bool bStarted = false;
	if (Time < 0.f && !Queue.IsEmpty() && !bCovered && MayStart(Queue[0], IsOtherBannerShowing(), DeltaSeconds))
	{
		const FShow Next = Queue[0];
		Queue.RemoveAt(0);
		Calm = 0.f;
		StartShow(Next);
		bStarted = true;
	}
	// Time under a menu or a scene doesn't count, so the whole show is seen once the game is back.
	if (Time >= 0.f && !bCovered && !bStarted)
	{
		Time += DeltaSeconds;
		if (Time >= ShowSeconds)
		{
			Time = -1.f;
		}
	}
	PaintShow(bCovered);
}

FString UHudGunSlotBannerWidget::GetShownReward() const
{
	return Time >= 0.f && RewardText ? RewardText->GetText().ToString() : FString();
}

bool UHudGunSlotBannerWidget::IsOpenedInShow() const
{
	return Time >= OpenAt;
}

// ---------------------------------------------------------------------------
// The show
// ---------------------------------------------------------------------------

void UHudGunSlotBannerWidget::PaintShow(bool bCovered)
{
	if (!Banner)
	{
		return;
	}
	const bool bShowing = Time >= 0.f && !bCovered;
	const ESlateVisibility Wanted = bShowing ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (Banner->GetVisibility() != Wanted)
	{
		Banner->SetVisibility(Wanted);
	}
	if (!bShowing)
	{
		return;
	}
	const float Now = Time;
	// A render scale of exactly zero can't be inverted; the piece is see-through until it starts anyway.
	auto Grow = [](UWidget* Widget, float Alpha)
	{
		Widget->SetRenderScale(FVector2D(FMath::Max(Alpha, 0.001f), 1.f));
		Widget->SetRenderOpacity(Alpha > 0.f ? 1.f : 0.f);
	};
	auto Rise = [](UWidget* Widget, float Alpha)
	{
		Widget->SetRenderOpacity(Alpha);
		Widget->SetRenderTranslation(FVector2D(0.f, WordRise * (1.f - Alpha)));
	};

	const float Out = 1.f - FMath::SmoothStep(0.f, 1.f, Span(Now, OutStart, ShowSeconds));
	Banner->SetRenderOpacity(EaseOut(Span(Now, 0.f, FadeInEnd)) * Out);
	Rise(TitleText, EaseOut(Span(Now, 0.f, FadeInEnd + 0.07f)));
	const float Wipe = EaseOut(Span(Now, WipeStart, WipeEnd));
	Grow(LeftLine, Wipe);
	Grow(RightLine, Wipe);
	const float Arrows = EaseOut(Span(Now, ArrowStart, ArrowEnd));
	LeftArrow->SetRenderOpacity(Arrows);
	RightArrow->SetRenderOpacity(Arrows);

	// The slot: shut and dim, then at OpenAt the padlock pops up and off, a soul-gold ring flashes out, the gunmetal lights up
	// and the slot's number pops in.
	const bool bOpen = Now >= OpenAt;
	const float LockOff = EaseOut(Span(Now, OpenAt, OpenAt + LockOffSeconds));
	SlotLock->SetRenderOpacity(1.f - LockOff);
	SlotLock->SetRenderScale(FVector2D(FMath::Lerp(1.f, LockOffScale, LockOff)));
	SlotLock->SetRenderTranslation(FVector2D(0.f, -LockOffRise * LockOff));
	SlotMetal->SetRenderOpacity(bOpen ? FMath::Lerp(ShutMetal, 1.f, EaseOut(Span(Now, OpenAt, OpenAt + 0.2f))) : ShutMetal);
	SlotHairline->SetRenderOpacity(bOpen ? EaseOut(Span(Now, OpenAt, OpenAt + 0.3f)) : 0.f);
	const float RingAlpha = Span(Now, OpenAt, OpenAt + RingSeconds);
	SlotRing->SetRenderOpacity(bOpen ? FMath::Square(1.f - RingAlpha) : 0.f);
	SlotRing->SetRenderScale(FVector2D((UHudWeaponSlotsWidget::SlotDiameter + UHudWeaponSlotsWidget::AccentMargin * 2.f)
		/ UHudWeaponSlotsWidget::SlotDiameter * (1.f + RingGrow * EaseOut(RingAlpha))));
	SlotNumber->SetRenderOpacity(bOpen ? 1.f : 0.f);
	SlotNumber->SetRenderScale(FVector2D(bOpen ? FMath::Lerp(PopScale, 1.f, EaseOut(Span(Now, OpenAt, OpenAt + PopSeconds))) : 1.f));
	const float GlowIn = Span(Now, OpenAt, OpenAt + GlowUp);
	const float GlowOut = FMath::SmoothStep(0.f, 1.f, Span(Now, OpenAt + GlowUp, OpenAt + GlowUp + GlowDown));
	SlotGlow->SetRenderOpacity(GlowIn * (1.f - GlowOut));

	Grow(GoldBar, EaseOut(Span(Now, BarStart, BarEnd)));
	Rise(RewardText, EaseOut(Span(Now, RewardStart, RewardEnd)));
	Rise(CauseText, EaseOut(Span(Now, CauseStart, CauseEnd)));
}
