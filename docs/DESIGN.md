# Design notes

## Product goal

Turn outpost labor assignment from a stack of individual Shift-click operations into a dedicated management screen while preserving Kenshi's native permanent-job queues and GOAP execution.

The eventual interface should answer three questions at a glance:

1. Which workers belong to this outpost?
2. Which machines, farms, defenses, and support stations need assignments?
3. What exact priority order will Kenshi execute for each worker?

## Non-negotiable architecture rule

**The plugin owns presentation and intent translation. Kenshi owns jobs, serialization, and AI execution.**

That means the first implementation writes through the public reconstructed methods on `Character` and `OrdersReceiver`. It does not mutate `lektor<Tasker*>` containers directly, create a second queue, or replace GOAP.

## 0.1 vertical slice

The initial queue editor proves the riskiest shared adapter before outpost discovery is added:

1. Hook `PlayerInterface::updateUT`, which runs on the UI thread.
2. Edge-detect `Ctrl+J` and create a native MyGUI window.
3. Resolve `PlayerInterface::selectedCharacter` through its stable `hand`.
4. Snapshot the selected character's permanent jobs using `getPermajobCount`, `getPermajobName`, `getPermajob`, and `getPermajobData`.
5. Revalidate the selected row immediately before each mutation.
6. Call `movePermajob`, `removePermajob`, `clearPermajobs`, or `OrdersReceiver::setJobsEnabled`.
7. Ask the character to reconsider its current AI action and flag the vanilla selection UI for refresh.
8. Poll the queue at a low cadence so external vanilla changes appear in the custom window.
9. Destroy plugin-owned widgets when `PlayerInterface::clearAndReset` begins.

Every engine read or write that dereferences reconstructed runtime objects lives in a small SEH wrapper. MyGUI calls remain on the UI thread.

## Why the first window only edits one character

Outpost-wide assignment requires reliable answers for building ownership, active-zone lifetime, machine function, valid task-target pairings, and multiple-base selection. Queue mutation itself is independent of those questions. Isolating it first gives the later scanner and assignment matrix a tested write path instead of debugging UI, discovery, and AI behavior simultaneously.

## Planned components

### CharacterJobAdapter

Reads a queue into immutable rows and exposes guarded operations:

- move an exact row;
- remove an exact row;
- clear the queue;
- toggle Jobs;
- later, add a validated task and target.

Rows retain the opaque `Tasker*` identity for short-lived UI revalidation only. The plugin never owns or deletes that object.

### OutpostScanner

Will identify a loaded player outpost and enumerate relevant targets. Its normalized result should contain:

```cpp
struct WorkTarget
{
    hand target;
    BuildingFunction function;
    TaskType recommendedTask;
    std::string displayName;
    bool operational;
    bool powered;
};
```

The scanner must prefer stable `hand` values over long-lived raw building pointers. It should reacquire objects on refresh because zones unload, buildings upgrade, and structures are dismantled.

### JobRuleRegistry

Maps functional building categories to ordinary player job types, then permits exceptions by FCS identity. Generic function-based matching should cover normal modded production buildings without a name list; UWE/Kaizo oddities can live in small override data rather than in UI code.

### AssignmentMatrix

The first outpost-scale UI will remain concrete:

- rows: player characters assigned to the selected outpost;
- columns or grouped cards: actual stations;
- cell action: add/remove the exact station job;
- queue drawer: reorder that worker's complete vanilla priority list.

### OptionalPriorityScheduler

A RimWorld-style category matrix is a later layer. It must convert abstract priorities such as Farming 1 or Hauling 3 into concrete target jobs and avoid continuously fighting Kenshi's own decisions. It should be optional and built only after direct station assignment is stable.

## Safety rules

- Never touch MyGUI from a worker thread.
- Never retain an unvalidated engine pointer across a load/reset transition.
- Never mutate the permanent-job container directly when a game method exists.
- Revalidate queue row identity immediately before removing or moving it.
- Treat destroyed, upgraded, unloaded, or replaced targets as expected state changes.
- Leave combat, rescue, self-preservation, and immediate orders to vanilla Kenshi.
- Require explicit confirmation for broad destructive actions.
- Test on disposable saves until save/load/import behavior is proven.

## Roadmap

### 0.1: queue field test

- selected-character queue window;
- Jobs toggle;
- move, remove, guarded clear;
- runtime refresh and transition cleanup;
- source validator and build package.

### 0.2: reliable target assignment

- inspect a selected world object;
- ask Kenshi for valid player-task probability;
- add one concrete permanent job through the game method;
- display target identity and validation failures.

### 0.3: loaded outpost scanner

- identify the active player outpost;
- enumerate and categorize its work targets;
- display power, completion, and target-lifetime state;
- add mod override data.

### 0.4: assignment board

- worker roster plus station browser;
- batch assignment to selected workers;
- queue conflicts and duplicate warnings;
- role presets that expand into concrete jobs.

### 0.5+: optional scheduling

- abstract work-category priorities;
- demand detection and worker selection;
- skill/distance recommendations;
- multiple loaded outposts and diagnostics.
