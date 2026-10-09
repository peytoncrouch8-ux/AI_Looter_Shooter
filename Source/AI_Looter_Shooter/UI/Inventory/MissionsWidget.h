#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "MissionsWidget.generated.h"

class ALooterHUD;
class UHorizontalBox;
class UImage;
class ULooterButton;
class UMissionDefinition;
class UMissionRunner;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
enum class EInventoryPage : uint8;
enum class EMissionStatus : uint8;

/**
 * The missions, the inventory's third page, laid out like the pages beside it:
 *  - left: the mission log by section: active (the tracked one marked), available, completed
 *  - right: the chosen mission: its kind and area, title and summary, Track / Untrack for one running here; under it
 *    its steps (the ones done ticked off, the current one's objectives with their progress, or once all are done "Ready
 *    to turn in: talk to Delia") and what it gives
 *  - bottom: what the keys do
 * W / S, the arrows or the D-pad choose a mission (so does the mouse); E, Enter or the button tracks or untracks it (the
 * minimap guides to the tracked one); 1 and 2 (the left shoulder, or the title tabs) turn to the loadout and the
 * bestiary; Esc, Tab and I close. It follows the mission runner while it's open, so progress shows as it's made.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMissionsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(ALooterHUD* InHUD);
	void Close();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** A mission's card in the list, and the parts that restyle with the selection. */
	struct FCard
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
	};

	// MissionsWidget.cpp: opening, layout and the list
	void Gather();
	void RebuildList();
	FCard MakeMissionCard(int32 Index);
	void Select(int32 Index, bool bScrollIntoView);
	void Restyle();
	void ShowPage(EInventoryPage Page);
	void HandleCardClicked(ULooterButton* Button);
	void HandleCardHovered(ULooterButton* Button);
	void HandleTabClicked(ULooterButton* Button);
	void HandleRunnerChanged();

	// MissionsWidgetDetails.cpp: the chosen mission, the prompts, tracking, and what the page asks
	void RefreshDetails();
	void AddSteps(const UMissionDefinition& Mission, EMissionStatus Status);
	void AddStepLine(const FString& Text, bool bDone, bool bCurrent);
	void AddObjectiveLine(const FString& Text, const FString& Progress, int32 Required, float Fraction, bool bDone);
	void AddRewards(const UMissionDefinition& Mission);
	void RefreshPrompts();
	bool CanTrackSelected() const;
	void ToggleTrack();
	void HandleTrackClicked(ULooterButton* Button);

	UMissionRunner* GetRunner() const;
	const UMissionDefinition* GetSelected() const;
	EMissionStatus GetSelectedStatus() const;

	/** "Ransom's Rest" for the area id RansomsRest (its asset's name), or the id. */
	FString AreaName(FName AreaId) const;

	/** What a card's second line says: its step, where it waits, or that it's done. */
	FString DescribeStanding(const UMissionDefinition& Mission, EMissionStatus Status) const;

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<UMissionRunner> Runner;
	FDelegateHandle ChangedHandle;

	/** Every mission listed, in the list's order, and where each stands (parallel arrays). */
	UPROPERTY(Transient) TArray<TObjectPtr<UMissionDefinition>> Listed;
	TArray<EMissionStatus> Statuses;

	/** Areas' names by id, read as the page opens. */
	TMap<FName, FString> AreaNames;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> ListCount;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ListBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailsHeader;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> DetailsBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> StepsBox;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;

	/** One per listed mission, by index into Listed. */
	TArray<FCard> Cards;
	int32 Selected = 0;
	/** The runner changed while the page was open: it refreshes on the next tick (a burst of changes, one refresh). */
	bool bDirty = false;

	FSlateBrush DoneBrush;
	FSlateBrush CurrentBrush;
};
