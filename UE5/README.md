# Generals → Unreal Engine 5 (gameplay remake)

A from-scratch RTS built in UE C++ around the *gameplay* of C&C Generals: Zero Hour — controls,
supply harvesting, unit production, and a skirmish AI opponent. Visuals are placeholder cubes;
original models/maps (from your own game install) can be slotted in later and are not the focus.

**Status: written but not yet compiled or run** (no Unreal install in the authoring environment; target
engine version 5.8 not checked against headers). Expect a round of compile fixes on first build.

## Run it
1. Open `GeneralsRTS.uproject` with UE 5.8 (let it generate project files and build the plugin).
2. Create an empty level (File → New Level → Empty) and save it; make it the default map.
3. Play. `ARTSGameMode` is the default game mode (see `Config/DefaultEngine.ini`) and builds a floor,
   a human base, an AI base, and supply piles at runtime.

## Controls
| Input | Action |
|---|---|
| Left click / drag box | Select (Shift adds) |
| Right click | Contextual: enemy → attack, supply pile → harvest, ground → move (Shift queues) |
| `A` then left click | Attack-move |
| `S` | Stop |
| `Ctrl+1..9` / `1..9` | Save / recall control group |
| Arrow keys, screen edge, wheel | Pan, zoom |
| Select HQ: `Z` `X` `C` `V` / `B` | Queue units / cancel last; right click sets rally point |

## How it maps to the original
| Feature | Here | Original |
|---|---|---|
| Fixed 30 Hz sim | `RTSSimSubsystem` | `GameLogic`, `LOGICFRAMES_PER_SECOND` |
| Orders | `FRTSCommand` → `ARTSUnit::IssueCommand` (same path for human and AI) | `MessageStream`, `AIUpdate` |
| Harvest loop | `ARTSUnit::TickGather`, `ARTSResourceNode`, depot buildings, `URTSEconomyComponent` | `SupplyTruckAIUpdate`, `DockUpdate`, `Money` |
| Production | `URTSProductionComponent` (queue, cost, refund, rally) | `ProductionUpdate` |
| Combat | auto-acquire, chase, cooldown-based attacks | `AIStates`, `Weapon` |
| Skirmish AI | `URTSSkirmishAI`: worker upkeep, unit rotation, growing attack waves, base defense | `AIPlayer`, `AISkirmishPlayer` |

## Known gaps / next steps (in priority order)
1. **Compile and playtest**; fix inevitable API issues.
2. **Pathfinding + crowd separation**: units currently steer in straight lines and overlap.
3. **Base building** (dozer/worker placing structures) and tech requirements/upgrades.
4. **Scale for a massive map and huge armies**: swap per-unit actors for MassEntity, spatial-grid queries
   instead of linear scans, flow-field pathing, World Partition streaming, fog of war.
5. **Richer unit system**: composable chassis/locomotor/weapon-slot data assets, veterancy, abilities/special powers,
   armor and damage types, StateTree behavior per unit class.
6. Smarter AI: build orders per faction, scouting, target priority, harassment/retreat logic.
7. Real art: import models/maps from an owned install (W3D/`.map` importers) or use new content.
8. Multiplayer (the sim is stepped at a fixed rate and driven by commands, which keeps lockstep possible).

## Licensing
Original source is GPL v3 (see `../LICENSE.md`); derived code must comply. Game assets remain EA's:
don't redistribute converted assets.
