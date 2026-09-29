#include "AI_Looter_Shooter.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Containers/Ticker.h"
#include "Framework/Commands/InputBindingManager.h"
#include "Framework/Commands/UICommandInfo.h"
#endif

DEFINE_LOG_CATEGORY(LogLooter);

class FLooterGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		// The editor normally stops Play-In-Editor on Escape, which would swallow the game's pause menu key.
		// Move Stop to F10 for this project's sessions only (not saved to the global editor prefs). Not Shift+Escape:
		// Shift is the sprint key, so sprinting into the pause menu would end the session.
		// The editor registers its Play commands after our module loads, so retry until they exist.
		if (!IsRunningCommandlet())
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&FLooterGameModule::TryFreeEscapeForGame), 1.f);
		}
#endif
	}

private:
#if WITH_EDITOR
	/** Returns true to keep retrying. */
	static bool TryFreeEscapeForGame(float DeltaTime)
	{
		static int32 Attempts = 0;
		const TSharedPtr<FUICommandInfo> Stop = FInputBindingManager::Get().FindCommandInContext(TEXT("PlayWorld"), TEXT("StopPlaySession"));
		if (!Stop.IsValid())
		{
			if (++Attempts == 300)
			{
				UE_LOG(LogLooter, Warning, TEXT("Could not find the editor's Stop command; Escape will still stop Play-In-Editor."));
			}
			return Attempts < 300;
		}

		const EMultipleKeyBindingIndex Primary = EMultipleKeyBindingIndex::Primary;
		const FInputChord& Current = *Stop->GetActiveChord(Primary);
		if (Current.Key == EKeys::Escape && !Current.NeedsShift() && !Current.NeedsControl() && !Current.NeedsAlt())
		{
			Stop->SetActiveChord(FInputChord(EKeys::F10), Primary);
			FInputBindingManager::Get().NotifyActiveChordChanged(*Stop, Primary);
			UE_LOG(LogLooter, Log, TEXT("Play-In-Editor Stop moved to F10 so Escape opens the in-game settings menu."));
		}
		else
		{
			UE_LOG(LogLooter, Log, TEXT("Play-In-Editor Stop is bound to %s; leaving it as is."), *Current.GetInputText().ToString());
		}
		return false;
	}
#endif
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLooterGameModule, AI_Looter_Shooter, "AI_Looter_Shooter");
