// SPDX-License-Identifier: GPL-3.0-only
    void OnJobListSelectionChanged(MyGUI::ListBox*, size_t)
    {
        if (g_refreshInProgress)
        {
            return;
        }

        ResetClearConfirmation();
        UpdateControlState(g_displayedCharacter.valid);
    }

    void OnRefreshClicked(MyGUI::Widget*)
    {
        ResetClearConfirmation();
        RefreshJobWindow(true);
        SetStatus("Queue refreshed from Kenshi's current runtime state.");
    }

    void OnJobsToggleClicked(MyGUI::Widget*)
    {
        ResetClearConfirmation();
        Character* character = ResolveDisplayedCharacter();
        if (character == NULL)
        {
            return;
        }

        bool enabled = false;
        if (!TryGetJobsEnabled(character, &enabled))
        {
            SetStatus("Kenshi did not expose a usable Jobs state for this character.");
            return;
        }

        if (!TrySetJobsEnabled(character, !enabled))
        {
            SetStatus("The Jobs toggle failed; no queue entries were changed.");
            return;
        }

        TryNotifyVanillaSelectionUI();
        RefreshJobWindow(true);
        SetStatus(enabled ? "Jobs disabled for the selected character." :
                            "Jobs enabled for the selected character.");
    }

    void MoveSelectedJob(int direction)
    {
        ResetClearConfirmation();
        Character* character = ResolveDisplayedCharacter();
        if (character == NULL || g_jobList == NULL)
        {
            return;
        }

        const size_t selected = g_jobList->getIndexSelected();
        if (selected == MyGUI::ITEM_NONE || selected >= g_rows.size())
        {
            SetStatus("Select a job before moving it.");
            return;
        }

        const int target = static_cast<int>(selected) + direction;
        if (target < 0 || target >= static_cast<int>(g_rows.size()))
        {
            return;
        }

        if (!ValidateUiSlot(character, selected))
        {
            RefreshJobWindow(true);
            SetStatus("The queue changed underneath the window. Review it and try again.");
            return;
        }

        if (!TryMovePermajob(character, static_cast<int>(selected), target))
        {
            SetStatus("Kenshi rejected the queue reorder.");
            return;
        }

        TryNotifyVanillaSelectionUI();
        RefreshJobWindow(true, static_cast<size_t>(target));
        SetStatus(direction < 0 ? "Job moved up one priority slot." :
                                  "Job moved down one priority slot.");
    }

    void OnMoveUpClicked(MyGUI::Widget*)
    {
        MoveSelectedJob(-1);
    }

    void OnMoveDownClicked(MyGUI::Widget*)
    {
        MoveSelectedJob(1);
    }

    void OnRemoveClicked(MyGUI::Widget*)
    {
        ResetClearConfirmation();
        Character* character = ResolveDisplayedCharacter();
        if (character == NULL || g_jobList == NULL)
        {
            return;
        }

        const size_t selected = g_jobList->getIndexSelected();
        if (selected == MyGUI::ITEM_NONE || selected >= g_rows.size())
        {
            SetStatus("Select a job before removing it.");
            return;
        }

        if (!ValidateUiSlot(character, selected))
        {
            RefreshJobWindow(true);
            SetStatus("The queue changed underneath the window. Review it and try again.");
            return;
        }

        const std::string removedName = g_rows[selected].name;
        if (!TryRemovePermajob(character, static_cast<int>(selected)))
        {
            SetStatus("Kenshi rejected the job removal.");
            return;
        }

        TryNotifyVanillaSelectionUI();
        size_t nextSelection = selected;
        if (nextSelection > 0 && nextSelection >= g_rows.size() - 1)
        {
            --nextSelection;
        }
        RefreshJobWindow(true, nextSelection);

        std::ostringstream status;
        status << "Removed: " << removedName;
        SetStatus(status.str());
    }

    void OnClearClicked(MyGUI::Widget*)
    {
        Character* character = ResolveDisplayedCharacter();
        if (character == NULL)
        {
            ResetClearConfirmation();
            return;
        }

        const DWORD now = GetTickCount();
        if (!g_clearArmed || HasElapsed(now, g_clearArmedTick, CLEAR_CONFIRMATION_MS))
        {
            g_clearArmed = true;
            g_clearArmedTick = now;
            if (g_clearButton != NULL)
            {
                g_clearButton->setCaption("Confirm Clear");
            }
            SetStatus("Click Confirm Clear again within four seconds to remove every permanent job.");
            return;
        }

        ResetClearConfirmation();
        if (!TryClearPermajobs(character))
        {
            SetStatus("Kenshi rejected the clear-all operation.");
            return;
        }

        TryNotifyVanillaSelectionUI();
        RefreshJobWindow(true);
        SetStatus("All permanent jobs were removed from the selected character.");
    }
