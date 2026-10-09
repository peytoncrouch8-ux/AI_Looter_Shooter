// USettingsMenuWidget's Audio section: the Master, Effects, Interface and Music volume sliders (UAudioSettingsSubsystem).

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace SettingsMenu;

namespace
{
	/** The sliders, top to bottom, in EAudioVolume's order (VolumeSliders and VolumeValues follow it). */
	constexpr EAudioVolume Volumes[] = { EAudioVolume::Master, EAudioVolume::Effects, EAudioVolume::Interface, EAudioVolume::Music };

	// "80%": a volume as its slider shows it is SettingsMenu::PercentText (SettingsMenuParts.h), shared with the camera
	// shake's slider.
}

UWidget* USettingsMenuWidget::MakeAudioRows()
{
	UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	VolumeSliders.Reset();
	VolumeValues.Reset();
	for (const EAudioVolume Which : Volumes)
	{
		USlider* Slider = nullptr;
		UTextBlock* Value = nullptr;
		Rows->AddChildToVerticalBox(MakeSliderRow(UAudioSettingsSubsystem::VolumeName(Which), 0.f, 1.f, Slider, Value,
			UAudioSettingsSubsystem::VolumeStep))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		Slider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleVolumeReleased);
		VolumeSliders.Add(Slider);
		VolumeValues.Add(Value);
	}
	VolumeSliders[static_cast<int32>(EAudioVolume::Master)]->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleMasterVolumeChanged);
	VolumeSliders[static_cast<int32>(EAudioVolume::Effects)]->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleEffectsVolumeChanged);
	VolumeSliders[static_cast<int32>(EAudioVolume::Interface)]->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleInterfaceVolumeChanged);
	VolumeSliders[static_cast<int32>(EAudioVolume::Music)]->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleMusicVolumeChanged);
	return Rows;
}

void USettingsMenuWidget::RefreshAudio()
{
	const UAudioSettingsSubsystem* Audio = GetAudio();
	if (!Audio)
	{
		return;
	}
	for (const EAudioVolume Which : Volumes)
	{
		const int32 Index = static_cast<int32>(Which);
		if (VolumeSliders.IsValidIndex(Index) && VolumeValues.IsValidIndex(Index))
		{
			const float Share = Audio->GetVolume(Which);
			VolumeSliders[Index]->SetValue(Share);
			VolumeValues[Index]->SetText(FText::FromString(PercentText(Share)));
		}
	}
}

void USettingsMenuWidget::ChangeVolume(EAudioVolume Which, float Value)
{
	// Heard at once (under the pause menu too: the menus' own sounds go on over a paused game).
	LastVolumeMoved = Which;
	const float Share = UAudioSettingsSubsystem::ClampVolume(Value);
	if (UAudioSettingsSubsystem* Audio = GetAudio())
	{
		Audio->SetVolume(Which, Share, /*bSave*/ false);
	}
	const int32 Index = static_cast<int32>(Which);
	if (VolumeValues.IsValidIndex(Index))
	{
		VolumeValues[Index]->SetText(FText::FromString(PercentText(Share)));
	}
	// A soft tick at each step, at the new level: the slider is heard as it's moved.
	LooterSound::Play2D(this, LooterSoundCue::Hover, 0.6f);
}

void USettingsMenuWidget::HandleMasterVolumeChanged(float Value)
{
	ChangeVolume(EAudioVolume::Master, Value);
}

void USettingsMenuWidget::HandleEffectsVolumeChanged(float Value)
{
	ChangeVolume(EAudioVolume::Effects, Value);
}

void USettingsMenuWidget::HandleInterfaceVolumeChanged(float Value)
{
	ChangeVolume(EAudioVolume::Interface, Value);
}

void USettingsMenuWidget::HandleMusicVolumeChanged(float Value)
{
	ChangeVolume(EAudioVolume::Music, Value);
}

void USettingsMenuWidget::HandleVolumeReleased()
{
	if (UAudioSettingsSubsystem* Audio = GetAudio())
	{
		Audio->SaveSettings();
		SetStatus(FString::Printf(TEXT("%s volume %s."), *UAudioSettingsSubsystem::VolumeName(LastVolumeMoved),
			*PercentText(Audio->GetVolume(LastVolumeMoved))), LooterUI::Color::TextDim());
	}
	// A click at the level just set (the menus' sounds follow Master and Interface).
	LooterSound::Play2D(this, LooterSoundCue::Click);
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}
