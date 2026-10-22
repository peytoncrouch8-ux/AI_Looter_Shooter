# HUD portraits per hero: wiring (for Main, 2026-10-20)

What exists (HUD-PORTRAITS agent): `Art/Icons/HudPortrait_<Hero>.svg` for Odessa, Crane, Gauge, Pike (Ellis's
`HudPortrait.svg` untouched); `Art/Icons/HudPortrait.py [Hero | all]` (default Ellis, byte-identical output) writes
`Source/AI_Looter_Shooter/UI/Style/HudPortraitData_<Hero>.inl`. Nothing includes them yet. Each hero file:

- includes `UI/Style/LooterUIStyle.h` and `UI/Style/HudPortraitData.inl` itself (both `#pragma once`), so it shares
  Ellis's `HudPortraitData::FPicture`, `FGlow` and `WindowUnits`;
- defines `namespace HudPortraitData::<Hero>` with exactly Ellis's names: `constexpr double BlinkCenterY`,
  `FPicture Glass() Bust() EyesOpen() BrowsCalm() EyesHurt() EyesFlare()`,
  `TArray<FGlow> EyesOpenGlows() EyesHurtGlows() EyesFlareGlows()`.

Everything per hero is in the data: Crane has one eye (his glow lists hold 2 glows, not 4; the flare adds a warm glint on
his coin), Gauge's lamp glows warm (`#FFE2A8` body, `#FFB347` edge) and its blink centre is the lamp (89.0); the widget's
loops already take any count and colour. So the only C++ work: pick the set by hero id, and give each hero its own
texture names (`PaintedIconBrush` caches by name for the session).

## 1. `UI/HUD/HudPortraitWidget.h`

- Forward-declare `class UCanvasPanel;`.
- Public, after `PlayLevelUp`:
```cpp
	/**
	 * Whose portrait the window shows (UHeroDefinition::Id: "Ellis", "Odessa", "Crane", "Gauge", "Pike"; any other id
	 * shows Ellis's). Unset, the window asks for the hero played as it builds; a change once built redraws the window.
	 */
	void SetHero(FName InHeroId);
```
- Private:
```cpp
	/** Fills Content for HeroId: glass, bust, the eye groups and the red flash (again on a hero change). */
	void BuildPortrait();

	/** The upright content inside the diamond clip; BuildPortrait fills it. */
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Content;
	/** The hero shown (None until set or found: Ellis's portrait). */
	FName HeroId;
```
- Class comment: "a masked gunslinger ... (Art/Icons/HudPortrait.svg, the user's pick)" -> "the played hero's inked bust
  (Art/Icons/HudPortrait[_<Hero>].svg; Ellis's the user's pick)".

## 2. `UI/HUD/HudPortraitWidget.cpp`

Includes (after `HudPortraitData.inl`), plus the hero lookup:
```cpp
#include "UI/Style/HudPortraitData_Odessa.inl"
#include "UI/Style/HudPortraitData_Crane.inl"
#include "UI/Style/HudPortraitData_Gauge.inl"
#include "UI/Style/HudPortraitData_Pike.inl"
#include "Embers/HeroDefinition.h"
#include "Session/SessionSubsystem.h"
```

In the anonymous namespace (after `PlaceOnCanvas`), the set table:
```cpp
	/** One hero's portrait: the functions Art/Icons/HudPortrait.py generates in that hero's HudPortraitData namespace. */
	struct FPortraitSet
	{
		/** Starts each picture's texture name: painted textures are shared by name for the session, so one set per hero. */
		const TCHAR* TexturePrefix;
		HudPortraitData::FPicture (*Glass)();
		HudPortraitData::FPicture (*Bust)();
		HudPortraitData::FPicture (*EyesOpen)();
		HudPortraitData::FPicture (*BrowsCalm)();
		HudPortraitData::FPicture (*EyesHurt)();
		HudPortraitData::FPicture (*EyesFlare)();
		TArray<HudPortraitData::FGlow> (*EyesOpenGlows)();
		TArray<HudPortraitData::FGlow> (*EyesHurtGlows)();
		TArray<HudPortraitData::FGlow> (*EyesFlareGlows)();
		double BlinkCenterY;
	};

#define LOOTER_PORTRAIT_SET(Prefix, Ns) FPortraitSet{ TEXT(Prefix), &Ns::Glass, &Ns::Bust, &Ns::EyesOpen, &Ns::BrowsCalm, \
	&Ns::EyesHurt, &Ns::EyesFlare, &Ns::EyesOpenGlows, &Ns::EyesHurtGlows, &Ns::EyesFlareGlows, Ns::BlinkCenterY }

	/** The hero's portrait by UHeroDefinition::Id; Ellis's for any other id (None, a hero without one). */
	const FPortraitSet& PortraitSetFor(FName HeroId)
	{
		// Ellis's textures keep the names they had before the pick ("HudPortraitBust").
		static const FPortraitSet EllisSet = LOOTER_PORTRAIT_SET("HudPortrait", HudPortraitData);
		static const FPortraitSet OdessaSet = LOOTER_PORTRAIT_SET("HudPortraitOdessa", HudPortraitData::Odessa);
		static const FPortraitSet CraneSet = LOOTER_PORTRAIT_SET("HudPortraitCrane", HudPortraitData::Crane);
		static const FPortraitSet GaugeSet = LOOTER_PORTRAIT_SET("HudPortraitGauge", HudPortraitData::Gauge);
		static const FPortraitSet PikeSet = LOOTER_PORTRAIT_SET("HudPortraitPike", HudPortraitData::Pike);
		static const FName Odessa(TEXT("Odessa")), Crane(TEXT("Crane")), Gauge(TEXT("Gauge")), Pike(TEXT("Pike"));
		return HeroId == Odessa ? OdessaSet : HeroId == Crane ? CraneSet : HeroId == Gauge ? GaugeSet
			: HeroId == Pike ? PikeSet : EllisSet;
	}

#undef LOOTER_PORTRAIT_SET

	/** "HudPortrait" + hero + picture: "HudPortraitBust" for Ellis, "HudPortraitCraneBust" for Crane. */
	FName PortraitTextureName(const FPortraitSet& Set, const TCHAR* Picture)
	{
		return FName(*FString::Printf(TEXT("%s%s"), Set.TexturePrefix, Picture));
	}
```

`RebuildWidget`: keep everything up to and including `ContentSlot->SetSize(FVector2D(WindowSize));`, but name the local
panel `ContentPanel` and store it; then ask for the hero if nobody set one, and build:
```cpp
		Content = ContentPanel;
		if (HeroId.IsNone())
		{
			// The hero played: the session's pick, or Looter.Hero's with no session (the same answer UEmberComponent and
			// UBodyFigureComponent get as the character begins; works before the pawn exists). Null in a test world: Ellis.
			if (const UHeroDefinition* Hero = USessionSubsystem::FindPlayedHero(this))
			{
				HeroId = Hero->Id;
			}
		}
		BuildPortrait();
	}
	return Super::RebuildWidget();
}
```

Move today's lines from `// The glass is the window's background` to the last `ShowOpacity(FlareEyes, ...)` into
`BuildPortrait()`, with these changes only:
```cpp
void UHudPortraitWidget::BuildPortrait()
{
	if (!WidgetTree || !Content)
	{
		return;
	}
	const FPortraitSet& Set = PortraitSetFor(HeroId);
	Content->ClearChildren();
	// ... today's code, with:
	//   HudPortraitData::Glass()          -> Set.Glass()            (and Bust, EyesOpen, BrowsCalm, EyesHurt, EyesFlare)
	//   HudPortraitData::EyesOpenGlows()  -> Set.EyesOpenGlows()    (and EyesHurtGlows, EyesFlareGlows)
	//   TEXT("HudPortraitGlass")          -> PortraitTextureName(Set, TEXT("Glass"))   (likewise Bust, EyesOpen,
	//                                        BrowsCalm, EyesHurt, EyesFlare)
	//   HudPortraitData::BlinkCenterY     -> Set.BlinkCenterY       (the blink group's pivot)
	//   Content (the local)               -> Content (the member)
	// The "Start at rest" block stays at the end, so a redraw starts calm too.
}

void UHudPortraitWidget::SetHero(FName InHeroId)
{
	if (InHeroId == HeroId)
	{
		return;
	}
	HeroId = InHeroId;
	if (Content)
	{
		BuildPortrait();   // already on screen: redraw for the new hero
	}
}
```
`NativeTick` needs no change (it checks the group pointers, which BuildPortrait sets again). `HudPortraitData::WindowUnits`
(the `PortraitScale` constant) stays as is: every hero's SVG is the same 200-unit square.

Alternative source of the id, if you'd rather follow the pawn: `UEmberComponent::Find(GetOwningPlayerPawn())->GetHero()->Id`
(null-check both); on the player's character it is the same hero, but the pawn may not exist yet when the HUD builds.

## 3. `Dev/HeroDevCommands.cpp` (Looter.Hero switching mid-play)

In `WearHeroNow(UWorld* GameWorld)`, after the body loop (include `UI/HUD/HudPortraitWidget.h`):
```cpp
		// The HUD's portrait window too.
		const UHeroDefinition* Played = USessionSubsystem::FindPlayedHero(GameWorld);
		for (TObjectIterator<UHudPortraitWidget> It; It; ++It)
		{
			if (Played && It->GetWorld() == GameWorld)
			{
				It->SetHero(Played->Id);
			}
		}
```

## 4. Optional: Gauge's lamp flickers while hurt

The data already shows the visor dropped and the lamp dimmed (EyesHurt). For the flicker the brief describes, add
`bool bHurtFlickers = false;` as the last member of `FPortraitSet` (the macro leaves it false) and set it for Gauge only:
```cpp
		static const FPortraitSet GaugeSet = []
		{
			FPortraitSet Set = LOOTER_PORTRAIT_SET("HudPortraitGauge", HudPortraitData::Gauge);
			Set.bHurtFlickers = true;
			return Set;
		}();
```
Members `UPROPERTY(Transient) TObjectPtr<UWidget> HurtGlows;`, `float ShownHurtGlow = 1.f;`, `bool bHurtFlickers = false;`;
in BuildPortrait put the hurt glows in their own group (still behind the hurt picture):
```cpp
	UCanvasPanel* HurtGlowGroup = AddPortraitGroup(WidgetTree, HurtGroup);
	HurtGlows = HurtGlowGroup;
	bHurtFlickers = Set.bHurtFlickers;
	AddPortraitGlows(WidgetTree, HurtGlowGroup, Set.EyesHurtGlows());
```
and in NativeTick after the squint's ShowOpacity:
```cpp
	if (bHurtFlickers && HurtGlows && Squint > 0.f)
	{
		// A failing lamp: two beating sines, never fully out (on BlinkClock: its wrap every 5.2 s can't show in a flicker).
		const float Flicker = 0.45f + 0.55f * FMath::Abs(FMath::Sin(BlinkClock * 23.f) * FMath::Sin(BlinkClock * 7.3f + 1.f));
		ShowOpacity(HurtGlows, Flicker, ShownHurtGlow);
	}
```

## 5. Docs and checks

- `CODEMAP.md` line for LooterUIPaintedIcons/HudPortraitData: "`UI/Style/HudPortraitData.inl` (Ellis) and
  `HudPortraitData_<Hero>.inl` (Odessa, Crane, Gauge, Pike; namespace `HudPortraitData::<Hero>`): generated by
  `Art/Icons/HudPortrait.py [Hero|all]` from `Art/Icons/HudPortrait[_<Hero>].svg` (don't edit)." HudPortraitWidget's line:
  add "the played hero's portrait (SetHero)".
- `Docs/Pipeline.md` (HUD): each hero's HUD portrait is an SVG in Ellis's structure (groups glass, colour offset 1.3 x
  0.9, rim, ink, eyes-open/hurt/flare with gradient ellipses as glows); the generator now also reads plain `<circle>` and
  `<ellipse>` and refuses a gradient ellipse outside the eye groups; previews `Saved/ArtPreviews/Icons/HudPortrait_<Hero>_<State>[_Hud].png`.
- Check: build; `Looter.Hero Crane` (+ `Looter.Hero.Unlock 1`) then `Looter.PlayLevel`; `Tools\hudshots.ps1` per hero
  (calm, a hit, low health, a level-up). Optional test `Looter.HUD.PortraitSets`: for each roster id, `PortraitSetFor`
  gives non-empty Glass/Bust layers and glows, and an unknown id gives Ellis's.
- Cost: four .inl (50-70 KB each) compile in this one .cpp; only the played hero's textures are ever drawn (Bust about
  280 x 240 px at 2x, like Ellis's).
