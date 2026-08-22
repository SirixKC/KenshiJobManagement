# Kenshi Job Management

Kenshi Job Management adds an ingame UI for visualizing and editing job queues.

**Version: 0.3.0-alpha**

## Disclaimer

This is an alpha release. It has been tested for hours on my machine with ~240 other mods, but I can't promise it's safe for every save/mod list. Back up your save just incase. There isn't currently an undo button.

I use Radiant Dark True UI, so the colors may look different with other UI mods. Default settings should be vanilla friendly, there's a dark UI toggle in the options menu.

## Requirements

- Kenshi Steam 1.0.65+
- RE_Kenshi 0.3.4 or newer;

## Features

### Squad Jobs

- Open the manager with **Ctrl+J** or the Job Manager button on the HUD.
- View every active player squad in Kenshi `TAB`.
- TAB cycles between active squdas, expanding squad displays automatically and returning them to closed if applicable.
- Supports copy and paste, dragging and dropping. Multiple jobs dragged onto one character will move any non duplicate jobs, leaving duplicates in place.
- Select recipients by clicking portraits or the bottom squad selector. (Hold ctrl to select more than one character or squad.)
- Add missing Rescue, Put in Bed, Medic, Robotics, and Splinting jobs with the compact Add Medic/Robotics button. `Prioritize Healing` reorders existing healing jobs. (Is this the ideal order??)
- Use `Remove Invalid Jobs` to clean up unresolved fixed-target jobs. (Should only remove jobs that can't otherwise be performed. Such as mining natural resource deposits after an import).

### Stations

- Browse all player-owned workstations and assigned natural resource nodes in a grouped grid.
- See exact station names, including renamed buildings, area, assignment count, and issues that may prevent its use. (ex: Not enough power)
- Click on a station to open a new UI to assign/remove workers.
- Workers sort by relevant skill. (Highest to lowest, training benches are lowest to highest.)

The game pauses while the manager is open and restores the prior pause/speed
state when it closes.

## Installation

1. Subscribe on steam https://steamcommunity.com/sharedfiles/filedetails/?id=3785306982

Alternatively:

1. Download the source and compile yourself!


## Credits and license

Codex generated the code and GUI icons. I wrote the prompts, tested the mod in
game, took the screenshots, and made the preview art. Codex created the initial version of this readme, I edited and removed about 80% of it because it was dumb LLM fluff.

Kenshi and its assets belong to Lo-Fi Games. RE_Kenshi and KenshiLib are
separate projects with their own licenses and release notes.

This project is licensed under **GPL-3.0-only**.
