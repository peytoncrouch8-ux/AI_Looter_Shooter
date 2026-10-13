// The Unpaid's lines (CreatureBarks::AllLines): Ransom's Rest's own dead, home from the Sundown Road when Saint Ada went
// dark, never paid their due. Bits of the lives they had (farms, debts, Sunday clothes, Ma's supper) said in the Reaches'
// voice: western, eerie, sometimes darkly funny, never gory. The angrier ones belong to souls fed longer on the dark
// (Restless, Gravebound), who know Ellis for a Ransom, the keepers whose light went out. The Bell-Ringer, the Providence's
// dead miners and their Foreman have lines of their own (a kind says only its own). Original lines, written for the game.

#include "Audio/CreatureVoiceBarks.h"

namespace
{
	constexpr ECreatureBark Idle = ECreatureBark::Idle;
	constexpr ECreatureBark Spot = ECreatureBark::Spot;
	constexpr ECreatureBark Hurt = ECreatureBark::Hurt;
	constexpr ECreatureBark PackmateDeath = ECreatureBark::PackmateDeath;
	constexpr ECreatureBark Death = ECreatureBark::Death;

	/** The Bell-Ringer's own lines (ABellRingerCreature::BarkKind): the chapel's old ringer, still ringing for service. */
	const TCHAR* const BellRinger = TEXT("BellRinger");

	/** The Providence's dead miners' own lines (ADeadMinerCreature::BarkKind): the eleven still working the shift they never left. */
	const TCHAR* const DeadMinerKind = TEXT("DeadMiner");

	/** The Foreman's own lines (AForemanCreature::BarkKind): Cyrus Teague, who ordered them down and never let them up. */
	const TCHAR* const ForemanKind = TEXT("Foreman");

	const FCreatureBarkLine Lines[] = {
		// --- The Bell-Ringer's: forty years at the chapel's rope, and nobody rang for him ---
		{ Idle, false, TEXT("Forty years I rang that bell. Who rings for me?"), BellRinger },
		{ Idle, false, TEXT("Service at six. Service at six. Nobody comes."), BellRinger },
		{ Idle, true, TEXT("She's dark, and still I ring. Still I ring."), BellRinger },
		{ Spot, false, TEXT("Late for service! The bell's been ringing!"), BellRinger },
		{ Spot, true, TEXT("Keeper's whelp. I'll ring your mourners in."), BellRinger },
		{ Hurt, false, TEXT("Mind the bell! Mind the bell!"), BellRinger },
		{ PackmateDeath, false, TEXT("Up, Wilbur. The bell's not done with you."), BellRinger },
		{ PackmateDeath, true, TEXT("Up! All of you, up! The bell's calling!"), BellRinger },
		{ Death, false, TEXT("Who'll ring... for me?"), BellRinger },
		{ Death, true, TEXT("Toll me out, Ransom. Toll me out."), BellRinger },

		// --- The dead miners': the shift that never ended, and the pay that never came. A kind with no angry line for a
		// situation says its plain one in its place (PickLine), so Hurt and PackmateDeath have no angry ones ---
		{ Idle, false, TEXT("Shift's not over."), DeadMinerKind },
		{ Idle, false, TEXT("Pay's Friday. Pay's always Friday."), DeadMinerKind },
		{ Idle, false, TEXT("Who's on the lift?"), DeadMinerKind },
		{ Idle, true, TEXT("Teague said the roof would hold."), DeadMinerKind },
		{ Spot, false, TEXT("You the paymaster?"), DeadMinerKind },
		{ Spot, true, TEXT("Eleven in. None out."), DeadMinerKind },
		{ Hurt, false, TEXT("Mind the timbers!"), DeadMinerKind },
		{ PackmateDeath, false, TEXT("Another one under. Keep digging."), DeadMinerKind },
		{ Death, false, TEXT("Coming up... coming up now."), DeadMinerKind },
		{ Death, true, TEXT("Tell Teague we waited."), DeadMinerKind },

		// --- The Foreman's: the company's man, still docking wages and still sure they're just past the fall. His last
		// words are one line for either tier (the table never says a line twice, and PickLine falls back to the plain one) ---
		{ Idle, false, TEXT("Just past the fall. They're just past the fall."), ForemanKind },
		{ Idle, true, TEXT("Dig. Keep digging."), ForemanKind },
		{ Spot, false, TEXT("Back to work!"), ForemanKind },
		{ Spot, true, TEXT("You're not on my payroll."), ForemanKind },
		{ Hurt, false, TEXT("That's docked from your wages!"), ForemanKind },
		{ Hurt, true, TEXT("Company property, that!"), ForemanKind },
		{ PackmateDeath, false, TEXT("Up! Shift's not over!"), ForemanKind },
		{ PackmateDeath, true, TEXT("Nobody leaves till the drift's cleared!"), ForemanKind },
		{ Death, false, TEXT("The roof..."), ForemanKind },

		// --- Idle: muttered to itself, rarely ---
		{ Idle, false, TEXT("Forty acres, and not one of 'em dry.") },
		{ Idle, false, TEXT("Somebody owes me for a mule.") },
		{ Idle, false, TEXT("I paid my tab at the Spur. I'm near certain.") },
		{ Idle, false, TEXT("Cold... why's it cold in August?") },
		{ Idle, false, TEXT("Ma'll have supper on. Ma'll have supper on.") },
		{ Idle, false, TEXT("The road was right there. Then the light went out.") },
		{ Idle, false, TEXT("Somebody's moved my fence.") },
		{ Idle, false, TEXT("Bury me in my good boots, I said. Did they listen?") },
		{ Idle, true, TEXT("Somebody put her light out. Somebody pays.") },
		{ Idle, true, TEXT("Hungry. Been hungry since the dark.") },
		{ Idle, true, TEXT("Years on that road. Years. For nothing.") },

		// --- Spot: it has seen Ellis ---
		{ Spot, false, TEXT("You're cold too. You're one of us.") },
		{ Spot, false, TEXT("Get off my land!") },
		{ Spot, false, TEXT("That's a Ransom. I know that walk.") },
		{ Spot, false, TEXT("Hey! You owe me!") },
		{ Spot, false, TEXT("Fresh out the ground, are you?") },
		{ Spot, false, TEXT("Keeper's kin! Where's the light?") },
		{ Spot, true, TEXT("Keeper's whelp. You let it go dark.") },
		{ Spot, true, TEXT("Iron in a dead hand. I'll have that.") },
		{ Spot, true, TEXT("Your saint's out, Ransom. Who pays?") },
		{ Spot, true, TEXT("We waited on that light. Years.") },

		// --- Hurt: a bullet through it ---
		{ Hurt, false, TEXT("That stung! Dead don't sting!") },
		{ Hurt, false, TEXT("Not the Sunday shirt!") },
		{ Hurt, false, TEXT("I'm already dead, you fool!") },
		{ Hurt, false, TEXT("Hey, that's my good side.") },
		{ Hurt, false, TEXT("I've had worse. I died of worse.") },
		{ Hurt, true, TEXT("Is that all a Ransom's got?") },
		{ Hurt, true, TEXT("Again. I dare you.") },
		{ Hurt, true, TEXT("Lead don't scare me no more.") },

		// --- Packmate death: one of its own fell near it ---
		{ PackmateDeath, false, TEXT("Lou? Lou, you went on without me.") },
		{ PackmateDeath, false, TEXT("There goes Wilbur. Again.") },
		{ PackmateDeath, false, TEXT("Did they cross? Did they find the shore?") },
		{ PackmateDeath, false, TEXT("Nobody dies twice! Nobody!") },
		{ PackmateDeath, true, TEXT("You'll pay for that one too.") },
		{ PackmateDeath, true, TEXT("That was my brother. Both times.") },

		// --- Death: its last words ---
		{ Death, false, TEXT("Tell Ma... I tried to get home.") },
		{ Death, false, TEXT("Oh. There it is. The road.") },
		{ Death, false, TEXT("Warm. Finally... warm.") },
		{ Death, false, TEXT("Paid... in full.") },
		{ Death, false, TEXT("Leave the porch light on.") },
		{ Death, true, TEXT("This don't square us, Ransom.") },
		{ Death, true, TEXT("I'll be back... at sundown.") },
	};
}

TConstArrayView<FCreatureBarkLine> CreatureBarks::AllLines()
{
	return MakeArrayView(Lines);
}
