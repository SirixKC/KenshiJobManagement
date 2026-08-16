// SPDX-License-Identifier: GPL-3.0-only
// Read-only, virtualized Stations matrix.
//
// Integration contract
// --------------------
// Include this file inside the plug-in's anonymous namespace after JobView.inl
// and JobActions.inl.  StationScanner.inl must define these value types first:
//
// StationScanState
//   bool started, complete, truncated, rosterIncomplete;
//   size_t targetsCompleted, targetsFailed, nextTarget;
//   bool ownershipCopyTruncated, ownershipResolutionIncomplete;
//   std::vector<StationOwnedHandRecord> ownedBuildingRecords;
//   std::vector<hand> assignedTargetHandles;
//   std::vector<std::string> errors;
//   std::vector<StationTargetSnapshot> stations; // already display sorted
//   std::vector<StationSquadSnapshot> squads;    // Kenshi vanilla squad order
//
// StationSquadSnapshot
//   HandleIdentity identity; std::string name; bool loaded;
//   int unavailableMemberCount;
//   std::vector<StationMemberSnapshot> members;  // Kenshi vanilla member order
//
// StationMemberSnapshot
//   HandleIdentity identity; hand handle; std::string name, condition;
//   bool loaded, queueAvailable, jobsEnabled;
//   int permanentJobCount;
//   std::vector<SkillValue> topSkills;           // filtered top three
//
// StationTargetSnapshot
//   HandleIdentity identity;
//   std::string areaName, name, relevantSkillName, blockingStatus;
//   StationCategory category; bool relevantSkillKnown, blocking;
//   std::vector<StationAssignmentSnapshot> assignments;
//
// StationAssignmentSnapshot
//   HandleIdentity member; int priority, relevantSkillValue;
//   std::string jobLabel, squadName;
//
// IsStationCategoryEnabled(category), g_skillEnabled and
// GetStationVisualIconResource(category, subtype) must be available.  The view applies
// both filters and always controls unknown-skill stations through the
// Other/Unclassified category checkbox.  Scanner code owns the 2,048-target
// cap and sets truncated when it is reached.
//
// The owner calls CreateStationView(), SetStationBoardSnapshot(),
// SetStationViewVisible(), and DestroyStationView().  RefreshStationView()
// may be called after an assignment/filter update without resetting scroll,
// collapsed squads, or the selected column. Queue mutations are never made in
// a MyGUI callback. Assignment-card drops only enqueue a validated move for
// the manager's next update tick.

    const int STATION_HEADER_HEIGHT = 132;
    const int STATION_AREA_BAND_HEIGHT = 23;
    const int STATION_CATEGORY_BAND_HEIGHT = 23;
    const int STATION_CARD_HEIGHT =
        STATION_HEADER_HEIGHT - STATION_AREA_BAND_HEIGHT -
        STATION_CATEGORY_BAND_HEIGHT;
    const int STATION_COLUMN_WIDTH = 154;
    const int STATION_COLUMN_GAP = 4;
    const int STATION_COLUMN_STRIDE = STATION_COLUMN_WIDTH + STATION_COLUMN_GAP;
    const int STATION_SQUAD_ROW_HEIGHT = 30;
    const int STATION_MEMBER_ROW_HEIGHT = 116;
    const int STATION_UNAVAILABLE_ROW_HEIGHT = 28;
    const int STATION_OVERSCAN = 1;

    enum StationRosterRowKind
    {
        STATION_ROSTER_SQUAD,
        STATION_ROSTER_MEMBER,
        STATION_ROSTER_UNAVAILABLE
    };

    struct StationRosterRow
    {
        StationRosterRowKind kind;
        size_t squadIndex;
        size_t memberIndex;
        int top;
        int height;

        StationRosterRow() :
            kind(STATION_ROSTER_SQUAD), squadIndex(0), memberIndex(0),
            top(0), height(0)
        {
        }
    };

    struct StationVisibleWidget
    {
        MyGUI::Widget* root;
        MyGUI::Widget* outlineTop;
        MyGUI::Widget* outlineBottom;
        MyGUI::Widget* outlineLeft;
        MyGUI::Widget* outlineRight;
        int rowIndex;
        int stationIndex;

        StationVisibleWidget() :
            root(NULL), outlineTop(NULL), outlineBottom(NULL),
            outlineLeft(NULL), outlineRight(NULL), rowIndex(-1),
            stationIndex(-1)
        {
        }
    };

    struct StationAssignmentDragState
    {
        bool armed;
        bool active;
        MyGUI::IntPoint pressPoint;
        HandleIdentity sourceMember;
        HandleIdentity stationTarget;
        JobRowSnapshot job;
        std::vector<JobRowSnapshot> sourceSequence;
        int sourceRow;
        int sourceStation;
        int destinationRow;

        StationAssignmentDragState() :
            armed(false), active(false), pressPoint(0, 0),
            sourceRow(-1), sourceStation(-1), destinationRow(-1)
        {
        }
    };

    struct StationViewState
    {
        MyGUI::Widget* root;
        MyGUI::TextBox* scanBanner;
        MyGUI::Widget* progressTrack;
        MyGUI::Widget* progressFill;
        MyGUI::Widget* rosterHeader;
        MyGUI::Widget* headerViewport;
        MyGUI::Widget* headerCanvas;
        MyGUI::Widget* rosterViewport;
        MyGUI::Widget* rosterCanvas;
        MyGUI::Widget* matrixViewport;
        MyGUI::Widget* matrixCanvas;
        MyGUI::ScrollBar* verticalScroll;
        MyGUI::ScrollBar* horizontalScroll;
        MyGUI::TextBox* emptyText;
        const StationScanState* snapshot;
        std::vector<size_t> visibleStations;
        std::vector<StationRosterRow> rows;
        std::vector<StationVisibleWidget> rosterWidgets;
        std::vector<StationVisibleWidget> headerWidgets;
        std::vector<StationVisibleWidget> cellWidgets;
        // These are kept separately from virtual station widgets because a
        // divider spans the full header/body viewport rather than one row.
        std::vector<MyGUI::Widget*> columnDividers;
        std::vector<HandleIdentity> collapsedSquads;
        HandleIdentity selectedStation;
        int verticalOffset;
        int horizontalOffset;
        int maxVerticalOffset;
        int maxHorizontalOffset;
        int rosterWidth;
        int matrixWidth;
        int bodyHeight;
        int hoveredRow;
        int hoveredStation;
        int headerDragStartX;
        int headerDragStartOffset;
        bool headerDragArmed;
        bool headerDragActive;
        bool suppressHeaderClick;
        bool changingScroll;
        bool visible;
        bool virtualRefreshRequested;
        StationAssignmentDragState assignmentDrag;

        StationViewState() :
            root(NULL), scanBanner(NULL), progressTrack(NULL), progressFill(NULL),
            rosterHeader(NULL), headerViewport(NULL), headerCanvas(NULL),
            rosterViewport(NULL), rosterCanvas(NULL), matrixViewport(NULL),
            matrixCanvas(NULL), verticalScroll(NULL), horizontalScroll(NULL),
            emptyText(NULL), snapshot(NULL), verticalOffset(0),
            horizontalOffset(0), maxVerticalOffset(0), maxHorizontalOffset(0),
            rosterWidth(310), matrixWidth(0), bodyHeight(0), hoveredRow(-1),
            hoveredStation(-1), headerDragStartX(0), headerDragStartOffset(0),
            headerDragArmed(false), headerDragActive(false),
            suppressHeaderClick(false), changingScroll(false), visible(false),
            virtualRefreshRequested(false)
        {
        }
    };

    StationViewState g_stationView;

    void RefreshStationView();
    void OnStationVerticalScroll(MyGUI::ScrollBar*, size_t);
    void OnStationHorizontalScroll(MyGUI::ScrollBar*, size_t);
    void OnStationMouseWheel(MyGUI::Widget*, int);
    void OnStationSquadClicked(MyGUI::Widget*);
    void OnStationColumnClicked(MyGUI::Widget*);
    void OnStationHeaderPressed(
        MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnStationHeaderDrag(
        MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnStationHeaderReleased(
        MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnStationAssignmentPressed(
        MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnStationAssignmentDrag(
        MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnStationAssignmentReleased(
        MyGUI::Widget*, int, int, MyGUI::MouseButton);
    void OnStationWidgetFocus(MyGUI::Widget*, MyGUI::Widget*);
    void OnStationWidgetLostFocus(MyGUI::Widget*, MyGUI::Widget*);
    void ApplyStationOutlines();
    void CancelStationAssignmentDrag();

    int StationParseIndex(MyGUI::Widget* widget, const char* key)
    {
        if (widget == NULL || !widget->isUserString(key))
        {
            return -1;
        }
        return std::atoi(widget->getUserString(key).c_str());
    }

    std::string StationIntegerString(int value)
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    bool IsStationSquadCollapsed(const HandleIdentity& identity)
    {
        for (size_t index = 0; index < g_stationView.collapsedSquads.size(); ++index)
        {
            if (SameHandleIdentity(g_stationView.collapsedSquads[index], identity))
            {
                return true;
            }
        }
        return false;
    }

    void ToggleStationSquadCollapsed(const HandleIdentity& identity)
    {
        for (size_t index = 0; index < g_stationView.collapsedSquads.size(); ++index)
        {
            if (SameHandleIdentity(g_stationView.collapsedSquads[index], identity))
            {
                g_stationView.collapsedSquads.erase(
                    g_stationView.collapsedSquads.begin() + index);
                return;
            }
        }
        g_stationView.collapsedSquads.push_back(identity);
    }

    void DestroyStationWidgetVector(std::vector<StationVisibleWidget>& widgets)
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            for (size_t index = 0; index < widgets.size(); ++index)
            {
                if (widgets[index].root != NULL)
                {
                    gui->destroyWidget(widgets[index].root);
                }
            }
        }
        widgets.clear();
    }

    void DestroyStationColumnDividers()
    {
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui != NULL)
        {
            for (size_t index = 0; index < g_stationView.columnDividers.size();
                 ++index)
            {
                if (g_stationView.columnDividers[index] != NULL)
                {
                    gui->destroyWidget(g_stationView.columnDividers[index]);
                }
            }
        }
        g_stationView.columnDividers.clear();
    }

    void DestroyStationVirtualWidgets()
    {
        if (g_tooltip != NULL)
        {
            g_tooltip->setVisible(false);
        }
        DestroyStationColumnDividers();
        DestroyStationWidgetVector(g_stationView.cellWidgets);
        DestroyStationWidgetVector(g_stationView.headerWidgets);
        DestroyStationWidgetVector(g_stationView.rosterWidgets);
    }

    void CreateStationColumnDividers(int firstStation, int lastStation)
    {
        if (g_stationView.headerCanvas == NULL ||
            g_stationView.matrixCanvas == NULL || firstStation > lastStation)
        {
            return;
        }

        // The four-pixel gap between station cards is intentionally retained.
        // Put one thin neutral line in the middle of that gap, so the divider
        // stays readable without covering station artwork or assignment text.
        const int dividerWidth = 1;
        const int dividerX = STATION_COLUMN_WIDTH + STATION_COLUMN_GAP / 2;
        const MyGUI::Colour dividerColour(0.76f, 0.76f, 0.76f);
        for (int stationIndex = firstStation;
             stationIndex <= lastStation; ++stationIndex)
        {
            const int x = stationIndex * STATION_COLUMN_STRIDE -
                g_stationView.horizontalOffset + dividerX;
            MyGUI::Widget* headerDivider =
                g_stationView.headerCanvas->createWidget<MyGUI::Widget>(
                    "WhiteSkin",
                    MyGUI::IntCoord(x, 0, dividerWidth,
                        STATION_HEADER_HEIGHT),
                    MyGUI::Align::Left | MyGUI::Align::Top,
                    "KJM_StationColumnDividerHeader");
            headerDivider->setColour(dividerColour);
            headerDivider->setAlpha(0.33f);
            headerDivider->setDepth(0);
            headerDivider->setNeedMouseFocus(false);
            headerDivider->setUserString(
                "KJM_StationColumn", StationIntegerString(stationIndex));
            g_stationView.columnDividers.push_back(headerDivider);

            MyGUI::Widget* bodyDivider =
                g_stationView.matrixCanvas->createWidget<MyGUI::Widget>(
                    "WhiteSkin",
                    MyGUI::IntCoord(x, 0, dividerWidth,
                        g_stationView.bodyHeight),
                    MyGUI::Align::Left | MyGUI::Align::Top,
                    "KJM_StationColumnDividerBody");
            bodyDivider->setColour(dividerColour);
            bodyDivider->setAlpha(0.33f);
            bodyDivider->setDepth(0);
            bodyDivider->setNeedMouseFocus(false);
            bodyDivider->setUserString(
                "KJM_StationColumn", StationIntegerString(stationIndex));
            g_stationView.columnDividers.push_back(bodyDivider);
        }
    }

    void CreateStationOutlineWidgets(
        StationVisibleWidget* visible,
        int width,
        int height)
    {
        if (visible == NULL || visible->root == NULL || width <= 0 ||
            height <= 0)
        {
            return;
        }
        visible->outlineTop = visible->root->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(0, 0, width, 2),
            MyGUI::Align::Top | MyGUI::Align::HStretch, "KJM_StationOutlineTop");
        visible->outlineBottom = visible->root->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(0, height - 2, width, 2),
            MyGUI::Align::Bottom | MyGUI::Align::HStretch, "KJM_StationOutlineBottom");
        visible->outlineLeft = visible->root->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(0, 0, 2, height),
            MyGUI::Align::Left | MyGUI::Align::VStretch, "KJM_StationOutlineLeft");
        visible->outlineRight = visible->root->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(width - 2, 0, 2, height),
            MyGUI::Align::Right | MyGUI::Align::VStretch, "KJM_StationOutlineRight");
        visible->outlineTop->setVisible(false);
        visible->outlineBottom->setVisible(false);
        visible->outlineLeft->setVisible(false);
        visible->outlineRight->setVisible(false);
        visible->outlineTop->setNeedMouseFocus(false);
        visible->outlineBottom->setNeedMouseFocus(false);
        visible->outlineLeft->setNeedMouseFocus(false);
        visible->outlineRight->setNeedMouseFocus(false);
        visible->outlineTop->setDepth(0);
        visible->outlineBottom->setDepth(0);
        visible->outlineLeft->setDepth(0);
        visible->outlineRight->setDepth(0);
    }

    void BuildStationRosterRows()
    {
        g_stationView.rows.clear();
        if (g_stationView.snapshot == NULL)
        {
            return;
        }

        int top = 0;
        const std::vector<StationSquadSnapshot>& squads =
            g_stationView.snapshot->squads;
        for (size_t squadIndex = 0; squadIndex < squads.size(); ++squadIndex)
        {
            StationRosterRow squadRow;
            squadRow.kind = STATION_ROSTER_SQUAD;
            squadRow.squadIndex = squadIndex;
            squadRow.top = top;
            squadRow.height = STATION_SQUAD_ROW_HEIGHT;
            g_stationView.rows.push_back(squadRow);
            top += squadRow.height;

            const StationSquadSnapshot& squad = squads[squadIndex];
            if (IsStationSquadCollapsed(squad.identity) || !squad.loaded)
            {
                continue;
            }
            for (size_t memberIndex = 0; memberIndex < squad.members.size(); ++memberIndex)
            {
                if (!squad.members[memberIndex].loaded)
                {
                    continue;
                }
                StationRosterRow memberRow;
                memberRow.kind = STATION_ROSTER_MEMBER;
                memberRow.squadIndex = squadIndex;
                memberRow.memberIndex = memberIndex;
                memberRow.top = top;
                memberRow.height = STATION_MEMBER_ROW_HEIGHT;
                g_stationView.rows.push_back(memberRow);
                top += memberRow.height;
            }
            if (squad.unavailableMemberCount > 0)
            {
                StationRosterRow missingRow;
                missingRow.kind = STATION_ROSTER_UNAVAILABLE;
                missingRow.squadIndex = squadIndex;
                missingRow.top = top;
                missingRow.height = STATION_UNAVAILABLE_ROW_HEIGHT;
                g_stationView.rows.push_back(missingRow);
                top += missingRow.height;
            }
        }
    }

    int GetStationRosterContentHeight()
    {
        if (g_stationView.rows.empty())
        {
            return 0;
        }
        const StationRosterRow& last = g_stationView.rows.back();
        return last.top + last.height;
    }

    bool IsStationRowVisible(const StationRosterRow& row)
    {
        const int top = g_stationView.verticalOffset - STATION_MEMBER_ROW_HEIGHT;
        const int bottom = g_stationView.verticalOffset +
            g_stationView.bodyHeight + STATION_MEMBER_ROW_HEIGHT;
        return row.top + row.height > top && row.top < bottom;
    }

    int GetFirstVisibleStationIndex()
    {
        if (g_stationView.snapshot == NULL ||
            g_stationView.visibleStations.empty())
        {
            return 0;
        }
        return ClampInt(
            g_stationView.horizontalOffset / STATION_COLUMN_STRIDE -
                STATION_OVERSCAN,
            0,
            static_cast<int>(g_stationView.visibleStations.size()) - 1);
    }

    int GetLastVisibleStationIndex()
    {
        if (g_stationView.snapshot == NULL ||
            g_stationView.visibleStations.empty())
        {
            return -1;
        }
        return ClampInt(
            (g_stationView.horizontalOffset + g_stationView.matrixWidth) /
                STATION_COLUMN_STRIDE + STATION_OVERSCAN,
            0,
            static_cast<int>(g_stationView.visibleStations.size()) - 1);
    }

    const StationTargetSnapshot* GetVisibleStation(int stationIndex)
    {
        if (g_stationView.snapshot == NULL || stationIndex < 0 ||
            stationIndex >= static_cast<int>(g_stationView.visibleStations.size()))
        {
            return NULL;
        }
        const size_t sourceIndex = g_stationView.visibleStations[stationIndex];
        if (sourceIndex >= g_stationView.snapshot->stations.size())
        {
            return NULL;
        }
        return &g_stationView.snapshot->stations[sourceIndex];
    }

    void SetStationOutlineStyle(
        const StationVisibleWidget& visible,
        int width,
        int height,
        const MyGUI::Colour& colour,
        int thickness,
        bool shown)
    {
        if (visible.outlineTop == NULL || visible.outlineBottom == NULL ||
            visible.outlineLeft == NULL || visible.outlineRight == NULL ||
            width <= 0 || height <= 0)
        {
            return;
        }
        int edge = std::max(1, thickness);
        edge = std::min(edge, std::max(1, std::min(width, height) / 2));
        visible.outlineTop->setCoord(0, 0, width, edge);
        visible.outlineBottom->setCoord(0, height - edge, width, edge);
        visible.outlineLeft->setCoord(0, 0, edge, height);
        visible.outlineRight->setCoord(width - edge, 0, edge, height);
        visible.outlineTop->setColour(colour);
        visible.outlineBottom->setColour(colour);
        visible.outlineLeft->setColour(colour);
        visible.outlineRight->setColour(colour);
        visible.outlineTop->setVisible(shown);
        visible.outlineBottom->setVisible(shown);
        visible.outlineLeft->setVisible(shown);
        visible.outlineRight->setVisible(shown);
    }

    void ApplyStationOutline(const StationVisibleWidget& visible)
    {
        if (visible.root == NULL)
        {
            return;
        }

        if (visible.stationIndex >= 0)
        {
            const StationTargetSnapshot* station =
                GetVisibleStation(visible.stationIndex);
            if (station == NULL)
            {
                SetStationOutlineStyle(
                    visible, STATION_COLUMN_WIDTH, STATION_HEADER_HEIGHT,
                    MyGUI::Colour(1.0f, 1.0f, 1.0f), 1, false);
                return;
            }
            const bool selected = g_stationView.selectedStation.valid &&
                SameHandleIdentity(
                    g_stationView.selectedStation, station->identity);
            const bool hoveredColumn =
                g_stationView.hoveredStation == visible.stationIndex;
            const bool hoveredRow = visible.rowIndex >= 0 &&
                g_stationView.hoveredRow == visible.rowIndex;
            const bool assignmentDestination =
                g_stationView.assignmentDrag.active &&
                visible.rowIndex >= 0 &&
                g_stationView.assignmentDrag.destinationRow ==
                    visible.rowIndex;
            const bool header = visible.rowIndex < 0;
            const bool highlighted = selected || hoveredColumn || hoveredRow ||
                assignmentDestination;
            const int height = header ? STATION_HEADER_HEIGHT :
                (visible.rowIndex >= 0 &&
                 visible.rowIndex < static_cast<int>(g_stationView.rows.size()) ?
                    g_stationView.rows[visible.rowIndex].height :
                    STATION_MEMBER_ROW_HEIGHT);
            if (highlighted)
            {
                SetStationOutlineStyle(
                    visible, STATION_COLUMN_WIDTH, height,
                    assignmentDestination ? MyGUI::Colour(0.40f, 1.0f, 0.48f) :
                    (selected ? MyGUI::Colour(1.0f, 0.72f, 0.20f) :
                        MyGUI::Colour(0.71f, 0.58f, 0.32f)),
                    assignmentDestination ? 3 : (selected ? 2 : 1), true);
            }
            else
            {
                SetStationOutlineStyle(
                    visible, STATION_COLUMN_WIDTH, height,
                    MyGUI::Colour(1.0f, 1.0f, 1.0f), 1, false);
            }
            return;
        }

        if (visible.rowIndex >= 0 &&
            visible.rowIndex < static_cast<int>(g_stationView.rows.size()) &&
            g_stationView.snapshot != NULL)
        {
            const StationRosterRow& row =
                g_stationView.rows[visible.rowIndex];
            if (g_stationView.assignmentDrag.active &&
                g_stationView.assignmentDrag.destinationRow ==
                    visible.rowIndex)
            {
                SetStationOutlineStyle(
                    visible, g_stationView.rosterWidth, row.height,
                    MyGUI::Colour(0.40f, 1.0f, 0.48f), 3, true);
                return;
            }
            if (row.kind == STATION_ROSTER_SQUAD &&
                row.squadIndex < g_stationView.snapshot->squads.size() &&
                SameHandleIdentity(
                    g_squad.identity,
                    g_stationView.snapshot->squads[row.squadIndex].identity))
            {
                SetStationOutlineStyle(
                    visible, g_stationView.rosterWidth, row.height,
                    MyGUI::Colour(1.0f, 0.72f, 0.20f), 2, true);
                return;
            }
            SetStationOutlineStyle(
                visible, g_stationView.rosterWidth, row.height,
                MyGUI::Colour(1.0f, 1.0f, 1.0f), 1, false);
            return;
        }

        SetStationOutlineStyle(
            visible, g_stationView.rosterWidth, STATION_MEMBER_ROW_HEIGHT,
            MyGUI::Colour(1.0f, 1.0f, 1.0f), 1, false);
    }

    void ApplyStationOutlines()
    {
        for (size_t index = 0; index < g_stationView.rosterWidgets.size(); ++index)
        {
            ApplyStationOutline(g_stationView.rosterWidgets[index]);
        }
        for (size_t index = 0; index < g_stationView.headerWidgets.size(); ++index)
        {
            ApplyStationOutline(g_stationView.headerWidgets[index]);
        }
        for (size_t index = 0; index < g_stationView.cellWidgets.size(); ++index)
        {
            ApplyStationOutline(g_stationView.cellWidgets[index]);
        }
    }

    std::string GetStationPresentationName(int stationIndex)
    {
        const StationTargetSnapshot* station = GetVisibleStation(stationIndex);
        if (station == NULL)
        {
            return std::string();
        }
        int ordinal = 0;
        int duplicateCount = 0;
        for (size_t index = 0; index < g_stationView.visibleStations.size(); ++index)
        {
            const StationTargetSnapshot* candidate =
                GetVisibleStation(static_cast<int>(index));
            if (candidate != NULL && candidate->areaName == station->areaName &&
                candidate->category == station->category &&
                candidate->name == station->name)
            {
                ++duplicateCount;
                if (static_cast<int>(index) <= stationIndex)
                {
                    ++ordinal;
                }
            }
        }
        if (duplicateCount <= 1)
        {
            return station->name;
        }
        std::ostringstream caption;
        caption << station->name << " #" << ordinal;
        return caption.str();
    }

    void BuildVisibleStationList()
    {
        g_stationView.visibleStations.clear();
        if (g_stationView.snapshot == NULL)
        {
            return;
        }
        for (size_t index = 0; index < g_stationView.snapshot->stations.size(); ++index)
        {
            const StationTargetSnapshot& station =
                g_stationView.snapshot->stations[index];
            if (!IsStationCategoryEnabled(station.category))
            {
                continue;
            }
            // A known relevant skill participates in the shared skill filter.
            // Stations with no known skill remain controlled by category only.
            if (station.relevantSkillKnown &&
                (station.relevantStat <= STAT_NONE ||
                 station.relevantStat >= STAT_END ||
                 !g_skillEnabled[station.relevantStat]))
            {
                continue;
            }
            g_stationView.visibleStations.push_back(index);
        }
    }

    const StationAssignmentSnapshot* FindStationAssignment(
        const StationTargetSnapshot& station,
        const HandleIdentity& worker,
        size_t occurrence)
    {
        size_t found = 0;
        for (size_t index = 0; index < station.assignments.size(); ++index)
        {
            if (SameHandleIdentity(station.assignments[index].member, worker))
            {
                if (found == occurrence)
                {
                    return &station.assignments[index];
                }
                ++found;
            }
        }
        return NULL;
    }

    size_t CountStationAssignments(
        const StationTargetSnapshot& station,
        const HandleIdentity& worker)
    {
        size_t count = 0;
        for (size_t index = 0; index < station.assignments.size(); ++index)
        {
            if (SameHandleIdentity(station.assignments[index].member, worker))
            {
                ++count;
            }
        }
        return count;
    }

    void CopyStationMemberQueue(
        const StationMemberSnapshot& member,
        std::vector<JobRowSnapshot>* jobsOut)
    {
        if (jobsOut == NULL)
        {
            return;
        }
        jobsOut->clear();
        jobsOut->reserve(member.jobs.size());
        for (size_t index = 0; index < member.jobs.size(); ++index)
        {
            jobsOut->push_back(member.jobs[index].exactJob);
        }
    }

    bool TryGetStationMemberForRow(
        int rowIndex,
        const StationMemberSnapshot** memberOut)
    {
        if (memberOut == NULL || g_stationView.snapshot == NULL ||
            rowIndex < 0 ||
            rowIndex >= static_cast<int>(g_stationView.rows.size()))
        {
            return false;
        }
        *memberOut = NULL;
        const StationRosterRow& row = g_stationView.rows[rowIndex];
        if (row.kind != STATION_ROSTER_MEMBER ||
            row.squadIndex >= g_stationView.snapshot->squads.size())
        {
            return false;
        }
        const StationSquadSnapshot& squad =
            g_stationView.snapshot->squads[row.squadIndex];
        if (row.memberIndex >= squad.members.size())
        {
            return false;
        }
        *memberOut = &squad.members[row.memberIndex];
        return true;
    }

    int FindStationMemberRowAtPoint(const MyGUI::IntPoint& mouse)
    {
        if (g_stationView.rosterViewport == NULL ||
            g_stationView.matrixViewport == NULL)
        {
            return -1;
        }
        const MyGUI::IntCoord roster =
            g_stationView.rosterViewport->getAbsoluteCoord();
        const MyGUI::IntCoord matrix =
            g_stationView.matrixViewport->getAbsoluteCoord();
        const bool inRoster = PointInside(roster, mouse);
        const bool inMatrix = PointInside(matrix, mouse);
        if (!inRoster && !inMatrix)
        {
            return -1;
        }
        const int viewportTop = inRoster ? roster.top : matrix.top;
        const int contentY = mouse.top - viewportTop +
            g_stationView.verticalOffset;
        for (size_t index = 0; index < g_stationView.rows.size(); ++index)
        {
            const StationRosterRow& row = g_stationView.rows[index];
            if (row.kind == STATION_ROSTER_MEMBER &&
                contentY >= row.top && contentY < row.top + row.height)
            {
                const StationMemberSnapshot* member = NULL;
                if (TryGetStationMemberForRow(
                        static_cast<int>(index), &member) && member != NULL &&
                    member->loaded && member->queueAvailable &&
                    !member->truncated)
                {
                    return static_cast<int>(index);
                }
                return -1;
            }
        }
        return -1;
    }

    std::string BuildStationWorkerSkills(const StationMemberSnapshot& worker)
    {
        std::ostringstream caption;
        const size_t count = std::min<size_t>(3, worker.topSkills.size());
        for (size_t index = 0; index < count; ++index)
        {
            if (index != 0)
            {
                caption << "\n";
            }
            caption << worker.topSkills[index].name << " " << worker.topSkills[index].value;
        }
        if (count == 0)
        {
            return worker.loaded ? "No enabled stats above 1" :
                "Stats unavailable";
        }
        return caption.str();
    }

    std::string TrimStationAssignmentText(const std::string& text)
    {
        size_t first = 0;
        while (first < text.size() &&
            (text[first] == ' ' || text[first] == '\t' ||
             text[first] == '\r' || text[first] == '\n'))
        {
            ++first;
        }
        size_t last = text.size();
        while (last > first &&
            (text[last - 1] == ' ' || text[last - 1] == '\t' ||
             text[last - 1] == '\r' || text[last - 1] == '\n'))
        {
            --last;
        }
        return text.substr(first, last - first);
    }

    std::string GetStationAssignmentWorkLabel(
        const StationTargetSnapshot& station,
        const StationAssignmentSnapshot& assignment)
    {
        // Reuse the squad-card normalizer so MyGUI colour markup and the
        // engine's leading queue number never leak into the station matrix.
        std::string label = StripLeadingPriorityPrefix(assignment.jobLabel);
        label = TrimStationAssignmentText(label);

        // Station order text normally uses "work type: target".  The
        // target is already the column header, so keep only the work type in
        // the assignment row.  Prefer the exact target match when available,
        // then fall back to the first task separator for older/localized
        // labels whose target text cannot be matched exactly.
        if (!station.name.empty())
        {
            const size_t target = label.rfind(station.name);
            if (target != std::string::npos && target > 0)
            {
                // Do not require punctuation.  Some Kenshi order strings
                // use "Hauling to Bread Oven" instead of a colon, and the
                // renamed station is still the reliable suffix.
                label = TrimStationAssignmentText(label.substr(0, target));
                while (!label.empty() &&
                    (label[label.size() - 1] == ':' ||
                     label[label.size() - 1] == '-' ||
                     label[label.size() - 1] == '|'))
                {
                    label = TrimStationAssignmentText(
                        label.substr(0, label.size() - 1));
                }
            }
        }

        const size_t separator = label.find(':');
        if (separator != std::string::npos)
        {
            label = TrimStationAssignmentText(label.substr(0, separator));
        }
        if (label.empty())
        {
            label = "Assigned job";
        }
        return label;
    }

    std::string GetCompactStationAssignmentWorkLabel(
        const StationTargetSnapshot& station,
        const StationAssignmentSnapshot& assignment)
    {
        const std::string fullLabel =
            GetStationAssignmentWorkLabel(station, assignment);
        if (fullLabel.compare(0, 9, "Operating") == 0)
        {
            return "Operating...";
        }
        if (fullLabel.compare(0, 7, "Hauling") == 0)
        {
            return "Hauling...";
        }

        // Keep an unfamiliar/localized work type readable too.  The first
        // word identifies the task category, while the tooltip retains the
        // full order text and target.
        const size_t separator = fullLabel.find_first_of(" \t");
        if (separator != std::string::npos && separator > 0)
        {
            return fullLabel.substr(0, separator) + "...";
        }
        return fullLabel;
    }

    void SetFittedStationAssignmentLabel(
        MyGUI::TextBox* text,
        const std::string& caption,
        int maxWidth,
        int startingFontHeight)
    {
        if (text == NULL)
        {
            return;
        }
        text->setCaption(caption);
        int fontHeight = startingFontHeight;
        text->setFontHeight(fontHeight);
        while (fontHeight > 10 && text->getTextSize().width > maxWidth)
        {
            --fontHeight;
            text->setFontHeight(fontHeight);
        }
    }

    void ApplyStationPortrait(
        MyGUI::Button* border,
        MyGUI::ImageBox* background,
        MyGUI::ImageBox* portrait,
        MyGUI::ImageBox* backOverlay,
        MyGUI::ImageBox* frontOverlay,
        const StationMemberSnapshot& worker)
    {
        bool selected = false;
        std::string backgroundName;
        std::string backName;
        std::string frontName;
        if (TryGetPortraitVisuals(
                worker.handle, &selected, &backgroundName, &backName, &frontName))
        {
            border->setStateSelected(selected);
            SetPortraitImage(background, "Background", backgroundName);
            SetPortraitImage(backOverlay, "BackOverlay", backName);
            SetPortraitImage(frontOverlay, "FrontOverlay", frontName);
        }
        portrait->setVisible(true);
        if (!TryBindPortrait(worker.handle, portrait, false))
        {
            TryBindPortrait(worker.handle, portrait, true);
        }
    }

    void AttachStationInput(
        MyGUI::Widget* widget,
        int rowIndex,
        int stationIndex,
        const std::string& tooltip)
    {
        if (widget == NULL)
        {
            return;
        }
        widget->setUserString("KJM_StationRow", StationIntegerString(rowIndex));
        widget->setUserString(
            "KJM_StationColumn", StationIntegerString(stationIndex));
        widget->eventMouseSetFocus += MyGUI::newDelegate(OnStationWidgetFocus);
        widget->eventMouseLostFocus += MyGUI::newDelegate(OnStationWidgetLostFocus);
        widget->eventMouseWheel += MyGUI::newDelegate(OnStationMouseWheel);
        if (stationIndex >= 0)
        {
            widget->eventMouseButtonClick +=
                MyGUI::newDelegate(OnStationColumnClicked);
        }
        if (!tooltip.empty())
        {
            widget->setUserString("KJM_ToolTip", tooltip);
            widget->setNeedToolTip(true);
            widget->eventToolTip += MyGUI::newDelegate(OnCardToolTip);
        }
    }

    // The header strip behaves like a small grab-to-pan surface.  Attach the
    // handlers to both the header canvas (so the gaps between cards work) and
    // each card (so a grab that starts on artwork still captures the pointer).
    // The callbacks use the absolute mouse position because MyGUI reports
    // drag coordinates in the sender's coordinate space.
    void AttachStationHeaderDragInput(MyGUI::Widget* widget)
    {
        if (widget == NULL)
        {
            return;
        }
        widget->eventMouseButtonPressed +=
            MyGUI::newDelegate(OnStationHeaderPressed);
        widget->eventMouseDrag += MyGUI::newDelegate(OnStationHeaderDrag);
        widget->eventMouseButtonReleased +=
            MyGUI::newDelegate(OnStationHeaderReleased);
    }

    void CreateStationPortraitWidgets(
        MyGUI::Widget* root,
        const StationMemberSnapshot& worker)
    {
        MyGUI::Button* border = root->createWidget<MyGUI::Button>(
            "Kenshi_PortraitFrameSkin",
            MyGUI::IntCoord(7, 6, MEMBER_PORTRAIT_SIZE, MEMBER_PORTRAIT_SIZE),
            MyGUI::Align::Left | MyGUI::Align::Top, "KJM_StationPortraitFrame");
        border->setNeedMouseFocus(false);
        MyGUI::ImageBox* background = border->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                MEMBER_PORTRAIT_INSET, MEMBER_PORTRAIT_INSET,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2),
            MyGUI::Align::Stretch,
            "KJM_StationPortraitBackground");
        MyGUI::ImageBox* portrait = border->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                MEMBER_PORTRAIT_INSET, MEMBER_PORTRAIT_INSET,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2),
            MyGUI::Align::Stretch,
            "KJM_StationPortrait");
        MyGUI::ImageBox* backOverlay = border->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                MEMBER_PORTRAIT_INSET, MEMBER_PORTRAIT_INSET,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2),
            MyGUI::Align::Stretch,
            "KJM_StationPortraitBackOverlay");
        MyGUI::ImageBox* frontOverlay = border->createWidget<MyGUI::ImageBox>(
            "ImageBox", MyGUI::IntCoord(
                MEMBER_PORTRAIT_INSET, MEMBER_PORTRAIT_INSET,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2,
                MEMBER_PORTRAIT_SIZE - MEMBER_PORTRAIT_INSET * 2),
            MyGUI::Align::Stretch,
            "KJM_StationPortraitFrontOverlay");
        background->setDepth(5);
        backOverlay->setDepth(4);
        portrait->setDepth(3);
        frontOverlay->setDepth(2);
        background->setNeedMouseFocus(false);
        portrait->setNeedMouseFocus(false);
        backOverlay->setNeedMouseFocus(false);
        frontOverlay->setNeedMouseFocus(false);
        ApplyStationPortrait(
            border, background, portrait, backOverlay, frontOverlay, worker);
    }

    void CreateStationRosterWidget(size_t rowIndex)
    {
        if (g_stationView.snapshot == NULL ||
            rowIndex >= g_stationView.rows.size())
        {
            return;
        }
        const StationRosterRow& row = g_stationView.rows[rowIndex];
        const StationSquadSnapshot& squad =
            g_stationView.snapshot->squads[row.squadIndex];
        const int y = row.top - g_stationView.verticalOffset;
        StationVisibleWidget visible;
        visible.rowIndex = static_cast<int>(rowIndex);
        visible.root = g_stationView.rosterCanvas->createWidget<MyGUI::Widget>(
            row.kind == STATION_ROSTER_MEMBER ? "Kenshi_SelectionPanel" : "WhiteSkin",
            MyGUI::IntCoord(0, y, g_stationView.rosterWidth, row.height),
            MyGUI::Align::Left | MyGUI::Align::Top, "KJM_StationRosterRow");
        visible.root->setColour(
            row.kind == STATION_ROSTER_SQUAD ? MyGUI::Colour(0.20f, 0.16f, 0.11f) :
            MyGUI::Colour(0.12f, 0.10f, 0.08f));
        if (row.kind != STATION_ROSTER_MEMBER)
        {
            visible.root->eventMouseWheel +=
                MyGUI::newDelegate(OnStationMouseWheel);
        }

        if (row.kind == STATION_ROSTER_SQUAD)
        {
            MyGUI::Button* button = visible.root->createWidget<MyGUI::Button>(
                "Kenshi_Button1", MyGUI::IntCoord(2, 2, g_stationView.rosterWidth - 4,
                row.height - 4), MyGUI::Align::Stretch, "KJM_StationSquadHeader");
            std::ostringstream caption;
            caption << ((!squad.loaded || IsStationSquadCollapsed(squad.identity)) ?
                    "> " : "V ")
                    << squad.name;
            if (!squad.loaded)
            {
                caption << "  -  LIVE DATA UNAVAILABLE";
            }
            button->setCaption(caption.str());
            button->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
            button->setFontHeight(14);
            button->setUserString("KJM_StationSquad", IntegerString(row.squadIndex));
            button->eventMouseButtonClick += MyGUI::newDelegate(OnStationSquadClicked);
            button->eventMouseWheel += MyGUI::newDelegate(OnStationMouseWheel);
        }
        else if (row.kind == STATION_ROSTER_UNAVAILABLE)
        {
            MyGUI::TextBox* label = visible.root->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(12, 3, g_stationView.rosterWidth - 20, row.height - 6),
                MyGUI::Align::Stretch, "KJM_StationUnavailableMembers");
            std::ostringstream caption;
            caption << squad.unavailableMemberCount << " members unavailable";
            label->setCaption(caption.str());
            label->setTextColour(MyGUI::Colour(0.92f, 0.63f, 0.43f));
            label->setNeedMouseFocus(false);
        }
        else
        {
            const StationMemberSnapshot& worker = squad.members[row.memberIndex];
            CreateStationPortraitWidgets(visible.root, worker);
            MyGUI::TextBox* name = visible.root->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(
                    MEMBER_TEXT_LEFT, 2,
                    g_stationView.rosterWidth - MEMBER_TEXT_LEFT - 8, 25),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationWorkerName");
            name->setCaption(worker.name);
            name->setFontHeight(21);
            name->setTextColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
            name->setNeedMouseFocus(false);
            MyGUI::TextBox* condition = visible.root->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(
                    MEMBER_TEXT_LEFT, 27,
                    g_stationView.rosterWidth - MEMBER_TEXT_LEFT - 8, 16),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationWorkerCondition");
            condition->setCaption(worker.condition);
            condition->setFontHeight(15);
            condition->setTextColour(MyGUI::Colour(0.96f, 0.48f, 0.36f));
            condition->setNeedMouseFocus(false);
            MyGUI::TextBox* skills = visible.root->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(
                    MEMBER_TEXT_LEFT, 43,
                    g_stationView.rosterWidth - MEMBER_TEXT_LEFT - 8, 48),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationWorkerSkills");
            skills->setCaption(BuildStationWorkerSkills(worker));
            skills->setFontHeight(16);
            skills->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
            skills->setTextColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
            skills->setNeedMouseFocus(false);
            MyGUI::TextBox* queue = visible.root->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(
                    MEMBER_TEXT_LEFT, 94,
                    g_stationView.rosterWidth - MEMBER_TEXT_LEFT - 8, 19),
                MyGUI::Align::Top | MyGUI::Align::HStretch,
                "KJM_StationWorkerQueueState");
            queue->setFontHeight(13);
            std::ostringstream queueCaption;
            if (!worker.queueAvailable)
            {
                queueCaption << "JOBS UNAVAILABLE";
                queue->setTextColour(MyGUI::Colour(1.0f, 0.50f, 0.38f));
            }
            else if (worker.permanentJobCount == 0)
            {
                queueCaption << "NO PERMANENT JOBS";
                queue->setTextColour(MyGUI::Colour(1.0f, 0.42f, 0.32f));
            }
            else
            {
                queueCaption << worker.permanentJobCount << " JOBS";
                queue->setTextColour(MyGUI::Colour(0.94f, 0.88f, 0.69f));
            }
            if (worker.queueAvailable && !worker.jobsEnabled)
            {
                queueCaption << "  |  JOBS OFF";
            }
            queue->setCaption(queueCaption.str());
            queue->setNeedMouseFocus(false);
            AttachStationInput(
                visible.root, static_cast<int>(rowIndex), -1, std::string());
        }
        CreateStationOutlineWidgets(
            &visible, g_stationView.rosterWidth, row.height);
        ApplyStationOutline(visible);
        g_stationView.rosterWidgets.push_back(visible);
    }

    std::string StationCategoryGlyph(StationCategory category)
    {
        switch (category)
        {
        case STATION_CRAFTING: return "ANVIL";
        case STATION_REFINING: return "FURNACE";
        case STATION_FARMING: return "WHEAT";
        case STATION_MINING: return "PICK";
        case STATION_RESEARCH: return "FLASK";
        case STATION_TRAINING: return "TRAIN";
        case STATION_STORAGE_HAULING: return "CRATE";
        case STATION_DEFENSE: return "TURRET";
        default: return "?";
        }
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
            const MyGUI::IntSize imageSize = icon->getImageSize();
            return imageSize.width > 0 && imageSize.height > 0;
        }
        catch (...)
        {
            return false;
        }
    }

    void SetFittedStationName(
        MyGUI::TextBox* text,
        const std::string& caption)
    {
        if (text == NULL)
        {
            return;
        }
        // The category artwork now fills the card background, so the name
        // does not need to reserve a narrow strip beside a small icon.  Give
        // it the full card width and start from a readable 16px size.  The
        // fitter still reduces unusually long renamed buildings as needed.
        const int maxWidth = STATION_COLUMN_WIDTH - 10;
        const int maxHeight = 45;
        text->setCaption(caption);
        text->setFontHeight(16);
        if (text->getTextSize().width > maxWidth)
        {
            text->setCaption(WrapCardJobCaption(caption, 20));
        }
        int fontHeight = text->getFontHeight();
        while (fontHeight > 9 &&
            (text->getTextSize().width > maxWidth ||
             text->getTextSize().height > maxHeight))
        {
            --fontHeight;
            text->setFontHeight(fontHeight);
        }
    }

    void CreateStationHeaderWidget(int stationIndex)
    {
        const StationTargetSnapshot* stationPointer =
            GetVisibleStation(stationIndex);
        if (stationPointer == NULL)
        {
            return;
        }
        const StationTargetSnapshot& station = *stationPointer;
        const int x = stationIndex * STATION_COLUMN_STRIDE -
            g_stationView.horizontalOffset;
        StationVisibleWidget visible;
        visible.stationIndex = stationIndex;
        visible.root = g_stationView.headerCanvas->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(x, 0, STATION_COLUMN_WIDTH,
            STATION_HEADER_HEIGHT), MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_StationHeader");
        visible.root->setColour(MyGUI::Colour(0.14f, 0.11f, 0.08f));
        visible.root->setUserString("KJM_StationColumn", IntegerString(stationIndex));
        visible.root->setUserString("KJM_StationHeaderCard", "1");

        const bool viewportStart =
            stationIndex == GetFirstVisibleStationIndex();
        const bool areaStart = viewportStart || stationIndex == 0 ||
            GetVisibleStation(stationIndex - 1)->areaName != station.areaName;
        const bool categoryStart = viewportStart || areaStart || stationIndex == 0 ||
            GetVisibleStation(stationIndex - 1)->category != station.category;
        MyGUI::TextBox* area = visible.root->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(2, 1, STATION_COLUMN_WIDTH - 4,
                STATION_AREA_BAND_HEIGHT - 2),
            MyGUI::Align::Top | MyGUI::Align::HStretch, "KJM_StationAreaBand");
        area->setCaption(areaStart ? station.areaName : "");
        area->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        area->setFontHeight(14);
        area->setTextColour(MyGUI::Colour(1.0f, 0.86f, 0.54f));
        area->setNeedMouseFocus(false);
        MyGUI::TextBox* category = visible.root->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(2, STATION_AREA_BAND_HEIGHT + 1,
                STATION_COLUMN_WIDTH - 4, STATION_CATEGORY_BAND_HEIGHT - 2),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_StationCategoryBand");
        category->setCaption(
            categoryStart ? GetStationCategoryName(station.category) : "");
        category->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        category->setFontHeight(14);
        category->setTextColour(MyGUI::Colour(0.87f, 0.75f, 0.53f));
        category->setNeedMouseFocus(false);

        const int cardTop = STATION_AREA_BAND_HEIGHT + STATION_CATEGORY_BAND_HEIGHT;
        const char* iconResource = GetStationVisualIconResource(
            station.category, station.visualSubtype);
        bool iconApplied = false;
        if (iconResource != NULL && iconResource[0] != '\0')
        {
            MyGUI::ImageBox* icon = visible.root->createWidget<MyGUI::ImageBox>(
                "ImageBox", MyGUI::IntCoord(0, 0,
                    STATION_COLUMN_WIDTH, STATION_HEADER_HEIGHT),
                MyGUI::Align::Stretch, "KJM_StationCategoryBackground");
            icon->setDepth(10);
            // Render category artwork as a true background watermark: the
            // image itself is 33% opaque (67% transparent), while all header
            // text remains fully opaque above it.
            icon->setAlpha(0.33f);
            icon->setInheritsAlpha(false);
            icon->setNeedMouseFocus(false);
            iconApplied = TrySetStationCategoryIcon(icon, iconResource);
            if (!iconApplied)
            {
                icon->setVisible(false);
            }
        }

        // Keep the existing dark card treatment behind the artwork. Both the
        // image and tint cover the complete station header, including its
        // area/category bands.  The near-square header also avoids severely
        // stretching the square source artwork.  The root remains the dark
        // fallback when an icon cannot load.
        MyGUI::Widget* cardOverlay = visible.root->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(0, 0,
                STATION_COLUMN_WIDTH, STATION_HEADER_HEIGHT),
            MyGUI::Align::Stretch, "KJM_StationCardOverlay");
        cardOverlay->setColour(MyGUI::Colour(0.14f, 0.11f, 0.08f));
        cardOverlay->setAlpha(0.62f);
        // Keep this dark base behind the icon. The ImageBox now owns the
        // requested 67% transparency directly.
        cardOverlay->setDepth(11);
        cardOverlay->setNeedMouseFocus(false);

        if (!iconApplied)
        {
            MyGUI::TextBox* icon = visible.root->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText_Small",
                MyGUI::IntCoord(5, cardTop + 6,
                    STATION_COLUMN_WIDTH - 10, 34),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationCategoryGlyph");
            icon->setCaption(StationCategoryGlyph(station.category));
            icon->setDepth(8);
            icon->setFontHeight(18);
            icon->setTextAlign(MyGUI::Align::Center);
            icon->setTextColour(MyGUI::Colour(0.91f, 0.77f, 0.48f, 0.45f));
            icon->setNeedMouseFocus(false);
        }
        MyGUI::TextBox* name = visible.root->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(5, cardTop + 4, STATION_COLUMN_WIDTH - 10, 45),
            MyGUI::Align::Top | MyGUI::Align::HStretch, "KJM_StationExactName");
        SetFittedStationName(
            name, GetStationPresentationName(stationIndex));
        name->setTextAlign(MyGUI::Align::Center);
        name->setTextColour(MyGUI::Colour(1.0f, 0.96f, 0.84f));
        name->setColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
        name->setAlpha(1.0f);
        name->setInheritsAlpha(false);
        name->setNeedMouseFocus(false);
        MyGUI::TextBox* skill = visible.root->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(4, cardTop + 49, STATION_COLUMN_WIDTH - 8, 19),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_StationRelevantSkill");
        skill->setCaption(
            station.relevantSkillName.empty() ? "Relevant skill: None" :
            (std::string("Relevant skill: ") + station.relevantSkillName));
        skill->setFontHeight(13);
        skill->setTextAlign(MyGUI::Align::Center);
        skill->setTextColour(MyGUI::Colour(1.0f, 0.96f, 0.84f));
        skill->setColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
        skill->setAlpha(1.0f);
        skill->setInheritsAlpha(false);
        skill->setNeedMouseFocus(false);
        MyGUI::TextBox* status = visible.root->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(4, cardTop + 68, STATION_COLUMN_WIDTH - 8, 14),
            MyGUI::Align::Bottom | MyGUI::Align::HStretch,
            "KJM_StationBlockingStatus");
        if (!station.blockingStatus.empty())
        {
            status->setCaption(station.blockingStatus);
            status->setTextColour(MyGUI::Colour(1.0f, 0.38f, 0.27f));
        }
        else if (station.assignments.empty())
        {
            status->setCaption("UNASSIGNED");
            status->setTextColour(MyGUI::Colour(1.0f, 0.82f, 0.42f));
        }
        status->setFontHeight(10);
        status->setTextAlign(MyGUI::Align::Center);
        status->setNeedMouseFocus(false);

        std::ostringstream tooltip;
        tooltip << station.name << "\nArea: " << station.areaName
                << "\nCategory: " << GetStationCategoryName(station.category)
                << "\nRelevant skill: "
                << (station.relevantSkillName.empty() ? "None" :
                    station.relevantSkillName);
        if (!station.blockingStatus.empty())
        {
            tooltip << "\nCannot work: " << station.blockingStatus;
        }
        if (station.assignments.empty())
        {
            tooltip << "\nAssignment: UNASSIGNED";
        }
        AttachStationInput(visible.root, -1, stationIndex, tooltip.str());
        AttachStationHeaderDragInput(visible.root);
        CreateStationOutlineWidgets(
            &visible, STATION_COLUMN_WIDTH, STATION_HEADER_HEIGHT);
        ApplyStationOutline(visible);
        g_stationView.headerWidgets.push_back(visible);
    }

    std::string BuildStationAssignmentTooltip(
        const StationTargetSnapshot& station,
        const StationMemberSnapshot& worker,
        const StationAssignmentSnapshot& assignment)
    {
        std::ostringstream tooltip;
        tooltip << GetStationAssignmentWorkLabel(station, assignment)
                << "\nStation: " << station.name
                << "\nPriority: " << assignment.priority
                << "\nSquad: " << assignment.squadName;
        const std::string fullOrder = TrimStationAssignmentText(
            StripLeadingPriorityPrefix(assignment.jobLabel));
        if (!fullOrder.empty() && fullOrder !=
            GetStationAssignmentWorkLabel(station, assignment))
        {
            tooltip << "\nOrder: " << fullOrder;
        }
        if (assignment.relevantSkillKnown &&
            !station.relevantSkillName.empty())
        {
            tooltip << "\n" << station.relevantSkillName << ": "
                    << assignment.relevantSkillValue;
        }
        if (!worker.jobsEnabled)
        {
            tooltip << "\nJobs: OFF";
        }
        if (!station.blockingStatus.empty())
        {
            tooltip << "\nCannot work: " << station.blockingStatus;
        }
        return tooltip.str();
    }

    void AttachStationAssignmentCardInput(
        MyGUI::Button* card,
        int rowIndex,
        int stationIndex,
        size_t occurrence,
        const std::string& tooltip)
    {
        if (card == NULL)
        {
            return;
        }
        card->setUserString("KJM_StationRow", StationIntegerString(rowIndex));
        card->setUserString(
            "KJM_StationColumn", StationIntegerString(stationIndex));
        card->setUserString(
            "KJM_StationAssignment",
            StationIntegerString(static_cast<int>(occurrence)));
        card->eventMouseButtonPressed +=
            MyGUI::newDelegate(OnStationAssignmentPressed);
        card->eventMouseDrag +=
            MyGUI::newDelegate(OnStationAssignmentDrag);
        card->eventMouseButtonReleased +=
            MyGUI::newDelegate(OnStationAssignmentReleased);
        card->eventMouseSetFocus +=
            MyGUI::newDelegate(OnStationWidgetFocus);
        card->eventMouseLostFocus +=
            MyGUI::newDelegate(OnStationWidgetLostFocus);
        card->eventMouseWheel += MyGUI::newDelegate(OnStationMouseWheel);
        card->setUserString("KJM_ToolTip", tooltip);
        card->setNeedToolTip(true);
        card->eventToolTip += MyGUI::newDelegate(OnCardToolTip);
    }

    void CreateStationCellWidget(size_t rowIndex, int stationIndex)
    {
        if (g_stationView.snapshot == NULL || rowIndex >= g_stationView.rows.size() ||
            stationIndex < 0 ||
            stationIndex >= static_cast<int>(g_stationView.visibleStations.size()))
        {
            return;
        }
        const StationRosterRow& row = g_stationView.rows[rowIndex];
        const StationTargetSnapshot* stationPointer = GetVisibleStation(stationIndex);
        if (stationPointer == NULL)
        {
            return;
        }
        const StationTargetSnapshot& station = *stationPointer;
        const int x = stationIndex * STATION_COLUMN_STRIDE -
            g_stationView.horizontalOffset;
        const int y = row.top - g_stationView.verticalOffset;
        StationVisibleWidget visible;
        visible.rowIndex = static_cast<int>(rowIndex);
        visible.stationIndex = stationIndex;
        visible.root = g_stationView.matrixCanvas->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(x, y, STATION_COLUMN_WIDTH, row.height),
            MyGUI::Align::Left | MyGUI::Align::Top, "KJM_StationMatrixCell");
        visible.root->setColour(
            row.kind == STATION_ROSTER_SQUAD ? MyGUI::Colour(0.18f, 0.14f, 0.10f) :
            MyGUI::Colour(0.105f, 0.09f, 0.07f));
        visible.root->setAlpha(row.kind == STATION_ROSTER_MEMBER ? 0.78f : 0.60f);
        if (row.kind != STATION_ROSTER_MEMBER)
        {
            visible.root->eventMouseWheel +=
                MyGUI::newDelegate(OnStationMouseWheel);
        }

        if (row.kind == STATION_ROSTER_MEMBER)
        {
            const StationMemberSnapshot& worker =
                g_stationView.snapshot->squads[row.squadIndex].members[row.memberIndex];
            const size_t count = CountStationAssignments(station, worker.identity);
            std::ostringstream tooltip;
            for (size_t occurrence = 0; occurrence < count; ++occurrence)
            {
                const StationAssignmentSnapshot* assignment = FindStationAssignment(
                    station, worker.identity, occurrence);
                if (assignment == NULL)
                {
                    continue;
                }
                if (occurrence != 0)
                {
                    tooltip << "\n\n";
                }
                tooltip << BuildStationAssignmentTooltip(
                    station, worker, *assignment);
            }
            // Use one compact queue card per visible assignment. The exact
            // full order stays in that card's tooltip. A pathological queue
            // uses an explicit overflow row instead of silently hiding jobs.
            const size_t shownCount = std::min<size_t>(5, count);
            const bool hasOverflow = count > shownCount;
            const size_t lineCount = shownCount + (hasOverflow ? 1 : 0);
            const int assignmentTop = lineCount <= 1 ? 17 :
                (lineCount == 2 ? 11 :
                 (lineCount == 3 ? 7 :
                  (lineCount == 4 ? 4 :
                   (lineCount == 5 ? 3 : 2))));
            const int assignmentHeight = lineCount <= 1 ? 27 :
                (lineCount == 2 ? 26 :
                 (lineCount == 3 ? 24 :
                  (lineCount == 4 ? 23 :
                   (lineCount == 5 ? 21 : 18))));
            const int assignmentFont = lineCount <= 1 ? 16 :
                (lineCount == 2 ? 15 :
                 (lineCount == 3 ? 14 :
                  (lineCount == 4 ? 13 :
                   (lineCount == 5 ? 12 : 11))));
            for (size_t occurrence = 0; occurrence < shownCount; ++occurrence)
            {
                const StationAssignmentSnapshot* assignment =
                    FindStationAssignment(station, worker.identity, occurrence);
                if (assignment == NULL)
                {
                    continue;
                }
                std::ostringstream caption;
                caption << assignment->priority << "  "
                        << GetCompactStationAssignmentWorkLabel(
                            station, *assignment);
                MyGUI::Button* card = visible.root->createWidget<MyGUI::Button>(
                    "Kenshi_Button1",
                    MyGUI::IntCoord(4,
                        assignmentTop + static_cast<int>(occurrence) * assignmentHeight,
                        STATION_COLUMN_WIDTH - 8, assignmentHeight - 2),
                    MyGUI::Align::Top | MyGUI::Align::HStretch,
                    "KJM_StationAssignmentCard");
                card->setCaption(caption.str());
                card->setFontHeight(assignmentFont);
                card->setTextAlign(
                    MyGUI::Align::Left | MyGUI::Align::VCenter);
                card->setTextColour(MyGUI::Colour(1.0f, 0.95f, 0.78f));
                card->setColour(MyGUI::Colour(0.25f, 0.20f, 0.14f));
                card->setAlpha(1.0f);
                card->setInheritsAlpha(false);
                AttachStationAssignmentCardInput(
                    card, static_cast<int>(rowIndex), stationIndex,
                    occurrence,
                    BuildStationAssignmentTooltip(
                        station, worker, *assignment));
            }
            if (hasOverflow)
            {
                std::ostringstream overflowCaption;
                overflowCaption << "+" << (count - shownCount) << " more job";
                if (count - shownCount != 1)
                {
                    overflowCaption << "s";
                }
                MyGUI::TextBox* overflow =
                    visible.root->createWidget<MyGUI::TextBox>(
                        "Kenshi_TextboxStandardText_Small",
                        MyGUI::IntCoord(8,
                            assignmentTop +
                                static_cast<int>(shownCount) * assignmentHeight,
                            STATION_COLUMN_WIDTH - 14, assignmentHeight),
                        MyGUI::Align::Top | MyGUI::Align::HStretch,
                        "KJM_StationAssignmentOverflow");
                overflow->setCaption(overflowCaption.str());
                overflow->setFontHeight(assignmentFont);
                overflow->setTextAlign(
                    MyGUI::Align::Left | MyGUI::Align::VCenter);
                overflow->setTextColour(MyGUI::Colour(1.0f, 0.72f, 0.35f));
                overflow->setColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
                overflow->setAlpha(1.0f);
                overflow->setInheritsAlpha(false);
                overflow->setNeedMouseFocus(false);
            }
            if (count > 0 && lineCount <= 3)
            {
                const StationAssignmentSnapshot* first = FindStationAssignment(
                    station, worker.identity, 0);
                if (first != NULL && first->relevantSkillKnown &&
                    !station.relevantSkillName.empty())
                {
                    const int skillTop = assignmentTop +
                        static_cast<int>(shownCount) * assignmentHeight + 4;
                    const int skillHeight = std::max(
                        14, STATION_MEMBER_ROW_HEIGHT - 6 - skillTop);
                    MyGUI::TextBox* skill = visible.root->createWidget<MyGUI::TextBox>(
                        "Kenshi_TextboxStandardText_Small",
                        MyGUI::IntCoord(4, skillTop, STATION_COLUMN_WIDTH - 8,
                            skillHeight),
                        MyGUI::Align::Top | MyGUI::Align::HStretch,
                        "KJM_StationCellRelevantSkill");
                    std::ostringstream skillCaption;
                    skillCaption << station.relevantSkillName << " "
                                 << first->relevantSkillValue;
                    skill->setCaption(skillCaption.str());
                    skill->setFontHeight(shownCount == 3 ? 11 : 13);
                    skill->setTextAlign(MyGUI::Align::Center);
                    skill->setTextColour(MyGUI::Colour(0.94f, 0.88f, 0.70f));
                    skill->setColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
                    skill->setAlpha(1.0f);
                    skill->setInheritsAlpha(false);
                    skill->setNeedMouseFocus(false);
                }
            }
            AttachStationInput(
                visible.root, static_cast<int>(rowIndex), stationIndex,
                tooltip.str());
        }

        CreateStationOutlineWidgets(&visible, STATION_COLUMN_WIDTH, row.height);
        ApplyStationOutline(visible);
        g_stationView.cellWidgets.push_back(visible);
    }

    bool HasStationHeaderWidget(int stationIndex)
    {
        for (size_t index = 0; index < g_stationView.headerWidgets.size(); ++index)
        {
            if (g_stationView.headerWidgets[index].stationIndex == stationIndex)
            {
                return true;
            }
        }
        return false;
    }

    bool HasStationCellWidget(size_t rowIndex, int stationIndex)
    {
        for (size_t index = 0; index < g_stationView.cellWidgets.size(); ++index)
        {
            if (g_stationView.cellWidgets[index].rowIndex ==
                    static_cast<int>(rowIndex) &&
                g_stationView.cellWidgets[index].stationIndex == stationIndex)
            {
                return true;
            }
        }
        return false;
    }

    int FindStationMemberRow(const HandleIdentity& identity)
    {
        if (!identity.valid || g_stationView.snapshot == NULL)
        {
            return -1;
        }
        for (size_t rowIndex = 0; rowIndex < g_stationView.rows.size();
             ++rowIndex)
        {
            const StationRosterRow& row = g_stationView.rows[rowIndex];
            if (row.kind != STATION_ROSTER_MEMBER ||
                row.squadIndex >= g_stationView.snapshot->squads.size())
            {
                continue;
            }
            const StationSquadSnapshot& squad =
                g_stationView.snapshot->squads[row.squadIndex];
            if (row.memberIndex < squad.members.size() &&
                SameHandleIdentity(
                    squad.members[row.memberIndex].identity, identity))
            {
                return static_cast<int>(rowIndex);
            }
        }
        // A collapsed squad, an unavailable member, or an inactive Stations
        // tab has no member row to redraw. The patched snapshot will be used
        // the next time that row is created.
        return -1;
    }

    bool DestroyStationWidgetsForRow(
        std::vector<StationVisibleWidget>* widgets,
        int rowIndex)
    {
        if (widgets == NULL || rowIndex < 0)
        {
            return false;
        }
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
        {
            return false;
        }
        for (size_t index = widgets->size(); index > 0; --index)
        {
            const size_t itemIndex = index - 1;
            if ((*widgets)[itemIndex].rowIndex != rowIndex)
            {
                continue;
            }
            // Remove the binding before asking MyGUI to destroy the widget.
            // If MyGUI throws, the guarded caller can safely fall back to a
            // full virtual redraw without retaining a possibly destroyed
            // pointer and attempting to destroy it twice.
            MyGUI::Widget* root = (*widgets)[itemIndex].root;
            widgets->erase(widgets->begin() + itemIndex);
            if (root != NULL)
            {
                gui->destroyWidget(root);
            }
        }
        return true;
    }

    bool RefreshStationMemberRowWidgets(int rowIndex)
    {
        if (rowIndex < 0 ||
            rowIndex >= static_cast<int>(g_stationView.rows.size()))
        {
            return true;
        }
        const StationRosterRow& row = g_stationView.rows[rowIndex];
        if (row.kind != STATION_ROSTER_MEMBER)
        {
            return false;
        }

        bool rosterWasCreated = false;
        std::vector<int> stationIndexes;
        for (size_t index = 0; index < g_stationView.rosterWidgets.size();
             ++index)
        {
            if (g_stationView.rosterWidgets[index].rowIndex == rowIndex)
            {
                rosterWasCreated = true;
                break;
            }
        }
        for (size_t index = 0; index < g_stationView.cellWidgets.size();
             ++index)
        {
            if (g_stationView.cellWidgets[index].rowIndex == rowIndex)
            {
                stationIndexes.push_back(
                    g_stationView.cellWidgets[index].stationIndex);
            }
        }

        // No live widget means the row is collapsed, unavailable, off-screen,
        // or the Stations tab is hidden. The snapshot is already current, so
        // normal virtualization will create the new cards when needed.
        if (!rosterWasCreated && stationIndexes.empty())
        {
            return !IsStationRowVisible(row);
        }
        if (!rosterWasCreated)
        {
            return false;
        }

        if (!DestroyStationWidgetsForRow(
                &g_stationView.cellWidgets, rowIndex) ||
            !DestroyStationWidgetsForRow(
                &g_stationView.rosterWidgets, rowIndex))
        {
            return false;
        }

        CreateStationRosterWidget(static_cast<size_t>(rowIndex));
        for (size_t index = 0; index < stationIndexes.size(); ++index)
        {
            CreateStationCellWidget(
                static_cast<size_t>(rowIndex), stationIndexes[index]);
        }
        return true;
    }

    bool RefreshStationTransferredMemberRows(
        const HandleIdentity& source,
        const HandleIdentity& destination)
    {
        if (!source.valid || !destination.valid ||
            SameHandleIdentity(source, destination) ||
            g_stationView.headerDragArmed ||
            g_stationView.assignmentDrag.armed)
        {
            return false;
        }
        if (!g_stationView.visible || g_stationView.snapshot == NULL ||
            g_stationView.root == NULL)
        {
            g_stationView.virtualRefreshRequested = false;
            return true;
        }

        const int sourceRow = FindStationMemberRow(source);
        const int destinationRow = FindStationMemberRow(destination);
        if (g_tooltip != NULL)
        {
            g_tooltip->setVisible(false);
        }
        if (!RefreshStationMemberRowWidgets(sourceRow) ||
            !RefreshStationMemberRowWidgets(destinationRow))
        {
            return false;
        }

        // CancelStationAssignmentDrag requested a general virtual redraw after
        // release. These two rows now contain the verified projection, and no
        // other board structure changed, so that redraw is no longer needed.
        g_stationView.virtualRefreshRequested = false;
        ApplyStationOutlines();
        return true;
    }

    void EnsureStationHeaderDragBuffer()
    {
        if (g_stationView.snapshot == NULL ||
            g_stationView.visibleStations.empty())
        {
            return;
        }
        // A pointer cannot move farther than roughly one viewport in a normal
        // drag. Pre-create that range on each side, without deleting any
        // current widget, so the captured header remains valid and newly
        // exposed columns do not turn blank while the pointer moves.
        const int stationCount =
            static_cast<int>(g_stationView.visibleStations.size());
        const int bufferColumns = std::min(
            12,
            std::max(
                1, g_stationView.matrixWidth / STATION_COLUMN_STRIDE + 2));
        const int firstStation = ClampInt(
            g_stationView.horizontalOffset / STATION_COLUMN_STRIDE -
                bufferColumns,
            0, stationCount - 1);
        const int lastStation = ClampInt(
            (g_stationView.horizontalOffset + g_stationView.matrixWidth) /
                STATION_COLUMN_STRIDE + bufferColumns,
            0, stationCount - 1);

        for (int stationIndex = firstStation;
             stationIndex <= lastStation; ++stationIndex)
        {
            if (!HasStationHeaderWidget(stationIndex))
            {
                CreateStationHeaderWidget(stationIndex);
            }
        }
        for (size_t rowIndex = 0; rowIndex < g_stationView.rows.size(); ++rowIndex)
        {
            if (!IsStationRowVisible(g_stationView.rows[rowIndex]))
            {
                continue;
            }
            for (int stationIndex = firstStation;
                 stationIndex <= lastStation; ++stationIndex)
            {
                if (!HasStationCellWidget(rowIndex, stationIndex))
                {
                    CreateStationCellWidget(rowIndex, stationIndex);
                }
            }
        }
        DestroyStationColumnDividers();
        CreateStationColumnDividers(firstStation, lastStation);
        ApplyStationOutlines();
    }

    void UpdateStationScanBanner()
    {
        if (g_stationView.scanBanner == NULL || g_stationView.snapshot == NULL)
        {
            return;
        }
        const StationScanState& snapshot = *g_stationView.snapshot;
        std::ostringstream caption;
        MyGUI::Colour colour(0.95f, 0.83f, 0.55f);
        const size_t candidateCount = StationScanCandidateCount(snapshot);
        if (!snapshot.complete)
        {
            caption << "READING PLAYER STATION CANDIDATES - RESULTS INCOMPLETE  |  Candidate "
                    << snapshot.targetsCompleted << " of "
                    << candidateCount;
        }
        else if (snapshot.truncated)
        {
            caption << "PLAYER STATION RESULT LIST TRUNCATED AT 2,048 - RESULTS INCOMPLETE";
            colour = MyGUI::Colour(1.0f, 0.38f, 0.27f);
        }
        else if (snapshot.ownershipCopyTruncated)
        {
            caption << "PLAYER STATION OWNERSHIP COPY TRUNCATED AT 8,192 - RESULTS INCOMPLETE";
            colour = MyGUI::Colour(1.0f, 0.38f, 0.27f);
        }
        else if (!snapshot.errors.empty() || snapshot.targetsFailed > 0 ||
                 snapshot.rosterIncomplete)
        {
            caption << "PLAYER STATION VIEW INCOMPLETE";
            if (snapshot.targetsFailed > 0)
            {
                caption << ": " << snapshot.targetsFailed
                        << " station target(s) could not be read";
            }
            if (snapshot.rosterIncomplete)
            {
                caption << (snapshot.targetsFailed > 0 ? "  |  " : ": ")
                        << "some squad job data is unavailable";
            }
            colour = MyGUI::Colour(1.0f, 0.38f, 0.27f);
        }
        else
        {
            caption << g_stationView.visibleStations.size()
                    << " PLAYER STATIONS LOADED";
            colour = MyGUI::Colour(0.80f, 0.86f, 0.65f);
        }
        g_stationView.scanBanner->setCaption(caption.str());
        g_stationView.scanBanner->setTextColour(colour);
        std::ostringstream errorTooltip;
        for (size_t index = 0; index < snapshot.errors.size(); ++index)
        {
            if (index != 0)
            {
                errorTooltip << "\n";
            }
            errorTooltip << snapshot.errors[index];
        }
        g_stationView.scanBanner->setUserString(
            "KJM_ToolTip", errorTooltip.str());
        g_stationView.scanBanner->setNeedToolTip(!snapshot.errors.empty());
        if (g_stationView.progressTrack != NULL &&
            g_stationView.progressFill != NULL)
        {
            const int trackWidth = g_stationView.progressTrack->getWidth();
            int fillWidth = trackWidth;
            if (candidateCount != 0)
            {
                fillWidth = static_cast<int>(
                    (static_cast<double>(snapshot.targetsCompleted) /
                     static_cast<double>(candidateCount)) * trackWidth);
            }
            fillWidth = ClampInt(fillWidth, 0, trackWidth);
            g_stationView.progressFill->setSize(fillWidth,
                g_stationView.progressFill->getHeight());
            g_stationView.progressTrack->setVisible(!snapshot.complete);
        }
    }

    void UpdateStationScrollRanges()
    {
        const int contentHeight = GetStationRosterContentHeight();
        const int stationCount = g_stationView.snapshot == NULL ? 0 :
            static_cast<int>(g_stationView.visibleStations.size());
        const int contentWidth = stationCount * STATION_COLUMN_STRIDE;
        g_stationView.maxVerticalOffset = std::max(
            0, contentHeight - g_stationView.bodyHeight);
        g_stationView.maxHorizontalOffset = std::max(
            0, contentWidth - g_stationView.matrixWidth);
        g_stationView.verticalOffset = ClampInt(
            g_stationView.verticalOffset, 0, g_stationView.maxVerticalOffset);
        g_stationView.horizontalOffset = ClampInt(
            g_stationView.horizontalOffset, 0, g_stationView.maxHorizontalOffset);
        g_stationView.changingScroll = true;
        if (g_stationView.verticalScroll != NULL)
        {
            g_stationView.verticalScroll->setScrollRange(
                static_cast<size_t>(g_stationView.maxVerticalOffset + 1));
            g_stationView.verticalScroll->setScrollPage(
                static_cast<size_t>(std::max(1, g_stationView.bodyHeight)));
            g_stationView.verticalScroll->setScrollViewPage(
                static_cast<size_t>(STATION_MEMBER_ROW_HEIGHT));
            g_stationView.verticalScroll->setScrollPosition(
                static_cast<size_t>(g_stationView.verticalOffset));
            g_stationView.verticalScroll->setEnabled(
                g_stationView.maxVerticalOffset > 0);
        }
        if (g_stationView.horizontalScroll != NULL)
        {
            g_stationView.horizontalScroll->setScrollRange(
                static_cast<size_t>(g_stationView.maxHorizontalOffset + 1));
            g_stationView.horizontalScroll->setScrollPage(
                static_cast<size_t>(std::max(1, g_stationView.matrixWidth)));
            g_stationView.horizontalScroll->setScrollViewPage(
                static_cast<size_t>(STATION_COLUMN_STRIDE));
            g_stationView.horizontalScroll->setScrollPosition(
                static_cast<size_t>(g_stationView.horizontalOffset));
            g_stationView.horizontalScroll->setEnabled(
                g_stationView.maxHorizontalOffset > 0);
        }
        g_stationView.changingScroll = false;
    }

    void UpdateStationHorizontalWidgetPositions()
    {
        const int dividerX = STATION_COLUMN_WIDTH + STATION_COLUMN_GAP / 2;
        for (size_t index = 0; index < g_stationView.headerWidgets.size(); ++index)
        {
            const StationVisibleWidget& visible =
                g_stationView.headerWidgets[index];
            if (visible.root != NULL && visible.stationIndex >= 0)
            {
                visible.root->setPosition(
                    visible.stationIndex * STATION_COLUMN_STRIDE -
                        g_stationView.horizontalOffset,
                    visible.root->getTop());
            }
        }
        for (size_t index = 0; index < g_stationView.cellWidgets.size(); ++index)
        {
            const StationVisibleWidget& visible =
                g_stationView.cellWidgets[index];
            if (visible.root != NULL && visible.stationIndex >= 0)
            {
                visible.root->setPosition(
                    visible.stationIndex * STATION_COLUMN_STRIDE -
                        g_stationView.horizontalOffset,
                    visible.root->getTop());
            }
        }
        for (size_t index = 0; index < g_stationView.columnDividers.size(); ++index)
        {
            MyGUI::Widget* divider = g_stationView.columnDividers[index];
            const int stationIndex = StationParseIndex(
                divider, "KJM_StationColumn");
            if (divider != NULL && stationIndex >= 0)
            {
                divider->setPosition(
                    stationIndex * STATION_COLUMN_STRIDE -
                        g_stationView.horizontalOffset + dividerX,
                    divider->getTop());
            }
        }
    }

    void SetStationHorizontalOffset(int offset, bool requestVirtualRefresh)
    {
        g_stationView.horizontalOffset = ClampInt(
            offset, 0, g_stationView.maxHorizontalOffset);
        g_stationView.hoveredStation = -1;
        g_stationView.changingScroll = true;
        if (g_stationView.horizontalScroll != NULL)
        {
            g_stationView.horizontalScroll->setScrollPosition(
                static_cast<size_t>(g_stationView.horizontalOffset));
        }
        g_stationView.changingScroll = false;
        UpdateStationHorizontalWidgetPositions();
        ApplyStationOutlines();
        if (requestVirtualRefresh)
        {
            g_stationView.virtualRefreshRequested = true;
        }
    }

    void RefreshStationVirtualWidgets()
    {
        if (g_stationView.headerDragArmed ||
            g_stationView.assignmentDrag.armed)
        {
            g_stationView.virtualRefreshRequested = true;
            return;
        }
        DestroyStationVirtualWidgets();
        g_stationView.virtualRefreshRequested = false;
        if (!g_stationView.visible || g_stationView.snapshot == NULL ||
            g_stationView.rosterCanvas == NULL ||
            g_stationView.matrixCanvas == NULL ||
            g_stationView.headerCanvas == NULL)
        {
            return;
        }
        const int firstStation = GetFirstVisibleStationIndex();
        const int lastStation = GetLastVisibleStationIndex();
        for (int stationIndex = firstStation;
             stationIndex <= lastStation; ++stationIndex)
        {
            CreateStationHeaderWidget(stationIndex);
        }
        for (size_t rowIndex = 0; rowIndex < g_stationView.rows.size(); ++rowIndex)
        {
            if (!IsStationRowVisible(g_stationView.rows[rowIndex]))
            {
                continue;
            }
            CreateStationRosterWidget(rowIndex);
            for (int stationIndex = firstStation;
                 stationIndex <= lastStation; ++stationIndex)
            {
                CreateStationCellWidget(rowIndex, stationIndex);
            }
        }
        CreateStationColumnDividers(firstStation, lastStation);
        ApplyStationOutlines();
    }

    void RefreshStationView()
    {
        if (g_stationView.root == NULL)
        {
            return;
        }
        if (g_stationView.headerDragArmed ||
            g_stationView.assignmentDrag.armed)
        {
            g_stationView.virtualRefreshRequested = true;
            return;
        }
        BuildVisibleStationList();
        BuildStationRosterRows();
        UpdateStationScanBanner();
        UpdateStationScrollRanges();
        if (g_stationView.emptyText != NULL)
        {
            const bool empty = g_stationView.snapshot != NULL &&
                g_stationView.visibleStations.empty();
            g_stationView.emptyText->setVisible(empty);
            if (empty)
            {
                g_stationView.emptyText->setCaption(
                    g_stationView.snapshot->complete ?
                    "No player-owned station targets match the current filters." :
                    "Reading player-owned stations and exact assigned targets. Results are not complete yet.");
            }
        }
        RefreshStationVirtualWidgets();
    }

    void SetStationBoardSnapshot(const StationScanState* snapshot)
    {
        g_stationView.snapshot = snapshot;
        if (g_stationView.selectedStation.valid && snapshot != NULL)
        {
            bool stillPresent = false;
            for (size_t index = 0; index < snapshot->stations.size(); ++index)
            {
                if (SameHandleIdentity(
                        g_stationView.selectedStation,
                        snapshot->stations[index].identity))
                {
                    stillPresent = true;
                    break;
                }
            }
            if (!stillPresent)
            {
                ResetHandleIdentity(&g_stationView.selectedStation);
            }
        }
        RefreshStationView();
    }

    bool IsStationHeaderDragArmed()
    {
        return g_stationView.headerDragArmed;
    }

    bool IsStationAssignmentDragArmed()
    {
        return g_stationView.assignmentDrag.armed;
    }

    bool IsStationInteractionDragArmed()
    {
        return g_stationView.headerDragArmed ||
            g_stationView.assignmentDrag.armed;
    }

    void CancelStationHeaderDrag()
    {
        if (!g_stationView.headerDragArmed)
        {
            return;
        }
        g_stationView.headerDragArmed = false;
        g_stationView.headerDragActive = false;
        g_stationView.suppressHeaderClick = false;
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        if (input != NULL)
        {
            try
            {
                input->resetMouseCaptureWidget();
            }
            catch (...)
            {
            }
        }
        g_stationView.virtualRefreshRequested = true;
    }

    void CancelStationAssignmentDrag()
    {
        if (!g_stationView.assignmentDrag.armed)
        {
            return;
        }
        g_stationView.assignmentDrag = StationAssignmentDragState();
        g_stationView.hoveredRow = -1;
        g_stationView.hoveredStation = -1;
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        if (input != NULL)
        {
            try
            {
                input->resetMouseCaptureWidget();
            }
            catch (...)
            {
            }
        }
        ApplyStationOutlines();
        g_stationView.virtualRefreshRequested = true;
    }

    void SetStationViewVisible(bool visible)
    {
        if (!visible)
        {
            CancelStationHeaderDrag();
            CancelStationAssignmentDrag();
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
            DestroyStationVirtualWidgets();
        }
    }

    // Root-facing switch alias.  Keeping this wrapper separate lets the tab
    // controller remain independent of the view implementation.
    void SwitchStationView(bool visible)
    {
        SetStationViewVisible(visible);
    }

    // Call from the manager update after scanner work.  The snapshot address
    // may remain stable; pass snapshotChanged when its vectors or scan state
    // were changed.  Deferred virtual refresh avoids destroying a MyGUI widget
    // from inside that widget's own mouse callback.
    void TickStationView(
        const StationScanState* snapshot,
        bool snapshotChanged)
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
        if (g_stationView.headerDragArmed ||
            g_stationView.assignmentDrag.armed)
        {
            if (snapshotChanged)
            {
                g_stationView.virtualRefreshRequested = true;
            }
            return;
        }
        if (snapshotChanged)
        {
            RefreshStationView();
        }
        else if (g_stationView.virtualRefreshRequested)
        {
            RefreshStationVirtualWidgets();
        }
    }

    void CreateStationView(MyGUI::Widget* client, const MyGUI::IntCoord& bounds)
    {
        if (client == NULL || g_stationView.root != NULL)
        {
            return;
        }
        g_stationView = StationViewState();
        g_stationView.rosterWidth = ClampInt(bounds.width / 4, 280, 340);
        const int bannerHeight = 37;
        const int scrollSize = 20;
        const int matrixLeft = g_stationView.rosterWidth + 8;
        g_stationView.matrixWidth = std::max(
            STATION_COLUMN_WIDTH,
            bounds.width - matrixLeft - scrollSize);
        g_stationView.bodyHeight = std::max(
            STATION_MEMBER_ROW_HEIGHT,
            bounds.height - bannerHeight - STATION_HEADER_HEIGHT - scrollSize);
        g_stationView.root = client->createWidget<MyGUI::Widget>(
            "PanelEmpty", bounds, MyGUI::Align::Stretch, "KJM_StationsTab");
        g_stationView.root->setVisible(false);

        g_stationView.scanBanner = g_stationView.root->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText_Small",
            MyGUI::IntCoord(4, 0, bounds.width - 8, 27),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_StationScanBanner");
        g_stationView.scanBanner->setTextAlign(MyGUI::Align::Center);
        g_stationView.scanBanner->setNeedToolTip(true);
        g_stationView.scanBanner->eventToolTip +=
            MyGUI::newDelegate(OnCardToolTip);
        g_stationView.progressTrack = g_stationView.root->createWidget<MyGUI::Widget>(
            "WhiteSkin", MyGUI::IntCoord(4, 28, bounds.width - 8, 5),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_StationProgressTrack");
        g_stationView.progressTrack->setColour(MyGUI::Colour(0.20f, 0.16f, 0.12f));
        g_stationView.progressTrack->setNeedMouseFocus(false);
        g_stationView.progressFill =
            g_stationView.progressTrack->createWidget<MyGUI::Widget>(
                "WhiteSkin", MyGUI::IntCoord(0, 0, 0, 5),
                MyGUI::Align::Left | MyGUI::Align::VStretch,
                "KJM_StationProgressFill");
        g_stationView.progressFill->setColour(MyGUI::Colour(0.86f, 0.65f, 0.25f));
        g_stationView.progressFill->setNeedMouseFocus(false);

        g_stationView.rosterHeader = g_stationView.root->createWidget<MyGUI::Widget>(
            "WhiteSkin",
            MyGUI::IntCoord(0, bannerHeight, g_stationView.rosterWidth,
                STATION_HEADER_HEIGHT),
            MyGUI::Align::Left | MyGUI::Align::Top,
            "KJM_StationRosterHeader");
        g_stationView.rosterHeader->setColour(MyGUI::Colour(0.15f, 0.12f, 0.09f));
        MyGUI::TextBox* rosterCaption =
            g_stationView.rosterHeader->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText",
                MyGUI::IntCoord(8, 8, g_stationView.rosterWidth - 16,
                    STATION_HEADER_HEIGHT - 16),
                MyGUI::Align::Stretch, "KJM_StationRosterCaption");
        rosterCaption->setCaption(
            "ALL LOADED SQUADS\nPortrait  |  Name  |  Top enabled skills\nPermanent job count");
        rosterCaption->setTextAlign(MyGUI::Align::Center);
        rosterCaption->setNeedMouseFocus(false);

        g_stationView.headerViewport = g_stationView.root->createWidget<MyGUI::Widget>(
            "PanelEmpty", MyGUI::IntCoord(matrixLeft, bannerHeight,
                g_stationView.matrixWidth, STATION_HEADER_HEIGHT),
            MyGUI::Align::Top | MyGUI::Align::HStretch,
            "KJM_StationHeaderViewport");
        g_stationView.headerViewport->eventMouseWheel +=
            MyGUI::newDelegate(OnStationMouseWheel);
        AttachStationHeaderDragInput(g_stationView.headerViewport);
        g_stationView.headerCanvas =
            g_stationView.headerViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(0, 0, g_stationView.matrixWidth,
                    STATION_HEADER_HEIGHT),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationHeaderCanvas");
        AttachStationHeaderDragInput(g_stationView.headerCanvas);

        const int bodyTop = bannerHeight + STATION_HEADER_HEIGHT;
        g_stationView.rosterViewport = g_stationView.root->createWidget<MyGUI::Widget>(
            "PanelEmpty", MyGUI::IntCoord(0, bodyTop, g_stationView.rosterWidth,
                g_stationView.bodyHeight),
            MyGUI::Align::Left | MyGUI::Align::VStretch,
            "KJM_StationRosterViewport");
        g_stationView.rosterViewport->eventMouseWheel +=
            MyGUI::newDelegate(OnStationMouseWheel);
        g_stationView.rosterCanvas =
            g_stationView.rosterViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(0, 0, g_stationView.rosterWidth,
                    g_stationView.bodyHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationRosterCanvas");
        g_stationView.matrixViewport = g_stationView.root->createWidget<MyGUI::Widget>(
            "PanelEmpty", MyGUI::IntCoord(matrixLeft, bodyTop,
                g_stationView.matrixWidth, g_stationView.bodyHeight),
            MyGUI::Align::Stretch, "KJM_StationMatrixViewport");
        g_stationView.matrixViewport->eventMouseWheel +=
            MyGUI::newDelegate(OnStationMouseWheel);
        g_stationView.matrixCanvas =
            g_stationView.matrixViewport->createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(0, 0, g_stationView.matrixWidth,
                    g_stationView.bodyHeight),
                MyGUI::Align::Left | MyGUI::Align::Top,
                "KJM_StationMatrixCanvas");

        g_stationView.verticalScroll = g_stationView.root->createWidget<MyGUI::ScrollBar>(
            "Kenshi_ScrollBarV",
            MyGUI::IntCoord(matrixLeft + g_stationView.matrixWidth, bodyTop,
                scrollSize, g_stationView.bodyHeight),
            MyGUI::Align::Right | MyGUI::Align::VStretch,
            "KJM_StationVerticalScroll");
        g_stationView.verticalScroll->eventScrollChangePosition +=
            MyGUI::newDelegate(OnStationVerticalScroll);
        g_stationView.horizontalScroll =
            g_stationView.root->createWidget<MyGUI::ScrollBar>(
                "Kenshi_ScrollBarH",
                MyGUI::IntCoord(matrixLeft, bodyTop + g_stationView.bodyHeight,
                    g_stationView.matrixWidth, scrollSize),
                MyGUI::Align::Bottom | MyGUI::Align::HStretch,
                "KJM_StationHorizontalScroll");
        g_stationView.horizontalScroll->eventScrollChangePosition +=
            MyGUI::newDelegate(OnStationHorizontalScroll);
        g_stationView.emptyText = g_stationView.root->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            MyGUI::IntCoord(matrixLeft + 20, bodyTop + 40,
                g_stationView.matrixWidth - 40, 80),
            MyGUI::Align::Center, "KJM_StationEmpty");
        g_stationView.emptyText->setTextAlign(MyGUI::Align::Center);
        g_stationView.emptyText->setNeedMouseFocus(false);
        g_stationView.emptyText->setVisible(false);
    }

    void DestroyStationView()
    {
        CancelStationHeaderDrag();
        CancelStationAssignmentDrag();
        DestroyStationVirtualWidgets();
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

    void OnStationVerticalScroll(MyGUI::ScrollBar*, size_t position)
    {
        if (g_stationView.changingScroll)
        {
            return;
        }
        g_stationView.verticalOffset = ClampInt(
            static_cast<int>(position), 0, g_stationView.maxVerticalOffset);
        g_stationView.hoveredRow = -1;
        ApplyStationOutlines();
        g_stationView.virtualRefreshRequested = true;
    }

    void OnStationHorizontalScroll(MyGUI::ScrollBar*, size_t position)
    {
        if (g_stationView.changingScroll)
        {
            return;
        }
        SetStationHorizontalOffset(static_cast<int>(position), true);
    }

    void OnStationMouseWheel(MyGUI::Widget*, int relative)
    {
        if (g_stationView.assignmentDrag.armed)
        {
            return;
        }
        MyGUI::InputManager* input = MyGUI::InputManager::getInstancePtr();
        const bool horizontal = input != NULL && input->isShiftPressed();
        if (horizontal)
        {
            SetStationHorizontalOffset(
                g_stationView.horizontalOffset - relative * 48, true);
        }
        else
        {
            g_stationView.verticalOffset = ClampInt(
                g_stationView.verticalOffset - relative * 48,
                0, g_stationView.maxVerticalOffset);
        }
        if (!horizontal)
        {
            g_stationView.changingScroll = true;
            if (g_stationView.verticalScroll != NULL)
            {
                g_stationView.verticalScroll->setScrollPosition(
                    static_cast<size_t>(g_stationView.verticalOffset));
            }
            g_stationView.changingScroll = false;
        }
        g_stationView.hoveredRow = -1;
        g_stationView.hoveredStation = -1;
        ApplyStationOutlines();
        g_stationView.virtualRefreshRequested = true;
    }

    void OnStationHeaderPressed(
        MyGUI::Widget*, int, int, MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left || !g_stationView.visible)
        {
            return;
        }
        if (g_stationView.assignmentDrag.armed)
        {
            return;
        }
        const MyGUI::IntPoint mouse =
            MyGUI::InputManager::getInstance().getMousePosition();
        g_stationView.headerDragStartX = mouse.left;
        g_stationView.headerDragStartOffset =
            g_stationView.horizontalOffset;
        g_stationView.headerDragArmed = true;
        g_stationView.headerDragActive = false;
        // A new press starts a new click/drag sequence.  This also clears a
        // stale suppression flag if a previous drag ended outside a card.
        g_stationView.suppressHeaderClick = false;
        EnsureStationHeaderDragBuffer();
    }

    void OnStationHeaderDrag(
        MyGUI::Widget*, int, int, MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left ||
            !g_stationView.headerDragArmed)
        {
            return;
        }
        const MyGUI::IntPoint mouse =
            MyGUI::InputManager::getInstance().getMousePosition();
        const int delta = mouse.left - g_stationView.headerDragStartX;
        if (!g_stationView.headerDragActive)
        {
            if (std::abs(delta) < 6)
            {
                return;
            }
            g_stationView.headerDragActive = true;
            g_stationView.suppressHeaderClick = true;
        }
        // Move the content with the pointer: dragging left increases the
        // content offset, while dragging right moves back toward column zero.
        // Defer virtual widget replacement until release so MyGUI's captured
        // sender remains alive for the complete press/drag/release sequence.
        SetStationHorizontalOffset(
            g_stationView.headerDragStartOffset - delta, false);
    }

    void OnStationHeaderReleased(
        MyGUI::Widget* widget, int, int, MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left ||
            !g_stationView.headerDragArmed)
        {
            return;
        }
        const bool dragged = g_stationView.headerDragActive;
        g_stationView.headerDragArmed = false;
        g_stationView.headerDragActive = false;
        if (dragged)
        {
            // The meaningful drag must not be converted into a station
            // selection by MyGUI's subsequent click notification.  Rebuild
            // the virtual range only after release, when capture is safe.
            // Only a release from an actual header card can emit that click.
            // A canvas/viewport drag must not suppress the next real card
            // click after the pan finishes.
            g_stationView.suppressHeaderClick =
                widget != NULL &&
                widget->isUserString("KJM_StationHeaderCard");
            g_stationView.virtualRefreshRequested = true;
        }
    }

    void OnStationSquadClicked(MyGUI::Widget* widget)
    {
        if (g_stationView.snapshot == NULL)
        {
            return;
        }
        const int squadIndex = StationParseIndex(widget, "KJM_StationSquad");
        if (squadIndex < 0 ||
            squadIndex >= static_cast<int>(g_stationView.snapshot->squads.size()))
        {
            return;
        }
        ToggleStationSquadCollapsed(
            g_stationView.snapshot->squads[squadIndex].identity);
        BuildStationRosterRows();
        UpdateStationScrollRanges();
        g_stationView.virtualRefreshRequested = true;
    }

    void ToggleStationColumnSelection(int stationIndex)
    {
        if (g_stationView.snapshot == NULL || stationIndex < 0 ||
            stationIndex >= static_cast<int>(
                g_stationView.visibleStations.size()))
        {
            return;
        }
        const StationTargetSnapshot* station =
            GetVisibleStation(stationIndex);
        if (station == NULL)
        {
            return;
        }
        const HandleIdentity& identity = station->identity;
        if (g_stationView.selectedStation.valid &&
            SameHandleIdentity(g_stationView.selectedStation, identity))
        {
            ResetHandleIdentity(&g_stationView.selectedStation);
        }
        else
        {
            g_stationView.selectedStation = identity;
        }
        ApplyStationOutlines();
    }

    void OnStationColumnClicked(MyGUI::Widget* widget)
    {
        if (g_stationView.suppressHeaderClick && widget != NULL &&
            widget->isUserString("KJM_StationHeaderCard"))
        {
            g_stationView.suppressHeaderClick = false;
            return;
        }
        g_stationView.suppressHeaderClick = false;
        const int stationIndex = StationParseIndex(widget, "KJM_StationColumn");
        ToggleStationColumnSelection(stationIndex);
    }

    void OnStationAssignmentPressed(
        MyGUI::Widget* widget,
        int,
        int,
        MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left ||
            g_stationView.snapshot == NULL ||
            g_stationView.headerDragArmed ||
            g_pendingAction.type != ACTION_NONE)
        {
            return;
        }
        const int rowIndex = StationParseIndex(widget, "KJM_StationRow");
        const int stationIndex =
            StationParseIndex(widget, "KJM_StationColumn");
        const int occurrence =
            StationParseIndex(widget, "KJM_StationAssignment");
        const StationMemberSnapshot* member = NULL;
        const StationTargetSnapshot* station =
            GetVisibleStation(stationIndex);
        if (occurrence < 0 || station == NULL ||
            !TryGetStationMemberForRow(rowIndex, &member) || member == NULL ||
            !member->loaded || !member->queueAvailable || member->truncated)
        {
            SetStatus("This assignment is read-only because its queue is unavailable.");
            return;
        }
        const StationAssignmentSnapshot* assignment =
            FindStationAssignment(
                *station, member->identity,
                static_cast<size_t>(occurrence));
        if (assignment == NULL || assignment->exactJob.taskToken == 0 ||
            !assignment->exactJob.hasTarget ||
            !SameHandleIdentity(
                assignment->exactJob.target, station->identity))
        {
            SetStatus("The exact assignment could not be identified. No jobs were changed.");
            return;
        }

        g_stationView.assignmentDrag = StationAssignmentDragState();
        g_stationView.assignmentDrag.armed = true;
        g_stationView.assignmentDrag.pressPoint =
            MyGUI::InputManager::getInstance().getMousePosition();
        g_stationView.assignmentDrag.sourceMember = member->identity;
        g_stationView.assignmentDrag.stationTarget = station->identity;
        g_stationView.assignmentDrag.job = assignment->exactJob;
        CopyStationMemberQueue(
            *member, &g_stationView.assignmentDrag.sourceSequence);
        g_stationView.assignmentDrag.sourceRow = rowIndex;
        g_stationView.assignmentDrag.sourceStation = stationIndex;
    }

    void OnStationAssignmentDrag(
        MyGUI::Widget*, int, int, MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left ||
            !g_stationView.assignmentDrag.armed)
        {
            return;
        }
        const MyGUI::IntPoint mouse =
            MyGUI::InputManager::getInstance().getMousePosition();
        if (!g_stationView.assignmentDrag.active)
        {
            const int dx = std::abs(
                mouse.left - g_stationView.assignmentDrag.pressPoint.left);
            const int dy = std::abs(
                mouse.top - g_stationView.assignmentDrag.pressPoint.top);
            if (dx < DRAG_THRESHOLD && dy < DRAG_THRESHOLD)
            {
                return;
            }
            g_stationView.assignmentDrag.active = true;
            if (g_tooltip != NULL)
            {
                g_tooltip->setVisible(false);
            }
            SetStatus("Drop this job on another loaded member row.");
        }

        int destinationRow = FindStationMemberRowAtPoint(mouse);
        const StationMemberSnapshot* destination = NULL;
        if (destinationRow >= 0 &&
            TryGetStationMemberForRow(destinationRow, &destination) &&
            destination != NULL &&
            SameHandleIdentity(
                destination->identity,
                g_stationView.assignmentDrag.sourceMember))
        {
            destinationRow = -1;
        }
        if (destinationRow !=
            g_stationView.assignmentDrag.destinationRow)
        {
            g_stationView.assignmentDrag.destinationRow = destinationRow;
            ApplyStationOutlines();
        }
    }

    void OnStationAssignmentReleased(
        MyGUI::Widget*, int, int, MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left ||
            !g_stationView.assignmentDrag.armed)
        {
            return;
        }
        const StationAssignmentDragState drag =
            g_stationView.assignmentDrag;
        if (!drag.active)
        {
            ToggleStationColumnSelection(drag.sourceStation);
            CancelStationAssignmentDrag();
            return;
        }

        const MyGUI::IntPoint mouse =
            MyGUI::InputManager::getInstance().getMousePosition();
        const int destinationRow = FindStationMemberRowAtPoint(mouse);
        const StationMemberSnapshot* destination = NULL;
        if (destinationRow < 0 ||
            !TryGetStationMemberForRow(destinationRow, &destination) ||
            destination == NULL)
        {
            SetStatus("Drop cancelled. Choose another loaded member row.");
            CancelStationAssignmentDrag();
            return;
        }
        if (SameHandleIdentity(destination->identity, drag.sourceMember))
        {
            SetStatus("Choose a different member. No jobs were changed.");
            CancelStationAssignmentDrag();
            return;
        }
        if (!destination->loaded || !destination->queueAvailable ||
            destination->truncated)
        {
            SetStatus("The destination queue is unavailable. No jobs were changed.");
            CancelStationAssignmentDrag();
            return;
        }
        if (g_pendingAction.type != ACTION_NONE)
        {
            SetStatus("Another job change is pending. Try again after it finishes.");
            CancelStationAssignmentDrag();
            return;
        }

        g_pendingAction = PendingAction();
        g_pendingAction.type = ACTION_TRANSFER_STATION_JOB;
        g_pendingAction.member = drag.sourceMember;
        g_pendingAction.destinationMember = destination->identity;
        g_pendingAction.stationTarget = drag.stationTarget;
        g_pendingAction.job = drag.job;
        g_pendingAction.sequence = drag.sourceSequence;
        CopyStationMemberQueue(
            *destination, &g_pendingAction.destinationSequence);
        SetStatus("Validating both queues before moving the job...");
        CancelStationAssignmentDrag();
    }

    void OnStationWidgetFocus(MyGUI::Widget* widget, MyGUI::Widget*)
    {
        const int row = StationParseIndex(widget, "KJM_StationRow");
        const int station = StationParseIndex(widget, "KJM_StationColumn");
        if (g_stationView.hoveredRow == row &&
            g_stationView.hoveredStation == station)
        {
            return;
        }
        g_stationView.hoveredRow = row;
        g_stationView.hoveredStation = station;
        ApplyStationOutlines();
    }

    void OnStationWidgetLostFocus(MyGUI::Widget*, MyGUI::Widget*)
    {
        if (g_stationView.hoveredRow < 0 && g_stationView.hoveredStation < 0)
        {
            return;
        }
        g_stationView.hoveredRow = -1;
        g_stationView.hoveredStation = -1;
        ApplyStationOutlines();
    }
