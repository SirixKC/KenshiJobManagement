// SPDX-License-Identifier: GPL-3.0-only
// Value-only bindings and widget ownership for the multi-squad Jobs board.

    const int SQUAD_GROUP_HEADER_HEIGHT = 30;
    const int SQUAD_GROUP_GAP = ROW_HEIGHT / 2;

    struct VisibleMemberBinding
    {
        HandleIdentity squad;
        HandleIdentity member;
        size_t squadIndex;
        size_t memberIndex;
        int rowTop;

        VisibleMemberBinding() : squadIndex(0), memberIndex(0), rowTop(0) {}
    };

    struct SquadGroupWidgets
    {
        HandleIdentity squad;
        std::string name;
        size_t memberCount;
        bool collapsed;
        MyGUI::Button* memberHeader;
        MyGUI::Widget* jobHeader;

        SquadGroupWidgets() :
            memberCount(0), collapsed(false),
            memberHeader(NULL), jobHeader(NULL) {}
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
        JOB_BATCH_UI_PRIORITIZE_HEALING,
        JOB_BATCH_UI_REMOVE_INVALID
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
    void OnRemoveInvalidJobsClicked(MyGUI::Widget* widget);
    void OnRecipientPortraitClicked(MyGUI::Widget* widget);
    void ApplyRecipientSelectionStates();
    void UpdateSquadHeading();
    void UpdateSquadSelectorSelection();

    bool IsDisplayedRecipientEditable(
        const HandleIdentity& squadIdentity,
        const HandleIdentity& memberIdentity)
    {
        if (g_allSquads.incomplete)
        {
            return false;
        }
        for (size_t squadIndex = 0;
             squadIndex < g_allSquads.squads.size(); ++squadIndex)
        {
            const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
            if (!SameHandleIdentity(squad.identity, squadIdentity))
            {
                continue;
            }
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                const MemberSnapshot& member = squad.members[memberIndex];
                if (SameHandleIdentity(member.identity, memberIdentity))
                {
                    return member.queueAvailable;
                }
            }
            return false;
        }
        return false;
    }

    bool SquadRecipientSelectionAvailable(const SquadSnapshot& squad)
    {
        if (g_allSquads.incomplete || squad.incomplete ||
            squad.unavailable || squad.members.empty())
        {
            return false;
        }
        for (size_t index = 0; index < squad.members.size(); ++index)
        {
            if (!squad.members[index].queueAvailable)
            {
                return false;
            }
        }
        return true;
    }

    bool JobCardSelectionMode()
    {
        return !g_selectedJobs.empty() && g_selectedRecipients.empty();
    }

    bool RecipientSelectionMode()
    {
        if (!g_selectedJobs.empty() || g_selectedRecipients.empty())
        {
            return false;
        }
        for (size_t index = 0; index < g_selectedRecipients.size(); ++index)
        {
            if (!IsDisplayedRecipientEditable(
                    g_selectedRecipients[index].squad,
                    g_selectedRecipients[index].member))
            {
                return false;
            }
        }
        return true;
    }

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
            !IsDisplayedRecipientEditable(squad, member) ||
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
                    if (squad.members[memberIndex].queueAvailable &&
                        SameHandleIdentity(
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
        VisibleMemberBinding& binding =
            g_visibleMemberBindings[visibleIndex];
        if (binding.squadIndex < g_allSquads.squads.size())
        {
            const SquadSnapshot& indexedSquad =
                g_allSquads.squads[binding.squadIndex];
            if (SameHandleIdentity(indexedSquad.identity, binding.squad) &&
                binding.memberIndex < indexedSquad.members.size() &&
                SameHandleIdentity(
                    indexedSquad.members[binding.memberIndex].identity,
                    binding.member))
            {
                if (squadOut != NULL)
                {
                    *squadOut = &indexedSquad;
                }
                return &indexedSquad.members[binding.memberIndex];
            }
        }

        // A structural refresh should rebuild all bindings before this path
        // is needed. Keep an identity-based repair fallback so a stale index
        // can never address the wrong engine-facing row.
        const int squadIndex = FindAllSquadSnapshotIndex(binding.squad);
        const SquadSnapshot* squad = squadIndex >= 0 ?
            &g_allSquads.squads[squadIndex] : NULL;
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
        binding.squadIndex = static_cast<size_t>(squadIndex);
        binding.memberIndex = static_cast<size_t>(memberIndex);
        if (squadOut != NULL)
        {
            *squadOut = squad;
        }
        return &squad->members[memberIndex];
    }

    bool SquadWidgetStructureMatchesBoard()
    {
        if (g_squadGroupWidgets.size() != g_allSquads.squads.size())
        {
            return false;
        }
        size_t visibleIndex = 0;
        for (size_t squadIndex = 0;
             squadIndex < g_allSquads.squads.size(); ++squadIndex)
        {
            const SquadGroupWidgets& group =
                g_squadGroupWidgets[squadIndex];
            const SquadSnapshot& squad = g_allSquads.squads[squadIndex];
            const bool collapsed = IsSquadCollapsed(squad.identity);
            if (!SameHandleIdentity(group.squad, squad.identity) ||
                group.name != squad.name ||
                group.memberCount != squad.members.size() ||
                group.collapsed != collapsed)
            {
                return false;
            }
            if (collapsed)
            {
                continue;
            }
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                if (visibleIndex >= g_visibleMemberBindings.size())
                {
                    return false;
                }
                const VisibleMemberBinding& binding =
                    g_visibleMemberBindings[visibleIndex];
                if (binding.squadIndex != squadIndex ||
                    binding.memberIndex != memberIndex ||
                    !SameHandleIdentity(binding.squad, squad.identity) ||
                    !SameHandleIdentity(
                        binding.member,
                        squad.members[memberIndex].identity))
                {
                    return false;
                }
                ++visibleIndex;
            }
        }
        return visibleIndex == g_visibleMemberBindings.size() &&
            visibleIndex == g_memberWidgets.size();
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

    bool ApplyNativeTabSquadCollapseTransition()
    {
        // The hook runs after Kenshi processes TAB. Resolve the final native
        // current squad before changing UI-only collapse state. The refresh
        // that follows this helper then needs only one widget rebuild.
        if (!TrySkipDeadCurrentSquad())
        {
            return false;
        }

        hand currentHandle;
        std::string currentName;
        RootObjectContainer* currentActive = NULL;
        if (!TryGetCurrentSquad(
                g_playerInterface,
                &currentHandle,
                &currentName,
                &currentActive))
        {
            return false;
        }
        HandleIdentity currentIdentity;
        CaptureHandleIdentity(currentHandle, &currentIdentity);
        if (!currentIdentity.valid ||
            SameHandleIdentity(currentIdentity, g_squad.identity))
        {
            return true;
        }

        const int previousIndex =
            FindSquadCollapseStateIndex(g_squad.identity);
        if (previousIndex >= 0 &&
            g_squadCollapseStates[previousIndex].autoExpandedByNativeTab)
        {
            SetSquadCollapsedForNativeTab(
                g_squad.identity, true, false);
        }

        if (IsSquadCollapsed(currentIdentity))
        {
            SetSquadCollapsedForNativeTab(
                currentIdentity, false, true);
        }
        return true;
    }
