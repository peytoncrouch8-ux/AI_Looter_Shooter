#include "UI/Menus/MenuNav.h"

MenuNav::EMove MenuNav::MoveOf(const FKey& Key)
{
	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
	{
		return EMove::Up;
	}
	if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
	{
		return EMove::Down;
	}
	if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left)
	{
		return EMove::Left;
	}
	if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right)
	{
		return EMove::Right;
	}
	return EMove::None;
}

int32 MenuNav::VerticalStep(EMove Move)
{
	return Move == EMove::Up ? -1 : (Move == EMove::Down ? 1 : 0);
}

int32 MenuNav::HorizontalStep(EMove Move)
{
	return Move == EMove::Left ? -1 : (Move == EMove::Right ? 1 : 0);
}

bool MenuNav::IsAccept(const FKey& Key)
{
	return Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Bottom;
}

bool MenuNav::IsBack(const FKey& Key)
{
	return Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
}

bool MenuNav::IsMenuButton(const FKey& Key)
{
	return Key == EKeys::Gamepad_Special_Right;
}

int32 MenuNav::SectionStepOf(const FKey& Key)
{
	if (Key == EKeys::PageUp || Key == EKeys::Gamepad_LeftShoulder)
	{
		return -1;
	}
	if (Key == EKeys::PageDown || Key == EKeys::Gamepad_RightShoulder)
	{
		return 1;
	}
	return 0;
}

bool MenuNav::IsStickKey(const FKey& Key)
{
	return Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Gamepad_LeftStick_Down
		|| Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Gamepad_LeftStick_Right;
}

MenuNav::EMove MenuNav::StickMove(const FVector2D& Stick, EMove Held)
{
	// Held: kept while the stick stays past the let-go line that way, even as it drifts across the other axis.
	const double Along = Held == EMove::Up ? Stick.Y : Held == EMove::Down ? -Stick.Y : Held == EMove::Right ? Stick.X : Held == EMove::Left ? -Stick.X : 0.0;
	if (Held != EMove::None && Along >= StickRelease)
	{
		return Held;
	}
	const double X = FMath::Abs(Stick.X);
	const double Y = FMath::Abs(Stick.Y);
	if (FMath::Max(X, Y) < StickPress)
	{
		return EMove::None;
	}
	if (X > Y)
	{
		return Stick.X > 0.0 ? EMove::Right : EMove::Left;
	}
	return Stick.Y > 0.0 ? EMove::Up : EMove::Down;
}

FKey MenuNav::StickKey(EMove Move)
{
	switch (Move)
	{
	case EMove::Up:    return EKeys::Gamepad_LeftStick_Up;
	case EMove::Down:  return EKeys::Gamepad_LeftStick_Down;
	case EMove::Left:  return EKeys::Gamepad_LeftStick_Left;
	case EMove::Right: return EKeys::Gamepad_LeftStick_Right;
	default:           return EKeys::Invalid;
	}
}

// ---------------------------------------------------------------------------
// Held keys
// ---------------------------------------------------------------------------

void MenuNav::FRepeat::Press(const FKey& InKey, EMove InMove, bool bInAccelerate)
{
	Key = InKey;
	Move = InMove;
	bAccelerate = bInAccelerate;
	Held = 0.f;
	NextAt = RepeatDelay;
}

bool MenuNav::FRepeat::Release(const FKey& InKey)
{
	if (!IsHeld() || InKey != Key)
	{
		return false;
	}
	Stop();
	return true;
}

void MenuNav::FRepeat::Stop()
{
	Key = EKeys::Invalid;
	Move = EMove::None;
	Held = 0.f;
	NextAt = 0.f;
}

int32 MenuNav::FRepeat::Tick(float DeltaSeconds)
{
	if (!IsHeld())
	{
		return 0;
	}
	// A hitch never fires a burst: a few repeats at most, and the clock catches up past the rest.
	constexpr int32 MaxPerFrame = 4;
	Held += FMath::Max(DeltaSeconds, 0.f);
	int32 Due = 0;
	while (Held >= NextAt && Due < MaxPerFrame)
	{
		++Due;
		NextAt += IntervalAt(NextAt, bAccelerate);
	}
	if (Held >= NextAt)
	{
		NextAt = Held + IntervalAt(Held, bAccelerate);
	}
	return Due;
}

float MenuNav::FRepeat::IntervalAt(float HeldSeconds, bool bInAccelerate)
{
	if (!bInAccelerate)
	{
		return RepeatInterval;
	}
	// Easing in: a slider creeps for fine steps at first, then races across its range.
	const float Alpha = FMath::Clamp((HeldSeconds - RepeatDelay) / AccelerateSeconds, 0.f, 1.f);
	return FMath::Lerp(RepeatInterval, FastestInterval, Alpha * Alpha * (3.f - 2.f * Alpha));
}

// ---------------------------------------------------------------------------
// Prompts
// ---------------------------------------------------------------------------

FString MenuNav::PromptKeyName(EPromptKey Key, bool bGamepad)
{
	// Word key names: the kit's font has no arrow glyphs, and words read the same on every pad.
	switch (Key)
	{
	case EPromptKey::Select:   return bGamepad ? TEXT("A") : TEXT("Enter");
	case EPromptKey::Back:     return bGamepad ? TEXT("B") : TEXT("Esc");
	case EPromptKey::Sections: return bGamepad ? TEXT("LB / RB") : TEXT("PgUp / PgDn");
	case EPromptKey::Adjust:   return bGamepad ? TEXT("D-pad") : TEXT("Left / Right");
	case EPromptKey::Move:     return bGamepad ? TEXT("D-pad") : TEXT("Arrows");
	}
	return FString();
}

MenuNav::FPrompt MenuNav::MakePrompt(EPromptKey Key, bool bGamepad, const FString& Text)
{
	return FPrompt{ PromptKeyName(Key, bGamepad), Text };
}

FString MenuNav::PromptsText(const TArray<FPrompt>& Prompts)
{
	TArray<FString> Parts;
	for (const FPrompt& Prompt : Prompts)
	{
		Parts.Add(FString::Printf(TEXT("%s %s"), *Prompt.Key, *Prompt.Text));
	}
	return FString::Join(Parts, TEXT("  "));
}
