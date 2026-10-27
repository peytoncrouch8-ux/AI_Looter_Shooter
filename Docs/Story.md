# Revenant

> *Seven outlaws killed you. Something sent you back to bring them in.*

On Teropa, in the frontier country called the Reaches, the Dunne Gang shot Ellis Ransom on the lookout above the family farm on Ransom's Rest. They shot Ellis's father on the path below, robbed the town's saint and flew off in their skiff. A week later Ellis claws out of a grave in the family plot. A soft-spoken stranger holds their soul in pawn until all seven outlaws are brought down. No living gun can kill what the gang has become.

The campaign's single canon (`CLAUDE.md` points here), from `Docs/Story/2-Revenant.md` on the stories branch, revised by the user's decisions (*Decisions*). Other docs: the first area `Docs/Areas/RansomsRest.md` (*Ransom's Rest: the first area*); its 2026-10-13 expansion, with every mission's givers, steps and lines, `Docs/Areas/RansomsRest_Expansion.md`; experience and pacing `Docs/Progression.md`. History and superseded text: `Docs/Archive/Story_history.md`.

**The saga (2026-10-15).** The user asked for a long-running Borderlands-like story across many areas ("Why is Ellis
here? Where does the story go? What's the goal? Who's the final boss? What is the world lore?") and chose: deepen and
scale Revenant (keep its bones), 15–18 regions, and seed the arc into Ransom's Rest. The readable bible for the user:
https://claude.ai/artifact/Gr66uUTXWzetBTaHvE9j9Z. Everything it adds is folded in below and marked *(saga)*; the user's
open calls on it are Decisions 15. Nothing saga goes into the game before those calls.

- **Genre and tone:** Weird West revenge. Dark and mythic, with dry humor (mostly from a crow).
- **Feels like:** Red Dead's *Undead Nightmare*, Hunt: Showdown, *The Dark Tower*, *Kill Bill*, *The Ballad of Buster Scruggs*.
- **Themes:** debts and who really owes what; mentors and betrayal; fathers; letting the dead go.
- **Setting:** the Reaches, a frontier on Teropa, an earth-like planet.
- **Protagonist:** Ellis Ransom, an outlaw shot by their own gang, now a Revenant.
- **Antagonist:** Mister Sexton, the polite stranger who sent Ellis back, and the gang's leader, Ambrose Dunne, “the Deacon”.
- **Hub:** Ransom's Rest: the Ransom farm and family plot, the town and its Main Street, the chapel, Ransom's Point where Ellis died, and the burial boards.
- **Map order** (the user's call, 2026-10-13): 1 Ransom's Rest, 2 the *Gilded Lily* (by train), 3 Skyreach (by skylift from the *Lily*'s north side).
- **Endgame:** the Ledger (bounties on escaped souls), the Gravesend Deeps, Hanging Moon nights.

## What changed

The before/after table against the stories branch's original is in the archive. The current rules each live in one section: the setting (*The world*), areas opened by missions (*Campaign gating*), the railroad (*Getting around*), legendaries from anything (*Enemy ranks and legendary drops*), the new cast (*Cast*), Skyreach (*Skyreach, area 3*).

**Writing notes** (the game's voice):
- The planet is **Teropa**. Name it in the docs, and on screen as the session picker's subtitle under each session (“Teropa · Ransom's Rest”, built at step 8); the main menu's subtitle says Teropa (2026-10-13). Characters rarely say it, just as people rarely say “Earth”.
- Ransom's Rest is a high farm valley, and its town shares the name; locals call it “the Rest”. Write “Ransom's Rest” in prose, and never stack a second possessive on it: rephrase as “the Rest's”, “on Ransom's Rest” (as you would of a ranch) or “of Ransom's Rest”.
- Never call a story place an island but Skyreach, Teropa's one sky island.
- The town's main street is just “Main Street”.
- Ellis is “they” in all text.
- Abel calls Ellis “El”, the Deacon calls Ellis “kid”, Sexton says “friend” and Hob says “sunshine”.
- Sexton's copperplate hand is in the fiction only: on screen every page and panel keeps the game's UI style (Concept C).

## Cold open

*The intro* (2026-10-27, note 3): on a new game, before the cold open, Teropa from orbit and Ransom's Rest in eleven narration captions ending on the logline; its words are DA_Lines_NewGameIntro (create_story_lines.py).

Plays as soon as a new game starts, skippable, once per session (since 2026-10-13). The title REVENANT rises through the white of a bank of evening cloud, and the white thins on the Dunne Gang's skiff gliding out of it. Ellis comes out of the grave with the Common Bullpup they were buried with. (A session begun on Skyreach before then and still there gets one trip from the jetty's board: the packet skiff's ride into the cloud, then this; `StationBoard`.)

1. **Seven days ago.** The player stands on the Dunne Gang's skiff's deck, free to look around: dark-hulled, gliding out of a bank of evening cloud in the west, low over the plains and up Gravewind Canyon toward Ransom's Rest at sunset. Seven figures stand on deck in silhouette. The Deacon, beside Ellis: *“Home, kid.”* Caption: *Ransom's Rest. Seven days ago.* Ellis told them where to moor: in a cleft of the canyon under the cliffs of Ransom's Point, the one place on Ransom's Rest nobody in town can see. Fade.
2. **Dusk on Ransom's Point**, the lookout above the Ransom farm, seen through Ellis's eyes. The chapel bell clangs across the Rest. The gang is back from the chapel, and the Deacon holds Saint Ada's ember, a burning orange light, in a gloved hand. Ellis steps between him and the cliff stairs down to the skiff: *“Put her back, Deacon. Not here. Not my home.”* Abel Ransom, Ellis's father and the Keeper of Saint Ada, runs up the bluff path with a lantern and a shotgun: *“El!”* Lucky Ned fires. Abel falls, and his lantern rolls off the path into the dark. Ellis draws on the Deacon. The Deacon is faster, and his face is sad and fond: *“I'm sorry, kid.”* The sky turns over. Far along the railing, a tall man in a stovepipe hat sits with his legs crossed, watching. Black.
3. **Seven days later.** Birdsong, bees, a golden late-summer afternoon. A hand breaks through the dirt of a fresh grave in the Ransom family plot; this is the player's first input. The headboard reads *ELLIS RANSOM. CAME HOME AT THE LAST.* The one beside it reads *ABEL RANSOM. KEEPER OF SAINT ADA. HE HELD THE DOOR.* Its mound is whole, but frost lies on it in the afternoon sun, and the flowers on it have gone black. A one-eyed crow lands on Ellis's headboard: *“Morning, sunshine. Most folks who get up do it after dark.”*

Staged cheaply on purpose: a camera moved by code, captions, sound and one muzzle flash. The gang: flat black silhouettes (the UE mannequin, posed, no costumes). The man on the railing: Sexton's seated model drawn in black. The gang's skiff: the packet skiff model in dark paint; the evening cloud: the cloud-bank card.

## The world

- **Teropa** (new): an earth-like world with one sun, one moon, seasons, seas and mountain ranges, and solid ground under every town. All of the story happens in **the Reaches**, the western frontier of Teropa's great continent, where settlement ends: farm valleys and river gorges, mesas, swamp country, smelter towns and quarries. Its western edge is **the Rim**, a long rimrock escarpment over deep canyons, which the Rim Rangers ride. Past it the plains run empty to the sunset: nobody settles the land the dead cross. The railroad runs to the Rim and no farther.
- **Ransom's Rest** (new): a high valley of late-summer farmland on the Rim. Ridges close it on the north, east and south: Larkspur Ridge, the East Ridge and the Hogback. On the west it ends at a 70 m drop into Gravewind Canyon; past the canyon the empty plains run on to the sunset, so from the valley you can watch the sun go all the way down. The ground is broken: cliff bands at the ridges' feet, rock outcrops and a lone butte in the fields, a dry wash running to the Rim, and the ring of cliffs where the Sink fell in. The place and its town share the name; locals call it “the Rest”.
- In the Reaches the dead are **given to the wind**. Farm families bury their dead for seven days to say goodbye, with their guns beside them (“buried with their irons”). Then the keeper lays them on a **burial board** on high ground facing west and sits the night beside them by lantern light. At dusk the **Gravewind**, a cold wind that blows only then, runs down off the high ground, lifts their souls and carries them west into the sunset, down **the Sundown Road** (new), toward **the Far Shore**. At dawn the keeper carries the body home to its grave for good, under its headboard. On Ransom's Rest the burial boards stand on Gravewind Point, a promontory of the Rim, where the wind pours off the Rim and out over the canyon at dusk.
- **The Far Shore is on no map** (new). The Sundown Road leaves the world where the sun goes down and ends at a grey pier on a sea of fog, and the shore lies across the fog. The crossing is long: a soul follows its saint's light for years before it gets there.
- Every town has a **saint**: its first settler, laid in a granite **Reliquary** banded in gold. The saint's soul stays behind as an **ember** of orange light floating over the lid. The saints are lighthouses: their light shines down the Sundown Road and lays a way across the fog, and a town's dead steer by it all the way to the Far Shore. Nobody touches a Reliquary, ever. *(saga)* A saint is a soul that refused to cross: Saint Ada lay down on the first burial board, asked the wind to take her and stayed at the edge of the world so her people would never pay the fare. Every saint is a sacrifice standing watch, and no saint reaches the Far Shore. The towns have forgotten this; Father Aldana tells it (*Seeds in Ransom's Rest*).
- **The old fare** (new). Before there were saints, the dead crossed the fog by ferry and paid the ferryman, and families laid a gold coin on each eye of their dead for the fare. When the first settlers became saints and lit the way, the dead stopped needing a ferry, and the custom died out. The old coins still turn up in old graves, and folks call them **grave gold**. The pier at the end of the road is still called **the Toll Gate**. *(saga)* The poor who couldn't pay waited on the pier: the first Unpaid. When the saints lit the way the ferry rotted at its mooring and the Ferryman starved. A polite buyer pays well for grave gold today (Skyreach's salvagers sell it).
- **Keepers** (new). Every saint has a keeper family. They tend the Reliquary, walk the dead to the burial boards by lantern light and sit up with them until dawn. A keeper's lantern is lit from the saint, and its flame leans toward a saint's light wherever that light is carried. On Ransom's Rest the keepers are the Ransoms. A keeper doesn't lie still while his saint is dark.
- **The Unpaid.** When a saint goes dark, every soul of its town still on the Sundown Road loses the light and drifts home to its own grave. Those are the Unpaid: cold, hungry and angry, and the living near them sicken and go strange. Ransom's Rest went dark seven days ago, and years of its dead have come home.
- **Dead hands.** No living gun can kill a Hollow, but any gun in a dead hand hurts the dead. The Unpaid know it, and they come for Ellis on sight. *(saga)* Prying an ember out of a Hollow takes a dead hand that carries a saint's light: Ellis's, and only Ellis's (*Protagonist*, the spark). Sexton's earlier Revenants (Hob) collected ordinary fares.
- **The Dunne Gang.** Six months ago the gang, eight outlaws led by a defrocked preacher, was hired by a stranger who paid in grave gold. The job was to rob seven Reliquaries in seven towns across the Reaches and take the embers. In every town they shot the keeper first and smashed his lantern, because a keeper's lantern can find an ember. Seven of the gang ended up carrying one apiece. Ellis never would: something about the embers felt wrong. Each ember changed its carrier. Six of them became **the Hollow**, a saint's power soured inside a living sinner. The Deacon fought his ember, and it's killing him instead. No living hand can kill a Hollow. It takes someone already dead.
- **The seventh job** was Ransom's Rest, Ellis's home. Ellis guided the gang in by the route in *Cold open*, 1. Ellis balked at the end. Abel came running, Ned shot him, and the Deacon shot Ellis.
- **What a lantern can't find** (new). A lantern leans toward the nearest saint's light burning in the world. Two things hide an ember from it. An ember paid to Sexton goes into his ledger, past any lantern's reach. And a Hollow who fights his ember holds its light down; only the Deacon fights his.
- **Getting around** (new; the one place for travel). The railroad carries people, freight and Ellis between the regions of the Reaches (the game's areas). Every region has a station: a depot with a platform; its station board is in *Campaign gating*. Undertakers keep black hearse cars for coffins, hitched behind the passenger car. Some are bound for **Gravesend**, the great necropolis on the Rim's highest cliffs, where the Gravewind blows hardest and where towns with no keeper send their dead. Ellis rides in the hearse car, Tilly's late father's funeral car: the living won't share a car with a corpse. On Ransom's Rest the line comes in through Stage Gap, a rail cut through a saddle in the East Ridge, from the wooded valley beyond, and the depot stands at the east end of town by the undertaker's yard. The train arrives and departs in a short shot, then a fade (step 9). Skiffs, small gas-bag airships, are rare: the Dunne Gang's is an outlaw's airship, bought with Sexton's grave gold, and it comes in where no depot sees it. A skylift from the *Lily*'s north side climbs to Skyreach.
- **Skyreach, the sky island.** North of the *Lily*'s country one island floats over the hills, and a skylift climbs to it. Nobody on Teropa knows why it floats; the player learns in Act I *(saga)*: the Gravewind is born there. It pours up through **the Well**, a hole at the island's heart, and the island rides the updraft, drifting a little further west every year. Crossroads Town grew on it from **sky salvagers** who fish the wind for grave gold the dead let fall. At dusk, through the Well, the Sundown Road itself shows: a river of lanterns walking west, and the dark gaps where seven towns' lights should be (*Skyreach, area 3*).
- **The Hanging Moon** *(saga)*. Once a season the moon hangs full at dusk and the Gravewind doesn't blow: no soul leaves, and the dead walk the orchards (some in the scarecrows: Strawmen); the towns bar their doors. The endgame's Hanging Moon nights are this.

## Protagonist: Ellis Ransom

Ellis grew up on the Ransom farm on Ransom's Rest, in the family that kept Saint Ada. At sixteen Ellis ran from that duty to ride with the Deacon, a charismatic outlaw-preacher who took in strays and called them family. Ten years of stagecoach and bank jobs later, Ellis was the youngest and the best shot in the gang, and the one the Deacon loved most.

Now Ellis is a **Revenant**: cold to the touch, no heartbeat, wounds that close, and eyes that see souls. Ellis wants three things, in the order they admit them: revenge, rest, and to make right what they helped do to the dead of Ransom's Rest, starting with Pa. The arc runs from vengeance to atonement. The story's real question isn't whether Ellis can kill seven Hollow. It's whether Ellis can forgive the one who shot them, and themself.

**The spark** *(saga; the secret until the end of Act II)*: the Deacon's gun carried Saint Ada's ember when it fired, and
a spark of her lodged in the wound. That is why the lantern can't find Ada's ember (half her light walks around in Ellis;
the Deacon holding his down is the other half), why Ellis alone can pull embers out of the Hollow, and why Sexton needs
Ellis specifically. Sexton thinks he made Ellis; he didn't make the part that matters. Ellis is a keeper by blood who
ran, and the saint came after them. Abel tells it at the boards after the new moon (*Plot*, 10). Seeded in the cold
open: the ember in the Deacon's hand flares and dims as he fires.

> *“I'm not here to kill you, Ned. You're already dead. I'm here to collect.”*

## The heroes (the pick; saga, the user's calls 2026-10-15)

- Before a new game the player picks one of five, Borderlands' way: **Ellis Ransom** the Revenant, **Odessa Lark** the
  Cardsharp, **Hollis Crane** the Unpaid, **Gauge** the Iron Hand, **Wendell Pike** the Surveyor. Looks, stories and
  three powers each: `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`, "Five heroes" and "Ember powers"; concept
  models in `Art/Backlog/Characters`. Ellis's game model is SK_Ellis. **The portraits the user picked (2026-10-19)**,
  the specs for the game models (`Art/Portraits/<Hero>.py` `OPTIONS`; sheets `Saved/ArtPreviews/Portraits/<Hero>/`):
  Ellis A The Revenant, Crane **A The Marshal**, Pike **A The Surveyor**, Gauge **C Company Iron**, Odessa **A The
  Dealer**. **The cold open is the same for every hero** (the user, 2026-10-20): whoever is picked is shot on Ransom's
  Point and raised by Sexton; each hero's earlier past (Odessa's bullet, Pike's ravine, Crane's forty years, Gauge's
  wreck) is told in their own region.
- **One story for everyone** (the user: "You take Ellis's place"). The picked hero is the Revenant Sexton raised in the
  Ransom plot; missions, areas and bosses are the same for all five. The family story is told in full only as Ellis.
  For the other four the shared text goes hero-neutral: Pa is **the Keeper** (Abel Ransom), Delia the Keeper's mother,
  the Deacon's "kid" is whoever you are, the headboard carries the hero's name, Ruth's "the Ransom kid drew" is "the
  kid drew". Each hero's own death is told in one region's lines (Odessa the *Lily*, Pike Dustwater, Gauge Halfway
  Junction, Crane the Mire). **Heroes not picked don't appear** (co-op is where they would meet, later).
- **Writing rule:** every new shared line is written without "Pa", "Ellis" or "kid"; Ellis's lines are kept as that
  hero's variants. The built Ransom's Rest text is Ellis's and stays; variants are added when the pick is built.
- **Ember powers** (the user, 2026-10-15): each hero has three (the cloud handoff's fifteen: Ellis Dust Devil, Sundown,
  Borrowed Time; Odessa Lucky Streak, Fifty-Two Pickup, Double or Nothing; Crane Toll Chain, Grave Ward, Last Rites;
  Gauge Slag Bomb, Iron Hide, Rail Spike; Pike Spyglass, Tripwire, Long Shot; numbers in
  `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`, "Ember powers" and "Five heroes"). **All three are owned from the
  start and one is equipped at a time**, Borderlands' way: one key in the fight (**Q** by default, rebindable in
  Settings), the other two swapped to on the **powers page, the inventory's second tab** after the Loadout (the Ledger
  becomes the third); the HUD's power socket sits **beside the weapon column, bottom-right** (the user's calls,
  2026-10-15 night). The outlaws' embers give no powers.
- **Build order** (the user, revised 2026-10-15): first the ember-power system with Ellis's three, playable on Ransom's
  Rest (Ellis's model exists); then the hero pick (the select screen, the four game models, their twelve powers, the
  line variants); the Rest's seeds alongside as an art-and-lines task; then the *Lily*.

## Antagonist: Mister Sexton

He is tall and patient and always polite, in an undertaker's coat and a stovepipe hat, his face in shadow even at noon. He smells like rain and keeps a ledger. He sits on the railing of Ransom's Point with his legs crossed and offers Ellis a deal: bring back the seven embers and their soul will be released to rest. Fail, and Ellis rides the Gravewind forever with no shore at the end. He adds a sweetener: *“And your father crosses. I'll see to his fare myself.”*

He was on that railing the night Ellis died. Asked, he doesn't deny it: *“I attend every death, friend. It's my trade.”* That's true, and it isn't the whole truth.

What he is: before there were saints, the dead crossed the sea of fog at the end of the Sundown Road on his ferry, and each paid the **ferryman** with a gold coin on each eye. When the saints' light laid a way across, nobody needed the ferry. He has been starving at the **Toll Gate**, the pier at the road's end, ever since. He hired the gang. Now he needs a Revenant to pry the embers back out of the Hollow so he can snuff them at the Toll Gate. With every saint dark, every soul in the Reaches would have to pay his toll again, and the ones who can't pay would serve him.

> *“Everyone pays the ferryman, friend. I'm only asking for what's owed.”*

**The second light** *(saga; Decisions 15, the user's call)*: what the first half lets the player believe is that he
collects the embers to snuff them. What Act II reveals is that he snuffs nothing. At the Toll Gate he is building a
lighthouse of his own with the stolen embers caged inside it as its lamp: a false light at the end of the world, brighter
than any saint, so every soul in the Reaches steers to his pier. The ones who can pay cross; the ones who can't serve
(the Chainman drives them up Hoarfrost Pass in chains). From the top of the Needles at dusk Lena's glass already shows
two lights where there should be one; from Gravewind Point with Grave Sight, a second light low on the horizon (*Seeds*).
"Sexton" is a trade name: a sexton digs graves.

### The Deacon, Ambrose Dunne

The gang's leader and Ellis's mentor, almost a father. He was a preacher thrown out of his church for saying heaven was a toll road, and he kept the Rest's ember, Saint Ada's, for himself. Unlike the others he didn't go Hollow. He fought it, and it's killing him instead; that fight is also why no lantern can find Saint Ada. He shot Ellis because Ellis drew on him first and he was faster. There was no secret mercy in it, and he has been sick about it every day since. He's the last name on the list and the hardest.

> *“I've buried a lot of people, kid. You're the only one who ever came back to ask me why.”*

## The Dunne Gang (the bosses)

| Outlaw | Their ember's power | Their ground | The fight |
|---|---|---|---|
| “Lucky” Ned Purcell, the gambler, who shot Abel | Fortune | The *Gilded Lily*, a paddle-wheel casino (airship or riverboat: decision 12) | His bullets ricochet toward you; the house shuffles luck, buffs and curses at random |
| “Whistling” Ira Gale, the horse thief | Wind | Dustwater Mesa, wind-scoured mesas joined by rope bridges | Rides inside a dust storm; gusts push you toward the edges |
| Sister Constance Holloway, the gang's medic | Mercy, twisted | Saint Agnes Mire, a swamp mission | Raises the Unpaid as her “flock” and heals them; kill the flock to reach her |
| Bartholomew “Barrels” Kessler, the powder man | The forge | Furnace Hollow, a smelter town | Becomes a walking furnace; the arena floods with slag |
| Lena “Spyglass” Okoro, the sniper | Sight | The Needles, stone spires and rope bridges | Sees you through walls and jumps between perches |
| Tobias “Mule” Grant, the strongman | Burden | Stoneweight Quarry, terraces and cranes | Unmovable; shakes the ground and throws stone blocks. He doesn't want to fight |
| The Deacon, Ambrose Dunne | Saint Ada's ember (from Ransom's Rest) | Gravesend, the necropolis | See Act III |

## Cast

| Name | Who they are | Bestiary (the Ledger) |
|---|---|---|
| Abel Ransom (new) | Ellis's father, the Keeper of Saint Ada. He ran up the bluff path with his lantern and shotgun the night of the robbery, and Ned shot him. His body lies in its grave, but he walks as one of the Unpaid: a keeper doesn't lie still while his saint is dark. He is Ransom's Rest's last boss (after the Foreman, since 2026-10-27). Afterwards he sits on his burial board at Gravewind Point, with new lines after each ember | Enemies (boss), then Friends |
| Tilly Bright | The undertaker's daughter on Ransom's Rest. She dressed Ellis and Abel for burial and is the first in town to talk to Ellis since (through her shop window). Knows the old rites; sells grave goods (charms, salts, consumables) once vendors exist. Her late father's funeral car, the hearse car, carries Ellis between regions on the train: the dead ride in the funeral car | Friend (vendor) |
| Hob | A one-eyed crow, Sexton's messenger, whom only Ellis can hear. Sardonic and secretly kind. He was a Revenant once; he paid his debt, and Sexton made him a crow | Friend |
| Ranger Ruth Calder | A Rim Ranger hunting the Dunne Gang. Her signature is on the wanted posters on Ransom's Rest. She watched the robbery through her glass, saw Ellis draw on the Deacon, and closed Ellis's eyes at dawn (2026-10-13). She shoots at Ellis on sight in Act I and rides beside them by Act III | Friend |
| Grandma Delia Ransom | Abel's mother. She buried her son and her grandchild on the same day, and her son got up on the Wednesday night. She won't open the door to Ellis yet, but she still sets a plate on the porch | NPC (quest giver) |
| Father Moses Aldana | The preacher of Ransom's Rest, terrified of Ellis, then the first to help | NPC |
| Amos Whitlock (new) | A farmer who died last harvest with his hay half in. He was still on the Sundown Road when the saint went dark, and drifted home to find it rotting in the field. One of the Unpaid, but not angry yet | Friend |
| Ozias Penhallow | A blind gunsmith who hears the dead in metal and rebuilds the Hollow's guns. Joins the hub after the *Gilded Lily* | NPC (vendor) |
| Thaddeus Bright (new, 2026-10-13) | Tilly's dead father, the undertaker, home from the Sundown Road to his work in her cellar (Knocking Below) | Friend |
| Hiram Coyle (new, 2026-10-13) | The Old Quarry ore lift's dead engineman, still by its boiler (Dead Lift); up the lift head's speaking tube he tells who Cyrus Teague was (Back Pay) | Friend |
| Mr. Sloane (new, 2026-10-13) | The station agent at the depot (The Old Fare; Lost and Found, which opens his LOST PROPERTY window, 2026-10-27) | — |
| Josiah Fenn (new, 2026-10-13) | A dead peddler in his coffin in the depot's baggage room, bound for Gravesend (The Old Fare) | — |
| Mrs. Pruitt (new, 2026-10-13) | The storekeeper of Pruitt's General Store, the late Sheriff Odell's sister; heard through the store's hatch | — |
| Cyrus Teague, the Foreman (new, 2026-10-13) | The Providence silver mine's foreman, company man to the bone. Eleven men barricaded the lower drift for three months' pay; his "small charge" brought the roof down on them, and he swore it was the roof. Dead, he still drives the mine's dead shift and digs at the fall every night. Abel went up every spring to ask for the eleven, and Teague ran him off company ground. Ransom's Rest's second boss and a Legendary monster (Back Pay, main since 2026-10-27, the user's note 32): put down before the keeper's night, or his dead would follow Ellis to the boards (Aldana) | Enemies |
| Odessa Lark, the Cardsharp *(saga; from the cloud heroes)* | A riverboat gambler who caught Ned dealing seconds and caught his bullet for it; Sexton bought her marker. Her death is told on the *Gilded Lily* | Playable hero (*The heroes*): the Revenant when picked, absent otherwise |
| Wendell Pike, the Surveyor *(saga)* | Walked every mile the railroad will ever run until the gang left him in a Dustwater ravine | Playable hero |
| Gauge, the Iron Hand *(saga)* | The railroad's clockwork track-layer, woken in the mail-train wreck at Halfway Junction by a spark of ember sealed in its firebox | Playable hero |
| Hollis Crane, the Unpaid *(saga)* | A marshal forty years dead, a ferryman's coin over one eye, a toll chain on his shoulder; Sexton promised him a fare. His town is Saint Agnes Mire's | Playable hero |
| Marguerite Vane *(saga)* | Skyreach's salvage boss, living and going strange, who sells the Well's grave gold to a polite buyer. Skyreach's boss | Enemies (boss) |
| Marshal Cole Bratton *(saga)* | The bounty king of Halfway Junction, paid in grave gold; his deputies are the first Mourners | Enemies (boss) |
| The Chainman *(saga)* | A Mourner lord who herds the Unpaid who can't pay up Hoarfrost Pass on a toll chain. Hoarfrost's boss | Enemies (boss) |
| Maren Oriel *(saga)* | The last keeper standing: old Saint Oriel's granddaughter in her watch-house on Hoarfrost Pass | Friend |

## Enemies and creatures

- **The Unpaid**: the restless dead of the robbed towns. They are pale shapes in the clothes they died in, fading below the waist into a trailing shroud, with a dull red coal where the heart should be. They drift, phase through walls and shriek. Ellis sees them clearly; the living barely do. **Dead miners** (2026-10-13) are a kind of the Unpaid in the Providence mine: miners' gear, head lamps, picks. **The Bell-Ringer** (2026-10-12) is the chapel's old ringer, an Unpaid who tolls a bronze hand bell to rally the dead and hangs back behind them (`Docs/Areas/RansomsRest.md`).
- **Ranks**: some creatures and dead have fed longer on the dark. Ellis sees it as soul-light, on the living and the dead: the Restless glow blue, the Gravebound purple, the Hallowed saint-gold, the rare Ethereal red, the Soulfed and the named horrors orange. An Unpaid's coal burns in the same color. Brighter souls hit harder and are likelier to carry soul-forged iron, which is why a legendary can drop from anything but comes far more often from Rares, Epics, Legendaries and bosses (rules: *Enemy ranks and legendary drops*).
- **Each outlaw's following**, the living, slowly going Hollow: Ned's card sharps and bouncers, Ira's dust riders, Constance's flock of zealots, Barrels's furnace men, Lena's spotters, Tobias's quarry crew. One rig set, seven costumes.
- **Bounty hunters**: there's a price on the walking corpse, and the living come to claim it in Act II.
- **The Mourners**: Sexton's servants from the end of Act II, murders of crows that fold into figures in black.
- **Creatures**, drawn to the dead:
  - Meadow Wolf spiders nest in graveyards and hoard anything a saint has touched: the Rest's chapel yard, the bluff and the Sink. The Gravemother is the legendary spider of Ransom's Rest, with a side mission of her own. **Venom Spitters** (2026-10-12) are a thin-legged, olive spider kind that hangs back and spits from a glowing venom sac.
  - Meadow slimes live in the creek on Ransom's Rest as plain wildlife.
  - MossBacks in Saint Agnes Mire.
  - Ember Tarantulas in Furnace Hollow's slag.
  - Cliff Huntsmen on the Needles.
  - Sky Jumpers in the *Gilded Lily*'s rigging.
  - **Bone Hounds** (coyotes that raid the burial boards on nights when no keeper sits up; with the keepers shot, there are more of them every month) and **Carrion Gulls**.
- **More kinds by region** (rats, goblins, grave robbers, golems, wolves, giants, scorpions, bats, vampires, dragons, big
  insects): *Creature roster*.

## Creature roster (the user's ask, 2026-10-21)

The user: more enemies and creatures as the game goes on, the story reading as having "Goblins, Rats, Human Bandits,
Golems, Wolves, Giants, Scorpions, Big Flying Bats, Vampires, Dragons, Big Insects and More" ("subject to change"); where
the story gives a reason for a different enemy in an area, a placeholder blob of clay with its name. Additions only: no
story enemy, boss or beat replaced. The core-loop freeze holds: kinds of unbuilt regions wait for their regions.
- **Placeholders** (`APlaceholderCreature`, Pipeline/Enemies.md "Placeholder creatures"): a clay dome stretched to the kind's size, a
  clay head (the crit), its tag "Name (plain type)" and its bestiary page named the same, marked placeholder (the user's
  call, 2026-10-22). They fight on today's systems (director turns and ring, ranks, levels, traits, gunfeel) and pay kill XP
  and loot as real enemies by level, rank and role (the user's call, 2026-10-22). Each is swapped for a real model later.
- Status: **placed** = in Ransom's Rest now (`Tools/Unreal/build_area_placeholders.py`, places and counts in
  `Saved/Roster/PLAN.md`); **kind** = its definition, class and page can be made (`create_placeholder_creatures.py all`),
  placed when its region is built. Marked † in the *Areas* table.

| Name (plain type) | Asked for | Regions | Role | Why it's there | Status |
|---|---|---|---|---|---|
| Coffin Rat (rat) | Rats | Ransom's Rest (Tilly's cellar, the depot's baggage room); Halfway Junction's yards; Gravesend's hearse cars | swarm | The dead wait in their boxes for the boards; with no keepers they wait longer, and the rats grow fat on them | placed |
| Bone Hound (coyote) | Wolves | Ransom's Rest (the keeper's road, after Main 4, until Abel keeps his board after Main 6); Dustwater Mesa; the Sundown Road | melee pack | The story's own: they raid burial boards no keeper sits up with | placed |
| Whiteback Wolf (wolf) | Wolves | Hoarfrost Pass | melee pack | Follow the Chainman's columns up the pass and take the chained dead who fall | kind |
| Knocker (goblin) | Goblins | Ransom's Rest (the Bench's ore tramway, after Dead Lift); Furnace Hollow's mines; Stoneweight Quarry | swarm | Miners' folklore: little grey people who knock on the timbers before a fall. They knocked all the night before Teague sent the eleven down; with the saint dark they stopped warning | placed |
| Gravepicker (bandit), Gravepicker Lookout (bandit rifleman) | Human bandits | Ransom's Rest (Boot Hill, after Main 3, gone once the dead walk it after Main 4); any town with old graves; Gravesend | melee; ranged | Living grave robbers, up through Stage Gap for the grave gold in graves nobody keeps now; they sell it to a polite buyer (a seed: Sexton buys the old fare). The other living foes stay as written: the outlaws' followings, bounty hunters | placed |
| Slagback (golem) | Golems | Furnace Hollow | big | Slag from Saint Maud's forge stood up by Barrels's forge ember; it walks while it's hot | kind |
| Cairn Walker (stone golem) | Golems | Stoneweight Quarry | big | Cut blocks made heavy enough to walk by Tobias's ember of burden | kind |
| Spikedriver (clockwork golem) | More | Halfway Junction (the mail-train wreck, the yards) | big | The railroad's clockwork track-layers, woken by the sparks that woke Gauge, without its mind | kind |
| Pallbearer (giant) | Giants | Hoarfrost Pass; Gravesend; the Toll Gate | big | Giants in mourning black, Sexton's oldest servants from the ferry's days, who carry the unclaimed dead to his gate | kind |
| Mesa Stinger (scorpion) | Scorpions | Dustwater Mesa | melee | Nest in Saint Ezra's cistern, dry since the saint went dark | kind |
| Gloam Bat (giant bat) | Big flying bats | The Needles' spire caves; Gravesend's mausoleums; the Gravesend Deeps | flyer | Roost where the Gravewind never reaches and drink the soul-light off the dying | kind |
| Sallow (vampire) | Vampires | Saint Agnes Mire; Hanging Moon nights | melee, fast | Constance's night congregation: living folk who drink the Unpaid's coals to stay young ("communion"); the dawn wind frightens them | kind |
| Cinder Drake (dragon) | Dragons | Furnace Hollow's chimneys | flyer | Slag-glass drakes hatched where an ember spark fell in the forge; their mother, Old Bellows, in the blast furnace (a Legendary monster idea) | kind |
| Sexton Beetle (giant insect) | Big insects | Gravesend's bone gardens; Saint Agnes Mire | big | Burying beetles the size of carts that drag the unclaimed dead underground; the towns named them for the gravedigger (a fair-play nod) | kind |
| Bog Mantis (giant insect) | Big insects | Saint Agnes Mire | melee | Waits in the reeds as still as a cross | kind |
| Rimrock Rattler (giant snake) | More | Dustwater Mesa; the Needles | melee | Rattlers as long as a wagon in the wind-cut rock | kind |
| Dust Wight (wind spirit) | More | Dustwater Mesa | ranged | Dustwater's dead caught in Ira's wind, throwing grit | kind |
| Strawman (scarecrow) | More | Ransom's Rest on Hanging Moon nights (endgame) | melee | The dead walk the orchards when the wind doesn't blow, some in the scarecrows | kind |
| Carrion Gull (gull); MossBack (swamp turtle); Ember Tarantula, Cliff Huntsman (spider); Sky Jumper (jumping spider) | The story's own, unmodelled | Their regions (*Enemies and creatures*) | flyer; big; melee; melee; flyer | As written | kind |

- **Legendary monster ideas** (no placeholder yet): Old Bellows (the great cinder drake, Furnace Hollow), the Wellwyrm
  (Skyreach: rides the Gravewind up the Well and swallows the grave gold the salvagers fish for), the Tangle (a rat king of
  coffin rats knotted together, the Gravesend Deeps).

## Skyreach, area 3

The user's call (2026-10-13): Skyreach becomes the story's third area.
- In the game it is `/Game/Maps/Lvl_TutorialIsland`, called Skyreach in its text, with Crossroads Town. It is not the old sky-islands map (`Lvl_Skyreach`), which stays only for reference. It stays the main menu's backdrop.
- Teropa's one sky island, reached from the north side of the *Gilded Lily* by a skylift. Opens after a mission on the *Lily* (“The Salvage Line”, to design). Hob comes along.
- Its story *(saga)*: the Gravewind is born in the Well at its heart and the island rides the updraft (*The world*). Crossroads Town is a sky-salvagers' town fishing grave gold from the wind. Marguerite Vane, the salvage boss, has been selling it to a polite buyer; she is the boss, living and going strange, and Sexton's crows gather for the first time. At the Well, with the Keeper's Lantern, Ellis sees the Sundown Road: thousands of lights walking west and seven dark gaps. Band 12–16. Legendary: Downdraft (a shotgun whose blast lifts). Mission name: “What the Wind Carries”.
- Gone: the tutorial (the controls are taught by hints in Mains 1–3), “Skip the tutorial”, the skiff exit and the cloud match cut, practice trips, 0 XP and ammo-only drops on return visits.
- Until its rework: `DA_Area_Skyreach` is still marked practice (`UAreaDefinition::bPractice`: no kill XP, no guns after the first cast-off), no station board lists it, `Looter.Travel Skyreach` goes there for testing, and it keeps today's house set, its existing respawn and fall recovery (no roosts, no Hob).

### The tutorial island, outside the story (retired 2026-10-13)

Retired; the old text is in the archive.

### Leaving the tutorial island (retired 2026-10-13)

Retired for new games. What remains: the packet skiff (`Art/Models/Vehicles/Skiff.py`) and Skyreach's jetty (`ASkiffJetty`), whose board gives a session begun there before 2026-10-13 one trip to the story's first arrival. The old text is in the archive.

### Back to Skyreach for practice (retired 2026-10-13)

Retired; no board lists Skyreach. The old text is in the archive.

## Ransom's Rest, the first area (summary)

**The place** (on solid ground; ground and travel in *The world*):
- A high farm valley on the Rim, about 309 × 350 m and 64,000 m², with Larkspur Bench, Calder's Post and the Providence mine up the ore lift; one level.
- The gang's hidden mooring is a ledge in a cleft of the canyon under Ransom's Point. The burial boards stand on Gravewind Point over the canyon. Mill Creek falls 14 m into a gorge through the Hogback.
- Cliffs and obstacles break up the open ground: Coffin Rock (a lone butte), the Dry Wash, rock outcrops such as the Three Widows and the Stone Teeth, the Tumble's rockfall, Widow's Pines, ruins, boulders, stone walls and fences. Outside the zones no open stretch is wider than about 19 m, except the crossroads at the keeper's grave, kept clear on purpose.
- Inside a fight's ground they are only cover, so every fight stays on one level of ground.
- Placeholders (*Creature roster*): coffin rats in Tilly's cellar and the depot's baggage room, Bone Hounds on the keeper's road, Knockers at the Bench's ore tramway, Gravepickers digging Boot Hill between Main 3 and Main 4.

The main missions in order (full plan: `Docs/Areas/RansomsRest.md`; givers, side missions and their lines: `Docs/Areas/RansomsRest_Expansion.md`):

1. **Seven Days.** The cold open, the grave, Hob, and Delia through the screen door.
2. **Shall We Talk Business?** Up the bluff to Ransom's Point, the lookout where Ellis died, about 15 minutes in. The first ranked enemy, a blue Restless spider, nests by the lookout. Sexton's deal comes with the Ledger of seven names: bring each ember to this lookout at the new moon. Each one paid returns a little of Ellis's life (warmth, color, a heartbeat for a minute). Bring all seven and Ellis rests, *“and your father crosses.”* The gang scattered, so Ellis asks where to start. Sexton: *“Ask a keeper. Their lanterns lean toward a saint's light.”* *“The keeper's dead.”* *“So are you, friend.”*
3. **Cold Welcome.** Into town. The Unpaid come for the corpse at the gate. Tilly, at her window, says Pa's lantern wasn't on him when she laid him out.
4. **Hallowed Ground.** The chapel yard, the bell and the smashed Reliquary. Aldana says the gang shot the keeper first in every town and smashed his lantern. Abel's fell in the dark, whole, and spiders hoard what a saint has touched: look in the Sink.
5. **The Keeper's Lantern.** Down into the Sink for it. The altar candle won't light it, and Aldana warns: not yet, with Cyrus Teague still driving his dead shift in the Providence, or they'd follow Ellis to the boards at dusk.
5a. **Dead Lift** (main since 2026-10-27, the user's note 32). Hiram Coyle's ore lift up Larkspur Ridge: its clamp pin back from Abe Haskett's crew, steam raised, the first ride.
5b. **Back Pay** (main since 2026-10-27). Hiram, up the speaking tube, tells who Teague was; down into the Providence, the payroll book's "small charge", the eleven's tags, **Cyrus Teague, the Foreman**, the second boss; the choice at the tag board (pay the eleven or keep the payroll). Hob: "Now the Keeper, sunshine."
6. **The Gravewind.** At dusk Ellis carries the lantern to the burial boards. Pa is the boss. Afterwards he lights the lantern from the ghost light he carries, and its flame leans north-east, over the ridges, toward Ned.
7. **The Lantern Leans.** Delia hands Heirloom, Pa's shotgun, out through the door. Tilly's hearse car waits at the depot, hitched to the train, and the station board lists the *Gilded Lily*. (The train stands at the platform from the start, cold and shut; from this mission it has steam up and the hearse car's door is open.)

## Campaign gating (new)

Areas open when story missions are done. No area, mission or gun ever checks the player's level.

**In the story.** The Keeper's Lantern leans toward the nearest saint's light still burning in the world (*What a lantern can't find*).
- When Abel lights it, it leans toward Ned's ember on the *Gilded Lily*.
- When an ember is paid to Sexton at the new moon, it goes into his ledger, out of the lantern's reach, and the flame swings to the next one.
- Sexton's Ledger records each bearing, and the station board lists a story place only once the lantern has found it. The next place exists for Ellis exactly when the story points there.
- Saint Ada's own ember should pull hardest of all, but the Deacon is holding its light down. At the end of Act II he starts losing that fight, and for the first time the flame finds her at Gravesend.
- A fair-play seed: the flame never once leans toward a paid ember, and Ellis notices.

**In the game (new):**
- Each area is a `UAreaDefinition` data asset: its map, display name, enemy level band, the mission that opens it, its station and its rank promotion chances.
- The session save records completed missions, the active mission and step, opened areas, first boss defeats, and each map's world state (Skyreach's included).
- Every area has a station: a depot with a platform. Its station board, a LooterUI panel, lists every opened area, so travel can't strand the player, plus one blank line naming the mission that opens the next (for example “Finish ‘The Lantern Leans’”). Later areas aren't shown. Coming in at a station, or loading a session, is an arrival.
- Side areas can open from side missions later, for example the wreck of the *Gilded Lily* for “The House Always Wins”.

**What levels still mean: difficulty, XP and loot level.** Numbers, tables and the Borderlands research: `Docs/Progression.md`.
- **Enemy levels.** Enemies don't scale with the player (the user's call, 2026-10-12): an enemy's level is where it stands. Every area has an enemy level band and zones (its layout's `gameplay.levels`); a creature takes a level rolled between its zone's two levels, then plus its rank's offset. Bosses and Legendary monsters have set levels (the Gravemother 6, the Foreman 6, Abel 7; the Foreman was 7 until 2026-10-27, when he became the story's second boss and Abel its last). A player who rushes the story meets the same levels as one who comes back strong. `Docs/Progression.md`, "Creature levels", has the zones.
- **Health and damage.** Enemy health and damage grow linearly, ×(1 + 0.08 × (level − 1)). That is the rule guns use (`UWeaponDefinition::DamagePerLevel` 0.08), so a gun of the enemy's level always needs the same number of hits.
- **Loot level.** Dropped guns take the enemy's level.
- **Player health.** +8% max health per level (`HealthPerLevel` 0.08), so the player keeps pace with the enemies' damage growth.
- **The curve** (the user's call, 2026-10-13: like Borderlands): total XP to reach level L = 180 × (L^2.8 − 1), Borderlands 2's shape at three times the scale. The top level is **65**.
- **Kill XP** = the creature's `XPReward` (10 for a level 1 Basic) × its level^1.2 × the rank's XP multiplier. Below the player's level it falls 15 points a level (85% one level below), down to 10%, so farming old areas isn't the fast road.
- **Mission XP** is a fixed amount per mission, given only when the mission is turned in to its giver (Borderlands' way), never for its steps. Only kills and turn-ins give XP. Ransom's Rest ends about level 6.9 on the story alone, about 7.5 with everything (since the mine joined the story, 2026-10-27; a player arrives near each boss's level).
- **After the campaign** every area's band rises to follow the player to 65. Open: this clashes with the 2026-10-12 rule above, and waits for the user's endgame call.

## Enemy ranks and legendary drops (new)

Legendary weapons can drop from anything that can die: rarer from basic creatures, increasingly common from Rare, Epic and Legendary monsters and bosses. Built on what exists:
- `ULootTable`'s `WeaponDropChance`, `Min/MaxWeaponDrops` and `Luck`.
- Each gun's rarity weights, 60/25/10/4/1.
- The luck rule in `UWeaponRollLibrary::RollRarityWith`, which multiplies rarity tier *n*'s weight by (1 + Luck)^*n*.

The user's rule that Common is the most likely rarity and each tier up is rarer holds at every rank. Luck above 1.4 would make Uncommon guns more likely than Common ones (and Epic passes Common near 1.5), so the tables stop at Luck 1.3 and give the top ranks more guns instead.

**Rarer early** (the user, 2026-10-27: legendaries were "super easy to get"; their call "Rarer, bosses still drop"): a gun of level 8 or under (Ransom's Rest: its creatures 1–6 before their ranks' +1/+2 levels, its chests kept in its band; the *Lily* starts at 9) is legendary at most 1 time in 300 (`ULootLibrary::LegendaryCapFor`; cvars `Looter.Loot.EarlyLegendary.Share` 1/300, `.MaxLevel` 8; the weight taken off goes to the other rarities in their proportions). Tables that keep their odds everywhere (`ULootTable::bKeepsLegendaryOdds`): Soulfed (the Gravemother, the Foreman) and Boss (Abel), set by `create_rank_assets.py`, and a bounty's (its mark's table copied by `ULootLibrary::KeepingLegendaryOdds`, its reward's flagged). From level 9 every table has the odds in the last column. Mission rewards and set rarities aren't rolled and don't change.

**Five tiers** for every regular enemy (the user's ask and approval, 2026-10-20; the design with every number: `Saved/Tiers/DESIGN.md`, kept in `UCreatureRankSettings` for tuning): Regular, Restless, Gravebound, Hallowed, Ethereal, each harder, better paid and rarer; Soulfed monsters and bosses stand above them. Code names stay Basic/Rare/Epic; Hallowed and Ethereal were appended to `ECreatureRank` after Boss (saved numbers keep their meaning); order only through `LooterRanks` (`IsAbove`, `Order`).

| Rank | Word and tag | Where it comes from | Stats, on top of level | Loot table | Legendary per kill: Ransom's Rest (level 8 and under) / later |
|---|---|---|---|---|---|
| Basic (Regular) | No word; the name tag as today | Default | ×1 | `DA_LootTable_Default` (unchanged, keeps its name): a gun on 30% of kills, 1 gun, Luck 0, ammo 1–2 | 0.1% (1 in 1,000) / 0.3% (1 in 333) |
| Rare (Restless) | “Restless”, blue (#7CAAFF) | 8% of regular spawns on Ransom's Rest, 12% by the late areas; no cap; plus one placed in most scripted fights | Health ×2.0, defense 10%, damage ×1.15, XP ×2, silver as before the tiers, size ×1.08, +1 level | `DA_LootTable_Rare`: 60%, 1 gun, Luck 0.5, ammo 2–3 | 0.2% (1 in 500) / 2.2% (1 in 45) |
| Epic (Gravebound) | “Gravebound”, purple (#CC7CF4) | 2% → 4%; at most 4 alive per area; plus placed ones | Health ×3.0, defense 20%, damage ×1.3, XP ×3.5, silver as before the tiers, size ×1.15, +1 level, 1 trait | `DA_LootTable_Epic`: 90%, 1 gun, Luck 0.8, ammo 2–4 | 0.3% (1 in 333) / 5.5% (1 in 18) |
| Hallowed | “Hallowed”, saint-gold yellow (#F5C842) | 0.4% (1 in 250) → 1%; at most 2 alive | Health ×3.5, defense 28%, damage ×1.45, XP ×6, silver ×1.6 a Gravebound kill's, size ×1.22, +2 levels, 2 traits | `DA_LootTable_Hallowed`: every kill, 1–2 guns, Luck 1.1, ammo 3–5 | 0.5% (1 in 200) / 13.3% (1 in 7.5) |
| Ethereal | “Ethereal”, red (#FF3B3B) | 0.08% (1 in 1,250) → 0.25% (1 in 400); at most 1 alive | Health ×4.0, defense 35%, damage ×1.6, XP ×9, silver ×2.5 a Gravebound kill's, size ×1.3, +2 levels, 3 traits (Phasing always one) | `DA_LootTable_Ethereal`: every kill, 2 guns, Luck 1.3, ammo 4–6 | 0.67% (1 in 150) / 21.3% (1 in 4.7) |
| Legendary | A name, or “Soulfed”; legendary orange (#FF9900, apart from Hallowed's yellow) | Hand-placed as an area's design names them (Ransom's Rest: the Gravemother and the Foreman); 0.5% of spawns later, at most one random Soulfed alive per area | Health ×12, damage ×1.6, XP ×10, size ×1.4–1.8, +2 levels, a lair and one unique move | `DA_LootTable_Legendary`: every kill, 2 guns, Luck 1.3, keeps its odds | 21.4% (about 1 in 5) everywhere |
| Boss | Orange (#FF9900), plus a boss bar | Story fights | Health set per boss, XP ×20, +1 level | `DA_LootTable_Boss`: every kill, 3 guns, Luck 1.3, keeps its odds | 30.3% (about 1 in 3) everywhere |
| Bounty mark | Its tier's (Gravebound, Hallowed, Ethereal) | The bounty board's fights | Its tier's | Its tier's table, copied to keep its odds | Its tier's later odds everywhere (5.5% / 13.3% / 21.3%) |

- **Defense** takes its share of every hit (bullets, blows, grenades, ember powers: `UHealthComponent`, once); a crit pierces half of it (the user's call). Effective health against a Regular one at the valley's level 4: 2.4, 4.0, 5.5, 7.0 up the tiers (Soulfed about 13.6).
- **Spawn odds** rise with the area's place in the campaign, read from its creatures' lowest level: 1 (Ransom's Rest) gives the first numbers, 50 and up the late ones, straight between (the Lily, from 9: 8/49 of the way). They roll for placed Basic creatures on arrival and for spawners rolling by area; a roll past a tier's cap of living ones steps down a tier. Areas' `RarePromotionChance`/`EpicPromotionChance` now only switch promotions on (Skyreach off).
- **Traits** (Gravebound 1, Hallowed 2, Ethereal 3): Fleet, Mending, Warded, Grave-burst, Rallying, Vengeful, Phasing (Pipeline/Enemies.md "Enemy tiers" has what each does), rolled from a seed of the creature's name in its level and its promotion roll, so a load rebuilds them with nothing saved.
- **Silver** a tier's kill gives: a Regular kill's × its multiplier on average, dropping more often (`LooterEconomy::ScaledKillOdds`); Soulfed and bosses keep their own.
- **The tag**: word and name both in the tier's colour. Tier-only colours (the user's call): yellow and red are no gun rarity's; blue and purple happen to match the Rare and Epic beams.

Rarity of each dropped gun at each luck (Common / Uncommon / Rare / Epic / Legendary), level 9 and up and the tables that keep their odds (on Ransom's Rest the others' Legendary is 0.33% at every luck):

| Luck | Used by | Shares |
|---|---|---|
| 0 | Regular | 60 / 25 / 10 / 4 / 1 % |
| 0.5 | Restless, Supply Crate | 43 / 27 / 16 / 9.7 / 3.7 % |
| 0.8 | Gravebound | 35 / 26 / 19 / 14 / 6.1 % |
| 1.0 | Strongbox | 30 / 25 / 20 / 16 / 8.1 % |
| 1.1 | Hallowed | 28 / 25 / 21 / 17 / 9.1 % |
| 1.3 | Ethereal, Legendary, Boss | 24 / 23 / 21 / 20 / 11.3 % |

**Rules.**
- **Anything means anything.** Every enemy that can die, including boss adds, spiderlings and egg-sac spiders, uses at least the Basic table, so any kill can drop a legendary. The one exception: a practice area after the first cast-off (Skyreach, until its rework) drops only ammo.
- **Ranks show in soul-light.** Name tags show the tier's colour (above), and an Unpaid's coal burns in it (a basic one's is dull red). Bosses also get a slim slanted bar at the top of the HUD.
- **Promotions** are rolled on each arrival in an area. A promoted creature that dies comes back as Basic.
- **No reload farming.** Each map saves when its promotions were last rolled and when its Legendary monster last died, counted in time played. An arrival rerolls promotions and brings the Legendary monster back only after 20 minutes of play, so Save & Quit then Continue doesn't reroll them.
- **Ammo** keeps the user's rule: every kill, leaning 2× to the kill gun's class. Ranked enemies drop more boxes through `Min/MaxAmmoDrops`: Regular 1–2, Restless 2–3, Gravebound 2–4, Hallowed 3–5, Ethereal and Legendary 4–6, Boss 8–10.
- **Signature legendaries** (from the *Gilded Lily* on). On top of the boss table, each Hollow boss carries its soul-forged legendary (Dead Man's Hand, Whistler and so on): a separate roll after the boss table, guaranteed on the first defeat (or always 15%; the user's call when the *Lily* starts) and 15% on refights. Needs new `ULootTable` fields (the signature weapon, its chance, a first-defeat guarantee) and first defeats saved in the session. Ransom's Rest has no signature gun; its first named gun is Heirloom.
- **Refights.** Anything with a page in Sexton's Ledger can be called back to account, before the ending and after it. Hollow bosses and Legendary monsters come back on a later arrival (after the 20 minutes), and the endgame's Ledger bounties work the same way. Abel is fought once; he becomes a friend.
- **Tests and tools.**
  - `Looter.Loot.RankOdds` simulates 200,000 kills per rank with a fixed seed: the legendary rate rises up the tiers (and from Soulfed to Boss), sits within 10% of the table, and every rank table's rarity shares fall from Common to Legendary.
  - `Looter.Tiers.*`: the order, defense with and without crits, stats by tier, spawn odds and caps, traits, a save round trip.
  - `Looter.Loot.RankOdds` reads the tables' own odds (no level); `Looter.Loot.EarlyLegendary.Rules` (the cap's arithmetic, where it applies, uncapped rolls drawn as before, a bounty's copy) and `.Odds` (every rank and chest kind on Ransom's Rest and later; Soulfed and Boss flagged) the early cap.
  - `Looter.Loot.SimulateDrops <rank> [kills] [level, 1 by default]` and `Looter.Tier.Odds [kills]` print the counts (level 1: Regular about 0.10%, Restless 0.20%, Gravebound 0.30%, Hallowed 0.50%, Ethereal 0.67%, Soulfed 21.4%, Boss 30.3%; level 9: the later column).
  - The DropOdds, DefaultTable and WeaponRarityOrder tests stay.

**Chests** (in the game since step 26, the user's OK 2026-10-07): a Supply Crate gives 1 gun at Luck 0.5, a Strongbox 2 guns at Luck 1.0, and a saint's Reliquary opens after each Hollow falls. A legendary from one on Ransom's Rest / later: Strongbox 0.67% (1 in 150) / 15.5%, Supply Crate 0.33% / 3.7%, Footlocker 0.12% / 0.73%, Coffin 0.05% / 0.27%, Grave 0.04% / 0.22%, Mailbox 0.03% / 0.08%. A bounty's reward box keeps its odds.

## Areas

| Area | Opens after | Enemy levels | What it is | Enemies | Boss |
|---|---|---|---|---|---|
| Ransom's Rest (home and hub) | A new game | 1–7 | Home: a high farm valley on the Rim, over Gravewind Canyon. The farm and family plot, the town and Main Street, the chapel, the Sink, the burial boards; up the ore lift, Larkspur Bench, Calder's Post and the Providence mine | The Unpaid, dead miners, spiders, slimes; roster†: Coffin Rats, Bone Hounds, Knockers, Gravepickers | Cyrus Teague, the Foreman (Back Pay, the second), then Abel Ransom, the Keeper (the last). Legendary monster: the Gravemother (Side 3) |
| The *Gilded Lily* | “The Lantern Leans” (Abel lights the lantern; it leans toward Ned) | 8–13 | A paddle-wheel casino airship tethered over the canyon's morning cloud at **Highwater Landing** *(saga)*, a rim boomtown where the line ends (its saint **Saint Hester**, who staked the landing): card rooms, a ballroom, gondolas, rigging (an airship: the user's call, 2026-10-15, closing decision 12) | Card sharps, bouncers, Sky Jumpers; roster†: Carrion Gulls | Lucky Ned (Odessa Lark's own death, played as her) |
| Skyreach | “The Salvage Line” on the *Lily* (to design), by its skylift | 12–16 | Teropa's one sky island: Crossroads Town, the salvage rigs, the Well where the Gravewind is born (*Skyreach, area 3*) | Sky salvagers going strange, Sky Jumpers, the first Mourners; roster†: Carrion Gulls | Marguerite Vane *(saga)* |
| Dustwater Mesa | “The House Always Loses” (Ned's ember paid at the new moon; the lantern swings to Ira) | 15–20 | Wind-scoured mesas and rope bridges; the wind is part of every fight. Its saint **Saint Ezra**, who dug the cistern | Dust riders, Bone Hounds; roster†: Mesa Stingers, Rimrock Rattlers, Dust Wights | Whistling Ira (Wendell Pike's ravine, played as him) |
| Halfway Junction *(saga)* | “Back to Back” (Ruth's truce; it ends with Ira's ember paid) | 19–24 | The hub where every line of the Reaches meets: yards, a roundhouse, a hotel full of bounty hunters, the wreck of the mail train the gang derailed. Its board opens the whole network | Bounty hunters, the Mourners as deputies; roster†: Spikedrivers, Coffin Rats, Gravepickers | Marshal Cole Bratton (legendary Due Process; Gauge's wreck, played as it) |
| Saint Agnes Mire | “Dead or Alive” (Bratton down; the Junction's board) | 23–29 | Swamp country in the river delta: a mission church, cypress, fog and black water. Its saint **Saint Agnes**, who built the mission | The flock, the Unpaid, MossBacks; roster†: Sallow, Bog Mantises, Sexton Beetles | Sister Constance (Hollis Crane's dark town, played as him) |
| Furnace Hollow | “And They Shall Rise” (Constance's ember paid) | 28–33 | A smelter town: chimneys, slag rivers, ore carts; the town's saint, **Saint Maud**, was a smith and her Reliquary is cast iron *(saga)* | Furnace men, Ember Tarantulas; roster†: Slagbacks, Cinder Drakes, Knockers | Barrels Kessler |
| The Needles | “Slag” (Barrels's ember paid) | 32–37 | Stone spires joined by bridges, a sniper's dream (its saint **Saint Corliss**, who strung the first bridge); from the top spire at dusk, two lights at the end of the world *(saga)* | Spotters, Cliff Huntsmen; roster†: Gloam Bats, Rimrock Rattlers | Lena Okoro |
| Stoneweight Quarry | “Feathers” (Hob's confession after Lena; it ends with Lena's ember paid) | 36–42 | A terraced quarry, cranes, stone blocks hanging over the drop. Its saint **Saint Gideon**, who cut the first terrace | Quarry crew, bounty hunters; roster†: Cairn Walkers, Knockers | Tobias Grant |
| Ransom's Rest at the new moon | “The Sixth Ember” (Tobias's ember in Ellis's hand) | 41–42 | The hub at night, grey and cold | The Mourners | None: Sexton's betrayal |
| Hoarfrost Pass *(saga)* | “The New Moon” (the lantern finds Saint Ada for the first time) | 42–45 | The climb to Gravesend over the Rim's snowline, where the Gravewind is a blizzard: frozen dead who never reached the road, ice-bound hearses, Maren Oriel's watch-house | The frozen Unpaid, the Mourners, the Chainman's columns; roster†: Whiteback Wolves, Pallbearers | The Chainman (legendary Hobble) |
| Gravesend | “The Chain” (the Chainman down) | 45–48 | The great necropolis on the Rim's highest cliffs, where the Gravewind blows hardest and towns with no keeper send their dead in hearse cars on the train: mausoleums, burial boards, bone gardens | The Unpaid, the Mourners; roster†: Pallbearers, Sexton Beetles, Gloam Bats, Coffin Rats, Gravepickers | The Hollow Deacon |
| The Sundown Road *(saga)* | “The Kid Who Came Home” (Saint Ada's ember in Ellis's hand). Off Gravewind Point at dusk, riding the Gravewind west, not by train | 48–50 | The road itself: a river of lanterns walking west, the Bone Hounds' crossing, the Chainman's columns, the fog sea's edge, the false lighthouse growing until it fills the sky. Ellis carries the only true light | The Mourner swarms, Bone Hounds | No boss (legendary Lamplighter from the swarms) |
| The Toll Gate | The road's end | 50 | Where the road leaves the world: a grey pier on a sea of fog under a sun that never quite sets, the ferry rotting at its mooring, the dead in line, the Ferryman's lighthouse with six embers caged in its lamp. On no map of Teropa | Everything Sexton owns; roster†: Pallbearers | Mister Sexton: six phases, an ember freed in each, the false light failing; then the man himself |
| Endgame | Finishing the campaign | 50–65 (every band follows the player; open, see *What levels still mean*) | The Ledger's bounties, the Gravesend Deeps, the wreck of the *Gilded Lily*, Hanging Moon nights on Ransom's Rest, the Midnight Stage | Everything; roster†: Strawmen and the Sallow on Hanging Moon nights, Gloam Bats in the Deeps | Named Hollow bounties with rolled traits |

## Plot

### Act I: Dust to Dust

1. **Ransom's Rest.** The cold open, the grave, Sexton's deal on Ransom's Point about fifteen minutes in, the town, the dark chapel, the lantern in the Sink, and Pa at the burial boards (*Ransom's Rest, the first area*). Abel lights the Keeper's Lantern from the ghost light he carries, and its flame leans north-east, over the ridges, toward Ned.
2. **The *Gilded Lily*.** Ned Purcell, the man who shot Abel, runs a floating casino where the house always wins. The house loses. His ember goes to Sexton at the new moon on Ransom's Point, and Ellis feels the sun on their skin for the first time since dying. The flame swings toward Dustwater, and never once toward Sexton. Ellis: *“Where'd Ned's go?”* Hob: *“Paid's paid, sunshine.”* *(saga)* Played as Odessa Lark, this is where your own death at Ned's table is told.
2a. **Skyreach** *(saga)*. By skylift from the *Lily*'s north side, between Ned and the new moon. The salvagers' Well, where the wind is born; Marguerite Vane selling grave gold to a polite buyer; the first Mourners. At the Well the Keeper's Lantern shows the Sundown Road: thousands of lights walking west, seven dark gaps. The embers are the way home for everyone.
3. **Dustwater Mesa.** Ira Gale, inside his storm; stingers nest in Saint Ezra's dry cistern. Dying, he coughs: *“He paid us in grave gold, Ellis. The old kind they used to lay on dead men's eyes.”* *(saga)* Played as Wendell Pike, the ravine the gang left you in is here.
4. Ranger Ruth Calder ambushes Ellis outside Dustwater: dead or not, Ellis rode with the Dunne Gang. Then the Unpaid pour out of the dust and the two of them fight back to back. It's an uneasy truce. At the new moon Ruth watches from the bluff path while Ellis pays Ira's ember, and the flame swings toward the Mire.

### Act II: The Unpaid

4a. **Halfway Junction** *(saga)*. The price on the walking corpse: Marshal Cole Bratton's bounty hunters hold the rail hub, and his deputies are men in black who are crows. Played as Gauge, the wreck of the mail train the gang derailed is where you woke. Bratton down, the Junction's board opens the whole network.
5. **Saint Agnes Mire.** Sister Constance raises the dead because she thinks she's helping them, since they can't cross; her night congregation, the Sallow, drink the flock's coals. Before the end she tells Ellis: *“The saints don't hold souls back, child. They light the way. Put them out and the dead can't find the shore. And somebody's waiting at the gate to sell them directions.”* *(saga)* Played as Hollis Crane, the drowned mission is your own town, forty years dark, and the coin over your eye is the old fare.
6. **Furnace Hollow.** Barrels Kessler, burning from the inside out. His foundry is full of the Unpaid of the town he robbed; his ember stands the slag up (Slagbacks), and drakes nest in the chimneys.
7. **The Needles.** Lena Okoro describes the man who hired them: tall, polite, a stovepipe hat. Abel's line at the boards (*“a tall fella on the rail who never lifted a finger”*) and Sexton's own (*“I attend every death”*) click into place. *(saga)* From the top spire at dusk her glass shows two lights at the end of the world.
8. Hob confesses. He was a Revenant once, eighty years ago. He paid his debt in full, and Sexton made him a crow. *“Every Revenant he makes ends up with feathers, sunshine. Ask me how I know.”* Lena's ember is paid anyway; Ellis isn't ready to break the deal yet. The flame swings toward the Quarry.
9. **Stoneweight Quarry.** Tobias Grant doesn't want to fight. He offers his ember if Ellis lets his crew walk. Ellis can accept, but Sexton's Mourners arrive and force the fight anyway.
10. **The new moon.** Ellis comes to Ransom's Point with the sixth ember and refuses to hand it over. Sexton stops being polite. He takes it, and Ellis dies again. They wake in the family plot beside Pa's frosted grave, greyer and colder than before, with Ransom's Rest gone dark. Sexton now holds six saints. He needs one more: Saint Ada's, in the Deacon's pocket. The Deacon is losing his fight with it, and for the first time the lantern leans toward Gravesend. *(saga)* Two truths land here. Hob: Sexton snuffs nothing; he is building a lighthouse at the Toll Gate with the embers as its lamp (*Antagonist*, the second light). Abel, at the boards: why Ellis came back when the others didn't last, the spark of Ada in the wound (*Protagonist*). Ellis is a keeper, and the saint came after them.

### Act III: The Gravewind

11. Ellis, Ruth, Tilly and Hob make a plan: reach the Deacon before Sexton does, keep Saint Ada's ember, and take the other six back at the Toll Gate.
11a. **Hoarfrost Pass** *(saga)*. The climb to Gravesend through the Gravewind as a blizzard: the frozen dead, ice-bound hearses, Maren Oriel's watch-house (the last keeper standing). The Chainman herds the Unpaid who can't pay up the pass on a toll chain, to Sexton's gate, Pallbearers carrying the ones who can't walk and whiteback wolves taking the ones who fall; played as Crane, the chain on your shoulder is one of these.
12. **Gravesend.** The necropolis, crawling with the Unpaid. The Deacon is dying in a mausoleum, holding Saint Ada's ember to his chest. Ellis and the Deacon finally talk. There are no excuses and no secret reasons, just a coward's confession and a kid who came home, which echoes Ellis's headboard: *“Came home at the last.”* The Mourners attack, and for one fight the Deacon stands beside Ellis again.
13. **The Hollow Deacon.** He dies, and Sexton climbs into his body. Ellis has to fight their mentor's corpse while his own voice keeps apologizing. When it's over, Saint Ada's ember is in Ellis's hand.
13a. **The Sundown Road** *(saga)*. At dusk Ellis walks out past Pa's board on Gravewind Point. Abel says *“Go on, El. I held the door this long.”* Ellis steps off into the canyon and rides the Gravewind west, then walks the road itself: a river of lanterns, the Bone Hounds' crossing, the Chainman's columns, the fog sea's edge, and the false lighthouse growing on the horizon until it fills the sky. Ellis carries the only true light.
14. **The Toll Gate.** Where the road leaves the world: a pier of grey wood on a sea of fog, the ferry rotting at its mooring, a lighthouse with six embers caged in its lamp, and every unpaid soul in the Reaches standing in line. Sexton at the gate is a towering ferryman made of black cloth and crows, with a long rifle called Toll. *(saga)* Six phases, an ember freed in each as the false light fails; then the man himself. Hob flies at his face.

### Ending

The six embers spill free of Sexton's ledger, and Ellis carries them and Saint Ada's home: seven saints relit, and seven towns whose dead can finally cross. The Gravewind blows, and thousands of souls walk past Ellis onto the light laid across the fog, toward the Far Shore. The Deacon is among them, and he tips his hat. Abel walks past last and just nods. Ellis's debt is paid. They could go too.

They don't. Sexton's ledger, now in Ellis's hand, is full of souls who slipped away while the saints were dark. *“Not yet.”* Hob settles on their shoulder. Grandma Delia sets a plate. *(saga)* The epilogue is one shot: the Keeper's Lantern hangs lit on the farm's porch, its flame leaning toward the chapel where Ada burns again, and this time Delia opens the door.

## Seeds in Ransom's Rest (saga, 2026-10-15; the user: plan them)

Small things to plant in the built area so the arc is in view from hour one; none changes a built mission. Built
2026-10-16 (overnight, unattended; placed by `Tools/Unreal/build_area_seeds.py`, words in `Tools/Unreal/story_texts.py`
READABLES and LEDGER_TOWNS, line sets in `create_story_lines.py`).
- **The Ledger's loose pages** (collectible; built): six pages in Sexton's hand, each a ledger entry on one robbed town:
  its saint, the fares its dead crossed free since the saint was lit (owing at two coins the soul), the robbery's date,
  the carrier, a margin note. Where: the sheriff's lockup (The Needles), the depot's baggage room (Highwater Landing),
  the vestry door's step (Ransom's Rest), Calder's Post (Dustwater Mesa), the Providence's tag board (Furnace Hollow),
  the Mooring Ledge (Saint Agnes Mire); Stoneweight Quarry's is found later in the campaign. Reading one sets
  `Ledger.Page.<Town>` and opens its town's page in the Ledger (NPCs, `DA_Bestiary_Town_<Town>`). The robberies:
  Highwater Landing 3 March (Ned), Dustwater Mesa 1 April (Ira), Saint Agnes Mire 9 May (Constance), Furnace Hollow
  14 June (Barrels), the Needles 12 July (Lena), Stoneweight Quarry 2 August (Tobias), Ransom's Rest 21 August at dusk
  (the Deacon; Ellis woke the 28th). The Rest's page is entered 21 July, a month before, with Ellis's name already in it
  twice (guide, then "Collector, thereafter: E. Ransom. Engaged from the 28th."); the margin notes hint at the lamp (Furnace
  Hollow: "Note it for the lamp") and the spire (the Needles: "Keep her off the top spire at dusk").
- **The Deacon's letters** (collectible; built): the strongbox slip exists; the letter never sent on the Mooring Ledge
  (*“Kid. The man who pays us isn't a man.”*, then: paid in coin off dead men's eyes; six towns, and he wants yours worst;
  take your share and ride), and one in the saloon's post rack to Delia, postmarked Halfway Junction 9 August, never
  opened: his warning to get Abel and the lantern off the Rest before the month was out (a coward's mercy by post, not on
  the bluff). Flags `Letter.Deacon.Cleft`, `Letter.Deacon.Saloon`.
- **The map of the Reaches** (prop + lines; built): a wall map over the agent's desk in the depot's ticket office (the
  lines, Halfway Junction at their heart, the seven towns, "Highwater Landing: end of the line"); Mr. Sloane names
  Highwater Landing and the Junction at his window after Main 7.
- **The Seven, on the wall** (prop; built): a wanted poster with seven faces on the Rangers' board, Ruth's signature
  under it; a face crossed out as each ember is paid (flags `Ember.Paid.<Name>`, set by later missions).
- **The road at dusk** (look; the light built): after The Gravewind, at dusk, a second light far west, low and steady
  over the fog (a steady `AFarSignalLight`, 9 km out, shown only at Dusk after Main 6). Hob says *“Don't look at that
  one, sunshine.”* landing again on the bier beside Pa's as the scene ends. Not built: the river of small lights and Grave
  Sight (Grave Sight as a vision mode is still a later option).
- **What Ada gave up** (a page since 2026-10-22, the user's cut of the story's talk; was one talk): Father Aldana's page,
  slid out under the vestry door after Hallowed Ground (reading it sets `Ada.Told`), in his words: the first winter
  the Rest had no coin for the old fare; Ada lay down on the first board and the wind carried her to the edge of the
  world, where she stopped and held up her light; no saint crosses. The smashed Reliquary's lid (prised up at one edge
  in the model) shows the old toll mark, a coin with a line through it, scratched out underneath.
- **Hob's feathers** (lines; built as his mutters from Main 1's end): *“Eighty years I've been collecting for him. You'd think he'd pay a man.”* *“Careful what you shake on. His deals have a long tail.”* *“I had hands once. Don't ask.”*
- **The spark** (look): in the cold open's second shot the ember in the Deacon's hand flares and dims as he fires.
- **Fenn's fare** (built): The Old Fare already shows grave gold on a dead man's eyes and Ned's marker; keep it exactly as it is.

**How the Rest tells it** (the user's calls, 2026-10-21; built 2026-10-22, Pipeline/Story.md "The story's talk cut"): every beat,
character and seed above kept; the talk cut about in half (offers two or three lines, turn-ins three); most of what's left
said on the move by Hob as each step starts; the long scenes are the big beats only (the cold open, Sexton's deal, Sit with
Pa, Delia's Heirloom). Amos's twist (his unpaid hands) is said as the last of them falls; Tilly hands Pa's bail over at Main
3. Things to find off the path, from facts the story already has (no new canon, Main's ruling): a strongbox from the gang's
old bank jobs under the middle Widow, the gang's locker on the Mooring Ledge, W. Pike's survey bench mark on Coffin Rock,
the Averys' savings in the Burnt Homestead's hearth, Thaddeus's bottle in the springhouse, the keepers' stores in the old
lantern house, a spiders' hoard under the hanging tree, Teague's powder slip (six sticks for the lower drift: the proof
Odell wanted), a crate fallen from the old derrick.

## Side quests (samples)

Ransom's Rest's built side missions (Wanted: Already Dead, Calder's Caches, The Old Fare, Lost and Found, Unfinished Business, Knocking Below, The Gravemother, The Last Watch) are in `Docs/Areas/RansomsRest_Expansion.md`; Dead Lift and Back Pay are main missions since 2026-10-27.
- **Unfinished Business** (Ransom's Rest). Amos Whitlock, an Unpaid farmer, won't rest until his hay is in. Load it, under fire.
- **Burial Board.** A town lost its undertaker. Escort its dead to the Gravewind while the Bone Hounds circle.
- **Ruth's Brother.** Ruth's brother rode with a rival gang and died Hollow. She wants him found and put down by the only hand that can.
- **The House Always Wins.** A card game with the dead on the wreck of the *Gilded Lily*. The pot is a legendary; the stakes are your next life. It opens the wreck as a side area.

## Loot in this world

- **Who makes the guns** (the user's pick, 2026-10-22; rules `Docs/Work/2026-10-22-Next/MAKERS.md`). Four makers, each with its trade, paint and mark: **Roundhouse** (Roundhouse Rail Works, the railroad's own shops in Halfway Junction's roundhouse: heavy, big magazines, little kick), **Saint Maud** (Saint Maud's Foundry, the smith-sisters of Furnace Hollow's mission, who bless every barrel: hard-hitting, slow, often element rounds), **Windfall** (Windfall Salvage, Skyreach's sky salvagers in Crossroads Town: fast, loose, odd parts, always worn) and **Meridian** (Meridian Instrument Co. of Highwater Landing, the surveyors' instrument makers: accurate and far-reaching, small magazines). Roundhouse and Windfall are the common stock, Saint Maud and Meridian the fine work. Out here guns come off the dead and get rebuilt by whoever's handy, so every gun is a patchwork of other guns' parts. The Hollow's guns are **soul-forged irons**: weapons that killed so much that the dead stayed in them.
- **Rarity and the beams.** Everything a dead hand held keeps a little of its owner, and Ellis sees it burn. The loot beam is that leftover soul-light, and its color is how much of the owner's will stayed in the gun. Ranks: *Enemies and creatures*.
- **Chests.** Supply Crates are Ruth's Ranger caches. Strongboxes are safes from the gang's old bank jobs. Reliquaries are the saints themselves: each opens for Ellis the moment its ember is pried out of a Hollow, as the saint's thanks, before the ember has to be handed on. The Reliquary of Ransom's Rest lies smashed and dark in the chapel. It's the last one Ellis gets back.
- **Heirloom.** Abel's Ranchhand pump shotgun, a named Epic with fixed parts and the line *Hold the door.* A keeper is buried with his lantern, not his iron, and Abel's lantern wasn't on him, so Delia kept the gun back. She gives it to Ellis after the boss. It's the game's first named gun, and his ghost carries its spectral twin.
- **Elements (when they come).** **Ember** (fire), **Grave** (rot over time, with bonus damage against the Hollow), **Wind** (knockback toward edges) and **Spirit** (bonus damage against the Unpaid; passes through thin cover).

| Legendary | Base | What it does | Flavor text |
|---|---|---|---|
| Last Rites | The Deacon's Coachman shotgun | The last shell of a reload deals double damage | *I'm sorry, kid.* |
| Dead Man's Hand | Ned's pistol | Every eighth shot is a guaranteed critical | *Aces and eights. Always.* |
| Whistler | Ira's carbine | Shots carry gusts that push targets toward edges | *You'll hear it. Once.* |
| Mercy | Constance's rifle | Kills raise a friendly Unpaid for a few seconds | *And they shall rise.* |
| Forge-Hand | Barrels Kessler's shotgun | After a kill, the next shell is molten slag that burns | *Hot enough to hurt the dead.* |
| Iris | Lena Okoro's sniper | Aiming marks every enemy in view, through walls | *I see you, Ellis. I always did.* |
| Millstone | Tobias Grant's heavy shotgun | Each hit makes the target heavier: slower, and it falls faster | *He didn't want to fight. Neither does this.* |
| Toll | Sexton's long rifle (post-game) | Each kill adds a stacking damage bonus that the next miss resets | *Everyone pays.* |

## The game's systems, explained

| System | In this story |
|---|---|
| Death and respawn | Ellis can't stay dead until the debt is paid. Hob's crows drag them back to the nearest of Hob's roosts (the Crow's Mast: the one save, respawn and fast-travel point on every map, its beacon lit in Hob's Ember once found; the user's call, 2026-10-13; 11 on Ransom's Rest), and Hob comments every time. The grave wake in the cold open stays a story scene. On Skyreach, until its own rework, the existing respawn: no roosts and no Hob |
| Falling | When Ellis goes off a big drop at a map's edge (the Rim, the deck on Gravewind Point, the gang's old mooring ledge), Hob and his crows catch them and drop them back on solid ground. Hob complains about the weight. There is ground below now, so leaving the playable outline over an open edge starts the catch, not a fall height. On Skyreach, the existing fall recovery, with no Hob |
| Travel | The railroad and the hearse car (*Getting around*); fast travel is Hob's crows carrying Ellis between found roosts |
| Areas opening | The Keeper's Lantern leans toward the next stolen ember; the station board lists only the story places it has found |
| Enemy ranks | Soul-light: blue, purple and orange tags and coals, in the loot-beam colors |
| Refights | Anything with a page in the Ledger can be called back to account |
| Mission markers | Hob perches near the next objective (never on Skyreach), alongside the minimap arrow. A ? over whoever offers a mission, a ! over whoever it's turned in to (the user's swap of Borderlands', 2026-10-13) |
| The HUD | The cyan lines are **Grave Sight**: Ellis sees souls, weak points and the Unpaid |
| Loot beams | Soul-light left in a dead owner's gun |
| Bestiary | Sexton's **Ledger**, written in his voice until Act III, when Ellis starts writing in it. In the fiction it's his copperplate hand; on screen it keeps the game's UI style |
| Level-ups | Pages of the debt paid down |

## Endgame (levels 50–65)

- **The Ledger.** Repeatable bounties on souls who escaped while the saints were dark: named Hollow outlaws with rolled traits, each with a chance at a soul-forged legendary. Bounty tiers set the danger. Every area's band rises to follow the player (open: *What levels still mean*).
- **The Gravesend Deeps.** Catacombs under the necropolis, built from a room kit, deeper and deadlier floor by floor.
- **The Midnight Stage.** Escort a ghostly stagecoach of the dead along the Reaches' roads to the Gravewind while the Unpaid try to drag its passengers off.
- **Hanging Moon.** A recurring night on Ransom's Rest when the dead walk the orchard. Hold the graves until dawn. It uses the endgame band.

## Signature moments

- The title rising through cloud that parts on the gang's skiff, seven days ago.
- Clawing out of a grave into a golden afternoon, next to your father's frosted one.
- Sexton on the lookout railing: *“Shall we talk business?”*
- Hanging the lantern on the keeper's post while Pa walks the boards at dusk.
- A casino where the house's luck bends bullets.
- Constance's flock rising out of the black water.
- Fighting the Deacon's body while his voice apologizes.
- The pier at the end of the Sundown Road, and the dead walking past you to the Far Shore.
- *(saga)* The Well on Skyreach, and the road seen through it with seven dark gaps.
- *(saga)* Two lights at the end of the world, through Lena's glass.
- *(saga)* Walking the Sundown Road in a river of lanterns, the false lighthouse filling the sky.
- *(saga)* Delia opening the door.

## Production notes

The rules that still hold (the original plan is in the archive):
- **Reuses:** the western art kit (farm, cabins, windmill, fences, rocks, cliffs, trees), the spider and the slime, both gun bases, `UMissionSubsystem` with the minimap arrow. Graves, crosses and burial boards are cheap props; the Reliquary is the story's centerpiece.
- **The Unpaid:** a floating figure on its own small rig: no legs, so no walk cycle and no navmesh. Masked and dithered with an emissive coal, never translucent; priced on Medium before anything else.
- **The railroad:** a locomotive, a passenger car and the hearse car, a rail track kit, the depot with its platform, a water tower and a signal; the station board (a LooterUI panel). The skiff model has two paints (packet, dark).
- **Bosses:** eight bespoke fights (the seven and Sexton) plus Abel: the main cost. The first humans (Sexton, then the outlaws' followings on the *Lily*) set the art bar; concepts come first.
- **Grounded maps:** the terrain scripts have a grounded setting beside the island one, and the tutorial island must still come out identical. The terrain runs past the valley up the ridges and down the Rim into Gravewind Canyon. The horizon is cheap: canyon walls, plains and far mesas, unlit, no collision, no masked leaves. Everything past the valley gets at most 30 draws and 0.6 ms of GPU at the heaviest view, Ransom's Point. The sky: the cloud dome grows past the horizon, the fog turns warm, the sun sits low in the west.
- **Bounds:** an invisible playable outline (`APlayableArea`) backs up the cliffs and steep slopes. Going off one of its open edges starts Hob's catch; the minimap covers only the valley.
- **Open ground:** obstacles mostly from the existing kit, instanced (cliff faces stood up as outcrops, boulders, trees, fences, ruins), plus a freestanding outcrop kit, stone walls and hedgerows. A check in the terrain scripts measures open ground: under 10% of the walkable floor more than 20 m from anything, outside ground left open on purpose (the hayfields, Main Street).
- **Cheap paths first:** the living townsfolk of Ransom's Rest talk through their doors and windows; the cold open is a code-driven camera with silhouettes; captions only, with no voice acting; the Grave Sight vision mode (a post-process) stays a later, measured option.

## Why pick it, and what to watch

Built like a looter shooter: seven names on a list from the first quarter hour, each a place, a boss and a legendary; the lantern makes “why can I go there now” part of the story; the supernatural layer adds a whole enemy family without a second art kit; Abel gives the first hour its heart.

Watch out:
- It's darker than the sunny farm suggests. That contrast is the hook, so own it.
- A father as the first boss is heavy for hour one. Hob's humor and the grief mechanic carry it.
- “The patron is the villain” has to be seeded fairly: the man on the railing, Abel's line, Ira's grave gold, the flame that never leans toward a paid ember, Constance and Lena.

## Decisions

The user's, word for word in the archive. The six questions of step 1 (2026-10-01):
1. **The name of Ellis's home: Ransom's Rest.** Place and town both; locals call it “the Rest”; the main street is just “Main Street”. In the game: the level `/Game/Maps/Lvl_RansomsRest`, the layout folder `Art/Levels/RansomsRest`, the area asset `DA_Area_RansomsRest`. Docs: `Docs/Story.md` and `Docs/Areas/RansomsRest.md`.
2. **When Sexton makes the deal: early**, about 15 minutes in, on the lookout where Ellis died. The list of seven names lands in the first quarter hour.
3. **The first boss: Abel Ransom**, Ellis's father, as one of the Unpaid. The Gravemother stays as the Legendary monster's side mission.
4. *Superseded by 14:* the tutorial exit by the cloud match cut.
5. *Superseded by 14:* Skyreach reachable as a practice island (0 XP, ammo-only drops, “Skip the tutorial”).
6. *Superseded by 14:* the tutorial's guns come along as the guns Ellis was buried with (now Ellis is always buried with a Common Bullpup).

Step 2, the map (2026-10-01):

7. **The layout of Ransom's Rest is approved, with two changes:** it is grounded on **Teropa**, an earth-like planet, not an island; and the open ground gets cliffs and obstacles to break it up.

The six questions the step 2 changes raised (2026-10-01): four approved as proposed, travel changed to the train, the *Gilded Lily* deferred.

8. **Travel between regions: by train** (the user's change; the proposal was small gas-bag airships). In the user's words: *“I think fast travel between regions should be via train.”* Regions are the game's areas. The rules: *Getting around* and *Campaign gating*.
9. **The ground: a high valley that ends at the Rim**, as in *The world*. Everything that hung over the island's rim keeps its drop with the layout unchanged: Ransom's Point and the gang's hidden mooring, the deck on Gravewind Point, and Mill Falls, now into a gorge through the Hogback. The ridges' cliff bands and the canyon give the cliffs the user asked for.
10. **Where the dead go: west, down the Sundown Road**, to the Toll Gate and the Far Shore (*The world*). The Toll Gate is on no map, and Ellis still reaches it by stepping off Gravewind Point at dusk.
11. **The body after its night on the board: back to its grave at dawn.** It makes “the Unpaid drift home to their graves” literal, fits Abel's body in his frosted grave, and keeps the Bone Hounds, which raid the boards on nights no keeper sits up.
12. **The *Gilded Lily*: deferred until Ransom's Rest is complete.** In the user's words: *“Wait until Ransom's Rest is complete.”* Ransom's Rest was approved on 2026-10-08 after the user's play-through; the *Lily* is on hold until the user says (other polish first). Still open: an airship as written (tethered high over a deep canyon, above the cloud that fills it each morning; its fight with the falls, the rigging, the gondolas and the Sky Jumpers), or a casino riverboat on the big river, which loses the falls, the gondolas and the Sky Jumpers. Either way the train brings Ellis to its area's station.
13. **Names: keep the Reaches and the Rim Rangers**, as in *The world*. The Far Shore stays, and the Sundown Road is new. Teropa is named in the docs and on the session picker's subtitle (step 8), rarely in dialogue.

The expansion (2026-10-13; `Docs/Areas/RansomsRest_Expansion.md`, sections 1 and 13, and memory `rr-expansion`):

14. **The map order** (at the top; the *Lily* lies behind the north mountain, and the mountain is optional, not the way to it). **The opening** (*Cold open*, *Skyreach, area 3*). **Givers** for every mission but Main 1, turned in, ? and ! (*The game's systems*, Mission markers). **Hob's roosts** as the save points, replacing the save graves (*The game's systems*, Death and respawn). **The mountain** (*Ransom's Rest, the first area*): the Old Quarry's ore lift opens after Main 5; Back Pay ends in a choice; Thaddeus is in Tilly's cellar; Ruth saw it all; Hiram in person (*Cast*). **Houses:** Ransom's Rest's own house kit, no model used more than twice; Skyreach keeps today's set until its rework. **XP:** the story alone ends about level 5.5, a full clear about 6.8; the Borderlands-like curve with the top level 65 (*What levels still mean*).

The saga (2026-10-15; the readable bible: https://claude.ai/artifact/Gr66uUTXWzetBTaHvE9j9Z):

15. **The user's direction** (tap answers): **deepen and scale Revenant**, keeping its bones; **15–18 regions**; **plan the
    seeds** in Ransom's Rest. Built from it: the saints as souls who refused to cross; the spark; the second light; the
    Well; Halfway Junction, Hoarfrost Pass and the Sundown Road as regions; the cloud heroes as companions; the seeds
    list. **The user's calls (2026-10-15):** (a) the *Lily* is **an airship** over the canyon at Highwater Landing
    (decision 12 closed); (b) the five heroes are **playable characters chosen before the story starts, like
    Borderlands 2** (the user's own words; not "companions playable later"); (c) **the false lighthouse** as written;
    (d) **keep all three new regions**; (e) a non-Ellis pick **takes Ellis's place**: one story for everyone, Pa
    becomes the Keeper, the Deacon's "kid" is whoever you are, Ellis exists only when picked, heroes not picked don't
    appear (*The heroes*); (f) the hero pick is built **after the Rest's seeds, before the *Lily*** (revised by 16).
16. **Ember powers and the first build** (2026-10-15, tap answers): the first build is **the ember-power system with
    Ellis's three**; the hero pick follows it, the seeds run alongside (revising 15f). **One power equipped at a time**,
    like Borderlands; **all three owned from the start**, one equipped.

## Build steps

Ransom's Rest's build steps (1–27) ended with its approval on 2026-10-08. The table is in the archive; each step's deliverables are in `Docs/Areas/RansomsRest.md`. Later decisions are asked at their own build step.
