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

    void SetStatus(const std::string& message)
    {
        if (g_statusText != NULL)
        {
            g_statusText->setCaption(message.c_str());
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

    bool TryGetOrderedMemberHandles(
        RootObjectContainer* active,
        std::vector<hand>* handlesOut)
    {
        if (active == NULL || handlesOut == NULL)
        {
            return false;
        }

        handlesOut->clear();
        __try
        {
            int count = active->getNumThings();
            if (count < 0 || count > 256)
            {
                return false;
            }

            handlesOut->reserve(static_cast<size_t>(count));
            for (int index = 0; index < count; ++index)
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
                if (TryResolveTargetName(target, &row.targetLabel))
                {
                    row.targetAvailable = true;
                }
                else
                {
                    row.targetLabel = "Target unavailable";
                }
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
                left[index].hasTarget != right[index].hasTarget ||
                left[index].targetAvailable != right[index].targetAvailable)
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

    int FindMemberIndex(const HandleIdentity& identity)
    {
        for (size_t index = 0; index < g_squad.members.size(); ++index)
        {
            if (SameHandleIdentity(g_squad.members[index].identity, identity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
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

    void StoreCurrentSquadCache()
    {
        if (!g_squad.identity.valid || !g_squad.live)
        {
            return;
        }
        int cacheIndex = FindSquadCache(g_squad.identity);
        if (cacheIndex < 0)
        {
            g_squadCaches.push_back(SquadCache());
            cacheIndex = static_cast<int>(g_squadCaches.size() - 1);
        }
        SquadCache& cache = g_squadCaches[cacheIndex];
        const std::vector<MemberSnapshot> previous = cache.members;
        cache.identity = g_squad.identity;
        cache.name = g_squad.name;
        cache.members.clear();
        cache.members.reserve(g_squad.members.size());
        for (size_t index = 0; index < g_squad.members.size(); ++index)
        {
            const MemberSnapshot& current = g_squad.members[index];
            if (current.loaded && current.queueAvailable)
            {
                cache.members.push_back(current);
                continue;
            }
            const int previousIndex = FindMemberInList(previous, current.identity);
            if (previousIndex >= 0)
            {
                cache.members.push_back(previous[previousIndex]);
            }
            else
            {
                cache.members.push_back(current);
            }
        }
        cache.horizontalOffset = g_horizontalOffset;
        cache.verticalOffset = g_verticalOffset;
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
        if (active == NULL || !TryGetOrderedMemberHandles(active, &handles))
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
            *snapshotOut = next;
            return true;
        }

        next.live = true;
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
                const int oldIndex = FindMemberIndex(member.identity);
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

        *snapshotOut = next;
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
            // Bypass the hook. While the manager is open, its hook suppresses
            // Kenshi's TAB event so the edge-detected manager path is the one
            // and only owner of squad cycling.
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

    bool TryCycleCurrentSquad()
    {
        if (!TryCallOriginalCycleSquad())
        {
            return false;
        }
        return TrySkipDeadCurrentSquad();
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
