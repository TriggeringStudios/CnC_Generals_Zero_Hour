# Generals → Unreal Engine 5 remake

Design and roadmap for a UE5 remake of the gameplay in `GeneralsMD/` (Zero Hour), adapted to a
very large map with a much larger unit count. `Plugins/GeneralsRTS` is the starting skeleton.

Status: **scaffold only, not yet compiled.** No UE install was available when this was written, and
the target engine version (5.8) was not checked against real headers. Expect API fixes on first build.

## Assets: what this repo does and does not contain
This repo is source code only. There are no models, maps, textures, INI files or audio in it. Those live
in the `.big` archives (`INI.big`, `W3D.big`, `Maps.big`, `Textures.big`, ...) of a copy of the game you own.
The remake therefore uses **importers that read your install**; do not commit converted assets.

| Original | Format | UE target | Importer work |
|---|---|---|---|
| `.big` archives | BIGF container (simple table of contents) | virtual file system | small reader; do this first |
| Models | `.w3d` (chunked; meshes, hierarchy, animations, HLOD) | Static/Skeletal Mesh + AnimSequence | chunk parser; reference `Libraries/Source/WWVegas/WW3D2` |
| Textures | `.tga`/`.dds` | Texture2D | standard |
| Maps | `.map` (chunked; heightmap, blend tiles, objects, waypoints, scripts, triggers) | Landscape + World Partition level | parser in `GameClient/MapUtil.cpp`, `Common/System/DataChunk.cpp`, `GameClient/Terrain/` |
| Rules | `.ini` (Object, Weapon, Locomotor, Armor, Upgrade, SpecialPower, Science...) | `URTSObjectDefinition` etc. | parser mirroring `Common/INI/` |
| Scripts in maps | binary script chunks | data + StateTree/Blueprint | `GameLogic/ScriptEngine/` (~40k lines) |

## System mapping (original → UE)
| Original system | Where | UE approach |
|---|---|---|
| Fixed 30 Hz logic loop | `GameLogic.cpp`, `LOGICFRAMES_PER_SECOND` | Fixed-step sim subsystem; render interpolates. Keeps determinism option open |
| Object + Module system (~220 module headers: Update, Die, Create, Body, Contain, Upgrade, SpecialPower) | `GameLogic/Object/*` | Data-driven modules → UE components / Mass fragments+processors, configured from imported INI |
| Input, selection, control bar, hotkeys, command messages | `GameClient/MessageStream`, `GUI/ControlBar`, `Input` | Enhanced Input + `ARTSPlayerController`; all actions become `FRTSCommand` (see `RTSTypes.h`), the only way to affect the sim |
| Resource gathering (supply centers, docks, supply trucks/chinooks) | `Update/SupplyTruckAIUpdate`, `DockUpdate/*`, `Money.cpp` | Dock/gather components + `URTSEconomyComponent` (done) |
| Unit production, rally points, exit points, upgrades | `ProductionUpdate`, `ProductionExitUpdate`, `Upgrade/*` | `URTSProductionComponent` (started), upgrade components |
| Pathfinding (grid, layers, bridges, hierarchical) | `AI/AIPathfind.cpp` | Custom flow-field/hierarchical layer over Mass; navmesh is too slow for thousands of units |
| Locomotors (legs, wheels, treads, hover, wings, thrust) | `Object/Locomotor.cpp` | Mass movement processors + locomotor data assets |
| Combat: weapons, armor, damage types, veterancy | `Weapon.cpp`, `Armor.cpp`, `ExperienceTracker.cpp` | Data assets + damage processors |
| Skirmish AI | `AI/AIPlayer.cpp` (3.8k lines), `AISkirmishPlayer.cpp`, build lists | Build-order/economy/attack-wave AI as StateTree or C++ player controller issuing the same `FRTSCommand`s as a human |
| Unit AI (guard, attack-move, states) | `AI/AIStates.cpp`, `AIGuard.cpp`, `Squad.cpp`, `AIGroup.cpp` | StateTree per unit-class, group orders via commands |
| Fog of war / shroud, partition manager | `PartitionManager.cpp`, `GhostObject.cpp` | Grid-based vision on GPU/CPU; spatial hash for queries |
| Map scripting, victory conditions | `ScriptEngine/` | Interpreter for imported scripts, or reimplement per map in Blueprint/StateTree |
| Lockstep multiplayer | `GameNetwork/` | Deferred; sim uses no UE physics/floating nondeterminism to keep it possible (fixed-point or careful float) |
| GameSpy, Miles, Bink, SafeDisk | libs | Drop; use UE audio/Media/online subsystems |

## "New technology" opportunities
- **Massive map**: World Partition + Landscape with streaming; sim regions decoupled from render streaming.
- **Large unit counts**: MassEntity for infantry/vehicles (thousands), full actors only for heroes/structures; Nanite for structures/terrain detail, instanced skinned meshes or Vertex Animation Textures for crowds.
- **Unit system expansion**: units defined as composable data (chassis + locomotor + weapon slots + upgrades) so new units need no code; per-unit-class StateTree behavior.
- **Vision/pathing** at scale: hierarchical flow fields, chunked and streamed with the world.

## Roadmap
1. **Foundation**: `.big` reader, INI parser → `URTSObjectDefinition`; fixed-step sim subsystem; `FRTSCommand` pipeline. *(types + data asset started)*
2. **Controls**: RTS camera, box/group selection, control groups, right-click contextual orders, command card UI.
3. **Economy loop**: supply source + dock + truck/harvester gather cycle → money → build queue → spawn (Economy done, Production spawn TODO).
4. **Movement & combat**: locomotors, pathing, weapons/armor/damage, veterancy.
5. **Skirmish AI**: port AIPlayer build lists/team logic; plays via commands.
6. **Assets**: W3D and map importers; first playable on an original map.
7. **Scale-up**: large World Partition map, Mass conversion, perf budgets.
8. Later: scripts, all factions/generals' powers, multiplayer.

## Licensing note
The original source is released under GPL v3 (see `LICENSE.md`); derived code must comply. Game assets remain EA's property: require the user's own install and never redistribute converted assets.
