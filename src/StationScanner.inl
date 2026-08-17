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
// The station board merges two exact sources: player-owned BUILDING records
// copied from the player's borrowed faction ownership storage, and exact
// permanent-job targets found in readable loaded-player queues.  It never
// enumerates zones or unrelated world buildings.  Ownership records are copied
// as plug-in-owned scalar POD, then one local handle is reconstructed per UI
// call.  Building, Platoon, and Character pointers never survive their guarded
// leaf operation.  BeginStationScan() takes both value snapshots, and
// StepStationScan() enriches no more than one candidate per guarded call.

#ifndef KENSHI_JOB_MANAGEMENT_STATION_SCANNER_INL
#define KENSHI_JOB_MANAGEMENT_STATION_SCANNER_INL

    // The final board cap is intentionally separate from the borrowed source
    // copy cap.  Ownership records can include non-work buildings, so the
    // source pass must be allowed to inspect more records than the board can
    // ultimately display.
    const size_t STATION_SCAN_TARGET_LIMIT = 2048;
    const unsigned int STATION_OWNERSHIP_COPY_LIMIT = 8192;
    // Each guarded call remains bounded to one candidate. The Stations button
    // runs the finite copied candidate list synchronously, then builds the grid
    // once. Publish normalized results in small value-only batches internally
    // so vector growth remains bounded without exposing partial UI state.
    const size_t STATION_SCAN_PRESENTATION_BATCH_SIZE = 16;

    // These offsets are part of the reconstructed KenshiLib 0.4.0 ABI.  The
    // ownership source is read as a borrowed array of scalar records only;
    // fail the build if a dependency update changes the layout underneath the
    // guarded copy path.
    typedef char StationScannerAssertLektorSize[
        (sizeof(lektor<hand>) == 0x18) ? 1 : -1];
    typedef char StationScannerAssertLektorCount[
        (offsetof(lektor<hand>, count) == 0x08) ? 1 : -1];
    typedef char StationScannerAssertLektorMax[
        (offsetof(lektor<hand>, maxSize) == 0x0C) ? 1 : -1];
    typedef char StationScannerAssertLektorStuff[
        (offsetof(lektor<hand>, stuff) == 0x10) ? 1 : -1];
    typedef char StationScannerAssertHandSize[
        (sizeof(hand) == 0x20) ? 1 : -1];
    typedef char StationScannerAssertItemTypeSize[
        (sizeof(itemType) == 0x04) ? 1 : -1];
    typedef char StationScannerAssertHandType[
        (offsetof(hand, type) == 0x08) ? 1 : -1];
    typedef char StationScannerAssertHandContainer[
        (offsetof(hand, container) == 0x0C) ? 1 : -1];
    typedef char StationScannerAssertHandContainerSerial[
        (offsetof(hand, containerSerial) == 0x10) ? 1 : -1];
    typedef char StationScannerAssertHandIndex[
        (offsetof(hand, index) == 0x14) ? 1 : -1];
    typedef char StationScannerAssertHandSerial[
        (offsetof(hand, serial) == 0x18) ? 1 : -1];

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
        bool assignmentSupported;
        bool relevantSkillKnown;
        StatsEnumerated relevantStat;
        std::string relevantSkillName;
        bool blockingStatusKnown;
        bool blocking;
        std::string blockingStatus;
        std::vector<StationAssignmentSnapshot> assignments;

        StationTargetSnapshot() :
            zoneX(0), zoneY(0), category(STATION_OTHER),
            visualSubtype(STATION_VISUAL_DEFAULT),
            naturalResourceException(false), assignmentSupported(false),
            relevantSkillKnown(false),
            relevantStat(STAT_NONE), blockingStatusKnown(false),
            blocking(false)
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

    // A copied ownership record contains only the hand vtable address and
    // the five scalar identity fields.  The vtable is a numeric validation
    // token; no source pointer or engine-owned container is retained.
    struct StationOwnedHandRecord
    {
        ULONG_PTR vtable;
        unsigned int type;
        unsigned int container;
        unsigned int containerSerial;
        unsigned int index;
        unsigned int serial;
    };

    struct StationOwnershipHeaderSnapshot
    {
        ULONG_PTR player;
        ULONG_PTR faction;
        ULONG_PTR ownerships;
        ULONG_PTR sourceObject;
        ULONG_PTR sourceVtable;
        unsigned int count;
        unsigned int maxSize;
        ULONG_PTR stuff;
    };

    enum StationOwnedRecordResult
    {
        STATION_OWNED_RECORD_OK,
        STATION_OWNED_RECORD_UNLOADED,
        STATION_OWNED_RECORD_FAULT
    };

    struct StationOwnedBuildingResolution
    {
        bool constructorMatches;
        bool handValid;
        bool buildingFound;
        bool handleMatches;
        bool playerOwned;
        StationOwnedRecordResult result;

        StationOwnedBuildingResolution() :
            constructorMatches(false), handValid(false),
            buildingFound(false), handleMatches(false), playerOwned(false),
            result(STATION_OWNED_RECORD_UNLOADED)
        {
        }
    };

    struct StationScanState
    {
        bool started;
        bool complete;
        bool truncated;
        bool rosterIncomplete;
        bool ownershipCopyTruncated;
        bool ownershipResolutionIncomplete;
        size_t nextTarget;
        size_t targetsCompleted;
        size_t targetsFailed;
        std::vector<StationOwnedHandRecord> ownedBuildingRecords;
        std::vector<hand> assignedTargetHandles;
        std::vector<StationTargetSnapshot> pendingStations;
        std::vector<StationSquadSnapshot> squads;
        std::vector<StationTargetSnapshot> stations;
        std::vector<std::string> errors;

        StationScanState() :
            started(false), complete(false), truncated(false),
            rosterIncomplete(false), ownershipCopyTruncated(false),
            ownershipResolutionIncomplete(false),
            nextTarget(0), targetsCompleted(0), targetsFailed(0)
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
        // queue; exclude them from the station projection and station-scoped
        // bundle removal because their incidental subject is not authoritative.
        return taskType != JOB_BUILDER &&
            taskType != JOB_MEDIC &&
            taskType != JOB_REPAIR_ROBOT &&
            taskType != FIND_AND_RESCUE &&
            taskType != FIND_BED_AND_PUT_IN &&
            taskType != FIND_AND_RESCUE_IF_THERES_BEDS;
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

    bool StationOwnershipHeaderIsSane(
        const StationOwnershipHeaderSnapshot& header)
    {
        if (header.player == 0 || header.faction == 0 ||
            header.ownerships == 0 || header.sourceObject == 0 ||
            header.sourceVtable == 0)
        {
            return false;
        }
        if (header.count > header.maxSize ||
            header.maxSize > 65536)
        {
            return false;
        }
        if (header.count != 0 && header.stuff == 0)
        {
            return false;
        }
        return true;
    }

    bool SameStationOwnershipHeader(
        const StationOwnershipHeaderSnapshot& left,
        const StationOwnershipHeaderSnapshot& right)
    {
        return left.player == right.player &&
            left.faction == right.faction &&
            left.ownerships == right.ownerships &&
            left.sourceObject == right.sourceObject &&
            left.sourceVtable == right.sourceVtable &&
            left.count == right.count &&
            left.maxSize == right.maxSize &&
            left.stuff == right.stuff;
    }

    bool StationOwnershipSpanFits(
        const StationOwnershipHeaderSnapshot& header,
        unsigned int recordCount)
    {
        if (!StationOwnershipHeaderIsSane(header) ||
            recordCount > header.count)
        {
            return false;
        }
        if (recordCount == 0)
        {
            return true;
        }
        if (header.stuff == 0)
        {
            return false;
        }

        const ULONG_PTR maxAddress =
            static_cast<ULONG_PTR>(~static_cast<ULONG_PTR>(0));
        const ULONG_PTR bytes =
            static_cast<ULONG_PTR>(recordCount) *
            static_cast<ULONG_PTR>(sizeof(hand));
        const ULONG_PTR begin = header.stuff;
        return begin <= maxAddress - bytes;
    }

    // Read only scalar ownership-list metadata.  Pointer values are kept as
    // numeric bracket tokens in this local POD and never leave the guarded
    // copy operation.
    __declspec(noinline) bool TryReadStationOwnershipHeaderOnce(
        PlayerInterface* player,
        StationOwnershipHeaderSnapshot* output,
        DWORD* exceptionCode)
    {
        if (output != NULL)
        {
            memset(output, 0, sizeof(*output));
        }
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        __try
        {
            if (player == NULL || output == NULL)
            {
                return false;
            }

            Faction* faction = player->getFaction();
            if (faction == NULL || !faction->isThePlayer())
            {
                return false;
            }

            Ownerships* ownerships = faction->factionOwnerships;
            if (ownerships == NULL)
            {
                return false;
            }

            lektor<hand>* source = &ownerships->stuff;
            output->player = reinterpret_cast<ULONG_PTR>(player);
            output->faction = reinterpret_cast<ULONG_PTR>(faction);
            output->ownerships = reinterpret_cast<ULONG_PTR>(ownerships);
            output->sourceObject = reinterpret_cast<ULONG_PTR>(source);
            output->sourceVtable = reinterpret_cast<ULONG_PTR>(
                *reinterpret_cast<const void* const*>(source));
            output->count = source->count;
            output->maxSize = source->maxSize;
            output->stuff = reinterpret_cast<ULONG_PTR>(source->stuff);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (exceptionCode != NULL)
            {
                *exceptionCode = GetExceptionCode();
            }
            return false;
        }
    }

    // Bracket the borrowed array with exact owner/header reads.  The output
    // buffer belongs to the plug-in and receives only scalar hand fields; no
    // pointer into Kenshi's list survives this function.
    __declspec(noinline) bool TryCopyStationOwnershipRecords(
        PlayerInterface* player,
        StationOwnedHandRecord* output,
        unsigned int outputCapacity,
        unsigned int* copiedOut,
        bool* truncatedOut,
        DWORD* exceptionCode)
    {
        if (copiedOut != NULL)
        {
            *copiedOut = 0;
        }
        if (truncatedOut != NULL)
        {
            *truncatedOut = false;
        }
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        __try
        {
            if (player == NULL || output == NULL || copiedOut == NULL ||
                truncatedOut == NULL || outputCapacity == 0 ||
                outputCapacity > STATION_OWNERSHIP_COPY_LIMIT)
            {
                return false;
            }

            DWORD innerException = 0;
            StationOwnershipHeaderSnapshot expected;
            StationOwnershipHeaderSnapshot current;
            StationOwnershipHeaderSnapshot after;
            if (!TryReadStationOwnershipHeaderOnce(
                    player, &expected, &innerException) ||
                !StationOwnershipHeaderIsSane(expected) ||
                !TryReadStationOwnershipHeaderOnce(
                    player, &current, &innerException) ||
                !StationOwnershipHeaderIsSane(current) ||
                !SameStationOwnershipHeader(expected, current))
            {
                if (exceptionCode != NULL)
                {
                    *exceptionCode = innerException;
                }
                return false;
            }

            const unsigned int copyCount =
                current.count < outputCapacity ?
                    current.count : outputCapacity;
            if (!StationOwnershipSpanFits(current, copyCount))
            {
                return false;
            }

            const hand* source = reinterpret_cast<const hand*>(
                current.stuff);
            for (unsigned int index = 0; index < copyCount; ++index)
            {
                const hand* currentHand = source + index;
                output[index].vtable = reinterpret_cast<ULONG_PTR>(
                    *reinterpret_cast<const void* const*>(currentHand));
                output[index].type = static_cast<unsigned int>(
                    currentHand->type);
                output[index].container = currentHand->container;
                output[index].containerSerial = currentHand->containerSerial;
                output[index].index = currentHand->index;
                output[index].serial = currentHand->serial;
            }

            if (!TryReadStationOwnershipHeaderOnce(
                    player, &after, &innerException) ||
                !StationOwnershipHeaderIsSane(after) ||
                !SameStationOwnershipHeader(after, current) ||
                !SameStationOwnershipHeader(after, expected))
            {
                if (exceptionCode != NULL)
                {
                    *exceptionCode = innerException;
                }
                return false;
            }

            *copiedOut = copyCount;
            *truncatedOut = current.count > outputCapacity;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            if (exceptionCode != NULL)
            {
                *exceptionCode = GetExceptionCode();
            }
            return false;
        }
    }

    void CompactStationOwnedBuildingRecords(
        std::vector<StationOwnedHandRecord>* records)
    {
        if (records == NULL)
        {
            return;
        }

        size_t writeIndex = 0;
        for (size_t readIndex = 0; readIndex < records->size(); ++readIndex)
        {
            if ((*records)[readIndex].type !=
                static_cast<unsigned int>(BUILDING))
            {
                continue;
            }
            if (writeIndex != readIndex)
            {
                (*records)[writeIndex] = (*records)[readIndex];
            }
            ++writeIndex;
        }
        records->resize(writeIndex);
    }

    bool CaptureStationOwnedBuildingRecords(
        PlayerInterface* player,
        StationScanState* state)
    {
        if (player == NULL || state == NULL)
        {
            return false;
        }

        state->ownedBuildingRecords.clear();
        state->ownershipCopyTruncated = false;
        state->ownedBuildingRecords.resize(STATION_OWNERSHIP_COPY_LIMIT);
        unsigned int copied = 0;
        bool truncated = false;
        DWORD exceptionCode = 0;
        if (!TryCopyStationOwnershipRecords(
                player,
                state->ownedBuildingRecords.empty() ? NULL :
                    &state->ownedBuildingRecords[0],
                STATION_OWNERSHIP_COPY_LIMIT,
                &copied, &truncated, &exceptionCode))
        {
            state->ownedBuildingRecords.clear();
            AddStationScanError(
                state,
                "Player station ownership records could not be read safely.");
            return false;
        }

        state->ownedBuildingRecords.resize(copied);
        CompactStationOwnedBuildingRecords(&state->ownedBuildingRecords);
        state->ownershipCopyTruncated = truncated;
        if (truncated)
        {
            AddStationScanError(
                state,
                "Player station ownership records reached the 8,192-record safety limit.");
        }
        return true;
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

    bool StationTargetAlreadyPublishedOrPending(
        const HandleIdentity& identity,
        const StationScanState& state)
    {
        return StationTargetAlreadyScanned(identity, state.stations) ||
            StationTargetAlreadyScanned(identity, state.pendingStations);
    }

    void JoinStationAssignments(
        const std::vector<StationSquadSnapshot>& squads,
        StationTargetSnapshot* station);

    size_t StationScanResultCount(const StationScanState& state)
    {
        return state.stations.size() + state.pendingStations.size();
    }

    // Append the pending value snapshots to the public list. StationView owns
    // category and card-name sorting, so the scanner does not copy and merge
    // every previously published snapshot at each presentation boundary. No
    // engine pointer crosses this boundary; pendingStations contains only
    // normalized snapshots.
    void PublishPendingStationResults(StationScanState* state)
    {
        if (state == NULL || state->pendingStations.empty())
        {
            return;
        }

        for (size_t index = 0; index < state->pendingStations.size(); ++index)
        {
            JoinStationAssignments(
                state->squads, &state->pendingStations[index]);
        }
        state->stations.insert(
            state->stations.end(),
            state->pendingStations.begin(), state->pendingStations.end());
        state->pendingStations.clear();
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
        bool* relevantOut,
        bool* assignmentSupportedOut)
    {
        if (building == NULL || categoryOut == NULL || statOut == NULL ||
            statKnownOut == NULL || naturalOut == NULL || relevantOut == NULL ||
            assignmentSupportedOut == NULL)
        {
            return false;
        }

        *categoryOut = STATION_OTHER;
        *statOut = STAT_NONE;
        *statKnownOut = false;
        *naturalOut = false;
        *relevantOut = false;
        *assignmentSupportedOut = false;

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
            case BCTYPE_ITEM_FURNACE:
                *categoryOut = STATION_REFINING;
                *statOut = STAT_LABOURING;
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

            // A known stat refines the card's worker-skill label, but it does
            // not make an object a workstation.  The relevance decision above
            // is deliberately limited to the stable BuildingClassType and
            // BuildingFunction allowlist.  In particular, do not use the
            // generic default task: walls, lights, chairs, and other
            // interactive furniture can expose one without being a station.
            if (!*relevantOut)
            {
                return true;
            }
            TaskType normalizedTask = NULL_TASK;
            bool automaticBundle = false;
            *assignmentSupportedOut = NormalizeStationTaskScalars(
                classType, function, building->getDefaultTask(),
                &normalizedTask, &automaticBundle);
            UseableStuff* usable = building->getUseableStuff();
            if (usable != NULL)
            {
                const StatsEnumerated engineStat = usable->getStatUsed();
                if (engineStat > STAT_NONE && engineStat < STAT_END)
                {
                    *statOut = engineStat;
                    *statKnownOut = true;
                }
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
        bool allowNaturalException,
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
            *manageableOut = allowNaturalException ?
                (natural || building->isThePlayer()) :
                building->isThePlayer();
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
        bool powerOff = false;
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
                // isOutOfPower() alone is not an outage test. Kenshi resets a
                // power consumer's current allocation while it is idle, so an
                // unstaffed bench can report zero local power even when its
                // town has a large surplus. howMuchPowerDoYouWantNow() is the
                // remaining live demand after generator/battery allocation.
                // It is zero for an idle bench and for a fully supplied one.
                if (usable->isOutOfPower() > 0.001f)
                {
                    powerOff = !usable->isPowerOn();
                    noPower = !powerOff &&
                        usable->howMuchPowerDoYouWantNow() > 0.001f;
                }
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
        if (powerOff)
        {
            if (!statusOut->empty()) *statusOut += " / ";
            *statusOut += "POWER OFF";
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

    bool StationOwnedRecordMatchesHand(
        const StationOwnedHandRecord& record,
        const hand& value)
    {
        return record.vtable == reinterpret_cast<ULONG_PTR>(
                   *reinterpret_cast<const void* const*>(&value)) &&
            record.type == static_cast<unsigned int>(value.type) &&
            record.container == value.container &&
            record.containerSerial == value.containerSerial &&
            record.index == value.index &&
            record.serial == value.serial;
    }

    bool TrySnapshotStationBuilding(
        Building* building,
        const std::vector<StationSquadSnapshot>& squads,
        bool allowNaturalException,
        StationTargetSnapshot* stationOut,
        bool* includeOut);

    // Reconstruct and validate one borrowed ownership record.  The runtime
    // object, Building pointer, exact live-handle check, owner check, and
    // metadata snapshot all stay inside this guarded leaf.  Only normalized
    // value data is written to the caller's snapshot.
    __declspec(noinline) bool TryResolveOwnedStationRecord(
        const StationOwnedHandRecord* record,
        const std::vector<StationSquadSnapshot>& squads,
        StationOwnedBuildingResolution* resultOut,
        StationTargetSnapshot* stationOut,
        bool* includeOut)
    {
        if (resultOut != NULL)
        {
            *resultOut = StationOwnedBuildingResolution();
        }
        if (includeOut != NULL)
        {
            *includeOut = false;
        }

        __declspec(align(16)) unsigned char storage[sizeof(hand)];
        __try
        {
            if (record == NULL || resultOut == NULL || stationOut == NULL ||
                includeOut == NULL ||
                record->type != static_cast<unsigned int>(BUILDING))
            {
                if (resultOut != NULL)
                {
                    resultOut->result = STATION_OWNED_RECORD_FAULT;
                }
                return false;
            }

            hand* reconstructed = reinterpret_cast<hand*>(storage);
            hand* constructed = reconstructed->_CONSTRUCTOR(
                record->index,
                record->serial,
                static_cast<itemType>(record->type),
                record->container,
                record->containerSerial);
            if (constructed != reconstructed)
            {
                resultOut->result = STATION_OWNED_RECORD_FAULT;
                return true;
            }

            resultOut->constructorMatches =
                StationOwnedRecordMatchesHand(*record, *reconstructed);
            if (!resultOut->constructorMatches)
            {
                resultOut->result = STATION_OWNED_RECORD_FAULT;
                return true;
            }

            resultOut->handValid = reconstructed->isValid();
            if (!resultOut->handValid)
            {
                resultOut->result = STATION_OWNED_RECORD_UNLOADED;
                return true;
            }

            Building* building = reconstructed->getBuilding();
            resultOut->buildingFound = building != NULL;
            if (building == NULL)
            {
                resultOut->result = STATION_OWNED_RECORD_UNLOADED;
                return true;
            }

            const hand& actual = building->getHandle();
            resultOut->handleMatches = StationOwnedRecordMatchesHand(
                *record, actual);
            if (!resultOut->handleMatches)
            {
                resultOut->result = STATION_OWNED_RECORD_FAULT;
                return true;
            }

            // The ownership decision is made only after the exact live handle
            // check.  A stale slot can never promote a replacement building.
            resultOut->playerOwned = building->isThePlayer();
            if (!resultOut->playerOwned)
            {
                resultOut->result = STATION_OWNED_RECORD_FAULT;
                return true;
            }

            if (!TrySnapshotStationBuilding(
                    building, squads, false, stationOut, includeOut))
            {
                resultOut->result = STATION_OWNED_RECORD_FAULT;
                return false;
            }
            resultOut->result = STATION_OWNED_RECORD_OK;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            *resultOut = StationOwnedBuildingResolution();
            resultOut->result = STATION_OWNED_RECORD_FAULT;
            *includeOut = false;
            return false;
        }
    }

    bool TrySnapshotStationBuilding(
        Building* building,
        const std::vector<StationSquadSnapshot>& squads,
        bool allowNaturalException,
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
        // Direct-owned records must pass the strict station allowlist and do
        // not need a full squad/member/job search. Exact queue targets were
        // copied separately and are resolved first with the natural-resource
        // exception enabled.
        const bool queueEvidence = allowNaturalException &&
            TryFindAssignedStationTargetLabel(
                squads, station.identity, &queueTargetLabel);
        if (allowNaturalException && !queueEvidence)
        {
            return true;
        }

        bool playerManaged = false;
        if (!TryIsPlayerManagedStation(
                building, allowNaturalException, &playerManaged))
        {
            return false;
        }
        if (!playerManaged)
        {
            if (!allowNaturalException)
            {
                // The ownership leaf already proved this was player-owned.
                // A contradictory second read is a validation fault, not an
                // ordinary public/city omission.
                return false;
            }
            // Fail closed. A public or city building referenced by a queue is
            // not part of the player's station board.
            return true;
        }

        bool relevant = false;
        bool natural = false;
        if (!TryClassifyStationBuilding(
                building, &station.category, &station.relevantStat,
                &station.relevantSkillKnown, &natural, &relevant,
                &station.assignmentSupported))
        {
            if (!queueEvidence)
            {
                // A direct-owned record must identify a station-specific
                // work building.  Queue targets retain the old Other fallback
                // because the exact assignment is still useful to the user.
                return false;
            }
            // The exact queue assignment remains useful even when optional
            // building metadata cannot be read.  Keep the target visible in
            // the catch-all category instead of dropping the whole column.
            station.category = STATION_OTHER;
            station.relevantStat = STAT_NONE;
            station.relevantSkillKnown = false;
            relevant = false;
        }

        if (!relevant && !queueEvidence)
        {
            return true;
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
        station.blockingStatusKnown = TryGetStationBlockingStatus(
                building, &destroyed, &station.blocking,
                &station.blockingStatus);
        if (!station.blockingStatusKnown)
        {
            station.blocking = false;
            station.blockingStatus.clear();
        }
        if (destroyed && station.blockingStatus.empty())
        {
            station.blocking = true;
            station.blockingStatus = "Destroyed";
        }
        if (destroyed)
        {
            // Existing exact queue rows remain removable, but the UI must not
            // offer a new assignment that the guarded backend will reject.
            station.assignmentSupported = false;
        }

        if (!TryReadRootObjectName(building, &station.name) ||
            station.name.empty())
        {
            station.name = queueEvidence ?
                (queueTargetLabel.empty() ?
                    "Unnamed assigned target" : queueTargetLabel) :
                "Unnamed player station";
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
                building, state->squads, true, &station, &include))
        {
            return false;
        }
        if (!include ||
            StationTargetAlreadyPublishedOrPending(
                station.identity, *state))
        {
            return true;
        }
        // Only report truncation after a 2,049th qualifying unique target is
        // observed.  Remaining non-station objects do not make the result
        // incomplete.
        if (StationScanResultCount(*state) >= STATION_SCAN_TARGET_LIMIT)
        {
            state->truncated = true;
            state->complete = true;
            return true;
        }
        state->pendingStations.push_back(station);
        return true;
    }

    void CaptureStationOwnedRecordIdentity(
        const StationOwnedHandRecord& record,
        HandleIdentity* identityOut);

    bool TryScanOwnedStationTarget(
        const StationOwnedHandRecord& record,
        StationScanState* state)
    {
        if (state == NULL)
        {
            return false;
        }

        // Assigned handles are resolved before borrowed ownership records.
        // Skip a matching record before reconstructing another live handle or
        // reading metadata a second time. Failed assigned targets are not in
        // either result list and still receive the normal direct-owned pass.
        HandleIdentity recordIdentity;
        CaptureStationOwnedRecordIdentity(record, &recordIdentity);
        if (recordIdentity.valid &&
            StationTargetAlreadyPublishedOrPending(recordIdentity, *state))
        {
            return true;
        }

        StationOwnedBuildingResolution resolution;
        StationTargetSnapshot station;
        bool include = false;
        const bool resolved = TryResolveOwnedStationRecord(
            &record, state->squads, &resolution, &station, &include);
        if (!resolved || resolution.result == STATION_OWNED_RECORD_FAULT)
        {
            if (!state->ownershipResolutionIncomplete)
            {
                state->ownershipResolutionIncomplete = true;
                AddStationScanError(
                    state,
                    "Some player station ownership records failed validation.");
            }
            return true;
        }
        if (resolution.result != STATION_OWNED_RECORD_OK ||
            !resolution.playerOwned)
        {
            // An ownership record can point at an unloaded or replaced
            // object.  It is an expected omission from the live board, not a
            // failed assigned target and therefore does not add a red error.
            return true;
        }

        if (!include ||
            StationTargetAlreadyPublishedOrPending(
                station.identity, *state))
        {
            return true;
        }
        if (StationScanResultCount(*state) >= STATION_SCAN_TARGET_LIMIT)
        {
            state->truncated = true;
            state->complete = true;
            return true;
        }
        state->pendingStations.push_back(station);
        return true;
    }

    size_t StationScanCandidateCount(const StationScanState& state)
    {
        return state.ownedBuildingRecords.size() +
            state.assignedTargetHandles.size();
    }

    void CaptureStationOwnedRecordIdentity(
        const StationOwnedHandRecord& record,
        HandleIdentity* identityOut)
    {
        if (identityOut == NULL)
        {
            return;
        }
        identityOut->valid = record.vtable != 0 &&
            record.type == static_cast<unsigned int>(BUILDING);
        identityOut->type = static_cast<itemType>(record.type);
        identityOut->container = record.container;
        identityOut->containerSerial = record.containerSerial;
        identityOut->index = record.index;
        identityOut->serial = record.serial;
    }

    bool TryGetStationScanCandidateIdentity(
        const StationScanState& state,
        size_t candidateIndex,
        HandleIdentity* identityOut)
    {
        if (identityOut == NULL ||
            candidateIndex >= StationScanCandidateCount(state))
        {
            return false;
        }

        if (candidateIndex < state.assignedTargetHandles.size())
        {
            CaptureHandleIdentity(
                state.assignedTargetHandles[candidateIndex], identityOut);
            return true;
        }

        CaptureStationOwnedRecordIdentity(
            state.ownedBuildingRecords[
                candidateIndex - state.assignedTargetHandles.size()],
            identityOut);
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
        // Copy borrowed ownership records into plug-in-owned scalar storage.
        // The list is bracket-validated; live handles are reconstructed later,
        // one candidate per guarded scanner call.
        CaptureStationOwnedBuildingRecords(player, state);

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

        const size_t resultCapacity = std::min(
            StationScanCandidateCount(*state), STATION_SCAN_TARGET_LIMIT);
        state->stations.reserve(resultCapacity);
        state->pendingStations.reserve(std::min(
            resultCapacity, STATION_SCAN_PRESENTATION_BATCH_SIZE));

        state->complete = StationScanCandidateCount(*state) == 0;
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
        if (state->nextTarget >= StationScanCandidateCount(*state))
        {
            PublishPendingStationResults(state);
            state->complete = true;
            return true;
        }

        const size_t candidateCount = StationScanCandidateCount(*state);
        const size_t candidateIndex = state->nextTarget;
        const bool ownedCandidate =
            candidateIndex >= state->assignedTargetHandles.size();
        ++state->nextTarget;

        bool scanSucceeded = true;
        if (ownedCandidate)
        {
            // Ownership records are best-effort live inventory.  An unloaded
            // slot is silently omitted and does not become an assigned-target
            // failure in the banner.
            scanSucceeded = TryScanOwnedStationTarget(
                state->ownedBuildingRecords[
                    candidateIndex - state->assignedTargetHandles.size()],
                state);
        }
        else
        {
            const hand target = state->assignedTargetHandles[candidateIndex];
            scanSucceeded = TryScanStationTarget(target, state);
        }
        if (!ownedCandidate && !scanSucceeded)
        {
            ++state->targetsFailed;
            AddStationScanError(
                state, "An assigned station target could not be read safely.");
        }
        ++state->targetsCompleted;
        const bool presentationBatchComplete =
            (state->targetsCompleted %
             STATION_SCAN_PRESENTATION_BATCH_SIZE) == 0;
        if (presentationBatchComplete ||
            state->nextTarget >= candidateCount ||
            state->truncated)
        {
            PublishPendingStationResults(state);
        }
        if (state->nextTarget >= candidateCount ||
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
        state->pendingStations.clear();
        state->ownedBuildingRecords.clear();
        state->assignedTargetHandles.clear();
        state->errors.clear();
        state->nextTarget = 0;
        state->targetsCompleted = 0;
        state->targetsFailed = 0;
        state->truncated = false;
        state->ownershipResolutionIncomplete = false;
        if (incomplete)
        {
            AddStationScanError(
                state, "Some loaded squad job data is unavailable.");
        }

        CaptureStationOwnedBuildingRecords(player, state);

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
        const size_t resultCapacity = std::min(
            StationScanCandidateCount(*state), STATION_SCAN_TARGET_LIMIT);
        state->stations.reserve(resultCapacity);
        state->pendingStations.reserve(std::min(
            resultCapacity, STATION_SCAN_PRESENTATION_BATCH_SIZE));
        state->complete = StationScanCandidateCount(*state) == 0;
        return true;
    }

#endif
