#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudNotchTallyWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * The gun in hand's notches beside its name in the weapon column (the user's call, 2026-10-14): a tally mark (four cuts
 * and a slash, inked so it reads over anything, in the notch tier's colour as the gun's card has it: dim, then Blooded's
 * orange, Named's light orange, Soul-forged's cyan) and the count in floating outlined type. Collapsed, gap and all, for
 * a gun with none. A kill with the gun pops the count and flashes the mark; another gun coming into hand just shows its
 * count. UPlayerHUDWidget feeds it every frame (Update); it only repaints on a change or while popping.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudNotchTallyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows Kills and moves the pop on by DeltaSeconds: a rise on the same gun pops; bNewGun (another gun came into hand)
	 * shows its count as it is. True when the count rose on the same gun (a kill with it).
	 */
	bool Update(int32 Kills, bool bNewGun, float DeltaSeconds);

	/** What the count says: "37", "1,024"; empty for none. */
	static FString CountText(int32 Kills);

	/** The tally mark's colour for a gun with Kills: its notch tier's (dim before Blooded). */
	static FLinearColor TallyColor(int32 Kills);

	/** The count's scale Age seconds into its pop: big at once, overshooting a little, 1 from PopSeconds on. */
	static float PopScaleAt(float Age);
	static constexpr float PopSeconds = 0.45f;

	/** For the tests: the count shown (INDEX_NONE before the first), whether it shows, its words and its scale now. */
	int32 GetShownKills() const { return ShownKills; }
	bool IsShowing() const;
	FString GetShownText() const;
	float GetCountScale() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** The words, the colours and the pop as they stand now. */
	void Apply();

	UPROPERTY(Transient) TObjectPtr<UImage> TallyMark;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountWords;

	int32 ShownKills = INDEX_NONE;
	/** Seconds into the pop (at or past PopSeconds: at rest). */
	float PopAge = 1000.f;
};
