// USessionSubsystem: words for people in the session picker (play time, when saved, where).

#include "Session/SessionSubsystem.h"
#include "Areas/AreaDefinition.h"
#include "Misc/PackageName.h"

FString USessionSubsystem::FormatPlayTime(double Seconds)
{
	const int64 Total = FMath::Max<int64>(FMath::FloorToInt64(Seconds), 0);
	if (Total < 60)
	{
		return FString::Printf(TEXT("%lld s"), Total);
	}
	const int64 Minutes = Total / 60;
	if (Minutes < 60)
	{
		return FString::Printf(TEXT("%lld min"), Minutes);
	}
	return FString::Printf(TEXT("%lld h %02lld min"), Minutes / 60, Minutes % 60);
}

FString USessionSubsystem::FormatSavedTime(const FDateTime& Saved, const FDateTime& Now)
{
	const FString Clock = FString::Printf(TEXT("%02d:%02d"), Saved.GetHour(), Saved.GetMinute());
	if (Saved.GetDate() == Now.GetDate())
	{
		return TEXT("Today ") + Clock;
	}
	if (Saved.GetDate() == (Now - FTimespan::FromDays(1.0)).GetDate())
	{
		return TEXT("Yesterday ") + Clock;
	}
	static const TCHAR* Months[] = { TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"), TEXT("Jul"),
		TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec") };
	return FString::Printf(TEXT("%s %d, %d"), Months[FMath::Clamp(Saved.GetMonth(), 1, 12) - 1], Saved.GetDay(), Saved.GetYear());
}

FString USessionSubsystem::PlaceName(const FString& Map)
{
	// "/Game/Maps/Lvl_TutorialIsland" -> "TutorialIsland" -> "Tutorial Island".
	FString Name = FPackageName::GetShortName(Map);
	Name.RemoveFromStart(TEXT("Lvl_"));
	return FName::NameToDisplayString(Name, false);
}

FString USessionSubsystem::AreaName(const FString& Map)
{
	// The area played there ("Skyreach" for the tutorial island) when the area assets are in; else the level file's name.
	const UAreaDefinition* Area = UAreaDefinition::FindByMap(Map);
	if (Area && !Area->DisplayName.IsEmpty())
	{
		return Area->DisplayName.ToString();
	}
	return PlaceName(Map);
}
