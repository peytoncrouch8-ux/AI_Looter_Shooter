#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Weapons/AmmoTypes.h"
#include "HudPickupFeedWidget.generated.h"

class AWeaponBase;
class UCanvasPanel;
class ULootFanfareSubsystem;
class UTextBlock;
class UWeaponManagerComponent;

/**
 * What the player just picked up, left of the crosshair where the eyes already are: "+36 AR Ammo" in bold white rounded
 * type with a black outline, deliberately not in the menus' style (the user's call: it should feel like a game pickup,
 * not a UI message). Each pickup adds a line under the last; the stack drifts slowly upward and each line fades away a
 * second or two after it came. A handful of text blocks are reused, so a stream of pickups costs nothing new.
 *
 * An Epic or a Legendary gun landing as a drop (ULootFanfareSubsystem::OnAnnounced) flashes its name in the feed: bigger,
 * popping in white-hot and shimmering in its rarity's colour for longer than a pickup's line.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudPickupFeedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Adds a line at the bottom of the stack. */
	void AddLine(const FString& Text);

	/** Adds a flashing line (a rare drop's name): bigger, white-hot as it pops in, shimmering in Color, lasting longer. */
	void AddFlashLine(const FString& Text, const FLinearColor& Color);

	/** The feed's box; lines end at its right edge, start at its bottom and rise. */
	static constexpr float Width = 300.f;
	static constexpr float Height = 160.f;

	/** Room each line takes in the stack (the newest line's middle is half this above the box's bottom). */
	static constexpr float LineSpacing = 26.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleAmmoPickedUp(EAmmoType Type, int32 Amount);

	/** An Epic or Legendary drop landed: its name flashes. */
	void HandleLootAnnounced(const AWeaponBase* Weapon);

	/** Follows the player's pawn (it changes on respawn), and listens to the level's loot fanfare once. */
	void BindToPawn();

	struct FLine
	{
		UTextBlock* Text = nullptr;
		float Age = 0.f;
		/** Height above the feed's bottom, eased toward where the line belongs in the stack. */
		float Rise = 0.f;
		bool bActive = false;
		/** A rare drop's flashing line, in its rarity's Color. */
		bool bFlash = false;
		FLinearColor Color = FLinearColor::White;
	};

	/** A free line from the pool, set to Text at the bottom of the stack (the oldest gives way when it's full). */
	FLine* TakeLine(const FString& Text);

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TextPool;

	/** Oldest first. */
	TArray<FLine> Lines;
	TWeakObjectPtr<UWeaponManagerComponent> BoundManager;

	/** The level's loot fanfare it listens to (once bound). */
	TWeakObjectPtr<ULootFanfareSubsystem> BoundFanfare;
	FDelegateHandle FanfareHandle;
};
