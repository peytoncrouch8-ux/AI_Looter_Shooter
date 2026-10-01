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
