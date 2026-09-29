#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Environment/LevelLayoutData.h"
#include "EnvironmentLayout.generated.h"

class UEnvironmentPalette;
struct FEnvironmentPaletteEntry;

/**
 * Place one per level. Spawns the saved layout on BeginPlay and is what Build Mode edits and saves.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AEnvironmentLayout : public AActor
{
	GENERATED_BODY()

public:
	AEnvironmentLayout();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TObjectPtr<UEnvironmentPalette> Palette;

	/** Where this level's layout is stored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
	TObjectPtr<ULevelLayoutData> LayoutData;

	/** Spawns a new layout object and starts tracking it. */
	AActor* PlaceObject(const FPlacedObjectRecord& Record);

	/** Spawns an untracked, collision-free copy for Build Mode's placement preview. */
	AActor* SpawnPreview(const FPlacedObjectRecord& Record);

	/** Destroys a tracked object. Returns its record (with current transform) for undo. */
	bool RemoveObject(AActor* Object, FPlacedObjectRecord& OutRecord);

	bool IsLayoutObject(const AActor* Actor) const;
	bool GetRecord(const AActor* Object, FPlacedObjectRecord& OutRecord) const;

	void ClearAll();
	int32 GetObjectCount() const { return Objects.Num(); }

	/** Writes all tracked objects into LayoutData and saves the asset to disk (editor only). */
	bool SaveLayout(FString& OutMessage);

	const FEnvironmentPaletteEntry* FindEntry(FName Id) const;

	/** Tracked objects placed from EntryId within Radius (measured flat, ignoring height) of Center. */
	void FindObjects(FName EntryId, const FVector& Center, float Radius, TArray<AActor*>& OutActors) const;

	/** The transform to save for an object: creatures report their home spot, not where they wandered to. */
	static FTransform GetLayoutTransform(const AActor* Object);

protected:
	virtual void BeginPlay() override;

private:
	AActor* SpawnFromRecord(const FPlacedObjectRecord& Record, bool bPreview) const;

	TMap<TWeakObjectPtr<AActor>, FPlacedObjectRecord> Objects;
};
