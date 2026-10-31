// The settings menu's descriptions: what each row does, from the code that applies it. Keep a row's words here in step
// with its setting: a change to what a setting does changes its sentence too.

#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"

namespace
{
	using namespace SettingsMenu;

	FSettingHelp Help(const TCHAR* Title, const TCHAR* Body, const FString& Detail = FString())
	{
		return FSettingHelp{ Title, Body, Detail };
	}

	FString OnOffDefault(bool bOn)
	{
		return bOn ? TEXT("Default: On.") : TEXT("Default: Off.");
	}

	/** "Default 100%.": the volume sliders' detail line. */
	FString VolumeDefault()
	{
		return FString::Printf(TEXT("Default %s."), *PercentText(UAudioSettingsSubsystem::DefaultVolume));
	}

	/** Each binding's words, by UKeyBindingSubsystem's Id; the weapon slots are worded from their number. */
	FString KeyBody(FName Id)
	{
		static const TMap<FName, const TCHAR*> Bodies = {
			{ TEXT("MoveForward"), TEXT("Walks forward, the way the view faces.") },
			{ TEXT("MoveBackward"), TEXT("Walks backward, keeping your view where it is.") },
			{ TEXT("MoveLeft"), TEXT("Steps to the left without turning.") },
			{ TEXT("MoveRight"), TEXT("Steps to the right without turning.") },
			{ TEXT("Jump"), TEXT("Jumps, and pulls you up onto ledges within reach.") },
			{ TEXT("Sprint"), TEXT("Runs faster. Crouch while sprinting to slide.") },
			{ TEXT("Crouch"), TEXT("Crouches low for a steadier aim. Pressed while sprinting, it slides instead.") },
			{ TEXT("Fire"), TEXT("Fires the gun in your hands.") },
			{ TEXT("Aim"), TEXT("Raises the gun's sight to your eye for a steadier, zoomed shot at range.") },
			{ UKeyBindingSubsystem::MeleeBindingId(), TEXT("Strikes whatever gets too close in front of you.") },
			{ UKeyBindingSubsystem::GrenadeBindingId(), TEXT("Throws a grave-salt grenade that bursts among a crowd.") },
			{ UKeyBindingSubsystem::EmberBindingId(), TEXT("Fires your hero's equipped ember power. It runs for a while, then cools down before it can be used again.") },
			{ UKeyBindingSubsystem::FlaskBindingId(), TEXT("Drinks from your soul flask: a third of your health back over a moment, even mid-fight. Kills and soul-motes fill it again.") },
			{ TEXT("Reload"), TEXT("Refills the gun's magazine from your ammo.") },
			{ TEXT("NextWeapon"), TEXT("Takes the next gun down the HUD's weapon column in hand.") },
			{ TEXT("PreviousWeapon"), TEXT("Takes the gun above in the HUD's weapon column in hand.") },
			{ TEXT("DropWeapon"), TEXT("Drops the gun in your hands to the ground.") },
			{ TEXT("Interact"), TEXT("Uses what's in front of you: open, read, talk, pick up. On a gun a tap picks it up, a hold takes it straight in hand.") },
			{ TEXT("ToggleView"), TEXT("Switches between the first-person view and the over-the-shoulder third-person view.") },
			{ TEXT("Inventory"), TEXT("Opens the inventory: your loadout, ember powers, bestiary, missions and map.") },
			{ TEXT("InventoryAlt"), TEXT("A second key for the inventory, so either key opens it.") },
			{ UKeyBindingSubsystem::MapBindingId(), TEXT("Opens the inventory straight on its map page.") },
		};
		if (const TCHAR* const* Body = Bodies.Find(Id))
		{
			return *Body;
		}
		for (int32 Slot = 0; Slot < UKeyBindingSubsystem::NumWeaponSlotKeys; ++Slot)
		{
			if (Id == UKeyBindingSubsystem::WeaponSlotBindingId(Slot))
			{
				return FString::Printf(TEXT("Takes the gun in weapon slot %d in hand."), Slot + 1);
			}
		}
		return FString();
	}
}

FString SettingsMenu::PageName(int32 Page)
{
	switch (static_cast<ESettingsPage>(Page))
	{
	case ESettingsPage::Graphics:  return TEXT("Graphics");
	case ESettingsPage::Audio:     return TEXT("Audio");
	case ESettingsPage::Interface: return TEXT("Interface");
	case ESettingsPage::Controls:  return TEXT("Controls");
	}
	return FString();
}

FSettingHelp SettingsMenu::PageHelp(int32 Page)
{
	const FString Title = PageName(Page);
	switch (static_cast<ESettingsPage>(Page))
	{
	case ESettingsPage::Graphics:
		return FSettingHelp{ Title, TEXT("How the world is drawn, and what it costs your graphics card."), TEXT("Point at a setting to see what it does.") };
	case ESettingsPage::Audio:
		return FSettingHelp{ Title, TEXT("How loud each kind of sound plays. A change is heard as the slider moves."), TEXT("Point at a setting to see what it does.") };
	case ESettingsPage::Interface:
		return FSettingHelp{ Title, TEXT("What the HUD shows, and how see-through the menus are."), TEXT("Point at a setting to see what it does.") };
	case ESettingsPage::Controls:
		return FSettingHelp{ Title, TEXT("How the view turns and shakes, the gamepad's rumble, and your keyboard and mouse keys."),
			TEXT("Point at a setting or a key to see what it does.") };
	}
	return FSettingHelp();
}

FString SettingsMenu::QualityPresetHelp(EGraphicsQuality Quality)
{
	// The presets' settings are UGraphicsSettingsSubsystem's QualityVariables; the costs are Docs/Performance.md's measures on
	// Ransom's Rest's heaviest views, RX 580 at 1080p (2026-10-14).
	switch (Quality)
	{
	case EGraphicsQuality::Low:
		return TEXT("Simpler meshes, one low-resolution shadow cascade, no ambient occlusion. Only slightly faster than Medium.");
	case EGraphicsQuality::Medium:
		return TEXT("Simpler meshes, two shadow cascades, TAA smoothing. Made to hold 120 fps at 1080p on a Radeon RX 580.");
	case EGraphicsQuality::High:
		return TEXT("Full-detail Nanite meshes, sharp virtual shadows, screen-space reflections and ambient occlusion. About twice Medium's cost (50-70 fps on an RX 580).");
	case EGraphicsQuality::Epic:
		return TEXT("Lumen lighting and reflections with TSR. About five times Medium's cost (22-28 fps on an RX 580): for strong graphics cards.");
	}
	return FString();
}

ESettingRow SettingsMenu::VolumeRow(EAudioVolume Which)
{
	switch (Which)
	{
	case EAudioVolume::Master:    return ESettingRow::MasterVolume;
	case EAudioVolume::Effects:   return ESettingRow::EffectsVolume;
	case EAudioVolume::Interface: return ESettingRow::InterfaceVolume;
	case EAudioVolume::Music:     return ESettingRow::MusicVolume;
	}
	return ESettingRow::MasterVolume;
}

FSettingHelp SettingsMenu::RowHelp(ESettingRow Row)
{
	using GS = UGraphicsSettingsSubsystem;
	using CS = UControlSettingsSubsystem;
	switch (Row)
	{
	case ESettingRow::Quality:
		// The detail names the preset pointed at or chosen (SettingsMenuTabs.cpp adds QualityPresetHelp).
		return Help(TEXT("Quality"), TEXT("Sets the whole look at once: mesh detail, shadows, lighting, reflections and edge smoothing. "
			"Higher presets look richer and cost far more frame time."));
	case ESettingRow::MotionBlur:
		return Help(TEXT("Motion blur"), TEXT("Smears fast movement, a quick turn or the world rushing past, the way a camera does. "
			"It costs frame time, so it's off by default."), OnOffDefault(false));
	case ESettingRow::FieldOfView:
		return Help(TEXT("Field of view (1st person)"), TEXT("How wide the first-person view is: lower brings far targets closer, higher shows more around you and draws more of the world. "
			"Sprinting widens it a little and a sight narrows it by its zoom; third person keeps its own."),
			FString::Printf(TEXT("%s to %s. Default %s. Over the game, the view behind the menu shows it while you hold the slider."),
				*DegreesText(GS::MinFieldOfView), *DegreesText(GS::MaxFieldOfView), *DegreesText(GS::DefaultFieldOfView)));
	case ESettingRow::MasterVolume:
		return Help(TEXT("Master volume"), TEXT("Turns every sound in the game up or down together. The other three sliders are set on top of it."), VolumeDefault());
	case ESettingRow::EffectsVolume:
		return Help(TEXT("Effects volume"), TEXT("Guns, impacts, explosions and creatures, and the world around you: its wind, water and town life."), VolumeDefault());
	case ESettingRow::InterfaceVolume:
		return Help(TEXT("Interface volume"), TEXT("Menu clicks, hit and kill markers, level-ups, mission chimes and the vendor's till."), VolumeDefault());
	case ESettingRow::MusicVolume:
		return Help(TEXT("Music volume"), TEXT("The score: each area's theme, the fight music that joins it, boss themes and victory stings. "
			"It plays on, quieter, while the game is paused."), VolumeDefault());
	case ESettingRow::UITransparency:
		return Help(TEXT("UI transparency"), TEXT("How see-through the backgrounds of every menu and panel are, this one included. Text, outlines and bars stay solid."),
			FString::Printf(TEXT("0%% is solid. Default %s."), *PercentText(0.f)));
	case ESettingRow::Minimap:
		return Help(TEXT("Minimap"), TEXT("The round map in the HUD's top-right corner. Off, the mission tracker moves up into its place and the map does no work at all."),
			OnOffDefault(true));
	case ESettingRow::MinimapSize:
		return Help(TEXT("Minimap size"), TEXT("How big the HUD's minimap is, against its standard size. While you set it, an outline in its corner shows the size."),
			FString::Printf(TEXT("%s to %s. Default 100%%."), *PercentText(GS::MinMinimapScale), *PercentText(GS::MaxMinimapScale)));
	case ESettingRow::MinimapZoom:
		// The default is ULooterGraphicsSave::MinimapZoom's.
		return Help(TEXT("Minimap zoom"), TEXT("How close the minimap looks: low shows a wide stretch of the land, high just the street around you. "
			"While you set it, a live minimap in its corner shows the zoom."),
			FString::Printf(TEXT("%.1fx to %.1fx. Default 1.0x."), GS::MinMinimapZoom, GS::MaxMinimapZoom));
	case ESettingRow::FrameRate:
		return Help(TEXT("FPS counter"), TEXT("Frames per second and the frame time, top left of the HUD. The number turns orange under 60 fps and red under 30."),
			OnOffDefault(true));
	case ESettingRow::CrosshairColour:
		return Help(TEXT("Crosshair colour"), TEXT("The colour of the crosshair's lines and dot, for every gun. Each colour keeps its dark edge, "
			"so it reads over bright sky and dark ground alike."), FString::Printf(TEXT("Default: %s."),
				*GS::CrosshairColourName(ECrosshairColour::White)));
	case ESettingRow::ControlHints:
		return Help(TEXT("Control hints"), TEXT("A key and what it does, shown near the mission tracker when that control would help. "
			"Each stops for good once you've used its control."), OnOffDefault(true));
	case ESettingRow::Subtitles:
		return Help(TEXT("Subtitles"), TEXT("The words of every spoken line: captions low on the screen, and over a creature's head. "
			"Off, the lines still play; notices, prompts and mission text show either way."), OnOffDefault(true));
	case ESettingRow::LookSensitivity:
		return Help(TEXT("Look sensitivity"), TEXT("How far the view turns for a move of the mouse or the right stick. A zoomed sight slows it further on top."),
			FString::Printf(TEXT("%s to %s. Default %s, the feel the game was made with."), *LookSensitivityText(CS::MinLookSensitivity),
				*LookSensitivityText(CS::MaxLookSensitivity), *LookSensitivityText(CS::DefaultLookSensitivity)));
	case ESettingRow::CameraShake:
		return Help(TEXT("Camera shake"), TEXT("How much the view jolts: each shot's kick, the punch of taking a hit or making a kill, a boss's slams. "
			"Your aim stays put either way; 0% keeps the view still."),
			FString::Printf(TEXT("%s to %s. Default %s."), *PercentText(CS::MinCameraShake), *PercentText(CS::MaxCameraShake), *PercentText(CS::DefaultCameraShake)));
	case ESettingRow::ControllerRumble:
		return Help(TEXT("Controller rumble"), TEXT("How hard a gamepad rumbles for your shots and the hits you take; 0% turns it off. "
			"It only rumbles while you're playing on the gamepad."),
			FString::Printf(TEXT("%s to %s. Default %s."), *PercentText(CS::MinControllerRumble), *PercentText(CS::MaxControllerRumble),
				*PercentText(CS::DefaultControllerRumble)));
	case ESettingRow::ResetAllKeys:
		return Help(TEXT("Reset all keys"), TEXT("Puts every key and every Hold or Toggle choice back to the game's defaults. Your other settings stay as they are."));
	case ESettingRow::BackToPause:
		return Help(TEXT("Back"), TEXT("Goes back to the pause menu. Every change here is already saved."));
	case ESettingRow::Back:
		return Help(TEXT("Back"), TEXT("Goes back to the main menu. Every change here is already saved."));
	case ESettingRow::Count:
		break;
	}
	return FSettingHelp();
}

FSettingHelp SettingsMenu::KeyHelp(FName Id, const FString& DisplayName, const FString& DefaultKey)
{
	FString Body = KeyBody(Id);
	if (Body.IsEmpty())
	{
		// A binding added without words of its own still gets a line (Looter.UI.SettingsMenu.Help fails until it has them).
		Body = FString::Printf(TEXT("The key for %s."), *DisplayName.ToLower());
	}
	return FSettingHelp{ DisplayName, Body, DefaultKey.IsEmpty() ? FString() : FString::Printf(TEXT("Default key: %s."), *DefaultKey) };
}

FSettingHelp SettingsMenu::KeyModeHelp(const FString& DisplayName)
{
	return FSettingHelp{ FString::Printf(TEXT("%s mode"), *DisplayName),
		FString::Printf(TEXT("Hold: %s lasts while the key is held down. Toggle: one press starts it and the next press stops it."), *DisplayName.ToLower()),
		TEXT("Default: Hold.") };
}

bool SettingsMenu::HasKeyWords(FName Id)
{
	return !KeyBody(Id).IsEmpty();
}
