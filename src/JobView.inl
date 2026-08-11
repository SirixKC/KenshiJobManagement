// SPDX-License-Identifier: GPL-3.0-only
    void ResetClearConfirmation()
    {
        g_clearArmed = false;
        g_clearArmedTick = 0;
        if (g_clearButton != NULL)
        {
            g_clearButton->setCaption("Clear All");
        }
    }

    void UpdateControlState(bool hasCharacter)
    {
        const size_t selected = g_jobList != NULL
            ? g_jobList->getIndexSelected()
            : MyGUI::ITEM_NONE;
        const bool hasSelectedRow = hasCharacter &&
            selected != MyGUI::ITEM_NONE &&
            selected < g_rows.size();

        if (g_jobsToggleButton != NULL)
        {
            g_jobsToggleButton->setEnabled(hasCharacter);
        }
        if (g_moveUpButton != NULL)
        {
            g_moveUpButton->setEnabled(hasSelectedRow && selected > 0);
        }
        if (g_moveDownButton != NULL)
        {
            g_moveDownButton->setEnabled(
                hasSelectedRow && selected + 1 < g_rows.size());
        }
        if (g_removeButton != NULL)
        {
            g_removeButton->setEnabled(hasSelectedRow);
        }
        if (g_clearButton != NULL)
        {
            g_clearButton->setEnabled(hasCharacter && !g_rows.empty());
        }
        if (g_refreshButton != NULL)
        {
            g_refreshButton->setEnabled(true);
        }
        if (g_closeButton != NULL)
        {
            g_closeButton->setEnabled(true);
        }
    }

    void PopulateList(const std::vector<JobRowSnapshot>& rows, size_t preferredIndex)
    {
        if (g_jobList == NULL)
        {
            return;
        }

        g_refreshInProgress = true;
        g_jobList->removeAllItems();

        if (rows.empty())
        {
            g_jobList->addItem("[No permanent jobs]");
            g_jobList->clearIndexSelected();
        }
        else
        {
            for (size_t index = 0; index < rows.size(); ++index)
            {
                std::ostringstream label;
                label << (index + 1) << ". " << rows[index].name;
                const std::string rowLabel = label.str();
                g_jobList->addItem(rowLabel.c_str());
            }

            if (preferredIndex == MyGUI::ITEM_NONE || preferredIndex >= rows.size())
            {
                preferredIndex = 0;
            }
            g_jobList->setIndexSelected(preferredIndex);
            g_jobList->beginToItemAt(preferredIndex);
        }

        g_refreshInProgress = false;
    }

    void RefreshJobWindow(bool force, size_t preferredIndex)
    {
        if (g_window == NULL || g_jobList == NULL)
        {
            return;
        }

        HandleIdentity selectedHandle;
        ResetHandleIdentity(&selectedHandle);
        Character* character = NULL;
        if (!TryResolveSelectedCharacter(
                g_playerInterface,
                &selectedHandle,
                &character))
        {
            const bool hadCharacter = g_displayedCharacter.valid;
            ResetClearConfirmation();
            ResetHandleIdentity(&g_displayedCharacter);
            g_rows.clear();
            PopulateList(g_rows, MyGUI::ITEM_NONE);
            if (g_characterText != NULL)
            {
                g_characterText->setCaption("No player character selected");
            }
            if (g_jobsToggleButton != NULL)
            {
                g_jobsToggleButton->setCaption("Jobs: unavailable");
            }
            UpdateControlState(false);
            if (hadCharacter || force)
            {
                SetStatus("Select a player character to manage their permanent jobs.");
            }
            return;
        }

        std::vector<JobRowSnapshot> nextRows;
        bool truncated = false;
        if (!BuildSnapshot(character, &nextRows, &truncated))
        {
            SetStatus("Could not read the selected character's permanent-job queue.");
            UpdateControlState(false);
            return;
        }

        const bool characterChanged = !g_displayedCharacter.valid ||
            !SameHandleIdentity(g_displayedCharacter, selectedHandle);
        const bool rowsChanged = !SnapshotEquals(g_rows, nextRows);
        const size_t previousSelection = g_jobList->getIndexSelected();

        if (characterChanged || rowsChanged)
        {
            ResetClearConfirmation();
        }

        if (force || characterChanged || rowsChanged)
        {
            g_rows = nextRows;
            g_displayedCharacter = selectedHandle;

            size_t selection = preferredIndex;
            if (selection == MyGUI::ITEM_NONE && !characterChanged)
            {
                selection = previousSelection;
            }
            PopulateList(g_rows, selection);
        }

        std::string characterName;
        if (!TryGetCharacterName(character, &characterName) || characterName.empty())
        {
            characterName = "Selected character";
        }

        if (g_characterText != NULL)
        {
            std::ostringstream heading;
            heading << characterName << "  |  " << g_rows.size() << " permanent job";
            if (g_rows.size() != 1)
            {
                heading << "s";
            }
            const std::string headingText = heading.str();
            g_characterText->setCaption(headingText.c_str());
        }

        bool jobsEnabled = false;
        if (g_jobsToggleButton != NULL)
        {
            if (TryGetJobsEnabled(character, &jobsEnabled))
            {
                g_jobsToggleButton->setCaption(jobsEnabled ? "Jobs: ON" : "Jobs: OFF");
            }
            else
            {
                g_jobsToggleButton->setCaption("Jobs: unknown");
            }
        }

        UpdateControlState(true);

        if (characterChanged)
        {
            std::ostringstream status;
            status << "Showing " << characterName << "'s vanilla permanent-job queue.";
            SetStatus(status.str());
        }
        if (truncated)
        {
            SetStatus("Queue exceeded the 512-row safety limit; only the first rows are shown.");
        }
    }

    void RefreshJobWindow(bool force)
    {
        RefreshJobWindow(force, MyGUI::ITEM_NONE);
    }

    Character* ResolveDisplayedCharacter()
    {
        HandleIdentity selectedHandle;
        ResetHandleIdentity(&selectedHandle);
        Character* character = NULL;
        if (!TryResolveSelectedCharacter(
                g_playerInterface,
                &selectedHandle,
                &character))
        {
            RefreshJobWindow(true);
            SetStatus("No player character is selected.");
            return NULL;
        }

        if (!g_displayedCharacter.valid ||
            !SameHandleIdentity(selectedHandle, g_displayedCharacter))
        {
            RefreshJobWindow(true);
            SetStatus("Selection changed. Review the refreshed queue and try again.");
            return NULL;
        }

        return character;
    }

    bool ValidateUiSlot(Character* character, size_t slot)
    {
        if (character == NULL || slot >= g_rows.size())
        {
            return false;
        }

        TaskType currentType = static_cast<TaskType>(0);
        std::string currentName;
        const Tasker* currentTaskData = NULL;
        if (!TryGetPermajobRow(
                character,
                static_cast<int>(slot),
                &currentType,
                &currentName,
                &currentTaskData))
        {
            return false;
        }

        const JobRowSnapshot& displayed = g_rows[slot];
        if (displayed.taskData != NULL || currentTaskData != NULL)
        {
            return displayed.taskData == currentTaskData;
        }

        return displayed.taskType == currentType && displayed.name == currentName;
    }
