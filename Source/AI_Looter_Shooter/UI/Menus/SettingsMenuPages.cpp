// USettingsMenuWidget's Graphics, Interface and Controls pages: their rows, each a keyboard and controller stop with its
// words for the help card (SettingsMenuHelp.h). The Audio page is SettingsMenuAudio.cpp.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsPageArt.h"
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
using namespace SettingsMenu::Layout;

namespace
{
	// The Controls page's two switches (MakeControlsPage): their buttons take their own clicks, so these live here.
	const FName ActionInvertYOn(TEXT("InvertYOn"));
	const FName ActionInvertYOff(TEXT("InvertYOff"));
	const FName ActionAimAssistOn(TEXT("AimAssistOn"));
	const FName ActionAimAssistOff(TEXT("AimAssistOff"));
}

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
		UGraphicsSettingsSubsystem::MaxFieldOfView, FovSlider, FovText, FieldOfViewStep, [](float Value) { return DegreesText(Value); });
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
	// The pause menu's size (the user, 2026-10-31): it shows the new size as the menu comes back.
	USlider* PauseSize = nullptr;
	UTextBlock* PauseSizeText = nullptr;
	UWidget* PauseSizeRow = MakeSliderRow(TEXT("Pause menu size"), UGraphicsSettingsSubsystem::MinPauseMenuScale,
		UGraphicsSettingsSubsystem::MaxPauseMenuScale, PauseSize, PauseSizeText);
	AddPageRow(Describe(NavSlider(PauseSizeRow, PauseSize), RowHelp(ESettingRow::PauseMenuSize)));
	PauseSizeSlider = PauseSize;
	PauseSizeValue = PauseSizeText;
	PauseSizeSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandlePauseSizeChanged);
	PauseSizeSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandlePauseSizeReleased);
	PauseSizeSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandlePauseSizeReleased);
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
		UGraphicsSettingsSubsystem::MaxMinimapZoom, MinimapZoom, MinimapZoomText, 0.05f,
		[](float Value) { return FString::Printf(TEXT("%.1fx"), Value); });
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
	// The crosshair's colour, every gun's (the user's call, 2026-10-29: white with a dark edge unless picked here).
	TArray<FString> ColourNames;
	for (const ECrosshairColour Colour : AllCrosshairColours)
	{
		ColourNames.Add(UGraphicsSettingsSubsystem::CrosshairColourName(Colour));
	}
	TArray<ULooterButton*> Colours;
	UWidget* ColourRow = MakeChoiceRow(TEXT("Crosshair colour"), ColourNames, ActionCrosshairColour, QualityColumnWidth, Colours);
	AddPageRow(Describe(NavChoices(ColourRow, Colours), RowHelp(ESettingRow::CrosshairColour)));
	for (ULooterButton* Button : Colours)
	{
		CrosshairColourButtons.Add(Button);
	}
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
		UControlSettingsSubsystem::MaxLookSensitivity, LookSlider, LookValueText, UControlSettingsSubsystem::LookSensitivityStep,
		[](float Value) { return LookSensitivityText(Value); });
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

	// Two switches: Invert Y (mouse and stick) and the gamepad's Aim assist. MakeArtButton wires a click to HandleButton, which
	// has no case for these, so each switch takes its own click here: it sets the control (saved at once, as a switch is),
	// says so in the status line and lights the side that is the setting.
	using FControlSetter = void (UControlSettingsSubsystem::*)(bool, bool);
	auto AddControlSwitch = [this](const TCHAR* Label, ESettingRow Row, FName OnAction, FName OffAction, bool bNow, FControlSetter Setter,
		const TCHAR* OnMessage, const TCHAR* OffMessage)
	{
		ULooterButton* OnSide = nullptr;
		ULooterButton* OffSide = nullptr;
		UWidget* SwitchRow = MakeToggleRow(Label, TEXT("On"), TEXT("Off"), OnAction, OffAction, 0, false, OnSide, OffSide);
		AddPageRow(Describe(NavChoices(SwitchRow, { OnSide, OffSide }), RowHelp(Row)));
		OnSide->SetHighlighted(bNow);
		OffSide->SetHighlighted(!bNow);
		for (ULooterButton* Side : { OnSide, OffSide })
		{
			Side->OnButtonClicked.BindWeakLambda(this, [this, Setter, OnAction, OnMessage, OffMessage,
				WeakOn = TWeakObjectPtr<ULooterButton>(OnSide), WeakOff = TWeakObjectPtr<ULooterButton>(OffSide)](ULooterButton* Clicked)
			{
				StopListening();
				const bool bOn = Clicked && Clicked->Action == OnAction;
				if (UControlSettingsSubsystem* Controls = GetControls())
				{
					(Controls->*Setter)(bOn, /*bSave*/ true);
				}
				if (ULooterButton* OnButton = WeakOn.Get())
				{
					OnButton->SetHighlighted(bOn);
				}
				if (ULooterButton* OffButton = WeakOff.Get())
				{
					OffButton->SetHighlighted(!bOn);
				}
				SetStatus(bOn ? OnMessage : OffMessage, Color::TextDim());
				RefreshKeyLabels();
				SetKeyboardFocus();
			});
		}
	};
	const UControlSettingsSubsystem* Controls = GetControls();
	AddControlSwitch(TEXT("Invert Y"), ESettingRow::InvertY, ActionInvertYOn, ActionInvertYOff,
		Controls ? Controls->IsInvertY() : UControlSettingsSubsystem::DefaultInvertY, &UControlSettingsSubsystem::SetInvertY,
		TEXT("Invert Y on: looking up and down is reversed."), TEXT("Invert Y off."));
	AddControlSwitch(TEXT("Aim assist"), ESettingRow::AimAssist, ActionAimAssistOn, ActionAimAssistOff,
		Controls ? Controls->IsAimAssistOn() : UControlSettingsSubsystem::DefaultAimAssist, &UControlSettingsSubsystem::SetAimAssistOn,
		TEXT("Aim assist on: the gamepad's view slows on creatures and drifts onto them."), TEXT("Aim assist off."));

	// The key list's heading: how to change a key (RefreshNavLook words it per device), with Reset all keys beside it, a stop
	// of its own above the keys (it resets only them, so it lives with them rather than in the footer).
	AddPageRow(MakeLedgerSection(TEXT("Keyboard and mouse keys")));
	UHorizontalBox* KeyHead = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	KeyHintText = MakeText(WidgetTree, TEXT(""), KeyHintSize, Color::SlateLabel());
	KeyHintText->SetFont(Font(KeyHintSize, false));
	KeyHintText->SetAutoWrapText(true);
	UHorizontalBoxSlot* HintSlot = KeyHead->AddChildToHorizontalBox(KeyHintText);
	HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HintSlot->SetVerticalAlignment(VAlign_Center);
	ResetAllButton = MakeArtButton(ActionResetAll, 0, TEXT("Reset all keys"), DefaultFontSize, EButtonKind::Normal);
	UHorizontalBoxSlot* ResetSlot = KeyHead->AddChildToHorizontalBox(ResetAllButton);
	ResetSlot->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
	ResetSlot->SetVerticalAlignment(VAlign_Center);
	Describe(ResetAllButton, RowHelp(ESettingRow::ResetAllKeys));
	// On a band of its own, unnumbered (it isn't a setting).
	UWidget* KeyHeadRow = MakeLedgerRow(KeyHead, nullptr, SettingsPageArt::RowHeight, /*bNumbered*/ false);
	NavButton(ResetAllButton, KeyHeadRow);
	AddPageRow(KeyHeadRow);
	KeyList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	AddPageRow(KeyList);
	// The key rows (RebuildControls) are numbered and banded on from the rows above them.
	KeyNumberStart = PageRowNumbers.IsValidIndex(BuildPage) ? PageRowNumbers[BuildPage] : 1;
	KeyBandStart = PageBandCount.IsValidIndex(BuildPage) ? PageBandCount[BuildPage] : 0;
}
