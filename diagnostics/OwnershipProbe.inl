// SPDX-License-Identifier: GPL-3.0-only
// Diagnostic-only borrowed Ownerships::stuff probe.
//
// This file is included only when KJM_SCANNER_PROBE is defined. It never calls
// an ownership discovery function or allocates/frees an engine container.
// Stages 4-5 reconstruct one local hand, with only isValid reached in stage 5.
// Engine pointers exist only in guarded leaf frames; the probe state retains
// numeric addresses for identity comparisons only.

    typedef char OwnershipProbeAssertLektorSize[
        (sizeof(lektor<hand>) == 0x18) ? 1 : -1];
    typedef char OwnershipProbeAssertLektorCount[
        (offsetof(lektor<hand>, count) == 0x08) ? 1 : -1];
    typedef char OwnershipProbeAssertLektorMax[
        (offsetof(lektor<hand>, maxSize) == 0x0C) ? 1 : -1];
    typedef char OwnershipProbeAssertLektorStuff[
        (offsetof(lektor<hand>, stuff) == 0x10) ? 1 : -1];
    typedef char OwnershipProbeAssertHandSize[
        (sizeof(hand) == 0x20) ? 1 : -1];
    typedef char OwnershipProbeAssertItemTypeSize[
        (sizeof(itemType) == 0x04) ? 1 : -1];
    typedef char OwnershipProbeAssertHandType[
        (offsetof(hand, type) == 0x08) ? 1 : -1];
    typedef char OwnershipProbeAssertHandContainer[
        (offsetof(hand, container) == 0x0C) ? 1 : -1];
    typedef char OwnershipProbeAssertHandContainerSerial[
        (offsetof(hand, containerSerial) == 0x10) ? 1 : -1];
    typedef char OwnershipProbeAssertHandIndex[
        (offsetof(hand, index) == 0x14) ? 1 : -1];
    typedef char OwnershipProbeAssertHandSerial[
        (offsetof(hand, serial) == 0x18) ? 1 : -1];

    enum OwnershipProbeState
    {
        OWNERSHIP_PROBE_IDLE = 0,
        OWNERSHIP_PROBE_OWNER_HEADER = 1,
        OWNERSHIP_PROBE_FIRST_HANDS = 2,
        OWNERSHIP_PROBE_FULL_HANDS = 3,
        OWNERSHIP_PROBE_HAND_CONSTRUCTED = 4,
        OWNERSHIP_PROBE_HAND_VALIDATED = 5,
        OWNERSHIP_PROBE_FAILED = -1,
        OWNERSHIP_PROBE_ABANDONED = -2
    };

    const unsigned int OWNERSHIP_PROBE_MAX_HEADER_CAPACITY = 65536;
    const unsigned int OWNERSHIP_PROBE_COPY8_CAPACITY = 8;
    const unsigned int OWNERSHIP_PROBE_COPYALL_CAPACITY = 8192;

    // These records contain only scalar values copied from the engine. No
    // engine pointer is retained in the diagnostic state.
    struct OwnershipProbeHeaderSnapshot
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

    struct OwnershipProbeRawHand
    {
        ULONG_PTR vtable;
        unsigned int type;
        unsigned int container;
        unsigned int containerSerial;
        unsigned int index;
        unsigned int serial;
    };

    volatile LONG g_ownershipProbeRequestedStage = 0;
    volatile LONG g_ownershipProbeState = OWNERSHIP_PROBE_IDLE;
    volatile LONG g_ownershipProbeBusy = 0;

    OwnershipProbeHeaderSnapshot g_ownershipProbeHeader = {};

    // Fixed plugin-owned POD storage. It is never passed to an engine API.
    OwnershipProbeRawHand g_ownershipProbeFirstEight[
        OWNERSHIP_PROBE_COPY8_CAPACITY] = {};
    unsigned int g_ownershipProbeFirstEightCount = 0;
    OwnershipProbeRawHand g_ownershipProbeAll[
        OWNERSHIP_PROBE_COPYALL_CAPACITY] = {};
    unsigned int g_ownershipProbeAllCount = 0;
    unsigned int g_ownershipProbeBuildingCount = 0;
    bool g_ownershipProbeAllTruncated = false;
    bool g_ownershipProbeF10WasDown = false;
    bool g_ownershipProbeF10Armed = false;
    DWORD g_ownershipProbeUpdateThreadId = 0;

    void OwnershipProbeWriteLog(const char* message)
    {
        if (message == NULL)
        {
            return;
        }

        char line[1024];
        _snprintf_s(
            line, sizeof(line), _TRUNCATE,
            "[KJM OwnershipProbe] tid=%lu %s",
            static_cast<unsigned long>(GetCurrentThreadId()), message);
        DebugLog(line);
        OutputDebugStringA(line);
        OutputDebugStringA("\n");
    }

    void OwnershipProbeLogException(const char* stage, DWORD exceptionCode)
    {
        char message[256];
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=%s exception=0x%08lX",
            stage != NULL ? stage : "unknown",
            static_cast<unsigned long>(exceptionCode));
        OwnershipProbeWriteLog(message);
    }

    LONG OwnershipProbeReadState()
    {
        return InterlockedCompareExchange(
            &g_ownershipProbeState,
            OWNERSHIP_PROBE_IDLE,
            OWNERSHIP_PROBE_IDLE);
    }

    bool OwnershipProbeOnExpectedThread()
    {
        return g_ownershipProbeUpdateThreadId != 0 &&
            GetCurrentThreadId() == g_ownershipProbeUpdateThreadId;
    }

    bool OwnershipProbeHeaderIsSane(
        const OwnershipProbeHeaderSnapshot& header)
    {
        if (header.player == 0 || header.faction == 0 ||
            header.ownerships == 0 || header.sourceObject == 0 ||
            header.sourceVtable == 0)
        {
            return false;
        }
        if (header.count > header.maxSize ||
            header.maxSize > OWNERSHIP_PROBE_MAX_HEADER_CAPACITY)
        {
            return false;
        }
        if (header.count != 0 && header.stuff == 0)
        {
            return false;
        }
        return true;
    }

    bool OwnershipProbeHeaderEquals(
        const OwnershipProbeHeaderSnapshot& left,
        const OwnershipProbeHeaderSnapshot& right)
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

    bool OwnershipProbeSpanFits(
        const OwnershipProbeHeaderSnapshot& header,
        unsigned int recordCount)
    {
        if (!OwnershipProbeHeaderIsSane(header) ||
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

    // This guarded leaf resolves the player owner and reads only scalar
    // fields from faction->factionOwnerships->stuff. Its pointer locals die
    // before the wrapper returns.
    __declspec(noinline) bool OwnershipProbeTryReadHeaderOnce(
        PlayerInterface* player,
        OwnershipProbeHeaderSnapshot* output,
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

    __declspec(noinline) bool OwnershipProbeTryReadHeaderTwice(
        PlayerInterface* player,
        OwnershipProbeHeaderSnapshot* first,
        OwnershipProbeHeaderSnapshot* second,
        DWORD* exceptionCode)
    {
        DWORD innerException = 0;
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        if (!OwnershipProbeTryReadHeaderOnce(
                player, first, &innerException))
        {
            if (exceptionCode != NULL)
            {
                *exceptionCode = innerException;
            }
            return false;
        }
        if (!OwnershipProbeTryReadHeaderOnce(
                player, second, &innerException))
        {
            if (exceptionCode != NULL)
            {
                *exceptionCode = innerException;
            }
            return false;
        }
        return true;
    }

    // Copy a bounded scalar prefix while the borrowed source header is stable.
    // The source pointer is local to this guarded frame and is never retained.
    __declspec(noinline) bool OwnershipProbeTryCopyRawHands(
        PlayerInterface* player,
        const OwnershipProbeHeaderSnapshot* expected,
        OwnershipProbeRawHand* output,
        unsigned int outputCapacity,
        unsigned int* copied,
        unsigned int* buildingCount,
        bool* truncated,
        OwnershipProbeHeaderSnapshot* before,
        OwnershipProbeHeaderSnapshot* after,
        DWORD* exceptionCode)
    {
        if (copied != NULL)
        {
            *copied = 0;
        }
        if (buildingCount != NULL)
        {
            *buildingCount = 0;
        }
        if (truncated != NULL)
        {
            *truncated = false;
        }
        if (before != NULL)
        {
            memset(before, 0, sizeof(*before));
        }
        if (after != NULL)
        {
            memset(after, 0, sizeof(*after));
        }
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        __try
        {
            if (player == NULL || expected == NULL || output == NULL ||
                copied == NULL || buildingCount == NULL ||
                truncated == NULL || before == NULL || after == NULL ||
                outputCapacity == 0 ||
                outputCapacity > OWNERSHIP_PROBE_COPYALL_CAPACITY)
            {
                return false;
            }

            DWORD innerException = 0;
            OwnershipProbeHeaderSnapshot current;
            if (!OwnershipProbeTryReadHeaderOnce(
                    player, &current, &innerException))
            {
                if (exceptionCode != NULL)
                {
                    *exceptionCode = innerException;
                }
                return false;
            }
            if (!OwnershipProbeHeaderIsSane(current) ||
                !OwnershipProbeHeaderEquals(current, *expected))
            {
                return false;
            }

            const unsigned int copyCount =
                current.count < outputCapacity
                    ? current.count : outputCapacity;
            const bool copyWasTruncated = current.count > outputCapacity;
            if (!OwnershipProbeSpanFits(current, copyCount))
            {
                return false;
            }

            unsigned int copiedBuildingCount = 0;
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
                if (output[index].type ==
                    static_cast<unsigned int>(BUILDING))
                {
                    ++copiedBuildingCount;
                }
            }

            if (!OwnershipProbeTryReadHeaderOnce(
                    player, after, &innerException))
            {
                if (exceptionCode != NULL)
                {
                    *exceptionCode = innerException;
                }
                return false;
            }
            if (!OwnershipProbeHeaderIsSane(*after) ||
                !OwnershipProbeHeaderEquals(*after, current) ||
                !OwnershipProbeHeaderEquals(*after, *expected))
            {
                return false;
            }

            *before = current;
            *copied = copyCount;
            *buildingCount = copiedBuildingCount;
            *truncated = copyWasTruncated;
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

    void OwnershipProbeLogRawHand(
        const char* prefix,
        unsigned int index,
        const OwnershipProbeRawHand& value)
    {
        char message[512];
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "%s[%u] vtable=%p type=%u container=%u containerSerial=%u "
            "index=%u serial=%u",
            prefix, index, reinterpret_cast<const void*>(value.vtable),
            value.type, value.container, value.containerSerial,
            value.index, value.serial);
        OwnershipProbeWriteLog(message);
    }

    bool OwnershipProbeRawHandEquals(
        const OwnershipProbeRawHand& left,
        const OwnershipProbeRawHand& right)
    {
        return left.vtable == right.vtable &&
            left.type == right.type &&
            left.container == right.container &&
            left.containerSerial == right.containerSerial &&
            left.index == right.index &&
            left.serial == right.serial;
    }

    bool OwnershipProbeCaptureCurrentRecordZero(
        PlayerInterface* player,
        OwnershipProbeRawHand* currentOut,
        DWORD* exceptionCode)
    {
        if (currentOut != NULL)
        {
            memset(currentOut, 0, sizeof(*currentOut));
        }
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }
        if (player == NULL || currentOut == NULL ||
            g_ownershipProbeAllTruncated ||
            g_ownershipProbeAllCount == 0 ||
            g_ownershipProbeAll[0].type !=
                static_cast<unsigned int>(BUILDING))
        {
            return false;
        }

        OwnershipProbeHeaderSnapshot before;
        OwnershipProbeHeaderSnapshot after;
        unsigned int copied = 0;
        unsigned int buildingCount = 0;
        bool prefixTruncated = false;
        if (!OwnershipProbeTryCopyRawHands(
                player,
                &g_ownershipProbeHeader,
                currentOut,
                1,
                &copied,
                &buildingCount,
                &prefixTruncated,
                &before,
                &after,
                exceptionCode))
        {
            return false;
        }
        (void)prefixTruncated;
        return copied == 1 && buildingCount == 1 &&
            OwnershipProbeRawHandEquals(
                *currentOut, g_ownershipProbeAll[0]);
    }

    // Construct a local hand through the engine's five-field constructor.
    // The raw storage and scalar locals require no C++ unwinding in this SEH
    // leaf. No hand pointer leaves the function and no destructor is called.
    __declspec(noinline) bool OwnershipProbeTryConstructRecord(
        const OwnershipProbeRawHand* expected,
        bool callIsValid,
        bool* constructorMatches,
        bool* validOut,
        LONG* phaseOut,
        DWORD* exceptionCode)
    {
        if (constructorMatches != NULL)
        {
            *constructorMatches = false;
        }
        if (validOut != NULL)
        {
            *validOut = false;
        }
        if (phaseOut != NULL)
        {
            *phaseOut = 0;
        }
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        __declspec(align(16)) unsigned char storage[sizeof(hand)];
        __try
        {
            if (expected == NULL || constructorMatches == NULL ||
                validOut == NULL || phaseOut == NULL ||
                expected->type != static_cast<unsigned int>(BUILDING))
            {
                return false;
            }

            *phaseOut = 1;
            hand* reconstructed = reinterpret_cast<hand*>(storage);
            hand* constructed = reconstructed->_CONSTRUCTOR(
                expected->index,
                expected->serial,
                static_cast<itemType>(expected->type),
                expected->container,
                expected->containerSerial);

            *phaseOut = 2;
            if (constructed != reconstructed)
            {
                return true;
            }
            *constructorMatches =
                reinterpret_cast<ULONG_PTR>(
                    *reinterpret_cast<const void* const*>(reconstructed)) ==
                    expected->vtable &&
                static_cast<unsigned int>(reconstructed->type) ==
                    expected->type &&
                reconstructed->container == expected->container &&
                reconstructed->containerSerial ==
                    expected->containerSerial &&
                reconstructed->index == expected->index &&
                reconstructed->serial == expected->serial;
            if (!*constructorMatches || !callIsValid)
            {
                *phaseOut = 3;
                return true;
            }

            *phaseOut = 4;
            *validOut = reconstructed->isValid();
            *phaseOut = 5;
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

    void OwnershipProbeRunStage1(PlayerInterface* player)
    {
        if (OwnershipProbeReadState() != OWNERSHIP_PROBE_IDLE)
        {
            OwnershipProbeWriteLog(
                "stage=1 rejected; probe is already active or complete");
            return;
        }

        OwnershipProbeWriteLog(
            "stage=1 before resolving player faction and Ownerships::stuff");
        OwnershipProbeHeaderSnapshot first;
        OwnershipProbeHeaderSnapshot second;
        DWORD exceptionCode = 0;
        if (!OwnershipProbeTryReadHeaderTwice(
                player, &first, &second, &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("1-header", exceptionCode);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "stage=1 player faction/Ownerships resolution failed");
            }
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }

        const bool firstSane = OwnershipProbeHeaderIsSane(first);
        const bool secondSane = OwnershipProbeHeaderIsSane(second);
        const bool exactMatch = OwnershipProbeHeaderEquals(first, second);
        if (!firstSane || !secondSane || !exactMatch)
        {
            OwnershipProbeWriteLog(
                firstSane && secondSane
                    ? "stage=1 owner/header changed between reads"
                    : "stage=1 source header failed count/max/stuff validation");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            return;
        }

        g_ownershipProbeHeader = first;
        g_ownershipProbeUpdateThreadId = GetCurrentThreadId();
        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_OWNER_HEADER);

        char message[768];
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=1 complete owner=%p faction=%p source=%p "
            "source-vtable=%p count=%u max=%u stuff=%p owner-header-exact=yes",
            reinterpret_cast<const void*>(first.ownerships),
            reinterpret_cast<const void*>(first.faction),
            reinterpret_cast<const void*>(first.sourceObject),
            reinterpret_cast<const void*>(first.sourceVtable),
            first.count, first.maxSize,
            reinterpret_cast<const void*>(first.stuff));
        OwnershipProbeWriteLog(message);
    }

    void OwnershipProbeRunStage2(PlayerInterface* player)
    {
        if (OwnershipProbeReadState() != OWNERSHIP_PROBE_OWNER_HEADER)
        {
            OwnershipProbeWriteLog(
                "stage=2 rejected; stage 1 must complete first");
            return;
        }
        if (!OwnershipProbeOnExpectedThread())
        {
            OwnershipProbeWriteLog(
                "stage=2 update thread changed; abandoning probe");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            return;
        }

        OwnershipProbeHeaderSnapshot before;
        OwnershipProbeHeaderSnapshot after;
        unsigned int copied = 0;
        unsigned int buildingCount = 0;
        bool truncated = false;
        DWORD exceptionCode = 0;
        OwnershipProbeWriteLog(
            "stage=2 before re-resolve and bracket-copy of up to eight hand records");
        if (!OwnershipProbeTryCopyRawHands(
                player,
                &g_ownershipProbeHeader,
                g_ownershipProbeFirstEight,
                OWNERSHIP_PROBE_COPY8_CAPACITY,
                &copied,
                &buildingCount,
                &truncated,
                &before,
                &after,
                &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("2-copy8", exceptionCode);
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "stage=2 owner/header pre/post mismatch or raw copy failed");
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            }
            return;
        }

        g_ownershipProbeFirstEightCount = copied;
        for (unsigned int index = 0; index < copied; ++index)
        {
            OwnershipProbeLogRawHand(
                "stage=2 hand", index, g_ownershipProbeFirstEight[index]);
        }
        char message[512];
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=2 complete copied=%u source-count=%u "
            "owner-header-pre-post-exact=yes",
            copied, before.count);
        OwnershipProbeWriteLog(message);
        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_FIRST_HANDS);
    }

    void OwnershipProbeRunStage3(PlayerInterface* player)
    {
        if (OwnershipProbeReadState() != OWNERSHIP_PROBE_FIRST_HANDS)
        {
            OwnershipProbeWriteLog(
                "stage=3 rejected; stage 2 must complete first");
            return;
        }
        if (!OwnershipProbeOnExpectedThread())
        {
            OwnershipProbeWriteLog(
                "stage=3 update thread changed; abandoning probe");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            return;
        }

        OwnershipProbeHeaderSnapshot before;
        OwnershipProbeHeaderSnapshot after;
        unsigned int copied = 0;
        unsigned int buildingCount = 0;
        bool truncated = false;
        DWORD exceptionCode = 0;
        OwnershipProbeWriteLog(
            "stage=3 before re-resolve and bracket-copy of up to 8192 hand records");
        if (!OwnershipProbeTryCopyRawHands(
                player,
                &g_ownershipProbeHeader,
                g_ownershipProbeAll,
                OWNERSHIP_PROBE_COPYALL_CAPACITY,
                &copied,
                &buildingCount,
                &truncated,
                &before,
                &after,
                &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("3-copyall", exceptionCode);
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "stage=3 owner/header pre/post mismatch or raw copy failed");
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            }
            return;
        }

        g_ownershipProbeAllCount = copied;
        g_ownershipProbeBuildingCount = buildingCount;
        g_ownershipProbeAllTruncated = truncated;
        char message[512];
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=3 complete copied=%u BUILDING=%u truncated=%s "
            "source-count=%u owner-header-pre-post-exact=yes",
            copied, buildingCount, truncated ? "yes" : "no", before.count);
        OwnershipProbeWriteLog(message);
        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_FULL_HANDS);
    }

    void OwnershipProbeRunStage4(PlayerInterface* player)
    {
        if (OwnershipProbeReadState() != OWNERSHIP_PROBE_FULL_HANDS)
        {
            OwnershipProbeWriteLog(
                "stage=4 rejected; stage 3 must complete first");
            return;
        }
        if (!OwnershipProbeOnExpectedThread())
        {
            OwnershipProbeWriteLog(
                "stage=4 update thread changed; abandoning probe");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            return;
        }

        OwnershipProbeRawHand currentBefore;
        DWORD exceptionCode = 0;
        if (!OwnershipProbeCaptureCurrentRecordZero(
                player, &currentBefore, &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("4-prevalidate", exceptionCode);
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "stage=4 owner/header/record changed; abandoning probe");
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            }
            return;
        }

        OwnershipProbeWriteLog(
            "stage=4 before five-field hand constructor; no hand method will run");
        bool constructorMatches = false;
        bool unusedValid = false;
        LONG phase = 0;
        if (!OwnershipProbeTryConstructRecord(
                &currentBefore,
                false,
                &constructorMatches,
                &unusedValid,
                &phase,
                &exceptionCode))
        {
            char stage[64];
            _snprintf_s(
                stage, sizeof(stage), _TRUNCATE,
                "4-constructor-phase-%ld", phase);
            OwnershipProbeLogException(stage, exceptionCode);
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }
        if (!constructorMatches)
        {
            OwnershipProbeWriteLog(
                "stage=4 constructor returned mismatched fields or vtable");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }

        OwnershipProbeRawHand currentAfter;
        if (!OwnershipProbeCaptureCurrentRecordZero(
                player, &currentAfter, &exceptionCode) ||
            !OwnershipProbeRawHandEquals(currentBefore, currentAfter))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("4-postvalidate", exceptionCode);
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "stage=4 source changed after constructor; abandoning probe");
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            }
            return;
        }

        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_HAND_CONSTRUCTED);
        OwnershipProbeWriteLog(
            "stage=4 complete; constructor fields and vtable match exactly");
    }

    void OwnershipProbeRunStage5(PlayerInterface* player)
    {
        if (OwnershipProbeReadState() != OWNERSHIP_PROBE_HAND_CONSTRUCTED)
        {
            OwnershipProbeWriteLog(
                "stage=5 rejected; stage 4 must complete first");
            return;
        }
        if (!OwnershipProbeOnExpectedThread())
        {
            OwnershipProbeWriteLog(
                "stage=5 update thread changed; abandoning probe");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            return;
        }

        OwnershipProbeRawHand currentBefore;
        DWORD exceptionCode = 0;
        if (!OwnershipProbeCaptureCurrentRecordZero(
                player, &currentBefore, &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("5-prevalidate", exceptionCode);
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "stage=5 owner/header/record changed; abandoning probe");
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            }
            return;
        }

        OwnershipProbeWriteLog(
            "stage=5 before fresh constructor and hand.isValid; no Building call will run");
        bool constructorMatches = false;
        bool handValid = false;
        LONG phase = 0;
        if (!OwnershipProbeTryConstructRecord(
                &currentBefore,
                true,
                &constructorMatches,
                &handValid,
                &phase,
                &exceptionCode))
        {
            char stage[64];
            _snprintf_s(
                stage, sizeof(stage), _TRUNCATE,
                "5-isValid-phase-%ld", phase);
            OwnershipProbeLogException(stage, exceptionCode);
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }
        if (!constructorMatches)
        {
            OwnershipProbeWriteLog(
                "stage=5 fresh constructor no longer matches copied record");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }

        OwnershipProbeRawHand currentAfter;
        if (!OwnershipProbeCaptureCurrentRecordZero(
                player, &currentAfter, &exceptionCode) ||
            !OwnershipProbeRawHandEquals(currentBefore, currentAfter))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("5-postvalidate", exceptionCode);
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "stage=5 source changed after hand.isValid; abandoning probe");
                InterlockedExchange(
                    &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            }
            return;
        }

        char message[256];
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=5 complete hand.isValid=%s; no Building pointer was requested",
            handValid ? "true" : "false");
        OwnershipProbeWriteLog(message);
        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_HAND_VALIDATED);
    }

    void OwnershipProbeRequestStage(int stage)
    {
        if (stage >= 1 && stage <= 5)
        {
            InterlockedExchange(&g_ownershipProbeRequestedStage, stage);
        }
    }

    void OwnershipProbeClearPODState()
    {
        InterlockedExchange(&g_ownershipProbeRequestedStage, 0);
        InterlockedExchange(&g_ownershipProbeBusy, 0);
        memset(&g_ownershipProbeHeader, 0,
            sizeof(g_ownershipProbeHeader));
        memset(g_ownershipProbeFirstEight, 0,
            sizeof(g_ownershipProbeFirstEight));
        g_ownershipProbeFirstEightCount = 0;
        memset(g_ownershipProbeAll, 0, sizeof(g_ownershipProbeAll));
        g_ownershipProbeAllCount = 0;
        g_ownershipProbeBuildingCount = 0;
        g_ownershipProbeAllTruncated = false;
        g_ownershipProbeF10WasDown = false;
        g_ownershipProbeF10Armed = false;
        g_ownershipProbeUpdateThreadId = 0;
        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_IDLE);
    }

    LONG OwnershipProbeGetState()
    {
        return OwnershipProbeReadState();
    }

    void OwnershipProbeOnWorldReset()
    {
        // Only plugin-owned POD storage is cleared. No game pointer is read,
        // dereferenced, or released while the world is being torn down.
        OwnershipProbeClearPODState();
        OwnershipProbeWriteLog(
            "world reset cleared borrowed-header snapshots and plugin-owned POD records");
    }

    void TickOwnershipProbe(PlayerInterface* player)
    {
        const bool modifiersDown =
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool f10Down =
            (GetAsyncKeyState(VK_F10) & 0x8000) != 0;

        if (!f10Down)
        {
            g_ownershipProbeF10Armed = true;
        }

        if (g_ownershipProbeF10Armed && modifiersDown && f10Down &&
            !g_ownershipProbeF10WasDown)
        {
            const LONG state = OwnershipProbeReadState();
            if (state == OWNERSHIP_PROBE_IDLE)
            {
                OwnershipProbeRequestStage(1);
            }
            else if (state == OWNERSHIP_PROBE_OWNER_HEADER)
            {
                OwnershipProbeRequestStage(2);
            }
            else if (state == OWNERSHIP_PROBE_FIRST_HANDS)
            {
                OwnershipProbeRequestStage(3);
            }
            else if (state == OWNERSHIP_PROBE_FULL_HANDS)
            {
                OwnershipProbeRequestStage(4);
            }
            else if (state == OWNERSHIP_PROBE_HAND_CONSTRUCTED)
            {
                OwnershipProbeRequestStage(5);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "Ctrl+Shift+F10 ignored; probe is complete, failed, or abandoned");
            }
        }
        g_ownershipProbeF10WasDown = f10Down;

        const LONG requested = InterlockedExchange(
            &g_ownershipProbeRequestedStage, 0);
        if (requested == 0)
        {
            return;
        }
        if (InterlockedCompareExchange(
                &g_ownershipProbeBusy, 1, 0) != 0)
        {
            OwnershipProbeWriteLog(
                "probe request rejected because another stage is active");
            return;
        }
        if (requested == 1)
        {
            OwnershipProbeRunStage1(player);
        }
        else if (requested == 2)
        {
            OwnershipProbeRunStage2(player);
        }
        else if (requested == 3)
        {
            OwnershipProbeRunStage3(player);
        }
        else if (requested == 4)
        {
            OwnershipProbeRunStage4(player);
        }
        else if (requested == 5)
        {
            OwnershipProbeRunStage5(player);
        }
        InterlockedExchange(&g_ownershipProbeBusy, 0);
    }
