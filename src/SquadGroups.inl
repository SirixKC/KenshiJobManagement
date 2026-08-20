// SPDX-License-Identifier: GPL-3.0-only
// Value-only bindings and widget ownership for the multi-squad Jobs board.

    const int SQUAD_GROUP_HEADER_HEIGHT = 30;
    const int SQUAD_GROUP_GAP = ROW_HEIGHT / 2;

    struct VisibleMemberBinding
    {
        HandleIdentity squad;
        HandleIdentity member;
        int rowTop;

        VisibleMemberBinding() : rowTop(0) {}
    };

    struct SquadGroupWidgets
    {
        HandleIdentity squad;
        MyGUI::Button* memberHeader;
        MyGUI::Widget* jobHeader;

        SquadGroupWidgets() : memberHeader(NULL), jobHeader(NULL) {}
    };

    struct SelectedRecipient
    {
        HandleIdentity squad;
        HandleIdentity member;
    };

    enum PendingJobBatchUiActionType
    {
        JOB_BATCH_UI_NONE,
        JOB_BATCH_UI_PASTE,
        JOB_BATCH_UI_MOVE,
        JOB_BATCH_UI_ADD_HEALING,
        JOB_BATCH_UI_PRIORITIZE_HEALING
    };

    struct PendingJobBatchUiAction
    {
        PendingJobBatchUiActionType type;
        std::vector<SelectedJob> selectedJobs;
        std::vector<HandleIdentity> recipients;
        HandleIdentity destination;

        PendingJobBatchUiAction() : type(JOB_BATCH_UI_NONE) {}
    };

    std::vector<VisibleMemberBinding> g_visibleMemberBindings;
    std::vector<SquadGroupWidgets> g_squadGroupWidgets;
    std::vector<SelectedRecipient> g_selectedRecipients;
    PendingJobBatchUiAction g_pendingJobBatchUiAction;
    int g_dragDestinationVisibleIndex = -1;
    MyGUI::Button* g_prioritizeCoreJobsButton = NULL;
    MyGUI::Button* g_addHealingJobsButton = NULL;
    bool g_squadGroupRebuildRequested = false;

    void OnSquadGroupToggle(MyGUI::Widget* widget);
    void OnPrioritizeCoreJobsClicked(MyGUI::Widget* widget);
    void OnAddHealingJobsClicked(MyGUI::Widget* widget);
    void OnRecipientPortraitClicked(MyGUI::Widget* widget);
    void ApplyRecipientSelectionStates();
    void UpdateSquadHeading();
    void UpdateSquadSelectorSelection();

    bool HasPendingJobBatchUiAction()
    {
        return g_pendingJobBatchUiAction.type != JOB_BATCH_UI_NONE;
    }

    int FindSelectedRecipientIndex(const HandleIdentity& member)
    {
        for (size_t index = 0; index < g_selectedRecipients.size(); ++index)
        {
            if (SameHandleIdentity(
                    g_selectedRecipients[index].member, member))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool IsRecipientSelected(const HandleIdentity& member)
    {
        return FindSelectedRecipientIndex(member) >= 0;
    }

    void AddSelectedRecipient(
        const HandleIdentity& squad,
        const HandleIdentity& member)
    {
        if (!squad.valid || !member.valid || member.type != CHARACTER ||
            IsRecipientSelected(member))
        {
            return;
        }
        SelectedRecipient selected;
        selected.squad = squad;
        selected.member = member;
        g_selectedRecipients.push_back(selected);
    }

    void SelectOnlyRecipient(
        const HandleIdentity& squad,
        const HandleIdentity& member)
    {
        g_selectedRecipients.clear();
        AddSelectedRecipient(squad, member);
    }

    void ToggleSelectedRecipient(
        const HandleIdentity& squad,
        const HandleIdentity& member)
    {
        const int index = FindSelectedRecipientIndex(member);
        if (index >= 0)
        {
            g_selectedRecipients.erase(g_selectedRecipients.begin() + index);
        }
        else
        {
            AddSelectedRecipient(squad, member);
        }
    }

    size_t GetSquadRecipientSelectionCount(const SquadSnapshot& squad)
    {
        size_t selected = 0;
        for (size_t index = 0; index < squad.members.size(); ++index)
        {
            if (IsRecipientSelected(squad.members[index].identity))
            {
                ++selected;
            }
        }
        return selected;
    }

    void SelectOnlySquadRecipients(const SquadSnapshot& squad)
    {
        g_selectedRecipients.clear();
        for (size_t index = 0; index < squad.members.size(); ++index)
        {
            AddSelectedRecipient(squad.identity, squad.members[index].identity);
        }
    }

    void ToggleSquadRecipients(const SquadSnapshot& squad)
    {
        const bool allSelected = !squad.members.empty() &&
            GetSquadRecipientSelectionCount(squad) == squad.members.size();
        for (size_t index = 0; index < squad.members.size(); ++index)
        {
            const int selectedIndex = FindSelectedRecipientIndex(
                squad.members[index].identity);
            if (allSelected)
            {
                if (selectedIndex >= 0)
                {
                    g_selectedRecipients.erase(
                        g_selectedRecipients.begin() + selectedIndex);
                }
            }
            else if (selectedIndex < 0)
            {
                AddSelectedRecipient(
                    squad.identity, squad.members[index].identity);
            }
        }
    }

    void PruneSelectedRecipientsToDisplayedRoster()
    {
        std::vector<SelectedRecipient> retained;
        retained.reserve(g_selectedRecipients.size());
        for (size_t selectedIndex = 0;
             selectedIndex < g_selectedRecipients.size(); ++selectedIndex)
        {
            bool found = false;
            for (size_t squadIndex = 0;
                 squadIndex < g_allSquads.squads.size() && !found;
                 ++squadIndex)
            {
                const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
                for (size_t memberIndex = 0;
                     memberIndex < squad.members.size(); ++memberIndex)
                {
                    if (SameHandleIdentity(
                            squad.members[memberIndex].identity,
                            g_selectedRecipients[selectedIndex].member))
                    {
                        SelectedRecipient selected =
                            g_selectedRecipients[selectedIndex];
                        selected.squad = squad.identity;
                        retained.push_back(selected);
                        found = true;
                        break;
                    }
                }
            }
        }
        g_selectedRecipients.swap(retained);
    }

    std::vector<HandleIdentity> GetSelectedRecipientIdentities()
    {
        std::vector<HandleIdentity> recipients;
        recipients.reserve(g_selectedRecipients.size());
        for (size_t index = 0; index < g_selectedRecipients.size(); ++index)
        {
            recipients.push_back(g_selectedRecipients[index].member);
        }
        return recipients;
    }

    void ResetRecipientSelection()
    {
        g_selectedRecipients.clear();
    }

    const SquadSnapshot* FindDisplayedSquad(
        const HandleIdentity& identity)
    {
        const int index = FindAllSquadSnapshotIndex(identity);
        return index >= 0 ? &g_allSquads.squads[index] : NULL;
    }

    SquadSnapshot* FindDisplayedSquadMutable(
        const HandleIdentity& identity)
    {
        const int index = FindAllSquadSnapshotIndex(identity);
        return index >= 0 ? &g_allSquads.squads[index] : NULL;
    }

    const MemberSnapshot* FindDisplayedMember(
        const HandleIdentity& memberIdentity,
        const SquadSnapshot** squadOut = NULL)
    {
        for (size_t squadIndex = 0;
             squadIndex < g_allSquads.squads.size(); ++squadIndex)
        {
            const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
            const int memberIndex = FindMemberSnapshotIndex(
                squad, memberIdentity);
            if (memberIndex < 0)
            {
                continue;
            }
            if (squadOut != NULL)
            {
                *squadOut = &squad;
            }
            return &squad.members[memberIndex];
        }
        if (squadOut != NULL)
        {
            *squadOut = NULL;
        }
        return NULL;
    }

    const MemberSnapshot* GetVisibleMember(
        size_t visibleIndex,
        const SquadSnapshot** squadOut = NULL)
    {
        if (visibleIndex >= g_visibleMemberBindings.size())
        {
            if (squadOut != NULL)
            {
                *squadOut = NULL;
            }
            return NULL;
        }
        const VisibleMemberBinding& binding =
            g_visibleMemberBindings[visibleIndex];
        const SquadSnapshot* squad = FindDisplayedSquad(binding.squad);
        if (squad == NULL)
        {
            if (squadOut != NULL)
            {
                *squadOut = NULL;
            }
            return NULL;
        }
        const int memberIndex = FindMemberSnapshotIndex(
            *squad, binding.member);
        if (memberIndex < 0)
        {
            if (squadOut != NULL)
            {
                *squadOut = NULL;
            }
            return NULL;
        }
        if (squadOut != NULL)
        {
            *squadOut = squad;
        }
        return &squad->members[memberIndex];
    }

    int FindVisibleMemberIndex(const HandleIdentity& memberIdentity)
    {
        for (size_t index = 0;
             index < g_visibleMemberBindings.size(); ++index)
        {
            if (SameHandleIdentity(
                    g_visibleMemberBindings[index].member,
                    memberIdentity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    int FindDisplayedMemberOrder(const HandleIdentity& memberIdentity)
    {
        int order = 0;
        for (size_t squadIndex = 0;
             squadIndex < g_allSquads.squads.size(); ++squadIndex)
        {
            const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex, ++order)
            {
                if (SameHandleIdentity(
                        squad.members[memberIndex].identity,
                        memberIdentity))
                {
                    return order;
                }
            }
        }
        return -1;
    }

    int GetVisibleContentHeight()
    {
        int height = 0;
        for (size_t squadIndex = 0;
             squadIndex < g_allSquads.squads.size(); ++squadIndex)
        {
            height += SQUAD_GROUP_HEADER_HEIGHT;
            const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
            if (!IsSquadCollapsed(squad.identity))
            {
                height += static_cast<int>(squad.members.size()) * ROW_STRIDE;
            }
            if (squadIndex + 1 < g_allSquads.squads.size())
            {
                height += SQUAD_GROUP_GAP;
            }
        }
        return height;
    }

    size_t GetDisplayedMaximumJobCount()
    {
        size_t maximum = 0;
        for (size_t index = 0;
             index < g_visibleMemberBindings.size(); ++index)
        {
            const MemberSnapshot* member = GetVisibleMember(index);
            if (member != NULL)
            {
                maximum = std::max(maximum, member->jobs.size());
            }
        }
        return maximum;
    }

    void ResetSquadGroupViewState()
    {
        g_visibleMemberBindings.clear();
        g_squadGroupWidgets.clear();
        g_dragDestinationVisibleIndex = -1;
        g_squadGroupRebuildRequested = false;
    }
