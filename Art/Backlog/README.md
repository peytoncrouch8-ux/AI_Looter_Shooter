# Backlog models

Finished models kept for later in the game's development. They follow every rule in `Art/README.md`, but
`Tools\models.ps1` only reads `Art/Models`, so nothing here reaches Unreal until it is brought in on purpose.

```
Art/Backlog/<Category>/<Name>.py   becomes /Game/Art/<Category>/SM_... when imported
```

To bring one into the game, move its file into `Art/Models/<Category>/` and run `Tools\models.ps1 -Only <Name>`, or
import it from here without moving it: `Tools\models.ps1 -Source Art\Backlog\<Category>`. To check that a file still
exports without touching the game, give the export a folder of its own and skip the import:
`Tools\models.ps1 -Source Art\Backlog\<Category> -Out <some folder> -NoImport`.

Previews: run a file with `-- --preview` (see its docstring); the pictures land in `Saved/ArtPreviews/Backlog`.

| File | Models | Notes |
|---|---|---|
| `Loot/Chests.py` | SupplyCrate (Uncommon), Strongbox (Rare), Reliquary (Legendary), each with a `_Lid`; Strongbox_Wheel, Reliquary_Crystal | Openable chests the user picked on 2026-09-30. Lids pivot on their hinge (SOCKET_Lid on the body); SOCKET_Loot is where loot comes out. |
| `Weapons/AssaultRifles.py` | Homestead, Regulator, Kestrel, Scrapjack, Zephyr (Body, Magazine, Sight each) | The five AR designs the user chose from; Kestrel became the game's AR (Art/Models/Weapons/Bullpup.py). The others are kept for reference (possible legendary bases). |
| `Weapons/Shotguns.py` | Farmhand, Coachman, Breacher, KestrelS, Thunderdrum (Body; Pump or Magazine) | Five shotgun designs for the user to choose from (2026-10-01). |
| `Weapons/ShotgunHybrids.py` | Heartwood, Ranchhand, Homesteader (Body, Pump each) | Three Kestrel-S x Farmhand mixes the user asked for after passing on the five shotguns (2026-10-01); Ranchhand became the game's shotgun (Art/Models/Weapons/Ranchhand.py). |
| `Creatures/SpiderConcepts.py` | MeadowWolf, MossBack, CliffHuntsman, EmberTarantula, SkyJumper (one mesh each) | Five spider rework concepts (2026-10-01), all on the current spider's anatomy so the pick can become the rig; pattern textures are painted into Intermediate/SpiderConcepts at build time. |
| `Creatures/SlimeConcepts.py` | MeadowSlime, HoneySlime, BoulderSlime, MagmaSlime, CrystalSlime, BogSlime, MushroomSlime, ScrapSlime (a few meshes each) | Eight slime concepts (2026-10-01); the gel is transmissive, so the concept renders use Cycles. |
| `Icons/IconConcepts.py` | none (writes HTML/SVG sheets) | Five icon styles for the weapons and ammo types (2026-10-01): Stencil, Blueprint, Cutout, Inked, Badge. The traced outlines become `LooterUI::FVectorIcon` data once the user picks a style. Run it with Blender's Python. |
