// SPDX-License-Identifier: GPL-3.0-only
    void ResetHandleIdentity(HandleIdentity* identity)
    {
        if (identity == NULL)
        {
            return;
        }

        identity->valid = false;
        identity->type = static_cast<itemType>(0);
        identity->container = 0;
        identity->containerSerial = 0;
        identity->index = 0;
        identity->serial = 0;
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

    bool TryResolveSelectedCharacter(
        PlayerInterface* player,
        HandleIdentity* handleOut,
        Character** characterOut)
    {
        if (handleOut != NULL)
        {
            ResetHandleIdentity(handleOut);
        }
        if (characterOut != NULL)
        {
            *characterOut = NULL;
        }
        if (player == NULL || handleOut == NULL || characterOut == NULL)
        {
            return false;
        }

        __try
        {
            const hand& selected = player->selectedCharacter;
            if (!selected || !selected.isValid())
            {
                return false;
            }

            Character* character = selected.getCharacter();
            if (character == NULL || !character->isPlayerCharacter())
            {
                return false;
            }

            handleOut->valid = true;
            handleOut->type = selected.type;
            handleOut->container = selected.container;
            handleOut->containerSerial = selected.containerSerial;
            handleOut->index = selected.index;
            handleOut->serial = selected.serial;
            *characterOut = character;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while resolving selected character.");
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
            *nameOut = character->displayName;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while reading character name.");
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
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while reading permanent-job count.");
            return false;
        }
    }

    bool TryGetPermajobRow(
        Character* character,
        int slot,
        TaskType* taskTypeOut,
        std::string* nameOut,
        const Tasker** taskDataOut)
    {
        if (character == NULL || slot < 0 || taskTypeOut == NULL ||
            nameOut == NULL || taskDataOut == NULL)
        {
            return false;
        }

        __try
        {
            const int count = character->getPermajobCount();
            if (slot >= count)
            {
                return false;
            }

            *taskTypeOut = character->getPermajob(slot);
            *nameOut = character->getPermajobName(slot);
            *taskDataOut = character->getPermajobData(slot);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while reading a permanent-job row.");
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
            ErrorLog("[KenshiJobManagement] Exception while reading Jobs state.");
            return false;
        }
    }

    bool TrySetJobsEnabled(Character* character, bool enabled)
    {
        if (character == NULL)
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

            receiver->setJobsEnabled(enabled);
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while changing Jobs state.");
            return false;
        }
    }

    bool TryMovePermajob(Character* character, int from, int to)
    {
        if (character == NULL || from < 0 || to < 0)
        {
            return false;
        }

        __try
        {
            character->movePermajob(from, to);
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while moving a permanent job.");
            return false;
        }
    }

    bool TryRemovePermajob(Character* character, int slot)
    {
        if (character == NULL || slot < 0)
        {
            return false;
        }

        __try
        {
            character->removePermajob(slot);
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while removing a permanent job.");
            return false;
        }
    }

    bool TryClearPermajobs(Character* character)
    {
        if (character == NULL)
        {
            return false;
        }

        __try
        {
            character->clearPermajobs();
            character->reThinkCurrentAIAction();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("[KenshiJobManagement] Exception while clearing permanent jobs.");
            return false;
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
            ErrorLog("[KenshiJobManagement] Exception while notifying the vanilla selection UI.");
        }
    }

    bool SnapshotEquals(
        const std::vector<JobRowSnapshot>& left,
        const std::vector<JobRowSnapshot>& right)
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (size_t index = 0; index < left.size(); ++index)
        {
            if (left[index].taskType != right[index].taskType ||
                left[index].name != right[index].name ||
                left[index].taskData != right[index].taskData)
            {
                return false;
            }
        }

        return true;
    }

    bool BuildSnapshot(
        Character* character,
        std::vector<JobRowSnapshot>* rowsOut,
        bool* truncatedOut)
    {
        if (rowsOut == NULL || truncatedOut == NULL)
        {
            return false;
        }

        rowsOut->clear();
        *truncatedOut = false;

        int count = 0;
        if (!TryGetPermajobCount(character, &count))
        {
            return false;
        }
        if (count < 0)
        {
            count = 0;
        }
        if (count > MAX_SAFE_JOB_ROWS)
        {
            count = MAX_SAFE_JOB_ROWS;
            *truncatedOut = true;
        }

        rowsOut->reserve(static_cast<size_t>(count));
        for (int slot = 0; slot < count; ++slot)
        {
            JobRowSnapshot row;
            row.taskType = static_cast<TaskType>(0);
            row.taskData = NULL;
            if (!TryGetPermajobRow(
                    character,
                    slot,
                    &row.taskType,
                    &row.name,
                    &row.taskData))
            {
                return false;
            }

            if (row.name.empty())
            {
                std::ostringstream fallback;
                fallback << "Unnamed job (task " << static_cast<int>(row.taskType) << ")";
                row.name = fallback.str();
            }

            rowsOut->push_back(row);
        }

        return true;
    }
