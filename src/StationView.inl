// SPDX-License-Identifier: GPL-3.0-only
// Category-grouped, virtualized Stations card grid.
//
// This file is included in the plug-in anonymous namespace after the station
// scanner and settings helpers.  It is deliberately a projection only: a
// MyGUI callback never changes an engine queue.  Add/remove callbacks pass
// stable HandleIdentity values to the manager for deferred validation.

    void RequestAddStationAssignment(
        const HandleIdentity& station,
        const HandleIdentity& member);
    void RequestRemoveStationAssignment(
        const HandleIdentity& station,
        const HandleIdentity& member);

    const int STATION_GRID_BANNER_HEIGHT = 38;
    const int STATION_GRID_SCROLL_SIZE = 20;
    const int STATION_GRID_GROUP_HEADER_HEIGHT = 34;
    const int STATION_GRID_CARD_WIDTH = 214;
    const int STATION_GRID_CARD_HEIGHT = 128;
    const int STATION_GRID_GAP = 10;
    const int STATION_GRID_SIDE_PAD = 8;
    const int STATION_GRID_OVERSCAN = STATION_GRID_CARD_HEIGHT;
    const int STATION_DETAIL_ROW_HEIGHT = 48;
    // JobWindow also uses these two historic sizing constants when it chooses
    // a safe minimum client size.  Keep them as view-interface constants even
    // though the matrix rows no longer exist.
    const int STATION_HEADER_HEIGHT = STATION_GRID_CARD_HEIGHT;
    const int STATION_MEMBER_ROW_HEIGHT = 116;

    struct StationGridGroup
    {
        StationCategory category;
        int top;
        int height;
        int unassignedCount;
        std::vector<size_t> stations;

        StationGridGroup() :
            category(STATION_OTHER), top(0), height(0), unassignedCount(0)
        {
        }
    };

    struct StationDetailPerson
    {
        HandleIdentity identity;
        std::string name;
        int relevantSkill;
        bool relevantSkillKnown;
        int totalJobs;

        StationDetailPerson() :
            relevantSkill(0), relevantSkillKnown(false), totalJobs(0)
        {
        }
    };

    struct StationGridCardBinding
    {
        HandleIdentity identity;
        MyGUI::Button* card;
        MyGUI::Widget* tint;
        MyGUI::Widget* hoverOverlay;
        MyGUI::Widget* unassignedTop;
        MyGUI::Widget* unassignedBottom;
        MyGUI::Widget* unassignedLeft;
        MyGUI::Widget* unassignedRight;
        MyGUI::Widget* recentMarker;
        MyGUI::TextBox* assignment;
        MyGUI::TextBox* status;

        StationGridCardBinding() :
            card(NULL), tint(NULL), hoverOverlay(NULL), unassignedTop(NULL),
            unassignedBottom(NULL), unassignedLeft(NULL),
            unassignedRight(NULL), recentMarker(NULL), assignment(NULL),
            status(NULL)
        {
        }
    };

    struct StationGridHeaderBinding
    {
        StationCategory category;
        MyGUI::Button* button;

        StationGridHeaderBinding() :
            category(STATION_OTHER), button(NULL)
        {
        }
    };

    struct StationViewState
    {
        MyGUI::Widget* root;
        MyGUI::TextBox* scanBanner;
        MyGUI::Widget* progressTrack;
        MyGUI::Widget* progressFill;
        MyGUI::Widget* viewport;
        MyGUI::Widget* canvas;
        MyGUI::ScrollBar* verticalScroll;
        MyGUI::TextBox* emptyText;
        const StationScanState* snapshot;
        std::vector<StationGridGroup> groups;
        std::vector<MyGUI::Widget*> virtualWidgets;
        std::vector<StationGridCardBinding> visibleCards;
        std::vector<StationGridHeaderBinding> visibleHeaders;
        std::vector<HandleIdentity> frozenCardOrder;
        int columns;
        int viewportWidth;
        int viewportHeight;
        int contentHeight;
        int verticalOffset;
        int maxVerticalOffset;
        bool changingScroll;
        bool visible;
        bool virtualRefreshRequested;
        bool multipleAreas;

        HandleIdentity pendingDetail;
        HandleIdentity detailStation;
        MyGUI::Widget* modalShade;
        MyGUI::Widget* modalPanel;
        MyGUI::TextBox* detailAssignedHeader;
        MyGUI::TextBox* detailAvailableHeader;
        MyGUI::Widget* detailViewport;
        MyGUI::Widget* detailCanvas;
        MyGUI::ScrollBar* detailScroll;
        MyGUI::Widget* detailAvailableViewport;
        MyGUI::Widget* detailAvailableCanvas;
        MyGUI::ScrollBar* detailAvailableScroll;
        MyGUI::TextBox* detailStatus;
        std::vector<MyGUI::Widget*> detailWidgets;
        std::vector<StationDetailPerson> assignedPeople;
        std::vector<StationDetailPerson> addPeople;
        int detailOffset;
        int detailContentHeight;
        bool detailScrollChanging;
        int detailAvailableOffset;
        int detailAvailableContentHeight;
        bool detailAvailableScrollChanging;
        bool detailRefreshRequested;
        bool fullRefreshRequested;
        bool detailCloseRequested;
        bool detailModalAdded;
        int hoveredCardIndex;
        std::string detailStatusText;
        HandleIdentity recentStation;
        std::vector<HandleIdentity> recentMembers;

        StationViewState() :
            root(NULL), scanBanner(NULL), progressTrack(NULL),
            progressFill(NULL), viewport(NULL), canvas(NULL),
            verticalScroll(NULL), emptyText(NULL), snapshot(NULL), columns(1),
            viewportWidth(0), viewportHeight(0), contentHeight(0),
            verticalOffset(0), maxVerticalOffset(0), changingScroll(false),
            visible(false), virtualRefreshRequested(false), multipleAreas(false),
            modalShade(NULL), modalPanel(NULL), detailAssignedHeader(NULL),
            detailAvailableHeader(NULL), detailViewport(NULL),
            detailCanvas(NULL), detailScroll(NULL),
            detailAvailableViewport(NULL), detailAvailableCanvas(NULL),
            detailAvailableScroll(NULL), detailStatus(NULL),
            detailOffset(0),
            detailContentHeight(0), detailScrollChanging(false),
            detailAvailableOffset(0), detailAvailableContentHeight(0),
            detailAvailableScrollChanging(false), detailRefreshRequested(false),
            fullRefreshRequested(false), detailCloseRequested(false),
            detailModalAdded(false), hoveredCardIndex(-1)
        {
        }
    };

    StationViewState g_stationView;

    void RefreshStationView();
    void RefreshStationVirtualWidgets();
    void RefreshStationDetail();
    void CloseStationDetail();
    void SetStationDetailStatus(const std::string& text);
    void MarkStationDetailChange(
        const HandleIdentity& station,
        const HandleIdentity& member);
    void OnStationVerticalScroll(MyGUI::ScrollBar*, size_t);
    void OnStationMouseWheel(MyGUI::Widget*, int);
    void OnStationCategoryClicked(MyGUI::Widget*);
    void OnStationCardClicked(MyGUI::Widget*);
    void OnStationCardMouseSetFocus(MyGUI::Widget*, MyGUI::Widget*);
    void OnStationCardMouseLostFocus(MyGUI::Widget*, MyGUI::Widget*);
    void OnStationDetailClose(MyGUI::Widget*);
    void OnStationDetailScroll(MyGUI::ScrollBar*, size_t);
    void OnStationAvailableScroll(MyGUI::ScrollBar*, size_t);
    void OnStationDetailWheel(MyGUI::Widget*, int);
    void OnStationAvailableWheel(MyGUI::Widget*, int);
    void OnStationAddPressed(
        MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnStationAddKey(
        MyGUI::Widget*, MyGUI::KeyCode, MyGUI::Char);
    void OnStationAssignedPressed(
        MyGUI::Widget*, int, int, MyGUI::MouseButton);

    std::string StationIntegerString(int value)
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    int StationParseIndex(MyGUI::Widget* widget, const char* key)
    {
        if (widget == NULL || !widget->isUserString(key))
        {
            return -1;
        }
        return std::atoi(widget->getUserString(key).c_str());
    }

    std::string StationIdentityString(const HandleIdentity& identity)
    {
        std::ostringstream stream;
        stream << (identity.valid ? 1 : 0) << ' '
               << static_cast<int>(identity.type) << ' '
               << identity.container << ' ' << identity.containerSerial << ' '
               << identity.index << ' ' << identity.serial;
        return stream.str();
    }

    bool ParseStationIdentity(
        MyGUI::Widget* widget,
        const char* key,
        HandleIdentity* identityOut)
    {
        if (widget == NULL || identityOut == NULL ||
            !widget->isUserString(key))
        {
            return false;
        }
        int valid = 0;
        int type = 0;
        HandleIdentity result;
        std::istringstream stream(widget->getUserString(key));
        stream >> valid >> type >> result.container >> result.containerSerial
               >> result.index >> result.serial;
        if (!stream || valid == 0)
        {
            return false;
        }
        result.valid = true;
        result.type = static_cast<itemType>(type);
        *identityOut = result;
        return true;
    }

    std::string StationLower(const std::string& value)
    {
        std::string result = value;
        for (size_t index = 0; index < result.size(); ++index)
        {
            const unsigned char character =
                static_cast<unsigned char>(result[index]);
            result[index] = static_cast<char>(std::tolower(character));
        }
        return result;
    }

    bool StationIdentityIn(
        const std::vector<HandleIdentity>& identities,
        const HandleIdentity& identity)
    {
        for (size_t index = 0; index < identities.size(); ++index)
        {
            if (SameHandleIdentity(identities[index], identity))
            {
                return true;
            }
        }
        return false;
    }

    const StationTargetSnapshot* FindStationSnapshot(
        const HandleIdentity& identity,
        size_t* indexOut)
    {
        if (g_stationView.snapshot == NULL || !identity.valid)
        {
            return NULL;
        }
        for (size_t index = 0;
             index < g_stationView.snapshot->stations.size(); ++index)
        {
            if (SameHandleIdentity(
                    g_stationView.snapshot->stations[index].identity,
                    identity))
            {
                if (indexOut != NULL)
                {
                    *indexOut = index;
                }
                return &g_stationView.snapshot->stations[index];
            }
        }
        return NULL;
    }

    const StationMemberSnapshot* FindStationMemberSnapshot(
        const HandleIdentity& identity)
    {
        if (g_stationView.snapshot == NULL || !identity.valid)
        {
            return NULL;
        }
        for (size_t squadIndex = 0;
             squadIndex < g_stationView.snapshot->squads.size(); ++squadIndex)
        {
            const StationSquadSnapshot& squad =
                g_stationView.snapshot->squads[squadIndex];
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                if (SameHandleIdentity(
                        squad.members[memberIndex].identity, identity))
                {
                    return &squad.members[memberIndex];
                }
            }
        }
        return NULL;
    }

    bool StationPassesFilters(const StationTargetSnapshot& station)
    {
        // Stations are filtered only by the broad category preference. The
        // former per-stat filter hid training, defense, and other stations
        // when their associated character skill was disabled. Character
        // stats are display-only on Squad Jobs and no longer affect this tab.
        return IsStationCategoryEnabled(station.category);
    }

    size_t CountUniqueStationPeople(const StationTargetSnapshot& station)
    {
        std::vector<HandleIdentity> people;
        for (size_t index = 0; index < station.assignments.size(); ++index)
        {
            if (!StationIdentityIn(people, station.assignments[index].member))
            {
                people.push_back(station.assignments[index].member);
            }
        }
        return people.size();
    }

    struct StationCardAlphabeticalLess
    {
        bool operator()(size_t left, size_t right) const
        {
            const StationTargetSnapshot& a =
                g_stationView.snapshot->stations[left];
            const StationTargetSnapshot& b =
                g_stationView.snapshot->stations[right];
            const bool aUnassigned = a.assignments.empty();
            const bool bUnassigned = b.assignments.empty();
            if (aUnassigned != bUnassigned)
            {
                return aUnassigned;
            }
            const std::string aName = StationLower(a.name);
            const std::string bName = StationLower(b.name);
            if (aName != bName)
            {
                return aName < bName;
            }
            const std::string aArea = StationLower(a.areaName);
            const std::string bArea = StationLower(b.areaName);
            if (aArea != bArea)
            {
                return aArea < bArea;
            }
            if (a.identity.container != b.identity.container)
            {
                return a.identity.container < b.identity.container;
            }
            if (a.identity.index != b.identity.index)
            {
                return a.identity.index < b.identity.index;
            }
            return a.identity.serial < b.identity.serial;
        }
    };

    int FrozenStationOrder(const HandleIdentity& identity)
    {
        for (size_t index = 0;
             index < g_stationView.frozenCardOrder.size(); ++index)
        {
            if (SameHandleIdentity(
                    g_stationView.frozenCardOrder[index], identity))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    struct StationFrozenLess
    {
        bool operator()(size_t left, size_t right) const
        {
            const int leftOrder = FrozenStationOrder(
                g_stationView.snapshot->stations[left].identity);
            const int rightOrder = FrozenStationOrder(
                g_stationView.snapshot->stations[right].identity);
            if (leftOrder >= 0 && rightOrder >= 0)
            {
                return leftOrder < rightOrder;
            }
            if (leftOrder >= 0)
            {
                return true;
            }
            if (rightOrder >= 0)
            {
                return false;
            }
            return StationCardAlphabeticalLess()(left, right);
        }
    };

    int StationCardRowStride()
    {
        return STATION_GRID_CARD_HEIGHT + STATION_GRID_GAP;
    }

    void BuildStationGroups()
    {
        g_stationView.groups.clear();
        g_stationView.multipleAreas = false;
        if (g_stationView.snapshot == NULL)
        {
            g_stationView.contentHeight = 0;
            return;
        }

        std::vector<std::string> areas;
        int top = 0;
        for (int categoryValue = static_cast<int>(STATION_CRAFTING);
             categoryValue <= static_cast<int>(STATION_OTHER);
             ++categoryValue)
        {
            StationGridGroup group;
            group.category = static_cast<StationCategory>(categoryValue);
            for (size_t index = 0;
                 index < g_stationView.snapshot->stations.size(); ++index)
            {
                const StationTargetSnapshot& station =
                    g_stationView.snapshot->stations[index];
                if (station.category != group.category ||
                    !StationPassesFilters(station))
                {
                    continue;
                }
                group.stations.push_back(index);
                if (station.assignments.empty())
                {
                    ++group.unassignedCount;
                }
                if (std::find(areas.begin(), areas.end(), station.areaName) ==
                    areas.end())
                {
                    areas.push_back(station.areaName);
                }
            }
            if (group.stations.empty())
            {
                continue;
            }
            if (g_stationView.detailStation.valid &&
                !g_stationView.frozenCardOrder.empty())
            {
                std::stable_sort(
                    group.stations.begin(), group.stations.end(),
                    StationFrozenLess());
            }
            else
            {
                std::stable_sort(
                    group.stations.begin(), group.stations.end(),
                    StationCardAlphabeticalLess());
            }
            group.top = top;
            if (IsStationCategoryCollapsed(group.category))
            {
                group.height = STATION_GRID_GROUP_HEADER_HEIGHT;
            }
            else
            {
                const int rows = static_cast<int>(
                    (group.stations.size() + g_stationView.columns - 1) /
                    g_stationView.columns);
                group.height = STATION_GRID_GROUP_HEADER_HEIGHT +
                    rows * StationCardRowStride();
            }
            top += group.height;
            g_stationView.groups.push_back(group);
        }
        g_stationView.multipleAreas = areas.size() > 1;
        g_stationView.contentHeight = top;
    }

    bool FindStationCardTop(
        const HandleIdentity& identity,
        int* topOut)
    {
        for (size_t groupIndex = 0;
             groupIndex < g_stationView.groups.size(); ++groupIndex)
        {
            const StationGridGroup& group = g_stationView.groups[groupIndex];
            if (IsStationCategoryCollapsed(group.category))
            {
                continue;
            }
            for (size_t stationIndex = 0;
                 stationIndex < group.stations.size(); ++stationIndex)
            {
                const StationTargetSnapshot& station =
                    g_stationView.snapshot->stations[group.stations[stationIndex]];
                if (SameHandleIdentity(station.identity, identity))
                {
                    *topOut = group.top + STATION_GRID_GROUP_HEADER_HEIGHT +
                        static_cast<int>(stationIndex / g_stationView.columns) *
                        StationCardRowStride();
                    return true;
                }
            }
        }
        return false;
    }

    bool CaptureStationScrollAnchor(HandleIdentity* identityOut, int* deltaOut)
    {
        if (identityOut == NULL || deltaOut == NULL ||
            g_stationView.snapshot == NULL)
        {
            return false;
        }
        for (size_t groupIndex = 0;
             groupIndex < g_stationView.groups.size(); ++groupIndex)
        {
            const StationGridGroup& group = g_stationView.groups[groupIndex];
            if (IsStationCategoryCollapsed(group.category))
            {
                continue;
            }
            for (size_t stationIndex = 0;
                 stationIndex < group.stations.size(); ++stationIndex)
            {
                const int top = group.top + STATION_GRID_GROUP_HEADER_HEIGHT +
                    static_cast<int>(stationIndex / g_stationView.columns) *
                    StationCardRowStride();
                if (top + STATION_GRID_CARD_HEIGHT >=
                    g_stationView.verticalOffset)
                {
                    *identityOut = g_stationView.snapshot->stations[
                        group.stations[stationIndex]].identity;
                    *deltaOut = g_stationView.verticalOffset - top;
                    return true;
                }
            }
        }
        return false;
    }

    void DestroyStationWidgetList(std::vector<MyGUI::Widget*>* widgets)
    {
        if (widgets == NULL)
        {
            return;
        }
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            for (size_t index = 0; index < widgets->size(); ++index)
            {
                if ((*widgets)[index] != NULL)
                {
                    gui->destroyWidget((*widgets)[index]);
                }
            }
        }
        widgets->clear();
    }

    bool TrySetStationCategoryIcon(
        MyGUI::ImageBox* icon,
        const char* resource)
    {
        if (icon == NULL || resource == NULL || resource[0] == '\0')
        {
            return false;
        }
        try
        {
            icon->setImageTexture(resource);
            const MyGUI::IntSize size = icon->getImageSize();
            return size.width > 0 && size.height > 0;
        }
        catch (...)
        {
            return false;
        }
    }

    void AttachStationTooltip(MyGUI::Widget* widget, const std::string& text)
    {
        if (widget == NULL || text.empty())
        {
            return;
        }
        widget->setUserString("KJM_ToolTip", text);
        widget->setNeedToolTip(true);
        widget->eventToolTip += MyGUI::newDelegate(OnCardToolTip);
    }

    void SetStationGroupHeaderCaption(
        MyGUI::Button* header,
        const StationGridGroup& group)
    {
        if (header == NULL)
        {
            return;
        }
        std::ostringstream caption;
        caption << (IsStationCategoryCollapsed(group.category) ? "+ " : "- ")
                << GetStationCategoryName(group.category) << "  |  "
                << group.stations.size() << " station";
        if (group.stations.size() != 1)
        {
            caption << "s";
        }
        caption << "  |  " << group.unassignedCount << " unassigned";
        header->setCaption(caption.str());
    }

    void CreateStationGroupHeader(const StationGridGroup& group)
    {
        const int y = group.top - g_stationView.verticalOffset;
        MyGUI::Button* header =
            g_stationView.canvas->createWidget<MyGUI::Button>(
                "Kenshi_Button1",
                MyGUI::IntCoord(4, y + 2, g_stationView.viewportWidth - 8,
                    STATION_GRID_GROUP_HEADER_HEIGHT - 4),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationCategoryHeader");
        SetStationGroupHeaderCaption(header, group);
        header->setFontHeight(16);
        header->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        header->setUserString(
            "KJM_StationCategory", StationIntegerString(group.category));
        header->eventMouseButtonClick +=
            MyGUI::newDelegate(OnStationCategoryClicked);
        header->eventMouseWheel += MyGUI::newDelegate(OnStationMouseWheel);
        TagThemeButtonText(header);
        g_stationView.virtualWidgets.push_back(header);
        StationGridHeaderBinding binding;
        binding.category = group.category;
        binding.button = header;
        g_stationView.visibleHeaders.push_back(binding);
    }

    void SetFittedStationGridName(
        MyGUI::TextBox* text,
        const std::string& caption)
    {
        text->setCaption(caption);
        text->setFontHeight(22);
        while (text->getFontHeight() > 10 &&
            (text->getTextSize().width > STATION_GRID_CARD_WIDTH - 20 ||
             text->getTextSize().height > 42))
        {
            text->setFontHeight(text->getFontHeight() - 1);
        }
    }

    void SetStationCardAssignment(
        MyGUI::TextBox* assignment,
        const StationTargetSnapshot& station)
    {
        if (assignment == NULL)
        {
            return;
        }
        const size_t assignedCount = CountUniqueStationPeople(station);
        if (assignedCount == 0)
        {
            assignment->setCaption("X");
            TagThemeWarningText(assignment);
        }
        else
        {
            std::ostringstream count;
            count << assignedCount;
            assignment->setCaption(count.str());
            TagThemeSuccessText(assignment);
        }
        assignment->setFontHeight(28);
        assignment->setTextAlign(MyGUI::Align::Center);
        assignment->setNeedMouseFocus(false);
    }

    bool IsStationUsableAndUnassigned(
        const StationTargetSnapshot& station)
    {
        return station.assignments.empty() && station.blockingStatusKnown &&
            !station.blocking && station.blockingStatus.empty();
    }

    void SetStationUnassignedOutlineVisible(
        StationGridCardBinding* binding,
        bool visible)
    {
        if (binding == NULL)
        {
            return;
        }
        if (binding->unassignedTop != NULL)
            binding->unassignedTop->setVisible(visible);
        if (binding->unassignedBottom != NULL)
            binding->unassignedBottom->setVisible(visible);
        if (binding->unassignedLeft != NULL)
            binding->unassignedLeft->setVisible(visible);
        if (binding->unassignedRight != NULL)
            binding->unassignedRight->setVisible(visible);
    }

    bool GetStationCardBinding(
        MyGUI::Widget* widget,
        int* indexOut)
    {
        if (widget == NULL || indexOut == NULL ||
            !widget->isUserString("KJM_StationVisibleCard"))
        {
            return false;
        }
        const int index = StationParseIndex(
            widget, "KJM_StationVisibleCard");
        if (index < 0 ||
            index >= static_cast<int>(g_stationView.visibleCards.size()) ||
            g_stationView.visibleCards[index].card != widget)
        {
            return false;
        }
        *indexOut = index;
        return true;
    }

    void SetStationCardHover(
        int index,
        bool visible)
    {
        if (index < 0 ||
            index >= static_cast<int>(g_stationView.visibleCards.size()))
        {
            return;
        }
        MyGUI::Widget* overlay =
            g_stationView.visibleCards[index].hoverOverlay;
        if (overlay != NULL && overlay->getVisible() != visible)
        {
            overlay->setVisible(visible);
        }
    }

    void ApplyStationCardHoverDelta(
        int previousIndex,
        int nextIndex)
    {
        if (previousIndex == nextIndex)
        {
            return;
        }
        SetStationCardHover(previousIndex, false);
        SetStationCardHover(nextIndex, true);
        g_stationView.hoveredCardIndex = nextIndex;
    }

    void ClearStationCardHover()
    {
        const int previousIndex = g_stationView.hoveredCardIndex;
        if (previousIndex < 0)
        {
            return;
        }
        SetStationCardHover(previousIndex, false);
        g_stationView.hoveredCardIndex = -1;
    }

    void SetFittedStationSingleLine(
        MyGUI::TextBox* text,
        const std::string& caption,
        int initialFontHeight,
        int minimumFontHeight)
    {
        if (text == NULL)
        {
            return;
        }
        text->setCaption(caption);
        text->setFontHeight(initialFontHeight);
        while (text->getFontHeight() > minimumFontHeight &&
            text->getTextSize().width > text->getWidth() - 4)
        {
            text->setFontHeight(text->getFontHeight() - 1);
        }
    }

    void CreateStationGridCard(
        const StationGridGroup& group,
        size_t position)
    {
        if (position >= group.stations.size())
        {
            return;
        }
        const StationTargetSnapshot& station =
            g_stationView.snapshot->stations[group.stations[position]];
        const int column = static_cast<int>(position % g_stationView.columns);
        const int row = static_cast<int>(position / g_stationView.columns);
        const int x = STATION_GRID_SIDE_PAD +
            column * (STATION_GRID_CARD_WIDTH + STATION_GRID_GAP);
        const int y = group.top + STATION_GRID_GROUP_HEADER_HEIGHT +
            row * StationCardRowStride() - g_stationView.verticalOffset;
        MyGUI::Button* card = g_stationView.canvas->createWidget<MyGUI::Button>(
            "Kenshi_Button1",
            MyGUI::IntCoord(x, y, STATION_GRID_CARD_WIDTH,
                STATION_GRID_CARD_HEIGHT),
            MyGUI::Align::Left | MyGUI::Align::Top, "KJM_StationCard");
        TagThemeBackground(card);
        card->setUserString(
            "KJM_StationIdentity", StationIdentityString(station.identity));
        card->setUserString(
            "KJM_StationVisibleCard",
            StationIntegerString(static_cast<int>(
                g_stationView.visibleCards.size())));
        card->eventMouseButtonClick += MyGUI::newDelegate(OnStationCardClicked);
        card->eventMouseWheel += MyGUI::newDelegate(OnStationMouseWheel);
        card->eventMouseSetFocus +=
            MyGUI::newDelegate(OnStationCardMouseSetFocus);
        card->eventMouseLostFocus +=
            MyGUI::newDelegate(OnStationCardMouseLostFocus);

        // Keep station labels on an explicit palette surface. The native
        // button atlas is dark and cannot be lightened reliably by tinting;
        // this child preserves the button hitbox and callback state.
        MyGUI::Widget* cardSurface = card->createWidget<MyGUI::Widget>(
            "WhiteSkin",
            MyGUI::IntCoord(
                3, 3, STATION_GRID_CARD_WIDTH - 6,
                STATION_GRID_CARD_HEIGHT - 6),
            MyGUI::Align::Stretch,
            "KJM_StationCardSurface");
        TagThemeSurface(cardSurface);

        const char* iconResource = GetStationVisualIconResource(
            station.category, station.visualSubtype);
        const int iconSize = STATION_GRID_CARD_HEIGHT;
        MyGUI::ImageBox* icon = card->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                (STATION_GRID_CARD_WIDTH - iconSize) / 2,
                0, iconSize, iconSize),
            MyGUI::Align::Top,
            "KJM_StationCardBackground");
        icon->setAlpha(0.33f);
        icon->setInheritsAlpha(false);
        icon->setNeedMouseFocus(false);
        if (!TrySetStationCategoryIcon(icon, iconResource))
        {
            icon->setVisible(false);
        }

        MyGUI::Widget* tint = card->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(0, 0, STATION_GRID_CARD_WIDTH,
                STATION_GRID_CARD_HEIGHT), MyGUI::Align::Stretch,
            "KJM_StationCardTint");
        const bool recentlyChanged =
            SameHandleIdentity(
                g_stationView.recentStation, station.identity) &&
            !g_stationView.recentMembers.empty();
        TagThemeStationCardTint(
            tint,
            station.blocking || !station.blockingStatus.empty());
        tint->setNeedMouseFocus(false);

        // Keep hover feedback independent from the blocking tint, the
        // unassigned warning, and the recent-change marker.  A subtle green
        // inner wash makes the card easier to track without replacing any of
        // those states, on either the light or dark palette.
        MyGUI::Widget* hoverOverlay = card->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(3, 3,
                STATION_GRID_CARD_WIDTH - 6,
                STATION_GRID_CARD_HEIGHT - 6),
            MyGUI::Align::Stretch, "KJM_StationCardHover");
        hoverOverlay->setColour(MyGUI::Colour(0.35f, 0.95f, 0.45f));
        hoverOverlay->setAlpha(0.18f);
        hoverOverlay->setNeedMouseFocus(false);
        hoverOverlay->setVisible(false);

        // A usable station with nobody assigned is a planning warning, not a
        // work-blocking failure. Four thin yellow edges keep that state
        // distinct from the red blocking tint and the red zero-person X.
        StationGridCardBinding binding;
        binding.identity = station.identity;
        binding.card = card;
        binding.tint = tint;
        binding.hoverOverlay = hoverOverlay;
        binding.unassignedTop = card->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(1, 1,
                STATION_GRID_CARD_WIDTH - 2, 3),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_StationUnassignedTop");
        binding.unassignedBottom = card->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(1, STATION_GRID_CARD_HEIGHT - 4,
                STATION_GRID_CARD_WIDTH - 2, 3),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_StationUnassignedBottom");
        binding.unassignedLeft = card->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(1, 4, 3,
                STATION_GRID_CARD_HEIGHT - 8),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_StationUnassignedLeft");
        binding.unassignedRight = card->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(STATION_GRID_CARD_WIDTH - 4, 4, 3,
                STATION_GRID_CARD_HEIGHT - 8),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_StationUnassignedRight");
        MyGUI::Widget* unassignedEdges[4] = {
            binding.unassignedTop, binding.unassignedBottom,
            binding.unassignedLeft, binding.unassignedRight
        };
        for (int edge = 0; edge < 4; ++edge)
        {
            unassignedEdges[edge]->setColour(
                MyGUI::Colour(1.0f, 0.80f, 0.20f));
            unassignedEdges[edge]->setAlpha(0.92f);
            unassignedEdges[edge]->setNeedMouseFocus(false);
        }
        SetStationUnassignedOutlineVisible(
            &binding,
            IsStationUsableAndUnassigned(station) && !recentlyChanged);

        // Keep recent successful changes visible even when the station is
        // also blocked.  The blocking tint remains red; this separate blue
        // bar is the non-competing recent-change indicator.
        MyGUI::Widget* recentMarker = card->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(2, 2,
                STATION_GRID_CARD_WIDTH - 4, 4),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_StationRecentMarker");
        recentMarker->setColour(MyGUI::Colour(0.48f, 0.82f, 1.0f));
        recentMarker->setAlpha(0.92f);
        recentMarker->setNeedMouseFocus(false);
        recentMarker->setVisible(recentlyChanged);
        binding.recentMarker = recentMarker;

        MyGUI::TextBox* name = card->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            MyGUI::IntCoord(8, 4, STATION_GRID_CARD_WIDTH - 16, 43),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_StationExactName");
        SetFittedStationGridName(name, station.name);
        name->setTextAlign(MyGUI::Align::Center);
        TagThemeStandardText(name);
        name->setNeedMouseFocus(false);

        int lineTop = 49;
        // Keep the location row stable when a category filter changes.  The
        // old multipleAreas optimization derived this from the currently
        // visible cards, so toggling Defense could make every other card lose
        // its location text when the filtered result collapsed to one area.
        if (!station.areaName.empty())
        {
            MyGUI::TextBox* area = card->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(7, lineTop, STATION_GRID_CARD_WIDTH - 14, 17),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationArea");
            SetFittedStationSingleLine(
                area, station.areaName, 13, 9);
            area->setTextAlign(MyGUI::Align::Center);
            TagThemeAccentText(area);
            area->setNeedMouseFocus(false);
            lineTop += 17;
        }

        MyGUI::TextBox* assignment = card->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(7, lineTop, STATION_GRID_CARD_WIDTH - 14, 35),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_StationAssignedCount");
        SetStationCardAssignment(assignment, station);
        binding.assignment = assignment;

        MyGUI::TextBox* status = card->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(7, STATION_GRID_CARD_HEIGHT - 27,
                STATION_GRID_CARD_WIDTH - 14, 20),
            MyGUI::Align::Bottom | MyGUI::Align::HStretch,
            "KJM_StationBlockingStatus");
        if (station.blocking || !station.blockingStatus.empty())
        {
            SetFittedStationSingleLine(
                status,
                station.blockingStatus.empty() ?
                    "CANNOT WORK" : station.blockingStatus,
                12, 9);
            TagThemeWarningText(status);
        }
        status->setTextAlign(MyGUI::Align::Center);
        status->setNeedMouseFocus(false);
        binding.status = status;

        std::ostringstream tooltip;
        tooltip << station.name << "\nCategory: "
                << GetStationCategoryName(station.category);
        if (!station.areaName.empty())
        {
            tooltip << "\nArea: " << station.areaName;
        }
        tooltip << "\nRelevant skill: "
                << (station.relevantSkillName.empty() ? "None" :
                    station.relevantSkillName);
        if (!station.assignmentSupported)
        {
            tooltip << "\nAssignment: Not supported for this station";
        }
        if (!station.blockingStatus.empty())
        {
            tooltip << "\nCannot work: " << station.blockingStatus;
        }
        else if (!station.blockingStatusKnown)
        {
            tooltip << "\nWork status: Unavailable";
        }
        AttachStationTooltip(card, tooltip.str());
        g_stationView.virtualWidgets.push_back(card);
        g_stationView.visibleCards.push_back(binding);
    }

    void UpdateStationScrollRange()
    {
        g_stationView.maxVerticalOffset = std::max(
            0, g_stationView.contentHeight - g_stationView.viewportHeight);
        g_stationView.verticalOffset = ClampInt(
            g_stationView.verticalOffset, 0, g_stationView.maxVerticalOffset);
        if (g_stationView.verticalScroll == NULL)
        {
            return;
        }
        g_stationView.changingScroll = true;
        g_stationView.verticalScroll->setScrollRange(
            static_cast<size_t>(g_stationView.maxVerticalOffset + 1));
        g_stationView.verticalScroll->setScrollPage(
            static_cast<size_t>(std::max(1, g_stationView.viewportHeight)));
        g_stationView.verticalScroll->setScrollViewPage(
            static_cast<size_t>(StationCardRowStride()));
        g_stationView.verticalScroll->setScrollPosition(
            static_cast<size_t>(g_stationView.verticalOffset));
        g_stationView.verticalScroll->setEnabled(
            g_stationView.maxVerticalOffset > 0);
        g_stationView.changingScroll = false;
    }

    void RefreshStationVirtualWidgets()
    {
        ClearStationCardHover();
        g_stationView.visibleCards.clear();
        g_stationView.visibleHeaders.clear();
        DestroyStationWidgetList(&g_stationView.virtualWidgets);
        g_stationView.virtualRefreshRequested = false;
        if (!g_stationView.visible || g_stationView.canvas == NULL ||
            g_stationView.snapshot == NULL)
        {
            return;
        }
        const int visibleTop = g_stationView.verticalOffset -
            STATION_GRID_OVERSCAN;
        const int visibleBottom = g_stationView.verticalOffset +
            g_stationView.viewportHeight + STATION_GRID_OVERSCAN;
        for (size_t groupIndex = 0;
             groupIndex < g_stationView.groups.size(); ++groupIndex)
        {
            const StationGridGroup& group = g_stationView.groups[groupIndex];
            if (group.top + group.height < visibleTop ||
                group.top > visibleBottom)
            {
                continue;
            }
            CreateStationGroupHeader(group);
            if (IsStationCategoryCollapsed(group.category))
            {
                continue;
            }
            for (size_t position = 0;
                 position < group.stations.size(); ++position)
            {
                const int cardTop = group.top +
                    STATION_GRID_GROUP_HEADER_HEIGHT +
                    static_cast<int>(position / g_stationView.columns) *
                    StationCardRowStride();
                if (cardTop + STATION_GRID_CARD_HEIGHT < visibleTop ||
                    cardTop > visibleBottom)
                {
                    continue;
                }
                CreateStationGridCard(group, position);
            }
        }
    }

    void UpdateStationScanBanner()
    {
        if (g_stationView.scanBanner == NULL || g_stationView.snapshot == NULL)
        {
            return;
        }
        const StationScanState& snapshot = *g_stationView.snapshot;
        const size_t candidateCount = StationScanCandidateCount(snapshot);
        const bool scanFault = snapshot.ownershipCopyTruncated ||
            snapshot.ownershipResolutionIncomplete ||
            snapshot.rosterIncomplete || snapshot.targetsFailed > 0 ||
            !snapshot.errors.empty();
        std::ostringstream caption;
        int themeRole = 0; // 0 success, 1 accent, 2 warning.
        if (snapshot.truncated)
        {
            caption << "PLAYER STATION RESULT LIST TRUNCATED AT 2,048 - RESULTS INCOMPLETE";
            caption << "  |  " << snapshot.targetsFailed
                    << " target(s) failed";
            themeRole = 2;
        }
        else if (!snapshot.complete)
        {
            caption << "PARTIAL SCAN - RESULTS INCOMPLETE  |  "
                    << snapshot.targetsCompleted << " of " << candidateCount
                    << " candidates read";
            caption << "  |  " << snapshot.targetsFailed
                    << " target(s) failed";
            themeRole = scanFault ? 2 : 1;
        }
        else if (scanFault)
        {
            caption << "PARTIAL SCAN - SOME STATION OR ROSTER DATA IS UNAVAILABLE";
            if (snapshot.targetsFailed > 0)
            {
                caption << "  |  " << snapshot.targetsFailed
                        << " target(s) failed";
            }
            themeRole = 2;
        }
        else
        {
            size_t visibleCount = 0;
            for (size_t index = 0; index < g_stationView.groups.size(); ++index)
            {
                visibleCount += g_stationView.groups[index].stations.size();
            }
            caption << visibleCount << " PLAYER STATIONS";
        }
        g_stationView.scanBanner->setCaption(caption.str());
        if (themeRole == 2)
        {
            TagThemeWarningText(g_stationView.scanBanner);
        }
        else if (themeRole == 1)
        {
            TagThemeAccentText(g_stationView.scanBanner);
        }
        else
        {
            TagThemeSuccessText(g_stationView.scanBanner);
        }
        std::ostringstream errors;
        for (size_t index = 0; index < snapshot.errors.size(); ++index)
        {
            if (index != 0)
            {
                errors << '\n';
            }
            errors << snapshot.errors[index];
        }
        g_stationView.scanBanner->setUserString("KJM_ToolTip", errors.str());
        g_stationView.scanBanner->setNeedToolTip(!snapshot.errors.empty());
        if (g_stationView.progressTrack != NULL &&
            g_stationView.progressFill != NULL)
        {
            const int width = g_stationView.progressTrack->getWidth();
            int filled = width;
            if (!snapshot.complete && candidateCount > 0)
            {
                filled = static_cast<int>(
                    static_cast<double>(snapshot.targetsCompleted) /
                    static_cast<double>(candidateCount) * width);
            }
            g_stationView.progressFill->setSize(
                ClampInt(filled, 0, width),
                g_stationView.progressFill->getHeight());
            g_stationView.progressTrack->setVisible(!snapshot.complete);
        }
    }

    int StationMemberRelevantSkill(
        const StationMemberSnapshot& member,
        const StationTargetSnapshot& station,
        bool* knownOut)
    {
        *knownOut = false;
        if (!station.relevantSkillKnown)
        {
            return 0;
        }
        for (size_t index = 0; index < member.baseStats.size(); ++index)
        {
            if (member.baseStats[index].stat == station.relevantStat)
            {
                *knownOut = true;
                return member.baseStats[index].displayValue;
            }
        }
        return 0;
    }

    struct StationDetailPersonLess
    {
        bool lowestSkillFirst;

        explicit StationDetailPersonLess(bool lowToHigh = false) :
            lowestSkillFirst(lowToHigh) {}

        bool operator()(
            const StationDetailPerson& left,
            const StationDetailPerson& right) const
        {
            if (left.relevantSkillKnown != right.relevantSkillKnown)
            {
                return left.relevantSkillKnown;
            }
            if (left.relevantSkillKnown &&
                left.relevantSkill != right.relevantSkill)
            {
                return lowestSkillFirst ?
                    left.relevantSkill < right.relevantSkill :
                    left.relevantSkill > right.relevantSkill;
            }
            if (left.totalJobs != right.totalJobs)
            {
                return left.totalJobs < right.totalJobs;
            }
            return StationLower(left.name) < StationLower(right.name);
        }
    };

    void CreateStationDetailPortrait(
        MyGUI::Button* row,
        const HandleIdentity& identity)
    {
        const StationMemberSnapshot* member =
            FindStationMemberSnapshot(identity);
        if (row == NULL || member == NULL)
        {
            return;
        }
        const int size = 36;
        const int inset = 2;
        MyGUI::Button* border = row->createWidget<MyGUI::Button>(
            "Kenshi_PortraitFrameSkin", MyGUI::IntCoord(4, 4, size, size),
            MyGUI::Align::Left | MyGUI::Align::VCenter,
            "KJM_StationDetailPortraitFrame");
        border->setNeedMouseFocus(false);
        border->setNeedKeyFocus(false);
        MyGUI::ImageBox* background = border->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(inset, inset,
                size - inset * 2, size - inset * 2), MyGUI::Align::Stretch,
            "KJM_StationDetailPortraitBackground");
        MyGUI::ImageBox* portrait = border->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(inset, inset,
                size - inset * 2, size - inset * 2), MyGUI::Align::Stretch,
            "KJM_StationDetailPortrait");
        MyGUI::ImageBox* back = border->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(inset, inset,
                size - inset * 2, size - inset * 2), MyGUI::Align::Stretch,
            "KJM_StationDetailPortraitBackOverlay");
        MyGUI::ImageBox* front = border->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(inset, inset,
                size - inset * 2, size - inset * 2), MyGUI::Align::Stretch,
            "KJM_StationDetailPortraitFrontOverlay");
        background->setDepth(5);
        back->setDepth(4);
        portrait->setDepth(3);
        front->setDepth(2);
        background->setNeedMouseFocus(false);
        portrait->setNeedMouseFocus(false);
        back->setNeedMouseFocus(false);
        front->setNeedMouseFocus(false);
        background->setNeedKeyFocus(false);
        portrait->setNeedKeyFocus(false);
        back->setNeedKeyFocus(false);
        front->setNeedKeyFocus(false);
        bool selected = false;
        std::string backgroundName;
        std::string backName;
        std::string frontName;
        if (TryGetPortraitVisuals(
                member->handle, &selected, &backgroundName, &backName,
                &frontName))
        {
            border->setStateSelected(selected);
            SetPortraitImage(background, "Background", backgroundName);
            SetPortraitImage(back, "BackOverlay", backName);
            SetPortraitImage(front, "FrontOverlay", frontName);
        }
        portrait->setVisible(true);
        if (!TryBindPortrait(member->handle, portrait, false))
        {
            TryBindPortrait(member->handle, portrait, true);
        }
    }

    void BuildStationDetailPeople(const StationTargetSnapshot& station)
    {
        g_stationView.assignedPeople.clear();
        g_stationView.addPeople.clear();
        std::vector<HandleIdentity> assigned;
        for (size_t index = 0; index < station.assignments.size(); ++index)
        {
            const StationAssignmentSnapshot& assignment =
                station.assignments[index];
            if (StationIdentityIn(assigned, assignment.member))
            {
                continue;
            }
            assigned.push_back(assignment.member);
            StationDetailPerson person;
            person.identity = assignment.member;
            person.name = assignment.memberName;
            person.relevantSkillKnown = assignment.relevantSkillKnown;
            person.relevantSkill = assignment.relevantSkillValue;
            const StationMemberSnapshot* member =
                FindStationMemberSnapshot(person.identity);
            if (member != NULL)
            {
                if (person.name.empty())
                {
                    person.name = member->name;
                }
                person.totalJobs = member->permanentJobCount;
                if (!person.relevantSkillKnown)
                {
                    person.relevantSkill = StationMemberRelevantSkill(
                        *member, station, &person.relevantSkillKnown);
                }
            }
            if (person.name.empty())
            {
                person.name = "Unknown person";
            }
            g_stationView.assignedPeople.push_back(person);
        }

        if (!station.assignmentSupported)
        {
            std::stable_sort(
                g_stationView.assignedPeople.begin(),
                g_stationView.assignedPeople.end(),
                StationDetailPersonLess(
                    station.category == STATION_TRAINING));
            return;
        }

        // The detail view groups by member identity, not by queue row.  Thus
        // OPERATE_STORAGE hauling is naturally hidden when the same member
        // also has a non-hauling exact-target job, as required; neither job
        // creates a duplicate person row.
        for (size_t squadIndex = 0;
             squadIndex < g_stationView.snapshot->squads.size(); ++squadIndex)
        {
            const StationSquadSnapshot& squad =
                g_stationView.snapshot->squads[squadIndex];
            if (!squad.loaded)
            {
                continue;
            }
            for (size_t memberIndex = 0;
                 memberIndex < squad.members.size(); ++memberIndex)
            {
                const StationMemberSnapshot& member =
                    squad.members[memberIndex];
                if (!member.loaded || !member.queueAvailable ||
                    member.truncated ||
                    StationIdentityIn(assigned, member.identity))
                {
                    continue;
                }
                StationDetailPerson person;
                person.identity = member.identity;
                person.name = member.name;
                person.totalJobs = member.permanentJobCount;
                person.relevantSkill = StationMemberRelevantSkill(
                    member, station, &person.relevantSkillKnown);
                g_stationView.addPeople.push_back(person);
            }
        }
        std::stable_sort(
            g_stationView.assignedPeople.begin(),
            g_stationView.assignedPeople.end(),
            StationDetailPersonLess(station.category == STATION_TRAINING));
        std::stable_sort(
            g_stationView.addPeople.begin(),
            g_stationView.addPeople.end(),
            StationDetailPersonLess(station.category == STATION_TRAINING));
    }

    void SetStationDetailPersonCaption(
        MyGUI::Button* row,
        const StationDetailPerson& person,
        const StationTargetSnapshot& station,
        bool addCandidate)
    {
        std::ostringstream caption;
        caption << person.name << "  |  ";
        if (!station.relevantSkillName.empty())
        {
            caption << station.relevantSkillName << ' ';
        }
        if (person.relevantSkillKnown)
        {
            caption << person.relevantSkill;
        }
        else
        {
            caption << "unknown";
        }
        caption << "  |  " << person.totalJobs << " total job";
        if (person.totalJobs != 1)
        {
            caption << 's';
        }
        // A run of spaces in a proportional font did not reserve a reliable
        // portrait column. Keep the button caption empty and render text in a
        // separate child that begins after the fixed 36-pixel portrait.
        row->setCaption("");
        TagThemeButtonText(row);
        MyGUI::Widget* rowSurface = row->createWidget<MyGUI::Widget>(
            "WhiteSkin",
            MyGUI::IntCoord(2, 2,
                std::max(1, row->getWidth() - 4),
                std::max(1, row->getHeight() - 4)),
            MyGUI::Align::Stretch,
            addCandidate ? "KJM_StationAvailablePersonSurface" :
                "KJM_StationAssignedPersonSurface");
        TagThemeSurface(rowSurface);
        MyGUI::TextBox* label = row->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            MyGUI::IntCoord(
                48, 0, std::max(20, row->getWidth() - 54), row->getHeight()),
            MyGUI::Align::Stretch,
            addCandidate ? "KJM_StationAvailablePersonText" :
                "KJM_StationAssignedPersonText");
        SetFittedStationSingleLine(label, caption.str(), 16, 13);
        label->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        TagThemeStandardText(label);
        label->setColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
        label->setAlpha(1.0f);
        label->setInheritsAlpha(false);
        label->setNeedMouseFocus(false);
        label->setNeedKeyFocus(false);
        if (StationIdentityIn(
                g_stationView.recentMembers, person.identity))
        {
            row->setColour(MyGUI::Colour(0.48f, 0.82f, 1.0f));
        }
        CreateStationDetailPortrait(row, person.identity);
        row->setUserString(
            "KJM_StationMemberIdentity",
            StationIdentityString(person.identity));
        if (addCandidate)
        {
            row->eventMouseWheel +=
                MyGUI::newDelegate(OnStationAvailableWheel);
            row->eventMouseButtonPressed +=
                MyGUI::newDelegate(OnStationAddPressed);
            row->eventKeyButtonPressed +=
                MyGUI::newDelegate(OnStationAddKey);
            AttachStationTooltip(row,
                caption.str() +
                "\n\nClick or press Enter to request assignment. Scrolling never assigns.");
        }
        else
        {
            row->eventMouseWheel +=
                MyGUI::newDelegate(OnStationDetailWheel);
            row->eventMouseButtonPressed +=
                MyGUI::newDelegate(OnStationAssignedPressed);
            AttachStationTooltip(row,
                caption.str() +
                "\n\nRight-click to request removal from this station.");
        }
    }

    void UpdateStationDetailPaneScrollRange(
        MyGUI::Widget* viewport,
        MyGUI::ScrollBar* scroll,
        int contentHeight,
        int* offset,
        bool* changing)
    {
        if (viewport == NULL || scroll == NULL || offset == NULL ||
            changing == NULL)
        {
            return;
        }
        const int maxOffset = std::max(
            0, contentHeight - viewport->getHeight());
        *offset = ClampInt(*offset, 0, maxOffset);
        *changing = true;
        scroll->setScrollRange(
            static_cast<size_t>(maxOffset + 1));
        scroll->setScrollPage(
            static_cast<size_t>(viewport->getHeight()));
        scroll->setScrollViewPage(
            static_cast<size_t>(STATION_DETAIL_ROW_HEIGHT));
        scroll->setScrollPosition(static_cast<size_t>(*offset));
        scroll->setEnabled(maxOffset > 0);
        *changing = false;
    }

    void ApplyStationDetailPaneOffset(
        MyGUI::Widget* viewport,
        MyGUI::Widget* canvas,
        int contentHeight,
        int offset)
    {
        if (viewport == NULL || canvas == NULL)
        {
            return;
        }
        canvas->setSize(
            viewport->getWidth(),
            std::max(viewport->getHeight(), contentHeight));
        canvas->setPosition(0, -offset);
    }

    void UpdateStationDetailScrollRange()
    {
        UpdateStationDetailPaneScrollRange(
            g_stationView.detailViewport,
            g_stationView.detailScroll,
            g_stationView.detailContentHeight,
            &g_stationView.detailOffset,
            &g_stationView.detailScrollChanging);
    }

    void UpdateStationAvailableScrollRange()
    {
        UpdateStationDetailPaneScrollRange(
            g_stationView.detailAvailableViewport,
            g_stationView.detailAvailableScroll,
            g_stationView.detailAvailableContentHeight,
            &g_stationView.detailAvailableOffset,
            &g_stationView.detailAvailableScrollChanging);
    }

    void ApplyStationDetailScrollOffset()
    {
        ApplyStationDetailPaneOffset(
            g_stationView.detailViewport,
            g_stationView.detailCanvas,
            g_stationView.detailContentHeight,
            g_stationView.detailOffset);
    }

    void ApplyStationAvailableScrollOffset()
    {
        ApplyStationDetailPaneOffset(
            g_stationView.detailAvailableViewport,
            g_stationView.detailAvailableCanvas,
            g_stationView.detailAvailableContentHeight,
            g_stationView.detailAvailableOffset);
    }

    void RefreshStationDetail()
    {
        g_stationView.detailRefreshRequested = false;
        MyGUI::InputManager* detailInput =
            MyGUI::InputManager::getInstancePtr();
        if (detailInput != NULL && g_stationView.modalPanel != NULL)
        {
            try
            {
                // A successful add/remove destroys and recreates person rows.
                // Move focus off the old row before its widget is destroyed.
                detailInput->setKeyFocusWidget(g_stationView.modalPanel);
            }
            catch (...)
            {
            }
        }
        DestroyStationWidgetList(&g_stationView.detailWidgets);
        if (g_stationView.modalPanel == NULL ||
            g_stationView.detailCanvas == NULL ||
            g_stationView.detailAvailableCanvas == NULL)
        {
            return;
        }
        const StationTargetSnapshot* station =
            FindStationSnapshot(g_stationView.detailStation, NULL);
        if (station == NULL)
        {
            // The previous grid refresh may have honored the modal's frozen
            // card order. Close now, then request one normal-order rebuild on
            // the next safe update.
            g_stationView.fullRefreshRequested = true;
            CloseStationDetail();
            return;
        }
        BuildStationDetailPeople(*station);
        if (g_stationView.detailAssignedHeader != NULL)
        {
            std::ostringstream assignedCaption;
            assignedCaption << "ASSIGNED WORKERS ("
                            << g_stationView.assignedPeople.size() << ")";
            g_stationView.detailAssignedHeader->setCaption(
                assignedCaption.str());
            ApplyThemeStandardText(g_stationView.detailAssignedHeader);
        }
        if (g_stationView.detailAvailableHeader != NULL)
        {
            std::ostringstream availableCaption;
            availableCaption << "AVAILABLE WORKERS ("
                             << g_stationView.addPeople.size() << ")";
            g_stationView.detailAvailableHeader->setCaption(
                availableCaption.str());
            ApplyThemeStandardText(g_stationView.detailAvailableHeader);
        }

        int assignedTop = 4;
        if (g_stationView.assignedPeople.empty())
        {
            MyGUI::TextBox* empty =
                g_stationView.detailCanvas->createWidget<MyGUI::TextBox>(
                    "Kenshi_TextboxStandardText_Small",
                    MyGUI::IntCoord(8, assignedTop,
                        g_stationView.detailCanvas->getWidth() - 16,
                        35), MyGUI::Align::Top | MyGUI::Align::HStretch,
                    "KJM_StationNoAssignedPeople");
            empty->setCaption("UNASSIGNED");
            TagThemeAccentText(empty);
            empty->setNeedMouseFocus(false);
            empty->setNeedKeyFocus(false);
            g_stationView.detailWidgets.push_back(empty);
            assignedTop += 37;
        }
        else
        {
            for (size_t index = 0;
                 index < g_stationView.assignedPeople.size(); ++index)
            {
                MyGUI::Button* row =
                    g_stationView.detailCanvas->createWidget<MyGUI::Button>(
                        "Kenshi_Button1",
                        MyGUI::IntCoord(4, assignedTop,
                            g_stationView.detailCanvas->getWidth() - 8,
                            STATION_DETAIL_ROW_HEIGHT - 4),
                        MyGUI::Align::Top | MyGUI::Align::HStretch,
                        "KJM_StationAssignedPerson");
                SetStationDetailPersonCaption(
                    row, g_stationView.assignedPeople[index], *station, false);
                g_stationView.detailWidgets.push_back(row);
                assignedTop += STATION_DETAIL_ROW_HEIGHT;
            }
        }
        g_stationView.detailContentHeight = assignedTop + 6;

        int availableTop = 4;
        if (!station->assignmentSupported)
        {
            MyGUI::TextBox* unsupported =
                g_stationView.detailAvailableCanvas->createWidget<MyGUI::TextBox>(
                    "Kenshi_TextboxStandardText_Small",
                    MyGUI::IntCoord(8, availableTop,
                        g_stationView.detailAvailableCanvas->getWidth() - 16,
                        52),
                    MyGUI::Align::Top | MyGUI::Align::HStretch,
                    "KJM_StationAssignmentUnsupported");
            unsupported->setCaption(
                "This station does not expose a verified permanent-job mapping.");
            TagThemeWarningText(unsupported);
            unsupported->setNeedMouseFocus(false);
            unsupported->setNeedKeyFocus(false);
            g_stationView.detailWidgets.push_back(unsupported);
            availableTop += 54;
        }
        else if (g_stationView.addPeople.empty())
        {
            MyGUI::TextBox* none =
                g_stationView.detailAvailableCanvas->createWidget<MyGUI::TextBox>(
                    "Kenshi_TextboxStandardText_Small",
                    MyGUI::IntCoord(8, availableTop,
                        g_stationView.detailAvailableCanvas->getWidth() - 16,
                        34),
                    MyGUI::Align::Top | MyGUI::Align::HStretch,
                    "KJM_StationNoAddCandidates");
            none->setCaption("No loaded readable unassigned characters.");
            TagThemeStandardText(none);
            none->setNeedMouseFocus(false);
            none->setNeedKeyFocus(false);
            g_stationView.detailWidgets.push_back(none);
            availableTop += 36;
        }
        else
        {
            for (size_t index = 0;
                 index < g_stationView.addPeople.size(); ++index)
            {
                MyGUI::Button* row =
                    g_stationView.detailAvailableCanvas->createWidget<MyGUI::Button>(
                        "Kenshi_Button1",
                        MyGUI::IntCoord(4, availableTop,
                            g_stationView.detailAvailableCanvas->getWidth() - 8,
                            STATION_DETAIL_ROW_HEIGHT - 4),
                        MyGUI::Align::Top | MyGUI::Align::HStretch,
                        "KJM_StationAddCandidate");
                SetStationDetailPersonCaption(
                    row, g_stationView.addPeople[index], *station, true);
                g_stationView.detailWidgets.push_back(row);
                availableTop += STATION_DETAIL_ROW_HEIGHT;
            }
        }
        g_stationView.detailAvailableContentHeight = availableTop + 6;
        UpdateStationDetailScrollRange();
        UpdateStationAvailableScrollRange();
        ApplyStationDetailScrollOffset();
        ApplyStationAvailableScrollOffset();
    }

    void FreezeStationCardOrder()
    {
        g_stationView.frozenCardOrder.clear();
        if (g_stationView.snapshot == NULL)
        {
            return;
        }
        for (size_t groupIndex = 0;
             groupIndex < g_stationView.groups.size(); ++groupIndex)
        {
            const StationGridGroup& group = g_stationView.groups[groupIndex];
            for (size_t index = 0; index < group.stations.size(); ++index)
            {
                g_stationView.frozenCardOrder.push_back(
                    g_stationView.snapshot->stations[
                        group.stations[index]].identity);
            }
        }
    }

    void OpenStationDetailNow(const HandleIdentity& identity)
    {
        if (g_stationView.root == NULL ||
            FindStationSnapshot(identity, NULL) == NULL)
        {
            return;
        }
        CloseStationDetail();
        if (g_tooltip != NULL)
        {
            g_tooltip->setVisible(false);
        }
        g_stationView.detailStation = identity;
        FreezeStationCardOrder();
        const StationTargetSnapshot* station =
            FindStationSnapshot(identity, NULL);
        if (station == NULL)
        {
            CloseStationDetail();
            return;
        }
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
        {
            CloseStationDetail();
            return;
        }
        const int rootWidth = g_stationView.root->getWidth();
        const int rootHeight = g_stationView.root->getHeight();
        const MyGUI::IntSize view =
            MyGUI::RenderManager::getInstance().getViewSize();
        const int panelWidth = std::min(
            1120,
            std::max(
                480, std::min(rootWidth - 40, view.width - 24)));
        const int panelHeight = std::min(
            720,
            std::max(
                350, std::min(rootHeight - 50, view.height - 24)));
        const int left = (view.width - panelWidth) / 2;
        const int top = (view.height - panelHeight) / 2;
        // MyGUI modal widgets must be roots. The manager itself is already a
        // root modal, so create the detail shade and panel on the Popup layer
        // just like the confirmation/options modals. Registering the former
        // child panel as modal raised "Modal widget must be root" and the
        // guarded failure path correctly closed the whole manager.
        g_stationView.modalShade =
            gui->createWidget<MyGUI::Widget>(
                "WhiteSkin", MyGUI::IntCoord(0, 0, view.width, view.height),
                MyGUI::Align::Stretch, "Popup",
                "KJM_StationDetailShade");
        g_stationView.modalShade->setColour(MyGUI::Colour(0.0f, 0.0f, 0.0f));
        g_stationView.modalShade->setAlpha(0.63f);
        g_stationView.modalShade->setNeedMouseFocus(true);
        g_stationView.modalPanel =
            gui->createWidget<MyGUI::Widget>(
                "Kenshi_SelectionPanel",
                MyGUI::IntCoord(left, top, panelWidth, panelHeight),
                MyGUI::Align::Center, "Popup",
                "KJM_StationDetailModal");
        // Kenshi_SelectionPanel uses a translucent native texture. Give the
        // detail popup the same fully opaque interior as the main manager so
        // the world and station grid cannot reduce text contrast. Keep this
        // as the first child so every detail control renders above it.
        MyGUI::Widget* modalBackground =
            g_stationView.modalPanel->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(4, 4, panelWidth - 8, panelHeight - 8),
                MyGUI::Align::Stretch,
                "KJM_StationDetailBackground");
        TagThemeBackground(modalBackground);
        modalBackground->setAlpha(1.0f);
        modalBackground->setDepth(100);
        modalBackground->setNeedMouseFocus(false);

        MyGUI::TextBox* title =
            g_stationView.modalPanel->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(14, 8, panelWidth - 70, 34),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationDetailTitle");
        title->setCaption(station->name);
        title->setFontHeight(23);
        TagThemeStandardText(title);
        while (title->getFontHeight() > 15 &&
            title->getTextSize().width > panelWidth - 70)
        {
            title->setFontHeight(title->getFontHeight() - 1);
        }
        title->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        AttachStationTooltip(title, station->name);
        MyGUI::Button* close =
            g_stationView.modalPanel->createWidget<MyGUI::Button>(
                "Kenshi_Button1", MyGUI::IntCoord(panelWidth - 48, 7, 38, 32),
                MyGUI::Align::Right | MyGUI::Align::Top,
                "KJM_StationDetailClose");
        close->setCaption("X");
        TagThemeButtonText(close);
        close->eventMouseButtonClick +=
            MyGUI::newDelegate(OnStationDetailClose);

        MyGUI::TextBox* summary =
            g_stationView.modalPanel->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(14, 43, panelWidth - 28, 66),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationDetailSummary");
        TagThemeStandardText(summary);
        std::ostringstream summaryCaption;
        summaryCaption << GetStationCategoryName(station->category)
                       << "  |  Relevant skill: "
                       << (station->relevantSkillName.empty() ? "None" :
                           station->relevantSkillName);
        if (!station->areaName.empty())
        {
            summaryCaption << "\nArea: " << station->areaName;
        }
        if (!station->blockingStatus.empty())
        {
            summaryCaption << "\nCannot work: " << station->blockingStatus;
            TagThemeWarningText(summary);
        }
        else if (!station->blockingStatusKnown)
        {
            summaryCaption << "\nWork status: Unavailable";
        }
        if (!station->assignmentSupported)
        {
            summaryCaption << "\nAssignment: Not supported for this station";
        }
        summary->setCaption(summaryCaption.str());
        summary->setFontHeight(14);
        AttachStationTooltip(summary, summaryCaption.str());

        const int listTop = 114;
        const int statusHeight = 30;
        const int listHeight = panelHeight - listTop - statusHeight - 12;
        const int detailHeaderHeight = 32;
        const int detailPaneGap = 14;
        const int detailScrollWidth = 20;
        const int detailPaneWidth =
            (panelWidth - 24 - detailPaneGap) / 2;
        const int detailViewportWidth =
            detailPaneWidth - detailScrollWidth - 3;
        const int detailListTop = listTop + detailHeaderHeight;
        const int detailViewportHeight = listHeight - detailHeaderHeight;
        const int assignedLeft = 12;
        const int availableLeft =
            assignedLeft + detailPaneWidth + detailPaneGap;

        g_stationView.detailAssignedHeader =
            g_stationView.modalPanel->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(
                    assignedLeft + 4, listTop, detailPaneWidth - 8,
                    detailHeaderHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationAssignedHeader");
        g_stationView.detailAssignedHeader->setFontHeight(17);
        g_stationView.detailAssignedHeader->setTextAlign(
            MyGUI::Align::Left | MyGUI::Align::VCenter);
        TagThemeStandardText(g_stationView.detailAssignedHeader);
        g_stationView.detailAssignedHeader->setNeedMouseFocus(false);
        g_stationView.detailAssignedHeader->setNeedKeyFocus(false);

        g_stationView.detailAvailableHeader =
            g_stationView.modalPanel->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(
                    availableLeft + 4, listTop, detailPaneWidth - 8,
                    detailHeaderHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationAvailableHeader");
        g_stationView.detailAvailableHeader->setFontHeight(17);
        g_stationView.detailAvailableHeader->setTextAlign(
            MyGUI::Align::Left | MyGUI::Align::VCenter);
        TagThemeStandardText(g_stationView.detailAvailableHeader);
        g_stationView.detailAvailableHeader->setNeedMouseFocus(false);
        g_stationView.detailAvailableHeader->setNeedKeyFocus(false);

        MyGUI::Widget* detailDivider =
            g_stationView.modalPanel->createWidget<MyGUI::Widget>(
                "WhiteSkin",
                MyGUI::IntCoord(
                    assignedLeft + detailPaneWidth + detailPaneGap / 2,
                    listTop, 1, listHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationDetailDivider");
        detailDivider->setColour(MyGUI::Colour(0.72f, 0.72f, 0.72f));
        detailDivider->setAlpha(0.33f);
        detailDivider->setNeedMouseFocus(false);

        g_stationView.detailViewport =
            g_stationView.modalPanel->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(
                    assignedLeft, detailListTop,
                    detailViewportWidth, detailViewportHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationDetailViewport");
        g_stationView.detailViewport->eventMouseWheel +=
            MyGUI::newDelegate(OnStationDetailWheel);
        g_stationView.detailCanvas =
            g_stationView.detailViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(0, 0,
                    g_stationView.detailViewport->getWidth(),
                    detailViewportHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationDetailCanvas");
        g_stationView.detailCanvas->eventMouseWheel +=
            MyGUI::newDelegate(OnStationDetailWheel);
        g_stationView.detailScroll =
            g_stationView.modalPanel->createWidget<MyGUI::ScrollBar>(
                "Kenshi_ScrollBarV",
                MyGUI::IntCoord(
                    assignedLeft + detailViewportWidth + 2,
                    detailListTop, detailScrollWidth, detailViewportHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationDetailScroll");
        g_stationView.detailScroll->eventScrollChangePosition +=
            MyGUI::newDelegate(OnStationDetailScroll);

        g_stationView.detailAvailableViewport =
            g_stationView.modalPanel->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(
                    availableLeft, detailListTop,
                    detailViewportWidth, detailViewportHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationAvailableViewport");
        g_stationView.detailAvailableViewport->eventMouseWheel +=
            MyGUI::newDelegate(OnStationAvailableWheel);
        g_stationView.detailAvailableCanvas =
            g_stationView.detailAvailableViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(0, 0,
                    g_stationView.detailAvailableViewport->getWidth(),
                    detailViewportHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationAvailableCanvas");
        g_stationView.detailAvailableCanvas->eventMouseWheel +=
            MyGUI::newDelegate(OnStationAvailableWheel);
        g_stationView.detailAvailableScroll =
            g_stationView.modalPanel->createWidget<MyGUI::ScrollBar>(
                "Kenshi_ScrollBarV",
                MyGUI::IntCoord(
                    availableLeft + detailViewportWidth + 2,
                    detailListTop, detailScrollWidth, detailViewportHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationAvailableScroll");
        g_stationView.detailAvailableScroll->eventScrollChangePosition +=
            MyGUI::newDelegate(OnStationAvailableScroll);
        g_stationView.detailStatus =
            g_stationView.modalPanel->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(12, panelHeight - statusHeight - 6,
                    panelWidth - 24, statusHeight),
                MyGUI::Align::Bottom | MyGUI::Align::HStretch,
                "KJM_StationDetailStatus");
        g_stationView.detailStatus->setTextAlign(
            MyGUI::Align::Center | MyGUI::Align::VCenter);
        g_stationView.detailStatus->setFontHeight(13);
        TagThemeAccentText(g_stationView.detailStatus);
        g_stationView.detailStatus->setNeedMouseFocus(false);
        g_stationView.detailStatusText.clear();
        g_stationView.detailStatus->setCaption("");
        g_stationView.detailOffset = 0;
        g_stationView.detailContentHeight = 0;
        g_stationView.detailAvailableOffset = 0;
        g_stationView.detailAvailableContentHeight = 0;
        MyGUI::InputManager* input =
            MyGUI::InputManager::getInstancePtr();
        if (input != NULL)
        {
            input->addWidgetModal(g_stationView.modalPanel);
            g_stationView.detailModalAdded = true;
            input->setKeyFocusWidget(g_stationView.modalPanel);
        }
        RefreshStationDetail();
    }

    bool IsStationDetailOpen()
    {
        return g_stationView.modalPanel != NULL &&
            g_stationView.detailStation.valid;
    }

    void CloseStationDetail()
    {
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        try
        {
            DestroyStationWidgetList(&g_stationView.detailWidgets);
        }
        catch (...)
        {
            // The parent panel still owns any rows not destroyed before the
            // exception. Clear the non-owning list so no later close retries a
            // child pointer that MyGUI may already have invalidated.
            g_stationView.detailWidgets.clear();
            ErrorLog("[KenshiJobManagement] MyGUI could not destroy every station-detail row cleanly.");
        }
        if (input != NULL && g_stationView.detailModalAdded &&
            g_stationView.modalPanel != NULL)
        {
            try
            {
                input->removeWidgetModal(g_stationView.modalPanel);
            }
            catch (...)
            {
                ErrorLog("[KenshiJobManagement] MyGUI could not remove the station-detail modal cleanly.");
            }
        }
        if (gui != NULL && g_stationView.modalPanel != NULL)
        {
            try
            {
                gui->destroyWidget(g_stationView.modalPanel);
            }
            catch (...)
            {
                ErrorLog("[KenshiJobManagement] MyGUI could not destroy the station-detail panel cleanly.");
            }
        }
        if (gui != NULL && g_stationView.modalShade != NULL)
        {
            try
            {
                gui->destroyWidget(g_stationView.modalShade);
            }
            catch (...)
            {
                ErrorLog("[KenshiJobManagement] MyGUI could not destroy the station-detail shade cleanly.");
            }
        }
        g_stationView.detailWidgets.clear();
        g_stationView.modalShade = NULL;
        g_stationView.modalPanel = NULL;
        g_stationView.detailAssignedHeader = NULL;
        g_stationView.detailAvailableHeader = NULL;
        g_stationView.detailViewport = NULL;
        g_stationView.detailCanvas = NULL;
        g_stationView.detailScroll = NULL;
        g_stationView.detailAvailableViewport = NULL;
        g_stationView.detailAvailableCanvas = NULL;
        g_stationView.detailAvailableScroll = NULL;
        g_stationView.detailStatus = NULL;
        g_stationView.detailOffset = 0;
        g_stationView.detailContentHeight = 0;
        g_stationView.detailScrollChanging = false;
        g_stationView.detailAvailableOffset = 0;
        g_stationView.detailAvailableContentHeight = 0;
        g_stationView.detailAvailableScrollChanging = false;
        g_stationView.detailRefreshRequested = false;
        g_stationView.detailCloseRequested = false;
        g_stationView.detailModalAdded = false;
        g_stationView.frozenCardOrder.clear();
        g_stationView.recentMembers.clear();
        ResetHandleIdentity(&g_stationView.recentStation);
        ResetHandleIdentity(&g_stationView.detailStation);
        if (input != NULL && g_window != NULL)
        {
            try
            {
                input->setKeyFocusWidget(g_window);
            }
            catch (...)
            {
                ErrorLog("[KenshiJobManagement] MyGUI could not restore manager focus after closing station detail.");
            }
        }
    }

    void SetStationDetailStatus(const std::string& text)
    {
        g_stationView.detailStatusText = text;
        if (g_stationView.detailStatus != NULL)
        {
            g_stationView.detailStatus->setCaption(text);
        }
    }

    void MarkStationDetailChange(
        const HandleIdentity& station,
        const HandleIdentity& member)
    {
        if (!station.valid || !member.valid)
        {
            return;
        }
        g_stationView.recentStation = station;
        if (!StationIdentityIn(g_stationView.recentMembers, member))
        {
            g_stationView.recentMembers.push_back(member);
        }
        if (SameHandleIdentity(g_stationView.detailStation, station))
        {
            g_stationView.detailRefreshRequested = true;
        }
    }

    void RefreshStationView()
    {
        if (g_stationView.root == NULL)
        {
            return;
        }
        HandleIdentity anchor;
        int anchorDelta = 0;
        const bool haveAnchor = CaptureStationScrollAnchor(
            &anchor, &anchorDelta);
        BuildStationGroups();
        if (haveAnchor)
        {
            int anchorTop = 0;
            if (FindStationCardTop(anchor, &anchorTop))
            {
                g_stationView.verticalOffset = anchorTop + anchorDelta;
            }
        }
        UpdateStationScanBanner();
        UpdateStationScrollRange();
        size_t visibleCount = 0;
        for (size_t index = 0; index < g_stationView.groups.size(); ++index)
        {
            visibleCount += g_stationView.groups[index].stations.size();
        }
        if (g_stationView.emptyText != NULL)
        {
            const bool empty = g_stationView.snapshot != NULL &&
                visibleCount == 0;
            g_stationView.emptyText->setVisible(empty);
            if (empty)
            {
                g_stationView.emptyText->setCaption(
                    g_stationView.snapshot->complete ?
                    "No stations match the current category filters." :
                    "Partial scan in progress. No matching stations are available yet.");
            }
        }
        RefreshStationVirtualWidgets();
        if (IsStationDetailOpen())
        {
            g_stationView.detailRefreshRequested = true;
        }
    }

    void SetStationBoardSnapshot(const StationScanState* snapshot)
    {
        g_stationView.snapshot = snapshot;
        if (IsStationDetailOpen() &&
            FindStationSnapshot(g_stationView.detailStation, NULL) == NULL)
        {
            CloseStationDetail();
        }
        RefreshStationView();
    }

    void SetStationViewVisible(bool visible)
    {
        if (!visible)
        {
            CloseStationDetail();
        }
        g_stationView.visible = visible;
        if (g_stationView.root != NULL)
        {
            g_stationView.root->setVisible(visible);
        }
        if (visible)
        {
            RefreshStationView();
        }
        else
        {
            ClearStationCardHover();
            g_stationView.visibleCards.clear();
            g_stationView.visibleHeaders.clear();
            DestroyStationWidgetList(&g_stationView.virtualWidgets);
        }
    }

    void SwitchStationView(bool visible)
    {
        SetStationViewVisible(visible);
    }

    void TickStationView(
        const StationScanState* snapshot,
        bool snapshotChanged,
        bool scanProgressChanged)
    {
        if (snapshot != g_stationView.snapshot)
        {
            g_stationView.snapshot = snapshot;
            snapshotChanged = true;
        }
        if (!g_stationView.visible)
        {
            return;
        }
        if (g_stationView.detailCloseRequested)
        {
            g_stationView.detailCloseRequested = false;
            CloseStationDetail();
            RefreshStationView();
        }
        if (g_stationView.pendingDetail.valid)
        {
            const HandleIdentity requested = g_stationView.pendingDetail;
            ResetHandleIdentity(&g_stationView.pendingDetail);
            OpenStationDetailNow(requested);
        }
        if (snapshotChanged || g_stationView.fullRefreshRequested)
        {
            g_stationView.fullRefreshRequested = false;
            RefreshStationView();
        }
        else
        {
            if (scanProgressChanged)
            {
                // Candidate progress and validation warnings do not change
                // card groups. Update only the banner and progress bar so a
                // presentation boundary does not destroy visible widgets.
                UpdateStationScanBanner();
            }
            if (g_stationView.virtualRefreshRequested)
            {
                RefreshStationVirtualWidgets();
            }
        }
        if (g_stationView.detailRefreshRequested)
        {
            RefreshStationDetail();
        }
    }

    void CreateStationView(MyGUI::Widget* client, const MyGUI::IntCoord& bounds)
    {
        if (client == NULL || g_stationView.root != NULL)
        {
            return;
        }
        g_stationView = StationViewState();
        g_stationView.root = client->createWidget<MyGUI::Widget>(
            "PanelEmpty", bounds, MyGUI::Align::Stretch, "KJM_StationsTab");
        g_stationView.root->setVisible(false);
        g_stationView.viewportWidth = std::max(
            STATION_GRID_CARD_WIDTH,
            bounds.width - STATION_GRID_SCROLL_SIZE);
        g_stationView.viewportHeight = std::max(
            STATION_GRID_CARD_HEIGHT,
            bounds.height - STATION_GRID_BANNER_HEIGHT);
        g_stationView.columns = std::max(
            1, (g_stationView.viewportWidth - STATION_GRID_SIDE_PAD * 2 +
                STATION_GRID_GAP) /
                (STATION_GRID_CARD_WIDTH + STATION_GRID_GAP));

        g_stationView.scanBanner =
            g_stationView.root->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(4, 0, bounds.width - 8, 28),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationScanBanner");
        g_stationView.scanBanner->setTextAlign(MyGUI::Align::Center);
        TagThemeSuccessText(g_stationView.scanBanner);
        g_stationView.scanBanner->eventToolTip +=
            MyGUI::newDelegate(OnCardToolTip);
        g_stationView.progressTrack =
            g_stationView.root->createWidget<MyGUI::Widget>(
                "WhiteSkin", MyGUI::IntCoord(4, 29, bounds.width - 8, 5),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationProgressTrack");
        g_stationView.progressTrack->setColour(
            MyGUI::Colour(0.20f, 0.16f, 0.12f));
        g_stationView.progressTrack->setNeedMouseFocus(false);
        g_stationView.progressFill =
            g_stationView.progressTrack->createWidget<MyGUI::Widget>(
                "WhiteSkin", MyGUI::IntCoord(0, 0, 0, 5),
                MyGUI::Align::Left | MyGUI::Align::VStretch,
                "KJM_StationProgressFill");
        g_stationView.progressFill->setColour(
            MyGUI::Colour(0.86f, 0.65f, 0.25f));
        g_stationView.progressFill->setNeedMouseFocus(false);

        g_stationView.viewport =
            g_stationView.root->createWidget<MyGUI::Widget>(
                "PanelEmpty",
                MyGUI::IntCoord(0, STATION_GRID_BANNER_HEIGHT,
                    g_stationView.viewportWidth, g_stationView.viewportHeight),
                MyGUI::Align::Stretch, "KJM_StationGridViewport");
        g_stationView.viewport->eventMouseWheel +=
            MyGUI::newDelegate(OnStationMouseWheel);
        g_stationView.canvas =
            g_stationView.viewport->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(0, 0,
                    g_stationView.viewportWidth, g_stationView.viewportHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationGridCanvas");
        g_stationView.canvas->eventMouseWheel +=
            MyGUI::newDelegate(OnStationMouseWheel);
        g_stationView.verticalScroll =
            g_stationView.root->createWidget<MyGUI::ScrollBar>(
                "Kenshi_ScrollBarV",
                MyGUI::IntCoord(g_stationView.viewportWidth,
                    STATION_GRID_BANNER_HEIGHT, STATION_GRID_SCROLL_SIZE,
                    g_stationView.viewportHeight),
                MyGUI::Align::Right | MyGUI::Align::VStretch,
                "KJM_StationGridScroll");
        g_stationView.verticalScroll->eventScrollChangePosition +=
            MyGUI::newDelegate(OnStationVerticalScroll);
        g_stationView.emptyText =
            g_stationView.root->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(40, STATION_GRID_BANNER_HEIGHT + 50,
                    bounds.width - 80, 80),
                MyGUI::Align::Center, "KJM_StationEmpty");
        g_stationView.emptyText->setTextAlign(MyGUI::Align::Center);
        g_stationView.emptyText->setNeedMouseFocus(false);
        g_stationView.emptyText->setVisible(false);
        TagThemeStandardText(g_stationView.emptyText);
    }

    void DestroyStationView()
    {
        CloseStationDetail();
        ClearStationCardHover();
        g_stationView.visibleCards.clear();
        g_stationView.visibleHeaders.clear();
        DestroyStationWidgetList(&g_stationView.virtualWidgets);
        if (g_stationView.root != NULL)
        {
            MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
            if (gui != NULL)
            {
                gui->destroyWidget(g_stationView.root);
            }
        }
        g_stationView = StationViewState();
    }

    void OnStationCardMouseSetFocus(
        MyGUI::Widget* widget,
        MyGUI::Widget*)
    {
        if (!g_stationView.visible || IsStationDetailOpen())
        {
            ClearStationCardHover();
            return;
        }
        int nextIndex = -1;
        if (!GetStationCardBinding(widget, &nextIndex))
        {
            ClearStationCardHover();
            return;
        }
        if (nextIndex == g_stationView.hoveredCardIndex)
        {
            return;
        }
        ApplyStationCardHoverDelta(
            g_stationView.hoveredCardIndex, nextIndex);
    }

    void OnStationCardMouseLostFocus(
        MyGUI::Widget*,
        MyGUI::Widget* nextWidget)
    {
        if (g_stationView.visible && !IsStationDetailOpen())
        {
            int nextIndex = -1;
            // MyGUI emits lost-focus before set-focus while crossing cards.
            // Resolve the incoming card here so one boundary crossing only
            // toggles the old and new overlays.
            if (GetStationCardBinding(nextWidget, &nextIndex))
            {
                if (nextIndex != g_stationView.hoveredCardIndex)
                {
                    ApplyStationCardHoverDelta(
                        g_stationView.hoveredCardIndex, nextIndex);
                }
                return;
            }
        }
        ClearStationCardHover();
    }

    void OnStationVerticalScroll(MyGUI::ScrollBar*, size_t position)
    {
        if (g_stationView.changingScroll)
        {
            return;
        }
        g_stationView.verticalOffset = ClampInt(
            static_cast<int>(position), 0, g_stationView.maxVerticalOffset);
        g_stationView.virtualRefreshRequested = true;
    }

    void OnStationMouseWheel(MyGUI::Widget*, int relative)
    {
        g_stationView.verticalOffset = ClampInt(
            g_stationView.verticalOffset - relative * 52,
            0, g_stationView.maxVerticalOffset);
        g_stationView.changingScroll = true;
        if (g_stationView.verticalScroll != NULL)
        {
            g_stationView.verticalScroll->setScrollPosition(
                static_cast<size_t>(g_stationView.verticalOffset));
        }
        g_stationView.changingScroll = false;
        g_stationView.virtualRefreshRequested = true;
    }

    void OnStationCategoryClicked(MyGUI::Widget* widget)
    {
        const int value = StationParseIndex(widget, "KJM_StationCategory");
        if (value < static_cast<int>(STATION_CRAFTING) ||
            value > static_cast<int>(STATION_OTHER))
        {
            return;
        }
        const StationCategory category = static_cast<StationCategory>(value);
        if (!SetStationCategoryCollapsed(
                category, !IsStationCategoryCollapsed(category)))
        {
            SetStatus(
                "The category was changed, but settings.ini could not be saved.");
        }
        // Rebuild only after this header callback returns. The current header
        // belongs to the virtual widget tree that the rebuild destroys.
        g_stationView.fullRefreshRequested = true;
    }

    void OnStationCardClicked(MyGUI::Widget* widget)
    {
        HandleIdentity identity;
        if (!ParseStationIdentity(
                widget, "KJM_StationIdentity", &identity))
        {
            return;
        }
        // Defer creation until TickStationView.  The clicked virtual card may
        // be destroyed by modal/card refresh, so it must leave its callback
        // before the widget tree changes.
        g_stationView.pendingDetail = identity;
    }

    void OnStationDetailClose(MyGUI::Widget*)
    {
        // The close button is a child of the modal. Destroy it only after its
        // callback returns to MyGUI.
        g_stationView.detailCloseRequested = true;
    }

    void OnStationDetailScroll(MyGUI::ScrollBar*, size_t position)
    {
        if (g_stationView.detailScrollChanging)
        {
            return;
        }
        g_stationView.detailOffset = static_cast<int>(position);
        ApplyStationDetailScrollOffset();
    }

    void OnStationAvailableScroll(MyGUI::ScrollBar*, size_t position)
    {
        if (g_stationView.detailAvailableScrollChanging)
        {
            return;
        }
        g_stationView.detailAvailableOffset = static_cast<int>(position);
        ApplyStationAvailableScrollOffset();
    }

    void OnStationDetailWheel(MyGUI::Widget*, int relative)
    {
        if (g_stationView.detailViewport == NULL)
        {
            return;
        }
        const int maximum = std::max(
            0, g_stationView.detailContentHeight -
                g_stationView.detailViewport->getHeight());
        g_stationView.detailOffset = ClampInt(
            g_stationView.detailOffset - relative * 46, 0, maximum);
        g_stationView.detailScrollChanging = true;
        if (g_stationView.detailScroll != NULL)
        {
            g_stationView.detailScroll->setScrollPosition(
                static_cast<size_t>(g_stationView.detailOffset));
        }
        g_stationView.detailScrollChanging = false;
        // Wheel input only moves the retained canvas. It never rebuilds live
        // portraits, changes selection, or commits a person.
        ApplyStationDetailScrollOffset();
    }

    void OnStationAvailableWheel(MyGUI::Widget*, int relative)
    {
        if (g_stationView.detailAvailableViewport == NULL)
        {
            return;
        }
        const int maximum = std::max(
            0, g_stationView.detailAvailableContentHeight -
                g_stationView.detailAvailableViewport->getHeight());
        g_stationView.detailAvailableOffset = ClampInt(
            g_stationView.detailAvailableOffset - relative * 46,
            0, maximum);
        g_stationView.detailAvailableScrollChanging = true;
        if (g_stationView.detailAvailableScroll != NULL)
        {
            g_stationView.detailAvailableScroll->setScrollPosition(
                static_cast<size_t>(
                    g_stationView.detailAvailableOffset));
        }
        g_stationView.detailAvailableScrollChanging = false;
        // Available-worker wheel input changes only the right retained canvas.
        // It never selects or assigns a worker.
        ApplyStationAvailableScrollOffset();
    }

    void RequestStationAddFromWidget(MyGUI::Widget* widget)
    {
        HandleIdentity member;
        if (!IsStationDetailOpen() ||
            !ParseStationIdentity(
                widget, "KJM_StationMemberIdentity", &member))
        {
            return;
        }
        RequestAddStationAssignment(g_stationView.detailStation, member);
        // Keep the modal open.  The next verified scanner snapshot refreshes
        // both lists and can apply a recent-change treatment externally.
    }

    void OnStationAddPressed(
        MyGUI::Widget* widget,
        int,
        int,
        MyGUI::MouseButton button)
    {
        if (button == MyGUI::MouseButton::Left)
        {
            RequestStationAddFromWidget(widget);
        }
    }

    void OnStationAddKey(
        MyGUI::Widget* widget,
        MyGUI::KeyCode key,
        MyGUI::Char)
    {
        if (key == MyGUI::KeyCode::Return ||
            key == MyGUI::KeyCode::NumpadEnter)
        {
            RequestStationAddFromWidget(widget);
        }
    }

    void OnStationAssignedPressed(
        MyGUI::Widget* widget,
        int,
        int,
        MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Right || !IsStationDetailOpen())
        {
            return;
        }
        HandleIdentity member;
        if (!ParseStationIdentity(
                widget, "KJM_StationMemberIdentity", &member))
        {
            return;
        }
        RequestRemoveStationAssignment(g_stationView.detailStation, member);
        // Removal is deferred and the modal intentionally stays open.
    }

    // Compatibility surface used by JobWindow while the old matrix drag UI
    // is removed.  The grouped card grid has no armed drag interaction.
    bool IsStationHeaderDragArmed()
    {
        return false;
    }

    bool IsStationAssignmentDragArmed()
    {
        return false;
    }

    bool IsStationInteractionDragArmed()
    {
        return false;
    }

    void CancelStationHeaderDrag()
    {
    }

    void CancelStationAssignmentDrag()
    {
    }

    bool RefreshStationActionProjection(
        const HandleIdentity& stationIdentity)
    {
        const StationTargetSnapshot* station =
            FindStationSnapshot(stationIdentity, NULL);
        if (station == NULL)
        {
            return false;
        }

        for (size_t groupIndex = 0;
             groupIndex < g_stationView.groups.size(); ++groupIndex)
        {
            StationGridGroup& group = g_stationView.groups[groupIndex];
            if (group.category != station->category)
            {
                continue;
            }
            group.unassignedCount = 0;
            for (size_t index = 0; index < group.stations.size(); ++index)
            {
                if (g_stationView.snapshot->stations[
                        group.stations[index]].assignments.empty())
                {
                    ++group.unassignedCount;
                }
            }
            for (size_t headerIndex = 0;
                 headerIndex < g_stationView.visibleHeaders.size();
                 ++headerIndex)
            {
                StationGridHeaderBinding& header =
                    g_stationView.visibleHeaders[headerIndex];
                if (header.category == group.category)
                {
                    SetStationGroupHeaderCaption(header.button, group);
                }
            }
            break;
        }

        for (size_t index = 0;
             index < g_stationView.visibleCards.size(); ++index)
        {
            StationGridCardBinding& binding =
                g_stationView.visibleCards[index];
            if (!SameHandleIdentity(binding.identity, stationIdentity))
            {
                continue;
            }
            if (binding.assignment != NULL)
            {
                SetStationCardAssignment(binding.assignment, *station);
            }
            if (binding.tint != NULL)
            {
                TagThemeStationCardTint(
                    binding.tint,
                    station->blocking ||
                        !station->blockingStatus.empty());
            }
            const bool recentlyChanged =
                SameHandleIdentity(
                    g_stationView.recentStation, stationIdentity) &&
                !g_stationView.recentMembers.empty();
            SetStationUnassignedOutlineVisible(
                &binding,
                IsStationUsableAndUnassigned(*station) && !recentlyChanged);
            if (binding.recentMarker != NULL)
            {
                binding.recentMarker->setVisible(recentlyChanged);
            }
        }
        if (SameHandleIdentity(
                g_stationView.detailStation, stationIdentity))
        {
            g_stationView.detailRefreshRequested = true;
        }
        return true;
    }

    bool RefreshStationTransferredMemberRows(
        const HandleIdentity& source,
        const HandleIdentity& destination)
    {
        if (!g_stationView.visible)
        {
            return true;
        }
        if (SameHandleIdentity(source, destination) &&
            g_stationView.recentStation.valid)
        {
            // Add/remove actions patch one member and one station. Update the
            // visible card, its category summary, and the open detail list in
            // place. Keep every other card/widget and the frozen order intact.
            return RefreshStationActionProjection(
                g_stationView.recentStation);
        }
        if (!SameHandleIdentity(source, destination))
        {
            RefreshStationView();
        }
        return true;
    }
