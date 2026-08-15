// SPDX-License-Identifier: GPL-3.0-only
// Diagnostic-only, one-shot probe for Ownerships::getOwnedBuildingsH.
//
// This file is included only when KJM_SCANNER_PROBE is defined.  The normal
// Release build does not contain this code or the discovery API symbol.

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

    enum OwnershipProbeState
    {
        OWNERSHIP_PROBE_IDLE = 0,
        OWNERSHIP_PROBE_CALL_RETURNED = 1,
        OWNERSHIP_PROBE_HEADER_READ = 2,
        OWNERSHIP_PROBE_HANDLES_READ = 3,
        OWNERSHIP_PROBE_FAILED = -1,
        OWNERSHIP_PROBE_ABANDONED = -2
    };

    struct OwnershipProbeLektorHeader
    {
        const void* object;
        const void* vtable;
        unsigned int count;
        unsigned int maxSize;
        const hand* stuff;

        OwnershipProbeLektorHeader() :
            object(NULL), vtable(NULL), count(0), maxSize(0), stuff(NULL)
        {
        }
    };

    struct OwnershipProbeRawHand
    {
        const void* vtable;
        unsigned int type;
        unsigned int container;
        unsigned int containerSerial;
        unsigned int index;
        unsigned int serial;

        OwnershipProbeRawHand() :
            vtable(NULL), type(0), container(0), containerSerial(0),
            index(0), serial(0)
        {
        }
    };

    volatile LONG g_ownershipProbeRequestedStage = 0;
    volatile LONG g_ownershipProbeState = OWNERSHIP_PROBE_IDLE;
    PlayerInterface* g_ownershipProbePlayer = NULL;
    Faction* g_ownershipProbeFaction = NULL;
    Ownerships* g_ownershipProbeOwnerships = NULL;
    lektor<hand> g_ownershipProbeOutput;
    OwnershipProbeLektorHeader g_ownershipProbeOutputHeader;
    OwnershipProbeLektorHeader g_ownershipProbeSourceHeader;
    bool g_ownershipProbeOutputAliasesSource = false;
    bool g_ownershipProbeF10WasDown = false;

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

    __declspec(noinline) bool OwnershipProbeTryResolveOwner(
        PlayerInterface* player,
        Faction** factionOut,
        Ownerships** ownershipsOut,
        DWORD* exceptionCode)
    {
        if (factionOut != NULL)
        {
            *factionOut = NULL;
        }
        if (ownershipsOut != NULL)
        {
            *ownershipsOut = NULL;
        }
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        __try
        {
            if (player == NULL || factionOut == NULL || ownershipsOut == NULL)
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

            *factionOut = faction;
            *ownershipsOut = ownerships;
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

    __declspec(noinline) bool OwnershipProbeTryCall(
        Ownerships* ownerships,
        lektor<hand>* output,
        DWORD* exceptionCode)
    {
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        __try
        {
            if (ownerships == NULL || output == NULL)
            {
                return false;
            }
            ownerships->getOwnedBuildingsH(*output);
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

    __declspec(noinline) bool OwnershipProbeTryReadHeaders(
        Ownerships* ownerships,
        const lektor<hand>* output,
        OwnershipProbeLektorHeader* sourceHeader,
        OwnershipProbeLektorHeader* outputHeader,
        DWORD* exceptionCode)
    {
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        __try
        {
            if (ownerships == NULL || output == NULL ||
                sourceHeader == NULL || outputHeader == NULL)
            {
                return false;
            }

            const lektor<hand>* source = &ownerships->stuff;
            sourceHeader->object = source;
            sourceHeader->vtable = *reinterpret_cast<const void* const*>(source);
            sourceHeader->count = source->count;
            sourceHeader->maxSize = source->maxSize;
            sourceHeader->stuff = source->stuff;

            outputHeader->object = output;
            outputHeader->vtable = *reinterpret_cast<const void* const*>(output);
            outputHeader->count = output->count;
            outputHeader->maxSize = output->maxSize;
            outputHeader->stuff = output->stuff;
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

    __declspec(noinline) bool OwnershipProbeTryReadRawHands(
        const hand* input,
        unsigned int count,
        OwnershipProbeRawHand* output,
        DWORD* exceptionCode)
    {
        if (exceptionCode != NULL)
        {
            *exceptionCode = 0;
        }

        __try
        {
            if ((count != 0 && input == NULL) || output == NULL || count > 8)
            {
                return false;
            }

            for (unsigned int i = 0; i < count; ++i)
            {
                const hand* current = input + i;
                output[i].vtable =
                    *reinterpret_cast<const void* const*>(current);
                output[i].type = static_cast<unsigned int>(current->type);
                output[i].container = current->container;
                output[i].containerSerial = current->containerSerial;
                output[i].index = current->index;
                output[i].serial = current->serial;
            }
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

    bool OwnershipProbeHeaderIsSane(
        const OwnershipProbeLektorHeader& header)
    {
        const unsigned int MAX_PROBE_CAPACITY = 65536;
        if (header.count > header.maxSize ||
            header.maxSize > MAX_PROBE_CAPACITY)
        {
            return false;
        }
        if (header.count != 0 && header.stuff == NULL)
        {
            return false;
        }
        return true;
    }

    bool OwnershipProbeResolveSameOwner(
        PlayerInterface* player,
        Faction** factionOut,
        Ownerships** ownershipsOut)
    {
        DWORD exceptionCode = 0;
        if (!OwnershipProbeTryResolveOwner(
                player, factionOut, ownershipsOut, &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("resolve-owner", exceptionCode);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "owner resolution failed; load a player save and retry");
            }
            return false;
        }

        if (g_ownershipProbeOwnerships != NULL &&
            (*ownershipsOut != g_ownershipProbeOwnerships ||
             *factionOut != g_ownershipProbeFaction ||
             player != g_ownershipProbePlayer))
        {
            OwnershipProbeWriteLog(
                "owner identity changed; abandoning probe until process restart");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            g_ownershipProbePlayer = NULL;
            g_ownershipProbeFaction = NULL;
            g_ownershipProbeOwnerships = NULL;
            return false;
        }
        return true;
    }

    void OwnershipProbeRunStage1(PlayerInterface* player)
    {
        if (InterlockedCompareExchange(
                &g_ownershipProbeState,
                OWNERSHIP_PROBE_IDLE,
                OWNERSHIP_PROBE_IDLE) != OWNERSHIP_PROBE_IDLE)
        {
            OwnershipProbeWriteLog(
                "stage=1 rejected; probe is one-shot and requires a process restart");
            return;
        }

        Faction* faction = NULL;
        Ownerships* ownerships = NULL;
        if (!OwnershipProbeResolveSameOwner(player, &faction, &ownerships))
        {
            return;
        }

        char message[512];
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=1 before-call player=%p faction=%p ownerships=%p "
            "out=%p sizeof(lektor<hand>)=0x%I64X",
            player, faction, ownerships, &g_ownershipProbeOutput,
            static_cast<unsigned __int64>(sizeof(lektor<hand>)));
        OwnershipProbeWriteLog(message);
        OwnershipProbeWriteLog(
            "stage=1 entering getOwnedBuildingsH; output will not be read or freed");

        DWORD exceptionCode = 0;
        if (!OwnershipProbeTryCall(
                ownerships, &g_ownershipProbeOutput, &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("1-call", exceptionCode);
            }
            else
            {
                OwnershipProbeWriteLog("stage=1 call rejected without exception");
            }
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }

        g_ownershipProbePlayer = player;
        g_ownershipProbeFaction = faction;
        g_ownershipProbeOwnerships = ownerships;
        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_CALL_RETURNED);
        OwnershipProbeWriteLog(
            "stage=1 returned; output remains unread and intentionally unfreed");
    }

    void OwnershipProbeRunStage2(PlayerInterface* player)
    {
        if (InterlockedCompareExchange(
                &g_ownershipProbeState,
                OWNERSHIP_PROBE_CALL_RETURNED,
                OWNERSHIP_PROBE_CALL_RETURNED) !=
            OWNERSHIP_PROBE_CALL_RETURNED)
        {
            OwnershipProbeWriteLog(
                "stage=2 rejected; stage 1 must return successfully first");
            return;
        }

        Faction* faction = NULL;
        Ownerships* ownerships = NULL;
        if (!OwnershipProbeResolveSameOwner(player, &faction, &ownerships))
        {
            return;
        }

        OwnershipProbeWriteLog(
            "stage=2 reading source and output lektor headers only");
        DWORD exceptionCode = 0;
        if (!OwnershipProbeTryReadHeaders(
                ownerships,
                &g_ownershipProbeOutput,
                &g_ownershipProbeSourceHeader,
                &g_ownershipProbeOutputHeader,
                &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("2-header", exceptionCode);
            }
            else
            {
                OwnershipProbeWriteLog("stage=2 header read rejected");
            }
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }

        char message[768];
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=2 source object=%p vtable=%p count=%u max=%u stuff=%p",
            g_ownershipProbeSourceHeader.object,
            g_ownershipProbeSourceHeader.vtable,
            g_ownershipProbeSourceHeader.count,
            g_ownershipProbeSourceHeader.maxSize,
            g_ownershipProbeSourceHeader.stuff);
        OwnershipProbeWriteLog(message);
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=2 output object=%p vtable=%p count=%u max=%u stuff=%p",
            g_ownershipProbeOutputHeader.object,
            g_ownershipProbeOutputHeader.vtable,
            g_ownershipProbeOutputHeader.count,
            g_ownershipProbeOutputHeader.maxSize,
            g_ownershipProbeOutputHeader.stuff);
        OwnershipProbeWriteLog(message);

        g_ownershipProbeOutputAliasesSource =
            g_ownershipProbeOutputHeader.stuff != NULL &&
            g_ownershipProbeOutputHeader.stuff ==
                g_ownershipProbeSourceHeader.stuff;
        _snprintf_s(
            message, sizeof(message), _TRUNCATE,
            "stage=2 output-aliases-source=%s output-sane=%s",
            g_ownershipProbeOutputAliasesSource ? "yes" : "no",
            OwnershipProbeHeaderIsSane(g_ownershipProbeOutputHeader)
                ? "yes" : "no");
        OwnershipProbeWriteLog(message);

        if (!OwnershipProbeHeaderIsSane(g_ownershipProbeOutputHeader))
        {
            OwnershipProbeWriteLog(
                "stage=2 output header is not sane; refusing all element reads");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }

        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_HEADER_READ);
        OwnershipProbeWriteLog("stage=2 complete; no elements were read or freed");
    }

    void OwnershipProbeRunStage3(PlayerInterface* player)
    {
        if (InterlockedCompareExchange(
                &g_ownershipProbeState,
                OWNERSHIP_PROBE_HEADER_READ,
                OWNERSHIP_PROBE_HEADER_READ) != OWNERSHIP_PROBE_HEADER_READ)
        {
            OwnershipProbeWriteLog(
                "stage=3 rejected; stage 2 must complete successfully first");
            return;
        }

        Faction* faction = NULL;
        Ownerships* ownerships = NULL;
        if (!OwnershipProbeResolveSameOwner(player, &faction, &ownerships))
        {
            return;
        }

        OwnershipProbeLektorHeader sourceNow;
        OwnershipProbeLektorHeader outputNow;
        DWORD exceptionCode = 0;
        if (!OwnershipProbeTryReadHeaders(
                ownerships,
                &g_ownershipProbeOutput,
                &sourceNow,
                &outputNow,
                &exceptionCode))
        {
            OwnershipProbeLogException("3-revalidate-header", exceptionCode);
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }

        if (outputNow.count != g_ownershipProbeOutputHeader.count ||
            outputNow.maxSize != g_ownershipProbeOutputHeader.maxSize ||
            outputNow.stuff != g_ownershipProbeOutputHeader.stuff)
        {
            OwnershipProbeWriteLog(
                "stage=3 output header changed; refusing stale element reads");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            return;
        }

        if (g_ownershipProbeOutputAliasesSource)
        {
            OwnershipProbeWriteLog(
                "stage=3 refused because output aliases engine-owned source memory");
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            return;
        }

        const unsigned int readCount =
            outputNow.count < 8 ? outputNow.count : 8;
        OwnershipProbeRawHand rawHands[8];
        OwnershipProbeWriteLog(
            "stage=3 reading at most eight raw hand records; no handle methods will run");
        if (!OwnershipProbeTryReadRawHands(
                outputNow.stuff, readCount, rawHands, &exceptionCode))
        {
            if (exceptionCode != 0)
            {
                OwnershipProbeLogException("3-hands", exceptionCode);
            }
            else
            {
                OwnershipProbeWriteLog("stage=3 raw handle read rejected");
            }
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_FAILED);
            return;
        }

        for (unsigned int i = 0; i < readCount; ++i)
        {
            char message[512];
            _snprintf_s(
                message, sizeof(message), _TRUNCATE,
                "stage=3 hand[%u] vtable=%p type=%u container=%u "
                "containerSerial=%u index=%u serial=%u",
                i,
                rawHands[i].vtable,
                rawHands[i].type,
                rawHands[i].container,
                rawHands[i].containerSerial,
                rawHands[i].index,
                rawHands[i].serial);
            OwnershipProbeWriteLog(message);
        }

        InterlockedExchange(
            &g_ownershipProbeState, OWNERSHIP_PROBE_HANDLES_READ);
        OwnershipProbeWriteLog(
            "stage=3 complete; buffer remains intentionally unfreed");
    }

    void OwnershipProbeRequestStage(int stage)
    {
        if (stage >= 1 && stage <= 3)
        {
            InterlockedExchange(&g_ownershipProbeRequestedStage, stage);
        }
    }

    LONG OwnershipProbeGetState()
    {
        return InterlockedCompareExchange(
            &g_ownershipProbeState,
            OWNERSHIP_PROBE_IDLE,
            OWNERSHIP_PROBE_IDLE);
    }

    void OwnershipProbeOnWorldReset()
    {
        InterlockedExchange(&g_ownershipProbeRequestedStage, 0);
        g_ownershipProbeF10WasDown = false;

        const LONG state = OwnershipProbeGetState();
        if (state != OWNERSHIP_PROBE_IDLE)
        {
            // Never dereference or free retained data while the world tears
            // down.  A possible deep-copy allocation is intentionally leaked
            // until process exit; retrying requires a full process restart.
            g_ownershipProbePlayer = NULL;
            g_ownershipProbeFaction = NULL;
            g_ownershipProbeOwnerships = NULL;
            InterlockedExchange(
                &g_ownershipProbeState, OWNERSHIP_PROBE_ABANDONED);
            OwnershipProbeWriteLog(
                "world reset detected; retained pointers abandoned without access or free");
        }
    }

    void TickOwnershipProbe(PlayerInterface* player)
    {
        const bool modifiersDown =
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool f10Down =
            modifiersDown && (GetAsyncKeyState(VK_F10) & 0x8000) != 0;

        if (f10Down && !g_ownershipProbeF10WasDown)
        {
            const LONG state = OwnershipProbeGetState();
            if (state == OWNERSHIP_PROBE_IDLE)
            {
                OwnershipProbeRequestStage(1);
            }
            else if (state == OWNERSHIP_PROBE_CALL_RETURNED)
            {
                OwnershipProbeRequestStage(2);
            }
            else if (state == OWNERSHIP_PROBE_HEADER_READ)
            {
                OwnershipProbeRequestStage(3);
            }
            else
            {
                OwnershipProbeWriteLog(
                    "Ctrl+Shift+F10 ignored; probe is complete, failed, or abandoned");
            }
        }

        g_ownershipProbeF10WasDown = f10Down;

        const LONG requested =
            InterlockedExchange(&g_ownershipProbeRequestedStage, 0);
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
    }
