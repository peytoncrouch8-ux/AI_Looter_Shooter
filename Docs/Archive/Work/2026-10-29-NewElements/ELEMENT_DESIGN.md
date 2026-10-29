# Why the elements exist: bodies, wards, immunities (the user's calls, 2026-10-29 evening)

The user: "Now that elements are in the game, they need to have a reason to exist ... some enemies have wards that have
to be broken before getting to their health ... some enemies are made of different materials ... some enemies are special
or unique that may not be affected by any element ... what enemies get affected or don't, new enemies that can be added."

## The user's answers (verbatim where it matters)
- **Wards**: "Mix of option 1 and 2. Wards can vary in what type they are. A true ward (teal) is weak to ward break, red
  health bars are weak to cinder, shock wards are weak to shock, and this should be the same for all element types. The
  most common ward should be shock wards on enemies with wards. Once the ward is broken, their health is open and is
  susceptible to cinder, once the ward is broken they stagger. Ward Color bar should be above their health bar."
  Reading: Borderlands' layers. A ward has a type by element and its matching element breaks it fast: true ward (teal) ->
  Wardbreak, shock ward (blue) -> Thunder, and so for every element (hallow ward gold -> Hallow, rot ward green -> Rot,
  cinder ward orange -> Cinder). Shock wards are the common ward. Health (the red bar) is Cinder's layer. A broken ward
  staggers the creature. The HUD draws the ward bar over the health bar in the ward's colour.
- **Immunity**: bodies + Touched variants: spirit / stone / iron bodies immune to the off elements, Touched variants
  immune to their own element, each boss one story immunity (Gravemother Rot, Foreman Cinder, Abel Hallow).
- **New enemies first**: Touched variants (elemental versions of spiders, slimes, miners, the Unpaid: no new models, the
  element's glow, drop its guns) and the Stone Sentinel (a grave statue that wakes: stone body, Rot immune, Cinder
  resists, Wardbreak cracks it, Thunder weak).
- **Gap**: weak x2, resist x0.33 (was x1.75 / x0.5).

## Bodies (ElementRules::BodyOf grows; the table)
| Body | Who | Weak | Resist | Immune |
| Chitin | spiders, Spitter, Gravemother | Cinder, Rot | Wardbreak | - |
| Gel | slimes | Cinder, Thunder | Hallow, Rot | - |
| Flesh (Other today) | Gravepickers, Bone Hounds, Coffin Rats, Knockers, the player's foes that live | Thunder, Rot, Cinder (red health) | - | - |
| Bone (Undead) | dead miners, Bell-Ringer, Foreman, Abel's risen | Hallow | Thunder | - |
| Coal (Unpaid) | the Unpaid | Hallow; Wardbreak on the coal (crit) | Cinder, Thunder | - |
| Spirit | Ethereal tier, Phasing, (wisps later) | Wardbreak, Hallow | - | Cinder, Rot, Thunder |
| Stone (new) | Stone Sentinel | Wardbreak, Thunder | Cinder | Rot |
| Iron (later) | Tin Man (Furnace Hollow) | Thunder, Rot | Cinder | Wardbreak |

## Wards (UHealthComponent's Shield gains an element; the Warded trait picks one)
- A ward's element: the creature's own for a Touched variant; else rolled: shock 50%, true (Wardbreak) 20%, the rest 10% each.
- Damage to a ward: its matching element x3; Wardbreak x2 on any ward (the name); its own... (a shock ward is *weak* to
  shock, per the user: the matching element is the breaker, there is no "immune to its own"). Others x1. Statuses don't set
  through a standing ward (the ward takes the hit); the break staggers (FHitReaction, job Power) and bursts in its colour.
- HUD: the ward bar above the health bar in the ward's colour (UI/HUD creature bars; the bestiary's ward line).

## Touched variants (a rank-like prefix on Basic..Epic creatures from Gravebound areas; a trait "Touched" with an element)
- Smoldering spider (Cinder), Live-wire slime (Thunder), Rotted miner (Rot), Salt-crusted Unpaid (Wardbreak), Haloed
  Bell-Ringer (Hallow): the element's glow on the body (CreatureTierLook shell in the element's colour), immune to their
  element, weak to its counter (Cinder<-Wardbreak, Thunder<-Wardbreak, Rot<-Cinder, Wardbreak<-Thunder, Hallow<-Rot), a
  ward of their element when Warded, and their loot leans to that element's parts (x3 weight).
- Callouts: WEAK / RESIST / NO EFFECT on the damage number; the bestiary's weakness line adds "IMMUNE TO ...".

## Next job (after the Thunder/Rot commit): a contract for bodies + wards + Touched + Stone Sentinel, lanes:
rules/health (Main), HUD ward bar (UI lane), Touched look + loot (Sonnet), Stone Sentinel (model: reuse a grave statue
mesh from the chapel/graves set; brain: a slow heavy melee, Opus).
