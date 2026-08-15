// SPDX-License-Identifier: GPL-3.0-only
// Read-only, session-scoped data layer for the Stations tab.
//
// Integration contract
// --------------------
// Include this file after RuntimeAccess.inl, inside the plugin's anonymous
// namespace.  The including translation unit must also include:
//
//   <kenshi/Building/Building.h>
//   <kenshi/Faction.h>
//   <kenshi/Town.h>
//
// UseableStuff is needed for the exact engine-defined relevant stat and power
// state.  KenshiLib's generated UseableStuff header is not standalone: after
// the normal MyGUI headers, include <kenshi/gui/InventoryGUI.h> and then
// <kenshi/Building/UseableStuff.h>.
//
// RuntimeAccess.inl supplies HandleIdentity, MemberSnapshot,
// CaptureHandleIdentity, SameHandleIdentity, BuildMemberSnapshot,
// TryGetOrderedMemberHandles, TryReadRootObjectName, TryReadStatValue,
// TryGetLocalizedStatName, TryGetBlockingCondition and SKILL_DEFINITIONS.
//
// The station board is derived only from exact permanent-job targets found in
// readable loaded-player queues.  It never enumerates zones, ownership lists,
// or unrelated world buildings.  Building, Platoon and Character pointers
// never survive the guarded function that resolves them.  BeginStationScan()
// takes the queue snapshot.  StepStationScan() enriches no more than one known
// job target per UI call.

#ifndef KENSHI_JOB_MANAGEMENT_STATION_SCANNER_INL
#define KENSHI_JOB_MANAGEMENT_STATION_SCANNER_INL

    const size_t STATION_SCAN_TARGET_LIMIT = 2048;

    enum StationCategory
    {
        STATION_CRAFTING,
        STATION_REFINING,
        STATION_FARMING,
        STATION_MINING,
        STATION_RESEARCH,
        STATION_TRAINING,
        STATION_STORAGE_HAULING,
        STATION_DEFENSE,
        STATION_OTHER
    };

    // Visual detail is separate from StationCategory.  Categories remain the
    // stable unit for filtering and sorting; this enum only chooses more
    // specific artwork when Kenshi exposes an exact, non-localized prototype
    // or functionality string ID.
    enum StationVisualSubtype
    {
        STATION_VISUAL_DEFAULT,
        STATION_VISUAL_COPPER_ORE,
        STATION_VISUAL_IRON_ORE,
        STATION_VISUAL_COPPER_PLATES,
        STATION_VISUAL_IRON_PLATES,
        STATION_VISUAL_STEEL_BARS,
        STATION_VISUAL_COPPER_ALLOY_PLATES,
        STATION_VISUAL_ELECTRONICS,
        STATION_VISUAL_CROSSBOW,
        STATION_VISUAL_SKELETON_LIMBS,
        STATION_VISUAL_COUNT
    };

    struct StationBaseStatSnapshot
    {
        StatsEnumerated stat;
        std::string name;
        float rawValue;
        int displayValue;

        StationBaseStatSnapshot() :
            stat(STAT_NONE), rawValue(0.0f), displayValue(0)
        {
        }
    };

    struct StationQueuedJobSnapshot
    {
        JobRowSnapshot exactJob;
        TaskType taskType;
        std::string jobLabel;
        std::string targetLabel;
        HandleIdentity target;
        bool hasTarget;
        bool targetAvailable;
        int priority;

        StationQueuedJobSnapshot() :
            taskType(NULL_TASK), hasTarget(false), targetAvailable(false),
            priority(0)
        {
        }
    };

    struct StationMemberSnapshot
    {
        HandleIdentity identity;
        hand handle;
        std::string name;
        std::string condition;
        bool loaded;
        bool queueAvailable;
        bool truncated;
        bool jobsEnabled;
        int permanentJobCount;
        std::vector<SkillValue> topSkills;
        std::vector<StationBaseStatSnapshot> baseStats;
        std::vector<StationQueuedJobSnapshot> jobs;

        StationMemberSnapshot() :
            loaded(false), queueAvailable(false), truncated(false),
            jobsEnabled(false), permanentJobCount(0)
        {
        }
    };

    struct StationSquadSnapshot
    {
        HandleIdentity identity;
        hand handle;
        std::string name;
        bool loaded;
        bool queueIncomplete;
        int unavailableMemberCount;
        std::vector<StationMemberSnapshot> members;

        StationSquadSnapshot() :
            loaded(false), queueIncomplete(false), unavailableMemberCount(0)
        {
        }
    };

    struct StationAssignmentSnapshot
    {
        JobRowSnapshot exactJob;
        HandleIdentity squad;
        HandleIdentity member;
        std::string squadName;
        std::string memberName;
        TaskType taskType;
        std::string jobLabel;
        int priority;
        bool relevantSkillKnown;
        StatsEnumerated relevantStat;
        std::string relevantSkillName;
        int relevantSkillValue;

        StationAssignmentSnapshot() :
            taskType(NULL_TASK), priority(0), relevantSkillKnown(false),
            relevantStat(STAT_NONE), relevantSkillValue(0)
        {
        }
    };

    struct StationTargetSnapshot
    {
        HandleIdentity identity;
        hand handle;
        HandleIdentity areaIdentity;
        std::string areaName;
        int zoneX;
        int zoneY;
        StationCategory category;
        StationVisualSubtype visualSubtype;
        std::string name;
        bool naturalResourceException;
        bool relevantSkillKnown;
        StatsEnumerated relevantStat;
        std::string relevantSkillName;
        bool blocking;
        std::string blockingStatus;
        std::vector<StationAssignmentSnapshot> assignments;

        StationTargetSnapshot() :
            zoneX(0), zoneY(0), category(STATION_OTHER),
            visualSubtype(STATION_VISUAL_DEFAULT),
            naturalResourceException(false), relevantSkillKnown(false),
            relevantStat(STAT_NONE), blocking(false)
        {
        }
    };

    // Temporary roster reference.  Keep this POD so the guarded list readers
    // never need to unwind a C++ object after an access violation.
    struct StationPlatoonReference
    {
        Platoon* platoon;
        short order;
        unsigned int sourceOrder;

        StationPlatoonReference() : platoon(NULL), order(0), sourceOrder(0) {}
    };

    struct StationScanState
    {
        bool started;
        bool complete;
        bool truncated;
        bool rosterIncomplete;
        size_t nextTarget;
        size_t targetsCompleted;
        size_t targetsFailed;
        std::vector<hand> assignedTargetHandles;
        std::vector<StationSquadSnapshot> squads;
        std::vector<StationTargetSnapshot> stations;
        std::vector<std::string> errors;

        StationScanState() :
            started(false), complete(false), truncated(false),
            rosterIncomplete(false), nextTarget(0), targetsCompleted(0),
            targetsFailed(0)
        {
        }
    };

    const char* GetStationCategoryName(StationCategory category)
    {
        switch (category)
        {
        case STATION_CRAFTING: return "Crafting";
        case STATION_REFINING: return "Refining";
        case STATION_FARMING: return "Farming";
        case STATION_MINING: return "Mining";
        case STATION_RESEARCH: return "Research";
        case STATION_TRAINING: return "Training";
        case STATION_STORAGE_HAULING: return "Storage / Hauling";
        case STATION_DEFENSE: return "Defense";
        default: return "Other / Unclassified";
        }
    }

    bool IsStationDisplayJob(TaskType taskType)
    {
        // These are global worker behaviors in Kenshi. Their stored subject
        // does not define the scope of the permanent job, so it must not be
        // presented as a station assignment. Keep them in the Squad Jobs
        // queue; exclude them only from this read-only station projection.
        return taskType != JOB_BUILDER &&
            taskType != JOB_MEDIC &&
            taskType != JOB_REPAIR_ROBOT &&
            taskType != FIND_AND_RESCUE;
    }

    void AddStationScanError(StationScanState* state, const std::string& text)
    {
        if (state == NULL)
        {
            return;
        }
        // Keep the UI tooltip bounded even if a damaged save has many zones.
        if (state->errors.size() < 32)
        {
            state->errors.push_back(text);
        }
    }

    bool StationIdentityInList(
        const HandleIdentity& identity,
        const std::vector<HandleIdentity>& identities)
    {
        for (size_t index = 0; index < identities.size(); ++index)
        {
            if (SameHandleIdentity(identity, identities[index]))
            {
                return true;
            }
        }
        return false;
    }

    bool StationTargetAlreadyScanned(
        const HandleIdentity& identity,
        const std::vector<StationTargetSnapshot>& stations)
    {
        for (size_t index = 0; index < stations.size(); ++index)
        {
            if (SameHandleIdentity(identity, stations[index].identity))
            {
                return true;
            }
        }
        return false;
    }

    bool StationTargetSnapshotLess(
        const StationTargetSnapshot& left,
        const StationTargetSnapshot& right)
    {
        if (left.areaName != right.areaName)
        {
            return left.areaName < right.areaName;
        }
        if (left.category != right.category)
        {
            return left.category < right.category;
        }
        if (left.name != right.name)
        {
            return left.name < right.name;
        }
        if (left.identity.container != right.identity.container)
        {
            return left.identity.container < right.identity.container;
        }
        if (left.identity.index != right.identity.index)
        {
            return left.identity.index < right.identity.index;
        }
        return left.identity.serial < right.identity.serial;
    }

    void SortStationTargetsForDisplay(StationScanState* state)
    {
        if (state != NULL)
        {
            std::sort(
                state->stations.begin(), state->stations.end(),
                StationTargetSnapshotLess);
        }
    }

    bool TryBuildStationBaseStats(
        Character* character,
        std::vector<StationBaseStatSnapshot>* statsOut)
    {
        if (character == NULL || statsOut == NULL)
        {
            return false;
        }

        statsOut->clear();
        for (size_t index = 0; index < SKILL_DEFINITION_COUNT; ++index)
        {
            const SkillDefinition& definition = SKILL_DEFINITIONS[index];
            float raw = 0.0f;
            if (!TryReadStatValue(character, definition.stat, &raw))
            {
                continue;
            }

            StationBaseStatSnapshot stat;
            stat.stat = definition.stat;
            stat.name = definition.fallbackName;
            stat.rawValue = raw;
            stat.displayValue = static_cast<int>(std::floor(raw));
            statsOut->push_back(stat);
        }
        return true;
    }

    bool TryBuildStationMember(
        const hand& memberHandle,
        StationMemberSnapshot* memberOut)
    {
        if (memberOut == NULL)
        {
            return false;
        }

        StationMemberSnapshot result;
        result.handle = memberHandle;
        CaptureHandleIdentity(memberHandle, &result.identity);

        MemberSnapshot queueSnapshot;
        const bool queueRead = BuildMemberSnapshot(memberHandle, &queueSnapshot);
        result.name = queueSnapshot.name;
        result.condition = queueSnapshot.condition;
        result.loaded = queueSnapshot.loaded;
        result.queueAvailable = queueSnapshot.queueAvailable;
        result.truncated = queueSnapshot.truncated;
        result.jobsEnabled = queueSnapshot.jobsEnabled;
        result.topSkills = queueSnapshot.skills;
        result.permanentJobCount =
            static_cast<int>(queueSnapshot.jobs.size());

        Character* character = NULL;
        if (TryResolveCharacter(memberHandle, &character))
        {
            TryBuildStationBaseStats(character, &result.baseStats);
            int exactCount = 0;
            if (TryGetPermajobCount(character, &exactCount) && exactCount >= 0)
            {
                result.permanentJobCount = exactCount;
            }
        }

        result.jobs.reserve(queueSnapshot.jobs.size());
        for (size_t slot = 0; slot < queueSnapshot.jobs.size(); ++slot)
        {
            const JobRowSnapshot& source = queueSnapshot.jobs[slot];
            StationQueuedJobSnapshot job;
            job.exactJob = source;
            job.taskType = source.taskType;
            job.jobLabel = source.jobLabel;
            job.targetLabel = source.targetLabel;
            job.target = source.target;
            job.hasTarget = source.hasTarget;
            job.targetAvailable = source.targetAvailable;
            job.priority = static_cast<int>(slot) + 1;
            result.jobs.push_back(job);
        }

        if (result.name.empty())
        {
            result.name = "Unavailable member";
        }
        *memberOut = result;
        return queueRead;
    }

    __declspec(noinline) bool CallStationActiveSquadName(
        ActivePlatoon* active,
        std::string* nameOut)
    {
        try
        {
            if (active == NULL || nameOut == NULL)
            {
                return false;
            }
            *nameOut = active->getName();
            return !nameOut->empty();
        }
        catch (...)
        {
            return false;
        }
    }

    __declspec(noinline) bool CallStationPlatoonStringId(
        Platoon* platoon,
        std::string* nameOut)
    {
        try
        {
            if (platoon == NULL || nameOut == NULL)
            {
                return false;
            }
            *nameOut = platoon->stringID;
            return !nameOut->empty();
        }
        catch (...)
        {
            return false;
        }
    }

    bool TryReadStationPlatoonHeader(
        Platoon* platoon,
        hand* handleOut,
        ActivePlatoon** activeOut,
        int* unloadedCountOut,
        short* orderOut)
    {
        if (platoon == NULL || handleOut == NULL || activeOut == NULL ||
            unloadedCountOut == NULL || orderOut == NULL)
        {
            return false;
        }
        *activeOut = NULL;
        *unloadedCountOut = 0;
        *orderOut = 0;
        __try
        {
            *handleOut = platoon->getHandle();
            *activeOut = platoon->getActivePlatoon();
            *unloadedCountOut = platoon->_characterCountCurrent;
            *orderOut = platoon->index;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            handleOut->setNull();
            *activeOut = NULL;
            return false;
        }
    }

    bool TryReadStationSquadName(
        Platoon* platoon,
        ActivePlatoon* active,
        std::string* nameOut)
    {
        if (platoon == NULL || nameOut == NULL)
        {
            return false;
        }
        nameOut->clear();
        __try
        {
            if (active != NULL && CallStationActiveSquadName(active, nameOut))
            {
                return true;
            }
            return CallStationPlatoonStringId(platoon, nameOut);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            nameOut->clear();
            return false;
        }
    }

    bool TryBuildStationSquad(
        Platoon* platoon,
        StationSquadSnapshot* squadOut)
    {
        if (platoon == NULL || squadOut == NULL)
        {
            return false;
        }

        StationSquadSnapshot result;
        bool success = true;
        ActivePlatoon* active = NULL;
        int unloadedCount = 0;
        short order = 0;
        if (!TryReadStationPlatoonHeader(
                platoon, &result.handle, &active, &unloadedCount, &order))
        {
            result.queueIncomplete = true;
            result.name = "Unavailable squad";
            *squadOut = result;
            return false;
        }
        CaptureHandleIdentity(result.handle, &result.identity);
        result.loaded = active != NULL;
        if (!TryReadStationSquadName(platoon, active, &result.name))
        {
            result.name = "Unavailable squad";
            result.queueIncomplete = true;
            success = false;
        }

        if (active != NULL)
        {
            std::vector<hand> members;
            if (!TryGetOrderedMemberHandles(active, &members))
            {
                result.queueIncomplete = true;
                success = false;
            }
            else
            {
                result.members.reserve(members.size());
                for (size_t index = 0; index < members.size(); ++index)
                {
                    StationMemberSnapshot member;
                    if (!TryBuildStationMember(members[index], &member))
                    {
                        result.queueIncomplete = true;
                        success = false;
                    }
                    if (member.loaded)
                    {
                        result.members.push_back(member);
                    }
                    else
                    {
                        ++result.unavailableMemberCount;
                    }
                }
                // Active platoons can be only partly materialized.  Keep the
                // loaded members in their container order and represent the
                // remainder with the single UI placeholder required by the
                // station board.
                if (unloadedCount >= 0 && unloadedCount <= 256)
                {
                    const int loadedCount =
                        static_cast<int>(result.members.size());
                    const int missingCount = unloadedCount - loadedCount;
                    if (missingCount > result.unavailableMemberCount)
                    {
                        result.unavailableMemberCount = missingCount;
                    }
                }
                else
                {
                    result.queueIncomplete = true;
                    success = false;
                }
            }
        }
        else
        {
            result.unavailableMemberCount = unloadedCount;
            if (result.unavailableMemberCount < 0 ||
                result.unavailableMemberCount > 256)
            {
                result.unavailableMemberCount = 0;
                result.queueIncomplete = true;
                success = false;
            }
        }

        if (result.name.empty())
        {
            result.name = "Unavailable squad";
        }
        *squadOut = result;
        return success;
    }

    bool TryReadStationPlatoonListCount(
        const lektor<Platoon*>* source,
        size_t* countOut)
    {
        if (source == NULL || countOut == NULL)
        {
            return false;
        }
        *countOut = 0;
        __try
        {
            *countOut = source->size();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryReadStationPlatoonListItem(
        const lektor<Platoon*>* source,
        size_t index,
        Platoon** platoonOut)
    {
        if (source == NULL || platoonOut == NULL)
        {
            return false;
        }
        *platoonOut = NULL;
        __try
        {
            *platoonOut = (*source)[static_cast<unsigned int>(index)];
            return *platoonOut != NULL;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryCaptureStationRosterSources(
        PlayerInterface* player,
        Faction** factionOut,
        const lektor<Platoon*>** activeOut,
        const lektor<Platoon*>** unloadedOut,
        hand* deadSquadOut)
    {
        if (player == NULL || factionOut == NULL || activeOut == NULL ||
            unloadedOut == NULL || deadSquadOut == NULL)
        {
            return false;
        }
        *factionOut = NULL;
        *activeOut = NULL;
        *unloadedOut = NULL;
        deadSquadOut->setNull();
        __try
        {
            Faction* faction = player->getFaction();
            if (faction == NULL || !faction->isThePlayer())
            {
                return false;
            }
            *factionOut = faction;
            *activeOut = faction->getActivePlatoons();
            *unloadedOut = faction->getUnloadedPlatoons();
            *deadSquadOut = player->getDeadSquadHandle();
            return *activeOut != NULL && *unloadedOut != NULL;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            *factionOut = NULL;
            *activeOut = NULL;
            *unloadedOut = NULL;
            deadSquadOut->setNull();
            return false;
        }
    }

    bool StationPlatoonReferenceLess(
        const StationPlatoonReference& left,
        const StationPlatoonReference& right)
    {
        if (left.order != right.order)
        {
            return left.order < right.order;
        }
        return left.sourceOrder < right.sourceOrder;
    }

    void AppendStationPlatoonReferences(
        const lektor<Platoon*>* source,
        const HandleIdentity& deadSquad,
        unsigned int* sourceOrder,
        std::vector<StationPlatoonReference>* referencesOut,
        bool* incompleteOut)
    {
        size_t count = 0;
        if (!TryReadStationPlatoonListCount(source, &count) || count > 256)
        {
            *incompleteOut = true;
            return;
        }
        for (size_t index = 0; index < count; ++index)
        {
            Platoon* platoon = NULL;
            if (!TryReadStationPlatoonListItem(source, index, &platoon))
            {
                *incompleteOut = true;
                continue;
            }
            hand handle;
            ActivePlatoon* active = NULL;
            int unloadedCount = 0;
            short order = 0;
            if (!TryReadStationPlatoonHeader(
                    platoon, &handle, &active, &unloadedCount, &order))
            {
                *incompleteOut = true;
                continue;
            }
            HandleIdentity identity;
            CaptureHandleIdentity(handle, &identity);
            if (SameHandleIdentity(identity, deadSquad))
            {
                continue;
            }
            StationPlatoonReference reference;
            reference.platoon = platoon;
            reference.order = order;
            reference.sourceOrder = (*sourceOrder)++;
            referencesOut->push_back(reference);
        }
    }

    bool TryBuildStationRoster(
        PlayerInterface* player,
        std::vector<StationSquadSnapshot>* squadsOut,
        bool* incompleteOut)
    {
        if (player == NULL || squadsOut == NULL || incompleteOut == NULL)
        {
            return false;
        }

        squadsOut->clear();
        *incompleteOut = false;
        Faction* faction = NULL;
        const lektor<Platoon*>* active = NULL;
        const lektor<Platoon*>* unloaded = NULL;
        hand deadSquadHandle;
        if (!TryCaptureStationRosterSources(
                player, &faction, &active, &unloaded, &deadSquadHandle))
        {
            *incompleteOut = true;
            return false;
        }
        HandleIdentity deadSquad;
        CaptureHandleIdentity(deadSquadHandle, &deadSquad);
        std::vector<StationPlatoonReference> references;
        unsigned int sourceOrder = 0;
        AppendStationPlatoonReferences(
            active, deadSquad, &sourceOrder, &references, incompleteOut);
        AppendStationPlatoonReferences(
            unloaded, deadSquad, &sourceOrder, &references, incompleteOut);
        std::stable_sort(
            references.begin(), references.end(), StationPlatoonReferenceLess);
        for (size_t index = 0; index < references.size(); ++index)
        {
            StationSquadSnapshot squad;
            if (!TryBuildStationSquad(references[index].platoon, &squad))
            {
                *incompleteOut = true;
            }
            if (squad.name == "__DEAD__")
            {
                continue;
            }
            squadsOut->push_back(squad);
        }
        return !squadsOut->empty() || !*incompleteOut;
    }

    void CollectAssignedStationTargets(
        const std::vector<StationSquadSnapshot>& squads,
        std::vector<HandleIdentity>* targetsOut)
    {
        targetsOut->clear();
        for (size_t squadIndex = 0; squadIndex < squads.size(); ++squadIndex)
        {
            const StationSquadSnapshot& squad = squads[squadIndex];
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                const StationMemberSnapshot& member = squad.members[memberIndex];
                for (size_t jobIndex = 0; jobIndex < member.jobs.size(); ++jobIndex)
                {
                    const StationQueuedJobSnapshot& job = member.jobs[jobIndex];
                    if (IsStationDisplayJob(job.taskType) &&
                        job.hasTarget &&
                        !StationIdentityInList(job.target, *targetsOut))
                    {
                        targetsOut->push_back(job.target);
                    }
                }
            }
        }
    }

    bool SameStationBuildingHandle(const hand& left, const hand& right)
    {
        return left.type == right.type &&
            left.container == right.container &&
            left.containerSerial == right.containerSerial &&
            left.index == right.index &&
            left.serial == right.serial;
    }

    bool AppendUniqueStationBuildingHandle(
        const hand& candidate,
        std::vector<hand>* handlesOut)
    {
        if (handlesOut == NULL || candidate.type != BUILDING)
        {
            return true;
        }
        for (size_t index = 0; index < handlesOut->size(); ++index)
        {
            if (SameStationBuildingHandle((*handlesOut)[index], candidate))
            {
                return true;
            }
        }
        if (handlesOut->size() >= STATION_SCAN_TARGET_LIMIT)
        {
            return false;
        }
        handlesOut->push_back(candidate);
        return true;
    }

    void AppendAssignedStationBuildingHandles(
        const std::vector<HandleIdentity>& assignedTargets,
        std::vector<hand>* handlesOut,
        bool* incompleteOut)
    {
        for (size_t index = 0; index < assignedTargets.size(); ++index)
        {
            const HandleIdentity& identity = assignedTargets[index];
            if (!identity.valid || identity.type != BUILDING)
            {
                continue;
            }
            const hand candidate(
                identity.index, identity.serial, identity.type,
                identity.container, identity.containerSerial);
            if (!AppendUniqueStationBuildingHandle(candidate, handlesOut))
            {
                *incompleteOut = true;
                return;
            }
        }
    }

    bool TryClassifyStationBuilding(
        Building* building,
        StationCategory* categoryOut,
        StatsEnumerated* statOut,
        bool* statKnownOut,
        bool* naturalOut,
        bool* relevantOut)
    {
        if (building == NULL || categoryOut == NULL || statOut == NULL ||
            statKnownOut == NULL || naturalOut == NULL || relevantOut == NULL)
        {
            return false;
        }

        *categoryOut = STATION_OTHER;
        *statOut = STAT_NONE;
        *statKnownOut = false;
        *naturalOut = false;
        *relevantOut = false;

        __try
        {
            const BuildingClassType classType = building->getBuildingClass();
            const BuildingFunction function = building->getSpecialFunction();
            *naturalOut = function == BF_MINE_NATURAL;

            switch (classType)
            {
            case BCTYPE_CRAFTING:
                *categoryOut = STATION_CRAFTING;
                *relevantOut = true;
                break;
            case BCTYPE_FARM:
                *categoryOut = STATION_FARMING;
                *statOut = STAT_FARMING;
                *statKnownOut = true;
                *relevantOut = true;
                break;
            case BCTYPE_RESEARCH:
                *categoryOut = STATION_RESEARCH;
                *statOut = STAT_SCIENCE;
                *statKnownOut = true;
                *relevantOut = true;
                break;
            case BCTYPE_TURRET:
                *categoryOut = STATION_DEFENSE;
                *statOut = STAT_TURRETS;
                *statKnownOut = true;
                *relevantOut = true;
                break;
            case BCTYPE_STORAGE:
                *categoryOut = STATION_STORAGE_HAULING;
                *relevantOut = true;
                break;
            default:
                break;
            }

            switch (function)
            {
            case BF_MINE:
            case BF_MINE_NATURAL:
                *categoryOut = STATION_MINING;
                *statOut = STAT_LABOURING;
                *statKnownOut = true;
                *relevantOut = true;
                break;
            case BF_REFINERY:
            case BF_ITEM_FURNACE:
                *categoryOut = STATION_REFINING;
                *statOut = STAT_LABOURING;
                *statKnownOut = true;
                *relevantOut = true;
                break;
            case BF_RESEARCH:
                *categoryOut = STATION_RESEARCH;
                *statOut = STAT_SCIENCE;
                *statKnownOut = true;
                *relevantOut = true;
                break;
            case BF_TRAINING:
                *categoryOut = STATION_TRAINING;
                *relevantOut = true;
                break;
            case BF_RESOURCE_STORAGE:
            case BF_GENERAL_STORAGE:
                *categoryOut = STATION_STORAGE_HAULING;
                *relevantOut = true;
                break;
            case BF_TURRET:
                *categoryOut = STATION_DEFENSE;
                *statOut = STAT_TURRETS;
                *statKnownOut = true;
                *relevantOut = true;
                break;
            case BF_CRAFTING:
                *categoryOut = STATION_CRAFTING;
                *relevantOut = true;
                break;
            default:
                break;
            }

            UseableStuff* usable = building->getUseableStuff();
            if (usable != NULL)
            {
                const StatsEnumerated engineStat = usable->getStatUsed();
                if (engineStat > STAT_NONE && engineStat < STAT_END)
                {
                    *statOut = engineStat;
                    *statKnownOut = true;
                }
                if (usable->getDefaultTask() != NULL_TASK)
                {
                    *relevantOut = true;
                }
            }
            // The base Building virtual also covers modded or otherwise
            // unclassified work targets whose concrete class is not exposed as
            // UseableStuff by the reconstructed headers.
            if (building->getDefaultTask() != NULL_TASK)
            {
                *relevantOut = true;
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool StationStringIdEquals(const GameData* data, const char* expected)
    {
        return data != NULL && expected != NULL &&
            std::strcmp(data->stringID.c_str(), expected) == 0;
    }

    __declspec(noinline) StationVisualSubtype CallReadStationVisualSubtype(
        Building* building,
        bool natural)
    {
        if (building == NULL)
        {
            return STATION_VISUAL_DEFAULT;
        }

        GameData* buildingData = building->getGameData();
        UseableStuff* usable = building->getUseableStuff();
        GameData* functionalityData =
            usable == NULL ? NULL : usable->getFunctionalityData();

        // These are exact BUILDING_FUNCTIONALITY records for copper and iron
        // extraction.  They apply to both natural nodes and player machines
        // that produce the same resource.  The artwork represents the output
        // material, not the placed model, so an Ore Drill correctly keeps the
        // iron-ore symbol.
        if (StationStringIdEquals(
                functionalityData, "42251-gamedata.base"))
        {
            return STATION_VISUAL_COPPER_ORE;
        }
        if (StationStringIdEquals(
                functionalityData, "1869-gamedata.base"))
        {
            return STATION_VISUAL_IRON_ORE;
        }

        // Exact placed-building fallbacks cover natural nodes whose
        // reconstructed UseableStuff metadata is unavailable.  Only accept
        // these IDs when Kenshi itself marked the target as a natural mine.
        if (natural &&
            (StationStringIdEquals(buildingData, "42249-gamedata.base") ||
             StationStringIdEquals(buildingData, "99106-Boneyard Wolf.mod")))
        {
            return STATION_VISUAL_COPPER_ORE;
        }
        if (natural &&
            (StationStringIdEquals(buildingData, "42248-gamedata.base") ||
             StationStringIdEquals(buildingData, "99107-Boneyard Wolf.mod")))
        {
            return STATION_VISUAL_IRON_ORE;
        }

        // Use exact stable FCS IDs.  Do not use the live display name: a
        // player may rename any of these buildings without changing its
        // function or the icon that represents it.
        const GameData* records[2] = { functionalityData, buildingData };
        for (size_t index = 0; index < 2; ++index)
        {
            const GameData* data = records[index];
            if (StationStringIdEquals(
                    data,
                    "5038571-Universal Wasteland Expansion.mod"))
            {
                return STATION_VISUAL_COPPER_PLATES;
            }
            if (StationStringIdEquals(data, "42170-gamedata.base"))
            {
                return STATION_VISUAL_IRON_PLATES;
            }
            if (StationStringIdEquals(data, "2117-gamedata.base"))
            {
                return STATION_VISUAL_STEEL_BARS;
            }
            if (StationStringIdEquals(data, "50019-rebirth.mod"))
            {
                return STATION_VISUAL_COPPER_ALLOY_PLATES;
            }
            if (StationStringIdEquals(data, "43925-rebirth.mod"))
            {
                return STATION_VISUAL_ELECTRONICS;
            }
            if (StationStringIdEquals(data, "96449-crafting.mod"))
            {
                return STATION_VISUAL_CROSSBOW;
            }
            if (StationStringIdEquals(data, "96529-crafting.mod"))
            {
                return STATION_VISUAL_SKELETON_LIMBS;
            }
        }

        // Exact placed-building fallbacks keep the specialized art when a
        // reconstructed or mod-overridden building does not expose its
        // functionality record. These are immutable FCS identities, not the
        // player's renameable instance name.
        if (StationStringIdEquals(
                buildingData,
                "5038572-Universal Wasteland Expansion.mod") ||
            StationStringIdEquals(
                buildingData,
                "5038573-Universal Wasteland Expansion.mod") ||
            StationStringIdEquals(
                buildingData,
                "5038574-Universal Wasteland Expansion.mod") ||
            StationStringIdEquals(
                buildingData,
                "5038575-Universal Wasteland Expansion.mod") ||
            StationStringIdEquals(
                buildingData,
                "5038576-Universal Wasteland Expansion.mod") ||
            StationStringIdEquals(
                buildingData,
                "5038577-Universal Wasteland Expansion.mod"))
        {
            return STATION_VISUAL_COPPER_PLATES;
        }
        if (StationStringIdEquals(buildingData, "47237-rebirth.mod") ||
            StationStringIdEquals(buildingData, "42167-gamedata.base") ||
            StationStringIdEquals(buildingData, "42168-gamedata.base") ||
            StationStringIdEquals(buildingData, "42169-gamedata.base") ||
            StationStringIdEquals(buildingData, "1532831-rebirth.mod"))
        {
            return STATION_VISUAL_IRON_PLATES;
        }
        if (StationStringIdEquals(buildingData, "2116-gamedata.base") ||
            StationStringIdEquals(buildingData, "2118-gamedata.base") ||
            StationStringIdEquals(buildingData, "2119-gamedata.base"))
        {
            return STATION_VISUAL_STEEL_BARS;
        }
        if (StationStringIdEquals(buildingData, "50018-rebirth.mod"))
        {
            return STATION_VISUAL_COPPER_ALLOY_PLATES;
        }
        if (StationStringIdEquals(buildingData, "42166-gamedata.base"))
        {
            return STATION_VISUAL_ELECTRONICS;
        }
        if (StationStringIdEquals(buildingData, "96183-Newwworld.mod"))
        {
            return STATION_VISUAL_CROSSBOW;
        }
        if (StationStringIdEquals(buildingData, "96365-Newwworld.mod"))
        {
            return STATION_VISUAL_SKELETON_LIMBS;
        }
        return STATION_VISUAL_DEFAULT;
    }

    bool TryReadStationVisualSubtype(
        Building* building,
        bool natural,
        StationVisualSubtype* subtypeOut)
    {
        if (building == NULL || subtypeOut == NULL)
        {
            return false;
        }
        int subtypeValue = static_cast<int>(STATION_VISUAL_DEFAULT);
        bool faulted = false;
        __try
        {
            subtypeValue = static_cast<int>(
                CallReadStationVisualSubtype(building, natural));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            faulted = true;
        }
        if (faulted || subtypeValue < 0 ||
            subtypeValue >= static_cast<int>(STATION_VISUAL_COUNT))
        {
            *subtypeOut = STATION_VISUAL_DEFAULT;
            return false;
        }
        *subtypeOut = static_cast<StationVisualSubtype>(subtypeValue);
        return true;
    }

    bool TryIsPlayerManagedStation(
        Building* building,
        bool* manageableOut)
    {
        if (building == NULL || manageableOut == NULL)
        {
            return false;
        }
        *manageableOut = false;
        __try
        {
            // Natural resource nodes cannot be player-owned, but Kenshi lets
            // player queues target them directly. They are the one deliberate
            // exception to the player-owned station rule.
            const bool natural =
                building->getSpecialFunction() == BF_MINE_NATURAL;
            *manageableOut = natural || building->isThePlayer();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            *manageableOut = false;
            return false;
        }
    }

    bool TryGetStationBlockingStatus(
        Building* building,
        bool* destroyedOut,
        bool* blockingOut,
        std::string* statusOut)
    {
        if (building == NULL || destroyedOut == NULL ||
            blockingOut == NULL || statusOut == NULL)
        {
            return false;
        }
        *destroyedOut = false;
        *blockingOut = false;
        statusOut->clear();

        bool incomplete = false;
        bool dismantling = false;
        bool broken = false;
        bool disabled = false;
        bool noPower = false;
        __try
        {
            if (building->isDestroyed())
            {
                *destroyedOut = true;
                return true;
            }

            Building::ConstructionState* build = building->getBuildState();
            if (build != NULL)
            {
                incomplete = !build->isComplete;
                dismantling = build->isDismantled;
            }

            UseableStuff* usable = building->getUseableStuff();
            if (usable != NULL)
            {
                broken = usable->isBroken();
                disabled = usable->isDisabled();
                noPower = usable->isOutOfPower() > 0.001f;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }

        if (incomplete) *statusOut = "INCOMPLETE";
        if (dismantling)
        {
            if (!statusOut->empty()) *statusOut += " / ";
            *statusOut += "DISMANTLING";
        }
        if (broken)
        {
            if (!statusOut->empty()) *statusOut += " / ";
            *statusOut += "BROKEN";
        }
        if (disabled)
        {
            if (!statusOut->empty()) *statusOut += " / ";
            *statusOut += "DISABLED";
        }
        if (noPower)
        {
            if (!statusOut->empty()) *statusOut += " / ";
            *statusOut += "NO POWER";
        }
        *blockingOut = !statusOut->empty();
        return true;
    }

    void BuildWildernessStationAreaName(std::string* nameOut)
    {
        *nameOut = "Wilderness";
    }

    bool TryGetStationArea(
        Building* building,
        HandleIdentity* identityOut,
        std::string* nameOut)
    {
        if (building == NULL || identityOut == NULL || nameOut == NULL)
        {
            return false;
        }
        ResetHandleIdentity(identityOut);
        nameOut->clear();

        __try
        {
            TownBase* town = building->getCurrentTownLocation();
            if (town != NULL)
            {
                CaptureHandleIdentity(town->getHandle(), identityOut);
                TryReadRootObjectName(town, nameOut);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ResetHandleIdentity(identityOut);
            nameOut->clear();
        }

        if (nameOut->empty())
        {
            BuildWildernessStationAreaName(nameOut);
        }
        return true;
    }

    const StationBaseStatSnapshot* FindStationMemberStat(
        const StationMemberSnapshot& member,
        StatsEnumerated stat)
    {
        for (size_t index = 0; index < member.baseStats.size(); ++index)
        {
            if (member.baseStats[index].stat == stat)
            {
                return &member.baseStats[index];
            }
        }
        return NULL;
    }

    void JoinStationAssignments(
        const std::vector<StationSquadSnapshot>& squads,
        StationTargetSnapshot* station)
    {
        if (station == NULL)
        {
            return;
        }
        station->assignments.clear();

        for (size_t squadIndex = 0; squadIndex < squads.size(); ++squadIndex)
        {
            const StationSquadSnapshot& squad = squads[squadIndex];
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                const StationMemberSnapshot& member = squad.members[memberIndex];
                for (size_t jobIndex = 0; jobIndex < member.jobs.size(); ++jobIndex)
                {
                    const StationQueuedJobSnapshot& job = member.jobs[jobIndex];
                    if (!IsStationDisplayJob(job.taskType) ||
                        !job.hasTarget ||
                        !SameHandleIdentity(job.target, station->identity))
                    {
                        continue;
                    }

                    StationAssignmentSnapshot assignment;
                    assignment.exactJob = job.exactJob;
                    assignment.squad = squad.identity;
                    assignment.member = member.identity;
                    assignment.squadName = squad.name;
                    assignment.memberName = member.name;
                    assignment.taskType = job.taskType;
                    assignment.jobLabel = job.jobLabel;
                    assignment.priority = job.priority;
                    assignment.relevantStat = station->relevantStat;

                    if (station->relevantSkillKnown)
                    {
                        const StationBaseStatSnapshot* stat =
                            FindStationMemberStat(member, station->relevantStat);
                        if (stat != NULL)
                        {
                            assignment.relevantSkillKnown = true;
                            assignment.relevantSkillName = stat->name;
                            assignment.relevantSkillValue = stat->displayValue;
                        }
                    }
                    station->assignments.push_back(assignment);
                }
            }
        }
    }

    void CopyMemberQueueIntoStationProjection(
        const MemberSnapshot& source,
        StationMemberSnapshot* destination)
    {
        if (destination == NULL)
        {
            return;
        }

        // Keep baseStats from the station-roster snapshot.  The manager owns
        // Kenshi's pause while it is open, and a queue transfer cannot change
        // those raw stats.  Everything derived from the verified live queue
        // is replaced so the projection matches the successful mutation.
        destination->identity = source.identity;
        destination->handle = source.handle;
        destination->name = source.name;
        destination->condition = source.condition;
        destination->loaded = source.loaded;
        destination->queueAvailable = source.queueAvailable;
        destination->truncated = source.truncated;
        destination->jobsEnabled = source.jobsEnabled;
        destination->permanentJobCount =
            static_cast<int>(source.jobs.size());
        destination->topSkills = source.skills;
        destination->jobs.clear();
        destination->jobs.reserve(source.jobs.size());
        for (size_t slot = 0; slot < source.jobs.size(); ++slot)
        {
            const JobRowSnapshot& sourceJob = source.jobs[slot];
            StationQueuedJobSnapshot job;
            job.exactJob = sourceJob;
            job.taskType = sourceJob.taskType;
            job.jobLabel = sourceJob.jobLabel;
            job.targetLabel = sourceJob.targetLabel;
            job.target = sourceJob.target;
            job.hasTarget = sourceJob.hasTarget;
            job.targetAvailable = sourceJob.targetAvailable;
            job.priority = static_cast<int>(slot) + 1;
            destination->jobs.push_back(job);
        }
    }

    bool TryPatchStationTransferProjection(
        StationScanState* state,
        const MemberSnapshot& sourceAfter,
        const MemberSnapshot& destinationAfter)
    {
        if (state == NULL || !state->started ||
            !sourceAfter.identity.valid || !destinationAfter.identity.valid ||
            SameHandleIdentity(
                sourceAfter.identity, destinationAfter.identity))
        {
            return false;
        }

        StationMemberSnapshot* projectedSource = NULL;
        StationMemberSnapshot* projectedDestination = NULL;
        for (size_t squadIndex = 0; squadIndex < state->squads.size();
             ++squadIndex)
        {
            StationSquadSnapshot& squad = state->squads[squadIndex];
            for (size_t memberIndex = 0; memberIndex < squad.members.size();
                 ++memberIndex)
            {
                StationMemberSnapshot& member = squad.members[memberIndex];
                if (SameHandleIdentity(member.identity, sourceAfter.identity))
                {
                    projectedSource = &member;
                }
                else if (SameHandleIdentity(
                             member.identity, destinationAfter.identity))
                {
                    projectedDestination = &member;
                }
            }
        }
        if (projectedSource == NULL || projectedDestination == NULL)
        {
            return false;
        }

        CopyMemberQueueIntoStationProjection(
            sourceAfter, projectedSource);
        CopyMemberQueueIntoStationProjection(
            destinationAfter, projectedDestination);
        for (size_t stationIndex = 0; stationIndex < state->stations.size();
             ++stationIndex)
        {
            JoinStationAssignments(
                state->squads, &state->stations[stationIndex]);
        }
        return true;
    }

    bool TryFindAssignedStationTargetLabel(
        const std::vector<StationSquadSnapshot>& squads,
        const HandleIdentity& identity,
        std::string* labelOut)
    {
        if (labelOut == NULL)
        {
            return false;
        }
        labelOut->clear();
        for (size_t squadIndex = 0; squadIndex < squads.size(); ++squadIndex)
        {
            const StationSquadSnapshot& squad = squads[squadIndex];
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                const StationMemberSnapshot& member = squad.members[memberIndex];
                for (size_t jobIndex = 0; jobIndex < member.jobs.size(); ++jobIndex)
                {
                    const StationQueuedJobSnapshot& job = member.jobs[jobIndex];
                    if (IsStationDisplayJob(job.taskType) &&
                        job.hasTarget &&
                        SameHandleIdentity(job.target, identity))
                    {
                        if (!job.targetLabel.empty() &&
                            job.targetLabel != "Target unavailable")
                        {
                            *labelOut = job.targetLabel;
                        }
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool TryReadStationBuildingIdentity(
        Building* building,
        hand* handleOut)
    {
        if (building == NULL || handleOut == NULL)
        {
            return false;
        }
        handleOut->setNull();
        __try
        {
            *handleOut = building->getHandle();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            handleOut->setNull();
            return false;
        }
    }

    bool TrySnapshotStationBuilding(
        Building* building,
        const std::vector<StationSquadSnapshot>& squads,
        StationTargetSnapshot* stationOut,
        bool* includeOut)
    {
        if (building == NULL || stationOut == NULL || includeOut == NULL)
        {
            return false;
        }
        *includeOut = false;

        StationTargetSnapshot station;
        if (!TryReadStationBuildingIdentity(building, &station.handle))
        {
            return false;
        }
        CaptureHandleIdentity(station.handle, &station.identity);
        if (!station.identity.valid)
        {
            return true;
        }

        std::string queueTargetLabel;
        if (!TryFindAssignedStationTargetLabel(
                squads, station.identity, &queueTargetLabel))
        {
            return true;
        }

        bool playerManaged = false;
        if (!TryIsPlayerManagedStation(building, &playerManaged) ||
            !playerManaged)
        {
            // Fail closed. A public or city building referenced by a queue is
            // not part of the player's station board.
            return true;
        }

        bool relevant = false;
        bool natural = false;
        if (!TryClassifyStationBuilding(
                building, &station.category, &station.relevantStat,
                &station.relevantSkillKnown, &natural, &relevant))
        {
            // The exact queue assignment remains useful even when optional
            // building metadata cannot be read.  Keep the target visible in
            // the catch-all category instead of dropping the whole column.
            station.category = STATION_OTHER;
            station.relevantStat = STAT_NONE;
            station.relevantSkillKnown = false;
            relevant = false;
        }

        if (!relevant)
        {
            station.category = STATION_OTHER;
            station.relevantSkillKnown = false;
            station.relevantStat = STAT_NONE;
        }
        station.naturalResourceException = natural;
        TryReadStationVisualSubtype(
            building, natural, &station.visualSubtype);

        bool destroyed = false;
        if (!TryGetStationBlockingStatus(
                building, &destroyed, &station.blocking,
                &station.blockingStatus))
        {
            station.blocking = false;
            station.blockingStatus.clear();
        }
        if (destroyed && station.blockingStatus.empty())
        {
            station.blocking = true;
            station.blockingStatus = "Destroyed";
        }

        if (!TryReadRootObjectName(building, &station.name) ||
            station.name.empty())
        {
            station.name = queueTargetLabel.empty() ?
                "Unnamed assigned target" : queueTargetLabel;
        }
        TryGetStationArea(
            building, &station.areaIdentity, &station.areaName);

        if (station.relevantSkillKnown &&
            !TryGetLocalizedStatName(
                station.relevantStat, &station.relevantSkillName))
        {
            station.relevantSkillName = "Unknown";
        }
        else if (!station.relevantSkillKnown)
        {
            station.relevantSkillName =
                station.category == STATION_OTHER ? "Unknown" : "None";
        }

        *stationOut = station;
        *includeOut = true;
        return true;
    }

    bool TryResolveStationBuilding(
        const hand& handle,
        Building** buildingOut)
    {
        if (buildingOut == NULL)
        {
            return false;
        }
        *buildingOut = NULL;
        hand resolvedHandle;
        __try
        {
            if (handle.type != BUILDING || !handle.isValid())
            {
                return true;
            }
            Building* building = handle.getBuilding();
            if (building == NULL)
            {
                return true;
            }
            resolvedHandle = building->getHandle();
            if (resolvedHandle.type != handle.type ||
                resolvedHandle.container != handle.container ||
                resolvedHandle.containerSerial != handle.containerSerial ||
                resolvedHandle.index != handle.index ||
                resolvedHandle.serial != handle.serial)
            {
                return true;
            }
            *buildingOut = building;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            *buildingOut = NULL;
            return false;
        }
    }

    bool TryScanStationTarget(
        const hand& buildingHandle,
        StationScanState* state)
    {
        if (state == NULL)
        {
            return false;
        }

        Building* building = NULL;
        const bool resolvedSafely =
            TryResolveStationBuilding(buildingHandle, &building);
        if (!resolvedSafely || building == NULL)
        {
            // Ownership cannot be proven for an unloaded or faulting handle.
            // Fail closed instead of exposing a public/city target as if it
            // were a player station. Squad Jobs still shows the exact
            // unavailable assignment and keeps its normal red warning.
            return false;
        }

        StationTargetSnapshot station;
        bool include = false;
        if (!TrySnapshotStationBuilding(
                building, state->squads, &station, &include))
        {
            return false;
        }
        if (!include ||
            StationTargetAlreadyScanned(station.identity, state->stations))
        {
            return true;
        }
        // Only report truncation after a 2,049th qualifying unique target is
        // observed.  Remaining non-station objects do not make the result
        // incomplete.
        if (state->stations.size() >= STATION_SCAN_TARGET_LIMIT)
        {
            state->truncated = true;
            state->complete = true;
            return true;
        }
        JoinStationAssignments(state->squads, &station);
        state->stations.push_back(station);
        return true;
    }

    bool BeginStationScan(
        GameWorld* world,
        PlayerInterface* player,
        StationScanState* state)
    {
        if (state == NULL)
        {
            return false;
        }
        *state = StationScanState();
        state->started = true;
        (void)world;

        if (!TryBuildStationRoster(
                player, &state->squads, &state->rosterIncomplete))
        {
            state->rosterIncomplete = true;
            AddStationScanError(
                state, "Player squad roster could not be read completely.");
        }
        // The board is intentionally assignment-derived.  Do not enumerate
        // faction ownership, zones, towns, or unrelated world buildings.
        std::vector<HandleIdentity> assignedTargets;
        CollectAssignedStationTargets(state->squads, &assignedTargets);
        bool assignedTargetIncomplete = false;
        AppendAssignedStationBuildingHandles(
            assignedTargets, &state->assignedTargetHandles,
            &assignedTargetIncomplete);
        if (assignedTargetIncomplete)
        {
            AddStationScanError(
                state,
                "Assigned station target list reached its safety limit.");
        }

        state->complete = state->assignedTargetHandles.empty();
        return true;
    }

    bool StepStationScan(
        GameWorld* world,
        PlayerInterface* player,
        StationScanState* state)
    {
        if (state == NULL || !state->started || state->complete)
        {
            return false;
        }
        (void)world;
        (void)player;
        if (state->nextTarget >= state->assignedTargetHandles.size())
        {
            state->complete = true;
            return true;
        }

        const hand target = state->assignedTargetHandles[state->nextTarget];
        ++state->nextTarget;

        if (!TryScanStationTarget(target, state))
        {
            ++state->targetsFailed;
            AddStationScanError(
                state, "An assigned station target could not be read safely.");
        }
        ++state->targetsCompleted;
        SortStationTargetsForDisplay(state);
        if (state->nextTarget >= state->assignedTargetHandles.size() ||
            state->truncated)
        {
            state->complete = true;
        }
        return true;
    }

    bool RefreshStationAssignments(
        PlayerInterface* player,
        StationScanState* state)
    {
        if (player == NULL || state == NULL || !state->started)
        {
            return false;
        }

        std::vector<StationSquadSnapshot> freshSquads;
        bool incomplete = false;
        if (!TryBuildStationRoster(player, &freshSquads, &incomplete))
        {
            state->rosterIncomplete = true;
            AddStationScanError(
                state, "Station assignments could not be refreshed.");
            return false;
        }
        state->squads.swap(freshSquads);
        state->rosterIncomplete = incomplete;
        state->stations.clear();
        state->assignedTargetHandles.clear();
        state->errors.clear();
        state->nextTarget = 0;
        state->targetsCompleted = 0;
        state->targetsFailed = 0;
        state->truncated = false;
        if (incomplete)
        {
            AddStationScanError(
                state, "Some loaded squad job data is unavailable.");
        }

        std::vector<HandleIdentity> assignedTargets;
        CollectAssignedStationTargets(state->squads, &assignedTargets);
        bool targetListIncomplete = false;
        AppendAssignedStationBuildingHandles(
            assignedTargets, &state->assignedTargetHandles,
            &targetListIncomplete);
        if (targetListIncomplete)
        {
            AddStationScanError(
                state, "Assigned station target list reached its safety limit.");
        }
        state->complete = state->assignedTargetHandles.empty();
        return true;
    }

#endif
