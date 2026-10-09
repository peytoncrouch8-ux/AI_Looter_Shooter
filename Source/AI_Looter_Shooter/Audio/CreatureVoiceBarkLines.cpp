// The Unpaid's lines (CreatureBarks::AllLines): Ransom's Rest's own dead, home from the Sundown Road when Saint Ada went
// dark, never paid their due. Bits of the lives they had (farms, debts, Sunday clothes, Ma's supper) said in the Reaches'
// voice: western, eerie, sometimes darkly funny, never gory. The angrier ones belong to souls fed longer on the dark
// (Restless, Gravebound), who know Ellis for a Ransom, the keepers whose light went out. Original lines, written for the game.

#include "Audio/CreatureVoiceBarks.h"

namespace
{
	constexpr ECreatureBark Idle = ECreatureBark::Idle;
	constexpr ECreatureBark Spot = ECreatureBark::Spot;
	constexpr ECreatureBark Hurt = ECreatureBark::Hurt;
	constexpr ECreatureBark PackmateDeath = ECreatureBark::PackmateDeath;
	constexpr ECreatureBark Death = ECreatureBark::Death;

	const FCreatureBarkLine Lines[] = {
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
