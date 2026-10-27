// USettingsMenuWidget's Graphics, Interface and Controls pages: their rows, each a keyboard and controller stop with its
// words for the description column (SettingsMenuHelp.h). The Audio page is SettingsMenuAudio.cpp.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

using namespace SettingsMenu;

void USettingsMenuWidget::MakeGraphicsPage()
{
	BeginPage();
	TArray<FString> QualityNames;
	for (const EGraphicsQuality Quality : Qualities)
	{
		QualityNames.Add(UGraphicsSettingsSubsystem::QualityName(Quality));
	}
	TArray<ULooterButton*> Quality;
	UWidget* QualityRow = MakeChoiceRow(TEXT("Quality"), QualityNames, ActionQuality, QualityColumnWidth, Quality);
	AddPageRow(Describe(NavChoices(QualityRow, Quality), RowHelp(ESettingRow::Quality), false, /*bQuality*/ true), 0.f);
	for (ULooterButton* Button : Quality)
	{
		QualityButtons.Add(Button);
	}
	ULooterButton* BlurOn = nullptr;
	ULooterButton* BlurOff = nullptr;
	UWidget* BlurRow = MakeToggleRow(TEXT("Motion blur"), TEXT("On"), TEXT("Off"), ActionMotionBlurOn, ActionMotionBlurOff, 0, false, BlurOn, BlurOff);
	AddPageRow(Describe(NavChoices(BlurRow, { BlurOn, BlurOff }), RowHelp(ESettingRow::MotionBlur)));
	MotionBlurOn = BlurOn;
	MotionBlurOff = BlurOff;
	// The first-person view only: third person keeps its own over-the-shoulder framing.
	USlider* FovSlider = nullptr;
	UTextBlock* FovText = nullptr;
	UWidget* FovRow = MakeSliderRow(TEXT("Field of view (1st person)"), UGraphicsSettingsSubsystem::MinFieldOfView,
		UGraphicsSettingsSubsystem::MaxFieldOfView, FovSlider, FovText, FieldOfViewStep);
	AddPageRow(Describe(NavSlider(FovRow, FovSlider), RowHelp(ESettingRow::FieldOfView)));
	FieldOfViewSlider = FovSlider;
	FieldOfViewValue = FovText;
	FieldOfViewSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewChanged);
	// Taken and let go by the mouse or a controller (a controller's lock on a focused slider), the same to the preview.
	FieldOfViewSlider->OnMouseCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewGrabbed);
	FieldOfViewSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewReleased);
	FieldOfViewSlider->OnControllerCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewGrabbed);
	FieldOfViewSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewReleased);
}

void USettingsMenuWidget::MakeInterfacePage()
{
	BeginPage();
	USlider* Transparency = nullptr;
	UTextBlock* TransparencyText = nullptr;
	UWidget* TransparencyRow = MakeSliderRow(TEXT("UI transparency"), 0.f, 1.f, Transparency, TransparencyText);
	AddPageRow(Describe(NavSlider(TransparencyRow, Transparency), RowHelp(ESettingRow::UITransparency)), 0.f);
	TransparencySlider = Transparency;
	TransparencyValue = TransparencyText;
	TransparencySlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleTransparencyChanged);
	// Let go by the mouse, or by the key or stick that stepped it (the menu's keys hold a slider as a controller does).
	TransparencySlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleTransparencyReleased);
	TransparencySlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleTransparencyReleased);
	ULooterButton* MapOn = nullptr;
	ULooterButton* MapOff = nullptr;
	UWidget* MapRow = MakeToggleRow(TEXT("Minimap"), TEXT("On"), TEXT("Off"), ActionMinimapOn, ActionMinimapOff, 0, false, MapOn, MapOff);
	AddPageRow(Describe(NavChoices(MapRow, { MapOn, MapOff }), RowHelp(ESettingRow::Minimap)));
	MinimapOn = MapOn;
	MinimapOff = MapOff;
	USlider* MinimapSize = nullptr;
	UTextBlock* MinimapText = nullptr;
	UWidget* MinimapSizeRow = MakeSliderRow(TEXT("Minimap size"), UGraphicsSettingsSubsystem::MinMinimapScale,
		UGraphicsSettingsSubsystem::MaxMinimapScale, MinimapSize, MinimapText);
	AddPageRow(Describe(NavSlider(MinimapSizeRow, MinimapSize), RowHelp(ESettingRow::MinimapSize)));
	MinimapSlider = MinimapSize;
	MinimapValue = MinimapText;
	MinimapSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeChanged);
	MinimapSlider->OnMouseCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeGrabbed);
	MinimapSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeReleased);
	MinimapSlider->OnControllerCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeGrabbed);
	MinimapSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeReleased);
	USlider* MinimapZoom = nullptr;
	UTextBlock* MinimapZoomText = nullptr;
	UWidget* MinimapZoomRow = MakeSliderRow(TEXT("Minimap zoom"), UGraphicsSettingsSubsystem::MinMinimapZoom,
		UGraphicsSettingsSubsystem::MaxMinimapZoom, MinimapZoom, MinimapZoomText);
	AddPageRow(Describe(NavSlider(MinimapZoomRow, MinimapZoom), RowHelp(ESettingRow::MinimapZoom)));
	MinimapZoomSlider = MinimapZoom;
	MinimapZoomValue = MinimapZoomText;
	MinimapZoomSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomChanged);
	MinimapZoomSlider->OnMouseCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomGrabbed);
	MinimapZoomSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomReleased);
	MinimapZoomSlider->OnControllerCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomGrabbed);
	MinimapZoomSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomReleased);
	ULooterButton* RateOn = nullptr;
	ULooterButton* RateOff = nullptr;
	UWidget* RateRow = MakeToggleRow(TEXT("FPS counter"), TEXT("On"), TEXT("Off"), ActionFrameRateOn, ActionFrameRateOff, 0, false, RateOn, RateOff);
	AddPageRow(Describe(NavChoices(RateRow, { RateOn, RateOff }), RowHelp(ESettingRow::FrameRate)));
	FrameRateOn = RateOn;
	FrameRateOff = RateOff;
	// A key and what it does, near the mission tracker, until the player has used that control.
	ULooterButton* ShowHints = nullptr;
	ULooterButton* HideHints = nullptr;
	UWidget* HintsRow = MakeToggleRow(TEXT("Control hints"), TEXT("On"), TEXT("Off"), ActionHintsOn, ActionHintsOff, 0, false, ShowHints, HideHints);
	AddPageRow(Describe(NavChoices(HintsRow, { ShowHints, HideHints }), RowHelp(ESettingRow::ControlHints)));
	HintsOn = ShowHints;
	HintsOff = HideHints;
	// The words of every spoken line, the captions low on the screen and a creature's over its head; the lines still play.
	ULooterButton* ShowSubtitles = nullptr;
	ULooterButton* HideSubtitles = nullptr;
	UWidget* SubtitlesRow = MakeToggleRow(TEXT("Subtitles"), TEXT("On"), TEXT("Off"), ActionSubtitlesOn, ActionSubtitlesOff, 0, false, ShowSubtitles, HideSubtitles);
	AddPageRow(Describe(NavChoices(SubtitlesRow, { ShowSubtitles, HideSubtitles }), RowHelp(ESettingRow::Subtitles)));
	SubtitlesOn = ShowSubtitles;
	SubtitlesOff = HideSubtitles;
}

void USettingsMenuWidget::MakeControlsPage()
{
	using namespace LooterUI;
	BeginPage();
	// Mouse and stick alike; a zoomed sight still slows the turn by its zoom on top of it.
	USlider* LookSlider = nullptr;
	UTextBlock* LookValueText = nullptr;
	UWidget* LookRow = MakeSliderRow(TEXT("Look sensitivity"), UControlSettingsSubsystem::MinLookSensitivity,
		UControlSettingsSubsystem::MaxLookSensitivity, LookSlider, LookValueText, UControlSettingsSubsystem::LookSensitivityStep);
	AddPageRow(Describe(NavSlider(LookRow, LookSlider), RowHelp(ESettingRow::LookSensitivity)), 0.f);
	LookSensitivitySlider = LookSlider;
	LookSensitivityValue = LookValueText;
	LookSensitivitySlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleLookSensitivityChanged);
	LookSensitivitySlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleLookSensitivityReleased);
	LookSensitivitySlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleLookSensitivityReleased);
	// Every kick and shake of the view (shots, hits, kills, a boss's slam); 0% keeps it still.
	USlider* ShakeSlider = nullptr;
	UTextBlock* ShakeValueText = nullptr;
	UWidget* ShakeRow = MakeSliderRow(TEXT("Camera shake"), UControlSettingsSubsystem::MinCameraShake,
		UControlSettingsSubsystem::MaxCameraShake, ShakeSlider, ShakeValueText, UControlSettingsSubsystem::CameraShakeStep);
	AddPageRow(Describe(NavSlider(ShakeRow, ShakeSlider), RowHelp(ESettingRow::CameraShake)));
	CameraShakeSlider = ShakeSlider;
	CameraShakeValue = ShakeValueText;
	CameraShakeSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleCameraShakeChanged);
	CameraShakeSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleCameraShakeReleased);
	CameraShakeSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleCameraShakeReleased);
	// How hard a gamepad rumbles for shots and hits taken; 0% turns it off. (Nothing rumbles on a mouse and keys.)
	USlider* RumbleSlider = nullptr;
	UTextBlock* RumbleValueText = nullptr;
	UWidget* RumbleRow = MakeSliderRow(TEXT("Controller rumble"), UControlSettingsSubsystem::MinControllerRumble,
		UControlSettingsSubsystem::MaxControllerRumble, RumbleSlider, RumbleValueText, UControlSettingsSubsystem::ControllerRumbleStep);
	AddPageRow(Describe(NavSlider(RumbleRow, RumbleSlider), RowHelp(ESettingRow::ControllerRumble)));
	ControllerRumbleSlider = RumbleSlider;
	ControllerRumbleValue = RumbleValueText;
	ControllerRumbleSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleControllerRumbleChanged);
	ControllerRumbleSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleControllerRumbleReleased);
	ControllerRumbleSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleControllerRumbleReleased);

	// The key list's heading: how to change a key (RefreshNavLook words it per device), with Reset all keys beside it, a stop
	// of its own above the keys (it resets only them, so it lives with them rather than in the footer).
	AddPageRow(MakeSection(WidgetTree, TEXT("Keyboard and mouse keys")), 16.f);
	UHorizontalBox* KeyHead = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	KeyHintText = MakeText(WidgetTree, TEXT(""), 11, Color::TextDim());
	KeyHintText->SetAutoWrapText(true);
	UHorizontalBoxSlot* HintSlot = KeyHead->AddChildToHorizontalBox(KeyHintText);
	HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HintSlot->SetVerticalAlignment(VAlign_Center);
	ResetAllButton = MakeButton(ActionResetAll, 0, TEXT("Reset all keys"), 12);
	UHorizontalBoxSlot* ResetSlot = KeyHead->AddChildToHorizontalBox(MakeSized(WidgetTree, ResetAllButton, DefaultColumnWidth + KeyColumnWidth * 0.5f));
	ResetSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	ResetSlot->SetVerticalAlignment(VAlign_Center);
	Describe(ResetAllButton, RowHelp(ESettingRow::ResetAllKeys));
	NavButton(ResetAllButton, KeyHead);
	AddPageRow(KeyHead, 6.f);
	KeyList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddPageRow(KeyList, 8.f);
}
