// SPDX-License-Identifier: GPL-3.0-only
// Value-only bindings and widget ownership for the multi-squad Jobs board.

    const int SQUAD_GROUP_HEADER_HEIGHT = 30;

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

    std::vector<VisibleMemberBinding> g_visibleMemberBindings;
    std::vector<SquadGroupWidgets> g_squadGroupWidgets;
    int g_dragDestinationVisibleIndex = -1;
    MyGUI::Button* g_prioritizeCoreJobsButton = NULL;
    bool g_squadGroupRebuildRequested = false;

    void OnSquadGroupToggle(MyGUI::Widget* widget);
    void OnPrioritizeCoreJobsClicked(MyGUI::Widget* widget);

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
