// SPDX-License-Identifier: GPL-3.0-only
    const SkillDefinition SKILL_DEFINITIONS[] =
    {
        { STAT_STRENGTH,        "Strength" },
        { STAT_TOUGHNESS,       "Toughness" },
        { STAT_DEXTERITY,       "Dexterity" },
        { STAT_PERCEPTION,      "Perception" },

        { STAT_KATANAS,         "Katanas" },
        { STAT_SABRES,          "Sabres" },
        { STAT_HACKERS,         "Hackers" },
        { STAT_HEAVYWEAPONS,    "Heavy weapons" },
        { STAT_BLUNT,           "Blunt" },
        { STAT_POLEARMS,        "Polearms" },

        { STAT_MELEE_ATTACK,    "Melee attack" },
        { STAT_MELEE_DEFENCE,   "Melee defence" },
        { STAT_DODGE,           "Dodge" },
        { STAT_MARTIALARTS,     "Martial arts" },
        { STAT_WEAPONS,         "Weapons" },
        { STAT_MASSCOMBAT,      "Mass combat" },

        { STAT_TURRETS,         "Turrets" },
        { STAT_CROSSBOWS,       "Crossbows" },
        { STAT_FRIENDLY_FIRE,   "Precision shooting" },

        { STAT_STEALTH,         "Stealth" },
        { STAT_LOCKPICKING,     "Lockpicking" },
        { STAT_THIEVING,        "Thievery" },
        { STAT_ASSASSINATION,   "Assassination" },

        { STAT_ATHLETICS,       "Athletics" },
        { STAT_SWIMMING,        "Swimming" },
        { STAT_SURVIVAL,        "Survival" },

        { STAT_MEDIC,           "Field medic" },
        { STAT_HIVEMEDIC,       "Hive medic" },
        { STAT_VET,             "Veterinary" },
        { STAT_ENGINEERING,     "Engineer" },
        { STAT_ROBOTICS,        "Robotics" },
        { STAT_SCIENCE,         "Science" },
        { STAT_FARMING,         "Farming" },
        { STAT_COOKING,         "Cooking" },

        { STAT_SMITHING_WEAPON, "Weapon smith" },
        { STAT_SMITHING_ARMOUR, "Armour smith" },
        { STAT_SMITHING_BOW,    "Crossbow smith" },
        { STAT_LABOURING,       "Labouring" }
    };

    const size_t SKILL_DEFINITION_COUNT =
        sizeof(SKILL_DEFINITIONS) / sizeof(SKILL_DEFINITIONS[0]);

    void ResetHandleIdentity(HandleIdentity* identity)
    {
        if (identity != NULL)
        {
            *identity = HandleIdentity();
        }
    }

    void CaptureHandleIdentity(const hand& value, HandleIdentity* identity)
    {
        if (identity == NULL)
        {
            return;
        }

        identity->valid = static_cast<bool>(value);
        identity->type = value.type;
        identity->container = value.container;
        identity->containerSerial = value.containerSerial;
        identity->index = value.index;
        identity->serial = value.serial;
    }

    hand RestoreHandleIdentity(const HandleIdentity& identity)
    {
        if (!identity.valid)
        {
            return hand();
        }
        return hand(
            identity.index, identity.serial, identity.type,
            identity.container, identity.containerSerial);
    }

    bool SameHandleIdentity(
        const HandleIdentity& left,
        const HandleIdentity& right)
    {
        return left.valid == right.valid &&
            left.type == right.type &&
            left.container == right.container &&
            left.containerSerial == right.containerSerial &&
            left.index == right.index &&
            left.serial == right.serial;
    }

    bool HasElapsed(DWORD now, DWORD then, DWORD interval)
    {
        return static_cast<DWORD>(now - then) >= interval;
    }

    int ClampInt(int value, int low, int high)
    {
        if (value < low)
        {
            return low;
        }
        if (value > high)
        {
            return high;
        }
        return value;
    }

    std::string IntegerString(size_t value)
    {
        std::ostringstream text;
        text << value;
        return text.str();
    }

    void LogInfo(const std::string& message)
    {
        DebugLog(message.c_str());
    }

    std::string CurrentStatusTimestamp()
    {
        SYSTEMTIME now;
        GetLocalTime(&now);
        std::ostringstream timestamp;
        timestamp << (now.wHour < 10 ? "0" : "") << now.wHour << ":";
        timestamp << (now.wMinute < 10 ? "0" : "") << now.wMinute;
        return timestamp.str();
    }

    void RefreshStatusMessageFrame()
    {
        if (g_statusText == NULL)
        {
            return;
        }

        std::ostringstream caption;
        const size_t firstMessage =
            g_recentStatusMessages.size() > 3 ?
                g_recentStatusMessages.size() - 3 : 0;
        for (size_t index = firstMessage;
             index < g_recentStatusMessages.size(); ++index)
        {
            if (index != firstMessage)
            {
                caption << "\n";
            }
            caption << "[" << g_recentStatusMessages[index].timestamp << "] "
                    << g_recentStatusMessages[index].text;
        }
        g_statusText->setCaption(caption.str().c_str());
    }

    void RecordRecentManagerMessage(const std::string& message)
    {
        const DWORD now = GetTickCount();
        if (message.empty() ||
            (!g_recentStatusMessages.empty() &&
             g_recentStatusMessages.back().text == message &&
             g_lastRecentStatusMessageTick != 0 &&
             !HasElapsed(
                 now,
                 g_lastRecentStatusMessageTick,
                 RECENT_MESSAGE_DEDUP_WINDOW_MS)))
        {
            return;
        }
        try
        {
            if (g_recentStatusMessages.size() >=
                MAX_RECENT_STATUS_MESSAGES)
            {
                g_recentStatusMessages.erase(
                    g_recentStatusMessages.begin());
            }
            g_recentStatusMessages.push_back(
                RecentStatusMessage(CurrentStatusTimestamp(), message));
            g_lastRecentStatusMessageTick = now;
            RefreshStatusMessageFrame();
        }
        catch (...)
        {
            // The status display remains useful even if the optional
            // in-memory history cannot allocate another entry.
        }
    }

    void SetStatus(const std::string& message)
    {
        if (g_statusText != NULL)
        {
            RecordRecentManagerMessage(message);
            RefreshStatusMessageFrame();
        }
    }

    bool TryGetCurrentSquad(
        PlayerInterface* player,
        hand* squadHandleOut,
        std::string* nameOut,
        RootObjectContainer** activeOut)
    {
        if (squadHandleOut == NULL || nameOut == NULL || activeOut == NULL)
        {
            return false;
        }

        squadHandleOut->setNull();
        nameOut->clear();
        *activeOut = NULL;
        if (player == NULL)
        {
            return false;
        }

        __try
        {
            Platoon* platoon = player->getCurrentPlatoon();
            if (platoon == NULL)
            {
                return false;
            }

            *squadHandleOut = platoon->getHandle();
            *activeOut = platoon->getActivePlatoon();
            if (*activeOut != NULL)
            {
                *nameOut = static_cast<ActivePlatoon*>(*activeOut)->getName();
            }
            if (nameOut->empty())
            {
                // RootObjectBase::getName returns by value and cannot safely be
                // invoked inside a VC100 SEH frame. stringID is the stable
                // unloaded-squad fallback; the active platoon supplies the
                // player-facing name whenever it is loaded.
                *nameOut = platoon->stringID;
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while reading the current squad.");
            squadHandleOut->setNull();
            *activeOut = NULL;
            return false;
        }
    }

    bool ContainsSquadSelectorIdentity(
        const std::vector<SquadSelectorEntry>& entries,
        const HandleIdentity& identity)
    {
        for (size_t index = 0; index < entries.size(); ++index)
        {
            if (SameHandleIdentity(entries[index].identity, identity))
            {
                return true;
            }
        }
        return false;
    }

    // Copy the raw active-platoon list into value-only UI records in its
    // native lektor order. All borrowed engine pointers stay inside this
    // guarded read and are discarded before it returns.
    __declspec(noinline) bool TryBuildSquadSelectorSnapshotGuarded(
        PlayerInterface* player,
        std::vector<SquadSelectorEntry>* entriesOut,
        bool* incompleteOut)
    {
        if (player == NULL || entriesOut == NULL || incompleteOut == NULL)
        {
            return false;
        }

        size_t committedCount = 0;
        __try
        {
            Faction* faction = player->getFaction();
            if (faction == NULL || !faction->isThePlayer())
            {
                return false;
            }

            const lektor<Platoon*>* activePlatoons =
                faction->getActivePlatoons();
            if (activePlatoons == NULL)
            {
                return false;
            }

            const unsigned int rawCount = activePlatoons->size();
            if (rawCount != 0 && !activePlatoons->valid())
            {
                return false;
            }
            const unsigned int scanCount = rawCount > 256 ? 256 : rawCount;
            *incompleteOut = rawCount > scanCount;

            HandleIdentity deadIdentity;
            CaptureHandleIdentity(player->getDeadSquadHandle(), &deadIdentity);

            entriesOut->reserve(static_cast<size_t>(scanCount));
            for (unsigned int index = 0; index < scanCount; ++index)
            {
                Platoon* platoon = (*activePlatoons)[index];
                if (platoon == NULL || platoon->getFaction() != faction)
                {
                    continue;
                }

                ActivePlatoon* active = platoon->getActivePlatoon();
                if (active == NULL)
                {
                    continue;
                }
                const int memberCount = active->getNumThings();
                if (memberCount <= 0)
                {
                    continue;
                }

                HandleIdentity identity;
                CaptureHandleIdentity(platoon->getHandle(), &identity);
                if (!identity.valid ||
                    (deadIdentity.valid &&
                     SameHandleIdentity(identity, deadIdentity)) ||
                    ContainsSquadSelectorIdentity(*entriesOut, identity))
                {
                    continue;
                }

                const std::string& activeName = active->getName();
                if (activeName == "__DEAD__" ||
                    platoon->stringID == "__DEAD__")
                {
                    continue;
                }

                entriesOut->resize(entriesOut->size() + 1);
                SquadSelectorEntry& entry = entriesOut->back();
                entry.identity = identity;
                entry.memberCount = memberCount;
                if (!activeName.empty())
                {
                    entry.name = activeName;
                }
                else
                {
                    entry.name = platoon->stringID;
                }
                committedCount = entriesOut->size();
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            entriesOut->resize(committedCount);
            *incompleteOut = true;
            ErrorLog("[KenshiJobManagement] Exception while enumerating player squads.");
            return false;
        }
    }

    bool BuildSquadSelectorSnapshot(
        std::vector<SquadSelectorEntry>* entriesOut,
        bool* incompleteOut)
    {
        if (entriesOut == NULL || incompleteOut == NULL)
        {
            return false;
        }
        entriesOut->clear();
        *incompleteOut = false;
        try
        {
            if (!TryBuildSquadSelectorSnapshotGuarded(
                    g_playerInterface, entriesOut, incompleteOut))
            {
                entriesOut->clear();
                *incompleteOut = true;
                return false;
            }
            return true;
        }
        catch (...)
        {
            // Keep C++ allocation failures outside the SEH leaf. The UI can
            // retain its previous published snapshot when this fresh build
            // reports failure.
            entriesOut->clear();
            *incompleteOut = true;
            return false;
        }
    }

    // Temporary value-only bridge between the borrowed active-platoon list
    // and the durable all-squads board. The guarded enumerator below copies
    // only scalar handle identities, names, and bounds information.
    struct ActiveSquadValueSeed
    {
        HandleIdentity identity;
        std::string name;
        std::vector<HandleIdentity> members;
        bool incomplete;

        ActiveSquadValueSeed() : incomplete(false) {}
    };

    bool ContainsActiveSquadSeedIdentity(
        const std::vector<ActiveSquadValueSeed>& seeds,
        const HandleIdentity& identity)
    {
        for (size_t index = 0; index < seeds.size(); ++index)
        {
            if (SameHandleIdentity(seeds[index].identity, identity))
            {
                return true;
            }
        }
        return false;
    }

    // Copy every active player squad in the exact lektor order used by
    // vanilla squad cycling. Borrowed Platoon, ActivePlatoon, Character, and
    // RootObject pointers never leave this guarded call.
    __declspec(noinline) bool TryBuildActiveSquadValueSeedsGuarded(
        PlayerInterface* player,
        std::vector<ActiveSquadValueSeed>* seedsOut,
        bool* incompleteOut)
    {
        if (player == NULL || seedsOut == NULL || incompleteOut == NULL)
        {
            return false;
        }

        size_t committedSquadCount = 0;
        __try
        {
            Faction* faction = player->getFaction();
            if (faction == NULL || !faction->isThePlayer())
            {
                return false;
            }

            const lektor<Platoon*>* activePlatoons =
                faction->getActivePlatoons();
            if (activePlatoons == NULL)
            {
                return false;
            }

            const unsigned int rawSquadCount = activePlatoons->size();
            if (rawSquadCount != 0 && !activePlatoons->valid())
            {
                return false;
            }
            const unsigned int squadScanCount =
                rawSquadCount > 256 ? 256 : rawSquadCount;
            *incompleteOut = rawSquadCount > squadScanCount;

            HandleIdentity deadIdentity;
            CaptureHandleIdentity(player->getDeadSquadHandle(), &deadIdentity);

            seedsOut->reserve(static_cast<size_t>(squadScanCount));
            for (unsigned int squadIndex = 0;
                 squadIndex < squadScanCount;
                 ++squadIndex)
            {
                Platoon* platoon = (*activePlatoons)[squadIndex];
                if (platoon == NULL || platoon->getFaction() != faction)
                {
                    continue;
                }

                ActivePlatoon* active = platoon->getActivePlatoon();
                if (active == NULL)
                {
                    continue;
                }
                const int rawMemberCount = active->getNumThings();
                if (rawMemberCount <= 0)
                {
                    continue;
                }

                HandleIdentity squadIdentity;
                CaptureHandleIdentity(platoon->getHandle(), &squadIdentity);
                if (!squadIdentity.valid ||
                    (deadIdentity.valid &&
                     SameHandleIdentity(squadIdentity, deadIdentity)) ||
                    ContainsActiveSquadSeedIdentity(
                        *seedsOut, squadIdentity))
                {
                    continue;
                }

                const std::string& activeName = active->getName();
                if (activeName == "__DEAD__" ||
                    platoon->stringID == "__DEAD__")
                {
                    continue;
                }

                seedsOut->resize(seedsOut->size() + 1);
                ActiveSquadValueSeed& seed = seedsOut->back();
                seed.identity = squadIdentity;
                seed.name = !activeName.empty() ?
                    activeName : platoon->stringID;
                const int memberScanCount =
                    rawMemberCount > 256 ? 256 : rawMemberCount;
                seed.incomplete = rawMemberCount > memberScanCount;
                if (seed.incomplete)
                {
                    *incompleteOut = true;
                }
                seed.members.reserve(
                    static_cast<size_t>(memberScanCount));

                for (int memberIndex = 0;
                     memberIndex < memberScanCount;
                     ++memberIndex)
                {
                    RootObject* object = active->getThing(memberIndex);
                    if (object == NULL ||
                        object->getDataType() != CHARACTER)
                    {
                        continue;
                    }
                    Character* character =
                        static_cast<Character*>(object);
                    if (!character->isPlayerCharacter() ||
                        character->getFaction() != faction)
                    {
                        continue;
                    }

                    HandleIdentity memberIdentity;
                    CaptureHandleIdentity(
                        character->getHandle(), &memberIdentity);
                    if (memberIdentity.valid)
                    {
                        seed.members.push_back(memberIdentity);
                    }
                }
                committedSquadCount = seedsOut->size();
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            seedsOut->resize(committedSquadCount);
            *incompleteOut = true;
            ErrorLog(
                "[KenshiJobManagement] Exception while copying the all-squads roster.");
            return false;
        }
    }

    bool BuildActiveSquadValueSeeds(
        std::vector<ActiveSquadValueSeed>* seedsOut,
        bool* incompleteOut)
    {
        if (seedsOut == NULL || incompleteOut == NULL)
        {
            return false;
        }
        seedsOut->clear();
        *incompleteOut = false;
        try
        {
            if (!TryBuildActiveSquadValueSeedsGuarded(
                    g_playerInterface, seedsOut, incompleteOut))
            {
                seedsOut->clear();
                *incompleteOut = true;
                return false;
            }
            return true;
        }
        catch (...)
        {
            seedsOut->clear();
            *incompleteOut = true;
            return false;
        }
    }

    // Keep the engine mutation in a small guarded leaf. The caller validates
    // the fresh active-list candidate immediately before entering this leaf.
    __declspec(noinline) bool TrySetCurrentPlatoonGuardedLeaf(
        PlayerInterface* player,
        Platoon* platoon)
    {
        if (player == NULL || platoon == NULL)
        {
            return false;
        }
        __try
        {
            return player->setCurrentPlatoon(platoon);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while selecting a squad.");
            return false;
        }
    }

    __declspec(noinline) bool TrySelectSquadByIdentityGuarded(
        PlayerInterface* player,
        const HandleIdentity& target)
    {
        if (player == NULL || !target.valid)
        {
            return false;
        }

        __try
        {
            Faction* faction = player->getFaction();
            if (faction == NULL || !faction->isThePlayer())
            {
                return false;
            }
            const lektor<Platoon*>* activePlatoons =
                faction->getActivePlatoons();
            if (activePlatoons == NULL)
            {
                return false;
            }

            const unsigned int rawCount = activePlatoons->size();
            if (rawCount != 0 && !activePlatoons->valid())
            {
                return false;
            }
            const unsigned int scanCount = rawCount > 256 ? 256 : rawCount;

            HandleIdentity deadIdentity;
            CaptureHandleIdentity(player->getDeadSquadHandle(), &deadIdentity);
            for (unsigned int index = 0; index < scanCount; ++index)
            {
                Platoon* platoon = (*activePlatoons)[index];
                if (platoon == NULL || platoon->getFaction() != faction)
                {
                    continue;
                }

                HandleIdentity candidate;
                CaptureHandleIdentity(platoon->getHandle(), &candidate);
                if (!SameHandleIdentity(candidate, target) ||
                    (deadIdentity.valid &&
                     SameHandleIdentity(candidate, deadIdentity)))
                {
                    continue;
                }

                ActivePlatoon* active = platoon->getActivePlatoon();
                if (active == NULL || active->getNumThings() <= 0)
                {
                    return false;
                }
                const std::string& activeName = active->getName();
                if (activeName == "__DEAD__" ||
                    platoon->stringID == "__DEAD__")
                {
                    return false;
                }

                Platoon* current = player->getCurrentPlatoon();
                if (current != NULL)
                {
                    HandleIdentity currentIdentity;
                    CaptureHandleIdentity(
                        current->getHandle(), &currentIdentity);
                    if (SameHandleIdentity(currentIdentity, target))
                    {
                        return true;
                    }
                }

                // Do not trust the engine return value alone. A late fault or
                // false result can still leave currentPlatoon changed.
                TrySetCurrentPlatoonGuardedLeaf(player, platoon);
                current = player->getCurrentPlatoon();
                if (current == NULL)
                {
                    return false;
                }
                HandleIdentity selectedIdentity;
                CaptureHandleIdentity(current->getHandle(), &selectedIdentity);
                if (!SameHandleIdentity(selectedIdentity, target))
                {
                    return false;
                }
                return true;
            }
            return false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while validating a squad selection.");
            return false;
        }
    }

    bool TrySelectSquadByIdentity(const HandleIdentity& target)
    {
        return TrySelectSquadByIdentityGuarded(
            g_playerInterface, target);
    }

    bool TryGetOrderedMemberHandles(
        RootObjectContainer* active,
        std::vector<hand>* handlesOut,
        bool* incompleteOut = NULL)
    {
        if (active == NULL || handlesOut == NULL)
        {
            return false;
        }

        handlesOut->clear();
        if (incompleteOut != NULL)
        {
            *incompleteOut = false;
        }
        __try
        {
            int count = active->getNumThings();
            if (count < 0)
            {
                return false;
            }

            const int scanCount = count > 256 ? 256 : count;
            if (incompleteOut != NULL)
            {
                *incompleteOut = count > scanCount;
            }

            handlesOut->reserve(static_cast<size_t>(scanCount));
            for (int index = 0; index < scanCount; ++index)
            {
                RootObject* object = active->getThing(index);
                if (object != NULL && object->getDataType() == CHARACTER)
                {
                    Character* character = static_cast<Character*>(object);
                    if (character->isPlayerCharacter())
                    {
                        handlesOut->push_back(object->getHandle());
                    }
                }
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while enumerating squad members.");
            handlesOut->clear();
            return false;
        }
    }

    bool TryResolveCharacter(const hand& member, Character** characterOut)
    {
        if (characterOut == NULL)
        {
            return false;
        }
        *characterOut = NULL;

        __try
        {
            if (!member || !member.isValid())
            {
                return false;
            }
            Character* character = member.getCharacter();
            if (character == NULL || !character->isPlayerCharacter())
            {
                return false;
            }
            Faction* faction = character->getFaction();
            if (faction == NULL || !faction->isThePlayer())
            {
                return false;
            }
            *characterOut = character;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while resolving a squad member.");
            return false;
        }
    }

    __declspec(noinline) bool CallRootObjectName(
        RootObjectBase* object,
        std::string* nameOut)
    {
        try
        {
            *nameOut = object->getName();
            return !nameOut->empty();
        }
        catch (...)
        {
            return false;
        }
    }

    bool TryGetCharacterName(Character* character, std::string* nameOut)
    {
        if (character == NULL || nameOut == NULL)
        {
            return false;
        }
        __try
        {
            return CallRootObjectName(character, nameOut);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryGetJobsEnabled(Character* character, bool* enabledOut)
    {
        if (character == NULL || enabledOut == NULL)
        {
            return false;
        }
        __try
        {
            OrdersReceiver* receiver = character->getOrdersReciever();
            if (receiver == NULL)
            {
                return false;
            }
            *enabledOut = receiver->isJobsEnabled();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryGetPermajobCount(Character* character, int* countOut)
    {
        if (character == NULL || countOut == NULL)
        {
            return false;
        }
        __try
        {
            *countOut = character->getPermajobCount();
            return *countOut >= 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryConstructOrderData(OrderData* storage)
    {
        if (storage == NULL)
        {
            return false;
        }
        __try
        {
            storage->_CONSTRUCTOR();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void TryDestructOrderData(OrderData* order)
    {
        if (order == NULL)
        {
            return;
        }
        __try
        {
            order->_DESTRUCTOR();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] OrderData destruction failed.");
        }
    }

    OrderData* CreateOrderDataSafely()
    {
        void* storage = std::malloc(sizeof(OrderData));
        if (storage == NULL)
        {
            return NULL;
        }
        OrderData* order = reinterpret_cast<OrderData*>(storage);
        if (!TryConstructOrderData(order))
        {
            std::free(storage);
            return NULL;
        }
        return order;
    }

    void DestroyOrderDataSafely(OrderData* order)
    {
        if (order != NULL)
        {
            TryDestructOrderData(order);
            std::free(order);
        }
    }

    bool TryFillOrderText(
        OrderData* order,
        const Tasker* task,
        int slot,
        bool enabled,
        std::string* textOut)
    {
        __try
        {
            order->set(task, slot, enabled);
            order->updateText();
            *textOut = order->text;
            return !textOut->empty();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryGetOrderText(const Tasker* task, int slot, bool enabled, std::string* textOut)
    {
        if (task == NULL || textOut == NULL)
        {
            return false;
        }

        OrderData* order = CreateOrderDataSafely();
        if (order == NULL)
        {
            return false;
        }
        const bool success =
            TryFillOrderText(order, task, slot, enabled, textOut);
        DestroyOrderDataSafely(order);
        return success;
    }

    bool TryCopyTaskSubject(const Tasker* task, hand* targetOut)
    {
        if (task == NULL || targetOut == NULL)
        {
            return false;
        }
        void* storage = std::malloc(sizeof(TaskMatch));
        if (storage == NULL)
        {
            return false;
        }
        bool success = false;
        __try
        {
            // Let Kenshi translate the opaque Tasker through its exported
            // TaskMatch adapter. This avoids depending on a raw Tasker field
            // offset while still copying the target into a stable handle now.
            TaskMatch* match = reinterpret_cast<TaskMatch*>(storage);
            if (match->_CONSTRUCTOR(const_cast<Tasker*>(task)) != NULL)
            {
                *targetOut = match->subject;
                success = true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            targetOut->setNull();
        }
        std::free(storage);
        return success;
    }

    bool TryReadPermajobTargetContract(
        const Tasker* task,
        bool* fixedTargetOut,
        TaskType* associatedTaskTypeOut)
    {
        if (fixedTargetOut == NULL || associatedTaskTypeOut == NULL)
        {
            return false;
        }
        *fixedTargetOut = false;
        *associatedTaskTypeOut = NULL_TASK;
        // A missing Tasker cannot describe a fixed target.  Keep the row
        // readable so a malformed global job does not become a false red
        // card.
        if (task == NULL)
        {
            return true;
        }

        // Tasker and TaskData are intentionally opaque in this translation
        // unit.  These are the documented KenshiLib 0.4.0 x64 offsets used by
        // GeneralJobTransfer.inl.  Read them only inside the SEH guard and
        // retain no borrowed pointer after this function returns.
        const size_t TASKER_DATA_OFFSET = 0x70;
        const size_t TASK_DATA_FIXED_TARGET_OFFSET = 0x08;
        const size_t TASK_DATA_ASSOCIATED_OFFSET = 0x0C;
        __try
        {
            const unsigned char* taskBytes =
                reinterpret_cast<const unsigned char*>(task);
            const unsigned char* dataBytes =
                *reinterpret_cast<const unsigned char* const*>(
                    taskBytes + TASKER_DATA_OFFSET);
            if (dataBytes == NULL)
            {
                return false;
            }
            *fixedTargetOut = *reinterpret_cast<const bool*>(
                dataBytes + TASK_DATA_FIXED_TARGET_OFFSET);
            *associatedTaskTypeOut = *reinterpret_cast<const TaskType*>(
                dataBytes + TASK_DATA_ASSOCIATED_OFFSET);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            *fixedTargetOut = false;
            *associatedTaskTypeOut = NULL_TASK;
            return false;
        }
    }

    bool TryReadPermajobRaw(
        Character* character,
        int slot,
        JobRowSnapshot* rowOut,
        const Tasker** taskOut)
    {
        __try
        {
            const int count = character->getPermajobCount();
            if (slot >= count)
            {
                return false;
            }
            rowOut->taskType = character->getPermajob(slot);
            *taskOut = character->getPermajobData(slot);
            rowOut->jobLabel = character->getPermajobName(slot);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryGetPermajob(
        Character* character,
        int slot,
        bool jobsEnabled,
        JobRowSnapshot* rowOut,
        hand* targetOut)
    {
        if (character == NULL || slot < 0 || rowOut == NULL || targetOut == NULL)
        {
            return false;
        }
        targetOut->setNull();

        const Tasker* task = NULL;
        if (!TryReadPermajobRaw(character, slot, rowOut, &task))
        {
            return false;
        }
        rowOut->taskToken = reinterpret_cast<ULONG_PTR>(task);
        if (!TryReadPermajobTargetContract(
                task, &rowOut->fixedTarget,
                &rowOut->associatedTaskType))
        {
            return false;
        }
        if (task != NULL && !TryCopyTaskSubject(task, targetOut))
        {
            return false;
        }

        std::string vanillaText;
        if (TryGetOrderText(task, slot, jobsEnabled, &vanillaText))
        {
            rowOut->jobLabel = vanillaText;
        }
        if (rowOut->jobLabel.empty())
        {
            rowOut->jobLabel = "Unknown permanent job";
        }

        rowOut->hasTarget = static_cast<bool>(*targetOut);
        CaptureHandleIdentity(*targetOut, &rowOut->target);
        rowOut->targetAvailable = false;
        rowOut->targetResolvable = false;
        rowOut->targetLabel.clear();
        return true;
    }

    bool TryResolveTargetObject(const hand& target, RootObjectBase** objectOut)
    {
        if (objectOut == NULL)
        {
            return false;
        }
        *objectOut = NULL;
        __try
        {
            if (!target || !target.isValid())
            {
                return false;
            }
            *objectOut = target.getRootObjectBase();
            if (*objectOut == NULL)
            {
                return false;
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryReadRootObjectName(RootObjectBase* object, std::string* nameOut)
    {
        if (object == NULL || nameOut == NULL)
        {
            return false;
        }
        __try
        {
            return CallRootObjectName(object, nameOut);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryResolveTargetName(const hand& target, std::string* nameOut)
    {
        if (nameOut == NULL)
        {
            return false;
        }
        nameOut->clear();
        RootObjectBase* object = NULL;
        return TryResolveTargetObject(target, &object) &&
            TryReadRootObjectName(object, nameOut);
    }

    bool TryReadStatValue(
        Character* character,
        StatsEnumerated stat,
        float* valueOut)
    {
        if (character == NULL || valueOut == NULL)
        {
            return false;
        }
        __try
        {
            CharStats* stats = character->getStats();
            if (stats == NULL)
            {
                return false;
            }
            *valueOut = stats->getStat(stat, true);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryGetStatValue(
        Character* character,
        StatsEnumerated stat,
        float* valueOut,
        std::string* nameOut)
    {
        if (nameOut == NULL || !TryReadStatValue(character, stat, valueOut))
        {
            return false;
        }
        // Stage 1 ships English labels. Avoid an engine call that returns a
        // std::string by value; the definition table supplies the exact label.
        nameOut->clear();
        return true;
    }

    bool TryGetLocalizedStatName(StatsEnumerated stat, std::string* nameOut)
    {
        if (nameOut == NULL)
        {
            return false;
        }
        for (size_t index = 0; index < SKILL_DEFINITION_COUNT; ++index)
        {
            if (SKILL_DEFINITIONS[index].stat == stat)
            {
                *nameOut = SKILL_DEFINITIONS[index].fallbackName;
                return true;
            }
        }
        nameOut->clear();
        return false;
    }

    bool SkillValueLess(const SkillValue& left, const SkillValue& right)
    {
        if (left.sortValue != right.sortValue)
        {
            return left.sortValue > right.sortValue;
        }
        return left.name < right.name;
    }

    void BuildTopSkills(Character* character, std::vector<SkillValue>* skillsOut)
    {
        skillsOut->clear();
        for (size_t index = 0; index < SKILL_DEFINITION_COUNT; ++index)
        {
            const SkillDefinition& definition = SKILL_DEFINITIONS[index];
            float raw = 0.0f;
            std::string name;
            if (!TryGetStatValue(character, definition.stat, &raw, &name))
            {
                continue;
            }
            if (raw <= 1.0f)
            {
                continue;
            }
            const int value = static_cast<int>(std::floor(raw));
            if (name.empty())
            {
                name = definition.fallbackName;
            }
            SkillValue skill;
            skill.stat = definition.stat;
            skill.name = name;
            skill.sortValue = raw;
            skill.value = value;
            skillsOut->push_back(skill);
        }

        std::sort(skillsOut->begin(), skillsOut->end(), SkillValueLess);
        if (skillsOut->size() > 3)
        {
            skillsOut->resize(3);
        }
    }

    bool TryGetBlockingCondition(Character* character, std::string* conditionOut)
    {
        if (character == NULL || conditionOut == NULL)
        {
            return false;
        }
        conditionOut->clear();

        __try
        {
            if (character->isDead())
            {
                *conditionOut = "Dead";
            }
            else if (character->getMedical() != NULL &&
                     character->getMedical()->isProbablyDying())
            {
                *conditionOut = "Dying";
            }
            else if (character->getProneState() == PS_PLAYING_DEAD)
            {
                *conditionOut = "Playing dead";
            }
            else if (character->isLiterallyUnconciousNotPretending())
            {
                *conditionOut = "Unconscious";
            }
            else if (character->inSomething == IN_PRISON)
            {
                *conditionOut = "Imprisoned";
            }
            else if (character->isBeingCarried())
            {
                *conditionOut = "Carried";
            }
            else if (character->isKidnapped())
            {
                *conditionOut = "Kidnapped";
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool BuildMemberSnapshot(const hand& member, MemberSnapshot* snapshotOut)
    {
        if (snapshotOut == NULL)
        {
            return false;
        }

        MemberSnapshot result;
        result.handle = member;
        CaptureHandleIdentity(member, &result.identity);
        Character* character = NULL;
        if (!TryResolveCharacter(member, &character))
        {
            result.name = "Unavailable member";
            result.condition = "Unavailable";
            *snapshotOut = result;
            return false;
        }

        result.loaded = true;
        if (!TryGetCharacterName(character, &result.name) || result.name.empty())
        {
            result.name = "Unnamed squad member";
        }
        TryGetBlockingCondition(character, &result.condition);
        BuildTopSkills(character, &result.skills);

        int count = 0;
        if (!TryGetJobsEnabled(character, &result.jobsEnabled) ||
            !TryGetPermajobCount(character, &count))
        {
            *snapshotOut = result;
            return false;
        }
        result.queueAvailable = true;
        if (count > MAX_SAFE_JOB_ROWS)
        {
            count = MAX_SAFE_JOB_ROWS;
            result.truncated = true;
            result.queueAvailable = false;
            result.condition = "Queue exceeds safety limit (read-only)";
        }

        result.jobs.reserve(static_cast<size_t>(count));
        for (int slot = 0; slot < count; ++slot)
        {
            JobRowSnapshot row;
            hand target;
            if (!TryGetPermajob(character, slot, result.jobsEnabled, &row, &target))
            {
                result.queueAvailable = false;
                result.jobs.clear();
                *snapshotOut = result;
                return false;
            }
            if (row.hasTarget)
            {
                RootObjectBase* targetObject = NULL;
                row.targetResolvable = TryResolveTargetObject(
                    target, &targetObject);
                if (row.targetResolvable &&
                    TryReadRootObjectName(targetObject, &row.targetLabel))
                {
                    row.targetAvailable = true;
                }
                else if (row.targetResolvable)
                {
                    // The target handle is live even if Kenshi has no
                    // display name for it.  Do not classify that as an
                    // invalid target.
                    row.targetLabel = "Unnamed target";
                }
                else
                {
                    row.targetLabel = "Target unavailable";
                }
            }
            else if (row.fixedTarget)
            {
                row.targetLabel = "Target unavailable";
            }
            result.jobs.push_back(row);
        }

        *snapshotOut = result;
        return true;
    }

    bool SameJob(const JobRowSnapshot& left, const JobRowSnapshot& right)
    {
        if (left.taskToken != 0 || right.taskToken != 0)
        {
            return left.taskToken == right.taskToken &&
                left.taskType == right.taskType &&
                SameHandleIdentity(left.target, right.target);
        }
        return left.taskType == right.taskType &&
            SameHandleIdentity(left.target, right.target) &&
            left.jobLabel == right.jobLabel;
    }

    bool SameQueue(
        const std::vector<JobRowSnapshot>& left,
        const std::vector<JobRowSnapshot>& right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.size(); ++index)
        {
            if (!SameJob(left[index], right[index]))
            {
                return false;
            }
            if (left[index].jobLabel != right[index].jobLabel ||
                left[index].targetLabel != right[index].targetLabel ||
                left[index].associatedTaskType !=
                    right[index].associatedTaskType ||
                left[index].fixedTarget != right[index].fixedTarget ||
                left[index].hasTarget != right[index].hasTarget ||
                left[index].targetAvailable != right[index].targetAvailable ||
                left[index].targetResolvable !=
                    right[index].targetResolvable)
            {
                return false;
            }
        }
        return true;
    }

    bool SameSkills(
        const std::vector<SkillValue>& left,
        const std::vector<SkillValue>& right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.size(); ++index)
        {
            if (left[index].stat != right[index].stat ||
                left[index].value != right[index].value ||
                left[index].name != right[index].name)
            {
                return false;
            }
        }
        return true;
    }

    bool SameMemberSnapshot(const MemberSnapshot& left, const MemberSnapshot& right)
    {
        return SameHandleIdentity(left.identity, right.identity) &&
            left.name == right.name &&
            left.condition == right.condition &&
            left.loaded == right.loaded &&
            left.queueAvailable == right.queueAvailable &&
            left.jobsEnabled == right.jobsEnabled &&
            left.truncated == right.truncated &&
            SameSkills(left.skills, right.skills) &&
            SameQueue(left.jobs, right.jobs);
    }

    int FindMemberSnapshotIndex(
        const SquadSnapshot& squad,
        const HandleIdentity& identity)
    {
        for (size_t index = 0; index < squad.members.size(); ++index)
        {
            if (SameHandleIdentity(squad.members[index].identity, identity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    int FindSquadSnapshotIndex(
        const std::vector<SquadSnapshot>& squads,
        const HandleIdentity& identity)
    {
        for (size_t index = 0; index < squads.size(); ++index)
        {
            if (SameHandleIdentity(squads[index].identity, identity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    int FindAllSquadSnapshotIndex(const HandleIdentity& identity)
    {
        return FindSquadSnapshotIndex(g_allSquads.squads, identity);
    }

    bool TryFindAllSquadMemberIndices(
        const HandleIdentity& squadIdentity,
        const HandleIdentity& memberIdentity,
        int* squadIndexOut,
        int* memberIndexOut)
    {
        if (squadIndexOut == NULL || memberIndexOut == NULL)
        {
            return false;
        }
        *squadIndexOut = -1;
        *memberIndexOut = -1;

        const int squadIndex =
            FindAllSquadSnapshotIndex(squadIdentity);
        if (squadIndex < 0)
        {
            return false;
        }
        const int memberIndex = FindMemberSnapshotIndex(
            g_allSquads.squads[squadIndex], memberIdentity);
        if (memberIndex < 0)
        {
            return false;
        }

        *squadIndexOut = squadIndex;
        *memberIndexOut = memberIndex;
        return true;
    }

    bool TryCopyAllSquadMemberSnapshot(
        const HandleIdentity& squadIdentity,
        const HandleIdentity& memberIdentity,
        MemberSnapshot* memberOut)
    {
        if (memberOut == NULL)
        {
            return false;
        }
        int squadIndex = -1;
        int memberIndex = -1;
        if (!TryFindAllSquadMemberIndices(
                squadIdentity, memberIdentity,
                &squadIndex, &memberIndex))
        {
            return false;
        }
        *memberOut =
            g_allSquads.squads[squadIndex].members[memberIndex];
        return true;
    }

    int FindSquadCollapseStateIndex(const HandleIdentity& identity)
    {
        for (size_t index = 0;
             index < g_squadCollapseStates.size();
             ++index)
        {
            if (SameHandleIdentity(
                    g_squadCollapseStates[index].identity, identity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool IsSquadCollapsed(const HandleIdentity& identity)
    {
        const int index = FindSquadCollapseStateIndex(identity);
        // A squad has no state until the player first opens or closes it.
        // Treat that missing state as collapsed so a large roster does not
        // create every member and job widget when the manager opens.
        return index < 0 || g_squadCollapseStates[index].collapsed;
    }

    void SetSquadCollapsed(
        const HandleIdentity& identity,
        bool collapsed)
    {
        if (!identity.valid)
        {
            return;
        }
        int index = FindSquadCollapseStateIndex(identity);
        if (index < 0)
        {
            if (g_squadCollapseStates.size() >= 256)
            {
                return;
            }
            g_squadCollapseStates.push_back(SquadCollapseState());
            index = static_cast<int>(g_squadCollapseStates.size() - 1);
            g_squadCollapseStates[index].identity = identity;
        }
        g_squadCollapseStates[index].collapsed = collapsed;
        // This is the ordinary/manual setter. Any direct header choice takes
        // ownership of the state away from native-TAB auto expansion.
        g_squadCollapseStates[index].autoExpandedByNativeTab = false;
    }

    void SetSquadCollapsedForNativeTab(
        const HandleIdentity& identity,
        bool collapsed,
        bool autoExpanded)
    {
        if (!identity.valid)
        {
            return;
        }
        int index = FindSquadCollapseStateIndex(identity);
        if (index < 0)
        {
            if (g_squadCollapseStates.size() >= 256)
            {
                return;
            }
            g_squadCollapseStates.push_back(SquadCollapseState());
            index = static_cast<int>(g_squadCollapseStates.size() - 1);
            g_squadCollapseStates[index].identity = identity;
        }
        g_squadCollapseStates[index].collapsed = collapsed;
        g_squadCollapseStates[index].autoExpandedByNativeTab =
            autoExpanded;
    }

    int FindJobSlot(const MemberSnapshot& member, const JobRowSnapshot& job)
    {
        for (size_t slot = 0; slot < member.jobs.size(); ++slot)
        {
            if (SameJob(member.jobs[slot], job))
            {
                return static_cast<int>(slot);
            }
        }
        return -1;
    }

    int FindSquadCache(const HandleIdentity& identity)
    {
        for (size_t index = 0; index < g_squadCaches.size(); ++index)
        {
            if (SameHandleIdentity(g_squadCaches[index].identity, identity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    int FindMemberInList(
        const std::vector<MemberSnapshot>& members,
        const HandleIdentity& identity)
    {
        for (size_t index = 0; index < members.size(); ++index)
        {
            if (SameHandleIdentity(members[index].identity, identity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    // Active-platoon member lists are capped at 256 entries when copied into
    // value-only snapshots. Most refreshes preserve vanilla member order, so
    // checking the previous slot first avoids a linear search for every row.
    // Keep the fallback bounded as a guard against a malformed or unexpectedly
    // large cached list.
    int FindMemberInListAtOrFallback(
        const std::vector<MemberSnapshot>& members,
        const HandleIdentity& identity,
        size_t preferredIndex)
    {
        if (preferredIndex < members.size() &&
            SameHandleIdentity(members[preferredIndex].identity, identity))
        {
            return static_cast<int>(preferredIndex);
        }

        const size_t scanCount = members.size() > 256 ?
            256 : members.size();
        for (size_t index = 0; index < scanCount; ++index)
        {
            if (SameHandleIdentity(members[index].identity, identity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool TryGetCachedMember(
        const HandleIdentity& squadIdentity,
        const HandleIdentity& memberIdentity,
        MemberSnapshot* memberOut)
    {
        if (memberOut == NULL)
        {
            return false;
        }
        if (SameHandleIdentity(g_squad.identity, squadIdentity))
        {
            const int currentIndex = FindMemberInList(
                g_squad.members,
                memberIdentity);
            if (currentIndex >= 0 &&
                g_squad.members[currentIndex].loaded &&
                g_squad.members[currentIndex].queueAvailable)
            {
                *memberOut = g_squad.members[currentIndex];
                return true;
            }
        }

        const int cacheIndex = FindSquadCache(squadIdentity);
        if (cacheIndex >= 0)
        {
            const int memberIndex = FindMemberInList(
                g_squadCaches[cacheIndex].members,
                memberIdentity);
            if (memberIndex >= 0)
            {
                *memberOut = g_squadCaches[cacheIndex].members[memberIndex];
                return true;
            }
        }
        return false;
    }

    void UpdateSquadCacheMembers(
        SquadCache* cache,
        const std::vector<MemberSnapshot>& currentMembers)
    {
        if (cache == NULL)
        {
            return;
        }

        const bool sameMemberOrder =
            cache->members.size() == currentMembers.size();
        bool canUpdateInPlace = sameMemberOrder;
        if (canUpdateInPlace)
        {
            for (size_t index = 0; index < currentMembers.size(); ++index)
            {
                if (!SameHandleIdentity(
                        cache->members[index].identity,
                        currentMembers[index].identity))
                {
                    canUpdateInPlace = false;
                    break;
                }
            }
        }

        if (canUpdateInPlace)
        {
            // Unavailable rows intentionally retain their last complete
            // value-only copy. For available rows, assign only when the
            // snapshot changed; this avoids copying every job and skill
            // vector on repeated explicit refreshes.
            for (size_t index = 0; index < currentMembers.size(); ++index)
            {
                const MemberSnapshot& current = currentMembers[index];
                if (current.loaded && current.queueAvailable &&
                    !SameMemberSnapshot(cache->members[index], current))
                {
                    cache->members[index] = current;
                }
            }
            return;
        }

        // A roster insertion or reorder is rare. Swap the old vector into a
        // temporary value-only buffer instead of copying every nested job and
        // skill vector before rebuilding it.
        std::vector<MemberSnapshot> previous;
        previous.swap(cache->members);
        cache->members.reserve(currentMembers.size());
        for (size_t index = 0; index < currentMembers.size(); ++index)
        {
            const MemberSnapshot& current = currentMembers[index];
            if (current.loaded && current.queueAvailable)
            {
                cache->members.push_back(current);
                continue;
            }
            const int previousIndex = FindMemberInListAtOrFallback(
                previous, current.identity, index);
            if (previousIndex >= 0)
            {
                cache->members.push_back(previous[previousIndex]);
            }
            else
            {
                cache->members.push_back(current);
            }
        }
    }

    void StoreCurrentSquadCache()
    {
        if (!g_squad.identity.valid || !g_squad.live)
        {
            return;
        }
        int cacheIndex = FindSquadCache(g_squad.identity);
        if (cacheIndex < 0)
        {
            if (g_squadCaches.size() >= 256)
            {
                return;
            }
            g_squadCaches.push_back(SquadCache());
            cacheIndex = static_cast<int>(g_squadCaches.size() - 1);
        }
        SquadCache& cache = g_squadCaches[cacheIndex];
        cache.identity = g_squad.identity;
        if (cache.name != g_squad.name)
        {
            cache.name = g_squad.name;
        }
        UpdateSquadCacheMembers(&cache, g_squad.members);
        cache.horizontalOffset = g_horizontalOffset;
        cache.verticalOffset = g_verticalOffset;
    }

    // Cache the last complete value-only member rows for a non-current squad.
    // Scroll offsets are properties of the selected legacy view and are left
    // unchanged here.
    void StoreSquadSnapshotCache(const SquadSnapshot& squad)
    {
        if (!squad.identity.valid || !squad.live)
        {
            return;
        }
        int cacheIndex = FindSquadCache(squad.identity);
        if (cacheIndex < 0)
        {
            if (g_squadCaches.size() >= 256)
            {
                return;
            }
            g_squadCaches.push_back(SquadCache());
            cacheIndex = static_cast<int>(g_squadCaches.size() - 1);
        }

        SquadCache& cache = g_squadCaches[cacheIndex];
        cache.identity = squad.identity;
        if (cache.name != squad.name)
        {
            cache.name = squad.name;
        }
        UpdateSquadCacheMembers(&cache, squad.members);
    }

    void StoreCurrentSquadScrollOffsets()
    {
        const int cacheIndex = FindSquadCache(g_squad.identity);
        if (cacheIndex >= 0)
        {
            g_squadCaches[cacheIndex].horizontalOffset = g_horizontalOffset;
            g_squadCaches[cacheIndex].verticalOffset = g_verticalOffset;
        }
    }

    void PublishSquadSnapshot(
        SquadSnapshot* destination,
        SquadSnapshot* source)
    {
        if (destination == NULL || source == NULL)
        {
            return;
        }
        destination->identity = source->identity;
        destination->handle = source->handle;
        destination->name.swap(source->name);
        destination->live = source->live;
        destination->unavailable = source->unavailable;
        destination->incomplete = source->incomplete;
        destination->members.swap(source->members);
    }

    bool BuildCurrentSquadSnapshot(SquadSnapshot* snapshotOut)
    {
        if (snapshotOut == NULL)
        {
            return false;
        }

        SquadSnapshot next;
        hand squadHandle;
        RootObjectContainer* active = NULL;
        if (!TryGetCurrentSquad(g_playerInterface, &squadHandle, &next.name, &active))
        {
            return false;
        }
        CaptureHandleIdentity(squadHandle, &next.identity);
        next.handle = squadHandle;

        std::vector<hand> handles;
        bool memberListIncomplete = false;
        if (active == NULL || !TryGetOrderedMemberHandles(
                active, &handles, &memberListIncomplete))
        {
            const int cacheIndex = FindSquadCache(next.identity);
            if (cacheIndex >= 0)
            {
                next.members = g_squadCaches[cacheIndex].members;
                for (size_t index = 0; index < next.members.size(); ++index)
                {
                    next.members[index].loaded = false;
                    next.members[index].queueAvailable = false;
                    next.members[index].condition = "Cached / unloaded";
                    ++next.members[index].revision;
                }
            }
            next.live = false;
            next.unavailable = true;
            PublishSquadSnapshot(snapshotOut, &next);
            return true;
        }

        next.live = true;
        next.incomplete = memberListIncomplete;
        next.members.reserve(handles.size());
        for (size_t index = 0; index < handles.size(); ++index)
        {
            MemberSnapshot member;
            if (!BuildMemberSnapshot(handles[index], &member))
            {
                HandleIdentity memberIdentity;
                CaptureHandleIdentity(handles[index], &memberIdentity);
                MemberSnapshot cached;
                if (TryGetCachedMember(next.identity, memberIdentity, &cached))
                {
                    member = cached;
                    member.handle = handles[index];
                    member.identity = memberIdentity;
                    member.loaded = false;
                    member.queueAvailable = false;
                    member.condition = "Cached / unavailable";
                    ++member.revision;
                }
                else
                {
                    member.handle = handles[index];
                    member.identity = memberIdentity;
                    member.loaded = false;
                    member.queueAvailable = false;
                    member.jobs.clear();
                    member.condition = "Unavailable";
                }
            }

            if (SameHandleIdentity(g_squad.identity, next.identity))
            {
                const int oldIndex = FindMemberInListAtOrFallback(
                    g_squad.members, member.identity, index);
                if (oldIndex >= 0)
                {
                    if (SameMemberSnapshot(g_squad.members[oldIndex], member))
                    {
                        member.revision = g_squad.members[oldIndex].revision;
                    }
                    else
                    {
                        member.revision = g_squad.members[oldIndex].revision + 1;
                    }
                }
            }
            next.members.push_back(member);
        }

        PublishSquadSnapshot(snapshotOut, &next);
        return true;
    }

    bool SameSquadSnapshot(
        const SquadSnapshot& left,
        const SquadSnapshot& right)
    {
        if (!SameHandleIdentity(left.identity, right.identity) ||
            left.name != right.name ||
            left.live != right.live ||
            left.unavailable != right.unavailable ||
            left.incomplete != right.incomplete ||
            left.members.size() != right.members.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.members.size(); ++index)
        {
            // BuildAllActiveSquadsSnapshot assigns a new revision whenever a
            // value-only member row differs from its previous row. Reuse that
            // contract here instead of comparing every skill and job string a
            // second time during publication.
            if (!SameHandleIdentity(
                    left.members[index].identity,
                    right.members[index].identity) ||
                left.members[index].revision !=
                    right.members[index].revision)
            {
                return false;
            }
        }
        return true;
    }

    bool SameAllSquadsSnapshot(
        const AllSquadsSnapshot& left,
        const AllSquadsSnapshot& right)
    {
        if (left.incomplete != right.incomplete ||
            left.squads.size() != right.squads.size())
        {
            return false;
        }
        for (size_t index = 0; index < left.squads.size(); ++index)
        {
            if (!SameSquadSnapshot(
                    left.squads[index], right.squads[index]))
            {
                return false;
            }
        }
        return true;
    }

    const SquadSnapshot* FindPreviousSquadSnapshot(
        const HandleIdentity& squadIdentity,
        size_t preferredIndex)
    {
        if (preferredIndex < g_allSquads.squads.size() &&
            SameHandleIdentity(
                g_allSquads.squads[preferredIndex].identity,
                squadIdentity))
        {
            return &g_allSquads.squads[preferredIndex];
        }

        const int squadIndex = FindSquadSnapshotIndex(
            g_allSquads.squads, squadIdentity);
        if (squadIndex >= 0)
        {
            return &g_allSquads.squads[squadIndex];
        }
        return NULL;
    }

    const MemberSnapshot* FindPreviousAllSquadMember(
        const SquadSnapshot* previousSquad,
        const SquadSnapshot* currentSquad,
        const HandleIdentity& memberIdentity,
        size_t preferredIndex)
    {
        if (previousSquad != NULL)
        {
            const int memberIndex = FindMemberInListAtOrFallback(
                previousSquad->members, memberIdentity, preferredIndex);
            if (memberIndex >= 0)
            {
                return &previousSquad->members[memberIndex];
            }
        }
        if (currentSquad != NULL)
        {
            const int memberIndex = FindMemberInListAtOrFallback(
                currentSquad->members, memberIdentity, preferredIndex);
            if (memberIndex >= 0)
            {
                return &currentSquad->members[memberIndex];
            }
        }
        return NULL;
    }

    // Materialize the value-only active-squad seeds into complete member and
    // permanent-job snapshots. Engine objects are reacquired only for each
    // bounded member read and are never stored in the result.
    bool BuildAllActiveSquadsSnapshot(AllSquadsSnapshot* snapshotOut)
    {
        if (snapshotOut == NULL)
        {
            return false;
        }

        std::vector<ActiveSquadValueSeed> seeds;
        bool rosterIncomplete = false;
        if (!BuildActiveSquadValueSeeds(&seeds, &rosterIncomplete))
        {
            return false;
        }

        AllSquadsSnapshot next;
        next.incomplete = rosterIncomplete;
        next.squads.reserve(seeds.size());
        for (size_t squadIndex = 0;
             squadIndex < seeds.size();
             ++squadIndex)
        {
            const ActiveSquadValueSeed& seed = seeds[squadIndex];
            SquadSnapshot squad;
            squad.identity = seed.identity;
            squad.handle = RestoreHandleIdentity(seed.identity);
            squad.name = seed.name;
            squad.live = true;
            squad.unavailable = false;
            squad.incomplete = seed.incomplete;
            squad.members.reserve(seed.members.size());

            const SquadSnapshot* previousSquad =
                FindPreviousSquadSnapshot(squad.identity, squadIndex);
            const SquadSnapshot* currentSquad =
                SameHandleIdentity(g_squad.identity, squad.identity) ?
                    &g_squad : NULL;

            size_t unavailableMemberCount = 0;
            for (size_t memberIndex = 0;
                 memberIndex < seed.members.size();
                 ++memberIndex)
            {
                const HandleIdentity& memberIdentity =
                    seed.members[memberIndex];
                const hand memberHandle =
                    RestoreHandleIdentity(memberIdentity);
                MemberSnapshot member;
                if (!BuildMemberSnapshot(memberHandle, &member))
                {
                    MemberSnapshot cached;
                    if (TryGetCachedMember(
                            squad.identity, memberIdentity, &cached))
                    {
                        member = cached;
                        member.handle = memberHandle;
                        member.identity = memberIdentity;
                        member.loaded = false;
                        member.queueAvailable = false;
                        member.condition = "Cached / unavailable";
                    }
                    else
                    {
                        member.handle = memberHandle;
                        member.identity = memberIdentity;
                        member.loaded = false;
                        member.queueAvailable = false;
                        member.jobs.clear();
                        member.condition = "Unavailable";
                    }
                    ++unavailableMemberCount;
                }

                const MemberSnapshot* previous =
                    FindPreviousAllSquadMember(
                        previousSquad, currentSquad,
                        member.identity, memberIndex);
                if (previous != NULL)
                {
                    member.revision = SameMemberSnapshot(
                        *previous, member) ?
                        previous->revision : previous->revision + 1;
                }
                squad.members.push_back(member);
            }
            squad.unavailable = !squad.members.empty() &&
                unavailableMemberCount == squad.members.size();
            next.squads.push_back(squad);
        }

        snapshotOut->incomplete = next.incomplete;
        snapshotOut->revision = next.revision;
        snapshotOut->squads.swap(next.squads);
        return true;
    }

    void PruneSquadCollapseStates(
        const std::vector<SquadSnapshot>& squads)
    {
        std::vector<SquadCollapseState> retained;
        retained.reserve(g_squadCollapseStates.size());
        for (size_t index = 0;
             index < g_squadCollapseStates.size();
             ++index)
        {
            if (FindSquadSnapshotIndex(
                    squads,
                    g_squadCollapseStates[index].identity) >= 0)
            {
                retained.push_back(g_squadCollapseStates[index]);
            }
        }
        g_squadCollapseStates.swap(retained);
    }

    void MarkAllSquadsSnapshotUnavailable(
        AllSquadsSnapshot* snapshot)
    {
        if (snapshot == NULL)
        {
            return;
        }
        snapshot->incomplete = true;
        for (size_t squadIndex = 0;
             squadIndex < snapshot->squads.size();
             ++squadIndex)
        {
            SquadSnapshot& squad = snapshot->squads[squadIndex];
            squad.live = false;
            squad.unavailable = true;
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size();
                 ++memberIndex)
            {
                MemberSnapshot& member = squad.members[memberIndex];
                member.loaded = false;
                member.queueAvailable = false;
                member.condition = "Cached / unavailable";
                ++member.revision;
            }
        }
    }

    // Publish one coherent board. On a failed engine read, retain the last
    // value-only board but mark every cached row read-only until a later
    // refresh succeeds.
    bool RefreshAllActiveSquadsSnapshot()
    {
        AllSquadsSnapshot next;
        if (!BuildAllActiveSquadsSnapshot(&next))
        {
            MarkAllSquadsSnapshotUnavailable(&g_allSquads);
            ++g_allSquads.revision;
            return false;
        }

        const bool unchanged = SameAllSquadsSnapshot(
            g_allSquads, next);
        next.revision = unchanged ?
            g_allSquads.revision : g_allSquads.revision + 1;
        if (!unchanged)
        {
            for (size_t index = 0; index < next.squads.size(); ++index)
            {
                StoreSquadSnapshotCache(next.squads[index]);
            }
        }
        PruneSquadCollapseStates(next.squads);
        g_allSquads.incomplete = next.incomplete;
        g_allSquads.revision = next.revision;
        g_allSquads.squads.swap(next.squads);
        return true;
    }

    bool TryRefreshMemberByIdentity(
        const HandleIdentity& identity,
        MemberSnapshot* refreshedOut)
    {
        if (refreshedOut == NULL)
        {
            return false;
        }

        SquadSnapshot liveSquad;
        if (!BuildCurrentSquadSnapshot(&liveSquad) || !liveSquad.live ||
            !SameHandleIdentity(liveSquad.identity, g_squad.identity))
        {
            return false;
        }

        int liveIndex = -1;
        for (size_t index = 0; index < liveSquad.members.size(); ++index)
        {
            if (SameHandleIdentity(liveSquad.members[index].identity, identity))
            {
                liveIndex = static_cast<int>(index);
                break;
            }
        }
        if (liveIndex < 0)
        {
            return false;
        }

        *refreshedOut = liveSquad.members[liveIndex];
        if (!refreshedOut->loaded || !refreshedOut->queueAvailable)
        {
            return false;
        }
        return true;
    }

    bool TrySetJobsEnabled(const hand& member, bool enabled)
    {
        __try
        {
            if (!member || !member.isValid())
            {
                return false;
            }
            Character* character = member.getCharacter();
            if (character == NULL)
            {
                return false;
            }
            OrdersReceiver* receiver = character->getOrdersReciever();
            if (receiver == NULL)
            {
                return false;
            }
            receiver->setJobsEnabled(enabled);
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryMovePermajob(const hand& member, int from, int to)
    {
        if (from < 0 || to < 0)
        {
            return false;
        }
        __try
        {
            if (!member || !member.isValid())
            {
                return false;
            }
            Character* character = member.getCharacter();
            if (character == NULL)
            {
                return false;
            }
            character->movePermajob(from, to);
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryRemovePermajob(const hand& member, int slot)
    {
        if (slot < 0)
        {
            return false;
        }
        __try
        {
            if (!member || !member.isValid())
            {
                return false;
            }
            Character* character = member.getCharacter();
            if (character == NULL)
            {
                return false;
            }
            character->removePermajob(slot);
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // Cleanup needs to prove the exact vector erase before Kenshi is allowed
    // to reconsider the current AI action. Keep this raw leaf separate from
    // the established transfer helper above.
    bool TryRemovePermajobRaw(const hand& member, int slot)
    {
        if (slot < 0)
        {
            return false;
        }
        __try
        {
            if (!member || !member.isValid())
            {
                return false;
            }
            Character* character = member.getCharacter();
            if (character == NULL)
            {
                return false;
            }
            character->removePermajob(slot);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryRethinkCurrentAIAction(const hand& member)
    {
        __try
        {
            if (!member || !member.isValid())
            {
                return false;
            }
            Character* character = member.getCharacter();
            if (character == NULL)
            {
                return false;
            }
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TryClearPermajobs(const hand& member)
    {
        __try
        {
            if (!member || !member.isValid())
            {
                return false;
            }
            Character* character = member.getCharacter();
            if (character == NULL)
            {
                return false;
            }
            character->clearPermajobs();
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool NormalizeStationTaskScalars(
        BuildingClassType classType,
        BuildingFunction function,
        TaskType nativeTask,
        TaskType* taskOut,
        bool* automaticBundleOut)
    {
        if (taskOut == NULL || automaticBundleOut == NULL)
        {
            return false;
        }
        *taskOut = NULL_TASK;
        *automaticBundleOut = false;

        const bool storage =
            classType == BCTYPE_STORAGE ||
            function == BF_RESOURCE_STORAGE ||
            function == BF_GENERAL_STORAGE;
        const bool training = function == BF_TRAINING;
        const bool turret =
            classType == BCTYPE_TURRET || function == BF_TURRET;

        // These task types are themselves the stable production contract.
        // Accept them for modded usable buildings even when the class or
        // special-function metadata is generic.
        if (nativeTask == OPERATE_MACHINERY)
        {
            *taskOut = OPERATE_MACHINERY;
            return true;
        }
        if (nativeTask == OPERATE_AUTOMATIC_MACHINERY)
        {
            *taskOut = OPERATE_AUTOMATIC_MACHINERY;
            *automaticBundleOut = true;
            return true;
        }
        if (storage &&
            (nativeTask == LOOT_TARGET || nativeTask == OPERATE_STORAGE))
        {
            // StorageBuilding::getDefaultTask can report LOOT_TARGET even
            // though Kenshi's permanent hauling job is OPERATE_STORAGE.
            *taskOut = OPERATE_STORAGE;
            return true;
        }
        if (training && nativeTask == USE_TRAINING_DUMMY)
        {
            *taskOut = USE_TRAINING_DUMMY;
            return true;
        }
        if (turret &&
            (nativeTask == MAN_A_TURRET || nativeTask == USE_TURRET))
        {
            *taskOut = nativeTask;
            return true;
        }

        // Do not pass an unknown building default through to addJob. A mod
        // can expose a valid task which is not safe as a permanent station
        // assignment.
        return false;
    }

    bool StationActionHandleMatchesIdentity(
        const hand& value,
        const HandleIdentity& identity)
    {
        return identity.valid &&
            value.type == identity.type &&
            value.container == identity.container &&
            value.containerSerial == identity.containerSerial &&
            value.index == identity.index && value.serial == identity.serial;
    }

    bool IsStationActionPlayerManaged(
        Building* building,
        bool allowAssignedNaturalException)
    {
        if (building == NULL)
        {
            return false;
        }
        if (building->isThePlayer())
        {
            return true;
        }
        return allowAssignedNaturalException &&
            building->getSpecialFunction() == BF_MINE_NATURAL;
    }

    __declspec(noinline) bool TryValidateStationActionTargetIdentity(
        const HandleIdentity& station)
    {
        __try
        {
            if (!station.valid || station.type != BUILDING)
            {
                return false;
            }
            const hand stationHandle = RestoreHandleIdentity(station);
            if (!stationHandle || !stationHandle.isValid())
            {
                return false;
            }
            Building* building = stationHandle.getBuilding();
            if (building == NULL ||
                !StationActionHandleMatchesIdentity(
                    building->getHandle(), station) ||
                !IsStationActionPlayerManaged(building, true))
            {
                return false;
            }
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    enum StationAddJobResult
    {
        STATION_ADD_INVALID_TARGET,
        STATION_ADD_UNSUPPORTED_TASK,
        STATION_ADD_QUEUE_FULL,
        STATION_ADD_CALLED,
        STATION_ADD_FAULTED
    };

    __declspec(noinline) StationAddJobResult
    TryResolveAndAddStationJobOnce(
        const HandleIdentity& member,
        const HandleIdentity& station,
        size_t queueRowCount,
        bool allowAssignedNaturalException,
        TaskType* normalizedTaskOut,
        bool* automaticBundleOut)
    {
        if (normalizedTaskOut == NULL || automaticBundleOut == NULL)
        {
            return STATION_ADD_INVALID_TARGET;
        }
        *normalizedTaskOut = NULL_TASK;
        *automaticBundleOut = false;

        bool addWasCalled = false;
        __try
        {
            if (!member.valid || member.type != CHARACTER ||
                !station.valid || station.type != BUILDING)
            {
                return STATION_ADD_INVALID_TARGET;
            }
            const hand memberHandle = RestoreHandleIdentity(member);
            const hand stationHandle = RestoreHandleIdentity(station);
            if (!memberHandle || !memberHandle.isValid() ||
                !stationHandle || !stationHandle.isValid())
            {
                return STATION_ADD_INVALID_TARGET;
            }
            Character* character = memberHandle.getCharacter();
            Building* building = stationHandle.getBuilding();
            if (character == NULL || building == NULL ||
                !character->isPlayerCharacter() ||
                !StationActionHandleMatchesIdentity(
                    character->getHandle(), member) ||
                !StationActionHandleMatchesIdentity(
                    building->getHandle(), station) ||
                !IsStationActionPlayerManaged(
                    building, allowAssignedNaturalException))
            {
                return STATION_ADD_INVALID_TARGET;
            }
            Faction* faction = character->getFaction();
            if (faction == NULL || !faction->isThePlayer())
            {
                return STATION_ADD_INVALID_TARGET;
            }
            // Keep destroyed stations readable so existing exact jobs can be
            // cleaned up, but never send a new job to one.
            if (building->isDestroyed())
            {
                return STATION_ADD_INVALID_TARGET;
            }

            const BuildingClassType classType = building->getBuildingClass();
            const BuildingFunction function = building->getSpecialFunction();
            const TaskType nativeTask = building->getDefaultTask();
            if (!NormalizeStationTaskScalars(
                    classType, function, nativeTask,
                    normalizedTaskOut, automaticBundleOut))
            {
                return STATION_ADD_UNSUPPORTED_TASK;
            }
            const size_t maximumAddedRows = *automaticBundleOut ? 2 : 1;
            if (queueRowCount + maximumAddedRows >
                static_cast<size_t>(MAX_SAFE_JOB_ROWS))
            {
                return STATION_ADD_QUEUE_FULL;
            }

            const Ogre::Vector3 position = building->getPosition();
            // Call exactly once. Kenshi can append OPERATE_STORAGE as the
            // native secondary job for OPERATE_AUTOMATIC_MACHINERY.
            addWasCalled = true;
            character->addJob(
                *normalizedTaskOut, building, true, true, position);
            character->reThinkCurrentAIAction();
            return STATION_ADD_CALLED;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // The caller must reacquire and inspect the whole queue when the
            // fault happened after addJob was entered; the mutation can have
            // completed before a later engine operation faulted.
            return addWasCalled ?
                STATION_ADD_FAULTED : STATION_ADD_INVALID_TARGET;
        }
    }

    void TryNotifyVanillaSelectionUI()
    {
        if (g_playerInterface == NULL)
        {
            return;
        }
        __try
        {
            g_playerInterface->selectedObjectsChangedThisFrame = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    bool IsDeadSquadName(const std::string& name)
    {
        return name == "__DEAD__";
    }

    bool TryCallOriginalCycleSquad()
    {
        if (g_playerInterface == NULL ||
            g_playerInterfaceCycleSquadOriginal == NULL)
        {
            return false;
        }
        __try
        {
            // Bypass the observer hook only while continuing past Kenshi's
            // internal __DEAD__ holding squad. Ordinary TAB cycling enters
            // through the hook and remains owned by Kenshi.
            g_playerInterfaceCycleSquadOriginal(g_playerInterface);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TrySkipDeadCurrentSquad()
    {
        hand currentSquad;
        std::string currentName;
        RootObjectContainer* currentActive = NULL;
        if (!TryGetCurrentSquad(
                g_playerInterface,
                &currentSquad,
                &currentName,
                &currentActive))
        {
            // Let the normal snapshot fallback handle a temporarily
            // unavailable current squad.
            return true;
        }
        if (!IsDeadSquadName(currentName))
        {
            return true;
        }

        HandleIdentity startingDeadSquad;
        CaptureHandleIdentity(currentSquad, &startingDeadSquad);

        // __DEAD__ is Kenshi's internal holding platoon for recently dead
        // characters. It is not a player-manageable squad. Stop if cycling
        // returns to the same handle, and retain a small hard bound in case
        // corrupt handles make cycle detection unreliable.
        const int maximumAttempts = 16;
        for (int attempt = 0; attempt < maximumAttempts; ++attempt)
        {
            if (!TryCallOriginalCycleSquad() ||
                !TryGetCurrentSquad(
                    g_playerInterface,
                    &currentSquad,
                    &currentName,
                    &currentActive))
            {
                return false;
            }
            if (!IsDeadSquadName(currentName))
            {
                return true;
            }

            HandleIdentity cycledSquad;
            CaptureHandleIdentity(currentSquad, &cycledSquad);
            if (SameHandleIdentity(cycledSquad, startingDeadSquad))
            {
                return false;
            }
        }
        return false;
    }

    bool EnsureSettingsPath()
    {
        if (!g_settingsPath.empty())
        {
            return true;
        }

        char base[MAX_PATH] = { 0 };
        DWORD length = GetEnvironmentVariableA("LOCALAPPDATA", base, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
        {
            length = GetEnvironmentVariableA("APPDATA", base, MAX_PATH);
        }
        if (length == 0 || length >= MAX_PATH)
        {
            std::strcpy(base, ".");
        }

        std::string directory(base);
        directory += "\\KenshiJobManagement";
        const bool directoryReady =
            CreateDirectoryA(directory.c_str(), NULL) != FALSE ||
            GetLastError() == ERROR_ALREADY_EXISTS;
        g_settingsPath = directory + "\\settings.ini";
        return directoryReady;
    }

    bool TryCaptureAndPauseGame()
    {
        if (ou == NULL)
        {
            return false;
        }
        __try
        {
            g_wasPaused = ou->isPaused();
            g_previousSpeed = ou->getFrameSpeedMultiplier();
            ou->userPause(true);
            g_managerPausedSpeed = ou->getFrameSpeedMultiplier();
            g_pauseCaptured = true;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_pauseCaptured = false;
            return false;
        }
    }

    bool IsGamePausedSafely()
    {
        if (ou == NULL)
        {
            return true;
        }
        __try
        {
            return ou->isPaused();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return true;
        }
    }

    bool TryReadGamePauseAndSpeed(bool* pausedOut, float* speedOut)
    {
        if (ou == NULL || pausedOut == NULL || speedOut == NULL)
        {
            return false;
        }
        __try
        {
            *pausedOut = ou->isPaused();
            *speedOut = ou->getFrameSpeedMultiplier();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void RestoreGamePauseState(bool keepUserResume)
    {
        if (!g_pauseCaptured || ou == NULL || keepUserResume)
        {
            g_pauseCaptured = false;
            return;
        }
        __try
        {
            ou->userPause(g_wasPaused);
            if (g_wasPaused)
            {
                ou->setFrameSpeedMultiplier(g_previousSpeed);
            }
            else
            {
                ou->setGameSpeed(g_previousSpeed, false);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while restoring game speed.");
        }
        g_pauseCaptured = false;
    }
